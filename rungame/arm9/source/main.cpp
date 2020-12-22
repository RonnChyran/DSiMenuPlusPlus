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
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>

#include "nds_loader_arm9.h"

#include "inifile.h"

#include "perGameSettings.h"
#include "fileCopy.h"
#include "flashcard.h"

#include "saveMap.h"

const char* settingsinipath = "sd:/_nds/TWiLightMenu/settings.ini";
const char* bootstrapinipath = "sd:/_nds/nds-bootstrap.ini";

std::string dsiWareSrlPath;
std::string dsiWarePubPath;
std::string dsiWarePrvPath;
std::string homebrewArg[2];
std::string ndsPath;
std::string romfolder;
std::string filename;

const char *charUnlaunchBg;
std::string unlaunchBg = "default.gif";
bool removeLauncherPatches = true;

static const char *unlaunchAutoLoadID = "AutoLoadInfo";

typedef struct {
	char gameTitle[12];			//!< 12 characters for the game title.
	char gameCode[4];			//!< 4 characters for the game code.
} sNDSHeadertitlecodeonly;

static int consoleModel = 0;
/*	0 = Nintendo DSi (Retail)
	1 = Nintendo DSi (Dev/Panda)
	2 = Nintendo 3DS
	3 = New Nintendo 3DS	*/

static std::string romPath[2];

/**
 * Remove trailing slashes from a pathname, if present.
 * @param path Pathname to modify.
 */
void RemoveTrailingSlashes(std::string& path)
{
	while (!path.empty() && path[path.size()-1] == '/') {
		path.resize(path.size()-1);
	}
}

static const std::string slashchar = "/";
static const std::string woodfat = "fat0:/";
static const std::string dstwofat = "fat1:/";

static bool wifiLed = false;
static bool slot1Launched = false;
static int launchType[2] = {0};	// 0 = No launch, 1 = SD/Flash card, 2 = SD/Flash card (Direct boot), 3 = DSiWare, 4 = NES, 5 = (S)GB(C), 6 = SMS/GG
static bool useBootstrap = true;
static bool bootstrapFile = false;
static bool homebrewBootstrap = false;
static bool homebrewHasWide = false;
static bool fcSaveOnSd = false;
static bool wideScreen = false;

static bool soundfreq = false;	// false == 32.73 kHz, true == 47.61 kHz

static int gameLanguage = -1;
static bool boostCpu = false;	// false == NTR, true == TWL
static bool boostVram = false;
static bool bstrap_dsiMode = false;

TWL_CODE void LoadSettings(void) {
	// GUI
	CIniFile settingsini( settingsinipath );

	wifiLed = settingsini.GetInt("SRLOADER", "WIFI_LED", 0);
	soundfreq = settingsini.GetInt("SRLOADER", "SOUND_FREQ", 0);
	consoleModel = settingsini.GetInt("SRLOADER", "CONSOLE_MODEL", 0);
	previousUsedDevice = settingsini.GetInt("SRLOADER", "PREVIOUS_USED_DEVICE", previousUsedDevice);
	fcSaveOnSd = settingsini.GetInt("SRLOADER", "FC_SAVE_ON_SD", fcSaveOnSd);
	useBootstrap = settingsini.GetInt("SRLOADER", "USE_BOOTSTRAP", useBootstrap);
	bootstrapFile = settingsini.GetInt("SRLOADER", "BOOTSTRAP_FILE", 0);

    unlaunchBg = settingsini.GetString("SRLOADER", "UNLAUNCH_BG", unlaunchBg);
	charUnlaunchBg = unlaunchBg.c_str();
	removeLauncherPatches = settingsini.GetInt("SRLOADER", "UNLAUNCH_PATCH_REMOVE", removeLauncherPatches);

	// Default nds-bootstrap settings
	gameLanguage = settingsini.GetInt("NDS-BOOTSTRAP", "LANGUAGE", -1);
	boostCpu = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
	boostVram = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);
	bstrap_dsiMode = settingsini.GetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);

	dsiWareSrlPath = settingsini.GetString("SRLOADER", "DSIWARE_SRL", "");
	dsiWarePubPath = settingsini.GetString("SRLOADER", "DSIWARE_PUB", "");
	dsiWarePrvPath = settingsini.GetString("SRLOADER", "DSIWARE_PRV", "");
	slot1Launched = settingsini.GetInt("SRLOADER", "SLOT1_LAUNCHED", slot1Launched);
	launchType[0] = settingsini.GetInt("SRLOADER", "LAUNCH_TYPE", launchType[0]);
	launchType[1] = settingsini.GetInt("SRLOADER", "SECONDARY_LAUNCH_TYPE", launchType[1]);
	romPath[0] = settingsini.GetString("SRLOADER", "ROM_PATH", romPath[0]);
	romPath[1] = settingsini.GetString("SRLOADER", "SECONDARY_ROM_PATH", romPath[1]);
	homebrewArg[0] = settingsini.GetString("SRLOADER", "HOMEBREW_ARG", "");
	homebrewArg[1] = settingsini.GetString("SRLOADER", "SECONDARY_HOMEBREW_ARG", "");
	homebrewBootstrap = settingsini.GetInt("SRLOADER", "HOMEBREW_BOOTSTRAP", 0);
	homebrewHasWide = settingsini.GetInt("SRLOADER", "HOMEBREW_HAS_WIDE", 0);

	wideScreen = settingsini.GetInt("SRLOADER", "WIDESCREEN", wideScreen);

	// nds-bootstrap
	CIniFile bootstrapini( bootstrapinipath );

	ndsPath = bootstrapini.GetString( "NDS-BOOTSTRAP", "NDS_PATH", "");
}

using namespace std;

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

char filePath[PATH_MAX];

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

bool extention(const std::string& filename, const char* ext) {
	if(strcasecmp(filename.c_str() + filename.size() - strlen(ext), ext)) {
		return false;
	} else {
		return true;
	}
}

std::vector<char*> argarray;

TWL_CODE int lastRunROM() {
	LoadSettings();

	if (consoleModel < 2) {
		*(u8*)(0x023FFD00) = (wifiLed ? 0x13 : 0);		// WiFi On/Off
	}

	bool twlBgCxiFound = false;
	if (consoleModel >= 2) {
		twlBgCxiFound = (access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) == 0);
	}
	/*if (consoleModel >= 2 && wideScreen
	&& access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) == 0 && access("sd:/luma/sysmodules/TwlBg_bak.cxi", F_OK) == 0) {
		// Revert back to 4:3 for when returning to TWLMenu++
		if (remove("sd:/luma/sysmodules/TwlBg.cxi") == 0) {
			rename("sd:/luma/sysmodules/TwlBg_bak.cxi", "sd:/luma/sysmodules/TwlBg.cxi");
		} else {
			consoleDemoInit();
			printf("Failed to delete TwlBg.cxi");
			for (int i = 0; i < 60*2; i++) swiWaitForVBlank();
		}
	}*/

	argarray.push_back(strdup("null"));

	if (launchType[previousUsedDevice] > 3) {
		argarray.push_back(strdup(homebrewArg[previousUsedDevice].c_str()));
	}

	if (access(romPath[previousUsedDevice].c_str(), F_OK) != 0 || launchType[previousUsedDevice] == 0) {
		return runNdsFile ("/_nds/TWiLightMenu/main.srldr", 0, NULL, true, false, false, true, true);	// Skip to running TWiLight Menu++
	}

	if (slot1Launched) {
		return runNdsFile ("/_nds/TWiLightMenu/slot1launch.srldr", 0, NULL, true, false, false, true, true);
	}

	switch (launchType[previousUsedDevice]) {
		case 1:
			if ((useBootstrap && !homebrewBootstrap) || !previousUsedDevice)
			{
				std::string savepath;

				romfolder = romPath[previousUsedDevice];
				while (!romfolder.empty() && romfolder[romfolder.size()-1] != '/') {
					romfolder.resize(romfolder.size()-1);
				}
				chdir(romfolder.c_str());

				filename = romPath[previousUsedDevice];
				const size_t last_slash_idx = filename.find_last_of("/");
				if (std::string::npos != last_slash_idx)
				{
					filename.erase(0, last_slash_idx + 1);
				}

				loadPerGameSettings(filename);
				bool useNightly = (perGameSettings_bootstrapFile == -1 ? bootstrapFile : perGameSettings_bootstrapFile);

				if (!homebrewBootstrap) {
					const char *typeToReplace = ".nds";
					if (extention(filename, ".dsi")) {
						typeToReplace = ".dsi";
					} else if (extention(filename, ".ids")) {
						typeToReplace = ".ids";
					} else if (extention(filename, ".srl")) {
						typeToReplace = ".srl";
					} else if (extention(filename, ".app")) {
						typeToReplace = ".app";
					}

					char game_TID[5];

					FILE *f_nds_file = fopen(filename.c_str(), "rb");

					fseek(f_nds_file, offsetof(sNDSHeadertitlecodeonly, gameCode), SEEK_SET);
					fread(game_TID, 1, 4, f_nds_file);
					game_TID[4] = 0;
					game_TID[3] = 0;

					fclose(f_nds_file);

					std::string savename = ReplaceAll(filename, typeToReplace, getSavExtension());
					std::string romFolderNoSlash = romfolder;
					RemoveTrailingSlashes(romFolderNoSlash);
					mkdir ("saves", 0777);
					savepath = romFolderNoSlash+"/saves/"+savename;
					if (previousUsedDevice && fcSaveOnSd) {
						savepath = ReplaceAll(savepath, "fat:/", "sd:/");
					}

					if ((getFileSize(savepath.c_str()) == 0) && (strcmp(game_TID, "###") != 0) && (strcmp(game_TID, "NTR") != 0)) {
						int savesize = 524288;	// 512KB (default size for most games)

						for (auto i : saveMap) {
							if (i.second.find(game_TID) != i.second.cend()) {
								savesize = i.first;
								break;
							}
						}

						if (savesize > 0) {
							consoleDemoInit();
							printf("Creating save file...\n");

							FILE *pFile = fopen(savepath.c_str(), "wb");
							if (pFile) {
								fseek(pFile, savesize - 1, SEEK_SET);
								fputc('\0', pFile);
								fclose(pFile);
							}
							printf("Save file created!\n");
						
							for (int i = 0; i < 30; i++) {
								swiWaitForVBlank();
							}
						}
					}
				}

				char ndsToBoot[256];
				sprintf(ndsToBoot, "sd:/_nds/nds-bootstrap-%s%s.nds", homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
				if(access(ndsToBoot, F_OK) != 0) {
					sprintf(ndsToBoot, "fat:/_nds/nds-bootstrap-%s%s.nds", homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
				}

				argarray.at(0) = (char *)ndsToBoot;
				if (previousUsedDevice || !homebrewBootstrap) {
					CIniFile bootstrapini(bootstrapinipath);
					bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", romPath[previousUsedDevice]);
					bootstrapini.SetString("NDS-BOOTSTRAP", "SAV_PATH", savepath);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", perGameSettings_language == -2 ? gameLanguage : perGameSettings_language);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", perGameSettings_dsiMode == -1 ? bstrap_dsiMode : perGameSettings_dsiMode);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", perGameSettings_boostCpu == -1 ? boostCpu : perGameSettings_boostCpu);
					bootstrapini.SetInt( "NDS-BOOTSTRAP", "BOOST_VRAM", perGameSettings_boostVram == -1 ? boostVram : perGameSettings_boostVram);
					bootstrapini.SaveIniFile(bootstrapinipath);
				}

				return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], (homebrewBootstrap ? false : true), true, false, true, true);
			} else {
				std::string filename = romPath[1];
				const size_t last_slash_idx = filename.find_last_of("/");
				if (std::string::npos != last_slash_idx)
				{
					filename.erase(0, last_slash_idx + 1);
				}

				loadPerGameSettings(filename);
				bool runNds_boostCpu = perGameSettings_boostCpu == -1 ? boostCpu : perGameSettings_boostCpu;
				bool runNds_boostVram = perGameSettings_boostVram == -1 ? boostVram : perGameSettings_boostVram;

				std::string path;
				if ((memcmp(io_dldi_data->friendlyName, "R4(DS) - Revolution for DS", 26) == 0)
				 || (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0)) {
					CIniFile fcrompathini("fat:/_wfwd/lastsave.ini");
					path = ReplaceAll(romPath[1], "fat:/", woodfat);
					fcrompathini.SetString("Save Info", "lastLoaded", path);
					fcrompathini.SaveIniFile("fat:/_wfwd/lastsave.ini");
					return runNdsFile("fat:/Wfwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
				} else if (memcmp(io_dldi_data->friendlyName, "Acekard AK2", 0xB) == 0) {
					CIniFile fcrompathini("fat:/_afwd/lastsave.ini");
					path = ReplaceAll(romPath[1], "fat:/", woodfat);
					fcrompathini.SetString("Save Info", "lastLoaded", path);
					fcrompathini.SaveIniFile("fat:/_afwd/lastsave.ini");
					return runNdsFile("fat:/Afwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
				} else if (memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0) {
					CIniFile fcrompathini("fat:/_dstwo/autoboot.ini");
					path = ReplaceAll(romPath[1], "fat:/", dstwofat);
					fcrompathini.SetString("Dir Info", "fullName", path);
					fcrompathini.SaveIniFile("fat:/_dstwo/autoboot.ini");
					return runNdsFile("fat:/_dstwo/autoboot.nds", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
				} else if ((memcmp(io_dldi_data->friendlyName, "TTCARD", 6) == 0)
						 || (memcmp(io_dldi_data->friendlyName, "DSTT", 4) == 0)
						 || (memcmp(io_dldi_data->friendlyName, "DEMON", 5) == 0)) {
					CIniFile fcrompathini("fat:/TTMenu/YSMenu.ini");
					path = ReplaceAll(romPath[1], "fat:/", slashchar);
					fcrompathini.SetString("YSMENU", "AUTO_BOOT", path);
					fcrompathini.SaveIniFile("fat:/TTMenu/YSMenu.ini");
					return runNdsFile("fat:/YSMenu.nds", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
				}
			}
		case 2: {
			romfolder = romPath[previousUsedDevice];
			while (!romfolder.empty() && romfolder[romfolder.size()-1] != '/') {
				romfolder.resize(romfolder.size()-1);
			}
			chdir(romfolder.c_str());

			filename = romPath[previousUsedDevice];
			const size_t last_slash_idx = filename.find_last_of("/");
			if (std::string::npos != last_slash_idx)
			{
				filename.erase(0, last_slash_idx + 1);
			}

			argarray.at(0) = (char*)romPath[previousUsedDevice].c_str();

			loadPerGameSettings(filename);

			bool useWidescreen = (perGameSettings_wideScreen == -1 ? wideScreen : perGameSettings_wideScreen);

			if (consoleModel >= 2 && twlBgCxiFound && useWidescreen && homebrewHasWide) {
				argarray.push_back((char*)"wide");
			}

			bool runNds_boostCpu = perGameSettings_boostCpu == -1 ? boostCpu : perGameSettings_boostCpu;
			bool runNds_boostVram = perGameSettings_boostVram == -1 ? boostVram : perGameSettings_boostVram;

			return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, (!perGameSettings_dsiMode ? true : false), runNds_boostCpu, runNds_boostVram);
		} case 3: {
			char unlaunchDevicePath[256];
			if (previousUsedDevice) {
				snprintf(unlaunchDevicePath, (int)sizeof(unlaunchDevicePath), "sdmc:/_nds/TWiLightMenu/tempDSiWare.dsi");
			} else {
				snprintf(unlaunchDevicePath, (int)sizeof(unlaunchDevicePath), "__%s", dsiWareSrlPath.c_str());
				unlaunchDevicePath[0] = 's';
				unlaunchDevicePath[1] = 'd';
				unlaunchDevicePath[2] = 'm';
				unlaunchDevicePath[3] = 'c';
			}

			memcpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
			*(u16*)(0x0200080C) = 0x3F0;			// Unlaunch Length for CRC16 (fixed, must be 3F0h)
			*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
			*(u32*)(0x02000810) |= BIT(0);			// Load the title at 2000838h
			*(u32*)(0x02000810) |= BIT(1);			// Use colors 2000814h
			*(u16*)(0x02000814) = 0x7FFF;			// Unlaunch Upper screen BG color (0..7FFFh)
			*(u16*)(0x02000816) = 0x7FFF;			// Unlaunch Lower screen BG color (0..7FFFh)
			memset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);	// Unlaunch Reserved (zero)
			int i2 = 0;
			for (int i = 0; i < (int)sizeof(unlaunchDevicePath); i++) {
				*(u8*)(0x02000838+i2) = unlaunchDevicePath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
				i2 += 2;
			}
			while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
				*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
			}

			fifoSendValue32(FIFO_USER_08, 1);	// Reboot
			for (int i = 0; i < 15; i++) swiWaitForVBlank();
			break;
		} case 4:
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/nestwl.nds";
			return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);	// Pass ROM to nesDS as argument
		case 5:
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/gameyob.nds";
			return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);	// Pass ROM to GameYob as argument
		case 6:
			mkdir("sd:/data", 0777);
			mkdir("sd:/data/s8ds", 0777);
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/S8DS.nds";
			return runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to S8DS as argument
		case 7:
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
			return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);	// Pass video to Rocket Video Player as argument
		case 8:
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/apps/MPEG4Player.nds";
			return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);	// Pass video to MPEG4Player as argument
		case 9:
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/StellaDS.nds";
			return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);	// Pass ROM to StellaDS as argument
		case 10:
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds";
			return runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);	// Pass ROM to PicoDrive TWL as argument
	}
	
	return -1;
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// overwrite reboot stub identifier
	extern char *fake_heap_end;
	*fake_heap_end = 0;

	defaultExceptionHandler();

	if (!fatInitDefault()) {
		consoleDemoInit();
		printf("fatInitDefault failed!");
		stop();
	}

	flashcardInit();

	if (!flashcardFound()) {
		disableSlot1();
	}

	fifoWaitValue32(FIFO_USER_06);

	int err = lastRunROM();
	consoleDemoInit();
	iprintf ("Start failed. Error %i", err);
	stop();

	return 0;
}
