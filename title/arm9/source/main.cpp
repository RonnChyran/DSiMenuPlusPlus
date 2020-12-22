#include <nds.h>
#include <nds/arm9/dldi.h>
#include "io_m3_common.h"
#include "io_g6_common.h"
#include "io_sc_common.h"
#include "exptools.h"

#include "bootsplash.h"
#include "bootstrapsettings.h"
#include "common/bootstrappaths.h"
#include "common/cardlaunch.h"
#include "common/dsimenusettings.h"
#include "common/fileCopy.h"
#include "common/flashcard.h"
#include "common/inifile.h"
#include "common/nds_loader_arm9.h"
#include "common/nitrofs.h"
#include "common/pergamesettings.h"
#include "common/systemdetails.h"
#include "common/tonccpy.h"
#include "tool/stringtool.h"
#include "consolemodelselect.h"
#include "graphics/graphics.h"
#include "nandio.h"
#include "twlmenuppvideo.h"

#include "autoboot.h"

#include "saveMap.h"

bool useTwlCfg = false;

bool renderScreens = false;
bool fadeType = false; // false = out, true = in
bool fadeColor = true; // false = black, true = white
extern bool controlTopBright;
extern bool controlBottomBright;

//bool soundfreqsettingChanged = false;
bool hiyaAutobootFound = false;
//static int flashcard;
/* Flashcard value
	0: DSTT/R4i Gold/R4i-SDHC/R4 SDHC Dual-Core/R4 SDHC Upgrade/SC DSONE
	1: R4DS (Original Non-SDHC version)/ M3 Simply
	2: R4iDSN/R4i Gold RTS/R4 Ultra
	3: Acekard 2(i)/Galaxy Eagle/M3DS Real
	4: Acekard RPG
	5: Ace 3DS+/Gateway Blue Card/R4iTT
	6: SuperCard DSTWO
*/

const char *hiyacfwinipath = "sd:/hiya/settings.ini";
const char *settingsinipath = DSIMENUPP_INI;

const char *unlaunchAutoLoadID = "AutoLoadInfo";
char hiyaNdsPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};
char unlaunchDevicePath[256];

typedef struct {
	char gameTitle[12];			//!< 12 characters for the game title.
	char gameCode[4];			//!< 4 characters for the game code.
} sNDSHeadertitlecodeonly;

char soundBank[0x7D000] = {0};
bool soundBankInited = false;

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

typedef TWLSettings::TLaunchType Launch;

int screenmode = 0;
int subscreenmode = 0;

touchPosition touch;

using namespace std;

//---------------------------------------------------------------------------------
void stop(void)
{
	//---------------------------------------------------------------------------------
	while (1)
	{
		swiWaitForVBlank();
	}
}

char filePath[PATH_MAX];

/*//---------------------------------------------------------------------------------
void doPause(void)
{
	//---------------------------------------------------------------------------------
	printf("Press start...\n");
	//printSmall(false, x, y, "Press start...");
	while (1)
	{
		scanKeys();
		if (keysDown() & KEY_START)
			break;
	}
	scanKeys();
}*/

void rebootDSiMenuPP()
{
	fifoSendValue32(FIFO_USER_01, 1); // Fade out sound
	for (int i = 0; i < 25; i++)
		swiWaitForVBlank();
	memcpy((u32 *)0x02000300, autoboot_bin, 0x020);
	fifoSendValue32(FIFO_USER_08, 1); // Reboot DSiMenu++ to avoid potential crashing
	for (int i = 0; i < 15; i++)
		swiWaitForVBlank();
}

void loadMainMenu()
{
	fadeColor = true;
	controlTopBright = true;
	controlBottomBright = true;
	fadeType = false;
	fifoSendValue32(FIFO_USER_01, 1); // Fade out sound
	for (int i = 0; i < 25; i++)
		swiWaitForVBlank();
	fifoSendValue32(FIFO_USER_01, 0); // Cancel sound fade out

	if (!isDSiMode()) {
		chdir("fat:/");
	}
	runNdsFile("/_nds/TWiLightMenu/mainmenu.srldr", 0, NULL, true, false, false, true, true);
}

void loadROMselect(int number)
{
	fadeColor = true;
	controlTopBright = true;
	controlBottomBright = true;
	fadeType = false;
	fifoSendValue32(FIFO_USER_01, 1); // Fade out sound
	for (int i = 0; i < 25; i++)
		swiWaitForVBlank();
	fifoSendValue32(FIFO_USER_01, 0); // Cancel sound fade out

	if (!isDSiMode()) {
		chdir("fat:/");
	}
	switch (number) {
		/*case 3:
			runNdsFile("/_nds/TWiLightMenu/akmenu.srldr", 0, NULL, true, false, false, true, true);
			break;*/
		case 2:
		case 6:
			runNdsFile("/_nds/TWiLightMenu/r4menu.srldr", 0, NULL, true, false, false, true, true);
			break;
		default:
			runNdsFile("/_nds/TWiLightMenu/dsimenu.srldr", 0, NULL, true, false, false, true, true);
			break;
	}
	stop();
}

bool extention(const std::string& filename, const char* ext) {
	if(strcasecmp(filename.c_str() + filename.size() - strlen(ext), ext)) {
		return false;
	} else {
		return true;
	}
}

void unlaunchRomBoot(const char* rom) {
	if (strncmp(rom, "cart:", 5) == 0) {
		sprintf(unlaunchDevicePath, "cart:");
	} else {
		sprintf(unlaunchDevicePath, "__%s", rom);
		unlaunchDevicePath[0] = 's';
		unlaunchDevicePath[1] = 'd';
		unlaunchDevicePath[2] = 'm';
		unlaunchDevicePath[3] = 'c';
	}

	tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) = 0;			// Unlaunch Flags
	*(u32*)(0x02000810) |= BIT(0);		// Load the title at 2000838h
	*(u32*)(0x02000810) |= BIT(1);		// Use colors 2000814h
	*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < (int)sizeof(unlaunchDevicePath); i++) {
		*(u8*)(0x02000838+i2) = unlaunchDevicePath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
		*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
	}

	DC_FlushAll();						// Make reboot not fail
	fifoSendValue32(FIFO_USER_02, 1);	// Reboot into DSiWare title, booted via Unlaunch
	stop();
}

void dsCardLaunch() {
	*(u32 *)(0x02000300) = 0x434E4C54; // Set "CNLT" warmboot flag
	*(u16 *)(0x02000304) = 0x1801;
	*(u32 *)(0x02000308) = 0x43415254; // "CART"
	*(u32 *)(0x0200030C) = 0x00000000;
	*(u32 *)(0x02000310) = 0x43415254; // "CART"
	*(u32 *)(0x02000314) = 0x00000000;
	*(u32 *)(0x02000318) = 0x00000013;
	*(u32 *)(0x0200031C) = 0x00000000;
	while (*(u16 *)(0x02000306) == 0) { // Keep running, so that CRC16 isn't 0
		*(u16 *)(0x02000306) = swiCRC16(0xFFFF, (void *)0x02000308, 0x18);
	}

	unlaunchSetHiyaBoot();

	DC_FlushAll();						// Make reboot not fail
	fifoSendValue32(FIFO_USER_02, 1); // Reboot into DSiWare title, booted via Launcher
	stop();
}

void s2RamAccess(bool open) {
	if (io_dldi_data->ioInterface.features & FEATURE_SLOT_NDS) return;

	if (open) {
		if (*(u16*)(0x020000C0) == 0x334D) {
			_M3_changeMode(M3_MODE_RAM);
		} else if (*(u16*)(0x020000C0) == 0x3647) {
			_G6_SelectOperation(G6_MODE_RAM);
		} else if (*(u16*)(0x020000C0) == 0x4353) {
			_SC_changeMode(SC_MODE_RAM);
		}
	} else {
		if (*(u16*)(0x020000C0) == 0x334D) {
			_M3_changeMode(M3_MODE_MEDIA);
		} else if (*(u16*)(0x020000C0) == 0x3647) {
			_G6_SelectOperation(G6_MODE_MEDIA);
		} else if (*(u16*)(0x020000C0) == 0x4353) {
			_SC_changeMode(SC_MODE_MEDIA);
		}
	}
}

void gbaSramAccess(bool open) {
	if (open) {
		if (*(u16*)(0x020000C0) == 0x334D) {
			_M3_changeMode(M3_MODE_RAM);
		} else if (*(u16*)(0x020000C0) == 0x3647) {
			_G6_SelectOperation(G6_MODE_RAM);
		} else if (*(u16*)(0x020000C0) == 0x4353) {
			_SC_changeMode(SC_MODE_RAM_RO);
		}
	} else {
		if (*(u16*)(0x020000C0) == 0x334D) {
			_M3_changeMode((io_dldi_data->ioInterface.features & FEATURE_SLOT_GBA) ? M3_MODE_MEDIA : M3_MODE_RAM);
		} else if (*(u16*)(0x020000C0) == 0x3647) {
			_G6_SelectOperation((io_dldi_data->ioInterface.features & FEATURE_SLOT_GBA) ? G6_MODE_MEDIA : G6_MODE_RAM);
		} else if (*(u16*)(0x020000C0) == 0x4353) {
			_SC_changeMode((io_dldi_data->ioInterface.features & FEATURE_SLOT_GBA) ? SC_MODE_MEDIA : SC_MODE_RAM);
		}
	}
}

void lastRunROM()
{
	/*fifoSendValue32(FIFO_USER_01, 1); // Fade out sound
	for (int i = 0; i < 25; i++)
		swiWaitForVBlank();
	fifoSendValue32(FIFO_USER_01, 0); // Cancel sound fade out*/

	std::string romfolder = ms().romPath[ms().previousUsedDevice];
	while (!romfolder.empty() && romfolder[romfolder.size()-1] != '/') {
		romfolder.resize(romfolder.size()-1);
	}
	chdir(romfolder.c_str());

	std::string filename = ms().romPath[ms().previousUsedDevice];
	const size_t last_slash_idx = filename.find_last_of("/");
	if (std::string::npos != last_slash_idx)
	{
		filename.erase(0, last_slash_idx + 1);
	}

	vector<char *> argarray;
	if (ms().launchType[ms().previousUsedDevice] > Launch::EDSiWareLaunch)
	{
		argarray.push_back(strdup("null"));
		if (ms().launchType[ms().previousUsedDevice] == Launch::EGBANativeLaunch) {
			argarray.push_back(strdup(ms().romPath[ms().previousUsedDevice].c_str()));
		} else {
			argarray.push_back(strdup(ms().homebrewArg[ms().previousUsedDevice].c_str()));
		}
	}

	int err = 0;
	if (ms().slot1Launched && !flashcardFound())
	{
		if (ms().slot1LaunchMethod==0 || sys().arm7SCFGLocked()) {
			dsCardLaunch();
		} else if (ms().slot1LaunchMethod==2) {
			unlaunchRomBoot("cart:");
		} else {
			err = runNdsFile("/_nds/TWiLightMenu/slot1launch.srldr", 0, NULL, true, true, false, true, true);
		}
	}
	if (ms().launchType[ms().previousUsedDevice] == Launch::ESDFlashcardLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++
		if (ms().useBootstrap || !ms().previousUsedDevice)
		{
			std::string savepath;

			loadPerGameSettings(filename);

			bool useWidescreen = (perGameSettings_wideScreen == -1 ? ms().wideScreen : perGameSettings_wideScreen);

			if (ms().homebrewBootstrap) {
				bool useNightly = (perGameSettings_bootstrapFile == -1 ? ms().bootstrapFile : perGameSettings_bootstrapFile);
				argarray.push_back((char*)(useNightly ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds"));
			} else {
				if (ms().consoleModel >= 2 && !useWidescreen) {
					remove("/_nds/nds-bootstrap/wideCheatData.bin");
				}

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

				std::string savename = replaceAll(filename, typeToReplace, getSavExtension());
				std::string romFolderNoSlash = romfolder;
				RemoveTrailingSlashes(romFolderNoSlash);
				mkdir ("saves", 0777);
				savepath = romFolderNoSlash+"/saves/"+savename;
				if (ms().previousUsedDevice && ms().fcSaveOnSd) {
					savepath = replaceAll(savepath, "fat:/", "sd:/");
				}

				if ((strcmp(game_TID, "###") != 0) && (strcmp(game_TID, "NTR") != 0)) {
					int orgsavesize = getFileSize(savepath.c_str());
					int savesize = 524288;	// 512KB (default size for most games)

					for (auto i : saveMap) {
						if (i.second.find(game_TID) != i.second.cend()) {
							savesize = i.first;
							break;
						}
					}

					bool saveSizeFixNeeded = false;

					// TODO: If the list gets large enough, switch to bsearch().
					for (unsigned int i = 0; i < sizeof(saveSizeFixList) / sizeof(saveSizeFixList[0]); i++) {
						if (memcmp(game_TID, saveSizeFixList[i], 3) == 0) {
							// Found a match.
							saveSizeFixNeeded = true;
							break;
						}
					}

					if ((orgsavesize == 0 && savesize > 0) || (orgsavesize < savesize && saveSizeFixNeeded)) {
						consoleDemoInit();
						iprintf((orgsavesize == 0) ? "Creating save file...\n" : "Expanding save file...\n");
						iprintf ("\n");
						if (ms().consoleModel >= 2) {
							iprintf ("If this takes a while,\n");
							iprintf ("press HOME, and press B.\n");
						} else {
							iprintf ("If this takes a while, close\n");
							iprintf ("and open the console's lid.\n");
						}
						iprintf ("\n");
						fadeType = true;

						if (orgsavesize > 0) {
							fsizeincrease(savepath.c_str(), sdFound() ? "sd:/_nds/TWiLightMenu/temp.sav" : "fat:/_nds/TWiLightMenu/temp.sav", savesize);
						} else {
							FILE *pFile = fopen(savepath.c_str(), "wb");
							if (pFile) {
								fseek(pFile, savesize - 1, SEEK_SET);
								fputc('\0', pFile);
								fclose(pFile);
							}
						}
						iprintf((orgsavesize == 0) ? "Save file created!\n" : "Save file expanded!\n");

						for (int i = 0; i < 30; i++) {
							swiWaitForVBlank();
						}
						fadeType = false;
						for (int i = 0; i < 25; i++) {
							swiWaitForVBlank();
						}
					}
				}

				bool useNightly = (perGameSettings_bootstrapFile == -1 ? ms().bootstrapFile : perGameSettings_bootstrapFile);

				if (sdFound() && !ms().previousUsedDevice) {
					argarray.push_back((char*)(useNightly ? "sd:/_nds/nds-bootstrap-nightly.nds" : "sd:/_nds/nds-bootstrap-release.nds"));
				} else {
					if (isDSiMode()) {
						argarray.push_back((char*)(useNightly ? "/_nds/nds-bootstrap-nightly.nds" : "/_nds/nds-bootstrap-release.nds"));
					} else {
						argarray.push_back((char*)(useNightly ? "/_nds/b4ds-nightly.nds" : "/_nds/b4ds-release.nds"));
					}
				}
			}
			if (ms().previousUsedDevice || !ms().homebrewBootstrap) {
				CIniFile bootstrapini( sdFound() ? BOOTSTRAP_INI_SD : BOOTSTRAP_INI_FC );
				bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", ms().romPath[ms().previousUsedDevice]);
				bootstrapini.SetString("NDS-BOOTSTRAP", "SAV_PATH", savepath);
				bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE",
					(perGameSettings_language == -2 ? ms().gameLanguage : perGameSettings_language));
				bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE",
					(perGameSettings_dsiMode == -1 ? ms().bstrap_dsiMode : perGameSettings_dsiMode));
				bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU",
					(perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu));
				bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM",
					(perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram));
				bootstrapini.SaveIniFile( sdFound() ? BOOTSTRAP_INI_SD : BOOTSTRAP_INI_FC );
			}
			err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], (ms().homebrewBootstrap ? false : true), true, false, true, true);
		}
		else
		{
			bool runNds_boostCpu = false;
			bool runNds_boostVram = false;
			loadPerGameSettings(filename);
			if (REG_SCFG_EXT != 0) {
				runNds_boostCpu = perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu;
				runNds_boostVram = perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram;
			}

			// Move .sav outside of "saves" folder for flashcard kernel usage
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

			std::string savename = replaceAll(filename, typeToReplace, getSavExtension());
			std::string savenameFc = replaceAll(filename, typeToReplace, ".sav");
			std::string romFolderNoSlash = romfolder;
			RemoveTrailingSlashes(romFolderNoSlash);
			mkdir("saves", 0777);
			std::string savepath = romFolderNoSlash + "/saves/" + savename;
			std::string savepathFc = romFolderNoSlash + "/" + savenameFc;
			rename(savepath.c_str(), savepathFc.c_str());

			std::string fcPath;
			if ((memcmp(io_dldi_data->friendlyName, "R4(DS) - Revolution for DS", 26) == 0)
			 || (memcmp(io_dldi_data->friendlyName, "R4TF", 4) == 0)
			 || (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0)) {
				CIniFile fcrompathini("fat:/_wfwd/lastsave.ini");
				fcPath = replaceAll(ms().romPath[ms().previousUsedDevice], "fat:/", woodfat);
				fcrompathini.SetString("Save Info", "lastLoaded", fcPath);
				fcrompathini.SaveIniFile("fat:/_wfwd/lastsave.ini");
				err = runNdsFile("fat:/Wfwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
			} else if (memcmp(io_dldi_data->friendlyName, "Acekard AK2", 0xB) == 0) {
				CIniFile fcrompathini("fat:/_afwd/lastsave.ini");
				fcPath = replaceAll(ms().romPath[ms().previousUsedDevice], "fat:/", woodfat);
				fcrompathini.SetString("Save Info", "lastLoaded", fcPath);
				fcrompathini.SaveIniFile("fat:/_afwd/lastsave.ini");
				err = runNdsFile("fat:/Afwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
			} else if (memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0) {
				CIniFile fcrompathini("fat:/_dstwo/autoboot.ini");
				fcPath = replaceAll(ms().romPath[ms().previousUsedDevice], "fat:/", dstwofat);
				fcrompathini.SetString("Dir Info", "fullName", fcPath);
				fcrompathini.SaveIniFile("fat:/_dstwo/autoboot.ini");
				err = runNdsFile("fat:/_dstwo/autoboot.nds", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
			} else if ((memcmp(io_dldi_data->friendlyName, "TTCARD", 6) == 0)
					 || (memcmp(io_dldi_data->friendlyName, "DSTT", 4) == 0)
					 || (memcmp(io_dldi_data->friendlyName, "DEMON", 5) == 0)) {
				CIniFile fcrompathini("fat:/TTMenu/YSMenu.ini");
				fcPath = replaceAll(ms().romPath[ms().previousUsedDevice], "fat:/", slashchar);
				fcrompathini.SetString("YSMENU", "AUTO_BOOT", fcPath);
				fcrompathini.SaveIniFile("fat:/TTMenu/YSMenu.ini");
				err = runNdsFile("fat:/YSMenu.nds", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
			}
		}
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::ESDFlashcardDirectLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		argarray.push_back((char*)ms().romPath[ms().previousUsedDevice].c_str());

		bool runNds_boostCpu = false;
		bool runNds_boostVram = false;

		loadPerGameSettings(filename);

		bool useWidescreen = (perGameSettings_wideScreen == -1 ? ms().wideScreen : perGameSettings_wideScreen);

		bool twlBgCxiFound = false;
		if (ms().consoleModel >= 2) {
			twlBgCxiFound = (access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) == 0);
		}

		if (ms().consoleModel >= 2 && twlBgCxiFound && useWidescreen && ms().homebrewHasWide) {
			argarray.push_back((char*)"wide");
		}

		runNds_boostCpu = perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu;
		runNds_boostVram = perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram;

		err = runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, (!perGameSettings_dsiMode ? true : false), runNds_boostCpu, runNds_boostVram);
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::EDSiWareLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (ms().previousUsedDevice) {
			snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath), "sdmc:/_nds/TWiLightMenu/tempDSiWare.dsi");
		} else {
			snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath), "__%s", ms().dsiWareSrlPath.c_str());
			unlaunchDevicePath[0] = 's';
			unlaunchDevicePath[1] = 'd';
			unlaunchDevicePath[2] = 'm';
			unlaunchDevicePath[3] = 'c';
		}

		memcpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
		*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
		*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
		*(u32*)(0x02000810) = 0;			// Unlaunch Flags
		*(u32*)(0x02000810) |= BIT(0);		// Load the title at 2000838h
		*(u32*)(0x02000810) |= BIT(1);		// Use colors 2000814h
		*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
		*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
		memset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
		int i2 = 0;
		for (int i = 0; i < (int)sizeof(unlaunchDevicePath); i++) {
			*(u8*)(0x02000838+i2) = unlaunchDevicePath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
			i2 += 2;
		}
		while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
			*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
		}

		fifoSendValue32(FIFO_USER_08, 1); // Reboot
		for (int i = 0; i < 15; i++) swiWaitForVBlank();
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::ENESDSLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/emulators/nesds.nds";
		}
		else
		{
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/nestwl.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to nesDS as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::EGameYobLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/emulators/gameyob.nds";
		}
		else
		{
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/gameyob.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to GameYob as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::ES8DSLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			mkdir("fat:/data", 0777);
			mkdir("fat:/data/s8ds", 0777);
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/emulators/S8DS.nds";
		}
		else
		{
			mkdir("sd:/data", 0777);
			mkdir("sd:/data/s8ds", 0777);
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/S8DS.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to S8DS as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::ERVideoLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
		}
		else
		{
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass video to Rocket Video Player as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::EMPEG4Launch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/apps/MPEG4Player.nds";
		}
		else
		{
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/apps/MPEG4Player.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass video to MPEG4Player as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::EStellaDSLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/emulators/StellaDS.nds";
		}
		else
		{
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/StellaDS.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to StellaDS as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::EPicoDriveTWLLaunch)
	{
		if (ms().showMd >= 2 || access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds";
		}
		else
		{
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to PicoDrive TWL as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::EGBANativeLaunch)
	{
		if (!sys().isRegularDS() || *(u16*)(0x020000C0) == 0 || (ms().showGba != 1) || access(ms().romPath[true].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		std::string savepath = replaceAll(ms().romPath[true], ".gba", ".sav");
		u32 romSize = getFileSize(ms().romPath[true].c_str());
		if (romSize > 0x2000000) romSize = 0x2000000;
		u32 savesize = getFileSize(savepath.c_str());
		if (savesize > 0x20000) savesize = 0x20000;

		consoleDemoInit();
		iprintf("Now Loading...\n");
		iprintf("\n");
		iprintf("If this takes a while, close\n");
		iprintf("and open the console's lid.\n");
		iprintf("\n");
		fadeType = true;

		u32 ptr = 0x08000000;
		extern char copyBuf[0x8000];
		bool nor = false;
		if (*(u16*)(0x020000C0) == 0x5A45) {
			cExpansion::SetRompage(0);
			expansion().SetRampage(cExpansion::ENorPage);
			cExpansion::OpenNorWrite();
			cExpansion::SetSerialMode();
			nor = true;
		} else if (*(u16*)(0x020000C0) == 0x4353 && romSize > 0x1FFFFFE) {
			romSize = 0x1FFFFFE;
		}

		if (!nor) {
			FILE* gbaFile = fopen(ms().romPath[true].c_str(), "rb");
			for (u32 len = romSize; len > 0; len -= 0x8000) {
				if (fread(&copyBuf, 1, (len>0x8000 ? 0x8000 : len), gbaFile) > 0) {
					s2RamAccess(true);
					tonccpy((u16*)ptr, &copyBuf, (len>0x8000 ? 0x8000 : len));
					s2RamAccess(false);
					ptr += 0x8000;
				} else {
					break;
				}
			}
			fclose(gbaFile);
		}

		if (*(u16*)(0x020000C0) == 0x5A45) {
			expansion().SetRampage(0);	// Switch to GBA SRAM for EZ Flash
		}

		ptr = 0x0A000000;

		if (savesize > 0) {
			FILE* savFile = fopen(savepath.c_str(), "rb");
			for (u32 len = (savesize > 0x10000 ? 0x10000 : savesize); len > 0; len -= 0x8000) {
				if (fread(&copyBuf, 1, (len>0x8000 ? 0x8000 : len), savFile) > 0) {
					gbaSramAccess(true);	// Switch to GBA SRAM
					cExpansion::WriteSram(ptr,(u8*)copyBuf,0x8000);
					gbaSramAccess(false);	// Switch out of GBA SRAM
					ptr += 0x8000;
				} else {
					break;
				}
			}
			fclose(savFile);
		}

		fadeType = false;
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}

		argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/gbapatcher.srldr";
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to StellaDS as argument
	}
	else if (ms().launchType[ms().previousUsedDevice] == Launch::EA7800DSLaunch)
	{
		if (access(ms().romPath[ms().previousUsedDevice].c_str(), F_OK) != 0) return;	// Skip to running TWiLight Menu++

		if (sys().flashcardUsed())
		{
			argarray.at(0) = (char*)"fat:/_nds/TWiLightMenu/emulators/A7800DS.nds";
		}
		else
		{
			argarray.at(0) = (char*)"sd:/_nds/TWiLightMenu/emulators/A7800DS.nds";
		}
		err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true); // Pass ROM to StellaDS as argument
	}
	if (err > 0) {
		consoleDemoInit();
		iprintf("Start failed. Error %i\n", err);
		fadeType = true;
		for (int i = 0; i < 60*3; i++) {
			swiWaitForVBlank();
		}
		fadeType = false;
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
	}
}

void defaultExitHandler()
{
	/*if (!sys().arm7SCFGLocked())
	{
		rebootDSiMenuPP();
	}*/
	loadROMselect(ms().theme);
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv)
{
//---------------------------------------------------------------------------------

	// overwrite reboot stub identifier
	/*extern char *fake_heap_end;
	*fake_heap_end = 0;*/

	sys().initFilesystem("/_nds/TWiLightMenu/main.srldr");
	sys().flashcardUsed();
	ms();
	defaultExceptionHandler();

	if (!sys().fatInitOk())
	{
		consoleDemoInit();
		printf("fatInitDefault failed!");
		stop();
	}

	if (sys().isRegularDS()) {
		sysSetCartOwner(BUS_OWNER_ARM9); // Allow arm9 to access GBA ROM
		if (*(u16*)(0x020000C0) != 0x334D && *(u16*)(0x020000C0) != 0x3647 && *(u16*)(0x020000C0) != 0x4353 && *(u16*)(0x020000C0) != 0x5A45) {
			*(u16*)(0x020000C0) = 0;	// Clear Slot-2 flashcard flag
		}

		if (*(u16*)(0x020000C0) == 0) {
		  if (io_dldi_data->ioInterface.features & FEATURE_SLOT_NDS) {
			*(vu16*)(0x08000000) = 0x4D54;	// Write test
			if (*(vu16*)(0x08000000) != 0x4D54) {	// If not writeable
				_M3_changeMode(M3_MODE_RAM);	// Try again with M3
				*(u16*)(0x020000C0) = 0x334D;
				*(vu16*)(0x08000000) = 0x4D54;
			}
			if (*(vu16*)(0x08000000) != 0x4D54) {
				_G6_SelectOperation(G6_MODE_RAM);	// Try again with G6
				*(u16*)(0x020000C0) = 0x3647;
				*(vu16*)(0x08000000) = 0x4D54;
			}
			if (*(vu16*)(0x08000000) != 0x4D54) {
				_SC_changeMode(SC_MODE_RAM);	// Try again with SuperCard
				*(u16*)(0x020000C0) = 0x4353;
				*(vu16*)(0x08000000) = 0x4D54;
			}
			if (*(vu16*)(0x08000000) != 0x4D54) {
				cExpansion::SetRompage(381);	// Try again with EZ Flash
				expansion().SetRampage(cExpansion::EPsramPage);
				cExpansion::OpenNorWrite();
				cExpansion::SetSerialMode();
				*(u16*)(0x020000C0) = 0x5A45;
				*(vu16*)(0x08000000) = 0x4D54;
			}
			if (*(vu16*)(0x08000000) != 0x4D54) {
				*(u16*)(0x020000C0) = 0;
			}
		  } else if (io_dldi_data->ioInterface.features & FEATURE_SLOT_GBA) {
			if (memcmp(io_dldi_data->friendlyName, "M3 Adapter", 10) == 0) {
				*(u16*)(0x020000C0) = 0x334D;
			} else if (memcmp(io_dldi_data->friendlyName, "G6", 2) == 0) {
				*(u16*)(0x020000C0) = 0x3647;
			} else if (memcmp(io_dldi_data->friendlyName, "SuperCard", 9) == 0) {
				*(u16*)(0x020000C0) = 0x4353;
			}
		  }
		}
	}

	useTwlCfg = (REG_SCFG_EXT!=0 && (*(u8*)0x02000400 & 0x0F) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0) && (*(u8*)0x02000448 != 0));
	if (REG_SCFG_EXT!=0) {
		u16 cfgCrc = swiCRC16(0xFFFF, (void*)0x02000400, 0x128);
		u16 cfgCrcFromFile = 0;

		char twlCfgPath[256];
		sprintf(twlCfgPath, "%s:/_nds/TWiLightMenu/16KBcache.bin", sdFound() ? "sd" : "fat");

		FILE* twlCfg = fopen(twlCfgPath, "rb");
		if (twlCfg) {
			fseek(twlCfg, 0x4000, SEEK_SET);
			fread((u16*)&cfgCrcFromFile, sizeof(u16), 1, twlCfg);
		}
		if (useTwlCfg) {
			if (cfgCrcFromFile != cfgCrc) {
				fclose(twlCfg);
				// Cache first 16KB containing TWLCFG, in case some homebrew overwrites it
				twlCfg = fopen(twlCfgPath, "wb");
				fwrite((void*)0x02000000, 1, 0x4000, twlCfg);
				fwrite((u16*)&cfgCrc, sizeof(u16), 1, twlCfg);
			}
		} else if (twlCfg) {
			if (cfgCrc != cfgCrcFromFile) {
				// Reload first 16KB from cache
				fseek(twlCfg, 0, SEEK_SET);
				fread((void*)0x02000000, 1, 0x4000, twlCfg);
				useTwlCfg = ((*(u8*)0x02000400 & 0x0F) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0) && (*(u8*)0x02000448 != 0));
			}
		}
		fclose(twlCfg);
	}

	if (access(settingsinipath, F_OK) != 0 && (access("fat:/", F_OK) == 0)) {
		settingsinipath =
		    DSIMENUPP_INI_FC; // Fallback to .ini path on flashcard, if not found on
							   // SD card, or if SD access is disabled
	}

	ms().loadSettings();
	bs().loadSettings();

	if (isDSiMode()) {
		scanKeys();
		if (!(keysHeld() & KEY_SELECT)) {
			flashcardInit();
		}
	}

	bool fcFound = flashcardFound();

	if (fcFound) {
	  if (ms().launchType[true] == Launch::ESDFlashcardLaunch) {
		// Move .sav back to "saves" folder
		std::string filename = ms().romPath[true];
		const size_t last_slash_idx = filename.find_last_of("/");
		if (std::string::npos != last_slash_idx) {
			filename.erase(0, last_slash_idx + 1);
		}

		loadPerGameSettings(filename);

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

		std::string savename = replaceAll(filename, typeToReplace, getSavExtension());
		std::string savenameFc = replaceAll(filename, typeToReplace, ".sav");
		std::string romfolder = ms().romPath[true];
		while (!romfolder.empty() && romfolder[romfolder.size()-1] != '/') {
			romfolder.resize(romfolder.size()-1);
		}
		std::string romFolderNoSlash = romfolder;
		RemoveTrailingSlashes(romFolderNoSlash);
		std::string savepath = romFolderNoSlash + "/saves/" + savename;
		std::string savepathFc = romFolderNoSlash + "/" + savenameFc;
		if (access(savepathFc.c_str(), F_OK) == 0) {
			rename(savepathFc.c_str(), savepath.c_str());
		}
	  } else if (sys().isRegularDS() && (*(u16*)(0x020000C0) != 0) && (ms().launchType[true] == Launch::EGBANativeLaunch)) {
			if (*(u16*)(0x020000C0) == 0x5A45) {
				expansion().SetRampage(0);	// Switch to GBA SRAM for EZ Flash
			}
			u8 byteBak = *(vu8*)(0x0A000000);
			*(vu8*)(0x0A000000) = 'T';	// SRAM write test
		  if (*(vu8*)(0x0A000000) == 'T') {	// Check if SRAM is writeable
			*(vu8*)(0x0A000000) = byteBak;
			std::string savepath = replaceAll(ms().romPath[true], ".gba", ".sav");
			u32 savesize = getFileSize(savepath.c_str());
			if (savesize > 0x20000) savesize = 0x20000;
			if (savesize > 0) {
				// Try to restore save from SRAM
				bool restoreSave = false;
				extern char copyBuf[0x8000];
				u32 ptr = 0x0A000000;
				u32 len = savesize;
				gbaSramAccess(true);	// Switch to GBA SRAM
				for (u32 i = 0; i < savesize; i += 0x8000) {
					if (ptr >= 0x0A010000 || len <= 0) {
						break;
					}
					cExpansion::ReadSram(ptr,(u8*)copyBuf,(len>0x8000 ? 0x8000 : len));
					for (u32 i2 = 0; i2 < (len>0x8000 ? 0x8000 : len); i2++) {
						if (copyBuf[i2] != 0) {
							restoreSave = true;
							break;
						}
					}
					ptr += 0x8000;
					len -= 0x8000;
					if (restoreSave) break;
				}
				gbaSramAccess(false);	// Switch out of GBA SRAM
				if (restoreSave) {
					ptr = 0x0A000000;
					len = savesize;
					FILE* savFile = fopen(savepath.c_str(), "wb");
					for (u32 i = 0; i < savesize; i += 0x8000) {
						if (ptr >= 0x0A020000 || len <= 0) {
							break;	// In case if this writes a save bigger than 128KB
						} else if (ptr >= 0x0A010000) {
							toncset(&copyBuf, 0, 0x8000);
						} else {
							gbaSramAccess(true);	// Switch to GBA SRAM
							cExpansion::ReadSram(ptr,(u8*)copyBuf,0x8000);
							gbaSramAccess(false);	// Switch out of GBA SRAM
						}
						fwrite(&copyBuf, 1, (len>0x8000 ? 0x8000 : len), savFile);
						ptr += 0x8000;
						len -= 0x8000;
					}
					fclose(savFile);

					gbaSramAccess(true);	// Switch to GBA SRAM
					// Wipe out SRAM after restoring save
					toncset(&copyBuf, 0, 0x8000);
					cExpansion::WriteSram(0x0A000000, (u8*)copyBuf, 0x8000);
					cExpansion::WriteSram(0x0A008000, (u8*)copyBuf, 0x8000);
					gbaSramAccess(false);	// Switch out of GBA SRAM
				}
			}
		  }
			if (*(u16*)(0x020000C0) == 0x5A45) {
				expansion().SetRampage(cExpansion::EPsramPage);
			}
	  }
	}

	if (isDSiMode() && ms().consoleModel < 2) {
		if (ms().wifiLed == -1) {
			if (*(u8*)(0x023FFD01) == 0x13) {
				ms().wifiLed = true;
			} else if (*(u8*)(0x023FFD01) == 0 || *(u8*)(0x023FFD01) == 0x12) {
				ms().wifiLed = false;
			}
		} else {
			*(u8*)(0x023FFD00) = (ms().wifiLed ? 0x13 : 0);		// WiFi On/Off
		}
	}

	if (ms().dsiWareExploit == 0) {
		if (access("sd:/_nds/nds-bootstrap/srBackendId.bin", F_OK) == 0)
			remove("sd:/_nds/nds-bootstrap/srBackendId.bin");
	}

	if (sdFound()) {
		mkdir("sd:/_gba", 0777);
		mkdir("sd:/_nds/TWiLightMenu/gamesettings", 0777);
	}
	if (flashcardFound()) {
		mkdir("fat:/_gba", 0777);
		mkdir("fat:/_nds/TWiLightMenu/gamesettings", 0777);
	}

	runGraphicIrq();

	if (REG_SCFG_EXT != 0) {
		*(vu32*)(0x0DFFFE0C) = 0x53524C41;		// Check for 32MB of RAM
		bool isDevConsole = (*(vu32*)(0x0DFFFE0C) == 0x53524C41);
		if (isDevConsole)
		{
			// Check for Nintendo 3DS console
			if (isDSiMode() && sdFound())
			{
				bool is3DS = !nandio_startup();
				int resultModel = 1+is3DS;
				if (ms().consoleModel != resultModel || bs().consoleModel != resultModel)
				{
					ms().consoleModel = resultModel;
					bs().consoleModel = resultModel;
					ms().saveSettings();
					bs().saveSettings();
				}
			}
			else if (ms().consoleModel < 1 || ms().consoleModel > 2
				  || bs().consoleModel < 1 || bs().consoleModel > 2)
			{
				consoleModelSelect();			// There's no NAND or SD card
			}
		}
		else
		if (ms().consoleModel != 0 || bs().consoleModel != 0)
		{
			ms().consoleModel = 0;
			bs().consoleModel = 0;
			ms().saveSettings();
			bs().saveSettings();
		}
	}

	bool soundBankLoaded = false;

	bool softResetParamsFound = false;
	const char* softResetParamsPath = (isDSiMode() ? (!ms().previousUsedDevice ? "sd:/_nds/nds-bootstrap/softResetParams.bin" : "fat:/_nds/nds-bootstrap/softResetParams.bin") : "fat:/_nds/nds-bootstrap/B4DS-softResetParams.bin");
	u32 softResetParams = 0;
	FILE* file = fopen(softResetParamsPath, "rb");
	if (file) {
		fread(&softResetParams, sizeof(u32), 1, file);
		softResetParamsFound = (softResetParams != 0xFFFFFFFF);
	}
	fclose(file);

	if (softResetParamsFound) {
		scanKeys();
		if (keysHeld() & KEY_B) {
			softResetParams = 0xFFFFFFFF;
			file = fopen(softResetParamsPath, "wb");
			fwrite(&softResetParams, sizeof(u32), 1, file);
			fclose(file);
			softResetParamsFound = false;
		}
	}

	if (!softResetParamsFound && (ms().dsiSplash || ms().showlogo)) {
		// Get date
		int birthMonth = (useTwlCfg ? *(u8*)0x02000446 : PersonalData->birthMonth);
		int birthDay = (useTwlCfg ? *(u8*)0x02000447 : PersonalData->birthDay);
		char soundBankPath[32], currentDate[16], birthDate[16];
		time_t Raw;
		time(&Raw);
		const struct tm *Time = localtime(&Raw);

		strftime(currentDate, sizeof(currentDate), "%m/%d", Time);
		sprintf(birthDate, "%02d/%02d", birthMonth, birthDay);

		sprintf(soundBankPath, "nitro:/soundbank%s.bin", (strcmp(currentDate, birthDate) == 0) ? "_bday" : "");

		// Load sound bank into memory
		FILE* soundBankF = fopen(soundBankPath, "rb");
		fread(soundBank, 1, sizeof(soundBank), soundBankF);
		fclose(soundBankF);
		soundBankLoaded = true;
	}

	if (!softResetParamsFound && ms().dsiSplash && (isDSiMode() ? fifoGetValue32(FIFO_USER_01) != 0x01 : *(u32*)0x02000000 != 1)) {
		bootSplashInit();
		if (isDSiMode()) fifoSendValue32(FIFO_USER_01, 10);
	}
	*(u32*)0x02000000 = 1;

	if ((access(settingsinipath, F_OK) != 0)
	|| (ms().theme < 0) || (ms().theme == 3) || (ms().theme > 6)) {
		// Create or modify "settings.ini"
		(ms().theme == 3) ? ms().theme = 2 : ms().theme = 0;
		ms().saveSettings();
	}

	if (access(BOOTSTRAP_INI, F_OK) != 0) {
		u64 driveSize = 0;
		int gbNumber = 0;
		struct statvfs st;
		if (statvfs(sdFound() ? "sd:/" : "fat:/", &st) == 0) {
			driveSize = st.f_bsize * st.f_blocks;
		}
		for (u64 i = 0; i <= driveSize; i += 0x40000000) {
			gbNumber++;	// Get GB number
		}
		bs().cacheFatTable = (gbNumber < 32);	// Enable saving FAT table cache, if SD/Flashcard is 32GB or less

		// Create "nds-bootstrap.ini"
		bs().saveSettings();
	}
	
	scanKeys();

	if (softResetParamsFound
	 || (ms().autorun && !(keysHeld() & KEY_B))
	 || (!ms().autorun && (keysHeld() & KEY_B)))
	{
		lastRunROM();
	}

	// If in DSi mode with no flashcard & SCFG access, attempt to cut slot1 power to save battery
	if (isDSiMode() && !fcFound && !sys().arm7SCFGLocked() && !ms().autostartSlot1) {
		disableSlot1();
	}

	if (!softResetParamsFound && ms().autostartSlot1 && isDSiMode() && REG_SCFG_MC != 0x11 && !fcFound && !(keysHeld() & KEY_SELECT)) {
		if (ms().slot1LaunchMethod==0 || sys().arm7SCFGLocked()) {
			dsCardLaunch();
		} else if (ms().slot1LaunchMethod==2) {
			unlaunchRomBoot("cart:");
		} else {
			runNdsFile("/_nds/TWiLightMenu/slot1launch.srldr", 0, NULL, true, true, false, true, true);
		}
	}

	keysSetRepeat(25, 5);
	// snprintf(vertext, sizeof(vertext), "Ver %d.%d.%d   ", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH); // Doesn't work :(

	if (ms().showlogo)
	{
		if (!soundBankLoaded || strncmp((char*)soundBank+4, "*maxmod*", 8) != 0) {
			// Load sound bank into memory
			FILE* soundBankF = fopen("nitro:/soundbank.bin", "rb");
			fread(soundBank, 1, sizeof(soundBank), soundBankF);
			fclose(soundBankF);
			soundBankLoaded = true;
		}

		loadTitleGraphics();
		fadeType = true;
		for (int i = 0; i < 15; i++)
		{
			swiWaitForVBlank();
		}
		twlMenuVideo();
	}

	scanKeys();

	if (keysHeld() & KEY_SELECT)
	{
		screenmode = 1;
	}

	srand(time(NULL));

	while (1) {
		if (screenmode == 1) {
			fadeColor = true;
			controlTopBright = true;
			controlBottomBright = true;
			fadeType = false;
			for (int i = 0; i < 25; i++) {
				swiWaitForVBlank();
			}
			if (!isDSiMode()) {
				chdir("fat:/");
			}
			runNdsFile("/_nds/TWiLightMenu/settings.srldr", 0, NULL, true, false, false, true, true);
		} else {
			if (ms().showMainMenu) {
				loadMainMenu();
			}
			loadROMselect(ms().theme);
		}
	}

	return 0;
}
