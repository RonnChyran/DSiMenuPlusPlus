/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
		Michael "Chishm" Chisholm
		Dave "WinterMute" Murphy
		Claudio "sverx"
		Michael "mtheall" Theall

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/

#include <nds.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <gl2d.h>
#include "graphics/fontHandler.h"
#include "ndsheaderbanner.h"
#include "language.h"


#define LEFT_ALIGN 70
#define ICON_POS_X	112
#define ICON_POS_Y	96

static int BOX_PY = 11;
static int BOX_PY_spacing1 = 19;
static int BOX_PY_spacing2 = 9;
static int BOX_PY_spacing3 = 28;

// Graphic files
#include "icon_unk.h"
#include "icon_gbamode.h"
#include "icon_gba.h"
#include "icon_gb.h"
#include "icon_nes.h"

extern bool showdialogbox;
extern bool startMenu;
extern int startMenu_cursorPosition;

extern int theme;
extern bool useGbarunner;

extern bool flashcardUsed;
extern bool animateDsiIcons;

static int gbaTexID;
static int gbTexID;
static int nesTexID;
sNDSHeaderExt ndsHeader;
sNDSBannerExt ndsBanner;

#define TITLE_CACHE_SIZE 0x80

static bool infoFound[40] = {false};
static u16 cachedTitle[40][TITLE_CACHE_SIZE]; 
static char titleToDisplay[3][384]; 

static int iconTexID[6][8];
static glImage ndsIcon[6][8];

static glImage gbaIcon[1];
static glImage gbIcon[(32 / 32) * (64 / 32)];
static glImage nesIcon[1];

u8 *clearTiles;
u16 *blackPalette;
u8 *tilesModified;

void iconTitleInit()
{
	clearTiles = new u8[(32 * 256) / 2]();
	blackPalette = new u16[16*8]();
	tilesModified = new u8[(32 * 256) / 2];
}

static inline void writeBannerText(int textlines, const char* text1, const char* text2, const char* text3)
{
	reloadFonts();
	switch(textlines) {
		case 0:
		default:
			printLargeCentered(false, BOX_PY+BOX_PY_spacing1, text1);
			break;
		case 1:
			printLargeCentered(false, BOX_PY+BOX_PY_spacing2, text1);
			printLargeCentered(false, BOX_PY+BOX_PY_spacing3, text2);
			break;
		case 2:
			printLargeCentered(false, BOX_PY, text1);
			printLargeCentered(false, BOX_PY+BOX_PY_spacing1, text2);
			printLargeCentered(false, BOX_PY+BOX_PY_spacing1*2, text3);
			break;
	}
}

static void convertIconTilesToRaw(u8 *tilesSrc, u8 *tilesNew, bool twl)
{
	int PY = 32;
	if(twl) PY = 32*8;
	const int PX = 16;
	const int TILE_SIZE_Y = 8;
	const int TILE_SIZE_X = 4;
	int index = 0;
	for (int tileY = 0; tileY < PY / TILE_SIZE_Y; ++tileY)
	{
		for (int tileX = 0; tileX < PX / TILE_SIZE_X; ++tileX)
			for (int pY = 0; pY < TILE_SIZE_Y; ++pY)
				for (int pX = 0; pX < TILE_SIZE_X; ++pX)//TILE_SIZE/2 since one u8 equals two pixels (4 bit depth)
					tilesNew[pX + tileX * TILE_SIZE_X + PX * (pY + tileY * TILE_SIZE_Y)] = tilesSrc[index++];
	}
}


void loadIcon(u8 *tilesSrc, u16 *palSrc, int num, bool twl)//(u8(*tilesSrc)[(32 * 32) / 2], u16(*palSrc)[16])
{
	convertIconTilesToRaw(tilesSrc, tilesModified, twl);

	int Ysize = 32;
	int textureSizeY = TEXTURE_SIZE_32;
	if(twl) {
		Ysize = 256;
		textureSizeY = TEXTURE_SIZE_256;
	}

	for (int i = 0; i < 8; i++) {
		glDeleteTextures(1, &iconTexID[num][i]);
	}
	iconTexID[num][0] =
		glLoadTileSet(ndsIcon[num], // pointer to glImage array
					32, // sprite width
					32, // sprite height
					32, // bitmap image width
					Ysize, // bitmap image height
					GL_RGB16, // texture type for glTexImage2D() in videoGL.h
					TEXTURE_SIZE_32, // sizeX for glTexImage2D() in videoGL.h
					textureSizeY, // sizeY for glTexImage2D() in videoGL.h
					GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
					16, // Length of the palette to use (16 colors)
					(u16*) palSrc, // Image palette
					(u8*) tilesModified // Raw image data
					);
}

void loadUnkIcon(int num)
{
	for (int i = 0; i < 8; i++) {
		glDeleteTextures(1, &iconTexID[num][i]);
	}
	iconTexID[num][0] =
	glLoadTileSet(ndsIcon[num], // pointer to glImage array
				32, // sprite width
				32, // sprite height
				32, // bitmap image width
				32, // bitmap image height
				GL_RGB16, // texture type for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_32, // sizeX for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_32, // sizeY for glTexImage2D() in videoGL.h
				GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
				16, // Length of the palette to use (16 colors)
				(u16*) icon_unkPal, // Image palette
				(u8*) icon_unkBitmap // Raw image data
				);
}

void loadGBCIcon()
{
	glDeleteTextures(1, &gbTexID);
	
	gbTexID =
	glLoadTileSet(gbIcon, // pointer to glImage array
				32, // sprite width
				32, // sprite height
				32, // bitmap image width
				64, // bitmap image height
				GL_RGB16, // texture type for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_32, // sizeX for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_64, // sizeY for glTexImage2D() in videoGL.h
				GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
				16, // Length of the palette to use (16 colors)
				(u16*) icon_gbPal, // Image palette
				(u8*) icon_gbBitmap // Raw image data
				);
}
void loadNESIcon()
{
	glDeleteTextures(1, &gbaTexID);
	
	if (useGbarunner) {
		gbaTexID =
		glLoadTileSet(gbaIcon, // pointer to glImage array
					32, // sprite width
					32, // sprite height
					32, // bitmap image width
					32, // bitmap image height
					GL_RGB16, // texture type for glTexImage2D() in videoGL.h
					TEXTURE_SIZE_32, // sizeX for glTexImage2D() in videoGL.h
					TEXTURE_SIZE_32, // sizeY for glTexImage2D() in videoGL.h
					GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
					16, // Length of the palette to use (16 colors)
					(u16*) icon_gbaPal, // Image palette
					(u8*) icon_gbaBitmap // Raw image data
					);
	} else {
		gbaTexID =
		glLoadTileSet(gbaIcon, // pointer to glImage array
					32, // sprite width
					32, // sprite height
					32, // bitmap width
					32, // bitmap height
					GL_RGB16, // texture type for glTexImage2D() in videoGL.h
					TEXTURE_SIZE_32, // sizeX for glTexImage2D() in videoGL.h
					TEXTURE_SIZE_32, // sizeY for glTexImage2D() in videoGL.h
					GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT, // param for glTexImage2D() in videoGL.h
					16, // Length of the palette to use (16 colors)
					(u16*) icon_gbamodePal, // Load our 16 color tiles palette
					(u8*) icon_gbamodeBitmap // image data generated by GRIT
					);
	}

	glDeleteTextures(1, &nesTexID);
	
	nesTexID =
	glLoadTileSet(nesIcon, // pointer to glImage array
				32, // sprite width
				32, // sprite height
				32, // bitmap image width
				32, // bitmap image height
				GL_RGB16, // texture type for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_32, // sizeX for glTexImage2D() in videoGL.h
				TEXTURE_SIZE_32, // sizeY for glTexImage2D() in videoGL.h
				GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
				16, // Length of the palette to use (16 colors)
				(u16*) icon_nesPal, // Image palette
				(u8*) icon_nesBitmap // Raw image data
				);
}

static void clearIcon(int num)
{
	loadIcon(clearTiles, blackPalette, num, true);
}

void drawIcon(int Xpos, int Ypos, int num)
{
	int num2 = num;
	if(num >= 36) {
		num2 -= 36;
	} else if(num2 >= 30) {
		num2 -= 30;
	} else if(num2 >= 24) {
		num2 -= 24;
	} else if(num2 >= 18) {
		num2 -= 18;
	} else if(num2 >= 12) {
		num2 -= 12;
	} else if(num2 >= 6) {
		num2 -= 6;
	}
	//glSprite(Xpos, Ypos, bannerFlip[num], &ndsIcon[num2][bnriconPalLine[num]][bnriconframenumY[num] & 31]);
	glSprite(Xpos, Ypos, bannerFlip[num], &ndsIcon[num2][bnriconframenumY[num] & 31]);
}

void drawIconGBA(int Xpos, int Ypos)
{
	glSprite(Xpos, Ypos, GL_FLIP_NONE, gbaIcon);
}
void drawIconGB(int Xpos, int Ypos)
{
	glSprite(Xpos, Ypos, GL_FLIP_NONE, &gbIcon[0 & 31]);
}
void drawIconGBC(int Xpos, int Ypos)
{
	glSprite(Xpos, Ypos, GL_FLIP_NONE, &gbIcon[1 & 31]);
}
void drawIconNES(int Xpos, int Ypos)
{
	glSprite(Xpos, Ypos, GL_FLIP_NONE, nesIcon);
}

void loadFixedBanner(void) {
	/* Banner fixes start here */
	u32 bannersize = 0;

	// Fire Emblem - Heroes of Light and Shadow (English Translation)
	if(ndsBanner.crc[0] == 0xECF9
	&& ndsBanner.crc[1] == 0xD18F
	&& ndsBanner.crc[2] == 0xE22A
	&& ndsBanner.crc[3] == 0xD8F4)
	{
		// Use fixed banner.
		FILE* fixedBannerFile = fopen("nitro:/fixedbanners/Fire Emblem - Heroes of Light and Shadow (J) (Eng).bnr", "rb");
		bannersize = NDS_BANNER_SIZE_DSi;
		fread(&ndsBanner, 1, bannersize, fixedBannerFile);
		fclose(fixedBannerFile);
	} else // Pokemon Blaze Black (Clean Version)
	if(ndsBanner.crc[0] == 0x4683
	&& ndsBanner.crc[1] == 0x40AD
	&& ndsBanner.crc[2] == 0x5641
	&& ndsBanner.crc[3] == 0xEE5D)
	{
		// Use fixed banner.
		FILE* fixedBannerFile = fopen("nitro:/fixedbanners/Pokemon Blaze Black (Clean Version).bnr", "rb");
		bannersize = NDS_BANNER_SIZE_DSi;
		fread(&ndsBanner, 1, bannersize, fixedBannerFile);
		fclose(fixedBannerFile);
	} else // Pokemon Blaze Black (Full Version)
	if(ndsBanner.crc[0] == 0xA251
	&& ndsBanner.crc[1] == 0x40AD
	&& ndsBanner.crc[2] == 0x5641
	&& ndsBanner.crc[3] == 0xEE5D)
	{
		// Use fixed banner.
		FILE* fixedBannerFile = fopen("nitro:/fixedbanners/Pokemon Blaze Black (Full Version).bnr", "rb");
		bannersize = NDS_BANNER_SIZE_DSi;
		fread(&ndsBanner, 1, bannersize, fixedBannerFile);
		fclose(fixedBannerFile);
	} else // Pokemon Volt White (Clean Version)
	if(ndsBanner.crc[0] == 0x77F4
	&& ndsBanner.crc[1] == 0x5C94
	&& ndsBanner.crc[2] == 0xBF18
	&& ndsBanner.crc[3] == 0x0C88)
	{
		// Use fixed banner.
		FILE* fixedBannerFile = fopen("nitro:/fixedbanners/Pokemon Volt White (Clean Version).bnr", "rb");
		bannersize = NDS_BANNER_SIZE_DSi;
		fread(&ndsBanner, 1, bannersize, fixedBannerFile);
		fclose(fixedBannerFile);
	} else // Pokemon Volt White (Full Version)
	if(ndsBanner.crc[0] == 0x9CA8
	&& ndsBanner.crc[1] == 0x5C94
	&& ndsBanner.crc[2] == 0xBF18
	&& ndsBanner.crc[3] == 0x0C88)
	{
		// Use fixed banner.
		FILE* fixedBannerFile = fopen("nitro:/fixedbanners/Pokemon Volt White (Full Version).bnr", "rb");
		bannersize = NDS_BANNER_SIZE_DSi;
		fread(&ndsBanner, 1, bannersize, fixedBannerFile);
		fclose(fixedBannerFile);
	}
}

void clearTitle(int num) {
	for (int i = 0; i < TITLE_CACHE_SIZE; i++) {
		cachedTitle[num][i] = 0;
	}
}

void getGameInfo(bool isDir, const char* name, int num)
{
	bnriconPalLine[num] = 0;
	bnriconframenumY[num] = 0;
	bannerFlip[num] = GL_FLIP_NONE;
	bnriconisDSi[num] = false;
	bnrWirelessIcon[num] = 0;
	isDSiWare[num] = false;
	isHomebrew[num] = false;
	infoFound[num] = false;

	if (isDir)
	{
		clearTitle(num);
		clearBannerSequence(num);	// banner sequence
	}
	else if ((strlen(name) >= 5 && strcasecmp(name + strlen(name) - 5, ".argv") == 0)
		|| (strlen(name) >= 5 && strcasecmp(name + strlen(name) - 10, ".launcharg") == 0))
	{
		// look through the argv file for the corresponding nds file
		FILE *fp;
		char *line = NULL, *p = NULL;
		size_t size = 0;
		ssize_t rc;

		// open the argv file
		fp = fopen(name, "rb");
		if (fp == NULL)
		{
			clearTitle(num);
			clearBannerSequence(num);
			fclose(fp);
			return;
		}

		// read each line
		while ((rc = __getline(&line, &size, fp)) > 0)
		{
			// remove comments
			if ((p = strchr(line, '#')) != NULL)
				*p = 0;

			// skip leading whitespace
			for (p = line; *p && isspace((int) *p); ++p)
				;

			if (*p)
				break;
		}

		// done with the file at this point
		fclose(fp);

		if (p && *p)
		{
			// we found an argument
			struct stat st;

			// truncate everything after first argument
			strtok(p, "\n\r\t ");

			if ((strlen(p) >= 4 && strcasecmp(p + strlen(p) - 4, ".nds") == 0)
			|| (strlen(p) >= 4 && strcasecmp(p + strlen(p) - 4, ".app") == 0))
			{
				// let's see if this is a file or directory
				rc = stat(p, &st);
				if (rc != 0)
				{
					// stat failed
					clearTitle(num);
					clearBannerSequence(num);
				}
				else if (S_ISDIR(st.st_mode))
				{
					// this is a directory!
					clearTitle(num);
					clearBannerSequence(num);
				}
				else
				{
					getGameInfo(false, p, num);
				}
			}
			else
			{
				// this is not an nds/app file!
				clearTitle(num);
				clearBannerSequence(num);
			}
		}
		else
		{
			clearTitle(num);
			clearBannerSequence(num);
		}
		// clean up the allocated line
		free(line);
	}
	else if ((strlen(name) >= 4 && strcasecmp(name + strlen(name) - 4, ".nds") == 0)
			|| (strlen(name) >= 4 && strcasecmp(name + strlen(name) - 4, ".app") == 0))
	{
		// this is an nds/app file!
		FILE *fp;
		int ret;

		// open file for reading info
		fp = fopen(name, "rb");
		if (fp == NULL)
		{
			clearTitle(num);
			clearBannerSequence(num);	// banner sequence
			fclose(fp);
			return;
		}

		
		ret = fseek(fp, 0, SEEK_SET);
		if (ret == 0)
			ret = fread(&ndsHeader, sizeof (ndsHeader), 1, fp); // read if seek succeed
		else
			ret = 0; // if seek fails set to !=1

		if (ret != 1)
		{
			clearTitle(num);
			clearBannerSequence(num);
			fclose(fp);
			return;
		}

		if (ndsHeader.unitCode == 0x03 && strcmp(ndsHeader.gameCode, "####") != 0) {
			isDSiWare[num] = true;	// Make DSi-Exclusive/DSiWare game unlaunchable
		} else if (ndsHeader.unitCode == 0x02 || ndsHeader.unitCode == 0x03) {
			if(ndsHeader.arm9romOffset == 0x4000 && strcmp(ndsHeader.gameCode, "####") == 0)
				isHomebrew[num] = true;	// If homebrew has DSi-extended header,
											// do not use bootstrap/flashcard's ROM booter to boot it
		}

		if (ndsHeader.dsi_flags == 0x10) bnrWirelessIcon[num] = 1;
		else if (ndsHeader.dsi_flags == 0x0B) bnrWirelessIcon[num] = 2;

		if (ndsHeader.bannerOffset == 0)
		{
			fclose(fp);

			FILE* bannerFile = fopen("nitro:/noinfo.bnr", "rb");
			fread(&ndsBanner, 1, NDS_BANNER_SIZE_ZH_KO, bannerFile);
			fclose(bannerFile);

			for (int i = 0; i < 128; i++) {
				cachedTitle[num][i] = ndsBanner.titles[setGameLanguage][i];
			}

			return;
		}
		ret = fseek(fp, ndsHeader.bannerOffset, SEEK_SET);
		if (ret == 0)
			ret = fread(&ndsBanner, sizeof (ndsBanner), 1, fp); // read if seek succeed
		else
			ret = 0; // if seek fails set to !=1

		if (ret != 1)
		{
			// try again, but using regular banner size
			ret = fseek(fp, ndsHeader.bannerOffset, SEEK_SET);
			if (ret == 0)
				ret = fread(&ndsBanner, NDS_BANNER_SIZE_ORIGINAL, 1, fp); // read if seek succeed
			else
				ret = 0; // if seek fails set to !=1

			if (ret != 1)
			{
				fclose(fp);

				FILE* bannerFile = fopen("nitro:/noinfo.bnr", "rb");
				fread(&ndsBanner, 1, NDS_BANNER_SIZE_ZH_KO, bannerFile);
				fclose(bannerFile);

				for (int i = 0; i < TITLE_CACHE_SIZE; i++) {
					cachedTitle[num][i] = ndsBanner.titles[setGameLanguage][i];
				}

				return;
			}
		}

		// close file!
		fclose(fp);

		loadFixedBanner();

		DC_FlushAll();

		for (int i = 0; i < TITLE_CACHE_SIZE; i++) {
			cachedTitle[num][i] = ndsBanner.titles[setGameLanguage][i];
		}
		infoFound[num] = true;

		// banner sequence
		if(animateDsiIcons && ndsBanner.version == NDS_BANNER_VER_DSi) {
			grabBannerSequence(num);
			bnriconisDSi[num] = true;
		}
	}
}

void iconUpdate(bool isDir, const char* name, int num)
{
	if(num >= 36) {
		num -= 36;
	} else if(num >= 30) {
		num -= 30;
	} else if(num >= 24) {
		num -= 24;
	} else if(num >= 18) {
		num -= 18;
	} else if(num >= 12) {
		num -= 12;
	} else if(num >= 6) {
		num -= 6;
	}

	if (isDir)
	{
		// icon
		clearIcon(num);
	}
	else if ((strlen(name) >= 5 && strcasecmp(name + strlen(name) - 5, ".argv") == 0)
		|| (strlen(name) >= 5 && strcasecmp(name + strlen(name) - 10, ".launcharg") == 0))
	{
		// look through the argv file for the corresponding nds/app file
		FILE *fp;
		char *line = NULL, *p = NULL;
		size_t size = 0;
		ssize_t rc;

		// open the argv file
		fp = fopen(name, "rb");
		if (fp == NULL)
		{
			clearIcon(num);
			fclose(fp);
			return;
		}

		// read each line
		while ((rc = __getline(&line, &size, fp)) > 0)
		{
			// remove comments
			if ((p = strchr(line, '#')) != NULL)
				*p = 0;

			// skip leading whitespace
			for (p = line; *p && isspace((int) *p); ++p)
				;

			if (*p)
				break;
		}

		// done with the file at this point
		fclose(fp);

		if (p && *p)
		{
			// we found an argument
			struct stat st;

			// truncate everything after first argument
			strtok(p, "\n\r\t ");

			if ((strlen(p) >= 4 && strcasecmp(p + strlen(p) - 4, ".nds") == 0)
			|| (strlen(p) >= 4 && strcasecmp(p + strlen(p) - 4, ".app") == 0))
			{
				// let's see if this is a file or directory
				rc = stat(p, &st);
				if (rc != 0)
				{
					// stat failed
					clearIcon(num);
				}
				else if (S_ISDIR(st.st_mode))
				{
					// this is a directory!
					clearIcon(num);
				}
				else
				{
					iconUpdate(false, p, num);
				}
			}
			else
			{
				// this is not an nds/app file!
				clearIcon(num);
			}
		}
		else
		{
			clearIcon(num);
		}
		// clean up the allocated line
		free(line);
	}
	else if ((strlen(name) >= 4 && strcasecmp(name + strlen(name) - 4, ".nds") == 0)
			|| (strlen(name) >= 4 && strcasecmp(name + strlen(name) - 4, ".app") == 0))
	{
		// this is an nds/app file!
		FILE *fp;
		unsigned int iconTitleOffset;
		int ret;

		// open file for reading info
		fp = fopen(name, "rb");
		if (fp == NULL)
		{
			// icon
			clearIcon(num);
			fclose(fp);
			return;
		}

		
		ret = fseek(fp, offsetof(tNDSHeader, bannerOffset), SEEK_SET);
		if (ret == 0)
			ret = fread(&iconTitleOffset, sizeof (int), 1, fp); // read if seek succeed
		else
			ret = 0; // if seek fails set to !=1

		if (ret != 1)
		{
			// icon
			loadUnkIcon(num);
			fclose(fp);
			return;
		}

		if (iconTitleOffset == 0)
		{
			// icon
			loadUnkIcon(num);
			fclose(fp);
			return;
		}
		ret = fseek(fp, iconTitleOffset, SEEK_SET);
		if (ret == 0)
			ret = fread(&ndsBanner, sizeof (ndsBanner), 1, fp); // read if seek succeed
		else
			ret = 0; // if seek fails set to !=1

		if (ret != 1)
		{
			// try again, but using regular banner size
			ret = fseek(fp, iconTitleOffset, SEEK_SET);
			if (ret == 0)
				ret = fread(&ndsBanner, NDS_BANNER_SIZE_ORIGINAL, 1, fp); // read if seek succeed
			else
				ret = 0; // if seek fails set to !=1

			if (ret != 1)
			{
				// icon
				loadUnkIcon(num);
				fclose(fp);
				return;
			}
		}

		// close file!
		fclose(fp);

		loadFixedBanner();

		// icon
		DC_FlushAll();
		if(animateDsiIcons && ndsBanner.version == NDS_BANNER_VER_DSi) {
			loadIcon(ndsBanner.dsi_icon[0], ndsBanner.dsi_palette[0], num, true);
		} else {
			loadIcon(ndsBanner.icon, ndsBanner.palette, num, false);
		}
	}
}

static inline void writeDialogTitle(int textlines, const char* text1, const char* text2, const char* text3)
{
	switch(textlines) {
		case 0:
		default:
			printLarge(false, LEFT_ALIGN, BOX_PY+BOX_PY_spacing1, text1);
			break;
		case 1:
			printLarge(false, LEFT_ALIGN, BOX_PY+BOX_PY_spacing2, text1);
			printLarge(false, LEFT_ALIGN, BOX_PY+BOX_PY_spacing3, text2);
			break;
		case 2:
			printLarge(false, LEFT_ALIGN, BOX_PY, text1);
			printLarge(false, LEFT_ALIGN, BOX_PY+BOX_PY_spacing1, text2);
			printLarge(false, LEFT_ALIGN, BOX_PY+BOX_PY_spacing1*2, text3);
			break;
	}
}


void titleUpdate(bool isDir, const char* name, int num)
{
	clearText(false);
	if (showdialogbox) {
		BOX_PY = 10;
		BOX_PY_spacing1 = 17;
		BOX_PY_spacing2 = 7;
		BOX_PY_spacing3 = 26;
	} else if (theme == 1) {
		BOX_PY = 37;
		BOX_PY_spacing1 = 17;
		BOX_PY_spacing2 = 7;
		BOX_PY_spacing3 = 26;
	} else {
		BOX_PY = 11;
		BOX_PY_spacing1 = 19;
		BOX_PY_spacing2 = 9;
		BOX_PY_spacing3 = 28;
	}
	
	if (startMenu) {
		if (startMenu_cursorPosition == 0) {
			writeBannerText(0, "Settings", "", "");
		} else if (startMenu_cursorPosition == 1) {
			if (!flashcardUsed) {
				writeBannerText(1, "Launch Slot-1 card", "(NTR carts only)", "");
			} else {
				if (useGbarunner) {
					writeBannerText(0, "Start GBARunner2", "", "");
				} else {
					writeBannerText(0, "Start GBA Mode", "", "");
				}
			}
		} else if (startMenu_cursorPosition == 2) {
			writeBannerText(0, "Start GBARunner2", "", "");
		}
		return;
	}

	if (isDir)
	{
		// text
		if (strcmp(name, "..") == 0) {
			writeBannerText(0, "Back", "", "");
		} else {
			writeBannerText(0, name, "", "");
		}
	}
	else if (strcasecmp(name + strlen(name) - 3, ".gb") == 0 ||
				strcasecmp (name + strlen(name) - 4, ".sgb") == 0 ||
				strcasecmp (name + strlen(name) - 4, ".gbc") == 0 ||
				strcasecmp (name + strlen(name) - 4, ".nes") == 0 ||
				strcasecmp (name + strlen(name) - 4, ".fds") == 0  )
	{
		writeBannerText(0, name, "", "");
	}
	else
	{
		// this is an nds/app file!

		// turn unicode into ascii (kind of)
		// and convert 0x0A into 0x00
		int bannerlines = 0;
		// The index of the character array
		int charIndex = 0;
		for (int i = 0; i < TITLE_CACHE_SIZE; i++)
		{
			// todo: fix crash on titles that are too long (homebrew)
			if ((cachedTitle[num][i] == 0x000A) || (cachedTitle[num][i] == 0xFFFF)) {
				titleToDisplay[bannerlines][charIndex] = 0;
				bannerlines++;
				charIndex = 0;
			} else if (cachedTitle[num][i] <= 0x00FF) { // ASCII+Latin Extended Range is 0x0 to 0xFF.
				// Handle ASCII here
				titleToDisplay[bannerlines][charIndex] = cachedTitle[num][i];
				charIndex++;
			} else {
				// We need to split U16 into two characters here.
				char lowerBits = cachedTitle[num][i] & 0xFF;
 				char higherBits = cachedTitle[num][i] >> 8;
				// Since we have UTF16LE, assemble in FontGraphic the other way.
				// We will need to peek in FontGraphic.cpp since the higher bit is significant.
				titleToDisplay[bannerlines][charIndex] = UTF16_SIGNAL_BYTE; 
				// 0x0F signal bit to treat the next two characters as UTF
				titleToDisplay[bannerlines][charIndex+1] = lowerBits;
				titleToDisplay[bannerlines][charIndex+2] = higherBits;
				charIndex += 3;
			}
		}

		// text
		if (showdialogbox || infoFound[num]) {
			if (!showdialogbox) {
				writeBannerText(bannerlines, titleToDisplay[0], titleToDisplay[1], titleToDisplay[2]);
			} else {
				writeDialogTitle(bannerlines, titleToDisplay[0], titleToDisplay[1], titleToDisplay[2]);
			}
		} else {
			printLargeCentered(false, BOX_PY+BOX_PY_spacing2, name);
			printLargeCentered(false, BOX_PY+BOX_PY_spacing3, titleToDisplay[0]);
		}
		
	}
}
