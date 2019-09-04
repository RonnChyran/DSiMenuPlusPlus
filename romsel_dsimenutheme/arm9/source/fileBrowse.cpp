/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

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

#include "fileBrowse.h"
#include "buttontext.h"
#include <algorithm>
#include <dirent.h>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

#include "common/gl2d.h"
#include <maxmod9.h>
#include <nds.h>

#include "date.h"

#include "SwitchState.h"
#include "errorScreen.h"
#include "graphics/FontGraphic.h"
#include "graphics/TextPane.h"
#include "graphics/ThemeTextures.h"
#include "graphics/fontHandler.h"
#include "graphics/graphics.h"
#include "graphics/iconHandler.h"
#include "iconTitle.h"
#include "ndsheaderbanner.h"
#include "perGameSettings.h"

#include "gbaswitch.h"
#include "nds_loader_arm9.h"

#include "common/dsimenusettings.h"
#include "common/flashcard.h"
#include "common/inifile.h"
#include "common/systemdetails.h"

#include "fileCopy.h"
#include "sound.h"

#include "graphics/queueControl.h"

#define SCREEN_COLS 32
#define ENTRIES_PER_SCREEN 15
#define ENTRIES_START_ROW 3
#define ENTRY_PAGE_LENGTH 10

extern bool whiteScreen;
extern bool fadeType;
extern bool fadeSpeed;
extern bool controlTopBright;
extern bool controlBottomBright;

extern bool gbaBiosFound[2];

extern const char *unlaunchAutoLoadID;

extern bool dropDown;
extern bool redoDropDown;
extern int currentBg;
extern bool showSTARTborder;
extern bool buttonArrowTouched[2];
extern bool scrollWindowTouched;

extern bool titleboxXmoveleft;
extern bool titleboxXmoveright;

extern bool applaunchprep;

extern touchPosition touch;

extern bool showdialogbox;
extern bool dboxInFrame;
extern bool dbox_showIcon;
extern bool dbox_selectMenu;

extern bool applaunch;

extern int vblankRefreshCounter;
using namespace std;

extern bool startMenu;

int file_count = 0;

extern int spawnedtitleboxes;

extern int titleboxXpos[2];
extern int titlewindowXpos[2];
int movingApp = -1;
int movingAppYpos = 0;
bool movingAppIsDir = false;
extern bool showMovingArrow;
extern double movingArrowYpos;
extern bool displayGameIcons;

extern bool showLshoulder;
extern bool showRshoulder;

extern bool showProgressIcon;

bool dirInfoIniFound = false;
bool pageLoaded[100] = {false};
bool dirContentBlankFilled[100] = {false};
bool lockOutDirContentBlankFilling = false;

char boxArtPath[256];

bool boxArtLoaded = false;
bool shouldersRendered = false;
bool settingsChanged = false;

bool isScrolling = false;
bool edgeBumpSoundPlayed = false;
bool needToPlayStopSound = false;
bool stopSoundPlayed = false;
int waitForNeedToPlayStopSound = 0;

bool bannerTextShown = false;

extern std::string ReplaceAll(std::string str, const std::string &from, const std::string &to);

extern void stop();

extern void loadGameOnFlashcard(const char *ndsPath, std::string filename, bool usePerGameSettings);
extern void dsCardLaunch();
extern void unlaunchSetHiyaBoot();
extern void SetWidescreen(const char *filename);

extern bool rocketVideo_playVideo;

extern char usernameRendered[11];
extern bool usernameRenderedDone;

const char *gameOrderIniPath;
const char *recentlyPlayedIniPath;
const char *timesPlayedIniPath;
const char *hiddenGamesIniPath;

static bool inSelectMenu = false;

struct DirEntry {
	string name;
	bool isDirectory;
	int position;
	bool customPos;
};

struct TimesPlayed {
	string name;
	int amount;
};

TextEntry *pathText = nullptr;
char path[PATH_MAX] = {0};

#ifdef EMULATE_FILES
#define chdir(a) chdirFake(a)
void chdirFake(const char *dir) {
	string pathStr(path);
	string dirStr(dir);
	if (dirStr == "..") {
		pathStr.resize(pathStr.find_last_of("/"));
		pathStr.resize(pathStr.find_last_of("/") + 1);
	} else {
		pathStr += dirStr;
		pathStr += "/";
	}
	strcpy(path, pathStr.c_str());
}
#endif

bool nameEndsWith(const string &name, const vector<string> extensionList) {

	if (name.substr(0, 2) == "._")
		return false; // Don't show macOS's index files

	if (name.size() == 0)
		return false;

	if (extensionList.size() == 0)
		return true;

	for (int i = 0; i < (int)extensionList.size(); i++) {
		const string ext = extensionList.at(i);
		if (strcasecmp(name.c_str() + name.size() - ext.size(), ext.c_str()) == 0)
			return true;
	}
	return false;
}

bool dirEntryPredicate(const DirEntry &lhs, const DirEntry &rhs) {

	if (lhs.customPos) {
		if(lhs.position < rhs.position)	return true;
		else return false;
	}
	if (!lhs.isDirectory && rhs.isDirectory) {
		return false;
	}
	if (lhs.isDirectory && !rhs.isDirectory) {
		return true;
	}
	return strcasecmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
}

bool dirEntryPredicateMostPlayed(const DirEntry &lhs, const DirEntry &rhs) {
	if (!lhs.isDirectory && rhs.isDirectory)	return false;
	else if (lhs.isDirectory && !rhs.isDirectory)	return true;

	if(lhs.position > rhs.position)	return true;
	else if(lhs.position < rhs.position)	return false;
	else {
		return strcasecmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
	}
}

bool dirEntryPredicateFileType(const DirEntry &lhs, const DirEntry &rhs) {
	if (!lhs.isDirectory && rhs.isDirectory)	return false;
	else if (lhs.isDirectory && !rhs.isDirectory)	return true;

	if(strcasecmp(lhs.name.substr(lhs.name.find_last_of(".") + 1).c_str(), rhs.name.substr(rhs.name.find_last_of(".") + 1).c_str()) == 0) {
		return strcasecmp(lhs.name.c_str(), rhs.name.c_str()) < 0;
	} else {
		return strcasecmp(lhs.name.substr(lhs.name.find_last_of(".") + 1).c_str(), rhs.name.substr(rhs.name.find_last_of(".") + 1).c_str()) < 0;
	}
}

void updateDirectoryContents(vector<DirEntry> &dirContents) {
	if (!dirInfoIniFound || pageLoaded[PAGENUM]) return;

	if ((PAGENUM > 0) && !lockOutDirContentBlankFilling) {
		DirEntry dirEntry;
		dirEntry.name = "";
		dirEntry.isDirectory = false;
		dirEntry.customPos = false;

		for (int p = 0; p < PAGENUM; p++) {
			for (int i = 0; i < 40; i++) {
				dirEntry.position = i+(p*40);
				dirContents.insert(dirContents.begin() + i + (p * 40), dirEntry);
			}
			dirContentBlankFilled[p] = true;
		}
	}
	lockOutDirContentBlankFilling = true;

	char str[12] = {0};
	CIniFile twlmDirInfo("dirInfo.twlm.ini");
	int currentPos = PAGENUM*40;
	for (int i = 0; i < 40; i++) {
		sprintf(str, "%d", i+(PAGENUM*40));
		DirEntry dirEntry;
		std::string filename = twlmDirInfo.GetString("LIST", str, "");

		if (filename != "") {
			dirEntry.name = filename;
			dirEntry.isDirectory = false;
			dirEntry.position = currentPos;
			dirEntry.customPos = false;

			if (dirContentBlankFilled[PAGENUM]) {
				dirContents.erase(dirContents.begin() + i + (PAGENUM * 40));
			}
			dirContents.insert(dirContents.begin() + i + (PAGENUM * 40), dirEntry);
			currentPos++;
		} else {
			break;
		}
	}
	pageLoaded[PAGENUM] = true;
}

void getDirectoryContents(vector<DirEntry> &dirContents, const vector<string> extensionList) {

	dirContents.clear();

	file_count = 0;

	if (access("dirInfo.twlm.ini", F_OK) == 0) {
		dirInfoIniFound = true;

		for (int i = 0; i < (int)sizeof(pageLoaded); i++) {
			pageLoaded[i] = false;
		}

		CIniFile twlmDirInfo("dirInfo.twlm.ini");
		file_count = twlmDirInfo.GetInt("INFO", "GAMES", 0);
		return;
	} else {
		dirInfoIniFound = false;
	}

	struct stat st;
	DIR *pdir = opendir(".");

	if (pdir == NULL) {
		// iprintf("Unable to open the directory.\n");
		printSmall(false, 4, 4, "Unable to open the directory.");
	} else {
		CIniFile hiddenGamesIni(hiddenGamesIniPath);
		vector<std::string> hiddenGames;
		char str[12] = {0};

		for (int i = 0; true; i++) {
			sprintf(str, "%d", i);
			if (hiddenGamesIni.GetString(getcwd(path, PATH_MAX), str, "") != "") {
				hiddenGames.push_back(hiddenGamesIni.GetString(getcwd(path, PATH_MAX), str, ""));
			} else {
				break;
			}
		}
		int currentPos = 0;
		while (true) {
			snd().updateStream();

			DirEntry dirEntry;

			struct dirent *pent = readdir(pdir);
			if (pent == NULL)
				break;

			stat(pent->d_name, &st);
			dirEntry.name = pent->d_name;
			dirEntry.isDirectory = (st.st_mode & S_IFDIR) ? true : false;
			dirEntry.position = currentPos;
			dirEntry.customPos = false;

			if (ms().showDirectories) {
				if (dirEntry.name.compare(".") != 0 && dirEntry.name.compare("_nds") &&
				    dirEntry.name.compare("saves") != 0 &&
				    (dirEntry.isDirectory || nameEndsWith(dirEntry.name, extensionList))) {
					bool isHidden = false;
					for (int i = 0; i < (int)hiddenGames.size(); i++) {
						if (dirEntry.name == hiddenGames[i]) {
							isHidden = true;
							break;
						}
					}
					if (!isHidden || ms().showHidden) {
						dirContents.push_back(dirEntry);
						file_count++;
					}
				}
			} else {
				if (dirEntry.name.compare(".") != 0 && (nameEndsWith(dirEntry.name, extensionList))) {
					bool isHidden = false;
					for (int i = 0; i < (int)hiddenGames.size(); i++) {
						if (dirEntry.name == hiddenGames[i]) {
							isHidden = true;
							break;
						}
					}
					if (!isHidden || ms().showHidden) {
						dirContents.push_back(dirEntry);
						file_count++;
					}
				}
			}
			currentPos++;

			tex().drawVolumeImageCached();
			tex().drawBatteryImageCached();
			drawCurrentTime();
			drawCurrentDate();
			drawClockColon();
			
			snd().updateStream();
		}

		if(ms().sortMethod == 0) { // Alphabetical
			sort(dirContents.begin(), dirContents.end(), dirEntryPredicate);
		} else if(ms().sortMethod == 1) { // Recent
			CIniFile recentlyPlayedIni(recentlyPlayedIniPath);
			vector<std::string> recentlyPlayed;

			for (int i = 0; i < (int)dirContents.size(); i++) {
				sprintf(str, "%d", i);
				recentlyPlayed.push_back(recentlyPlayedIni.GetString(getcwd(path, PATH_MAX), str, "NULL"));
			}

			for (int i = 0; i < (int)recentlyPlayed.size(); i++) {
				for (int j = 0; j <= (int)dirContents.size(); j++) {
					if (recentlyPlayed[i] == dirContents[j].name) {
						dirContents[j].position = i;
						dirContents[j].customPos = true;
					}
				}
			}
			sort(dirContents.begin(), dirContents.end(), dirEntryPredicate);
		} else if(ms().sortMethod == 2) { // Most Played
			CIniFile timesPlayedIni(timesPlayedIniPath);
			vector<TimesPlayed> timesPlayed;

			for (int i = 0; i < (int)dirContents.size(); i++) {
				TimesPlayed timesPlayedTemp;
				timesPlayedTemp.name = dirContents[i].name;
				timesPlayedTemp.amount = timesPlayedIni.GetInt(getcwd(path, PATH_MAX), dirContents[i].name, 0);
				// if(timesPlayedTemp.amount)
					timesPlayed.push_back(timesPlayedTemp);
			}

			for (int i = 0; i < (int)timesPlayed.size(); i++) {
				for (int j = 0; j <= (int)dirContents.size(); j++) {
					if (timesPlayed[i].name == dirContents[j].name) {
						dirContents[j].position = timesPlayed[i].amount;
					}
				}
			}
			sort(dirContents.begin(), dirContents.end(), dirEntryPredicateMostPlayed);
		} else if(ms().sortMethod == 3) { // File type
			sort(dirContents.begin(), dirContents.end(), dirEntryPredicateFileType);
		} else if(ms().sortMethod == 4) { // Custom
			CIniFile gameOrderIni(gameOrderIniPath);
			vector<std::string> gameOrder;

			for (int i = 0; i < (int)dirContents.size(); i++) {
				sprintf(str, "%d", i);
				gameOrder.push_back(gameOrderIni.GetString(getcwd(path, PATH_MAX), str, "NULL"));
			}

			for (int i = 0; i < (int)gameOrder.size(); i++) {
				for (int j = 0; j <= (int)dirContents.size(); j++) {
					if (gameOrder[i] == dirContents[j].name) {
						dirContents[j].position = i;
						dirContents[j].customPos = true;
					}
				}
			}
			sort(dirContents.begin(), dirContents.end(), dirEntryPredicate);
		}
		closedir(pdir);
	}
}

void getDirectoryContents(vector<DirEntry> &dirContents) {
	vector<string> extensionList;
	getDirectoryContents(dirContents, extensionList);
}

void updatePath() {
#ifndef EMULATE_FILES
	getcwd(path, PATH_MAX);
#else
	if (strlen(path) < 1) {
		path[0] = '/';
		path[1] = '\0';
	}
#endif
	if (pathText == nullptr) {
		printLarge(false, 2 * FONT_SX, 1 * FONT_SY, path);
		pathText = getPreviousTextEntry(false);
		pathText->anim = TextEntry::AnimType::IN;
		pathText->fade = TextEntry::FadeType::NONE;
		pathText->y = 100;
		pathText->immune = true;
	}

	int pathWidth = calcLargeFontWidth(path);
	pathText->delay = TextEntry::ACTIVE;
	pathText->finalX = min(2 * FONT_SX, -(pathWidth + 2 * FONT_SX - 256));
}

bool isTopLevel(const char *path) {
#ifdef EMULATE_FILES
	return strlen(path) <= strlen("/");
#else
	return strlen(path) <= strlen("fat: /");
#endif
}

void waitForFadeOut(void) {
	if (ms().theme == 4) return;

	if (!dropDown && ms().theme == 0) {
		dropDown = true;
		for (int i = 0; i < 66; i++) {
			snd().updateStream();
			checkSdEject();
			tex().drawVolumeImageCached();
			tex().drawBatteryImageCached();
			drawCurrentTime();
			drawCurrentDate();
			drawClockColon();
			snd().updateStream();
			swiWaitForVBlank();
		}
	} else {
		for (int i = 0; i < 25; i++) {
			snd().updateStream();
			checkSdEject();
			tex().drawVolumeImageCached();
			tex().drawBatteryImageCached();
			drawCurrentTime();
			drawCurrentDate();
			drawClockColon();
			snd().updateStream();
			swiWaitForVBlank();
			snd().updateStream();
			swiWaitForVBlank();
		}
	}
}

bool nowLoadingDisplaying = false;

void displayNowLoading(void) {
	displayGameIcons = false;
	fadeType = true; // Fade in from white
	snd().updateStream();
	printLargeCentered(false, (ms().theme == 4 ? 32 : 88), "Now Loading...");
	if (!sys().isRegularDS()) {
		if (ms().theme == 4) {
			if (ms().secondaryDevice) {
				printSmallCentered(false, 48, "Location: Slot-1 microSD");
			} else if (ms().consoleModel < 3) {
				printSmallCentered(false, 48, "Location: SD Card");
			} else {
				printSmallCentered(false, 48, "Location: microSD Card");
			}
		} else {
			printSmall(false, 8, 152, "Location:");
			if (ms().secondaryDevice) {
				printSmall(false, 8, 168, "Slot-1 microSD Card");
			} else if (ms().consoleModel < 3) {
				printSmall(false, 8, 168, "SD Card");
			} else {
				printSmall(false, 8, 168, "microSD Card");
			}
		}
	}
	nowLoadingDisplaying = true;
	reloadFontPalettes();
	while (!screenFadedIn()) 
	{
		snd().updateStream();
	}
	snd().updateStream();
	showProgressIcon = true;
	controlTopBright = false;
}

void updateScrollingState(u32 held, u32 pressed) {
	snd().updateStream();

	bool isHeld = (held & KEY_LEFT) || (held & KEY_RIGHT) ||
			((held & KEY_TOUCH) && touch.py > 171 && touch.px < 19) || ((held & KEY_TOUCH) && touch.py > 171 && touch.px > 236);
	bool isPressed = (pressed & KEY_LEFT) || (pressed & KEY_RIGHT) ||
			((pressed & KEY_TOUCH) && touch.py > 171 && touch.px < 19) || ((pressed & KEY_TOUCH) && touch.py > 171 && touch.px > 236);

	// If we were scrolling before, but now let go of all keys, stop scrolling.
	if (isHeld && !isPressed && (CURPOS != 0 && CURPOS != 39)) {
		isScrolling = true;
		if (edgeBumpSoundPlayed) {
			edgeBumpSoundPlayed = false;
		}
	} else if (!isHeld && !isPressed && !titleboxXmoveleft && !titleboxXmoveright) {
		isScrolling = false;
	}

	if (isPressed && !isHeld) {
		if (edgeBumpSoundPlayed) {
			edgeBumpSoundPlayed = false;
		}
	}
}

void updateBoxArt(vector<vector<DirEntry>> dirContents, SwitchState scrn) {
	if (CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
		showSTARTborder = true;
		if (!ms().showBoxArt) {
			return;
		}

		if (!boxArtLoaded) {
			if (isDirectory[CURPOS]) {
				if (ms().theme == 1) {
					if (!rocketVideo_playVideo) {
						clearBoxArt(); // Clear box art, if it's a directory
						rocketVideo_playVideo = true;
					}
				} else {
					clearBoxArt(); // Clear box art, if it's a directory
				}
			} else {
				if (ms().theme == 1 && rocketVideo_playVideo) {
					// Clear top screen cubes or box art
					clearBoxArt();		
					rocketVideo_playVideo = false;
				}
				if (isDSiMode()) {
					tex().drawBoxArtFromMem(CURPOS); // Load box art
				} else {
					snprintf(boxArtPath, sizeof(boxArtPath),
						 (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png"
								: "fat:/_nds/TWiLightMenu/boxart/%s.png"),
						 dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str());
					if ((bnrRomType[CURPOS] == 0) && (access(boxArtPath, F_OK) != 0)) {
						snprintf(boxArtPath, sizeof(boxArtPath),
							 (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png"
									: "fat:/_nds/TWiLightMenu/boxart/%s.png"),
							 gameTid[CURPOS]);
					}
					tex().drawBoxArt(boxArtPath); // Load box art
				}
			}
			boxArtLoaded = true;
		}
	}
}


void launchSettings(void) {
	snd().playLaunch();
	controlTopBright = true;
	ms().gotosettings = true;

	fadeType = false;		  // Fade to white
	snd().fadeOutStream();
	for (int i = 0; i < 60; i++) {
		snd().updateStream();
		swiWaitForVBlank();
	}
	mmEffectCancelAll();
	snd().stopStream();
	ms().saveSettings();
	// Launch settings
	if (sdFound()) {
		chdir("sd:/");
	}
	int err = runNdsFile("/_nds/TWiLightMenu/settings.srldr", 0, NULL, true, false, false, true, true);
	char text[32];
	snprintf(text, sizeof(text), "Start failed. Error %i", err);
	fadeType = true;
	printLarge(false, 4, 4, text);
	stop();
}

void launchManual(void) {
	snd().playLaunch();
	
	controlTopBright = true;
	ms().gotosettings = true;

	fadeType = false;		  // Fade to white
	snd().fadeOutStream();
	for (int i = 0; i < 60; i++) {
		snd().updateStream();
		swiWaitForVBlank();
	}
	
	mmEffectCancelAll();
	snd().stopStream();
	ms().saveSettings();
	// Launch settings
	if (sdFound()) {
		chdir("sd:/");
	}
	int err = runNdsFile("/_nds/TWiLightMenu/manual.srldr", 0, NULL, true, false, false, true, true);
	char text[32];
	snprintf(text, sizeof(text), "Start failed. Error %i", err);
	fadeType = true;
	printLarge(false, 4, 4, text);
	stop();
}

void exitToSystemMenu(void) {
	snd().playLaunch();
	controlTopBright = true;

	fadeType = false;		  // Fade to white
	snd().fadeOutStream();
	for (int i = 0; i < 60; i++) {
		snd().updateStream();
		swiWaitForVBlank();
	}
	
	mmEffectCancelAll();
	snd().stopStream();

	if (settingsChanged) {
		ms().saveSettings();
		settingsChanged = false;
	}
	if (!isDSiMode() || ms().launcherApp == -1) {
		*(u32 *)(0x02000300) = 0x434E4C54; // Set "CNLT" warmboot flag
		*(u16 *)(0x02000304) = 0x1801;
		*(u32 *)(0x02000310) = 0x4D454E55; // "MENU"
		unlaunchSetHiyaBoot();
	} else {
		extern char unlaunchDevicePath[256];

		memcpy((u8 *)0x02000800, unlaunchAutoLoadID, 12);
		*(u16 *)(0x0200080C) = 0x3F0;			   // Unlaunch Length for CRC16 (fixed, must be 3F0h)
		*(u16 *)(0x0200080E) = 0;			   // Unlaunch CRC16 (empty)
		*(u32 *)(0x02000810) = (BIT(0) | BIT(1));	  // Load the title at 2000838h
								   // Use colors 2000814h
		*(u16 *)(0x02000814) = 0x7FFF;			   // Unlaunch Upper screen BG color (0..7FFFh)
		*(u16 *)(0x02000816) = 0x7FFF;			   // Unlaunch Lower screen BG color (0..7FFFh)
		memset((u8 *)0x02000818, 0, 0x20 + 0x208 + 0x1C0); // Unlaunch Reserved (zero)
		int i2 = 0;
		for (int i = 0; i < (int)sizeof(unlaunchDevicePath); i++) {
			*(u8 *)(0x02000838 + i2) =
			    unlaunchDevicePath[i]; // Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
			i2 += 2;
		}
		while (*(u16 *)(0x0200080E) == 0) { // Keep running, so that CRC16 isn't 0
			*(u16 *)(0x0200080E) = swiCRC16(0xFFFF, (void *)0x02000810, 0x3F0); // Unlaunch CRC16
		}
	}
	fifoSendValue32(FIFO_USER_02, 1); // ReturntoDSiMenu
}

void switchDevice(void) {
	if (bothSDandFlashcard()) {
		(ms().theme == 4) ? snd().playLaunch() : snd().playSwitch();
		fadeType = false; // Fade to white
		for (int i = 0; i < 25; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		ms().secondaryDevice = !ms().secondaryDevice;
		if (!rocketVideo_playVideo || ms().showBoxArt)
			clearBoxArt(); // Clear box art
		if (ms().theme != 4) whiteScreen = true;
		boxArtLoaded = false;
		rocketVideo_playVideo = true;
		redoDropDown = true;
		shouldersRendered = false;
		currentBg = 0;
		showSTARTborder = false;
		stopSoundPlayed = false;
		clearText();
		ms().saveSettings();
		settingsChanged = false;
		if (ms().theme == 4) {
			snd().playStartup();
		}
	} else {
		snd().playLaunch();
		controlTopBright = true;

		if (ms().theme != 4) {
			fadeType = false;		  // Fade to white
			snd().fadeOutStream();
			for (int i = 0; i < 60; i++) {
				snd().updateStream();
				swiWaitForVBlank();
			}
			mmEffectCancelAll();

			snd().stopStream();
		}

		ms().romPath = "";
		ms().launchType = DSiMenuPlusPlusSettings::TLaunchType::ESlot1; // 0
		ms().saveSettings();

		if (!ms().slot1LaunchMethod || sys().arm7SCFGLocked()) {
			dsCardLaunch();
		} else {
			SetWidescreen(NULL);
			if (sdFound()) {
				chdir("sd:/");
			}
			int err = runNdsFile("/_nds/TWiLightMenu/slot1launch.srldr", 0, NULL, true, true, false, true, true);
			char text[32];
			snprintf(text, sizeof(text), "Start failed. Error %i", err);
			fadeType = true;
			printLarge(false, 4, 4, text);
			stop();
		}
	}
}

void launchGba(void) {
	if (!gbaBiosFound[ms().secondaryDevice] && ms().useGbarunner) {
		snd().playWrong();
		clearText();
		dbox_selectMenu = false;
		if (!showdialogbox) {
			showdialogbox = true;
			for (int i = 0; i < 30; i++) {
				snd().updateStream();
				swiWaitForVBlank();
			}
		}
		printLarge(false, 16, 12, "Error code: BINF");
		printSmallCentered(false, 64, "The GBA BIOS is required");
		printSmallCentered(false, 78, "to run GBA games.");
		printSmallCentered(false, 112, "Please place the BIOS on the");
		printSmallCentered(false, 126, "root as \"bios.bin\".");
		printSmall(false, 208, 160, BUTTON_A " OK");
		int pressed = 0;
		do {
			scanKeys();
			pressed = keysDown();
			checkSdEject();
			tex().drawVolumeImageCached();
			tex().drawBatteryImageCached();
			drawCurrentDate();
			drawCurrentTime();
			drawClockColon();
			snd().updateStream();
			swiWaitForVBlank();
		} while (!(pressed & KEY_A));
		snd().playBack();
		clearText();
		if (!inSelectMenu) {
			showdialogbox = false;
			for (int i = 0; i < 15; i++) {
				snd().updateStream();
				swiWaitForVBlank();
			}
		} else {
			dbox_selectMenu = true;
		}
		return;
	}

	snd().playLaunch();
	controlTopBright = true;

	fadeType = false;		  // Fade to white
	snd().fadeOutStream();
	for (int i = 0; i < 60; i++) {
		snd().updateStream();
		swiWaitForVBlank();
	}
	
	mmEffectCancelAll();
	snd().stopStream();

	ms().saveSettings();

	// Switch to GBA mode
	if (ms().useGbarunner) {
		if (ms().secondaryDevice) {
			if (ms().useBootstrap) {
				int err = runNdsFile(ms().gbar2WramICache ? "fat:/_nds/GBARunner2_fc_wramicache.nds" : "fat:/_nds/GBARunner2_fc.nds", 0, NULL, true, true, true, true, false);
				iprintf("Start failed. Error %i\n", err);
			} else {
				loadGameOnFlashcard((ms().gbar2WramICache ? "fat:/_nds/GBARunner2_fc_wramicache.nds" : "fat:/_nds/GBARunner2_fc.nds"),
									(ms().gbar2WramICache ? "GBARunner2_fc_wramicache.nds" : "GBARunner2_fc.nds"), false);
			}
		} else {
			std::string bootstrapPath = (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds"
								: "sd:/_nds/nds-bootstrap-hb-release.nds");

			std::vector<char*> argarray;
			argarray.push_back(strdup(bootstrapPath.c_str()));
			argarray.at(0) = (char*)bootstrapPath.c_str();

			CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");
			bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", ms().gbar2WramICache ? "sd:/_nds/GBARunner2_wramicache.nds" : "sd:/_nds/GBARunner2.nds");
			bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "");
			bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", "");
			bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().bstrap_language);
			bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
			bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
			bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);
			bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
			int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], false, true, false, true, true);
			char text[32];
			snprintf(text, sizeof(text), "Start failed. Error %i", err);
			fadeType = true;
			printLarge(false, 4, 4, text);
			if (err == 1) {
				printLarge(false, 4, 20, ms().bootstrapFile ? "nds-bootstrap (Nightly) not found." : "nds-bootstrap (Release) not found.");
			}
			stop();
		}
	} else {
		gbaSwitch();
	}
}

void smsWarning(void) {
	if (ms().theme == 4) {
		snd().playStartup();
		fadeType = false;	   // Fade to black
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
		currentBg = 1;
		displayGameIcons = false;
		fadeType = true;
	} else {
		showdialogbox = true;
	}
	clearText();
	if (ms().theme == 4) {
		while (!screenFadedIn()) { swiWaitForVBlank(); }
	} else {
		for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
	}
	printSmallCentered(false, 64, "When the game starts, please");
	printSmallCentered(false, 78, "touch the screen to go into");
	printSmallCentered(false, 92, "the menu, and exit out of it");
	printSmallCentered(false, 106, "for the sound to work.");
	printSmall(false, 208, 160, BUTTON_A " OK");
	int pressed = 0;
	do {
		scanKeys();
		pressed = keysDown();
		checkSdEject();
		tex().drawVolumeImageCached();
		tex().drawBatteryImageCached();
		drawCurrentTime();
		drawCurrentDate();
		drawClockColon();
		snd().updateStream();
		swiWaitForVBlank();
	} while (!(pressed & KEY_A));
	showdialogbox = false;
	if (ms().theme == 4) {
		fadeType = false;	   // Fade to black
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
		clearText();
		currentBg = 0;
		displayGameIcons = true;
		fadeType = true;
		snd().playStartup();
		while (!screenFadedIn()) { swiWaitForVBlank(); }
	} else {
		clearText();
		for (int i = 0; i < 15; i++) { snd().updateStream(); swiWaitForVBlank(); }
	}
}

void mdRomTooBig(void) {
	// int bottomBright = 0;

	if (ms().theme == 4) {
		snd().playStartup();
		fadeType = false;	   // Fade to black
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
		currentBg = 1;
		displayGameIcons = false;
		fadeType = true;
	} else {
		snd().playWrong();
		dbox_showIcon = true;
		showdialogbox = true;
	}
	clearText();
	if (ms().theme == 4) {
		while (!screenFadedIn()) { swiWaitForVBlank(); }
		snd().playWrong();
	} else {
		for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
	}
	printSmallCentered(false, 64, "This Mega Drive or Genesis");
	printSmallCentered(false, 78, "ROM cannot be launched,");
	printSmallCentered(false, 92, "due to the size of it");
	printSmallCentered(false, 106, "being above 3MB.");
	printSmall(false, 208, 160, BUTTON_A " OK");
	int pressed = 0;
	do {
		scanKeys();
		pressed = keysDown();
		checkSdEject();
		tex().drawVolumeImageCached();
		tex().drawBatteryImageCached();
		drawCurrentTime();
		drawCurrentDate();
		drawClockColon();
		snd().updateStream();
		swiWaitForVBlank();

		// Debug code for changing brightness of BG layer

		/*if (pressed & KEY_UP) {
			bottomBright--;
		} else if (pressed & KEY_DOWN) {
			bottomBright++;
		}
		


		if (bottomBright < 0) bottomBright = 0;
		if (bottomBright > 15) bottomBright = 15;

		switch (bottomBright) {
			case 0:
			default:
				REG_BLDY = 0;
				break;
			case 1:
				REG_BLDY = (0b0001 << 1);
				break;
			case 2:
				REG_BLDY = (0b0010 << 1);
				break;
			case 3:
				REG_BLDY = (0b0011 << 1);
				break;
			case 4:
				REG_BLDY = (0b0100 << 1);
				break;
			case 5:
				REG_BLDY = (0b0101 << 1);
				break;
			case 6:
				REG_BLDY = (0b0110 << 1);
				break;
			case 7:
				REG_BLDY = (0b0111 << 1);
				break;
			case 8:
				REG_BLDY = (0b1000 << 1);
				break;
			case 9:
				REG_BLDY = (0b1001 << 1);
				break;
			case 10:
				REG_BLDY = (0b1010 << 1);
				break;
			case 11:
				REG_BLDY = (0b1011 << 1);
				break;
			case 12:
				REG_BLDY = (0b1100 << 1);
				break;
			case 13:
				REG_BLDY = (0b1101 << 1);
				break;
			case 14:
				REG_BLDY = (0b1110 << 1);
				break;
			case 15:
				REG_BLDY = (0b1111 << 1);
				break;
		}*/
	} while (!(pressed & KEY_A));
	snd().playBack();
	showdialogbox = false;
	if (ms().theme == 4) {
		fadeType = false;	   // Fade to black
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
		clearText();
		currentBg = 0;
		displayGameIcons = true;
		fadeType = true;
		snd().playStartup();
	} else {
		clearText();
	}
}

bool selectMenu(void) {
	inSelectMenu = true;
	dbox_showIcon = false;
	if (ms().theme == 4) {
		snd().playStartup();
		fadeType = false;	   // Fade to black
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
		currentBg = 1;
		displayGameIcons = false;
		fadeType = true;
	} else {
		dbox_selectMenu = true;
		showdialogbox = true;
	}
	clearText();
	if (!rocketVideo_playVideo || ms().showBoxArt)
		clearBoxArt(); // Clear box art
	boxArtLoaded = false;
	rocketVideo_playVideo = true;
	int maxCursors = 0;
	int selCursorPosition = 0;
	int assignedOp[5] = {-1};
	int selIconYpos = 96;
	if (isDSiMode() && sdFound()) {
		for (int i = 0; i < 5; i++) {
			selIconYpos -= 14;
		}
		assignedOp[0] = 0;
		assignedOp[1] = 1;
		assignedOp[2] = 2;
		assignedOp[3] = 3;
		assignedOp[4] = 4;
		maxCursors = 4;
	} else {
		for (int i = 0; i < 4; i++) {
			selIconYpos -= 14;
		}
		if (!sys().isRegularDS()) {
			assignedOp[0] = 0;
			assignedOp[1] = 1;
			assignedOp[2] = 3;
			assignedOp[3] = 4;
			maxCursors = 3;
		} else {
			assignedOp[0] = 1;
			assignedOp[1] = 3;
			assignedOp[2] = 4;
			maxCursors = 2;
		}
	}
	if (ms().theme == 4) {
		while (!screenFadedIn()) { swiWaitForVBlank(); }
		dbox_selectMenu = true;
	} else {
		for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
	}
	int pressed = 0;
	while (1) {
		int textYpos = selIconYpos + 4;
		clearText();
		printSmallCentered(false, (ms().theme == 4 ? 8 : 16), "SELECT menu");
		printSmall(false, 24, -2 + textYpos + (28 * selCursorPosition), ">");
		for (int i = 0; i <= maxCursors; i++) {
			if (assignedOp[i] == 0) {
				printSmall(false, 64, textYpos, (ms().consoleModel < 2) ? "DSi Menu" : "3DS HOME Menu");
			} else if (assignedOp[i] == 1) {
				printSmall(false, 64, textYpos, "TWLMenu++ Settings");
			} else if (assignedOp[i] == 2) {
				if (bothSDandFlashcard()) {
					if (ms().secondaryDevice) {
						if (ms().consoleModel < 3) {
							printSmall(false, 64, textYpos, "Switch to SD Card");
						} else {
							printSmall(false, 64, textYpos, "Switch to microSD Card");
						}
					} else {
						printSmall(false, 64, textYpos, "Switch to Slot-1 microSD");
					}
				} else {
					printSmall(false, 64, textYpos,
						   (REG_SCFG_MC == 0x11) ? "No Slot-1 card inserted"
									 : "Launch Slot-1 card");
				}
			} else if (assignedOp[i] == 3) {
				printSmall(false, 64, textYpos,
					   ms().useGbarunner ? "Start GBARunner2" : "Start GBA Mode");
			} else if (assignedOp[i] == 4) {
				printSmall(false, 64, textYpos, "Open Manual");
			}
			textYpos += 28;
		}
		printSmallCentered(false, (ms().theme == 4 ? 164 : 160), "SELECT/" BUTTON_B " Back, " BUTTON_A " Select");
		scanKeys();
		pressed = keysDown();
		checkSdEject();
		tex().drawVolumeImageCached();
		tex().drawBatteryImageCached();
		drawCurrentTime();
		drawCurrentDate();
		drawClockColon();
		snd().updateStream();
		swiWaitForVBlank();
		if (pressed & KEY_UP) {
			snd().playSelect();
			selCursorPosition--;
			if (selCursorPosition < 0)
				selCursorPosition = maxCursors;
		}
		if (pressed & KEY_DOWN) {
			snd().playSelect();
			selCursorPosition++;
			if (selCursorPosition > maxCursors)
				selCursorPosition = 0;
		}
		if (pressed & KEY_A) {
			switch (assignedOp[selCursorPosition]) {
			case 0:
			default:
				exitToSystemMenu();
				break;
			case 1:
				launchSettings();
				break;
			case 2:
				if (REG_SCFG_MC != 0x11) {
					switchDevice();
					inSelectMenu = false;
					return true;
				} else {
					snd().playWrong();
				}
				break;
			case 3:
				launchGba();
				break;
			case 4:
				launchManual();
				break;
			}
		}
		if ((pressed & KEY_B) || (pressed & KEY_SELECT)) {
			snd().playBack();
			break;
		}
	};
	showdialogbox = false;
	if (ms().theme == 4) {
		fadeType = false;	   // Fade to black
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
		clearText();
		dbox_selectMenu = false;
		inSelectMenu = false;
		currentBg = 0;
		displayGameIcons = true;
		fadeType = true;
		snd().playStartup();
	} else {
		clearText();
		inSelectMenu = false;
	}
	return false;
}

void getFileInfo(SwitchState scrn, vector<vector<DirEntry>> dirContents, bool reSpawnBoxes) {
	extern bool extention(const std::string& filename, const char* ext);

	if (reSpawnBoxes)
		spawnedtitleboxes = 0;
	for (int i = 0; i < 40; i++) {
		if (i + PAGENUM * 40 < file_count) {
			if (dirContents[scrn].at(i + PAGENUM * 40).isDirectory) {
				isDirectory[i] = true;
				bnrWirelessIcon[i] = 0;
			} else {
				isDirectory[i] = false;
				std::string std_romsel_filename = dirContents[scrn].at(i + PAGENUM * 40).name.c_str();

				if (extention(std_romsel_filename, ".nds")
				 || extention(std_romsel_filename, ".dsi")
				 || extention(std_romsel_filename, ".ids")
				 || extention(std_romsel_filename, ".app")
				 || extention(std_romsel_filename, ".argv"))
				{
					getGameInfo(isDirectory[i], dirContents[scrn].at(i + PAGENUM * 40).name.c_str(),
						    i);
					bnrRomType[i] = 0;
				} else if (extention(std_romsel_filename, ".plg") || extention(std_romsel_filename, ".rvid")) {
					bnrRomType[i] = 9;
				} else if (extention(std_romsel_filename, ".gba")) {
					bnrRomType[i] = 1;
				} else if (extention(std_romsel_filename, ".gb") || extention(std_romsel_filename, ".sgb")) {
					bnrRomType[i] = 2;
				} else if (extention(std_romsel_filename, ".gbc")) {
					bnrRomType[i] = 3;
				} else if (extention(std_romsel_filename, ".nes") || extention(std_romsel_filename, ".fds")) {
					bnrRomType[i] = 4;
				} else if (extention(std_romsel_filename, ".sms")) {
					bnrRomType[i] = 5;
				} else if (extention(std_romsel_filename, ".gg")) {
					bnrRomType[i] = 6;
				} else if (extention(std_romsel_filename, ".gen")) {
					bnrRomType[i] = 7;
				} else if (extention(std_romsel_filename, ".smc") || extention(std_romsel_filename, ".sfc")) {
					bnrRomType[i] = 8;
				}

				if (bnrRomType[i] > 0 && bnrRomType[i] < 10) {
					bnrWirelessIcon[i] = 0;
					isDSiWare[i] = false;
					isHomebrew[i] = 0;
				}

				if (ms().showBoxArt && !isDirectory[i] && isDSiMode()) {
					snprintf(boxArtPath, sizeof(boxArtPath),
						 (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png"
								: "fat:/_nds/TWiLightMenu/boxart/%s.png"),
						 dirContents[scrn].at(i + PAGENUM * 40).name.c_str());
					if ((bnrRomType[i] == 0) && (access(boxArtPath, F_OK) != 0)) {
						snprintf(boxArtPath, sizeof(boxArtPath),
							 (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png"
									: "fat:/_nds/TWiLightMenu/boxart/%s.png"),
							 gameTid[i]);
					}
					tex().loadBoxArtToMem(boxArtPath, i);
				}
			}
			if (reSpawnBoxes)
				spawnedtitleboxes++;

			checkSdEject();
			tex().drawVolumeImageCached();
			tex().drawBatteryImageCached();
			drawCurrentTime();
			drawCurrentDate();
			drawClockColon();
			snd().updateStream();
		}
	}
	if (nowLoadingDisplaying) {
		snd().updateStream();
		showProgressIcon = false;
		if (ms().theme != 4) fadeType = false; // Fade to white
	}
	// Load correct icons depending on cursor position
	if (CURPOS <= 1) {
		for (int i = 0; i < 5; i++) {
			if (bnrRomType[i] == 0 && i + PAGENUM * 40 < file_count) {
				snd().updateStream();
				swiWaitForVBlank();
				iconUpdate(dirContents[scrn].at(i + PAGENUM * 40).isDirectory,
					   dirContents[scrn].at(i + PAGENUM * 40).name.c_str(), i);
			}
		}
	} else if (CURPOS >= 2 && CURPOS <= 36) {
		for (int i = 0; i < 6; i++) {
			if (bnrRomType[i] == 0 && (CURPOS - 2 + i) + PAGENUM * 40 < file_count) {
				snd().updateStream();
				swiWaitForVBlank();
				iconUpdate(dirContents[scrn].at((CURPOS - 2 + i) + PAGENUM * 40).isDirectory,
					   dirContents[scrn].at((CURPOS - 2 + i) + PAGENUM * 40).name.c_str(),
					   CURPOS - 2 + i);
			}
		}
	} else if (CURPOS >= 37 && CURPOS <= 39) {
		for (int i = 0; i < 5; i++) {
			if (bnrRomType[i] == 0 && (35 + i) + PAGENUM * 40 < file_count) {
				snd().updateStream();
				swiWaitForVBlank();
				iconUpdate(dirContents[scrn].at((35 + i) + PAGENUM * 40).isDirectory,
					   dirContents[scrn].at((35 + i) + PAGENUM * 40).name.c_str(), 35 + i);
			}
		}
	}
}

string browseForFile(const vector<string> extensionList) {
	snd().updateStream();
	displayNowLoading();
	snd().updateStream();
	gameOrderIniPath =
	    sdFound() ? "sd:/_nds/TWiLightMenu/extras/gameorder.ini" : "fat:/_nds/TWiLightMenu/extras/gameorder.ini";
	recentlyPlayedIniPath =
	    sdFound() ? "sd:/_nds/TWiLightMenu/extras/recentlyplayed.ini" : "fat:/_nds/TWiLightMenu/extras/recentlyplayed.ini";
	timesPlayedIniPath =
	    sdFound() ? "sd:/_nds/TWiLightMenu/extras/timesplayed.ini" : "fat:/_nds/TWiLightMenu/extras/timesplayed.ini";
	hiddenGamesIniPath = sdFound() ? "sd:/_nds/TWiLightMenu/extras/hiddengames.ini"
				       : "fat:/_nds/TWiLightMenu/extras/hiddengames.ini";

	bool displayBoxArt = ms().showBoxArt;

	int pressed = 0;
	int held = 0;
	SwitchState scrn(3);
	vector<vector<DirEntry>> dirContents(scrn.SIZE);

	getDirectoryContents(dirContents[scrn], extensionList);

	while (1) {
		snd().updateStream();
		updateDirectoryContents(dirContents[scrn]);
		getFileInfo(scrn, dirContents, true);
		reloadIconPalettes();
		reloadFontPalettes();
		if (ms().theme != 4) {
			while (!screenFadedOut())
				;
		}
		nowLoadingDisplaying = false;
		whiteScreen = false;
		displayGameIcons = true;
		fadeType = true; // Fade in from white
		for (int i = 0; i < 5; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		clearText(false);
		snd().updateStream();
		waitForFadeOut();
		bool gameTapped = false;
		/* clearText(false);
		updatePath();
		TextPane *pane = &createTextPane(20, 3 + ENTRIES_START_ROW*FONT_SY, ENTRIES_PER_SCREEN);
		for (auto &i : dirContents[scrn])
			pane->addLine(i.visibleName.c_str());
		pane->createDefaultEntries();
		pane->slideTransition(true);

		printSmall(false, 12 - 16, 4 + 10 * (CURPOS - screenOffset +
		ENTRIES_START_ROW), ">"); TextEntry *cursor = getPreviousTextEntry(false); cursor->fade =
		TextEntry::FadeType::IN; cursor->finalX += 16; */

		while (1) {
			// cursor->finalY = 4 + 10 * (CURPOS - screenOffset +
			// ENTRIES_START_ROW); cursor->delay = TextEntry::ACTIVE;
			snd().updateStream();
			if (!stopSoundPlayed) {
				if ((ms().theme == 0 && !startMenu &&
				     CURPOS + PAGENUM * 40 <= ((int)dirContents[scrn].size() - 1)) ||
				    (ms().theme == 0 && startMenu &&
				     ms().startMenu_cursorPosition < (3 - flashcardFound()))) {
					needToPlayStopSound = true;
				}
				stopSoundPlayed = true;
			}

			if (!shouldersRendered) {
				showLshoulder = false;
				showRshoulder = false;
				if (PAGENUM != 0) {
					showLshoulder = true;
				}
				if (file_count > 40 + PAGENUM * 40) {
					showRshoulder = true;
				}
				tex().drawShoulders(showLshoulder, showRshoulder);
				shouldersRendered = true;
			}

			// u8 current_SCFG_MC = REG_SCFG_MC;

			// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing
			// else to do
			do {
				scanKeys();
				pressed = keysDown();
				held = keysDownRepeat();
				touchRead(&touch);
				updateScrollingState(held, pressed);
				snd().updateStream();

				if (isScrolling) {
					if (boxArtLoaded) {
						if (!rocketVideo_playVideo)
							clearBoxArt();
						rocketVideo_playVideo = (ms().theme == 1 ? true : false);
					}
				} else {
					updateBoxArt(dirContents, scrn);
					if (ms().theme < 4) {
						while (dboxInFrame) {
							snd().updateStream();
							swiWaitForVBlank();
						}
					}
					dbox_showIcon = false;
					dbox_selectMenu = false;
				}
				if (CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
					currentBg = (ms().theme == 4 ? 0 : 1), displayBoxArt = ms().showBoxArt;
					titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
						    dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(), CURPOS);
					bannerTextShown = true;
				} else {
					if (displayBoxArt && !rocketVideo_playVideo) {
						clearBoxArt();
						displayBoxArt = false;
					}
					bannerTextShown = false;
					clearText(false);
					currentBg = 0;
					showSTARTborder = rocketVideo_playVideo = (ms().theme == 1 ? true : false);
				}
				buttonArrowTouched[0] = ((keysHeld() & KEY_TOUCH) && touch.py > 171 && touch.px < 19);
				buttonArrowTouched[1] = ((keysHeld() & KEY_TOUCH) && touch.py > 171 && touch.px > 236);
				checkSdEject();
				tex().drawVolumeImageCached();
				tex().drawBatteryImageCached();
				drawCurrentTime();
				drawCurrentDate();
				drawClockColon();
				snd().updateStream();
				swiWaitForVBlank();
				/*if (REG_SCFG_MC != current_SCFG_MC) {
					break;
				}*/
			} while (!pressed && !held);

			buttonArrowTouched[0] = ((keysHeld() & KEY_TOUCH) && touch.py > 171 && touch.px < 19);
			buttonArrowTouched[1] = ((keysHeld() & KEY_TOUCH) && touch.py > 171 && touch.px > 236);

			if (((pressed & KEY_LEFT) && !titleboxXmoveleft && !titleboxXmoveright) ||
			    ((held & KEY_LEFT) && !titleboxXmoveleft && !titleboxXmoveright) ||
			    ((pressed & KEY_TOUCH) && touch.py > 171 && touch.px < 19 && ms().theme == 0 &&
			     !titleboxXmoveleft && !titleboxXmoveright) || // Button arrow (DSi theme)
			    ((held & KEY_TOUCH) && touch.py > 171 && touch.px < 19 && ms().theme == 0 &&
			     !titleboxXmoveleft && !titleboxXmoveright)) // Button arrow held (DSi theme)
			{
				CURPOS -= 1;
				if (CURPOS >= 0) {
					titleboxXmoveleft = true;
					waitForNeedToPlayStopSound = 1;
					snd().playSelect();
					boxArtLoaded = false;
					settingsChanged = true;
				} else if (!edgeBumpSoundPlayed) {
					snd().playWrong();
					edgeBumpSoundPlayed = true;
				}
				if (CURPOS >= 2 && CURPOS <= 36) {
					if (bnrRomType[CURPOS - 2] == 0 && (CURPOS - 2) + PAGENUM * 40 < file_count) {
						iconUpdate(
						    dirContents[scrn].at((CURPOS - 2) + PAGENUM * 40).isDirectory,
						    dirContents[scrn].at((CURPOS - 2) + PAGENUM * 40).name.c_str(),
						    CURPOS - 2);
						defer(reloadFontTextures);
					}
				}
			} else if (((pressed & KEY_RIGHT) && !titleboxXmoveleft && !titleboxXmoveright) ||
				   ((held & KEY_RIGHT) && !titleboxXmoveleft && !titleboxXmoveright) ||
				   ((pressed & KEY_TOUCH) && touch.py > 171 && touch.px > 236 && ms().theme == 0 &&
				    !titleboxXmoveleft && !titleboxXmoveright) || // Button arrow (DSi theme)
				   ((held & KEY_TOUCH) && touch.py > 171 && touch.px > 236 && ms().theme == 0 &&
				    !titleboxXmoveleft && !titleboxXmoveright)) // Button arrow held (DSi theme)
			{
				CURPOS += 1;
				if (CURPOS <= 39) {
					titleboxXmoveright = true;
					waitForNeedToPlayStopSound = 1;
					snd().playSelect();
					boxArtLoaded = false;
					settingsChanged = true;
				} else if (!edgeBumpSoundPlayed) {
					snd().playWrong();
					edgeBumpSoundPlayed = true;
				}

				if (CURPOS >= 3 && CURPOS <= 37) {
					if (bnrRomType[CURPOS + 2] == 0 && (CURPOS + 2) + PAGENUM * 40 < file_count) {
						iconUpdate(
						    dirContents[scrn].at((CURPOS + 2) + PAGENUM * 40).isDirectory,
						    dirContents[scrn].at((CURPOS + 2) + PAGENUM * 40).name.c_str(),
						    CURPOS + 2);
						defer(reloadFontTextures);
					}
				}
				// Move apps
			} else if ((pressed & KEY_UP) && (ms().theme != 4) && !dirInfoIniFound && (ms().sortMethod == 4)
				   && !titleboxXmoveleft && !titleboxXmoveright &&CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
				showSTARTborder = false;
				currentBg = 2;
				clearText();
				mkdir(sdFound() ? "sd:/_nds/TWiLightMenu/extras" : "fat:/_nds/TWiLightMenu/extras",
				      0777);
				std::string gameBeingMoved = dirContents[scrn].at((PAGENUM * 40) + (CURPOS)).name;
				movingApp = (PAGENUM * 40) + (CURPOS);
				if (dirContents[scrn][movingApp].isDirectory)
					movingAppIsDir = true;
				else
					movingAppIsDir = false;
				getGameInfo(dirContents[scrn].at(movingApp).isDirectory,
					    dirContents[scrn].at(movingApp).name.c_str(), -1);
				iconUpdate(dirContents[scrn].at(movingApp).isDirectory,
					   dirContents[scrn].at(movingApp).name.c_str(), -1);
				for (int i = 0; i < 10; i++) {
					if (i == 9) {
						movingAppYpos += 2;
					} else {
						movingAppYpos += 8;
					}
					snd().updateStream();
					swiWaitForVBlank();
				}
				int orgCursorPosition = CURPOS;
				int orgPage = PAGENUM;
				showMovingArrow = true;

				while (1) {
					scanKeys();
					pressed = keysDown();
					held = keysDownRepeat();
					checkSdEject();
					tex().drawVolumeImageCached();
					tex().drawBatteryImageCached();
					drawCurrentTime();
					drawCurrentDate();
					drawClockColon();
					snd().updateStream();
					swiWaitForVBlank();

					// RocketVideo video extraction
					/*if (pressed & KEY_X) {
						FILE* destinationFile =
					fopen("sd:/_nds/TWiLightMenu/extractedvideo.rvid", "wb");
						fwrite((void*)0x02800000, 1, 0x6A0000, destinationFile);
						fclose(destinationFile);
					}*/

					if ((pressed & KEY_LEFT && !titleboxXmoveleft && !titleboxXmoveright) ||
					    (held & KEY_LEFT && !titleboxXmoveleft && !titleboxXmoveright)) {
						if (CURPOS > 0) {
							snd().playSelect();
							titleboxXmoveleft = true;
							CURPOS--;
							if (bnrRomType[CURPOS + 2] == 0 &&
							    (CURPOS + 2) + PAGENUM * 40 < file_count && CURPOS >= 2 &&
							    CURPOS <= 36) {
								iconUpdate(dirContents[scrn]
									       .at((CURPOS - 2) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((CURPOS - 2) + PAGENUM * 40)
									       .name.c_str(),
									   CURPOS - 2);
								defer(reloadFontTextures);
							}
						} else if (!edgeBumpSoundPlayed) {
							snd().playWrong();
							edgeBumpSoundPlayed = true;
						}
					} else if ((pressed & KEY_RIGHT && !titleboxXmoveleft && !titleboxXmoveright) ||
						   (held & KEY_RIGHT && !titleboxXmoveleft && !titleboxXmoveright)) {
						if (CURPOS + (PAGENUM * 40) < (int)dirContents[scrn].size() - 1 &&
						    CURPOS < 39) {
							snd().playSelect();
							titleboxXmoveright = true;
							CURPOS++;
							if (bnrRomType[CURPOS + 2] == 0 &&
							    (CURPOS + 2) + PAGENUM * 40 < file_count && CURPOS >= 3 &&
							    CURPOS <= 37) {
								iconUpdate(dirContents[scrn]
									       .at((CURPOS + 2) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((CURPOS + 2) + PAGENUM * 40)
									       .name.c_str(),
									   CURPOS + 2);
								defer(reloadFontTextures);
							}
						} else if (!edgeBumpSoundPlayed) {
							snd().playWrong();
							edgeBumpSoundPlayed = true;
						}
					} else if (pressed & KEY_DOWN) {
						for (int i = 0; i < 10; i++) {
							showMovingArrow = false;
							movingArrowYpos = 59;
							if (i == 9) {
								movingAppYpos -= 2;
							} else {
								movingAppYpos -= 8;
							}
							snd().updateStream();
							swiWaitForVBlank();
						}
						break;
					} else if (pressed & KEY_L) {
						if (!startMenu && !titleboxXmoveleft && !titleboxXmoveright &&
						    PAGENUM != 0) {
							snd().playSwitch();
							fadeType = false; // Fade to white
							for (int i = 0; i < 30; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							PAGENUM -= 1;
							CURPOS = 0;
							titleboxXpos[ms().secondaryDevice] = 0;
							titlewindowXpos[ms().secondaryDevice] = 0;
							whiteScreen = true;
							shouldersRendered = false;
							displayNowLoading();
							getDirectoryContents(dirContents[scrn], extensionList);
							getFileInfo(scrn, dirContents, true);

							while (!screenFadedOut()) {
								snd().updateStream();
							}
							nowLoadingDisplaying = false;
							whiteScreen = false;
							displayGameIcons = true;
							fadeType = true; // Fade in from white
							for (int i = 0; i < 5; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							reloadIconPalettes();
							reloadFontPalettes();
							clearText();
						} else {
							snd().playWrong();
						}
					} else if (pressed & KEY_R) {
						if (!startMenu && !titleboxXmoveleft && !titleboxXmoveright &&
						    file_count > 40 + PAGENUM * 40) {
							snd().playSwitch();
							fadeType = false; // Fade to white
							for (int i = 0; i < 30; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							PAGENUM += 1;
							CURPOS = 0;
							titleboxXpos[ms().secondaryDevice] = 0;
							titlewindowXpos[ms().secondaryDevice] = 0;
							whiteScreen = true;
							shouldersRendered = false;
							displayNowLoading();
							getDirectoryContents(dirContents[scrn], extensionList);
							getFileInfo(scrn, dirContents, true);

							while (!screenFadedOut()) { snd().updateStream(); }
								
							nowLoadingDisplaying = false;
							whiteScreen = false;
							displayGameIcons = true;
							fadeType = true; // Fade in from white
							for (int i = 0; i < 5; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							reloadIconPalettes();
							reloadFontPalettes();
							clearText();
						} else {
							snd().playWrong();
						}
					}
				}
				if ((PAGENUM != orgPage) || (CURPOS != orgCursorPosition)) {
					currentBg = 1;
					writeBannerText(0, "Please wait...", "", "");

					CIniFile gameOrderIni(gameOrderIniPath);
					vector<std::string> gameOrder;
					char str[12] = {0};

					for (int i = 0; i < (int)dirContents[scrn].size(); i++) {
						sprintf(str, "%d", i);
						gameOrder.push_back(
						    gameOrderIni.GetString(getcwd(path, PATH_MAX), str, "NULL"));
						snd().updateStream();
						if (gameOrder[i] == "NULL" || ms().sortMethod != 3)
							gameOrder[i] = dirContents[scrn][i].name;
					}

					for (int i = 0; i < (int)gameOrder.size(); i++) {
						for (int j = 0; j < (int)gameOrder.size(); j++) {
							if (i != j) {
								if (gameOrder[i] == gameOrder[j]) {
									gameOrder.erase(gameOrder.begin() + j);
								}
							}
						}
					}

					for (int i = gameOrder.size(); true; i++) {
						sprintf(str, "%d", i);
						if (gameOrderIni.GetString(getcwd(path, PATH_MAX), str, "") != "") {
							gameOrderIni.SetString(getcwd(path, PATH_MAX), str, "");
							snd().updateStream();
						} else {
							break;
						}
					}

					for (int i = 0; i < (int)gameOrder.size(); i++) {
						bool stillExists = false;
						for (int j = 0; j < (int)dirContents[scrn].size(); j++) {
							if (gameOrder[i] == dirContents[scrn][j].name) {
								stillExists = true;
								break;
							}
						}
						if (!stillExists)
							gameOrder.erase(gameOrder.begin() + i);
					}

					gameOrder.erase(gameOrder.begin() + movingApp);
					gameOrder.insert(gameOrder.begin() + CURPOS + (PAGENUM * 40), gameBeingMoved);

					for (int i = 0; i < (int)gameOrder.size(); i++) {
						char str[12] = {0};
						sprintf(str, "%d", i);
						gameOrderIni.SetString(getcwd(path, PATH_MAX), str, gameOrder[i]);
						snd().updateStream();
					}
					snd().updateStream();
					gameOrderIni.SaveIniFile(gameOrderIniPath);

					ms().sortMethod = 4;
					ms().saveSettings();
					snd().updateStream();

					// getDirectoryContents(dirContents[scrn], extensionList);

					DirEntry dirEntry;

					dirEntry.name = gameBeingMoved.c_str();
					dirEntry.isDirectory = movingAppIsDir;

					dirContents[scrn].erase(dirContents[scrn].begin() + movingApp);
					dirContents[scrn].insert(dirContents[scrn].begin() + CURPOS + (PAGENUM * 40),
								 dirEntry);

					getFileInfo(scrn, dirContents, false);

					settingsChanged = true;
					boxArtLoaded = false;
				}
				movingApp = -1;

				// Scrollbar
			} else if (((pressed & KEY_TOUCH) && touch.py > 171 && touch.px >= 30 && touch.px <= 227 &&
				    ms().theme == 0 && !titleboxXmoveleft &&
				    !titleboxXmoveright)) // Scroll bar (DSi theme))
			{
				touchPosition startTouch = touch;
				int prevPos = CURPOS;
				showSTARTborder = false;
				scrollWindowTouched = true;
				while (1) {
					scanKeys();
					touchRead(&touch);

					if (!(keysHeld() & KEY_TOUCH))
						break;

					CURPOS = round((touch.px - 30) / 4.925);
					if (CURPOS > 39) {
						CURPOS = 39;
						titlewindowXpos[ms().secondaryDevice] = 192.075;
						titleboxXpos[ms().secondaryDevice] = 2496;
					} else if (CURPOS < 0) {
						CURPOS = 0;
						titlewindowXpos[ms().secondaryDevice] = 0;
						titleboxXpos[ms().secondaryDevice] = 0;
					} else {
						titlewindowXpos[ms().secondaryDevice] = touch.px - 30;
						titleboxXpos[ms().secondaryDevice] = (touch.px - 30) / 4.925 * 64;
					}

					// Load icons
					if (prevPos == CURPOS + 1) {
						if (CURPOS >= 2) {
							if (bnrRomType[0] == 0 &&
							    (CURPOS - 2) + PAGENUM * 40 < file_count) {
								iconUpdate(dirContents[scrn]
									       .at((CURPOS - 2) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((CURPOS - 2) + PAGENUM * 40)
									       .name.c_str(),
									   CURPOS - 2);
							}
						}
					} else if (prevPos == CURPOS - 1) {
						if (CURPOS <= 37) {
							if (bnrRomType[0] == 0 &&
							    (CURPOS + 2) + PAGENUM * 40 < file_count) {
								iconUpdate(dirContents[scrn]
									       .at((CURPOS + 2) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((CURPOS + 2) + PAGENUM * 40)
									       .name.c_str(),
									   CURPOS + 2);
							}
						}
					} else if (CURPOS <= 1) {
						for (int i = 0; i < 5; i++) {
							snd().updateStream();
							swiWaitForVBlank();
					
							if (bnrRomType[i] == 0 && i + PAGENUM * 40 < file_count) {
								iconUpdate(
								    dirContents[scrn].at(i + PAGENUM * 40).isDirectory,
								    dirContents[scrn].at(i + PAGENUM * 40).name.c_str(),
								    i);
							}
						}
					} else if (CURPOS >= 2 && CURPOS <= 36) {
						for (int i = 0; i < 6; i++) {
							snd().updateStream();
							swiWaitForVBlank();
							if (bnrRomType[i] == 0 &&
							    (CURPOS - 2 + i) + PAGENUM * 40 < file_count) {
								iconUpdate(dirContents[scrn]
									       .at((CURPOS - 2 + i) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((CURPOS - 2 + i) + PAGENUM * 40)
									       .name.c_str(),
									   CURPOS - 2 + i);
							}
						}
					} else if (CURPOS >= 37 && CURPOS <= 39) {
						for (int i = 0; i < 5; i++) {
							snd().updateStream();
							swiWaitForVBlank();
							if (bnrRomType[i] == 0 &&
							    (35 + i) + PAGENUM * 40 < file_count) {
								iconUpdate(dirContents[scrn]
									       .at((35 + i) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((35 + i) + PAGENUM * 40)
									       .name.c_str(),
									   35 + i);
							}
						}
					}

					clearText();
					if (CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
						currentBg = 1;
						titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
							    dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(),
							    CURPOS);
					} else {
						currentBg = 0;
					}
					prevPos = CURPOS;

					checkSdEject();
					tex().drawVolumeImageCached();
					tex().drawBatteryImageCached();
					drawCurrentTime();
					drawCurrentDate();
					drawClockColon();
					snd().updateStream();
				}
				scrollWindowTouched = false;
				titleboxXpos[ms().secondaryDevice] = CURPOS * 64;
				titlewindowXpos[ms().secondaryDevice] = CURPOS * 5;
				waitForNeedToPlayStopSound = 1;
				snd().playSelect();
				boxArtLoaded = false;
				settingsChanged = true;
				touch = startTouch;
				if (CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size()))
					showSTARTborder = true;

				// Draw icons 1 per vblank to prevent corruption
				if (CURPOS <= 1) {
					for (int i = 0; i < 5; i++) {
						snd().updateStream();
						swiWaitForVBlank();
						if (bnrRomType[i] == 0 && i + PAGENUM * 40 < file_count) {
							iconUpdate(dirContents[scrn].at(i + PAGENUM * 40).isDirectory,
								   dirContents[scrn].at(i + PAGENUM * 40).name.c_str(),
								   i);
						}
					}
				} else if (CURPOS >= 2 && CURPOS <= 36) {
					for (int i = 0; i < 6; i++) {
						snd().updateStream();
						swiWaitForVBlank();
						if (bnrRomType[i] == 0 &&
						    (CURPOS - 2 + i) + PAGENUM * 40 < file_count) {
							iconUpdate(dirContents[scrn]
								       .at((CURPOS - 2 + i) + PAGENUM * 40)
								       .isDirectory,
								   dirContents[scrn]
								       .at((CURPOS - 2 + i) + PAGENUM * 40)
								       .name.c_str(),
								   CURPOS - 2 + i);
						}
					}
				} else if (CURPOS >= 37 && CURPOS <= 39) {
					for (int i = 0; i < 5; i++) {
						snd().updateStream();
						swiWaitForVBlank();
						if (bnrRomType[i] == 0 && (35 + i) + PAGENUM * 40 < file_count) {
							iconUpdate(
							    dirContents[scrn].at((35 + i) + PAGENUM * 40).isDirectory,
							    dirContents[scrn].at((35 + i) + PAGENUM * 40).name.c_str(),
							    35 + i);
						}
					}
				}

				// Dragging icons (DSi & 3DS themes)
			} else if ((pressed & KEY_TOUCH) && touch.py > 88 && touch.py < 144 && !titleboxXmoveleft &&
				    !titleboxXmoveright) {
				touchPosition startTouch = touch;

				if (touch.px > 96 && touch.px < 160) {
					while (1) {
						scanKeys();
						touchRead(&touch);
						snd().updateStream();
						if (!(keysHeld() & KEY_TOUCH)) {
							gameTapped = true;
							break;
						} else if (touch.px < startTouch.px - 10 ||
							   touch.px > startTouch.px + 10)
							break;
					}
				}

				if (ms().theme != 4) {
				touchPosition prevTouch1 = touch;
				touchPosition prevTouch2 = touch;
				int prevPos = CURPOS;
				showSTARTborder = false;

				while (1) {
					if (gameTapped)
						break;
					scanKeys();
					touchRead(&touch);
					snd().updateStream();
					if (!(keysHeld() & KEY_TOUCH)) {
						bool tapped = false;
						int dX = (-(prevTouch1.px - prevTouch2.px));
						int decAmount = abs(dX);
						if (dX > 0) {
							while (decAmount > .25) {
								if (ms().theme &&
								    titleboxXpos[ms().secondaryDevice] > 2496)
									break;
								scanKeys();
								if (keysHeld() & KEY_TOUCH) {
									tapped = true;
									break;
								}

								titlewindowXpos[ms().secondaryDevice] =
								    (titleboxXpos[ms().secondaryDevice] + 32) *
								    0.078125;
								;
								if (titlewindowXpos[ms().secondaryDevice] > 192.075)
									titlewindowXpos[ms().secondaryDevice] = 192.075;

								for (int i = 0; i < 2; i++) {
									snd().updateStream();
									swiWaitForVBlank();
									if (titleboxXpos[ms().secondaryDevice] < 2496)
										titleboxXpos[ms().secondaryDevice] +=
										    decAmount / 2;
									else
										titleboxXpos[ms().secondaryDevice] +=
										    decAmount / 4;
								}
								decAmount = decAmount / 1.25;

								ms().cursorPosition[ms().secondaryDevice] = round(
								    (titleboxXpos[ms().secondaryDevice] + 32) / 64);

								if (CURPOS <= 37) {
									if (bnrRomType[0] == 0 &&
									    (CURPOS + 2) + PAGENUM * 40 < file_count) {
										iconUpdate(
										    dirContents[scrn]
											.at((CURPOS + 2) + PAGENUM * 40)
											.isDirectory,
										    dirContents[scrn]
											.at((CURPOS + 2) + PAGENUM * 40)
											.name.c_str(),
										    CURPOS + 2);
									}
								}
							}
						} else if (dX < 0) {
							while (decAmount > .25) {
								if (ms().theme &&
								    titleboxXpos[ms().secondaryDevice] < 0)
									break;
								scanKeys();
								if (keysHeld() & KEY_TOUCH) {
									touchRead(&touch);
									tapped = true;
									break;
								}

								titlewindowXpos[ms().secondaryDevice] =
								    (titleboxXpos[ms().secondaryDevice] + 32) *
								    0.078125;
								;
								if (titlewindowXpos[ms().secondaryDevice] < 0)
									titlewindowXpos[ms().secondaryDevice] = 0;

								for (int i = 0; i < 2; i++) {
									snd().updateStream();
									swiWaitForVBlank();
									if (titleboxXpos[ms().secondaryDevice] > 0)
										titleboxXpos[ms().secondaryDevice] -=
										    decAmount / 2;
									else
										titleboxXpos[ms().secondaryDevice] -=
										    decAmount / 4;
								}
								decAmount = decAmount / 1.25;

								ms().cursorPosition[ms().secondaryDevice] = round(
								    (titleboxXpos[ms().secondaryDevice] + 32) / 64);

								if (CURPOS >= 2) {
									if (bnrRomType[0] == 0 &&
									    (CURPOS - 2) + PAGENUM * 40 < file_count) {
										iconUpdate(
										    dirContents[scrn]
											.at((CURPOS - 2) + PAGENUM * 40)
											.isDirectory,
										    dirContents[scrn]
											.at((CURPOS - 2) + PAGENUM * 40)
											.name.c_str(),
										    CURPOS - 2);
									}
								}
							}
						}
						if (tapped) {
							prevTouch1 = touch;
							prevTouch2 = touch;
							continue;
						}

						if (CURPOS < 0)
							ms().cursorPosition[ms().secondaryDevice] = 0;
						else if (CURPOS > 39)
							ms().cursorPosition[ms().secondaryDevice] = 39;

						// Load icons
						if (CURPOS <= 1) {
							for (int i = 0; i < 5; i++) {
								snd().updateStream();
								swiWaitForVBlank();
								if (bnrRomType[i] == 0 &&
								    i + PAGENUM * 40 < file_count) {
									iconUpdate(dirContents[scrn]
										       .at(i + PAGENUM * 40)
										       .isDirectory,
										   dirContents[scrn]
										       .at(i + PAGENUM * 40)
										       .name.c_str(),
										   i);
								}
							}
						} else if (CURPOS >= 2 && CURPOS <= 36) {
							for (int i = 0; i < 6; i++) {
								snd().updateStream();
								swiWaitForVBlank();
								if (bnrRomType[i] == 0 &&
								    (CURPOS - 2 + i) + PAGENUM * 40 < file_count) {
									iconUpdate(
									    dirContents[scrn]
										.at((CURPOS - 2 + i) + PAGENUM * 40)
										.isDirectory,
									    dirContents[scrn]
										.at((CURPOS - 2 + i) + PAGENUM * 40)
										.name.c_str(),
									    CURPOS - 2 + i);
								}
							}
						} else if (CURPOS >= 37 && CURPOS <= 39) {
							for (int i = 0; i < 5; i++) {
								snd().updateStream();
								swiWaitForVBlank();
								if (bnrRomType[i] == 0 &&
								    (35 + i) + PAGENUM * 40 < file_count) {
									iconUpdate(dirContents[scrn]
										       .at((35 + i) + PAGENUM * 40)
										       .isDirectory,
										   dirContents[scrn]
										       .at((35 + i) + PAGENUM * 40)
										       .name.c_str(),
										   35 + i);
								}
							}
						}
						break;
					}

					titleboxXpos[ms().secondaryDevice] += (-(touch.px - prevTouch1.px));
					ms().cursorPosition[ms().secondaryDevice] =
					    round((titleboxXpos[ms().secondaryDevice] + 32) / 64);
					titlewindowXpos[ms().secondaryDevice] =
					    (titleboxXpos[ms().secondaryDevice] + 32) * 0.078125;
					if (titleboxXpos[ms().secondaryDevice] > 2496) {
						if (ms().theme)
							titleboxXpos[ms().secondaryDevice] = 2496;
						ms().cursorPosition[ms().secondaryDevice] = 39;
						titlewindowXpos[ms().secondaryDevice] = 192.075;
					} else if (titleboxXpos[ms().secondaryDevice] < 0) {
						if (ms().theme)
							titleboxXpos[ms().secondaryDevice] = 0;
						ms().cursorPosition[ms().secondaryDevice] = 0;
						titlewindowXpos[ms().secondaryDevice] = 0;
					}

					// Load icons
					if (prevPos == CURPOS + 1) {
						if (CURPOS > 2) {
							if (bnrRomType[0] == 0 &&
							    (CURPOS - 2) + PAGENUM * 40 < file_count) {
								iconUpdate(dirContents[scrn]
									       .at((CURPOS - 2) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((CURPOS - 2) + PAGENUM * 40)
									       .name.c_str(),
									   CURPOS - 2);
							}
						}
					} else if (prevPos == CURPOS - 1) {
						if (CURPOS < 37) {
							if (bnrRomType[0] == 0 &&
							    (CURPOS + 2) + PAGENUM * 40 < file_count) {
								iconUpdate(dirContents[scrn]
									       .at((CURPOS + 2) + PAGENUM * 40)
									       .isDirectory,
									   dirContents[scrn]
									       .at((CURPOS + 2) + PAGENUM * 40)
									       .name.c_str(),
									   CURPOS + 2);
							}
						}
					}

					if (prevPos != CURPOS) {
						clearText();
						if (CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
							currentBg = 1;
							titleUpdate(
							    dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
							    dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(),
							    CURPOS);
						} else {
							currentBg = 0;
						}
					}
					prevTouch2 = prevTouch1;
					prevTouch1 = touch;
					prevPos = CURPOS;

					checkSdEject();
					tex().drawVolumeImageCached();
					tex().drawBatteryImageCached();

					drawCurrentTime();
					drawCurrentDate();
					drawClockColon();
					snd().updateStream();
					swiWaitForVBlank();
					snd().updateStream();
					swiWaitForVBlank();
				}
				}	// End of DSi/3DS theme check
				titlewindowXpos[ms().secondaryDevice] = CURPOS * 5;
				;
				titleboxXpos[ms().secondaryDevice] = CURPOS * 64;
				boxArtLoaded = false;
				settingsChanged = true;
				touch = startTouch;
				if (!gameTapped && CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
					showSTARTborder = (ms().theme == 1 ? true : false);
				}
			}

			if (CURPOS < 0)
				ms().cursorPosition[ms().secondaryDevice] = 0;
			else if (CURPOS > 39)
				ms().cursorPosition[ms().secondaryDevice] = 39;

			// Startup...
			if (((pressed & KEY_A) && bannerTextShown && showSTARTborder && !titleboxXmoveleft &&
			     !titleboxXmoveright) ||
			    ((pressed & KEY_START) && bannerTextShown && showSTARTborder && !titleboxXmoveleft &&
			     !titleboxXmoveright) ||
			    (gameTapped && bannerTextShown && showSTARTborder
				// && showSTARTborder && !titleboxXmoveleft &&!titleboxXmoveright
			     )) {
				DirEntry *entry = &dirContents[scrn].at(CURPOS + PAGENUM * 40);
				if (entry->isDirectory) {
					// Enter selected directory
					(ms().theme == 4) ? snd().playLaunch() : snd().playSelect();
					if (ms().theme != 4) {
						fadeType = false; // Fade to white
						for (int i = 0; i < 30; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
					}
					ms().pagenum[ms().secondaryDevice] = 0;
					ms().cursorPosition[ms().secondaryDevice] = 0;
					titleboxXpos[ms().secondaryDevice] = 0;
					titlewindowXpos[ms().secondaryDevice] = 0;
					if (ms().theme != 4) whiteScreen = true;
					if (ms().showBoxArt)
						clearBoxArt(); // Clear box art
					boxArtLoaded = false;
					redoDropDown = true;
					shouldersRendered = false;
					currentBg = 0;
					showSTARTborder = false;
					stopSoundPlayed = false;
					clearText();
					chdir(entry->name.c_str());
					char buf[256];
					ms().romfolder[ms().secondaryDevice] = std::string(getcwd(buf, 256));
					ms().saveSettings();
					settingsChanged = false;
					return "null";
				} else if ((isDSiWare[CURPOS] && !isDSiMode()) || (isDSiWare[CURPOS] && !sdFound()) ||
					   (isDSiWare[CURPOS] && ms().consoleModel > 1)) {
					clearText();
					snd().playWrong();
					if (ms().theme != 4) {
						dbox_showIcon = true;
						showdialogbox = true;
						for (int i = 0; i < 30; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
						titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
								dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(), CURPOS);
					}
					int yPos1 = (ms().theme == 4 ? 24 : 112);
					int yPos2 = (ms().theme == 4 ? 40 : 128);
					printSmallCentered(false, yPos1, "This game cannot be launched");
					if (isDSiMode()) {
						if (sdFound()) {
							printSmallCentered(false, yPos2, "as a .nds file on 3DS/2DS.");
						} else {
							printSmallCentered(false, yPos2, "without an SD card.");
						}
					} else {
						printSmallCentered(false, yPos2, "in DS mode.");
					}
					printSmall(false, 208, (ms().theme == 4 ? 64 : 160), BUTTON_A " OK");
					pressed = 0;
					do {
						scanKeys();
						pressed = keysDown();
						checkSdEject();
						tex().drawVolumeImageCached();
						tex().drawBatteryImageCached();

						drawCurrentTime();
						drawCurrentDate();
						drawClockColon();
						snd().updateStream();
						swiWaitForVBlank();
					} while (!(pressed & KEY_A));
					clearText();
					if (ms().theme == 4) {
						snd().playLaunch();
					} else {
						showdialogbox = false;
					}
				} else {
					bool hasAP = false;
					bool proceedToLaunch = true;
					if (ms().useBootstrap && bnrRomType[CURPOS] == 0 && !isDSiWare[CURPOS] &&
					    isHomebrew[CURPOS] == 0 &&
					    checkIfShowAPMsg(dirContents[scrn].at(CURPOS + PAGENUM * 40).name)) {
						FILE *f_nds_file = fopen(
						    dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(), "rb");
						hasAP = checkRomAP(f_nds_file, CURPOS);
						fclose(f_nds_file);
					} else if (bnrRomType[CURPOS] == 5 || bnrRomType[CURPOS] == 6) {
						smsWarning();
					} else if (bnrRomType[CURPOS] == 7) {
						if (getFileSize(
							dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str()) >
						    0x300000) {
							proceedToLaunch = false;
							mdRomTooBig();
						}
					}
					if (hasAP) {
						if (ms().theme == 4) {
							snd().playStartup();
							fadeType = false;	   // Fade to black
							for (int i = 0; i < 25; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							currentBg = 1;
							displayGameIcons = false;
							fadeType = true;
						} else {
							dbox_showIcon = true;
							showdialogbox = true;
						}
						clearText();
						if (ms().theme == 4) {
							while (!screenFadedIn()) { swiWaitForVBlank(); }
							dbox_showIcon = true;
							snd().playWrong();
						} else {
							for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
						}
						titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
							    dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(),
							    CURPOS);
						printSmallCentered(false, 64, "This game may not work correctly,");
						printSmallCentered(false, 78, "if it's not AP-patched.");
						printSmallCentered(false, 112, "If the game freezes, does not");
						printSmallCentered(false, 126, "start, or doesn't seem normal,");
						printSmallCentered(false, 140, "it needs to be AP-patched.");
						printSmallCentered(false, 160, BUTTON_B "/" BUTTON_A " OK, " BUTTON_X " Don't show again");
						pressed = 0;
						while (1) {
							scanKeys();
							pressed = keysDown();
							checkSdEject();
							tex().drawVolumeImageCached();
							tex().drawBatteryImageCached();

							drawCurrentTime();
							drawCurrentDate();
							drawClockColon();
							snd().updateStream();
							swiWaitForVBlank();
							if (pressed & KEY_A) {
								pressed = 0;
								break;
							}
							if (pressed & KEY_B) {
								snd().playBack();
								proceedToLaunch = false;
								pressed = 0;
								break;
							}
							if (pressed & KEY_X) {
								dontShowAPMsgAgain(
								    dirContents[scrn].at(CURPOS + PAGENUM * 40).name);
								pressed = 0;
								break;
							}
						}
						showdialogbox = false;
						if (ms().theme == 4) {
							fadeType = false;	   // Fade to black
							for (int i = 0; i < 25; i++) {
								swiWaitForVBlank();
							}
							clearText();
							currentBg = 0;
							displayGameIcons = true;
							fadeType = true;
							snd().playStartup();
							if (proceedToLaunch) {
								while (!screenFadedIn()) { swiWaitForVBlank(); }
							}
						} else {
							clearText();
							for (int i = 0; i < (proceedToLaunch ? 20 : 15); i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
						}
						dbox_showIcon = false;
					}

					if (proceedToLaunch) {
						snd().playLaunch();
						controlTopBright = true;
						applaunch = true;

						if (ms().theme == 0) {
							applaunchprep = true;
							currentBg = 0;
							showSTARTborder = false;
							clearText(false); // Clear title

							fadeSpeed = false; // Slow fade speed
							for (int i = 0; i < 5; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
						}
						if (ms().theme != 4) {
							fadeType = false;		  // Fade to white
							snd().fadeOutStream();
							for (int i = 0; i < 60; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}

							mmEffectCancelAll();
							snd().stopStream();

							// Clear screen with white
							rocketVideo_playVideo = false;
							whiteScreen = true;
							tex().clearTopScreen();
						}
						clearText();

						if(ms().updateRecentlyPlayedList) {
							printLargeCentered(false, (ms().theme == 4 ? 32 : 88), "Now Saving...");
							if (ms().theme != 4) {
								fadeSpeed = true; // Fast fading
								fadeType = true; // Fade in from white
								for (int i = 0; i < 25; i++) {
									swiWaitForVBlank();
								}
								showProgressIcon = true;
							}

							mkdir(sdFound() ? "sd:/_nds/TWiLightMenu/extras" : "fat:/_nds/TWiLightMenu/extras",
						  0777);

							CIniFile recentlyPlayedIni(recentlyPlayedIniPath);
							vector<std::string> recentlyPlayed;
							char str[12] = {0};

							for (int i = 0; i < (int)dirContents[scrn].size(); i++) {
								sprintf(str, "%d", i);
								recentlyPlayed.push_back(
										recentlyPlayedIni.GetString(getcwd(path, PATH_MAX), str, "NULL"));
								if (recentlyPlayed[i] == "NULL")
									recentlyPlayed[i] = dirContents[scrn][i].name;
							}
							for (int i = 0; i < (int)recentlyPlayed.size(); i++) {
								for (int j = 0; j < (int)recentlyPlayed.size(); j++) {
									if (i != j) {
										if (recentlyPlayed[i] == recentlyPlayed[j]) {
											recentlyPlayed.erase(recentlyPlayed.begin() + j);
										}
									}
								}
							}
							for (int i = recentlyPlayed.size(); true; i++) {
								sprintf(str, "%d", i);
								if (recentlyPlayedIni.GetString(getcwd(path, PATH_MAX), str, "") != "") {
									recentlyPlayedIni.SetString(getcwd(path, PATH_MAX), str, "");
								} else {
									break;
								}
							}
							for (int i = 0; i < (int)recentlyPlayed.size(); i++) {
								bool stillExists = false;
								for (int j = 0; j < (int)dirContents[scrn].size(); j++) {
									if (recentlyPlayed[i] == dirContents[scrn][j].name) {
										stillExists = true;
										break;
									}
								}
								if (!stillExists)
									recentlyPlayed.erase(recentlyPlayed.begin() + i);
							}

							recentlyPlayed.erase(recentlyPlayed.begin() + CURPOS);
							int firstNonDir = 0;
							while(dirContents[scrn].at(firstNonDir).isDirectory) {
								firstNonDir++;
							}
							recentlyPlayed.insert(recentlyPlayed.begin()+firstNonDir, dirContents[scrn].at((PAGENUM * 40) + (CURPOS)).name);

							for (int i = 0; i < (int)recentlyPlayed.size(); i++) {
								char str[12] = {0};
								sprintf(str, "%d", i);
								recentlyPlayedIni.SetString(getcwd(path, PATH_MAX), str, recentlyPlayed[i]);
							}
							recentlyPlayedIni.SaveIniFile(recentlyPlayedIniPath);

							CIniFile timesPlayedIni(timesPlayedIniPath);
							timesPlayedIni.SetInt(getcwd(path, PATH_MAX),dirContents[scrn].at((PAGENUM * 40) + (CURPOS)).name, (timesPlayedIni.GetInt(getcwd(path, PATH_MAX),dirContents[scrn].at((PAGENUM * 40) + (CURPOS)).name,0) + 1));
							timesPlayedIni.SaveIniFile(timesPlayedIniPath);

							if(ms().sortMethod == 1) {
								ms().cursorPosition[ms().secondaryDevice] = firstNonDir;
							}

							if (ms().theme != 4) {
								showProgressIcon = false;
								fadeType = false;	   // Fade to white
								for (int i = 0; i < 25; i++) {
									swiWaitForVBlank();
								}
							}
							clearText();
						}

						// Return the chosen file
						return entry->name;
					}
				}
			}
			gameTapped = false;

			if (ms().theme == 1) {
				// Launch settings by touching corner button
				if ((pressed & KEY_TOUCH) && touch.py <= 26 && touch.px <= 44 && !titleboxXmoveleft &&
				    !titleboxXmoveright) {
					launchSettings();
				}

				// Exit to system menu by touching corner button
				if ((pressed & KEY_TOUCH) && touch.py <= 26 && touch.px >= 212 &&
				    !sys().isRegularDS() && !titleboxXmoveleft && !titleboxXmoveright) {
					exitToSystemMenu();
				}

				int topIconXpos = 116;
				int savedTopIconXpos[3] = {0};
				if (isDSiMode() && sdFound()) {
					for (int i = 0; i < 2; i++) {
						topIconXpos -= 14;
					}
					for (int i = 0; i < 3; i++) {
						savedTopIconXpos[i] = topIconXpos;
						topIconXpos += 28;
					}
				} else {
					// for (int i = 0; i < 2; i++) {
						topIconXpos -= 14;
					//}
					for (int i = 1; i < 3; i++) {
						savedTopIconXpos[i] = topIconXpos;
						topIconXpos += 28;
					}
				}

				if (isDSiMode() && sdFound()) {
					// Switch devices or launch Slot-1 by touching button
					if ((pressed & KEY_TOUCH) && touch.py <= 26 &&
					    touch.px >= savedTopIconXpos[0] && touch.px < savedTopIconXpos[0] + 24 &&
					    !titleboxXmoveleft && !titleboxXmoveright) {
						if (ms().secondaryDevice || REG_SCFG_MC != 0x11) {
							switchDevice();
							return "null";
						} else {
							snd().playWrong();
						}
					}
				}

				// Launch GBA by touching button
				if ((pressed & KEY_TOUCH) && touch.py <= 26 && touch.px >= savedTopIconXpos[1] &&
				    touch.px < savedTopIconXpos[1] + 24 && !titleboxXmoveleft && !titleboxXmoveright) {
					launchGba();
				}

				// Open the manual
				if ((pressed & KEY_TOUCH) && touch.py <= 26 && touch.px >= savedTopIconXpos[2] &&
				    touch.px < savedTopIconXpos[2] + 24 && !titleboxXmoveleft && !titleboxXmoveright) {
					launchManual();
				}
			}

			// page switch

			if (pressed & KEY_L) {
				if (CURPOS == 0 && !showLshoulder) {
					snd().playWrong();
				} else if (!startMenu && !titleboxXmoveleft && !titleboxXmoveright) {
					snd().playSwitch();
					if (ms().theme != 4) {
						fadeType = false; // Fade to white
						for (int i = 0; i < 30; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
					}
					if (showLshoulder)
						PAGENUM -= 1;
					CURPOS = 0;
					titleboxXpos[ms().secondaryDevice] = 0;
					titlewindowXpos[ms().secondaryDevice] = 0;
					if (ms().theme != 4) whiteScreen = true;
					if (ms().showBoxArt)
						clearBoxArt(); // Clear box art
					boxArtLoaded = false;
					rocketVideo_playVideo = true;
					if (showLshoulder)
						redoDropDown = true;
					shouldersRendered = false;
					currentBg = 0;
					showSTARTborder = false;
					stopSoundPlayed = false;
					clearText();
					ms().saveSettings();
					settingsChanged = false;
					displayNowLoading();
					break;
				}
			} else if (pressed & KEY_R) {
				if (CURPOS == (file_count - 1) - PAGENUM * 40 && !showRshoulder) {
					snd().playWrong();
				} else if (!startMenu && !titleboxXmoveleft && !titleboxXmoveright) {
					snd().playSwitch();
					if (ms().theme != 4) {
						fadeType = false; // Fade to white
						for (int i = 0; i < 30; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
					}
					if (showRshoulder) {
						PAGENUM += 1;
						CURPOS = 0;
						titleboxXpos[ms().secondaryDevice] = 0;
						titlewindowXpos[ms().secondaryDevice] = 0;
					} else {
						CURPOS = (file_count - 1) - PAGENUM * 40;
						if (CURPOS < 0) CURPOS = 0;
						titleboxXpos[ms().secondaryDevice] = CURPOS * 64;
						titlewindowXpos[ms().secondaryDevice] = CURPOS * 5;
					}
					if (ms().theme != 4) whiteScreen = true;
					if (ms().showBoxArt)
						clearBoxArt(); // Clear box art
					boxArtLoaded = false;
					rocketVideo_playVideo = true;
					if (showRshoulder)
						redoDropDown = true;
					shouldersRendered = false;
					currentBg = 0;
					showSTARTborder = false;
					stopSoundPlayed = false;
					clearText();
					ms().saveSettings();
					settingsChanged = false;
					displayNowLoading();
					break;
				}
			}

			if ((pressed & KEY_B) && ms().showDirectories) {
				// Go up a directory
				snd().playBack();
				if (ms().theme != 4) {
					fadeType = false; // Fade to white
					for (int i = 0; i < 30; i++) {
						snd().updateStream();
						swiWaitForVBlank();
					}
				}
				PAGENUM = 0;
				CURPOS = 0;
				titleboxXpos[ms().secondaryDevice] = 0;
				titlewindowXpos[ms().secondaryDevice] = 0;
				if (ms().theme != 4) whiteScreen = true;
				if (ms().showBoxArt)
					clearBoxArt(); // Clear box art
				boxArtLoaded = false;
				rocketVideo_playVideo = true;
				redoDropDown = true;
				shouldersRendered = false;
				currentBg = 0;
				showSTARTborder = false;
				stopSoundPlayed = false;
				clearText();
				chdir("..");
				char buf[256];

				ms().romfolder[ms().secondaryDevice] = std::string(getcwd(buf, 256));
				ms().saveSettings();
				settingsChanged = false;
				return "null";
			}

			if ((pressed & KEY_X) && !startMenu && bannerTextShown && showSTARTborder) {
				CIniFile hiddenGamesIni(hiddenGamesIniPath);
				vector<std::string> hiddenGames;
				char str[12] = {0};

				for (int i = 0; true; i++) {
					sprintf(str, "%d", i);
					if (hiddenGamesIni.GetString(getcwd(path, PATH_MAX), str, "") != "") {
						hiddenGames.push_back(
						    hiddenGamesIni.GetString(getcwd(path, PATH_MAX), str, ""));
					} else {
						break;
					}
				}

				for (int i = 0; i < (int)hiddenGames.size(); i++) {
					for (int j = 0; j < (int)hiddenGames.size(); j++) {
						if (i != j) {
							if (hiddenGames[i] == hiddenGames[j]) {
								hiddenGames.erase(hiddenGames.begin() + j);
							}
						}
					}
				}

				std::string gameBeingHidden = dirContents[scrn].at((PAGENUM * 40) + (CURPOS)).name;
				bool unHide = false;
				int whichToUnhide;

				for (int i = 0; i < (int)hiddenGames.size(); i++) {
					if (hiddenGames[i] == gameBeingHidden) {
						whichToUnhide = i;
						unHide = true;
					}
				}

				if (ms().theme == 4) {
					snd().playStartup();
					fadeType = false;	   // Fade to black
					for (int i = 0; i < 25; i++) {
						swiWaitForVBlank();
					}
					currentBg = 1;
					displayGameIcons = false;
					fadeType = true;
				} else {
					dbox_showIcon = true;
					showdialogbox = true;
				}
				clearText();
				if (ms().theme == 4) {
					while (!screenFadedIn()) { swiWaitForVBlank(); }
					dbox_showIcon = true;
				} else {
					for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
				}
				snprintf(fileCounter, sizeof(fileCounter), "%i/%i", (CURPOS + 1) + PAGENUM * 40,
					 file_count);
				titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
					    dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(), CURPOS);
				std::string dirContName = dirContents[scrn].at(CURPOS + PAGENUM * 40).name;
				// About 38 characters fit in the box.
				if (strlen(dirContName.c_str()) > 38) {
					// Truncate to 35, 35 + 3 = 38 (because we append "...").
					dirContName.resize(35, ' ');
					size_t first = dirContName.find_first_not_of(' ');
					size_t last = dirContName.find_last_not_of(' ');
					dirContName = dirContName.substr(first, (last - first + 1));
					dirContName.append("...");
				}
				printSmall(false, 16, 64, dirContName.c_str());
				printSmall(false, 16, 160, fileCounter);
				printSmallCentered(false, 112, "Are you sure you want to");
				if (isDirectory[CURPOS]) {
					if (unHide)
						printSmallCentered(false, 128, "unhide this folder?");
					else
						printSmallCentered(false, 128, "hide this folder?");
				} else {
					if (unHide)
						printSmallCentered(false, 128, "delete/unhide this game?");
					else
						printSmallCentered(false, 128, "delete/hide this game?");
				}
				for (int i = 0; i < 90; i++) {
					snd().updateStream();
					swiWaitForVBlank();
				}
				if (isDirectory[CURPOS]) {
					if (unHide)
						printSmall(false, 141, 160, BUTTON_Y " Unhide");
					else
						printSmall(false, 155, 160, BUTTON_Y " Hide");
					printSmall(false, 208, 160, BUTTON_B " No");
				} else {
					if (unHide)
						printSmall(false, 93, 160, BUTTON_Y" Unhide");
					else
						printSmall(false, 107, 160, BUTTON_Y" Hide");
					printSmall(false, 160, 160, BUTTON_A " Yes");
					printSmall(false, 208, 160, BUTTON_B " No");
				}
				while (1) {
					do {
						scanKeys();
						pressed = keysDown();
						checkSdEject();
						tex().drawVolumeImageCached();
						tex().drawBatteryImageCached();
						drawCurrentTime();
						drawCurrentDate();
						drawClockColon();
						snd().updateStream();
						swiWaitForVBlank();
					} while (!pressed);

					if (pressed & KEY_A && !isDirectory[CURPOS]) {
						snd().playLaunch();
						fadeType = false; // Fade to white
						for (int i = 0; i < 30; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
						whiteScreen = true;
						remove(dirContents[scrn]
							   .at(CURPOS + PAGENUM * 40)
							   .name.c_str()); // Remove game/folder
						if (ms().showBoxArt)
							clearBoxArt(); // Clear box art
						boxArtLoaded = false;
						rocketVideo_playVideo = true;
						shouldersRendered = false;
						currentBg = 0;
						showSTARTborder = false;
						stopSoundPlayed = false;
						clearText();
						showdialogbox = false;
						ms().saveSettings();
						settingsChanged = false;
						return "null";
					}

					if (pressed & KEY_B) {
						snd().playBack();
						break;
					}

					if (pressed & KEY_Y) {
						snd().playLaunch();
						fadeType = false; // Fade to white
						for (int i = 0; i < 30; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
						if (ms().theme != 4) whiteScreen = true;

						if (unHide) {
							hiddenGames.erase(hiddenGames.begin() + whichToUnhide);
							hiddenGames.push_back("");
						} else {
							hiddenGames.push_back(gameBeingHidden);
						}

						for (int i = 0; i < (int)hiddenGames.size(); i++) {
							char str[12] = {0};
							sprintf(str, "%d", i);
							hiddenGamesIni.SetString(getcwd(path, PATH_MAX), str,
										 hiddenGames[i]);
						}
						mkdir(sdFound() ? "sd:/_nds/TWiLightMenu/extras" : "fat:/_nds/TWiLightMenu/extras",
				      0777);
						hiddenGamesIni.SaveIniFile(hiddenGamesIniPath);

						if (ms().showBoxArt)
							clearBoxArt(); // Clear box art
						boxArtLoaded = false;
						shouldersRendered = false;
						currentBg = 0;
						showSTARTborder = false;
						stopSoundPlayed = false;
						clearText();
						showdialogbox = false;
						ms().saveSettings();
						settingsChanged = false;
						return "null";
					}
				}
				showdialogbox = false;
				if (ms().theme == 4) {
					fadeType = false;	   // Fade to black
					for (int i = 0; i < 25; i++) {
						swiWaitForVBlank();
					}
					clearText();
					currentBg = 0;
					displayGameIcons = true;
					fadeType = true;
					snd().playStartup();
				} else {
					clearText();
					for (int i = 0; i < 15; i++) { snd().updateStream(); swiWaitForVBlank(); }
				}
				dbox_showIcon = false;
			}

			if ((pressed & KEY_Y) && !startMenu && (isDirectory[CURPOS] == false) &&
			    (bnrRomType[CURPOS] == 0) && !titleboxXmoveleft && !titleboxXmoveright &&
			    bannerTextShown && showSTARTborder) {
				perGameSettings(dirContents[scrn].at(CURPOS + PAGENUM * 40).name);
			}

			if (pressed & KEY_SELECT) {
				if (ms().theme == 0 || ms().theme == 4) {
					if (selectMenu()) {
						clearText();
						showdialogbox = false;
						dbox_selectMenu = false;
						if (ms().theme == 4) currentBg = 0;
						return "null";
					}
				}
			}
		}
	}
}
