
#include "ThemeTextures.h"
#include "ThemeConfig.h"

#include "common/dsimenusettings.h"
#include "common/systemdetails.h"
#include <nds.h>

#include "paletteEffects.h"
#include "themefilenames.h"
#include "tool/colortool.h"
// Graphic files
#include "../include/startborderpal.h"

#include "color.h"
#include "common/lzss.h"
#include "common/tonccpy.h"
#include "errorScreen.h"
#include "tool/stringtool.h"
#include "uvcoord_date_time_font.h"
#include "uvcoord_top_font.h"
#include "sprite.h"
// #include <nds/arm9/decompress.h>
// extern u16 bmpImageBuffer[256*192];
extern s16 usernameRendered[11];

static u16 _bmpImageBuffer[256 * 192] = {0};
static u16 _bgMainBuffer[256 * 192] = {0};
static u16 _bgSubBuffer[256 * 192] = {0};
static u16 _bgTextBuffer[256 * 192] = {0};
static Sprite _oamTextSprites[12] = {0};


ThemeTextures::ThemeTextures()
    : previouslyDrawnBottomBg(-1), bubbleTexID(0), bipsTexID(0), scrollwindowTexID(0), buttonarrowTexID(0),
      movingarrowTexID(0), launchdotTexID(0), startTexID(0), startbrdTexID(0), settingsTexID(0), braceTexID(0),
      boxfullTexID(0), boxemptyTexID(0), folderTexID(0), cornerButtonTexID(0), smallCartTexID(0), progressTexID(0),
      dialogboxTexID(0), wirelessiconTexID(0), _cachedVolumeLevel(-1), _cachedBatteryLevel(-1) {
	// Overallocation, but thats fine,
	// 0: Top, 1: Bottom, 2: Bottom Bubble, 3: Moving, 4: MovingLeft, 5: MovingRight
	_backgroundTextures.reserve(6);
}

void ThemeTextures::loadBubbleImage(const Texture &tex, int sprW, int sprH) {
	_bubbleImage = std::move(loadTexture(&bubbleTexID, tex, 1, sprW, sprH));
}

void ThemeTextures::loadProgressImage(const Texture &tex) {
	// todo: 9 palette
	_progressImage = std::move(loadTexture(&progressTexID, tex, (16 / 16) * (128 / 16), 16, 16));
}

void ThemeTextures::loadDialogboxImage(const Texture &tex) {
	_dialogboxImage = std::move(loadTexture(&dialogboxTexID, tex, (256 / 16) * (256 / 16), 16, 16));
}

void ThemeTextures::loadBipsImage(const Texture &tex) {
	_bipsImage = std::move(loadTexture(&bipsTexID, tex, (8 / 8) * (32 / 8), 8, 8));
}

void ThemeTextures::loadScrollwindowImage(const Texture &tex) {
	_scrollwindowImage = std::move(loadTexture(&scrollwindowTexID, tex, (32 / 16) * (32 / 16), 32, 32));
}

void ThemeTextures::loadButtonarrowImage(const Texture &tex) {
	_buttonarrowImage = std::move(loadTexture(&buttonarrowTexID, tex, (32 / 32) * (128 / 32), 32, 32));
}

void ThemeTextures::loadMovingarrowImage(const Texture &tex) {
	_movingarrowImage = std::move(loadTexture(&movingarrowTexID, tex, (32 / 32) * (32 / 32), 32, 32));
}

void ThemeTextures::loadLaunchdotImage(const Texture &tex) {
	_launchdotImage = std::move(loadTexture(&launchdotTexID, tex, (16 / 16) * (96 / 16), 16, 16));
}

void ThemeTextures::loadStartImage(const Texture &tex) {
	_startImage = std::move(loadTexture(&startTexID, tex, (64 / 16) * (128 / 16), 64, 16));
}

void ThemeTextures::loadStartbrdImage(const Texture &tex, int sprH) {
	int arraysize = (tex.texWidth() / tc().startBorderSpriteW()) * (tex.texHeight() / sprH);
	_startbrdImage = std::move(loadTexture(&startbrdTexID, tex, arraysize, tc().startBorderSpriteW(), sprH));
}
void ThemeTextures::loadBraceImage(const Texture &tex) {
	// todo: confirm 4 palette
	_braceImage = std::move(loadTexture(&braceTexID, tex, (16 / 16) * (128 / 16), 16, 128));
}

void ThemeTextures::loadSettingsImage(const Texture &tex) {
	_settingsImage = std::move(loadTexture(&settingsTexID, tex, (64 / 16) * (128 / 64), 64, 64));
}

void ThemeTextures::loadBoxfullImage(const Texture &tex) {
	_boxfullImage = std::move(loadTexture(&boxfullTexID, tex, (64 / 16) * (128 / 64), 64, 64));
}

void ThemeTextures::loadBoxemptyImage(const Texture &tex) {
	_boxemptyImage = std::move(loadTexture(&boxemptyTexID, tex, (64 / 16) * (64 / 16), 64, 64));
}

void ThemeTextures::loadFolderImage(const Texture &tex) {
	_folderImage = std::move(loadTexture(&folderTexID, tex, (64 / 16) * (64 / 16), 64, 64));
}

void ThemeTextures::loadCornerButtonImage(const Texture &tex, int arraysize, int sprW, int sprH) {
	_cornerButtonImage = std::move(loadTexture(&cornerButtonTexID, tex, arraysize, sprW, sprH));
}

void ThemeTextures::loadSmallCartImage(const Texture &tex) {
	_smallCartImage = std::move(loadTexture(&smallCartTexID, tex, (32 / 16) * (256 / 32), 32, 32));
}

void ThemeTextures::loadWirelessIcons(const Texture &tex) {
	_wirelessIcons = std::move(loadTexture(&wirelessiconTexID, tex, (32 / 32) * (64 / 32), 32, 32));
}

inline GL_TEXTURE_SIZE_ENUM get_tex_size(int texSize) {
	if (texSize <= 8)
		return TEXTURE_SIZE_8;
	if (texSize <= 16)
		return TEXTURE_SIZE_16;
	if (texSize <= 32)
		return TEXTURE_SIZE_32;
	if (texSize <= 64)
		return TEXTURE_SIZE_64;
	if (texSize <= 128)
		return TEXTURE_SIZE_128;
	if (texSize <= 256)
		return TEXTURE_SIZE_256;
	if (texSize <= 512)
		return TEXTURE_SIZE_512;
	return TEXTURE_SIZE_1024;
}

inline const unsigned short *apply_personal_theme(const unsigned short *palette) {
	return palette + ((PersonalData->theme) * 16);
}

unique_ptr<glImage[]> ThemeTextures::loadTexture(int *textureId, const Texture &texture, unsigned int arraySize,
						 int sprW, int sprH) {

	// We need to delete the texture since the resource held by the unique pointer will be
	// immediately dropped when we assign it to the pointer.

	u32 texW = texture.texWidth();
	u32 texH = texture.texHeight();
	u8 paletteLength = texture.paletteLength();

	if (*textureId != 0) {
		nocashMessage("Existing texture found!?");
		glDeleteTextures(1, textureId);
	}

	// Do a heap allocation of arraySize glImage
	unique_ptr<glImage[]> texturePtr = std::make_unique<glImage[]>(arraySize);

	// Load the texture here.
	*textureId = glLoadTileSet(texturePtr.get(),   // pointer to glImage array
				   sprW,	       // sprite width
				   sprH,	       // sprite height
				   texW,	       // bitmap width
				   texH,	       // bitmap height
				   GL_RGB16,	   // texture type for glTexImage2D() in videoGL.h
				   get_tex_size(texW), // sizeX for glTexImage2D() in videoGL.h
				   get_tex_size(texH), // sizeY for glTexImage2D() in videoGL.h
				   TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT, // param for glTexImage2D() in videoGL.h
				   paletteLength,	    // Length of the palette to use (16 colors)
				   (u16 *)texture.palette(), // Load our 16 color tiles palette
				   (u8 *)texture.texture()   // image data generated by GRIT
	);
	return texturePtr;
}

void ThemeTextures::reloadPalDialogBox() {
	glBindTexture(0, dialogboxTexID);
	glColorSubTableEXT(0, 0, _dialogBoxTexture->paletteLength(), 0, 0, _dialogBoxTexture->palette());
	if (ms().theme != 1) {
		glBindTexture(0, cornerButtonTexID);
		glColorSubTableEXT(0, 0, 16, 0, 0, _cornerButtonTexture->palette());
	}
}

void ThemeTextures::loadBackgrounds() {
	// 0: Top, 1: Bottom, 2: Bottom Bubble, 3: Moving, 4: MovingLeft, 5: MovingRight

	// We reuse the _topBackgroundTexture as a buffer.
	_backgroundTextures.emplace_back(TFN_BG_TOPBG, TFN_FALLBACK_BG_TOPBG);

	if (ms().theme == 1 && !sys().isRegularDS()) {
		_backgroundTextures.emplace_back(TFN_BG_BOTTOMBG, TFN_FALLBACK_BG_BOTTOMBG);
		_backgroundTextures.emplace_back(TFN_BG_BOTTOMBUBBLEBG, TFN_FALLBACK_BG_BOTTOMBUBBLEBG);
		return;
	}

	if (ms().theme == 1 && sys().isRegularDS()) {
		_backgroundTextures.emplace_back(TFN_BG_BOTTOMBG_DS, TFN_FALLBACK_BG_BOTTOMBG_DS);
		_backgroundTextures.emplace_back(TFN_BG_BOTTOMBUBBLEBG_DS, TFN_FALLBACK_BG_BOTTOMBUBBLEBG_DS);
		return;
	}
	// DSi Theme
	_backgroundTextures.emplace_back(TFN_BG_BOTTOMBG, TFN_FALLBACK_BG_BOTTOMBG);
	_backgroundTextures.emplace_back(TFN_BG_BOTTOMBUBBLEBG, TFN_FALLBACK_BG_BOTTOMBUBBLEBG);
	_backgroundTextures.emplace_back(TFN_BG_BOTTOMMOVINGBG, TFN_FALLBACK_BG_BOTTOMMOVINGBG);
}

void ThemeTextures::load3DSTheme() {

	loadBackgrounds();
	loadUITextures();

	loadVolumeTextures();
	loadBatteryTextures();

	loadIconTextures();
	loadDateFont(_dateTimeFontTexture->texture());

	_bubbleTexture = std::make_unique<Texture>(TFN_GRF_BUBBLE, TFN_FALLBACK_GRF_BUBBLE);
	_settingsIconTexture = std::make_unique<Texture>(TFN_GRF_ICON_SETTINGS, TFN_FALLBACK_GRF_ICON_SETTINGS);

	_boxFullTexture = std::make_unique<Texture>(TFN_GRF_BOX_FULL, TFN_FALLBACK_GRF_BOX_FULL);
	_boxEmptyTexture = std::make_unique<Texture>(TFN_GRF_BOX_EMPTY, TFN_FALLBACK_GRF_BOX_EMPTY);
	_folderTexture = std::make_unique<Texture>(TFN_GRF_FOLDER, TFN_FALLBACK_GRF_FOLDER);
	_progressTexture = std::make_unique<Texture>(TFN_GRF_PROGRESS, TFN_FALLBACK_GRF_PROGRESS);

	_smallCartTexture = std::make_unique<Texture>(TFN_GRF_SMALL_CART, TFN_FALLBACK_GRF_SMALL_CART);
	_startBorderTexture = std::make_unique<Texture>(TFN_GRF_CURSOR, TFN_FALLBACK_GRF_CURSOR);
	_dialogBoxTexture = std::make_unique<Texture>(TFN_GRF_DIALOGBOX, TFN_FALLBACK_GRF_DIALOGBOX);

	if (ms().colorMode == 1) {
		applyGrayscaleToAllGrfTextures();
	}

	loadBubbleImage(*_bubbleTexture, tc().bubbleTipSpriteW(), tc().bubbleTipSpriteH());
	loadSettingsImage(*_settingsIconTexture);

	loadBoxfullImage(*_boxFullTexture);
	loadBoxemptyImage(*_boxEmptyTexture);
	loadFolderImage(*_folderTexture);

	loadSmallCartImage(*_smallCartTexture);
	loadStartbrdImage(*_startBorderTexture, tc().startBorderSpriteH());
	loadDialogboxImage(*_dialogBoxTexture);
	loadProgressImage(*_progressTexture);
	loadWirelessIcons(*_wirelessIconsTexture);
}

void ThemeTextures::loadDSiTheme() {

	loadBackgrounds();
	loadUITextures();

	loadVolumeTextures();
	loadBatteryTextures();
	loadIconTextures();

	loadDateFont(_dateTimeFontTexture->texture());

	_bipsTexture = std::make_unique<Texture>(TFN_GRF_BIPS, TFN_FALLBACK_GRF_BIPS);
	_boxTexture = std::make_unique<Texture>(TFN_GRF_BOX, TFN_FALLBACK_GRF_BOX);
	_braceTexture = std::make_unique<Texture>(TFN_GRF_BRACE, TFN_FALLBACK_GRF_BRACE);
	_bubbleTexture = std::make_unique<Texture>(TFN_GRF_BUBBLE, TFN_FALLBACK_GRF_BUBBLE);
	_buttonArrowTexture = std::make_unique<Texture>(TFN_GRF_BUTTON_ARROW, TFN_FALLBACK_GRF_BUTTON_ARROW);
	_cornerButtonTexture = std::make_unique<Texture>(TFN_GRF_CORNERBUTTON, TFN_FALLBACK_GRF_CORNERBUTTON);

	_dialogBoxTexture = std::make_unique<Texture>(TFN_GRF_DIALOGBOX, TFN_FALLBACK_GRF_DIALOGBOX);

	_folderTexture = std::make_unique<Texture>(TFN_GRF_FOLDER, TFN_FALLBACK_GRF_FOLDER);
	_launchDotTexture = std::make_unique<Texture>(TFN_GRF_LAUNCH_DOT, TFN_FALLBACK_GRF_LAUNCH_DOT);
	_movingArrowTexture = std::make_unique<Texture>(TFN_GRF_MOVING_ARROW, TFN_FALLBACK_GRF_MOVING_ARROW);

	_progressTexture = std::make_unique<Texture>(TFN_GRF_PROGRESS, TFN_FALLBACK_GRF_PROGRESS);
	_scrollWindowTexture = std::make_unique<Texture>(TFN_GRF_SCROLL_WINDOW, TFN_FALLBACK_GRF_SCROLL_WINDOW);
	_smallCartTexture = std::make_unique<Texture>(TFN_GRF_SMALL_CART, TFN_FALLBACK_GRF_SMALL_CART);
	_startBorderTexture = std::make_unique<Texture>(TFN_GRF_START_BORDER, TFN_FALLBACK_GRF_START_BORDER);
	_startTextTexture = std::make_unique<Texture>(TFN_GRF_START_TEXT, TFN_FALLBACK_GRF_START_TEXT);
	_wirelessIconsTexture = std::make_unique<Texture>(TFN_GRF_WIRELESSICONS, TFN_FALLBACK_GRF_WIRELESSICONS);
	_settingsIconTexture = std::make_unique<Texture>(TFN_GRF_ICON_SETTINGS, TFN_FALLBACK_GRF_ICON_SETTINGS);

	// Apply the DSi palette shifts
	if (tc().startTextUserPalette())
		_startTextTexture->applyPaletteEffect(effectDSiStartTextPalettes);
	if (tc().startBorderUserPalette())
		_startBorderTexture->applyPaletteEffect(effectDSiStartBorderPalettes);
	if (tc().buttonArrowUserPalette())
		_buttonArrowTexture->applyPaletteEffect(effectDSiArrowButtonPalettes);
	if (tc().movingArrowUserPalette())
		_movingArrowTexture->applyPaletteEffect(effectDSiArrowButtonPalettes);
	if (tc().launchDotsUserPalette())
		_launchDotTexture->applyPaletteEffect(effectDSiArrowButtonPalettes);
	if (tc().dialogBoxUserPalette())
		_dialogBoxTexture->applyPaletteEffect(effectDSiArrowButtonPalettes);

	if (ms().colorMode == 1) {
		applyGrayscaleToAllGrfTextures();
	}

	loadBipsImage(*_bipsTexture);

	loadBubbleImage(*_bubbleTexture, tc().bubbleTipSpriteW(), tc().bubbleTipSpriteH());
	loadScrollwindowImage(*_scrollWindowTexture);
	loadWirelessIcons(*_wirelessIconsTexture);
	loadSettingsImage(*_settingsIconTexture);
	loadBraceImage(*_braceTexture);

	loadStartImage(*_startTextTexture);
	loadStartbrdImage(*_startBorderTexture, tc().startBorderSpriteH());

	loadButtonarrowImage(*_buttonArrowTexture);
	loadMovingarrowImage(*_movingArrowTexture);
	loadLaunchdotImage(*_launchDotTexture);
	loadDialogboxImage(*_dialogBoxTexture);

	// careful here, it's boxTexture, not boxFulltexture.
	loadBoxfullImage(*_boxTexture);

	loadCornerButtonImage(*_cornerButtonTexture, (32 / 16) * (32 / 32), 32, 32);
	loadSmallCartImage(*_smallCartTexture);
	loadFolderImage(*_folderTexture);

	loadProgressImage(*_progressTexture);
	loadWirelessIcons(*_wirelessIconsTexture);
}
void ThemeTextures::loadVolumeTextures() {
	if (isDSiMode()) {
		_volume0Texture = std::make_unique<Texture>(TFN_VOLUME0, TFN_FALLBACK_VOLUME0);
		_volume1Texture = std::make_unique<Texture>(TFN_VOLUME1, TFN_FALLBACK_VOLUME1);
		_volume2Texture = std::make_unique<Texture>(TFN_VOLUME2, TFN_FALLBACK_VOLUME2);
		_volume3Texture = std::make_unique<Texture>(TFN_VOLUME3, TFN_FALLBACK_VOLUME3);
		_volume4Texture = std::make_unique<Texture>(TFN_VOLUME4, TFN_FALLBACK_VOLUME4);
	}
}

void ThemeTextures::loadBatteryTextures() {
	if (isDSiMode()) {
		_batterychargeTexture = std::make_unique<Texture>(TFN_BATTERY_CHARGE, TFN_FALLBACK_BATTERY_CHARGE);
		_battery1Texture = std::make_unique<Texture>(TFN_BATTERY1, TFN_FALLBACK_BATTERY1);
		_battery2Texture = std::make_unique<Texture>(TFN_BATTERY2, TFN_FALLBACK_BATTERY2);
		_battery3Texture = std::make_unique<Texture>(TFN_BATTERY3, TFN_FALLBACK_BATTERY3);
		_battery4Texture = std::make_unique<Texture>(TFN_BATTERY4, TFN_FALLBACK_BATTERY4);
	} else {
		_batteryfullTexture = std::make_unique<Texture>(TFN_BATTERY_FULL, TFN_FALLBACK_BATTERY_FULL);
		_batteryfullDSTexture = std::make_unique<Texture>(TFN_BATTERY_FULLDS, TFN_FALLBACK_BATTERY_FULLDS);
		_batterylowTexture = std::make_unique<Texture>(TFN_BATTERY_LOW, TFN_FALLBACK_BATTERY_LOW);
	}
}

void ThemeTextures::loadUITextures() {

	_dateTimeFontTexture = std::make_unique<Texture>(TFN_UI_DATE_TIME_FONT, TFN_FALLBACK_UI_DATE_TIME_FONT);
	_leftShoulderTexture = std::make_unique<Texture>(TFN_UI_LSHOULDER, TFN_FALLBACK_UI_LSHOULDER);
	_rightShoulderTexture = std::make_unique<Texture>(TFN_UI_RSHOULDER, TFN_FALLBACK_UI_RSHOULDER);
	_leftShoulderGreyedTexture =
	    std::make_unique<Texture>(TFN_UI_LSHOULDER_GREYED, TFN_FALLBACK_UI_LSHOULDER_GREYED);
	_rightShoulderGreyedTexture =
	    std::make_unique<Texture>(TFN_UI_RSHOULDER_GREYED, TFN_FALLBACK_UI_RSHOULDER_GREYED);
}

void ThemeTextures::loadIconTextures() {
	_iconGBTexture = std::make_unique<Texture>(TFN_GRF_ICON_GB, TFN_FALLBACK_GRF_ICON_GB);
	_iconGBATexture = std::make_unique<Texture>(TFN_GRF_ICON_GBA, TFN_FALLBACK_GRF_ICON_GBA);
	_iconGBAModeTexture = std::make_unique<Texture>(TFN_GRF_ICON_GBAMODE, TFN_FALLBACK_GRF_ICON_GBAMODE);
	_iconGGTexture = std::make_unique<Texture>(TFN_GRF_ICON_GG, TFN_FALLBACK_GRF_ICON_GG);
	_iconMDTexture = std::make_unique<Texture>(TFN_GRF_ICON_MD, TFN_FALLBACK_GRF_ICON_MD);
	_iconNESTexture = std::make_unique<Texture>(TFN_GRF_ICON_NES, TFN_FALLBACK_GRF_ICON_NES);
	_iconSMSTexture = std::make_unique<Texture>(TFN_GRF_ICON_SMS, TFN_FALLBACK_GRF_ICON_SMS);
	_iconSNESTexture = std::make_unique<Texture>(TFN_GRF_ICON_SNES, TFN_FALLBACK_GRF_ICON_SNES);
	_iconUnknownTexture = std::make_unique<Texture>(TFN_GRF_ICON_UNK, TFN_FALLBACK_GRF_ICON_UNK);

	// if (ms().colorMode == 1)
	// {
	// 	_iconGBTexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconGBATexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconGBAModeTexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconGGTexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconMDTexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconNESTexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconSMSTexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconSNESTexture->applyPaletteEffect(effectGrayscalePalette);
	// 	_iconUnknownTexture->applyPaletteEffect(effectGrayscalePalette);
	// }
}
u16 *ThemeTextures::beginBgSubModify() {
	dmaCopyWords(0, BG_GFX_SUB, _bgSubBuffer, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
	return _bgSubBuffer;
}

void ThemeTextures::commitBgSubModify() {
	DC_FlushRange(_bgSubBuffer, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
	dmaCopyWords(2, _bgSubBuffer, BG_GFX_SUB, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
}

void ThemeTextures::commitBgSubModifyAsync() {
	DC_FlushRange(_bgSubBuffer, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
	dmaCopyWordsAsynch(2, _bgSubBuffer, BG_GFX_SUB, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
}

u16 *ThemeTextures::beginBgMainModify() {
	dmaCopyWords(0, BG_GFX, _bgMainBuffer, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
	return _bgMainBuffer;
}

void ThemeTextures::commitBgMainModify() {
	DC_FlushRange(_bgMainBuffer, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
	dmaCopyWords(2, _bgMainBuffer, BG_GFX, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
}

void ThemeTextures::commitBgMainModifyAsync() {
	DC_FlushRange(_bgMainBuffer, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
	dmaCopyWordsAsynch(2, _bgMainBuffer, BG_GFX, sizeof(u16) * BG_BUFFER_PIXELCOUNT);
}

void ThemeTextures::drawTopBg() {
	beginBgSubModify();
	LZ77_Decompress((u8 *)_backgroundTextures[0].texture(), (u8 *)_bgSubBuffer);
	commitBgSubModify();
}

void ThemeTextures::drawBottomBg(int index) {

	// clamp index
	if (index < 1)
		index = 1;
	if (index > 3)
		index = 3;
	if (index > 2 && ms().theme == 1)
		index = 2;
	beginBgMainModify();

	if (previouslyDrawnBottomBg != index) {
		LZ77_Decompress((u8 *)_backgroundTextures[index].texture(), (u8 *)_bgMainBuffer);
		previouslyDrawnBottomBg = index;
	} else {
		DC_FlushRange(_backgroundTextures[index].texture(), 0x18000);
		dmaCopyWords(0, _backgroundTextures[index].texture(), BG_GFX, 0x18000);
		LZ77_Decompress((u8 *)_backgroundTextures[index].texture(), (u8 *)_bgMainBuffer);
	}

	if (ms().colorMode == 1) {
		for (u16 i = 0; i < BG_BUFFER_PIXELCOUNT; i++) {
			_bgMainBuffer[i] = convertVramColorToGrayscale(_bgMainBuffer[i]);
		}
	}

	commitBgMainModify();
}

void ThemeTextures::clearTopScreen() {
	beginBgSubModify();
	u16 val = 0xFFFF;
	for (int i = 0; i < BG_BUFFER_PIXELCOUNT; i++) {
		_bgSubBuffer[i] = ((val >> 10) & 31) | (val & (31 - 3 * ms().blfLevel) << 5) |
				  (val & (31 - 6 * ms().blfLevel)) << 10 | BIT(15);
	}
	commitBgSubModify();
}

void ThemeTextures::drawProfileName() {
	// Load username
	char fontPath[64] = {0};
	FILE *file;
	int x = (isDSiMode() ? 28 : 4);

	for (int c = 0; c < 10; c++) {
		unsigned int charIndex = getTopFontSpriteIndex(usernameRendered[c]);
		// 42 characters per line.
		unsigned int texIndex = charIndex / 42;
		sprintf(fontPath, "nitro:/graphics/top_font/small_font_%u.bmp", texIndex);

		file = fopen(fontPath, "rb");

		if (file) {
			beginBgSubModify();
			// Start loading
			fseek(file, 0xe, SEEK_SET);
			u8 pixelStart = (u8)fgetc(file) + 0xe;
			fseek(file, pixelStart, SEEK_SET);
			for (int y = 15; y >= 0; y--) {
				fread(_bmpImageBuffer, 2, 0x200, file);
				u16 *src = _bmpImageBuffer + (top_font_texcoords[0 + (4 * charIndex)]);

				for (u16 i = 0; i < top_font_texcoords[2 + (4 * charIndex)]; i++) {
					u16 val = *(src++);

					// Blend with pixel
					const u16 bg =
					    _bgSubBuffer[(y + 2) * 256 + (i + x)]; // grab the background pixel
					// Apply palette here.

					// Magic numbers were found by dumping val to stdout
					// on case default.
					switch (val) {
					// #ff00ff
					case 0xFC1F:
						break;
					// #404040
					case 0xA108:
						val = alphablend(bmpPal_topSmallFont[1 + ((PersonalData->theme) * 16)],
								 bg, 224U);
						break;
					// #808080
					case 0xC210:
						// blend the colors with the background to make it look better.
						// Fills in the
						// 1 for light
						val = alphablend(bmpPal_topSmallFont[1 + ((PersonalData->theme) * 16)],
								 bg, 224U);
						break;
					// #b8b8b8
					case 0xDEF7:
						// 6 looks good on lighter themes
						// 3 do an average blend twice
						//
						val = alphablend(bmpPal_topSmallFont[3 + ((PersonalData->theme) * 16)],
								 bg, 128U);
						break;
					default:
						break;
					}
					if (val != 0xFC1F && val != 0x7C1F) { // Do not render magneta pixel
						_bgSubBuffer[(y + 2) * 256 + (i + x)] = convertToDsBmp(val);
					}
				}
			}
			x += top_font_texcoords[2 + (4 * charIndex)];
			commitBgSubModify();
		}

		fclose(file);
	}
}

unsigned short ThemeTextures::convertToDsBmp(unsigned short val) {

	int blfLevel = ms().blfLevel;
	if (ms().colorMode == 1) {
		u16 newVal = ((val >> 10) & 31) | (val & 31 << 5) | (val & 31) << 10 | BIT(15);

		u8 b, g, r, max, min;
		b = ((newVal) >> 10) & 31;
		g = ((newVal) >> 5) & 31;
		r = (newVal)&31;
		// Value decomposition of hsv
		max = (b > g) ? b : g;
		max = (max > r) ? max : r;

		// Desaturate
		min = (b < g) ? b : g;
		min = (min < r) ? min : r;
		max = (max + min) / 2;

		newVal = 32768 | (max << 10) | (max << 5) | (max);

		b = ((newVal) >> 10) & (31 - 6 * blfLevel);
		g = ((newVal) >> 5) & (31 - 3 * blfLevel);
		r = (newVal)&31;

		return 32768 | (b << 10) | (g << 5) | (r);
	} else {
		return ((val >> 10) & 31) | (val & (31 - 3 * blfLevel) << 5) | (val & (31 - 6 * blfLevel)) << 10 |
		       BIT(15);
	}
}

/**
 * Get the index in the UV coordinate array where the letter appears
 */
unsigned int ThemeTextures::getTopFontSpriteIndex(const u16 letter) {
	unsigned int spriteIndex = 0;
	long int left = 0;
	long int right = TOP_FONT_NUM_IMAGES;
	long int mid = 0;

	while (left <= right) {
		mid = left + ((right - left) / 2);
		if (top_utf16_lookup_table[mid] == letter) {
			spriteIndex = mid;
			break;
		}

		if (top_utf16_lookup_table[mid] < letter) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return spriteIndex;
}

void ThemeTextures::drawBoxArt(const char *filename) {
	FILE *file = fopen(filename, "rb");
	if (!file)
		file = fopen("nitro:/graphics/boxart_unknown.bmp", "rb");

	if (file) {
		// Start loading
		beginBgSubModify();
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		fread(_bmpImageBuffer, 2, 0x7800, file);
		u16 *src = _bmpImageBuffer;
		int x = 64;
		int y = 40 + 114;
		for (int i = 0; i < 128 * 115; i++) {
			if (x >= 64 + 128) {
				x = 64;
				y--;
			}
			u16 val = *(src++);
			_bgSubBuffer[y * 256 + x] = convertToDsBmp(val);
			x++;
		}
		commitBgSubModify();
	}
	fclose(file);
}

void ThemeTextures::drawVolumeImage(int volumeLevel) {
	if (!isDSiMode())
		return;
	beginBgSubModify();

	const Texture *tex = volumeTexture(volumeLevel);
	const u16 *src = tex->texture();
	int x = 4;
	int y = 5 + 11;
	for (u32 i = 0; i < tex->pixelCount(); i++) {
		if (x >= 4 + 18) {
			x = 4;
			y--;
		}
		u16 val = *(src++);
		if (val != 0x7C1F) { // Do not render magneta pixel
			_bgSubBuffer[y * 256 + x] = convertToDsBmp(val);
		}
		x++;
	}
	commitBgSubModify();
}

void ThemeTextures::drawVolumeImageCached() {
	int volumeLevel = getVolumeLevel();
	if (_cachedVolumeLevel != volumeLevel) {
		_cachedVolumeLevel = volumeLevel;
		drawVolumeImage(volumeLevel);
	}
}

int ThemeTextures::getVolumeLevel(void) {
	if (!isDSiMode())
		return -1;
	u8 volumeLevel = *(u8 *)(0x023FF000);
	if (volumeLevel == 0)
		return 0;
	if (volumeLevel > 0x00 && volumeLevel < 0x07)
		return 1;
	if (volumeLevel >= 0x07 && volumeLevel < 0x11)
		return 2;
	if (volumeLevel >= 0x11 && volumeLevel < 0x1C)
		return 3;
	if (volumeLevel >= 0x1C && volumeLevel < 0x20)
		return 4;
	return -1;
}

int ThemeTextures::getBatteryLevel(void) {
	u8 batteryLevel = *(u8 *)(0x023FF001);

	if (!isDSiMode()) {
		if (batteryLevel & BIT(0))
			return 1;
		return 0;
	}

	// DSi Mode
	if (batteryLevel & BIT(7))
		return 7;
	if (batteryLevel == 0xF)
		return 4;
	if (batteryLevel == 0xB)
		return 3;
	if (batteryLevel == 0x7)
		return 2;
	if (batteryLevel == 0x3 || batteryLevel == 0x1)
		return 1;
	return 0;
}

void ThemeTextures::drawBatteryImage(int batteryLevel, bool drawDSiMode, bool isRegularDS) {
	// Start loading
	beginBgSubModify();
	const Texture *tex = batteryTexture(batteryLevel, drawDSiMode, isRegularDS);
	const u16 *src = tex->texture();
	u32 x = tc().batteryRenderX();
	u32 y = tc().batteryRenderY();
	for (u32 i = 0; i < tex->pixelCount(); i++) {
		if (x >= tc().batteryRenderX() + tex->texWidth()) {
			x = tc().batteryRenderX();
			y--;
		}
		u16 val = *(src++);
		if (val != 0x7C1F) { // Do not render magneta pixel
			_bgSubBuffer[y * 256 + x] = convertToDsBmp(val);
		}
		x++;
	}
	commitBgSubModify();
}

void ThemeTextures::drawBatteryImageCached() {
	int batteryLevel = getBatteryLevel();
	if (_cachedBatteryLevel != batteryLevel) {
		_cachedBatteryLevel = batteryLevel;
		drawBatteryImage(batteryLevel, isDSiMode(), sys().isRegularDS());
	}
}

#define TOPLINES 32 * 256
#define BOTTOMOFFSET ((tc().shoulderLRenderY() - 5) * 256)
#define BOTTOMLINES ((192 - (tc().shoulderLRenderY() - 5)) * 256)
// Load .bmp file without overwriting shoulder button images or username
void ThemeTextures::drawTopBgAvoidingShoulders() {

	// Copy current to _bmpImageBuffer
	dmaCopyWords(0, BG_GFX_SUB, _bmpImageBuffer, sizeof(u16) * BG_BUFFER_PIXELCOUNT);

	// Throw the entire top background into the sub buffer.
	LZ77_Decompress((u8 *)_backgroundTextures[0].texture(), (u8 *)_bgSubBuffer);

	// Copy top 32 lines from the buffer into the sub.
	tonccpy(_bgSubBuffer, _bmpImageBuffer, sizeof(u16) * TOPLINES);

	// Copy bottom tc().shoulderLRenderY() + 5 lines into the sub
	// ((192 - 32) * 256)
	tonccpy(_bgSubBuffer + BOTTOMOFFSET, _bmpImageBuffer + BOTTOMOFFSET, sizeof(u16) * BOTTOMLINES);

	commitBgSubModify();
}

void ThemeTextures::drawShoulders(bool showLshoulder, bool showRshoulder) {

	beginBgSubModify();

	const Texture *rightTex = showRshoulder ? _rightShoulderTexture.get() : _rightShoulderGreyedTexture.get();
	const u16 *rightSrc = rightTex->texture();

	const Texture *leftTex = showLshoulder ? _leftShoulderTexture.get() : _leftShoulderGreyedTexture.get();

	const u16 *leftSrc = leftTex->texture();

	for (u32 y = rightTex->texHeight(); y > 0; y--) {
		// Draw R Shoulders
		for (u32 i = 0; i < rightTex->texWidth(); i++) {
			u16 val = *(rightSrc++);
			if (val != 0xFC1F) { // Do not render magneta pixel
				_bgSubBuffer[((y - 1) + tc().shoulderRRenderY()) * 256 +
					     (i + tc().shoulderRRenderX())] = convertToDsBmp(val);
			}
		}
	}

	for (u32 y = leftTex->texHeight(); y > 0; y--) {
		// Draw L Shoulders
		for (u32 i = 0; i < leftTex->texWidth(); i++) {
			u16 val = *(leftSrc++);
			if (val != 0xFC1F) { // Do not render magneta pixel
				_bgSubBuffer[((y - 1) + tc().shoulderLRenderY()) * 256 +
					     (i + tc().shoulderLRenderX())] = convertToDsBmp(val);
			}
		}
	}
	commitBgSubModify();
}

void ThemeTextures::loadDateFont(const unsigned short *bitmap) {
	_dateFontImage = std::make_unique<u16[]>(128 * 16);

	int x = 0;
	int y = 15;
	for (int i = 0; i < 128 * 16; i++) {
		if (x >= 128) {
			x = 0;
			y--;
		}
		u16 val = *(bitmap++);
		if (val != 0x7C1F) { // Do not render magneta pixel
			_dateFontImage[y * 128 + x] = convertToDsBmp(val);
		} else {
			_dateFontImage[y * 128 + x] = 0x7C1F;
		}
		x++;
	}
}

unsigned int ThemeTextures::getDateTimeFontSpriteIndex(const u16 letter) {
	unsigned int spriteIndex = 0;
	long int left = 0;
	long int right = DATE_TIME_FONT_NUM_IMAGES;
	long int mid = 0;

	while (left <= right) {
		mid = left + ((right - left) / 2);
		if (date_time_utf16_lookup_table[mid] == letter) {
			spriteIndex = mid;
			break;
		}

		if (date_time_utf16_lookup_table[mid] < letter) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return spriteIndex;
}

void ThemeTextures::drawDateTime(const char *str, const int posX, const int posY, const int drawCount,
				 int *hourWidthPointer) {
	int x = posX;

	beginBgSubModify();
	for (int c = 0; c < drawCount; c++) {
		int imgY = posY;

		unsigned int charIndex = getDateTimeFontSpriteIndex(str[c]);
		// Start date
		for (int y = 14; y >= 6; y--) {
			for (u16 i = 0; i < date_time_font_texcoords[2 + (4 * charIndex)]; i++) {
				if (_dateFontImage[(imgY * 128) + (date_time_font_texcoords[0 + (4 * charIndex)] +
								   i)] != 0x7C1F) { // Do not render magneta pixel
					_bgSubBuffer[y * 256 + (i + x)] =
					    _dateFontImage[(imgY * 128) +
							   (date_time_font_texcoords[0 + (4 * charIndex)] + i)];
				}
			}
			imgY--;
		}
		x += date_time_font_texcoords[2 + (4 * charIndex)];
		if (hourWidthPointer != NULL) {
			if (c == 2)
				*hourWidthPointer = x;
		}
	}
	commitBgSubModify();
}

void ThemeTextures::applyGrayscaleToAllGrfTextures() {

	if (_bipsTexture) {
		_bipsTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_boxTexture) {
		_boxTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_braceTexture) {
		_braceTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_bubbleTexture) {
		_bubbleTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_buttonArrowTexture) {
		_buttonArrowTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_cornerButtonTexture) {
		_cornerButtonTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_dialogBoxTexture) {
		_dialogBoxTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_folderTexture) {
		_folderTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_launchDotTexture) {
		_launchDotTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_movingArrowTexture) {
		_movingArrowTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_progressTexture) {
		_progressTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_scrollWindowTexture) {
		_scrollWindowTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_smallCartTexture) {
		_smallCartTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_startBorderTexture) {
		_startBorderTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_startTextTexture) {
		_startTextTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_wirelessIconsTexture) {
		_wirelessIconsTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_settingsIconTexture) {
		_settingsIconTexture->applyPaletteEffect(effectGrayscalePalette);
	}

	if (_boxFullTexture) {
		_boxFullTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_boxEmptyTexture) {
		_boxEmptyTexture->applyPaletteEffect(effectGrayscalePalette);
	}

	if (_iconGBTexture) {
		_iconGBTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconGBATexture) {
		_iconGBATexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconGBAModeTexture) {
		_iconGBAModeTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconGGTexture) {
		_iconGGTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconMDTexture) {
		_iconMDTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconNESTexture) {
		_iconNESTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconSMSTexture) {
		_iconSMSTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconSNESTexture) {
		_iconSNESTexture->applyPaletteEffect(effectGrayscalePalette);
	}
	if (_iconUnknownTexture) {
		_iconUnknownTexture->applyPaletteEffect(effectGrayscalePalette);
	}
}

u16 *ThemeTextures::bmpImageBuffer() { return _bmpImageBuffer; }

void ThemeTextures::videoSetup() {
	//////////////////////////////////////////////////////////
	videoSetMode(MODE_5_3D | DISPLAY_BG3_ACTIVE);
	videoSetModeSub(MODE_3_2D | DISPLAY_BG3_ACTIVE);

	// Initialize gl2d
	glScreen2D();
	// Make gl2d render on transparent stage.
	glClearColor(31, 31, 31, 0);
	glDisable(GL_CLEAR_BMP);

	// Clear the GL texture state
	glResetTextures();

	// Set up enough texture memory for our textures
	// Bank A is just 128kb and we are using 194 kb of
	// sprites
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_MAIN_SPRITE_0x06400000);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	vramSetBankD(VRAM_D_MAIN_BG_0x06000000);
	vramSetBankE(VRAM_E_TEX_PALETTE);
	vramSetBankF(VRAM_F_TEX_PALETTE_SLOT4);
	vramSetBankG(VRAM_G_TEX_PALETTE_SLOT5); // 16Kb of palette ram, and font textures take up 8*16 bytes.
	vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
	vramSetBankI(VRAM_I_SUB_SPRITE_EXT_PALETTE);

	//	vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE); // Not sure this does anything...
	lcdMainOnBottom();

	REG_BG3CNT = BG_MAP_BASE(0) | BG_BMP16_256x256 | BG_PRIORITY(0);
	REG_BG3X = 0;
	REG_BG3Y = 0;
	REG_BG3PA = 1 << 8;
	REG_BG3PB = 0;
	REG_BG3PC = 0;
	REG_BG3PD = 1 << 8;

	REG_BG3CNT_SUB = BG_MAP_BASE(0) | BG_BMP16_256x256 | BG_PRIORITY(0);
	REG_BG3X_SUB = 0;
	REG_BG3Y_SUB = 0;
	REG_BG3PA_SUB = 1 << 8;
	REG_BG3PB_SUB = 0;
	REG_BG3PC_SUB = 0;
	REG_BG3PD_SUB = 1 << 8;

	REG_BLDCNT = BLEND_SRC_BG3 | BLEND_FADE_BLACK;
}

void ThemeTextures::oamSetup() {

	oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
	for (int i = 0; i < 128; i++) {
		oamMain.oamMemory[i].attribute[0] = ATTR0_DISABLED;
		oamMain.oamMemory[i].attribute[1] = 0;
		oamMain.oamMemory[i].attribute[2] = 0;
		oamMain.oamMemory[i].filler = 0;
	}

	for (size_t y = 0; y < 3; ++y) {
		for (size_t x = 0; x < 4; ++x) {
			size_t index = y * 4 + x;

			_oamTextSprites[index].init(index);
			_oamTextSprites[index].setSize(SS_SIZE_64);
			_oamTextSprites[index].setPriority(0);
			_oamTextSprites[index].setBufferOffset(32 + index * 64);
			_oamTextSprites[index].setPosition(x * 64, y * 64);
			_oamTextSprites[index].show();
		}
	}
	oamEnable(&oamMain);
	oamUpdate(&oamMain);
}

#define SCREEN_PITCH (SCREEN_WIDTH + (SCREEN_WIDTH & 1))


void ThemeTextures::blitTextToOAM(){
	// memset(_bgTextBuffer + 512, 0xff, sizeof(_bgTextBuffer) - 512);
	for (size_t y = 0; y < 3; ++y) {
		for (size_t x = 0; x < 4; ++x) {
			size_t index = y * 4 + x;

			for (size_t k = 0; k < 64; ++k) {
				for (size_t l = 0; l < 64; ++l) {
					((u16 *)_oamTextSprites[index].buffer())[k * 64 + l] = 
					     _bgTextBuffer[(k + y * 64) * SCREEN_PITCH + (l + x * 64)];
				}
			}
		}
	}
	oamUpdate(&oamMain);
	

}