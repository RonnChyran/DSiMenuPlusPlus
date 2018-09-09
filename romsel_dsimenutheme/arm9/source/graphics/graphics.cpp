/*-----------------------------------------------------------------
 Copyright (C) 2015
	Matthew Scholefield

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
#include <maxmod9.h>
#include "common/gl2d.h"
#include "bios_decompress_callback.h"
#include "FontGraphic.h"

// This is use for the top font.
#include "../include/startborderpal.h"

#include "graphics/ThemeTextures.h"
#include "graphics/DrawLayer.h"
#include "queueControl.h"
#include "uvcoord_top_font.h"

#include "../iconTitle.h"
#include "graphics.h"
#include "fontHandler.h"
#include "../ndsheaderbanner.h"
#include "../language.h"
#include "../perGameSettings.h"
#include "../flashcard.h"
#include "iconHandler.h"
#define CONSOLE_SCREEN_WIDTH 32
#define CONSOLE_SCREEN_HEIGHT 24

extern u16 usernameRendered[10];

extern bool whiteScreen;
extern bool fadeType;
extern bool fadeSpeed;
extern bool controlTopBright;
extern bool controlBottomBright;
int fadeDelay = 0;

extern bool music;
static int musicTime = 0;
static bool waitBeforeMusicPlay = true;
static int waitBeforeMusicPlayTime = 0;

extern int colorRvalue;
extern int colorGvalue;
extern int colorBvalue;

extern bool dropDown;
extern bool redoDropDown;
int dropTime[5] = {0};
int dropSeq[5] = {0};
int dropSpeed[5] = {5};
int dropSpeedChange[5] = {0};
int titleboxYposDropDown[5] = {-85-80};
int allowedTitleboxForDropDown = 0;
int delayForTitleboxToDropDown = 0;
extern bool showbubble;
extern bool showSTARTborder;
extern bool isScrolling;
extern bool needToPlayStopSound;
extern int waitForNeedToPlayStopSound;

extern bool titleboxXmoveleft;
extern bool titleboxXmoveright;

extern bool applaunchprep;

int screenBrightness = 31;

int movetimer = 0;

int titleboxYmovepos = 0;

extern int spawnedtitleboxes;

extern bool startMenu;
bool boxArtQueued;

extern int theme;
extern int subtheme;
extern int cursorPosition[2];
extern int startMenu_cursorPosition;
extern int pagenum[2];
//int titleboxXmovespeed[8] = {8};
int titleboxXmovespeed[8] = {12, 10, 8, 8, 8, 8, 6, 4};
int titleboxXpos[2] = {0};
int startMenu_titleboxXpos;
int titleboxYpos = 85;	// 85, when dropped down
int titlewindowXpos[2] = {0};
int startMenu_titlewindowXpos;

bool showLshoulder = false;
bool showRshoulder = false;

int movecloseXpos = 0;

bool showProgressIcon = false;

int progressAnimNum = 0;
int progressAnimDelay = 0;

bool startBorderZoomOut = false;
int startBorderZoomAnimSeq[5] = {0, 1, 2, 1, 0};
int startBorderZoomAnimNum = 0;
int startBorderZoomAnimDelay = 0;

int launchDotX[12] = {0};
int launchDotY[12] = {0};

bool launchDotXMove[12] = {false};	// false = left, true = right
bool launchDotYMove[12] = {false};	// false = up, true = down

int launchDotFrame[12] = {0};
int launchDotCurrentChangingFrame = 0;
bool launchDotDoFrameChange = false;

bool showdialogbox = false;
float dbox_movespeed = 22;
float dbox_Ypos = -192;

int bottomBg;
int drawLayer;

int bottomBgState = 0; // 0 = Uninitialized 1 = No Bubble 2 = bubble.

int vblankRefreshCounter = 0;

int bubbleYpos = 80;
int bubbleXpos = 122;

u16 subBuffer[256 * 192];
std::string boxArtPath;

void vramcpy_ui (void* dest, const void* src, int size) 
{
	u16* destination = (u16*)dest;
	u16* source = (u16*)src;
	while (size > 0) {
		*destination++ = *source++;
		size-=2;
	}
}

extern mm_sound_effect snd_stop;
extern mm_sound_effect mus_menu;

void ClearBrightness(void) {
	fadeType = true;
	screenBrightness = 0;
	swiIntrWait(0, 1);
	swiIntrWait(0, 1);
}

// Ported from PAlib (obsolete)
void SetBrightness(u8 screen, s8 bright) {
	u16 mode = 1 << 14;

	if (bright < 0) {
		mode = 2 << 14;
		bright = -bright;
	}
	if (bright > 31) bright = 31;
	*(u16*)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

void moveIconClose(int num) {
	if (titleboxXmoveleft) {
		movecloseXpos = 0;
		if(movetimer == 1) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 1;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -2;
		} else if(movetimer == 2) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 1;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -2;
		} else if(movetimer == 3) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 2;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -3;
		} else if(movetimer == 4) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 2;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -3;
		} else if(movetimer == 5) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 3;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -4;
		} else if(movetimer == 6) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 4;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -5;
		} else if(movetimer == 7) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 5;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -6;
		} else if(movetimer == 8) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 6;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -7;
		}
	}
	if (titleboxXmoveright) {
		movecloseXpos = 0;
		if(movetimer == 1) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 2;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -1;
		} else if(movetimer == 2) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 2;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -1;
		} else if(movetimer == 3) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 3;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -2;
		} else if(movetimer == 4) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 3;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -2;
		} else if(movetimer == 5) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 4;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -3;
		} else if(movetimer == 6) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 5;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -4;
		} else if(movetimer == 7) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 6;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -5;
		} else if(movetimer == 8) {
			if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 7;
			else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -6;
		}
	}
	if(!titleboxXmoveleft || !titleboxXmoveright) {
		if (cursorPosition[secondaryDevice]-2 == num) movecloseXpos = 6;
		else if (cursorPosition[secondaryDevice]+2 == num) movecloseXpos = -6;
		else movecloseXpos = 0;
	}
}

void startMenu_moveIconClose(int num) {
	if (titleboxXmoveleft) {
		movecloseXpos = 0;
		if(movetimer == 1) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 1;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -2;
		} else if(movetimer == 2) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 1;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -2;
		} else if(movetimer == 3) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 2;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -3;
		} else if(movetimer == 4) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 2;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -3;
		} else if(movetimer == 5) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 3;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -4;
		} else if(movetimer == 6) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 4;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -5;
		} else if(movetimer == 7) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 5;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -6;
		} else if(movetimer == 8) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 6;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -7;
		}
	}
	if (titleboxXmoveright) {
		movecloseXpos = 0;
		if(movetimer == 1) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 2;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -1;
		} else if(movetimer == 2) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 2;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -1;
		} else if(movetimer == 3) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 3;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -2;
		} else if(movetimer == 4) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 3;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -2;
		} else if(movetimer == 5) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 4;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -3;
		} else if(movetimer == 6) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 5;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -4;
		} else if(movetimer == 7) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 6;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -5;
		} else if(movetimer == 8) {
			if (startMenu_cursorPosition-2 == num) movecloseXpos = 7;
			else if (startMenu_cursorPosition+2 == num) movecloseXpos = -6;
		}
	}
	if(!titleboxXmoveleft || !titleboxXmoveright) {
		if (startMenu_cursorPosition-2 == num) movecloseXpos = 6;
		else if (startMenu_cursorPosition+2 == num) movecloseXpos = -6;
		else movecloseXpos = 0;
	}
}


void bottomBgLoad(bool drawBubble, bool init = false) {
	if (init || (!drawBubble && bottomBgState == 2)) {
		tex().drawBg(bottomBg);
		// Set that we've not drawn the bubble.
		bottomBgState = 1;
	} else if (drawBubble && bottomBgState == 1){
		tex().drawBubbleBg(bottomBg);
		// Set that we've drawn the bubble.
		bottomBgState = 2;
	}
}

// No longer used.
// void drawBG(glImage *images)
// {
// 	for (int y = 0; y < 256 / 16; y++)
// 	{
// 		for (int x = 0; x < 256 / 16; x++)
// 		{
// 			int i = y * 16 + x;
// 			glSprite(x * 16, y * 16, GL_FLIP_NONE, &images[i & 255]);
// 		}
// 	}
// }

void drawBubble(const glImage *images)
{
	glSprite(bubbleXpos, bubbleYpos, GL_FLIP_NONE, &images[0]);
}

void drawDbox()
{
	for (int y = 0; y < 192 / 16; y++)
	{
		for (int x = 0; x < 256 / 16; x++)
		{
			int i = y * 16 + x;
			glSprite(x * 16, dbox_Ypos+y * 16, GL_FLIP_NONE, &tex().dialogboxImage()[i & 255]);
		}
	}
}


void reloadDboxPalette() {
	tex().reloadPalDialogBox();
}

void vBlankHandler()
{
	execQueue(); // Execute any actions queued during last vblank.
	execDeferredIconUpdates(); // Update any icons queued during last vblank.
	presentSubBg();
	drw().presentBuffer(drawLayer);
	if (theme == 1 && waitBeforeMusicPlay) {
		if (waitBeforeMusicPlayTime == 60) {
			mmEffectEx(&mus_menu);
			waitBeforeMusicPlay = false;
		} else {
			waitBeforeMusicPlayTime++;
		}
	} else {
		waitBeforeMusicPlay = false;
	}
	
	if (music && !waitBeforeMusicPlay) {
		musicTime++;
		if (musicTime == 60*49) {	// Length of music file in seconds (60*ss)
			mmEffectEx(&mus_menu);
			musicTime = 0;
		}
	}

	if (waitForNeedToPlayStopSound > 0) {
		waitForNeedToPlayStopSound++;
		if (waitForNeedToPlayStopSound == 5) {
			waitForNeedToPlayStopSound = 0;
		}
		needToPlayStopSound = false;
	}

	glBegin2D();
	{
		if(fadeType == true) {
			if(!fadeDelay) {
				screenBrightness--;
				if (screenBrightness < 0) screenBrightness = 0;
			}
			if (!fadeSpeed) {
				fadeDelay++;
				if (fadeDelay == 3) fadeDelay = 0;
			} else {
				fadeDelay = 0;
			}
		} else {
			if(!fadeDelay) {
				screenBrightness++;
				if (screenBrightness > 31) screenBrightness = 31;
			}
			if (!fadeSpeed) {
				fadeDelay++;
				if (fadeDelay == 3) fadeDelay = 0;
			} else {
				fadeDelay = 0;
			}
		}
		if (controlBottomBright) SetBrightness(0, screenBrightness);
		if (controlTopBright) SetBrightness(1, screenBrightness);

		if (showdialogbox) {
			// Dialogbox moving up...
			if (dbox_movespeed <= 1) {
				if (dbox_Ypos >= 0) {
					// dbox stopped
					dbox_movespeed = 0;
					dbox_Ypos = 0;
				} else {
					// dbox moving up
					dbox_movespeed = 1;
				}
			} else {
				// Dbox decel
				dbox_movespeed -= 1.25;
			}
			dbox_Ypos += dbox_movespeed;
		} else {
			// Dialogbox moving down...
			if (dbox_Ypos <= -192 || dbox_Ypos >= 192) {
				dbox_movespeed = 22;
				dbox_Ypos = -192;
			} else {
				dbox_movespeed += 1;
				dbox_Ypos += dbox_movespeed;
			}
		}

		if (titleboxXmoveleft) {
			if(startMenu) {
				if (movetimer == 8) {
				//	if (showbubble && theme == 0) mmEffectEx(&snd_stop);
					needToPlayStopSound = true;
					startBorderZoomOut = true;
					startMenu_titlewindowXpos -= 1;
					movetimer++;
				} else if (movetimer < 8) {
					startMenu_titleboxXpos -= titleboxXmovespeed[movetimer];
					if(movetimer==0 || movetimer==2 || movetimer==4 || movetimer==6 ) startMenu_titlewindowXpos -= 1;
					movetimer++;
				} else {
					titleboxXmoveleft = false;
					movetimer = 0;
				}
			} else {
				if (movetimer == 8) {
				//	if (showbubble && theme == 0) mmEffectEx(&snd_stop);
					needToPlayStopSound = true;
					startBorderZoomOut = true;
					titlewindowXpos[secondaryDevice] -= 1;
					movetimer++;
				} else if (movetimer < 8) {
					titleboxXpos[secondaryDevice] -= titleboxXmovespeed[movetimer];
					if(movetimer==0 || movetimer==2 || movetimer==4 || movetimer==6 ) titlewindowXpos[secondaryDevice] -= 1;
					movetimer++;
				} else {
					titleboxXmoveleft = false;
					movetimer = 0;
				}
			}
		} else if (titleboxXmoveright) {
			if(startMenu) {
				if (movetimer == 8) {
					// if (showbubble && theme == 0) mmEffectEx(&snd_stop);
					needToPlayStopSound = true;
					startBorderZoomOut = true;
					startMenu_titlewindowXpos += 1;
					movetimer++;
				} else if (movetimer < 8) {
					startMenu_titleboxXpos += titleboxXmovespeed[movetimer];
					if(movetimer==0 || movetimer==2 || movetimer==4 || movetimer==6 ) startMenu_titlewindowXpos += 1;
					movetimer++;
				} else {
					titleboxXmoveright = false;
					movetimer = 0;
				}
			} else {
				if (movetimer == 8) {
				//	if (showbubble && theme == 0) mmEffectEx(&snd_stop);
					needToPlayStopSound = true;
					startBorderZoomOut = true;
					titlewindowXpos[secondaryDevice] += 1;
					movetimer++;
				} else if (movetimer < 8) {
					titleboxXpos[secondaryDevice] += titleboxXmovespeed[movetimer];
					if(movetimer==0 || movetimer==2 || movetimer==4 || movetimer==6 ) titlewindowXpos[secondaryDevice] += 1;
					movetimer++;
				} else {
					titleboxXmoveright = false;
					movetimer = 0;
				}
			}
		}

		if (redoDropDown && theme == 0) {
			for (int i = 0; i < 5; i++) {
				dropTime[i] = 0;
				dropSeq[i] = 0;
				dropSpeed[i] = 5;
				dropSpeedChange[i] = 0;
				titleboxYposDropDown[i] = -85-80;
			}
			allowedTitleboxForDropDown = 0;
			delayForTitleboxToDropDown = 0;
			dropDown = false;
			redoDropDown = false;
		}

		if (!whiteScreen && dropDown && theme == 0) {
			for (int i = 0; i <= allowedTitleboxForDropDown; i++) {
				if (dropSeq[i] == 0) {
					titleboxYposDropDown[i] += dropSpeed[i];
					if (titleboxYposDropDown[i] > 0) dropSeq[i] = 1;
				} else if (dropSeq[i] == 1) {
					titleboxYposDropDown[i] -= dropSpeed[i];
					dropTime[i]++;
					dropSpeedChange[i]++;
					if (dropTime[i] >= 15) {
						dropSpeedChange[i] = -1;
						dropSeq[i] = 2;
					}
					if (dropSpeedChange[i] == 2) {
						dropSpeed[i]--;
						if (dropSpeed[i] < 0) dropSpeed[i] = 0;
						dropSpeedChange[i] = -1;
					}
				} else if (dropSeq[i] == 2) {
					titleboxYposDropDown[i] += dropSpeed[i];
					if (titleboxYposDropDown[i] >= 0) {
						dropSeq[i] = 3;
						titleboxYposDropDown[i] = 0;
					}
					dropSpeedChange[i]++;
					if (dropSpeedChange[i] == 1) {
						dropSpeed[i]++;
						if (dropSpeed[i] > 4) dropSpeed[i] = 4;
						dropSpeedChange[i] = -1;
					}
				} else if (dropSeq[i] == 3) {
					titleboxYposDropDown[i] = 0;
				}
			}

			delayForTitleboxToDropDown++;
			if (delayForTitleboxToDropDown >= 5) {
				allowedTitleboxForDropDown++;
				if (allowedTitleboxForDropDown > 4) allowedTitleboxForDropDown = 4;
				delayForTitleboxToDropDown = 0;
			}
		}

		//if (renderingTop)
		//{
			/*glBoxFilledGradient(0, -64, 256, 112,
						  RGB15(colorRvalue,colorGvalue,colorBvalue), RGB15(0,0,0), RGB15(0,0,0), RGB15(colorRvalue,colorGvalue,colorBvalue)
						);
			glBoxFilledGradient(0, 112, 256, 192,
						  RGB15(0,0,0), RGB15(colorRvalue,colorGvalue,colorBvalue), RGB15(colorRvalue,colorGvalue,colorBvalue), RGB15(0,0,0)
						);
			drawBG(mainBgImage);
			glSprite(-2, 172, GL_FLIP_NONE, &shoulderImage[0 & 31]);
			glSprite(178, 172, GL_FLIP_NONE, &shoulderImage[1 & 31]);
			if (whiteScreen) glBoxFilled(0, 0, 256, 192, RGB15(31, 31, 31));
			updateText(renderingTop);
			glColor(RGB15(31, 31, 31));*/
		//}
		//else
		//{


		//	drawBG(subBgImage);
		//	if (!showbubble && theme==0) glSprite(0, 29, GL_FLIP_NONE, ndsimenutextImage);

			if (theme==0) {
				glColor(RGB15(31, 31, 31));
				int bipXpos = 27;
				if(startMenu) {
					glSprite(16+startMenu_titlewindowXpos, 171, GL_FLIP_NONE, tex().scrollwindowImage());
					for(int i = 0; i < 40; i++) {
						if (i == 0) glSprite(bipXpos, 178, GL_FLIP_NONE, &tex().bipsImage()[2 & 31]);
						else if (i == 1) glSprite(bipXpos, 178, GL_FLIP_NONE, tex().bipsImage());
						else glSprite(bipXpos, 178, GL_FLIP_NONE, &tex().bipsImage()[1 & 31]);
						bipXpos += 5;
					}
					glSprite(16+startMenu_titlewindowXpos, 171, GL_FLIP_NONE, &tex().buttonarrowImage()[1]);
				} else {
					glSprite(16+titlewindowXpos[secondaryDevice], 171, GL_FLIP_NONE, tex().scrollwindowImage());
					for(int i = 0; i < 40; i++) {
						if (i < spawnedtitleboxes) glSprite(bipXpos, 178, GL_FLIP_NONE, tex().bipsImage());
						else glSprite(bipXpos, 178, GL_FLIP_NONE, &tex().bipsImage()[1 & 31]);
						bipXpos += 5;
					}
					glSprite(16+titlewindowXpos[secondaryDevice], 171, GL_FLIP_NONE, &tex().buttonarrowImage()[1]);
				}
				glSprite(0, 171, GL_FLIP_NONE, &tex().buttonarrowImage()[0]);
				glSprite(224, 171, GL_FLIP_H, &tex().buttonarrowImage()[0]);
				glColor(RGB15(31, 31, 31));
				if (startMenu) {
					glSprite(72-startMenu_titleboxXpos, 81, GL_FLIP_NONE, tex().braceImage());
				} else {
					glSprite(72-titleboxXpos[secondaryDevice], 81, GL_FLIP_NONE, tex().braceImage());
				}
			}
			int spawnedboxXpos = 96;
			int iconXpos = 112;
			if(startMenu) {
				for(int i = 0; i < 40; i++) {
					if (theme == 0) {
						startMenu_moveIconClose(i);
					} else {
						movecloseXpos = 0;
					}
					if (i == 0 &&  (!applaunchprep || startMenu_cursorPosition != i)) {
						glSprite(spawnedboxXpos-startMenu_titleboxXpos+movecloseXpos, (titleboxYpos-1)+titleboxYposDropDown[i % 5], GL_FLIP_NONE, &tex().settingsImage()[1 & 63]);
					} else if (i == 1 && (!applaunchprep || startMenu_cursorPosition != i)) {
						if (!flashcardFound()) {
							glSprite(spawnedboxXpos-startMenu_titleboxXpos+movecloseXpos, titleboxYpos+titleboxYposDropDown[i % 5], GL_FLIP_NONE, &tex().settingsImage()[0 & 63]);
						} else {
							if (theme == 1) glSprite(spawnedboxXpos-startMenu_titleboxXpos+movecloseXpos, titleboxYpos, GL_FLIP_NONE, tex().boxfullImage());
							else glSprite(spawnedboxXpos-startMenu_titleboxXpos+movecloseXpos, (titleboxYpos)+titleboxYposDropDown[i % 5], GL_FLIP_NONE, &tex().boxfullImage()[0 & 63]);
							drawIconGBA(iconXpos-startMenu_titleboxXpos+movecloseXpos, (titleboxYpos+12)+titleboxYposDropDown[i % 5]);
						}
					} else if (i == 2 && !flashcardFound() && (!applaunchprep || startMenu_cursorPosition != i)) {
						if (theme == 1) glSprite(spawnedboxXpos-startMenu_titleboxXpos+movecloseXpos, titleboxYpos, GL_FLIP_NONE, tex().boxfullImage());
						else glSprite(spawnedboxXpos-startMenu_titleboxXpos+movecloseXpos, titleboxYpos+titleboxYposDropDown[i % 5], GL_FLIP_NONE, &tex().boxfullImage()[0 & 63]);
						drawIconGBA(iconXpos-startMenu_titleboxXpos+movecloseXpos, (titleboxYpos+12)+titleboxYposDropDown[i % 5]);
					} else if (!applaunchprep || startMenu_cursorPosition != i){
						if (theme == 1) {
							glSprite(spawnedboxXpos-startMenu_titleboxXpos, titleboxYpos, GL_FLIP_NONE, tex().boxemptyImage());
						} else {
							glSprite(spawnedboxXpos-startMenu_titleboxXpos+movecloseXpos, titleboxYpos+titleboxYposDropDown[i % 5], GL_FLIP_NONE, &tex().boxfullImage()[1 & 63]);
						}
					}
					spawnedboxXpos += 64;
					iconXpos += 64;
				}
				if (theme == 0) glSprite(spawnedboxXpos+10-startMenu_titleboxXpos, 81, GL_FLIP_H, tex().braceImage());
			} else {
				for(int i = 0; i < 40; i++) {
					if (theme == 0) {
						moveIconClose(i);
					} else {
						movecloseXpos = 0;
					}
					if (i < spawnedtitleboxes) {
						if (isDirectory[i]) {
							if (theme == 1) glSprite(spawnedboxXpos-titleboxXpos[secondaryDevice]+movecloseXpos, titleboxYpos, GL_FLIP_NONE, tex().folderImage());
							else glSprite(spawnedboxXpos-titleboxXpos[secondaryDevice]+movecloseXpos, (titleboxYpos-3)+titleboxYposDropDown[i % 5], GL_FLIP_NONE, tex().folderImage());
						} else if (!applaunchprep || cursorPosition[secondaryDevice] != i){ // Only draw the icon if we're not launching the selcted app
							if (theme == 1) {
								glSprite(spawnedboxXpos-titleboxXpos[secondaryDevice], titleboxYpos, GL_FLIP_NONE, tex().boxfullImage());
							} else { 
								glSprite(spawnedboxXpos-titleboxXpos[secondaryDevice]+movecloseXpos, titleboxYpos+titleboxYposDropDown[i % 5], GL_FLIP_NONE, &tex().boxfullImage()[0 & 63]);
							}
							if (bnrRomType[i] == 3) drawIconNES(iconXpos-titleboxXpos[secondaryDevice]+movecloseXpos, (titleboxYpos+12)+titleboxYposDropDown[i % 5]);
							else if (bnrRomType[i] == 2) drawIconGBC(iconXpos-titleboxXpos[secondaryDevice]+movecloseXpos, (titleboxYpos+12)+titleboxYposDropDown[i % 5]);
							else if (bnrRomType[i] == 1) drawIconGB(iconXpos-titleboxXpos[secondaryDevice]+movecloseXpos, (titleboxYpos+12)+titleboxYposDropDown[i % 5]);
							
							else drawIcon(iconXpos-titleboxXpos[secondaryDevice]+movecloseXpos, (titleboxYpos+12)+titleboxYposDropDown[i % 5], i);
						}
					} else {
						// Empty box
						if (theme == 1) {
							glSprite(spawnedboxXpos-titleboxXpos[secondaryDevice], titleboxYpos+titleboxYposDropDown[i % 5], GL_FLIP_NONE, tex().boxemptyImage());
						} else {
							glSprite(spawnedboxXpos-titleboxXpos[secondaryDevice]+movecloseXpos, titleboxYpos+titleboxYposDropDown[i % 5], GL_FLIP_NONE, &tex().boxfullImage()[1 & 63]);
						}
					}
					spawnedboxXpos += 64;
					iconXpos += 64;
				}
				if (theme == 0) glSprite(spawnedboxXpos+10-titleboxXpos[secondaryDevice], 81, GL_FLIP_H, tex().braceImage());
			}
			if (applaunchprep && theme==0) {
				
				if(startMenu) {
					if (startMenu_cursorPosition == 0) {
						glSprite(96, 83-titleboxYmovepos, GL_FLIP_NONE, &tex().settingsImage()[1 & 63]);
					} else if (startMenu_cursorPosition == 1) {
						if (!flashcardFound()) {
							glSprite(96, 83-titleboxYmovepos, GL_FLIP_NONE, &tex().settingsImage()[0 & 63]);
						} else {
							glSprite(96, 84-titleboxYmovepos, GL_FLIP_NONE, tex().boxfullImage());
							drawIconGBA(112, 96-titleboxYmovepos);
						}
					} else if (startMenu_cursorPosition == 2) {
						glSprite(96, 84-titleboxYmovepos, GL_FLIP_NONE, tex().boxfullImage());
						drawIconGBA(112, 96-titleboxYmovepos);
					}
				} else {
					if (isDirectory[cursorPosition[secondaryDevice]]) {
						glSprite(96, 87-titleboxYmovepos, GL_FLIP_NONE, tex().folderImage());
					} else {
						glSprite(96, 84-titleboxYmovepos, GL_FLIP_NONE, tex().boxfullImage());
						if (bnrRomType[cursorPosition[secondaryDevice]] == 3) drawIconNES(112, 96-titleboxYmovepos);
						else if (bnrRomType[cursorPosition[secondaryDevice]] == 2) drawIconGBC(112, 96-titleboxYmovepos);
						else if (bnrRomType[cursorPosition[secondaryDevice]] == 1) drawIconGB(112, 96-titleboxYmovepos);
						else drawIcon(112, 96-titleboxYmovepos, cursorPosition[secondaryDevice]);
					}
				}
				// Draw dots after selecting a game/app
				for (int i = 0; i < 11; i++) {
					glSprite(76+launchDotX[i], 69+launchDotY[i], GL_FLIP_NONE, &tex().launchdotImage()[(launchDotFrame[i]) & 15]);
					if (launchDotX[i] == 0) launchDotXMove[i] = true;
					if (launchDotX[i] == 88) launchDotXMove[i] = false;
					if (launchDotY[i] == 0) launchDotYMove[i] = true;
					if (launchDotY[i] == 88) launchDotYMove[i] = false;
					if (launchDotXMove[i] == false) {
						launchDotX[i]--;
					} else if (launchDotXMove[i] == true) {
						launchDotX[i]++;
					}
					if (launchDotYMove[i] == false) {
						launchDotY[i]--;
					} else if (launchDotYMove[i] == true) {
						launchDotY[i]++;
					}
				}
				titleboxYmovepos += 5;
			}
			if (showSTARTborder) {
				if (theme == 1) {
					glSprite(96, 92, GL_FLIP_NONE, &tex().startbrdImage()[startBorderZoomAnimSeq[startBorderZoomAnimNum] & 63]);
					glSprite(96+32, 92, GL_FLIP_H, &tex().startbrdImage()[startBorderZoomAnimSeq[startBorderZoomAnimNum] & 63]);
					glColor(RGB15(31, 31, 31));
					if (!startMenu) {
						if (bnrWirelessIcon[cursorPosition[secondaryDevice]] > 0) glSprite(96, 92, GL_FLIP_NONE, &tex().wirelessIcons()[(bnrWirelessIcon[cursorPosition[secondaryDevice]]-1) & 31]);
					}
				} else if (!isScrolling) {
					if (showbubble && theme == 0 && needToPlayStopSound && waitForNeedToPlayStopSound == 0) {
						mmEffectEx(&snd_stop);
						waitForNeedToPlayStopSound = 1;
						needToPlayStopSound = false;
					}
					glSprite(96, 81, GL_FLIP_NONE, &tex().startbrdImage()[startBorderZoomAnimSeq[startBorderZoomAnimNum] & 79]);
					glSprite(96+32, 81, GL_FLIP_H, &tex().startbrdImage()[startBorderZoomAnimSeq[startBorderZoomAnimNum] & 79]);
					glColor(RGB15(31, 31, 31));
					if (!startMenu) {
						if (bnrWirelessIcon[cursorPosition[secondaryDevice]] > 0) glSprite(96, 81, GL_FLIP_NONE, &tex().wirelessIcons()[(bnrWirelessIcon[cursorPosition[secondaryDevice]]-1) & 31]);
					}
				}
			}

			// Refresh the background layer.
			bottomBgLoad(showbubble);
			if (showbubble) drawBubble(tex().bubbleImage());
			if (showSTARTborder && theme == 0 && !isScrolling) glSprite(96, 144, GL_FLIP_NONE, &tex().startImage()[setLanguage]);
			if (dbox_Ypos != -192) {
				// Draw the dialog box.
				drawDbox();
				if (!isDirectory[cursorPosition[secondaryDevice]]) drawIcon(24, dbox_Ypos+20, cursorPosition[secondaryDevice]);
			}
			// Show button_arrowPals (debug feature)
			/*for (int i = 0; i < 16; i++) {
				for (int i2 = 0; i2 < 16; i2++) {
					glBox(i2,i,i2,i,button_arrowPals[(i*16)+i2]);
				}
			}*/
			if (whiteScreen) {
				glBoxFilled(0, 0, 256, 192, RGB15(31, 31, 31));
				if (showProgressIcon) glSprite(224, 152, GL_FLIP_NONE, &tex().progressImage()[progressAnimNum]);
			}
			
			if (vblankRefreshCounter >= REFRESH_EVERY_VBLANKS) {
				if (showdialogbox && dbox_Ypos == -192) {
					// Reload the dialog box palettes here...
					reloadDboxPalette();
				} else if (!showdialogbox) {
					reloadIconPalettes();
					reloadFontPalettes();
				}
				vblankRefreshCounter = 0;
			} else {
				vblankRefreshCounter++;
			}
			updateText(false);
			bgUpdate();
			glColor(RGB15(31, 31, 31));
		//}
	}
	glEnd2D();
	GFX_FLUSH = 0;

	if (showProgressIcon) {
		progressAnimDelay++;
		if (progressAnimDelay == 3) {
			progressAnimNum++;
			if (progressAnimNum > 7) progressAnimNum = 0;
			progressAnimDelay = 0;
		}
	}
	if (!whiteScreen) {
		// Playback animated icons
		for (int i = 0; i < 40; i++) {
			if(bnriconisDSi[i]==true) {
				playBannerSequence(i);
			}
		}
	}
	if (theme == 1) {
		startBorderZoomAnimDelay++;
		if (startBorderZoomAnimDelay == 8) {
			startBorderZoomAnimNum++;
			if(startBorderZoomAnimSeq[startBorderZoomAnimNum] == 0) {
				startBorderZoomAnimNum = 0;
			}
			startBorderZoomAnimDelay = 0;
		}
	} else if (startBorderZoomOut) {
		startBorderZoomAnimNum++;
		if(startBorderZoomAnimSeq[startBorderZoomAnimNum] == 0) {
			startBorderZoomAnimNum = 0;
			startBorderZoomOut = false;
		}
	} else {
		startBorderZoomAnimNum = 0;
	}
	if (applaunchprep && theme==0 && launchDotDoFrameChange) {
		launchDotFrame[0]--;
		if (launchDotCurrentChangingFrame >= 1) launchDotFrame[1]--;
		if (launchDotCurrentChangingFrame >= 2) launchDotFrame[2]--;
		if (launchDotCurrentChangingFrame >= 3) launchDotFrame[3]--;
		if (launchDotCurrentChangingFrame >= 4) launchDotFrame[4]--;
		if (launchDotCurrentChangingFrame >= 5) launchDotFrame[5]--;
		if (launchDotCurrentChangingFrame >= 6) launchDotFrame[6]--;
		if (launchDotCurrentChangingFrame >= 7) launchDotFrame[7]--;
		if (launchDotCurrentChangingFrame >= 8) launchDotFrame[8]--;
		if (launchDotCurrentChangingFrame >= 9) launchDotFrame[9]--;
		if (launchDotCurrentChangingFrame >= 10) launchDotFrame[10]--;
		if (launchDotCurrentChangingFrame >= 11) launchDotFrame[11]--;
		for (int i = 0; i < 12; i++) {
			if (launchDotFrame[i] < 0) launchDotFrame[i] = 0;
		}
		launchDotCurrentChangingFrame++;
		if (launchDotCurrentChangingFrame > 11) launchDotCurrentChangingFrame = 11;
	}
	if (applaunchprep && theme==0) launchDotDoFrameChange = !launchDotDoFrameChange;
}

void clearBmpScreen() {
	u16 val = 0xFFFF;
	for (int i = 0; i < 256*192; i++) {
		subBuffer[i] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
	}
}


void loadBoxArt(const char* filename) {
	FILE* file = fopen(filename, "rb");
	if (!file) file = fopen("nitro:/graphics/boxart_unknown.bmp", "rb");

	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		for (int y=114; y>=0; y--) {
			u16 buffer[128];
			fread(buffer, 2, 0x80, file);
			u16* src = buffer;
			for (int i=0; i<128; i++) {
				u16 val = *(src++);
				subBuffer[(y+40)*256+(i+64)] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
			}
		}
	}
	fclose(file);
}


void loadPhoto() {
	FILE* file = fopen("fat:/_nds/dsimenuplusplus/photo.bmp", "rb");
	if (!file) file = fopen("sd:/_nds/dsimenuplusplus/photo.bmp", "rb");
	if (!file) file = fopen("nitro:/graphics/photo_default.bmp", "rb");

	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		for (int y=155; y>=0; y--) {
			u16 buffer[208];
			fread(buffer, 2, 0xD0, file);
			u16* src = buffer;
			for (int i=0; i<208; i++) {
				u16 val = *(src++);
				subBuffer[(y+24)*256+(i+24)] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
			}
		}
	}

	fclose(file);
}

// Load photo without overwriting shoulder button images
void loadPhotoPart() {
	FILE* file = fopen("fat:/_nds/dsimenuplusplus/photo.bmp", "rb");
	if (!file) file = fopen("sd:/_nds/dsimenuplusplus/photo.bmp", "rb");
	if (!file) file = fopen("nitro:/graphics/photo_default.bmp", "rb");

	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		for (int y=155; y>=0; y--) {
			u16 buffer[208];
			fread(buffer, 2, 0xD0, file);
			if (y <= 147) {
				u16* src = buffer;
				for (int i=0; i<208; i++) {
					u16 val = *(src++);
					subBuffer[(y+24)*256+(i+24)] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
				}
			}
		}
	}

	fclose(file);
}

void loadBMP(const char* filename) {
	FILE* file = fopen(filename, "rb");

	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		for (int y=191; y>=0; y--) {
			u16 buffer[256];
			fread(buffer, 2, 0x100, file);
			u16* src = buffer;
			for (int i=0; i<256; i++) {
				u16 val = *(src++);
				if (val != 0xFC1F) {	// Do not render magneta pixel
					subBuffer[y*256+i] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
				}
			}
		}
	}

	fclose(file);
}

// Load .bmp file without overwriting shoulder button images or username
void loadBMPPart(const char* filename) {
	FILE* file = fopen(filename, "rb");

	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		for (int y=191; y>=32; y--) {
			u16 buffer[256];
			fread(buffer, 2, 0x100, file);
			if (y <= 167) {
				u16* src = buffer;
				for (int i=0; i<256; i++) {
					u16 val = *(src++);
					if (val != 0xFC1F) {	// Do not render magneta pixel
						subBuffer[y*256+i] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
					}
				}
			}
		}
	}

	fclose(file);
}

void loadShoulders() {
	FILE* file;

	// Draw L shoulder
	if (showLshoulder)
	{ 
		file = fopen(tex().shoulderLPath.c_str(), "rb");
	} else {
		file = fopen(tex().shoulderLGreyPath.c_str(), "rb");
	}

	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		for (int y=19; y>=0; y--) {
			u16 buffer[78];
			fread(buffer, 2, 0x4E, file);
			u16* src = buffer;
			for (int i=0; i<78; i++) {
				u16 val = *(src++);
				if (val != 0xFC1F) {	// Do not render magneta pixel
					subBuffer[(y+172)*256+i] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
				}
			}
		}
	}

	fclose(file);

	// Draw R shoulder

	if (showRshoulder)
	{ 
		file = fopen(tex().shoulderRPath.c_str(), "rb");
	} else {
		file = fopen(tex().shoulderRGreyPath.c_str(), "rb");
	}
	
	if (file) {
		// Start loading
		fseek(file, 0xe, SEEK_SET);
		u8 pixelStart = (u8)fgetc(file) + 0xe;
		fseek(file, pixelStart, SEEK_SET);
		for (int y=19; y>=0; y--) {
			u16 buffer[78];
			fread(buffer, 2, 0x4E, file);
			u16* src = buffer;
			for (int i=0; i<78; i++) {
				u16 val = *(src++);
				if (val != 0xFC1F) {	// Do not render magneta pixel
					subBuffer[(y+172)*256+(i+178)] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
				}
			}
		}
	}

	fclose(file);
}

void presentSubBg()
{
	DC_FlushRange((void*) subBuffer, 256 * 192 * 2);
    dmaCopyWords(3, (void*) subBuffer, BG_GFX_SUB, 256 * 192 * 2);
	//dmaCopyWords(3, (void*) subBuffer,  bgGetGfxPtr(drawLayer), 256 * 192 * 2);
    DC_InvalidateRange(BG_GFX_SUB, 256 * 192 * 2);
//	DC_InvalidateRange(bgGetGfxPtr(drawLayer), 256 * 192 * 2);

	//swiFastCopy((void*)(0xffffffff), subBuffer, (0x18000>>2) | COPY_MODE_WORD | COPY_MODE_FILL);


}

/**
 * Get the index in the UV coordinate array where the letter appears
 */
unsigned int getTopFontSpriteIndex(const u16 letter) {
	unsigned int spriteIndex = 0;
	for (unsigned int i = 0; i < TOP_FONT_NUM_IMAGES; i++) {
		if (top_utf16_lookup_table[i] == letter) {
			spriteIndex = i;
		}
	}
	return spriteIndex;
}

//   xrrrrrgggggbbbbb according to http://problemkaputt.de/gbatek.htm#dsvideobgmodescontrol
#define MASK_RB      0b0111110000011111
#define MASK_G       0b0000001111100000 
#define MASK_MUL_RB  0b0111110000011111000000 
#define MASK_MUL_G   0b0000001111100000000000 
#define MAX_ALPHA        64 // 6bits+1 with rounding

/**
 * Adapted from https://stackoverflow.com/questions/18937701/
 * applies alphablending with the given
 * RGB555 foreground, RGB555 background, and alpha from 
 * 0 to 128 (0, 1.0).
 * The lower the alpha the more transparent, but
 * this function does not produce good results at the extremes 
 * (near 0 or 128).
 */
inline u16 alphablend(u16 fg, u16 bg, u8 alpha)
{

	// alpha for foreground multiplication
	// convert from 8bit to (6bit+1) with rounding
	// will be in [0..64] inclusive
	alpha = (alpha + 2) >> 2;
	// "beta" for background multiplication; (6bit+1);
	// will be in [0..64] inclusive
	u8 beta = MAX_ALPHA - alpha;
	// so (0..64)*alpha + (0..64)*beta always in 0..64

	return (u16)((
					 ((alpha * (u32)(fg & MASK_RB) + beta * (u32)(bg & MASK_RB)) & MASK_MUL_RB) |
					 ((alpha * (fg & MASK_G) + beta * (bg & MASK_G)) & MASK_MUL_G)) >>
				 5);
}

void topBgLoad() {
	loadBMP(tex().topBgPath.c_str());

	// Load username
	char fontPath[64];
	FILE* file;
	int x = 24; 

	for (int c = 0; c < 10; c++) {
		unsigned int charIndex = getTopFontSpriteIndex(usernameRendered[c]);
		// 42 characters per line.
		unsigned int texIndex = charIndex / 42;
		sprintf(fontPath, "nitro:/graphics/top_font/small_font_%u.bmp", texIndex);
		
		file = fopen(fontPath, "rb");

		if (file) {
			// Start loading
			fseek(file, 0xe, SEEK_SET);
			u8 pixelStart = (u8)fgetc(file) + 0xe;
			fseek(file, pixelStart, SEEK_SET);
			for (int y=15; y>=0; y--) {
				u16 buffer[512];
				fread(buffer, 2, 0x200, file);
				u16* src = buffer+(top_font_texcoords[0+(4*charIndex)]);

				for (int i=0; i < top_font_texcoords[2+(4*charIndex)]; i++) {
					u16 val = *(src++);
					u16 bg = subBuffer[(y+1)*256+(i+x)]; //grab the background pixel
					// Apply palette here.
					
					// Magic numbers were found by dumping val to stdout
					// on case default.
					switch (val) {
						// #ff00ff
						case 0xFC1F:
							break;
						// #414141
						case 0xA108:
							val = bmpPal_topSmallFont[1+((PersonalData->theme)*16)];
							break;
						case 0xC210:
							// blend the colors with the background to make it look better.
							val = alphablend(bmpPal_topSmallFont[2+((PersonalData->theme)*16)], bg, 48);
							break;
						case 0xDEF7:
							val = alphablend(bmpPal_topSmallFont[3+((PersonalData->theme)*16)], bg, 64);
						default:
							break;
					}
					if (val != 0xFC1F) {	// Do not render magneta pixel
						subBuffer[(y+1)*256+(i+x)] = ((val>>10)&0x1f) | ((val)&(0x1f<<5)) | (val&0x1f)<<10 | BIT(15);
					}
				}
			}
			x += top_font_texcoords[2+(4*charIndex)];
		}

		fclose(file);
	}
}

void clearBoxArt() {
	if (theme == 1) {
		loadBMPPart("nitro:/graphics/3ds_top.bmp");
	} else {
		loadPhotoPart();
	}
}

void graphicsInit()
{
	
	for (int i = 0; i < 12; i++) {
		launchDotFrame[i] = 5;
	}

	for (int i = 0; i < 5; i++) {
		dropTime[i] = 0;
		dropSeq[i] = 0;
		dropSpeed[i] = 5;
		dropSpeedChange[i] = 0;
		if (theme == 1) titleboxYposDropDown[i] = 0;
		else titleboxYposDropDown[i] = -85-80;
	}
	allowedTitleboxForDropDown = 0;
	delayForTitleboxToDropDown = 0;
	dropDown = false;
	redoDropDown = false;

	launchDotXMove[0] = false;
	launchDotYMove[0] = true;
	launchDotX[0] = 44;
	launchDotY[0] = 0;
	launchDotXMove[1] = false;
	launchDotYMove[1] = true;
	launchDotX[1] = 28;
	launchDotY[1] = 16;
	launchDotXMove[2] = false;
	launchDotYMove[2] = true;
	launchDotX[2] = 12;
	launchDotY[2] = 32;
	launchDotXMove[3] = true;
	launchDotYMove[3] = true;
	launchDotX[3] = 4;
	launchDotY[3] = 48;
	launchDotXMove[4] = true;
	launchDotYMove[4] = true;
	launchDotX[4] = 20;
	launchDotY[4] = 64;
	launchDotXMove[5] = true;
	launchDotYMove[5] = true;
	launchDotX[5] = 36;
	launchDotY[5] = 80;
	launchDotXMove[6] = true;
	launchDotYMove[6] = false;
	launchDotX[6] = 52;
	launchDotY[6] = 80;
	launchDotXMove[7] = true;
	launchDotYMove[7] = false;
	launchDotX[7] = 68;
	launchDotY[7] = 64;
	launchDotXMove[8] = true;
	launchDotYMove[8] = false;
	launchDotX[8] = 84;
	launchDotY[8] = 48;
	launchDotXMove[9] = false;
	launchDotYMove[9] = false;
	launchDotX[9] = 76;
	launchDotY[9] = 36;
	launchDotXMove[10] = false;
	launchDotYMove[10] = false;
	launchDotX[10] = 60;
	launchDotY[10] = 20;
	launchDotX[11] = 44;
	launchDotY[11] = 0;

	titleboxXpos[0] = cursorPosition[0]*64;
	titlewindowXpos[0] = cursorPosition[0]*5;
	titleboxXpos[1] = cursorPosition[1]*64;
	titlewindowXpos[1] = cursorPosition[1]*5;
	startMenu_titleboxXpos = startMenu_cursorPosition*64;
	startMenu_titlewindowXpos = startMenu_cursorPosition*5;
	
	*(u16*)(0x0400006C) |= BIT(14);
	*(u16*)(0x0400006C) &= BIT(15);
	SetBrightness(0, 31);
	SetBrightness(1, 31);

	////////////////////////////////////////////////////////////
	videoSetMode(MODE_5_3D  | DISPLAY_BG3_ACTIVE |  DISPLAY_BG2_ACTIVE);
	videoSetModeSub(MODE_3_2D | DISPLAY_BG3_ACTIVE);

	// Initialize gl2d
	glScreen2D();
	// Make gl2d render on transparent stage.
	glClearColor(31,31,31,0);
	glDisable(GL_CLEAR_BMP);

	// Clear the GL texture state
	glResetTextures();

	// Set up enough texture memory for our textures
	// Bank A is just 128kb and we are using 194 kb of
	// sprites
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_TEXTURE);
	vramSetBankC(VRAM_C_SUB_BG_0x06200000);
	vramSetBankD(VRAM_D_MAIN_BG_0x06000000);
	vramSetBankE(VRAM_E_TEX_PALETTE);
	vramSetBankF(VRAM_F_TEX_PALETTE_SLOT4);
	vramSetBankG(VRAM_G_TEX_PALETTE_SLOT5); // 16Kb of palette ram, and font textures take up 8*16 bytes.
	vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE);
	vramSetBankI(VRAM_I_SUB_SPRITE);

	drw();
//	vramSetBankH(VRAM_H_SUB_BG_EXT_PALETTE); // Not sure this does anything... 
	lcdMainOnBottom();
	
	REG_BG3CNT_SUB = BG_MAP_BASE(0) | BG_BMP16_256x256 | BG_PRIORITY(0);
	REG_BG3X_SUB = 0;
	REG_BG3Y_SUB = 0;
	REG_BG3PA_SUB = 1<<8;
	REG_BG3PB_SUB = 0;
	REG_BG3PC_SUB = 0;
	REG_BG3PD_SUB = 1<<8;

	if (theme < 1) loadPhoto();

	// Initialize the bottom background
	drawLayer = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 2, 0);
	bottomBg = bgInit(3, BgType_ExRotation, BgSize_ER_256x256, 0,1);
	bgSetPriority(bottomBg, 3);
	bgSetPriority(drawLayer, 0);
	REG_BG0CNT = REG_BG0CNT | BG_PRIORITY(1); 
	bgShow(drawLayer);
	swiIntrWait(0, 1);

	if (theme == 1) {
		tex().load3DSTheme();
		titleboxYpos = 96;
		bubbleYpos += 18;
		bubbleXpos += 3;
		topBgLoad();
		bottomBgLoad(false, true);
	} else {
		switch(subtheme) {
			default:
			case 0:
				tex().loadDSiDarkTheme();
				break;
			case 1:
				tex().loadDSiWhiteTheme();
				break;
			case 2:
				tex().loadDSiRedTheme();
				break;
			case 3:
				tex().loadDSiBlueTheme();
				break;
			case 4:
				tex().loadDSiGreenTheme();
				break;
			case 5:
				tex().loadDSiYellowTheme();
				break;
			case 6:
				tex().loadDSiPinkTheme();
				break;
			case 7:
				tex().loadDSiPurpleTheme();
				break;
		}
		topBgLoad();
		bottomBgLoad(false, true);
	}

	irqSet(IRQ_VBLANK, vBlankHandler);
	irqEnable(IRQ_VBLANK);
	//consoleDemoInit();


}
