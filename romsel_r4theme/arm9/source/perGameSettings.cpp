#include "perGameSettings.h"
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <string>
#include <sstream>
#include <stdio.h>
#include <dirent.h>

#include <nds.h>
#include <nds/arm9/dldi.h>
#include <gl2d.h>

#include "sound.h"
#include "date.h"

#include "ndsheaderbanner.h"
#include "iconTitle.h"
#include "graphics/fontHandler.h"
#include "graphics/graphics.h"
#include "graphics/FontGraphic.h"
#include "graphics/TextPane.h"
#include "SwitchState.h"
#include "cheat.h"
#include "errorScreen.h"

#include "gbaswitch.h"
#include "nds_loader_arm9.h"

#include "common/inifile.h"
#include "flashcard.h"

#define SCREEN_COLS 32
#define ENTRIES_PER_SCREEN 15
#define ENTRIES_START_ROW 3
#define ENTRY_PAGE_LENGTH 10

extern const char *bootstrapinipath;

extern bool isRegularDS;
extern int consoleModel;

extern int bstrap_dsiMode;
extern bool useBootstrap;
extern int theme;

extern std::string romfolder[2];

const char* SDKnumbertext;

extern bool showdialogbox;
extern int dialogboxHeight;

bool perGameSettingsChanged = false;

int perGameSettings_cursorPosition = 0;
bool perGameSettings_directBoot = false;	// Homebrew only
int perGameSettings_dsiMode = -1;
int perGameSettings_language = -2;
int perGameSettings_saveNo = 0;
int perGameSettings_ramDiskNo = -1;
int perGameSettings_boostCpu = -1;
int perGameSettings_boostVram = -1;
int perGameSettings_heapShrink = -1;
int perGameSettings_bootstrapFile = -1;
int perGameSettings_wideScreen = -1;
int perGameSettings_expandRomSpace = -1;

static char SET_AS_DONOR_ROM[32];

char pergamefilepath[256];

extern void RemoveTrailingSlashes(std::string &path);

extern char usernameRendered[10];
extern bool usernameRenderedDone;

char fileCounter[8];
char gameTIDText[16];

int firstPerGameOpShown = 0;
int perGameOps = -1;
int perGameOp[10] = {-1};

void loadPerGameSettings (std::string filename) {
	snprintf(pergamefilepath, sizeof(pergamefilepath), "%s/_nds/TWiLightMenu/gamesettings/%s.ini", (secondaryDevice ? "fat:" : "sd:"), filename.c_str());
	CIniFile pergameini( pergamefilepath );
	perGameSettings_directBoot = pergameini.GetInt("GAMESETTINGS", "DIRECT_BOOT", (isModernHomebrew || previousUsedDevice));	// Homebrew only
	if ((isDSiMode() && useBootstrap) || !secondaryDevice) {
		perGameSettings_dsiMode = pergameini.GetInt("GAMESETTINGS", "DSI_MODE", (isModernHomebrew ? true : -1));
	} else {
		perGameSettings_dsiMode = -1;
	}
	perGameSettings_language = pergameini.GetInt("GAMESETTINGS", "LANGUAGE", -2);
	perGameSettings_saveNo = pergameini.GetInt("GAMESETTINGS", "SAVE_NUMBER", 0);
	perGameSettings_ramDiskNo = pergameini.GetInt("GAMESETTINGS", "RAM_DISK", -1);
	perGameSettings_boostCpu = pergameini.GetInt("GAMESETTINGS", "BOOST_CPU", -1);
	perGameSettings_boostVram = pergameini.GetInt("GAMESETTINGS", "BOOST_VRAM", -1);
	perGameSettings_heapShrink = pergameini.GetInt("GAMESETTINGS", "HEAP_SHRINK", -1);
	perGameSettings_bootstrapFile = pergameini.GetInt("GAMESETTINGS", "BOOTSTRAP_FILE", -1);
	perGameSettings_wideScreen = pergameini.GetInt("GAMESETTINGS", "WIDESCREEN", -1);
    perGameSettings_expandRomSpace = pergameini.GetInt("GAMESETTINGS", "EXTENDED_MEMORY", -1);
}

void savePerGameSettings (std::string filename) {
	snprintf(pergamefilepath, sizeof(pergamefilepath), "%s/_nds/TWiLightMenu/gamesettings/%s.ini", (secondaryDevice ? "fat:" : "sd:"), filename.c_str());
	CIniFile pergameini( pergamefilepath );
	if (isHomebrew) {
		if (!secondaryDevice) pergameini.SetInt("GAMESETTINGS", "LANGUAGE", perGameSettings_language);
		pergameini.SetInt("GAMESETTINGS", "DIRECT_BOOT", perGameSettings_directBoot);
		if (isDSiMode() || !secondaryDevice) {
			pergameini.SetInt("GAMESETTINGS", "DSI_MODE", perGameSettings_dsiMode);
		}
		if (!secondaryDevice) pergameini.SetInt("GAMESETTINGS", "RAM_DISK", perGameSettings_ramDiskNo);
		if (REG_SCFG_EXT != 0) {
			pergameini.SetInt("GAMESETTINGS", "BOOST_CPU", perGameSettings_boostCpu);
			pergameini.SetInt("GAMESETTINGS", "BOOST_VRAM", perGameSettings_boostVram);
		}
	} else {
		if (useBootstrap || !secondaryDevice) pergameini.SetInt("GAMESETTINGS", "LANGUAGE", perGameSettings_language);
		if ((isDSiMode() && useBootstrap) || !secondaryDevice) {
			pergameini.SetInt("GAMESETTINGS", "DSI_MODE", perGameSettings_dsiMode);
		}
		pergameini.SetInt("GAMESETTINGS", "SAVE_NUMBER", perGameSettings_saveNo);
		if (REG_SCFG_EXT != 0) {
			pergameini.SetInt("GAMESETTINGS", "BOOST_CPU", perGameSettings_boostCpu);
			pergameini.SetInt("GAMESETTINGS", "BOOST_VRAM", perGameSettings_boostVram);
		}
		if (useBootstrap || !secondaryDevice) {
			pergameini.SetInt("GAMESETTINGS", "HEAP_SHRINK", perGameSettings_heapShrink);
			pergameini.SetInt("GAMESETTINGS", "BOOTSTRAP_FILE", perGameSettings_bootstrapFile);
		}
		if (isDSiMode() && consoleModel >= 2 && sdFound()) {
			pergameini.SetInt("GAMESETTINGS", "WIDESCREEN", perGameSettings_wideScreen);
		}
		if ((isDSiMode() && useBootstrap) || !secondaryDevice) {
			pergameini.SetInt("GAMESETTINGS", "EXTENDED_MEMORY", perGameSettings_expandRomSpace);
		}
	}
	pergameini.SaveIniFile( pergamefilepath );
}

bool checkIfShowAPMsg (std::string filename) {
	snprintf(pergamefilepath, sizeof(pergamefilepath), "%s/_nds/TWiLightMenu/gamesettings/%s.ini", (secondaryDevice ? "fat:" : "sd:"), filename.c_str());
	CIniFile pergameini( pergamefilepath );
	if (pergameini.GetInt("GAMESETTINGS", "NO_SHOW_AP_MSG", 0) == 0) {
		return true;	// Show AP message
	}
	return false;	// Don't show AP message
}

void dontShowAPMsgAgain (std::string filename) {
	snprintf(pergamefilepath, sizeof(pergamefilepath), "%s/_nds/TWiLightMenu/gamesettings/%s.ini", (secondaryDevice ? "fat:" : "sd:"), filename.c_str());
	CIniFile pergameini( pergamefilepath );
	pergameini.SetInt("GAMESETTINGS", "NO_SHOW_AP_MSG", 1);
	pergameini.SaveIniFile( pergamefilepath );
}

bool checkIfDSiMode (std::string filename) {
	if (secondaryDevice && (!isDSiMode() || !useBootstrap)) {
		return false;
	}

	snprintf(pergamefilepath, sizeof(pergamefilepath), "%s/_nds/TWiLightMenu/gamesettings/%s.ini", (secondaryDevice ? "fat:" : "sd:"), filename.c_str());
	CIniFile pergameini( pergamefilepath );
	perGameSettings_dsiMode = pergameini.GetInt("GAMESETTINGS", "DSI_MODE", (isModernHomebrew ? true : -1));
	if (perGameSettings_dsiMode == -1) {
		return bstrap_dsiMode;
	} else {
		return perGameSettings_dsiMode;
	}
}

bool showSetDonorRom(u32 arm7size, u32 SDKVersion) {
	if (requiresDonorRom) return false;

	bool hasCycloDSi = (memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0);

	if (((!isDSiMode() || hasCycloDSi) && SDKVersion > 0x2000000 && SDKVersion < 0x2008000	// Early SDK2
	 && (arm7size==0x25F70
	  || arm7size==0x25FA4
	  || arm7size==0x2619C
	  || arm7size==0x26A60
	  || arm7size==0x27218
	  || arm7size==0x27224
	  || arm7size==0x2724C))
	 || (SDKVersion >= 0x2008000 && SDKVersion < 0x3000000 && (arm7size==0x26F24 || arm7size==0x26F28))	// Late SDK2
	 || ((!isDSiMode() || hasCycloDSi) && SDKVersion > 0x3000000 && SDKVersion < 0x4000000	// SDK3
	 && (arm7size==0x28464
	  || arm7size==0x28684
	  || arm7size==0x286A0
	  || arm7size==0x289A4
	  || arm7size==0x289C0
	  || arm7size==0x289F8
	  || arm7size==0x28FFC))
	 || (SDKVersion > 0x5000000 && (arm7size==0x26370 || arm7size==0x2642C || arm7size==0x26488))		// SDK5 (NTR)
	 || ((!isDSiMode() || hasCycloDSi) && SDKVersion > 0x5000000	// SDK5 (TWL)
	 && (arm7size==0x28F84
	  || arm7size==0x2909C
	  || arm7size==0x2914C
	  || arm7size==0x29164
	  || arm7size==0x29EE8
	  || arm7size==0x2A2EC
	  || arm7size==0x2A318
	  || arm7size==0x2AF18
	  || arm7size==0x2B184
	  || arm7size==0x2B24C
	  || arm7size==0x2C5B4)))
	{
		return true;
	}

	return false;
}

bool donorRomTextShown = false;
void revertDonorRomText(void) {
	if (!donorRomTextShown || strncmp(SET_AS_DONOR_ROM, "Done!", 5) != 0) return;

	sprintf(SET_AS_DONOR_ROM, "Set as Donor ROM");
}

void perGameSettings (std::string filename) {
	int pressed = 0, held = 0;

	keysSetRepeat(25, 5); // Slow down key repeat

	clearText();
	
	perGameSettings_cursorPosition = 0;
	loadPerGameSettings(filename);

	std::string filenameForInfo = filename;
	if((filenameForInfo.substr(filenameForInfo.find_last_of(".") + 1) == "argv")
	|| (filenameForInfo.substr(filenameForInfo.find_last_of(".") + 1) == "ARGV"))
	{
		std::vector<char*> argarray;

		FILE *argfile = fopen(filenameForInfo.c_str(),"rb");
			char str[PATH_MAX], *pstr;
		const char seps[]= "\n\r\t ";

		while( fgets(str, PATH_MAX, argfile) ) {
			// Find comment and end string there
			if( (pstr = strchr(str, '#')) )
				*pstr= '\0';

			// Tokenize arguments
			pstr= strtok(str, seps);

			while( pstr != NULL ) {
				argarray.push_back(strdup(pstr));
				pstr= strtok(NULL, seps);
			}
		}
		fclose(argfile);
		filenameForInfo = argarray.at(0);
	}

	FILE *f_nds_file = fopen(filenameForInfo.c_str(), "rb");

	char game_TID[5];
	grabTID(f_nds_file, game_TID);
	game_TID[4] = 0;

	bool showSDKVersion = false;
	u32 SDKVersion = 0;
	if (memcmp(game_TID, "HND", 3) == 0 || memcmp(game_TID, "HNE", 3) == 0 || !isHomebrew) {
		SDKVersion = getSDKVersion(f_nds_file);
		showSDKVersion = true;
	}
	u8 unitCode = 0;
	u32 arm9dst = 0;
	u32 arm9size = 0;
	u32 arm7off = 0;
	u32 arm7size = 0;
	u32 romSize = 0;
	fseek(f_nds_file, 0x12, SEEK_SET);
	fread(&unitCode, sizeof(u8), 1, f_nds_file);
	fseek(f_nds_file, 0x28, SEEK_SET);
	fread(&arm9dst, sizeof(u32), 1, f_nds_file);
	fseek(f_nds_file, 0x2C, SEEK_SET);
	fread(&arm9size, sizeof(u32), 1, f_nds_file);
	fseek(f_nds_file, 0x30, SEEK_SET);
	fread(&arm7off, sizeof(u32), 1, f_nds_file);
	fseek(f_nds_file, 0x3C, SEEK_SET);
	fread(&arm7size, sizeof(u32), 1, f_nds_file);
	fseek(f_nds_file, 0x80, SEEK_SET);
	fread(&romSize, sizeof(u32), 1, f_nds_file);
	fclose(f_nds_file);

	if (romSize > 0) {
		u32 overlaysSize = 0;
		// Calculate overlay pack size
		for (u32 i = 0x4000+arm9size; i < arm7off; i++) {
			overlaysSize++;
		}

		romSize -= arm7off;
		romSize -= arm7size;
		romSize += overlaysSize;
	}

	u32 romSizeLimit = (consoleModel > 0 ? 0x01800000 : 0x800000);
	if (SDKVersion > 0x5000000) {
		romSizeLimit = (consoleModel > 0 ? 0x01000000 : 0);
	}
	u32 romSizeLimit2 = (consoleModel > 0 ? 0x01C00000 : 0xC00000);

	bool showPerGameSettings =
		(!isDSiWare
		&& memcmp(game_TID, "HND", 3) != 0
		&& memcmp(game_TID, "HNE", 3) != 0);
	/*if (!useBootstrap && !isHomebrew && REG_SCFG_EXT == 0) {
		showPerGameSettings = false;
	}*/

	bool showCheats = (((isDSiMode() && useBootstrap)
	/*|| (secondaryDevice && !useBootstrap
		&& ((memcmp(io_dldi_data->friendlyName, "R4(DS) - Revolution for DS", 26) == 0)
		 || (memcmp(io_dldi_data->friendlyName, "R4TF", 4) == 0)
		 || (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0)
		 || (memcmp(io_dldi_data->friendlyName, "Acekard AK2", 0xB) == 0)))*/
	|| !secondaryDevice) && !isDSiWare);

	firstPerGameOpShown = 0;
	perGameOps = -1;
	for (int i = 0; i < 10; i++) {
		perGameOp[i] = -1;
	}
	if (isHomebrew) {		// Per-game settings for homebrew
		if (!secondaryDevice) {
			perGameOps++;
			perGameOp[perGameOps] = 0;	// Language
			perGameOps++;
			perGameOp[perGameOps] = 1;	// RAM disk number
		}
		perGameOps++;
		perGameOp[perGameOps] = 6;	// Direct boot
		if (isDSiMode() || !secondaryDevice) {
			perGameOps++;
			perGameOp[perGameOps] = 2;	// Run in
		}
		if ((REG_SCFG_EXT != 0) || !secondaryDevice) {
			perGameOps++;
			perGameOp[perGameOps] = 3;	// ARM9 CPU Speed
			perGameOps++;
			perGameOp[perGameOps] = 4;	// VRAM Boost
		}
		if (!secondaryDevice) {
			perGameOps++;
			perGameOp[perGameOps] = 7;	// Bootstrap
		}
		if (isDSiMode() && consoleModel >= 2 && sdFound() && (game_TID[0] == 'W' || romVersion == 0x57)) {
			perGameOps++;
			perGameOp[perGameOps] = 8;	// Screen Aspect Ratio
		}
	} else if (showPerGameSettings) {	// Per-game settings for retail/commercial games
		if (useBootstrap || !secondaryDevice) {
			perGameOps++;
			perGameOp[perGameOps] = 0;	// Language
		}
		perGameOps++;
		perGameOp[perGameOps] = 1;	// Save number
		if ((isDSiMode() && useBootstrap) || !secondaryDevice) {
			perGameOps++;
			perGameOp[perGameOps] = 2;	// Run in
		}
		if ((REG_SCFG_EXT != 0) || !secondaryDevice) {
			perGameOps++;
			perGameOp[perGameOps] = 3;	// ARM9 CPU Speed
			perGameOps++;
			perGameOp[perGameOps] = 4;	// VRAM Boost
		}
		if (useBootstrap || !secondaryDevice) {
			if ((isDSiMode() || !secondaryDevice) && arm9dst != 0x02004000 && SDKVersion >= 0x2008000 && SDKVersion < 0x5000000) {
				perGameOps++;
				perGameOp[perGameOps] = 5;	// Heap shrink
			}
			if ((isDSiMode() || !secondaryDevice)
			 && romSize > romSizeLimit && romSize <= romSizeLimit2+0x80000) {
				perGameOps++;
				perGameOp[perGameOps] = 10;	// Expand ROM space in RAM
			}
			perGameOps++;
			perGameOp[perGameOps] = 7;	// Bootstrap
			if (((isDSiMode() && sdFound()) || !secondaryDevice) && consoleModel >= 2) {
				perGameOps++;
				perGameOp[perGameOps] = 8;	// Screen Aspect Ratio
			}
			if (showSetDonorRom(arm7size, SDKVersion)) {
				perGameOps++;
				perGameOp[perGameOps] = 9;	// Set as Donor ROM
				donorRomTextShown = true;
			} else {
				donorRomTextShown = false;
			}
		}
	}

	snprintf (gameTIDText, sizeof(gameTIDText), game_TID[0]==0 ? "" : "TID: %s", game_TID);

	char saveNoDisplay[16];

	if((SDKVersion > 0x1000000) && (SDKVersion < 0x2000000)) {
		SDKnumbertext = "SDK ver: 1";
	} else if((SDKVersion > 0x2000000) && (SDKVersion < 0x3000000)) {
		SDKnumbertext = "SDK ver: 2";
	} else if((SDKVersion > 0x3000000) && (SDKVersion < 0x4000000)) {
		SDKnumbertext = "SDK ver: 3";
	} else if((SDKVersion > 0x4000000) && (SDKVersion < 0x5000000)) {
		SDKnumbertext = "SDK ver: 4";
	} else if((SDKVersion > 0x5000000) && (SDKVersion < 0x6000000)) {
		SDKnumbertext = "SDK ver: 5 (TWLSDK)";
	} else {
		SDKnumbertext = "SDK ver: ?";
	}
	if (!showPerGameSettings) {
		dialogboxHeight = 0;
	} else {
		dialogboxHeight = 4;
	}

	sprintf(SET_AS_DONOR_ROM, "Set as Donor ROM");

	showdialogbox = true;

	while (1) {
		clearText();
		if (theme == 6) {
			printLarge(false, 0, 56, filename.c_str());
		} else {
			titleUpdate(isDirectory, filename.c_str());
		}

		printLargeCentered(false, 74, showPerGameSettings ? "Game settings" : "Info");
		if (showSDKVersion) printSmall(false, 24, 90, SDKnumbertext);
		printSmallRightAlign(false, 232, 90, gameTIDText);

		int perGameOpYpos = 102;

		if (showPerGameSettings) {
			printSmall(false, 24, 102+(perGameSettings_cursorPosition*12)-(firstPerGameOpShown*12), ">");
		}

		for (int i = firstPerGameOpShown; i < firstPerGameOpShown+4; i++) {
		if (!showPerGameSettings || perGameOp[i] == -1) break;
		switch (perGameOp[i]) {
			case 0:
				printSmall(false, 32, perGameOpYpos, "Language:");
				if (perGameSettings_language == -2) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Default");
				} else if (perGameSettings_language == -1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "System");
				} else if (perGameSettings_language == 0) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Japanese");
				} else if (perGameSettings_language == 1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "English");
				} else if (perGameSettings_language == 2) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "French");
				} else if (perGameSettings_language == 3) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "German");
				} else if (perGameSettings_language == 4) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Italian");
				} else if (perGameSettings_language == 5) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Spanish");
				} else if (perGameSettings_language == 6) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Chinese");
				} else if (perGameSettings_language == 7) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Korean");
				}
				break;
			case 1:
				if (isHomebrew) {
					printSmall(false, 32, perGameOpYpos, "RAM disk:");
					snprintf (saveNoDisplay, sizeof(saveNoDisplay), "%i", perGameSettings_ramDiskNo);
				} else {
					printSmall(false, 32, perGameOpYpos, "Save Number:");
					snprintf (saveNoDisplay, sizeof(saveNoDisplay), "%i", perGameSettings_saveNo);
				}
				if (isHomebrew && perGameSettings_ramDiskNo == -1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "None");
				} else {
					printSmallRightAlign(false, 256-24, perGameOpYpos, saveNoDisplay);
				}
				break;
			case 2:
				printSmall(false, 32, perGameOpYpos, "Run in:");
				if (perGameSettings_dsiMode == -1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Default");
				} else if (perGameSettings_dsiMode == 2) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "DSi mode (Forced)");
				} else if (perGameSettings_dsiMode == 1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "DSi mode");
				} else {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "DS mode");
				}
				break;
			case 3:
				printSmall(false, 32, perGameOpYpos, "ARM9 CPU Speed:");
				if (perGameSettings_dsiMode > 0 && (isDSiMode() || !secondaryDevice)) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "133mhz (TWL)");
				} else {
					if (perGameSettings_boostCpu == -1) {
						printSmallRightAlign(false, 256-24, perGameOpYpos, "Default");
					} else if (perGameSettings_boostCpu == 1) {
						printSmallRightAlign(false, 256-24, perGameOpYpos, "133mhz (TWL)");
					} else {
						printSmallRightAlign(false, 256-24, perGameOpYpos, "67mhz (NTR)");
					}
				}
				break;
			case 4:
				printSmall(false, 32, perGameOpYpos, "VRAM Boost:");
				if (perGameSettings_dsiMode > 0 && (isDSiMode() || !secondaryDevice)) {
					printSmallRightAlign(false, 180, perGameOpYpos, "On");
				} else {
					if (perGameSettings_boostVram == -1) {
						printSmallRightAlign(false, 256-24, perGameOpYpos, "Default");
					} else if (perGameSettings_boostVram == 1) {
						printSmallRightAlign(false, 256-24, perGameOpYpos, "On");
					} else {
						printSmallRightAlign(false, 256-24, perGameOpYpos, "Off");
					}
				}
				break;
			case 5:
				printSmall(false, 32, perGameOpYpos, "Heap Shrink:");
				if (perGameSettings_heapShrink == -1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Auto");
				} else if (perGameSettings_heapShrink == 1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "On");
				} else {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Off");
				}
				break;
			case 6:
				printSmall(false, 32, perGameOpYpos, "Direct Boot:");
				if (perGameSettings_directBoot) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Yes");
				} else {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "No");
				}
				break;
			case 7:
				printSmall(false, 32, perGameOpYpos, "Bootstrap:");
				if (perGameSettings_bootstrapFile == -1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Default");
				} else if (perGameSettings_bootstrapFile == 1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Nightly");
				} else {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Release");
				}
				break;
			case 8:
				printSmall(false, 32, perGameOpYpos, "Screen Aspect Ratio:");
				if (perGameSettings_wideScreen == -1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Default");
				} else if (perGameSettings_wideScreen == 1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "16:10");
				} else {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "4:3");
				}
				break;
			case 9:
				printSmallCentered(false, perGameOpYpos, SET_AS_DONOR_ROM);
				break;
			case 10:
				printSmall(false, 32, perGameOpYpos, "Ex. ROM space in RAM:");
				if (perGameSettings_expandRomSpace == -1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Default");
				} else if (perGameSettings_expandRomSpace == 2) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Yes+512KB");
				} else if (perGameSettings_expandRomSpace == 1) {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "Yes");
				} else {
					printSmallRightAlign(false, 256-24, perGameOpYpos, "No");
				}
				break;
		}
		perGameOpYpos += 12;
		}
		if (isHomebrew) {		// Per-game settings for homebrew
			printSmallCentered(false, 132, "\u2428 Back");
		} else if (!showPerGameSettings && !showCheats) {
			printSmallCentered(false, 104, "\u2427 OK");
		} else { 	// Per-game settings for retail/commercial games
			printSmallCentered(false, 154, showCheats ? "\u2429 Cheats  \u2428 Back" : "\u2428 Back");
		}
		do {
			snd().updateStream();
			scanKeys();
			pressed = keysDown();
			held = keysDownRepeat();
			checkSdEject();
			swiWaitForVBlank();
		} while (!held);

		if (!showPerGameSettings) {
			if ((pressed & KEY_A) || (pressed & KEY_B)) {
				break;
			}
		} else {
			if (held & KEY_UP) {
				revertDonorRomText();
				perGameSettings_cursorPosition--;
				if (perGameSettings_cursorPosition < 0) {
					perGameSettings_cursorPosition = perGameOps;
					firstPerGameOpShown = (perGameOps>2 ? perGameOps-3 : 0);
				} else if (perGameSettings_cursorPosition < firstPerGameOpShown) {
					firstPerGameOpShown--;
				}
			}
			if (held & KEY_DOWN) {
				revertDonorRomText();
				perGameSettings_cursorPosition++;
				if (perGameSettings_cursorPosition > perGameOps) {
					perGameSettings_cursorPosition = 0;
					firstPerGameOpShown = 0;
				} else if (perGameSettings_cursorPosition > firstPerGameOpShown+3) {
					firstPerGameOpShown++;
				}
			}

			if (held & KEY_LEFT) {
				switch (perGameOp[perGameSettings_cursorPosition]) {
					case 0:
						perGameSettings_language--;
						if (perGameSettings_language < -2) perGameSettings_language = 7;
						break;
					case 1:
						if (isHomebrew) {
							if (!perGameSettings_directBoot) {
								perGameSettings_ramDiskNo--;
								if (perGameSettings_ramDiskNo < -1) perGameSettings_ramDiskNo = 9;
							}
						} else {
							perGameSettings_saveNo--;
							if (perGameSettings_saveNo < 0) perGameSettings_saveNo = 9;
						}
						break;
					case 2:
						perGameSettings_dsiMode--;
						if (perGameSettings_dsiMode < -1) perGameSettings_dsiMode = 2-isHomebrew;
						break;
					case 3:
						if (perGameSettings_dsiMode < 1) {
							perGameSettings_boostCpu--;
							if (perGameSettings_boostCpu < -1) perGameSettings_boostCpu = 1;
						}
						break;
					case 4:
						if (perGameSettings_dsiMode < 1) {
							perGameSettings_boostVram--;
							if (perGameSettings_boostVram < -1) perGameSettings_boostVram = 1;
						}
						break;
					case 5:
						perGameSettings_heapShrink--;
						if (perGameSettings_heapShrink < -1) perGameSettings_heapShrink = 1;
						break;
					case 6:
						perGameSettings_directBoot = !perGameSettings_directBoot;
						break;
					case 7:
						perGameSettings_bootstrapFile--;
						if (perGameSettings_bootstrapFile < -1) perGameSettings_bootstrapFile = 1;
						break;
					case 8:
						perGameSettings_wideScreen++;
						if (perGameSettings_wideScreen > 1) perGameSettings_wideScreen = -1;
						break;
					case 10:
						perGameSettings_expandRomSpace--;
						if (perGameSettings_expandRomSpace==1 && romSize > romSizeLimit2) {
							perGameSettings_expandRomSpace--;
						} else if (perGameSettings_expandRomSpace==2 && romSize <= romSizeLimit2) {
							perGameSettings_expandRomSpace--;
						}
						if (perGameSettings_expandRomSpace < -1) perGameSettings_expandRomSpace = 2;
						break;
				}
				perGameSettingsChanged = true;
			} else if ((pressed & KEY_A) || (held & KEY_RIGHT)) {
				switch (perGameOp[perGameSettings_cursorPosition]) {
					case 0:
						perGameSettings_language++;
						if (perGameSettings_language > 7) perGameSettings_language = -2;
						break;
					case 1:
						if (isHomebrew) {
							if (!perGameSettings_directBoot) {
								perGameSettings_ramDiskNo++;
								if (perGameSettings_ramDiskNo > 9) perGameSettings_ramDiskNo = -1;
							}
						} else {
							perGameSettings_saveNo++;
							if (perGameSettings_saveNo > 9) perGameSettings_saveNo = 0;
						}
						break;
					case 2:
						perGameSettings_dsiMode++;
						if (perGameSettings_dsiMode > 2-isHomebrew) perGameSettings_dsiMode = -1;
						break;
					case 3:
						if (perGameSettings_dsiMode < 1) {
							perGameSettings_boostCpu++;
							if (perGameSettings_boostCpu > 1) perGameSettings_boostCpu = -1;
						}
						break;
					case 4:
						if (perGameSettings_dsiMode < 1) {
							perGameSettings_boostVram++;
							if (perGameSettings_boostVram > 1) perGameSettings_boostVram = -1;
						}
						break;
					case 5:
						perGameSettings_heapShrink++;
						if (perGameSettings_heapShrink > 1) perGameSettings_heapShrink = -1;
						break;
					case 6:
						perGameSettings_directBoot = !perGameSettings_directBoot;
						break;
					case 7:
						perGameSettings_bootstrapFile++;
						if (perGameSettings_bootstrapFile > 1) perGameSettings_bootstrapFile = -1;
						break;
					case 8:
						perGameSettings_wideScreen++;
						if (perGameSettings_wideScreen > 1) perGameSettings_wideScreen = -1;
						break;
					case 9:
					  if (pressed & KEY_A) {
						const char* pathDefine = "DONOR_NDS_PATH";
						if (SDKVersion > 0x2000000 && SDKVersion < 0x2008000) {
							pathDefine = "DONORE2_NDS_PATH";
						} else if (SDKVersion > 0x2008000 && SDKVersion < 0x3000000) {
							pathDefine = "DONOR2_NDS_PATH";
						} else if (SDKVersion > 0x3000000 && SDKVersion < 0x5000000) {
							pathDefine = "DONOR3_NDS_PATH";
						} else if (unitCode > 0 && SDKVersion > 0x5000000) {
							pathDefine = "DONORTWL_NDS_PATH";
						}
						std::string romFolderNoSlash = romfolder[secondaryDevice];
						RemoveTrailingSlashes(romFolderNoSlash);
						bootstrapinipath = ((!secondaryDevice || (isDSiMode() && sdFound())) ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini");
						CIniFile bootstrapini(bootstrapinipath);
						bootstrapini.SetString("NDS-BOOTSTRAP", pathDefine, romFolderNoSlash+"/"+filename);
						bootstrapini.SaveIniFile(bootstrapinipath);
						sprintf(SET_AS_DONOR_ROM, "Done!");
					  }
						break;
					case 10:
						perGameSettings_expandRomSpace++;
						if (perGameSettings_expandRomSpace==1 && romSize > romSizeLimit2) {
							perGameSettings_expandRomSpace++;
						} else if (perGameSettings_expandRomSpace==2 && romSize <= romSizeLimit2) {
							perGameSettings_expandRomSpace++;
						}
						if (perGameSettings_expandRomSpace > 2) perGameSettings_expandRomSpace = -1;
						break;
				}
				perGameSettingsChanged = true;
			}

			if (pressed & KEY_B) {
				if (perGameSettingsChanged) {
					savePerGameSettings(filename);
					perGameSettingsChanged = false;
				}
				break;
			}
		}
		if ((pressed & KEY_X) && !isHomebrew && showCheats) {
			CheatCodelist codelist;
			codelist.selectCheats(filename);
		}
	}
	clearText();
	showdialogbox = false;
	dialogboxHeight = 0;
	keysSetRepeat(10, 2); // Reset key repeat
}

std::string getSavExtension(void) {
	switch (perGameSettings_saveNo) {
		case 0:
		default:
			return ".sav";
			break;
		case 1:
			return ".sav1";
			break;
		case 2:
			return ".sav2";
			break;
		case 3:
			return ".sav3";
			break;
		case 4:
			return ".sav4";
			break;
		case 5:
			return ".sav5";
			break;
		case 6:
			return ".sav6";
			break;
		case 7:
			return ".sav7";
			break;
		case 8:
			return ".sav8";
			break;
		case 9:
			return ".sav9";
			break;
	}
}

std::string getImgExtension(int number) {
	switch (number) {
		case 0:
		default:
			return ".img";
			break;
		case 1:
			return ".img1";
			break;
		case 2:
			return ".img2";
			break;
		case 3:
			return ".img3";
			break;
		case 4:
			return ".img4";
			break;
		case 5:
			return ".img5";
			break;
		case 6:
			return ".img6";
			break;
		case 7:
			return ".img7";
			break;
		case 8:
			return ".img8";
			break;
		case 9:
			return ".img9";
			break;
	}
}
