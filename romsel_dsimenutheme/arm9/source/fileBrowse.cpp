#include "fileBrowse.h"
#include <algorithm>
#include <dirent.h>
#include <math.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <vector>

#include <gl2d.h>
#include <maxmod9.h>
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <fat.h>

#include "date.h"

#include "SwitchState.h"
#include "errorScreen.h"
#include "graphics/FontGraphic.h"
#include "graphics/ThemeTextures.h"
#include "graphics/fontHandler.h"
#include "graphics/graphics.h"
#include "graphics/iconHandler.h"
#include "iconTitle.h"
#include "ndsheaderbanner.h"
#include "perGameSettings.h"
#include "incompatibleGameMap.h"

#include "gbaswitch.h"
#include "nds_loader_arm9.h"

#include "common/dsimenusettings.h"
#include "common/flashcard.h"
#include "common/inifile.h"
#include "common/systemdetails.h"
#include "language.h"

#include "fileCopy.h"
#include "sound.h"

#include "graphics/queueControl.h"

#define SCREEN_COLS 32
#define ENTRIES_PER_SCREEN 15
#define ENTRIES_START_ROW 3
#define ENTRY_PAGE_LENGTH 10

extern const char *bootstrapinipath;

extern bool whiteScreen;
extern bool fadeType;
extern bool fadeSpeed;
extern bool controlTopBright;
extern bool controlBottomBright;

extern const char *unlaunchAutoLoadID;
extern void unlaunchRomBoot(const char* rom);

extern bool dropDown;
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
extern bool showProgressBar;
extern int progressBarLength;

bool dirInfoIniFound = false;
bool pageLoaded[100] = {false};
bool dirContentBlankFilled[100] = {false};
bool lockOutDirContentBlankFilling = false;
std::string dirContName;

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

extern void stop();

extern void loadGameOnFlashcard(const char *ndsPath, bool dsGame);
extern void dsCardLaunch();
extern void unlaunchSetHiyaBoot();
extern void SetWidescreen(const char *filename);

extern bool rocketVideo_playVideo;

extern char usernameRendered[11];
extern bool usernameRenderedDone;

std::string gameOrderIniPath, recentlyPlayedIniPath, timesPlayedIniPath;

static bool inSelectMenu = false;

struct DirEntry {
	std::string name;
	bool isDirectory;
	int position;
	bool customPos;
};

struct TimesPlayed {
	std::string name;
	int amount;
};

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

bool nameEndsWith(const std::string &name, const std::vector<std::string> extensionList) {

	if (name.substr(0, 2) == "._")
		return false; // Don't show macOS's index files

	if (name.size() == 0)
		return false;

	if (extensionList.size() == 0)
		return true;

	for (int i = 0; i < (int)extensionList.size(); i++) {
		const std::string ext = extensionList.at(i);
		if (strcasecmp(name.c_str() + name.size() - ext.size(), ext.c_str()) == 0)
			return true;
	}
	return false;
}

bool dirEntryPredicate(const DirEntry &lhs, const DirEntry &rhs) {

	if (lhs.isDirectory && !lhs.customPos && !rhs.isDirectory) {
		return true;
	}
	if (!lhs.isDirectory && rhs.isDirectory && !rhs.customPos) {
		return false;
	}
	if (lhs.customPos || rhs.customPos) {
		if(!lhs.customPos)	return false;
		else if(!rhs.customPos)	return true;

		if(lhs.position < rhs.position)	return true;
		else return false;
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

void getDirectoryContents(std::vector<DirEntry> &dirContents, const std::vector<std::string> extensionList) {

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
		printSmall(false, 4, 4, STR_UNABLE_TO_OPEN_DIRECTORY);
	} else {
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
					if (ms().showHidden || !(FAT_getAttr(dirEntry.name.c_str()) & ATTR_HIDDEN || (strncmp(dirEntry.name.c_str(), ".", 1) == 0 && dirEntry.name != ".."))) {
						dirContents.push_back(dirEntry);
						file_count++;
					}
				}
			} else {
				if (dirEntry.name.compare(".") != 0 && (nameEndsWith(dirEntry.name, extensionList))) {
					if (ms().showHidden || !(FAT_getAttr(dirEntry.name.c_str()) & ATTR_HIDDEN)) {
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
			std::vector<std::string> recentlyPlayed;
			getcwd(path, PATH_MAX);
			recentlyPlayedIni.GetStringVector("RECENT", path, recentlyPlayed, ':');

			int k = 0;
			for (uint i = 0; i < recentlyPlayed.size(); i++) {
				for (uint j = 0; j <= dirContents.size(); j++) {
					if (recentlyPlayed[i] == dirContents[j].name) {
						dirContents[j].position = k++;
						dirContents[j].customPos = true;
						break;
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
			getcwd(path, PATH_MAX);
			gameOrderIni.GetStringVector("ORDER", path, gameOrder, ':');

			for (uint i = 0; i < gameOrder.size(); i++) {
				for (uint j = 0; j < dirContents.size(); j++) {
					if (gameOrder[i] == dirContents[j].name) {
						dirContents[j].position = i;
						dirContents[j].customPos = true;
						break;
					}
				}
			}
			sort(dirContents.begin(), dirContents.end(), dirEntryPredicate);
		}
		closedir(pdir);
	}
}

void getDirectoryContents(std::vector<DirEntry> &dirContents) {
	std::vector<std::string> extensionList;
	getDirectoryContents(dirContents, extensionList);
}

void waitForFadeOut(void) {
	if (!dropDown && ms().theme == 0) {
		dropDown = true;
		for (int i = 0; i < 60; i++) {
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
	}
}

bool nowLoadingDisplaying = false;

void displayNowLoading(void) {
	displayGameIcons = false;
	fadeType = true; // Fade in from white
	snd().updateStream();
	std::string *msg;
	if (showProgressBar) {
		if (isDSiMode() && memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
			msg = &STR_BARSTOPPED_TURNOFF;
		} else if (REG_SCFG_EXT != 0 && ms().consoleModel >= 2) {
			msg = &STR_BARSTOPPED_PRESSHOME;
		} else {
			msg = &STR_BARSTOPPED_CLOSELID;
		}
	} else {
		if (isDSiMode() && memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
			msg = &STR_TAKEWHILE_TURNOFF;
		} else if (REG_SCFG_EXT != 0 && ms().consoleModel >= 2) {
			msg = &STR_TAKEWHILE_PRESSHOME;
		} else {
			msg = &STR_TAKEWHILE_CLOSELID;
		}
	}
	printSmall(false, 0, 20, *msg, Alignment::center);
	printLarge(false, 0, 88, STR_NOW_LOADING, Alignment::center);
	if (!sys().isRegularDS()) {
		if (ms().theme == 4) {
			if (ms().secondaryDevice) {
				printSmall(false, 0, 20 + calcSmallFontHeight(*msg), STR_LOCATION_SLOT_1, Alignment::center);
			} else {
				printSmall(false, 0, 20 + calcSmallFontHeight(*msg), ms().showMicroSd ? STR_LOCATION_MICRO_SD : STR_LOCATION_SD, Alignment::center);
			}
		} else {
			if (ms().secondaryDevice) {
				printSmall(false, 0, 168, STR_LOCATION_SLOT_1, Alignment::center);
			} else {
				printSmall(false, 0, 168, ms().showMicroSd ? STR_LOCATION_MICRO_SD : STR_LOCATION_SD, Alignment::center);
			}
		}
	}
	updateText(false);
	nowLoadingDisplaying = true;
	while (!screenFadedIn()) 
	{
		snd().updateStream();
	}
	snd().updateStream();
	showProgressIcon = true;
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
		if (ms().theme == 5 || !ms().showBoxArt) {
			return;
		}

		if (!boxArtLoaded) {
			if (isDirectory[CURPOS]) {
				clearBoxArt(); // Clear box art, if it's a directory
				if (ms().theme == 1 && !rocketVideo_playVideo) {
					rocketVideo_playVideo = true;
				}
			} else {
				clearBoxArt();		
				if (ms().theme == 1 && rocketVideo_playVideo) {
					// Clear top screen cubes
					rocketVideo_playVideo = false;
				}
				if ((REG_SCFG_EXT != 0) && ms().showBoxArt == 2) {
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


void launchDsClassicMenu(void) {
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
	// Launch DS Classic Menu
	if (!isDSiMode()) {
		chdir("fat:/");
	} else if (sdFound()) {
		chdir("sd:/");
	}
	int err = runNdsFile("/_nds/TWiLightMenu/mainmenu.srldr", 0, NULL, true, false, false, true, true);
	char text[32];
	snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
	fadeType = true;
	printLarge(false, 4, 4, text);
	stop();
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
	// Launch TWLMenu++ Settings
	if (!isDSiMode()) {
		chdir("fat:/");
	} else if (sdFound()) {
		chdir("sd:/");
	}
	int err = runNdsFile("/_nds/TWiLightMenu/settings.srldr", 0, NULL, true, false, false, true, true);
	char text[32];
	snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
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
	// Launch manual
	if (!isDSiMode()) {
		chdir("fat:/");
	} else if (sdFound()) {
		chdir("sd:/");
	}
	int err = runNdsFile("/_nds/TWiLightMenu/manual.srldr", 0, NULL, true, false, false, true, true);
	char text[32];
	snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
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
		if (ms().theme != 4 && ms().theme != 5) {
			fadeType = false; // Fade to white
			for (int i = 0; i < 25; i++) {
				snd().updateStream();
				swiWaitForVBlank();
			}
		}
		ms().secondaryDevice = !ms().secondaryDevice;
		if (!rocketVideo_playVideo || ms().showBoxArt)
			clearBoxArt(); // Clear box art
		if (ms().theme != 4 && ms().theme != 5) whiteScreen = true;
		boxArtLoaded = false;
		rocketVideo_playVideo = true;
		shouldersRendered = false;
		currentBg = 0;
		showSTARTborder = false;
		stopSoundPlayed = false;
		clearText();
		updateText(false);
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

		ms().slot1Launched = true; // 0
		ms().saveSettings();

		if (ms().slot1LaunchMethod==0 || sys().arm7SCFGLocked()) {
			dsCardLaunch();
		} else if (ms().slot1LaunchMethod==2) {
			unlaunchRomBoot("cart:");
		} else {
			SetWidescreen(NULL);
			if (sdFound()) {
				chdir("sd:/");
			}
			int err = runNdsFile("/_nds/TWiLightMenu/slot1launch.srldr", 0, NULL, true, true, false, true, true);
			char text[32];
			snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
			fadeType = true;
			printLarge(false, 4, 4, text);
			stop();
		}
	}
}

void launchGba(void) {
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
	if (ms().showGba == 2) {
		if (ms().secondaryDevice) {
			const char* gbar2Path = ms().gbar2DldiAccess ? "fat:/_nds/GBARunner2_arm7dldi_ds.nds" : "fat:/_nds/GBARunner2_arm9dldi_ds.nds";
			if (isDSiMode()) {
				gbar2Path = ms().consoleModel>0 ? "fat:/_nds/GBARunner2_arm7dldi_3ds.nds" : "fat:/_nds/GBARunner2_arm7dldi_dsi.nds";
			}
			if (ms().useBootstrap) {
				int err = runNdsFile(gbar2Path, 0, NULL, true, true, false, true, false);
				iprintf("Start failed. Error %i\n", err);
			} else {
				loadGameOnFlashcard(gbar2Path, false);
			}
		} else {
			std::string bootstrapPath = (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds"
								: "sd:/_nds/nds-bootstrap-hb-release.nds");

			std::vector<char*> argarray;
			argarray.push_back(strdup(bootstrapPath.c_str()));
			argarray.at(0) = (char*)bootstrapPath.c_str();

			const char* gbar2Path = ms().consoleModel>0 ? "sd:/_nds/GBARunner2_arm7dldi_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_dsi.nds";
			if (isDSiMode() && sys().arm7SCFGLocked()) {
				gbar2Path = ms().consoleModel>0 ? "sd:/_nds/GBARunner2_arm7dldi_nodsp_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_nodsp_dsi.nds";
			}

			CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");
			bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", gbar2Path);
			bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "");
			bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", "");
			bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().gameLanguage);
			bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
			bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
			bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);
			bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
			if (!isDSiMode() && !ms().secondaryDevice) {
				extern void ntrStartSdGame();
				ntrStartSdGame();
			}
			int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], false, true, false, true, true);
			char text[32];
			snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
			fadeType = true;
			printLarge(false, 4, 4, text);
			if (err == 1) {
				printLarge(false, 4, 20, ms().bootstrapFile ? STR_BOOTSTRAP_NIGHTLY_NOT_FOUND : STR_BOOTSTRAP_RELEASE_NOT_FOUND);
			}
			updateText(false);
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
	printSmall(false, 0, 64, STR_SMS_WARNING, Alignment::center);
	printSmall(false, 240, 160, STR_A_OK, Alignment::right);
	updateText(false);
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
		updateText(false);
		currentBg = 0;
		displayGameIcons = true;
		fadeType = true;
		snd().playStartup();
		while (!screenFadedIn()) { swiWaitForVBlank(); }
	} else {
		clearText();
		updateText(false);
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
	updateText(false);
	if (ms().theme == 4) {
		while (!screenFadedIn()) { swiWaitForVBlank(); }
		snd().playWrong();
	} else {
		for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
	}
	printSmall(false, 0, 64, STR_MD_ROM_TOO_BIG, Alignment::center);
	printSmall(false, 240, 160, STR_A_OK, Alignment::right);
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
	updateText(false);
}

void ramDiskMsg(const char *filename) {
	clearText();
	updateText(false);
	snd().playWrong();
	if (ms().theme != 4) {
		dbox_showIcon = true;
		showdialogbox = true;
		for (int i = 0; i < 30; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		titleUpdate(false, filename, CURPOS);
	}
	dirContName = filename;
	// About 38 characters fit in the box.
	if (strlen(dirContName.c_str()) > 38) {
		// Truncate to 35, 35 + 3 = 38 (because we append "...").
		dirContName.resize(35, ' ');
		size_t first = dirContName.find_first_not_of(' ');
		size_t last = dirContName.find_last_not_of(' ');
		dirContName = dirContName.substr(first, (last - first + 1));
		dirContName.append("...");
	}
	printSmall(false, 16, 66, dirContName);
	printSmall(false, 0, (ms().theme == 4 ? 24 : 112), STR_RAM_DISK_REQUIRED, Alignment::center);
	printSmall(false, 240, (ms().theme == 4 ? 64 : 160), STR_A_OK, Alignment::right);
	updateText(false);
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
	clearText();
	updateText(false);
	if (ms().theme == 5) {
		dbox_showIcon = false;
	}
	if (ms().theme == 4) {
		snd().playLaunch();
	} else {
		showdialogbox = false;
	}
}

void dsiBinariesMissingMsg(const char *filename) {
	clearText();
	updateText(false);
	snd().playWrong();
	if (ms().theme != 4) {
		dbox_showIcon = true;
		showdialogbox = true;
		for (int i = 0; i < 30; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		titleUpdate(false, filename, CURPOS);
		dirContName = filename;
		// About 38 characters fit in the box.
		if (strlen(dirContName.c_str()) > 38) {
			// Truncate to 35, 35 + 3 = 38 (because we append "...").
			dirContName.resize(35, ' ');
			size_t first = dirContName.find_first_not_of(' ');
			size_t last = dirContName.find_last_not_of(' ');
			dirContName = dirContName.substr(first, (last - first + 1));
			dirContName.append("...");
		}
		printSmall(false, 16, 66, dirContName);
	}
	printSmall(false, 0, (ms().theme == 4 ? 8 : 96), STR_DSIBINARIES_MISSING, Alignment::center);
	printSmall(false, 240, (ms().theme == 4 ? 64 : 160), STR_A_OK, Alignment::right);
	updateText(false);
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
	clearText();
	if (ms().theme == 5) {
		dbox_showIcon = false;
	}
	if (ms().theme == 4) {
		snd().playLaunch();
	} else {
		showdialogbox = false;
	}
	updateText(false);
}

void donorRomMsg(const char *filename) {
	clearText();
	snd().playWrong();
	if (ms().theme != 4) {
		dbox_showIcon = true;
		showdialogbox = true;
		for (int i = 0; i < 30; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		titleUpdate(false, filename, CURPOS);
		dirContName = filename;
		// About 38 characters fit in the box.
		if (strlen(dirContName.c_str()) > 38) {
			// Truncate to 35, 35 + 3 = 38 (because we append "...").
			dirContName.resize(35, ' ');
			size_t first = dirContName.find_first_not_of(' ');
			size_t last = dirContName.find_last_not_of(' ');
			dirContName = dirContName.substr(first, (last - first + 1));
			dirContName.append("...");
		}
		printSmall(false, 16, 66, dirContName.c_str());
	}
	int yPos = (ms().theme == 4 ? 8 : 96);
	switch (requiresDonorRom[CURPOS]) {
		case 20:
			printSmall(false, 0, yPos, STR_DONOR_ROM_MSG_ESDK2, Alignment::center);
			break;
		case 2:
			printSmall(false, 0, yPos, STR_DONOR_ROM_MSG_SDK2, Alignment::center);
			break;
		case 3:
			printSmall(false, 0, yPos, STR_DONOR_ROM_MSG_ESDK3, Alignment::center);
			break;
		case 5:
		default:
			printSmall(false, 0, yPos, STR_DONOR_ROM_MSG_SDK5, Alignment::center);
			break;
		case 51:
			printSmall(false, 0, yPos, STR_DONOR_ROM_MSG_SDK5TWL, Alignment::center);
			break;
	}
	printSmall(false, 240, (ms().theme == 4 ? 64 : 160), STR_A_OK, Alignment::right);
	updateText(false);
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
	clearText();
	if (ms().theme == 5) {
		dbox_showIcon = false;
	}
	if (ms().theme == 4) {
		snd().playLaunch();
	} else {
		showdialogbox = false;
	}
	updateText(false);
}

bool checkForCompatibleGame(const char *filename) {
	bool proceedToLaunch = true;

	if (!isDSiMode() && ms().secondaryDevice) {
		// TODO: If the list gets large enough, switch to bsearch().
		for (unsigned int i = 0; i < sizeof(incompatibleGameListB4DS)/sizeof(incompatibleGameListB4DS[0]); i++) {
			if (memcmp(gameTid[CURPOS], incompatibleGameListB4DS[i], 3) == 0) {
				// Found match
				proceedToLaunch = false;
				break;
			}
		}
	}

	if (proceedToLaunch) {
		// TODO: If the list gets large enough, switch to bsearch().
		for (unsigned int i = 0; i < sizeof(incompatibleGameList)/sizeof(incompatibleGameList[0]); i++) {
			if (memcmp(gameTid[CURPOS], incompatibleGameList[i], 3) == 0) {
				// Found match
				proceedToLaunch = false;
				break;
			}
		}
	}

	if (proceedToLaunch) return true;	// Game is compatible

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
	titleUpdate(false, filename, CURPOS);
	printSmall(false, 0, 72, STR_GAME_INCOMPATIBLE_MSG, Alignment::center);
	printSmall(false, 0, 160, STR_A_IGNORE_B_DONT_LAUNCH, Alignment::center);
	updateText(false);
	int pressed = 0;
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
			proceedToLaunch = true;
			pressed = 0;
			break;
		}
		if (pressed & KEY_B) {
			snd().playBack();
			proceedToLaunch = false;
			pressed = 0;
			break;
		}
	}
	showdialogbox = false;
	if (ms().theme == 5) {
		dbox_showIcon = false;
	}
	if (ms().theme == 4) {
		fadeType = false;	   // Fade to black
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
		clearText();
		updateText(false);
		currentBg = 0;
		displayGameIcons = true;
		fadeType = true;
		snd().playStartup();
		if (proceedToLaunch) {
			while (!screenFadedIn()) { swiWaitForVBlank(); }
		}
	} else {
		clearText();
		updateText(false);
		for (int i = 0; i < (proceedToLaunch ? 20 : 15); i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
	}
	dbox_showIcon = false;

	return proceedToLaunch;
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
	updateText(false);
	if (!rocketVideo_playVideo || ms().showBoxArt)
		clearBoxArt(); // Clear box art
	boxArtLoaded = false;
	rocketVideo_playVideo = true;
	int maxCursors = 0;
	int selCursorPosition = 0;
	int assignedOp[5] = {-1};
	int selIconYpos = 96;
	if (isDSiMode() && memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) != 0) {
		for (int i = 0; i < 4; i++) {
			selIconYpos -= 14;
		}
		assignedOp[0] = 0;
		assignedOp[1] = 1;
		assignedOp[2] = 2;
		assignedOp[3] = 4;
		maxCursors = 3;
	} else {
		for (int i = 0; i < 3; i++) {
			selIconYpos -= 14;
		}
		if (!sys().isRegularDS()) {
			assignedOp[0] = 0;
			assignedOp[1] = 1;
			assignedOp[2] = 4;
			maxCursors = 2;
		} else if (ms().showGba == 2) {
			assignedOp[0] = 1;
			assignedOp[1] = 4;
			maxCursors = 1;
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
		printSmall(false, 0, (ms().theme == 4 ? 8 : 16), STR_SELECT_MENU, Alignment::center);
		printSmall(false, 24, -2 + textYpos + (28 * selCursorPosition), ">");
		for (int i = 0; i <= maxCursors; i++) {
			if (assignedOp[i] == 0) {
				printSmall(false, 64, textYpos, (ms().consoleModel < 2) ? STR_DSI_MENU : STR_3DS_HOME_MENU);
			} else if (assignedOp[i] == 1) {
				printSmall(false, 64, textYpos, STR_TWLMENU_SETTINGS);
			} else if (assignedOp[i] == 2) {
				if (bothSDandFlashcard()) {
					if (ms().secondaryDevice) {
						printSmall(false, 64, textYpos, ms().showMicroSd ? STR_SWITCH_TO_MICRO_SD : STR_SWITCH_TO_SD);
					} else {
						printSmall(false, 64, textYpos, STR_SWITCH_TO_SLOT_1);
					}
				} else if (isDSiMode() && memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) != 0) {
					printSmall(false, 64, textYpos, (REG_SCFG_MC == 0x11) ? STR_NO_SLOT_1 : STR_LAUNCH_SLOT_1);
				}
			} else if (assignedOp[i] == 3) {
				printSmall(false, 64, textYpos,
					   /*ms().useGbarunner ? "Start GBARunner2" :*/ "Start GBA Mode");
			} else if (assignedOp[i] == 4) {
				printSmall(false, 64, textYpos, STR_OPEN_MANUAL);
			}
			textYpos += 28;
		}
		printSmall(false, 0, (ms().theme == 4 ? 164 : 160), STR_SELECT_B_BACK_A_SELECT, Alignment::center);
		updateText(false);
		u8 current_SCFG_MC = REG_SCFG_MC;
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
			if (REG_SCFG_MC != current_SCFG_MC) {
				break;
			}
		} while(!pressed);
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
	updateText(false);
	return false;
}

void getFileInfo(SwitchState scrn, vector<vector<DirEntry>> dirContents, bool reSpawnBoxes) {
	extern bool extention(const std::string& filename, const char* ext);

	if (nowLoadingDisplaying) {
		clearText();
		showProgressBar = true;
		progressBarLength = 0;
		displayNowLoading();
	}
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
				 || extention(std_romsel_filename, ".srl")
				 || extention(std_romsel_filename, ".app")
				 || extention(std_romsel_filename, ".argv"))
				{
					getGameInfo(isDirectory[i], dirContents[scrn].at(i + PAGENUM * 40).name.c_str(),
							i);
					bnrRomType[i] = 0;
					boxArtType[i] = 0;
				} else if (extention(std_romsel_filename, ".pce")) {
					bnrRomType[i] = 11;
					boxArtType[i] = 0;
				} else if (extention(std_romsel_filename, ".a26") || extention(std_romsel_filename, ".a78")) {
					bnrRomType[i] = 10;
					boxArtType[i] = 0;
				} else if (extention(std_romsel_filename, ".plg") || extention(std_romsel_filename, ".rvid") || extention(std_romsel_filename, ".mp4")) {
					bnrRomType[i] = 9;
					boxArtType[i] = 0;
				} else if (extention(std_romsel_filename, ".gba")) {
					bnrRomType[i] = 1;
					boxArtType[i] = 1;
				} else if (extention(std_romsel_filename, ".gb") || extention(std_romsel_filename, ".sgb")) {
					bnrRomType[i] = 2;
					boxArtType[i] = 1;
				} else if (extention(std_romsel_filename, ".gbc")) {
					bnrRomType[i] = 3;
					boxArtType[i] = 1;
				} else if (extention(std_romsel_filename, ".nes")) {
					bnrRomType[i] = 4;
					boxArtType[i] = 2;
				} else if (extention(std_romsel_filename, ".fds")) {
					bnrRomType[i] = 4;
					boxArtType[i] = 1;
				} else if (extention(std_romsel_filename, ".sms")) {
					bnrRomType[i] = 5;
					boxArtType[i] = 2;
				} else if (extention(std_romsel_filename, ".gg")) {
					bnrRomType[i] = 6;
					boxArtType[i] = 2;
				} else if (extention(std_romsel_filename, ".gen")) {
					bnrRomType[i] = 7;
					boxArtType[i] = 2;
				} else if (extention(std_romsel_filename, ".smc")) {
					bnrRomType[i] = 8;
					boxArtType[i] = 3;
				} else if (extention(std_romsel_filename, ".sfc")) {
					bnrRomType[i] = 8;
					boxArtType[i] = 2;
				}

				if (bnrRomType[i] > 0 && bnrRomType[i] < 10) {
					bnrWirelessIcon[i] = 0;
					isDSiWare[i] = false;
					isHomebrew[i] = 0;
				}

				if ((REG_SCFG_EXT != 0) && ms().showBoxArt == 2 && ms().theme!=5 && !isDirectory[i]) {
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

			progressBarLength += (200/((file_count - (PAGENUM*40)) > 40 ? 40 : (file_count - (PAGENUM*40))));
			if (progressBarLength > 192) progressBarLength = 192;
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
		showProgressBar = false;
		progressBarLength = 0;
		if (ms().theme != 4 && ms().theme != 5) fadeType = false; // Fade to white
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

static bool previousPage(void) {
	if (CURPOS == 0 && !showLshoulder) {
		snd().playWrong();
	} else if (!titleboxXmoveleft && !titleboxXmoveright) {
		snd().playSwitch();
		if (ms().theme != 4 && ms().theme != 5) {
			fadeType = false; // Fade to white
			for (int i = 0; i < 6; i++) {
				snd().updateStream();
				swiWaitForVBlank();
			}
		}
		if (showLshoulder)
			PAGENUM -= 1;
		CURPOS = 0;
		titleboxXpos[ms().secondaryDevice] = 0;
		titlewindowXpos[ms().secondaryDevice] = 0;
		if (ms().theme != 4 && ms().theme != 5) whiteScreen = true;
		if (ms().showBoxArt)
			clearBoxArt(); // Clear box art
		boxArtLoaded = false;
		bannerTextShown = false;
		rocketVideo_playVideo = true;
		shouldersRendered = false;
		currentBg = 0;
		showSTARTborder = false;
		stopSoundPlayed = false;
		clearText();
		updateText(false);
		ms().saveSettings();
		settingsChanged = false;
		displayNowLoading();
		return true;
	}
	return false;
}

static bool nextPage(void) {
	if (CURPOS == (file_count - 1) - PAGENUM * 40 && !showRshoulder) {
		snd().playWrong();
	} else if (!titleboxXmoveleft && !titleboxXmoveright) {
		snd().playSwitch();
		if (ms().theme != 4 && ms().theme != 5) {
			fadeType = false; // Fade to white
			for (int i = 0; i < 6; i++) {
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
		if (ms().theme != 4 && ms().theme != 5) whiteScreen = true;
		if (ms().showBoxArt)
			clearBoxArt(); // Clear box art
		boxArtLoaded = false;
		bannerTextShown = false;
		rocketVideo_playVideo = true;
		shouldersRendered = false;
		currentBg = 0;
		showSTARTborder = false;
		stopSoundPlayed = false;
		clearText();
		updateText(false);
		ms().saveSettings();
		settingsChanged = false;
		displayNowLoading();
		return true;
	}
	return false;
}

std::string browseForFile(const std::vector<std::string> extensionList) {
	snd().updateStream();
	displayNowLoading();
	snd().updateStream();
	gameOrderIniPath = std::string(sdFound() ? "sd" : "fat") + ":/_nds/TWiLightMenu/extras/gameorder.ini";
	recentlyPlayedIniPath = std::string(sdFound() ? "sd" : "fat") + ":/_nds/TWiLightMenu/extras/recentlyplayed.ini";
	timesPlayedIniPath = std::string(sdFound() ? "sd" : "fat") + ":/_nds/TWiLightMenu/extras/timesplayed.ini";

	bool displayBoxArt = ms().showBoxArt;

	int pressed = 0;
	int held = 0;
	SwitchState scrn(3);
	vector<vector<DirEntry>> dirContents(scrn.SIZE);

	getDirectoryContents(dirContents[scrn], extensionList);

	controlTopBright = false;

	while (1) {
		snd().updateStream();
		updateDirectoryContents(dirContents[scrn]);
		getFileInfo(scrn, dirContents, true);
		reloadIconPalettes();
		if (ms().theme != 4 && ms().theme != 5) {
			while (!screenFadedOut());
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
		updateText(false);
		snd().updateStream();
		waitForFadeOut();
		bool gameTapped = false;

		while (1) {
			snd().updateStream();
			if (!stopSoundPlayed) {
				if (ms().theme == 0 &&
					 CURPOS + PAGENUM * 40 <= ((int)dirContents[scrn].size() - 1)) {
					needToPlayStopSound = true;
				}
				stopSoundPlayed = true;
			}

			if (!shouldersRendered) {
				showLshoulder = (PAGENUM != 0);
				showRshoulder = (file_count > 40 + PAGENUM * 40);
				if (ms().theme != 5) {
					tex().drawShoulders(showLshoulder, showRshoulder);
				}
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
					if(!bannerTextShown) {
						clearText();
						titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
								dirContents[scrn].at(CURPOS + PAGENUM * 40).name, CURPOS);
						bannerTextShown = true;
					}
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
				if (ms().theme == 5) {
					printLarge(false, 0, 142, "^", Alignment::center);
					printSmall(false, 4, 174, (showLshoulder ? STR_L_PREV : STR_L));
					printSmall(false, 256-4, 174, (showRshoulder ? STR_NEXT_R : STR_R), Alignment::right);
				}
				updateText(false);
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
					bannerTextShown = false;
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
					bannerTextShown = false;
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
					}
				}
				// Move apps
			} else if ((pressed & KEY_UP) && (ms().theme != 4 && ms().theme != 5) && !dirInfoIniFound && (ms().sortMethod == 4)
				   && !titleboxXmoveleft && !titleboxXmoveright &&CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
				bannerTextShown = false; // Redraw the title when done
				showSTARTborder = false;
				currentBg = 2;
				clearText();
				updateText(false);
				mkdir(sdFound() ? "sd:/_nds/TWiLightMenu/extras" : "fat:/_nds/TWiLightMenu/extras",
					  0777);
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
						if (!titleboxXmoveleft && !titleboxXmoveright &&
							PAGENUM != 0) {
							snd().playSwitch();
							fadeType = false; // Fade to white
							for (int i = 0; i < 6; i++) {
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
							clearText();
							updateText(false);
						} else {
							snd().playWrong();
						}
					} else if (pressed & KEY_R) {
						if (!titleboxXmoveleft && !titleboxXmoveright &&
							file_count > 40 + PAGENUM * 40) {
							snd().playSwitch();
							fadeType = false; // Fade to white
							for (int i = 0; i < 6; i++) {
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

							while (!screenFadedOut())
								;
							nowLoadingDisplaying = false;
							whiteScreen = false;
							displayGameIcons = true;
							fadeType = true; // Fade in from white
							for (int i = 0; i < 5; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							reloadIconPalettes();
							clearText();
							updateText(false);
						} else {
							snd().playWrong();
						}
					}
				}
				if ((PAGENUM != orgPage) || (CURPOS != orgCursorPosition)) {
					currentBg = 1;
					writeBannerText(STR_PLEASE_WAIT);

					int dest = CURPOS + (PAGENUM * 40);

					DirEntry entry = dirContents[scrn][movingApp];
					dirContents[scrn].erase(dirContents[scrn].begin()+movingApp);
					dirContents[scrn].insert(dirContents[scrn].begin() + dest, entry);

					std::vector<std::string> dirNames;
					for(uint i=0;i<dirContents[scrn].size();i++) {
						dirNames.push_back(dirContents[scrn][i].name);
					}

					CIniFile gameOrderIni(gameOrderIniPath);
					getcwd(path, PATH_MAX);
					gameOrderIni.SetStringVector("ORDER", path, dirNames, ':');
					gameOrderIni.SaveIniFile(gameOrderIniPath);

					if(ms().sortMethod != 4) {
						ms().sortMethod = 4;
						ms().saveSettings();
					}

					getFileInfo(scrn, dirContents, false);
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
				int prevCurPos;
				while (1) {
					scanKeys();
					touchRead(&touch);

					if (!(keysHeld() & KEY_TOUCH))
						break;

					prevCurPos = CURPOS;
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

					if (CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
						if(prevCurPos != CURPOS) {
							currentBg = 1;
							clearText();
							titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
									dirContents[scrn].at(CURPOS + PAGENUM * 40).name,
									CURPOS);
									bannerTextShown = true;
							updateText(false);
						}
					} else {
						clearText();
						updateText(false);
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
				bannerTextShown = false;
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
							gameTapped = (bannerTextShown && showSTARTborder);
							break;
						} else if (touch.px < startTouch.px - 20 ||
								   touch.px > startTouch.px + 20)
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
										updateText(false);
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
										updateText(false);
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
									updateText(false);
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
									updateText(false);
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
									updateText(false);
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
								updateText(false);
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
								updateText(false);
							}
						}
					}

					if (prevPos != CURPOS) {
						clearText();
						if (CURPOS + PAGENUM * 40 < ((int)dirContents[scrn].size())) {
							currentBg = 1;
							titleUpdate(
								dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
								dirContents[scrn].at(CURPOS + PAGENUM * 40).name,
								CURPOS);
						} else {
							currentBg = 0;
						}
						if (ms().theme == 5) {
							printSmall(false, 4, 174, (showLshoulder ? STR_L_PREV : STR_L));
							printSmall(false, 256-4, 174, (showRshoulder ? STR_NEXT_R : STR_R), Alignment::right);
						}
						updateText(false);
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
				titleboxXpos[ms().secondaryDevice] = CURPOS * 64;
				boxArtLoaded = false;
				settingsChanged = true;
				bannerTextShown = false;
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
			if ((((pressed & KEY_A) || (pressed & KEY_START)) && bannerTextShown && showSTARTborder &&
				!titleboxXmoveleft && !titleboxXmoveright) ||
				(gameTapped)) {
				bannerTextShown = false; // Redraw title when done
				DirEntry *entry = &dirContents[scrn].at(CURPOS + PAGENUM * 40);
				if (entry->isDirectory) {
					// Enter selected directory
					(ms().theme == 4) ? snd().playLaunch() : snd().playSelect();
					if (ms().theme != 4 && ms().theme != 5) {
						fadeType = false; // Fade to white
						for (int i = 0; i < 6; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
					}
					ms().pagenum[ms().secondaryDevice] = 0;
					ms().cursorPosition[ms().secondaryDevice] = 0;
					titleboxXpos[ms().secondaryDevice] = 0;
					titlewindowXpos[ms().secondaryDevice] = 0;
					if (ms().theme != 4 && ms().theme != 5) whiteScreen = true;
					if (ms().showBoxArt)
						clearBoxArt(); // Clear box art
					boxArtLoaded = false;
					shouldersRendered = false;
					currentBg = 0;
					showSTARTborder = false;
					stopSoundPlayed = false;
					clearText();
					updateText(false);
					chdir(entry->name.c_str());
					char buf[256];
					ms().romfolder[ms().secondaryDevice] = std::string(getcwd(buf, 256));
					ms().saveSettings();
					settingsChanged = false;
					return "null";
				} else if (isDSiWare[CURPOS] && !isDSiMode() && !sdFound()) {
					clearText();
					updateText(false);
					snd().playWrong();
					if (ms().theme != 4) {
						dbox_showIcon = true;
						showdialogbox = true;
						for (int i = 0; i < 30; i++) {
							snd().updateStream();
							swiWaitForVBlank();
						}
						titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
								dirContents[scrn].at(CURPOS + PAGENUM * 40).name, CURPOS);
					}
					int yPos = (ms().theme == 4 ? 24 : 112);
					printSmall(false, 0, yPos, isDSiMode() ? STR_CANNOT_LAUNCH_WITHOUT_SD : STR_CANNOT_LAUNCH_IN_DS_MODE, Alignment::center);
					printSmall(false, 240, (ms().theme == 4 ? 64 : 160), STR_A_OK, Alignment::right);
					updateText(false);
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
					updateText(false);
					if (ms().theme == 5) {
						dbox_showIcon = false;
					}
					if (ms().theme == 4) {
						snd().playLaunch();
					} else {
						showdialogbox = false;
					}
				} else {
					int hasAP = 0;
					bool proceedToLaunch = true;
					bool useBootstrapAnyway = (ms().useBootstrap || !ms().secondaryDevice);
					if (useBootstrapAnyway && bnrRomType[CURPOS] == 0 && !isDSiWare[CURPOS]
					 &&	isHomebrew[CURPOS] == 0)
					{
						proceedToLaunch = checkForCompatibleGame(dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str());
						if (proceedToLaunch && requiresDonorRom[CURPOS]) {
							const char* pathDefine = "DONOR_NDS_PATH";
							if (requiresDonorRom[CURPOS]==20) {
								pathDefine = "DONORE2_NDS_PATH";
							} else if (requiresDonorRom[CURPOS]==2) {
								pathDefine = "DONOR2_NDS_PATH";
							} else if (requiresDonorRom[CURPOS]==3) {
								pathDefine = "DONOR3_NDS_PATH";
							} else if (requiresDonorRom[CURPOS]==51) {
								pathDefine = "DONORTWL_NDS_PATH";
							}
							std::string donorRomPath;
							bootstrapinipath = ((!ms().secondaryDevice || (isDSiMode() && sdFound())) ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini");
							CIniFile bootstrapini(bootstrapinipath);
							donorRomPath = bootstrapini.GetString("NDS-BOOTSTRAP", pathDefine, "");
							if (donorRomPath == "" || access(donorRomPath.c_str(), F_OK) != 0) {
								proceedToLaunch = false;
								donorRomMsg(dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str());
							}
						}
						if (proceedToLaunch && checkIfShowAPMsg(dirContents[scrn].at(CURPOS + PAGENUM * 40).name)) {
							FILE *f_nds_file = fopen(
								dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(), "rb");
							hasAP = checkRomAP(f_nds_file, CURPOS);
							fclose(f_nds_file);
						}
					} else if (isHomebrew[CURPOS] == 1) {
						loadPerGameSettings(dirContents[scrn].at(CURPOS + PAGENUM * 40).name);
						if (requiresRamDisk[CURPOS] && perGameSettings_ramDiskNo == -1) {
							proceedToLaunch = false;
							ramDiskMsg(dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str());
						}
					} else if (bnrRomType[CURPOS] == 5 || bnrRomType[CURPOS] == 6) {
						if (!ms().smsGgInRam)
							smsWarning();
					} else if (bnrRomType[CURPOS] == 7) {
						if (ms().showMd==1 && getFileSize(
							dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str()) >
							0x300000) {
							proceedToLaunch = false;
							mdRomTooBig();
						}
					}
					if (hasAP > 0) {
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
						updateText(false);
						if (ms().theme == 4) {
							while (!screenFadedIn()) { swiWaitForVBlank(); }
							dbox_showIcon = true;
							snd().playWrong();
						} else {
							for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
						}
						titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
								dirContents[scrn].at(CURPOS + PAGENUM * 40).name,
								CURPOS);
						if (hasAP == 2) {
							printSmall(false, 0, 80, STR_AP_PATCH_RGF, Alignment::center);
						} else {
							printSmall(false, 0, 72, STR_AP_USE_LATEST, Alignment::center);
						}
						printSmall(false, 0, 160, STR_B_A_OK_X_DONT_SHOW, Alignment::center);
						updateText(false);
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
							updateText(false);
							currentBg = 0;
							displayGameIcons = true;
							fadeType = true;
							snd().playStartup();
							if (proceedToLaunch) {
								while (!screenFadedIn()) { swiWaitForVBlank(); }
							}
						} else {
							clearText();
							updateText(false);
							for (int i = 0; i < (proceedToLaunch ? 20 : 15); i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
						}
						dbox_showIcon = false;
					}

					if (proceedToLaunch && useBootstrapAnyway && bnrRomType[CURPOS] == 0 && !isDSiWare[CURPOS] &&
						isHomebrew[CURPOS] == 0 &&
						checkIfDSiMode(dirContents[scrn].at(CURPOS + PAGENUM * 40).name)) {
						bool hasDsiBinaries = true;
						FILE *f_nds_file = fopen(
							dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str(), "rb");
						hasDsiBinaries = checkDsiBinaries(f_nds_file);
						fclose(f_nds_file);

						if (!hasDsiBinaries) {
							dsiBinariesMissingMsg(dirContents[scrn].at(CURPOS + PAGENUM * 40).name.c_str());
							proceedToLaunch = false;
						}
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
							updateText(false);

							fadeSpeed = false; // Slow fade speed
							for (int i = 0; i < 5; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
						}
						if (ms().theme == 5) {
							currentBg = 0;
							snd().fadeOutStream();
						} else if (ms().theme != 4) {
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
						updateText(false);

						if(ms().updateRecentlyPlayedList) {
							printLarge(false, 0, (ms().theme == 4 ? 72 : 88), STR_NOW_SAVING, Alignment::center);
							updateText(false);
							if (ms().theme == 5) {
								displayGameIcons = false;
								showProgressIcon = true;
							} else if (ms().theme != 4) {
								fadeSpeed = true; // Fast fading
								fadeType = true; // Fade in from white
								for (int i = 0; i < 25; i++) {
									swiWaitForVBlank();
								}
								showProgressIcon = true;
							}

							printSmall(false, 0, 20, STR_IF_CRASH_DISABLE_RECENT, Alignment::center);
							updateText(false);

							mkdir(sdFound() ? "sd:/_nds/TWiLightMenu/extras" : "fat:/_nds/TWiLightMenu/extras",
						  0777);

							CIniFile recentlyPlayedIni(recentlyPlayedIniPath);
							std::vector<std::string> recentlyPlayed;

							getcwd(path, PATH_MAX);
							recentlyPlayedIni.GetStringVector("RECENT", path, recentlyPlayed, ':'); // : isn't allowed in FAT-32 names, so its a good deliminator

							std::vector<std::string>::iterator it = std::find(recentlyPlayed.begin(), recentlyPlayed.end(), entry->name);
							if(it != recentlyPlayed.end()) {
								recentlyPlayed.erase(it);
							}

							recentlyPlayed.insert(recentlyPlayed.begin(), entry->name);

							recentlyPlayedIni.SetStringVector("RECENT", path, recentlyPlayed, ':');
							recentlyPlayedIni.SaveIniFile(recentlyPlayedIniPath);

							CIniFile timesPlayedIni(timesPlayedIniPath);
							timesPlayedIni.SetInt(path, entry->name, (timesPlayedIni.GetInt(path, entry->name, 0) + 1));
							timesPlayedIni.SaveIniFile(timesPlayedIniPath);

							if (ms().theme == 5) {
								displayGameIcons = true;
							} else if (ms().theme != 4) {
								showProgressIcon = false;
								fadeType = false;	   // Fade to white
								for (int i = 0; i < 25; i++) {
									swiWaitForVBlank();
								}
							}
							clearText();
							updateText(false);
						}

						// Return the chosen file
						return entry->name;
					}
				}
			}
			gameTapped = false;

			if (ms().theme == 1) {
				// Launch TWLMenu++ Settings by touching corner button
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
				if ((isDSiMode() && sdFound()) || bothSDandFlashcard()) {
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

				if ((isDSiMode() && sdFound()) || bothSDandFlashcard()) {
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
				if (previousPage()) {
					break;
				}
			} else if (pressed & KEY_R) {
				if (nextPage()) {
					break;
				}
			}

			if ((pressed & KEY_B) && ms().showDirectories && !titleboxXmoveleft && !titleboxXmoveright) {
				// Go up a directory
				snd().playBack();
				if (ms().theme != 4 && ms().theme != 5) {
					fadeType = false; // Fade to white
					for (int i = 0; i < 6; i++) {
						snd().updateStream();
						swiWaitForVBlank();
					}
				}
				PAGENUM = 0;
				CURPOS = 0;
				titleboxXpos[ms().secondaryDevice] = 0;
				titlewindowXpos[ms().secondaryDevice] = 0;
				if (ms().theme != 4 && ms().theme != 5) whiteScreen = true;
				if (ms().showBoxArt)
					clearBoxArt(); // Clear box art
				boxArtLoaded = false;
				bannerTextShown = false;
				rocketVideo_playVideo = true;
				shouldersRendered = false;
				currentBg = 0;
				showSTARTborder = false;
				stopSoundPlayed = false;
				clearText();
				updateText(false);
				chdir("..");
				char buf[256];

				ms().romfolder[ms().secondaryDevice] = std::string(getcwd(buf, 256));
				ms().saveSettings();
				settingsChanged = false;
				return "null";
			}

			if ((pressed & KEY_X) && !ms().preventDeletion && bannerTextShown && showSTARTborder
			&& dirContents[scrn].at(CURPOS + PAGENUM * 40).name != "..") {
				DirEntry *entry = &dirContents[scrn].at((PAGENUM * 40) + (CURPOS));
				bool unHide = (FAT_getAttr(entry->name.c_str()) & ATTR_HIDDEN || (strncmp(entry->name.c_str(), ".", 1) == 0 && entry->name != ".."));
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
				updateText(false);
				if (ms().theme == 4) {
					while (!screenFadedIn()) { swiWaitForVBlank(); }
					dbox_showIcon = true;
				} else {
					for (int i = 0; i < 30; i++) { snd().updateStream(); swiWaitForVBlank(); }
				}
				snprintf(fileCounter, sizeof(fileCounter), "%i/%i", (CURPOS + 1) + PAGENUM * 40,
					 file_count);
				titleUpdate(dirContents[scrn].at(CURPOS + PAGENUM * 40).isDirectory,
						dirContents[scrn].at(CURPOS + PAGENUM * 40).name, CURPOS);
				dirContName = dirContents[scrn].at(CURPOS + PAGENUM * 40).name;
				// About 38 characters fit in the box.
				if (strlen(dirContName.c_str()) > 38) {
					// Truncate to 35, 35 + 3 = 38 (because we append "...").
					dirContName.resize(35, ' ');
					size_t first = dirContName.find_first_not_of(' ');
					size_t last = dirContName.find_last_not_of(' ');
					dirContName = dirContName.substr(first, (last - first + 1));
					dirContName.append("...");
				}
				printSmall(false, 16, 66, dirContName);
				printSmall(false, 16, 160, fileCounter);
				if (isDirectory[CURPOS]) {
					if (unHide)
						printSmall(false, 0, 112, STR_ARE_YOU_SURE_UNHIDE, Alignment::center);
					else
						printSmall(false, 0, 112, STR_ARE_YOU_SURE_HIDE, Alignment::center);
				} else {
					if (unHide)
						printSmall(false, 0, 112, STR_ARE_YOU_SURE_DELETE_UNHIDE, Alignment::center);
					else
						printSmall(false, 0, 112, STR_ARE_YOU_SURE_DELETE_HIDE, Alignment::center);
				}
				updateText(false);
				for (int i = 0; i < 90; i++) {
					snd().updateStream();
					swiWaitForVBlank();
				}
				if (isDirectory[CURPOS]) {
					printSmall(false, 240, 160, (unHide ? STR_Y_UNHIDE : STR_Y_HIDE) + "  " + STR_B_NO, Alignment::right);
				} else {
					printSmall(false, 240, 160, (unHide ? STR_Y_UNHIDE : STR_Y_HIDE) + "  " + STR_A_DEL + "  " + STR_B_NO, Alignment::right);
				}
				updateText(false);
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
						if (ms().theme != 4 && ms().theme != 5) {
							fadeType = false; // Fade to white
							for (int i = 0; i < 30; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							whiteScreen = true;
						}
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
						updateText(false);
						showdialogbox = false;
						dbox_showIcon = false;
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
						if (ms().theme != 4 && ms().theme != 5) {
							fadeType = false; // Fade to white
							for (int i = 0; i < 30; i++) {
								snd().updateStream();
								swiWaitForVBlank();
							}
							whiteScreen = true;
						}

						// Remove leading . if it exists
						if((strncmp(entry->name.c_str(), ".", 1) == 0 && entry->name != "..")) {
							rename(entry->name.c_str(), entry->name.substr(1).c_str());
						} else { // Otherwise toggle the hidden attribute bit
							FAT_setAttr(entry->name.c_str(), FAT_getAttr(entry->name.c_str()) ^ ATTR_HIDDEN);
						}

						if (ms().showBoxArt)
							clearBoxArt(); // Clear box art
						boxArtLoaded = false;
						bannerTextShown = false;
						shouldersRendered = false;
						currentBg = 0;
						showSTARTborder = false;
						stopSoundPlayed = false;
						clearText();
						updateText(false);
						showdialogbox = false;
						dbox_showIcon = false;
						ms().saveSettings();
						settingsChanged = false;
						return "null";
					}
				}
				showdialogbox = false;
				if (ms().theme == 5) {
					dbox_showIcon = false;
				}
				if (ms().theme == 4) {
					fadeType = false;	   // Fade to black
					for (int i = 0; i < 25; i++) {
						swiWaitForVBlank();
					}
					clearText();
					updateText(false);
					currentBg = 0;
					displayGameIcons = true;
					fadeType = true;
					if (ms().theme == 4) snd().playStartup();
				} else {
					clearText();
					updateText(false);
					for (int i = 0; i < 15; i++) { snd().updateStream(); swiWaitForVBlank(); }
				}
				dbox_showIcon = false;
				bannerTextShown = false;
			}

			if ((pressed & KEY_Y) && (isDirectory[CURPOS] == false) &&
				(bnrRomType[CURPOS] == 0) && !titleboxXmoveleft && !titleboxXmoveright &&
				bannerTextShown && showSTARTborder) {
				perGameSettings(dirContents[scrn].at(CURPOS + PAGENUM * 40).name);
				bannerTextShown = false;
			}

			if (held & KEY_SELECT) {
				bannerTextShown = false;
				bool runSelectMenu = true;
				bool break2 = false;
				while (held & KEY_SELECT) {
					scanKeys();
					pressed = keysDown();
					held = keysHeld();
					updateScrollingState(held, pressed);
					checkSdEject();
					tex().drawVolumeImageCached();
					tex().drawBatteryImageCached();
					drawCurrentTime();
					drawCurrentDate();
					drawClockColon();
					snd().updateStream();
					swiWaitForVBlank();

					// page switch
					if (pressed & KEY_LEFT) {
						runSelectMenu = false;
						if (previousPage()) {
							break2 = true;
							break;
						}
					} else if (pressed & KEY_RIGHT) {
						runSelectMenu = false;
						if (nextPage()) {
							break2 = true;
							break;
						}
					}

					if (ms().theme == 0 || ms().theme == 4 || ms().theme == 5) {
						if (bothSDandFlashcard() && ((pressed & KEY_UP) || (pressed & KEY_DOWN)))
						{
							switchDevice();
							return "null";
						}
					}
				}
				if (break2) break;
				if (runSelectMenu && (ms().theme == 0 || ms().theme == 4 || ms().theme == 5)) {
					if (ms().showSelectMenu) {
						if (selectMenu()) {
							clearText();
							updateText(false);
							showdialogbox = false;
							dbox_selectMenu = false;
							if (ms().theme == 4) currentBg = 0;
							return "null";
						}
					} else {
						launchDsClassicMenu();
					}
				}
			}
		}
	}
}
