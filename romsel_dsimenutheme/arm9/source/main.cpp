#include <nds.h>
#include <nds/arm9/dldi.h>
#include "io_m3_common.h"
#include "io_g6_common.h"
#include "io_sc_common.h"
#include "exptools.h"

#include <fat.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>

#include <gl2d.h>
#include <maxmod9.h>
#include <string.h>
#include <unistd.h>

#include "date.h"
#include "fileCopy.h"

#include "graphics/graphics.h"

#include "common/dsimenusettings.h"
#include "common/flashcard.h"
#include "common/nitrofs.h"
#include "common/systemdetails.h"
#include "graphics/ThemeConfig.h"
#include "graphics/ThemeTextures.h"
#include "graphics/themefilenames.h"

#include "errorScreen.h"
#include "fileBrowse.h"
#include "nds_loader_arm9.h"
#include "gbaswitch.h"
#include "ndsheaderbanner.h"
#include "perGameSettings.h"
//#include "tool/logging.h"

#include "graphics/fontHandler.h"
#include "graphics/iconHandler.h"

#include "common/inifile.h"
#include "tool/stringtool.h"
#include "common/tonccpy.h"

#include "sound.h"
#include "language.h"

#include "cheat.h"
#include "crc.h"

#include "donorMap.h"
#include "mpuMap.h"
#include "speedBumpExcludeMap.h"
#include "saveMap.h"

#include "sr_data_srllastran.h"		 // For rebooting into the game

bool useTwlCfg = false;

bool whiteScreen = true;
bool fadeType = false; // false = out, true = in
bool fadeSpeed = true; // false = slow (for DSi launch effect), true = fast
bool fadeColor = true; // false = black, true = white
bool controlTopBright = true;
bool controlBottomBright = true;
bool widescreenEffects = false;

extern void ClearBrightness();
extern bool displayGameIcons;
extern bool showProgressIcon;
extern bool showProgressBar;
extern int progressBarLength;

const char *settingsinipath = "sd:/_nds/TWiLightMenu/settings.ini";
const char *bootstrapinipath = "sd:/_nds/nds-bootstrap.ini";

const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char hiyaNdsPath[14] = {'s', 'd', 'm', 'c', ':', '/', 'h', 'i', 'y', 'a', '.', 'd', 's', 'i'};
char unlaunchDevicePath[256];

bool extention(const std::string& filename, const char* ext) {
	if(strcasecmp(filename.c_str() + filename.size() - strlen(ext), ext)) {
		return false;
	} else {
		return true;
	}
}

/**
 * Remove trailing slashes from a pathname, if present.
 * @param path Pathname to modify.
 */
void RemoveTrailingSlashes(std::string &path) {
	if (path.size() == 0) return;
	while (!path.empty() && path[path.size() - 1] == '/') {
		path.resize(path.size() - 1);
	}
}

/**
 * Remove trailing spaces from a cheat code line, if present.
 * @param path Code line to modify.
 */
/*static void RemoveTrailingSpaces(std::string& code)
{
	while (!code.empty() && code[code.size()-1] == ' ') {
		code.resize(code.size()-1);
	}
}*/

// These are used by flashcard functions and must retain their trailing slash.
static const std::string slashchar = "/";
static const std::string woodfat = "fat0:/";
static const std::string dstwofat = "fat1:/";

typedef TWLSettings::TLaunchType Launch;

int mpuregion = 0;
int mpusize = 0;
bool ceCached = true;

bool applaunch = false;

bool useBackend = false;

bool dropDown = false;
int currentBg = 0;
bool showSTARTborder = false;
bool buttonArrowTouched[2] = {false};
bool scrollWindowTouched = false;

bool titleboxXmoveleft = false;
bool titleboxXmoveright = false;

bool applaunchprep = false;

int spawnedtitleboxes = 0;

s16 usernameRendered[11] = {0};
bool usernameRenderedDone = false;

bool showColon = true;

touchPosition touch;

//---------------------------------------------------------------------------------
void stop(void) {
	//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

/**
 * Set donor SDK version for a specific game.
 */
int SetDonorSDK() {
	char gameTid3[5];
	for (int i = 0; i < 3; i++) {
		gameTid3[i] = gameTid[CURPOS][i];
	}

	for (auto i : donorMap) {
		if (i.first == 5 && gameTid3[0] == 'V')
			return 5;

		if (i.second.find(gameTid3) != i.second.cend())
			return i.first;
	}

	return 0;
}

/**
 * Set MPU settings for a specific game.
 */
void SetMPUSettings(const char *filename) {
	scanKeys();
	int pressed = keysHeld();

	if (pressed & KEY_B) {
		mpuregion = 1;
	} else if (pressed & KEY_X) {
		mpuregion = 2;
	} else if (pressed & KEY_Y) {
		mpuregion = 3;
	} else {
		mpuregion = 0;
	}
	if (pressed & KEY_RIGHT) {
		mpusize = 3145728;
	} else if (pressed & KEY_LEFT) {
		mpusize = 1;
	} else {
		mpusize = 0;
	}

	// Check for games that need an MPU size of 3 MB.
	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(mpu_3MB_list) / sizeof(mpu_3MB_list[0]); i++) {
		if (memcmp(gameTid[CURPOS], mpu_3MB_list[i], 3) == 0) {
			// Found a match.
			mpuregion = 1;
			mpusize = 3145728;
			break;
		}
	}
}

/**
 * Move nds-bootstrap's cardEngine_arm9 to cached memory region for some games.
 */
void SetSpeedBumpExclude(void) {
	if (!isDSiMode() || (perGameSettings_heapShrink >= 0 && perGameSettings_heapShrink < 2)) {
		ceCached = perGameSettings_heapShrink;
		return;
	}

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(sbeList2)/sizeof(sbeList2[0]); i++) {
		if (memcmp(gameTid[CURPOS], sbeList2[i], 3) == 0) {
			// Found match
			ceCached = false;
		}
	}
}

/**
 * Fix AP for some games.
 */
std::string setApFix(const char *filename) {
	remove("fat:/_nds/nds-bootstrap/apFix.ips");

	bool ipsFound = false;
	char ipsPath[256];
	snprintf(ipsPath, sizeof(ipsPath), "%s:/_nds/TWiLightMenu/apfix/%s.ips", sdFound() ? "sd" : "fat", filename);
	ipsFound = (access(ipsPath, F_OK) == 0);

	if (!ipsFound) {
		snprintf(ipsPath, sizeof(ipsPath), "%s:/_nds/TWiLightMenu/apfix/%s-%X.ips", sdFound() ? "sd" : "fat", gameTid[CURPOS], headerCRC[CURPOS]);
		ipsFound = (access(ipsPath, F_OK) == 0);
	}

	if (ipsFound) {
		if (ms().secondaryDevice && sdFound()) {
			mkdir("fat:/_nds", 0777);
			mkdir("fat:/_nds/nds-bootstrap", 0777);
			fcopy(ipsPath, "fat:/_nds/nds-bootstrap/apFix.ips");
			return "fat:/_nds/nds-bootstrap/apFix.ips";
		}
		return ipsPath;
	}

	return "";
}

sNDSHeader ndsCart;

/**
 * Enable widescreen for some games.
 */
void SetWidescreen(const char *filename) {
	remove("/_nds/nds-bootstrap/wideCheatData.bin");

	bool useWidescreen = (perGameSettings_wideScreen == -1 ? ms().wideScreen : perGameSettings_wideScreen);

	if ((isDSiMode() && sys().arm7SCFGLocked()) || ms().consoleModel < 2 || !useWidescreen
	|| (access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) != 0)) {
		ms().homebrewHasWide = false;
		return;
	}
	
	bool wideCheatFound = false;
	char wideBinPath[256];
	if (ms().launchType[ms().secondaryDevice] == Launch::ESDFlashcardLaunch) {
		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s.bin", filename);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	}

	if (ms().slot1Launched) {
		// Reset Slot-1 to allow reading card header
		sysSetCardOwner (BUS_OWNER_ARM9);
		disableSlot1();
		for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
		enableSlot1();
		for(int i = 0; i < 15; i++) { swiWaitForVBlank(); }

		cardReadHeader((uint8*)&ndsCart);

		char s1GameTid[5];
		tonccpy(s1GameTid, ndsCart.gameCode, 4);
		s1GameTid[4] = 0;

		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s-%X.bin", s1GameTid, ndsCart.headerCRC16);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	} else if (!wideCheatFound) {
		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s-%X.bin", gameTid[CURPOS], headerCRC[CURPOS]);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	}

	if (isHomebrew[CURPOS]) {
		return;
	}

	if (wideCheatFound) {
		std::string resultText;
		mkdir("/_nds", 0777);
		mkdir("/_nds/nds-bootstrap", 0777);
		if (fcopy(wideBinPath, "/_nds/nds-bootstrap/wideCheatData.bin") == 0) {
			return;
		} else {
			resultText = STR_FAILED_TO_COPY_WIDESCREEN;
		}
		remove("/_nds/nds-bootstrap/wideCheatData.bin");
		clearText();
		printLarge(false, 0, ms().theme == 4 ? 24 : 72, resultText, Alignment::center);
		if (ms().theme != 4) {
			fadeType = true; // Fade in from white
		}
		for (int i = 0; i < 60 * 3; i++) {
			swiWaitForVBlank(); // Wait 3 seconds
		}
		if (ms().theme != 4) {
			fadeType = false;	   // Fade to white
			for (int i = 0; i < 25; i++) {
				swiWaitForVBlank();
			}
		}
	}
}

char filePath[PATH_MAX];

void doPause() {
	while (1) {
		scanKeys();
		if (keysDown() & KEY_START)
			break;
		snd().updateStream();
		swiWaitForVBlank();
	}
	scanKeys();
}

void loadGameOnFlashcard (const char *ndsPath, bool dsGame) {
	bool runNds_boostCpu = false;
	bool runNds_boostVram = false;
	std::string filename = ndsPath;
	const size_t last_slash_idx = filename.find_last_of("/");
	if (std::string::npos != last_slash_idx) {
		filename.erase(0, last_slash_idx + 1);
	}

	loadPerGameSettings(filename);

	if ((REG_SCFG_EXT != 0) && dsGame) {
		runNds_boostCpu = perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu;
		runNds_boostVram = perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram;
	}
	if (dsGame) {
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
		std::string romFolderNoSlash = ms().romfolder[true];
		RemoveTrailingSlashes(romFolderNoSlash);
		mkdir("saves", 0777);
		std::string savepath = romFolderNoSlash + "/saves/" + savename;
		std::string savepathFc = romFolderNoSlash + "/" + savenameFc;
		rename(savepath.c_str(), savepathFc.c_str());
	}

	std::string fcPath;
	int err = 0;
	snd().stopStream();

	if ((memcmp(io_dldi_data->friendlyName, "R4(DS) - Revolution for DS", 26) == 0)
	 || (memcmp(io_dldi_data->friendlyName, "R4TF", 4) == 0)
	 || (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0)) {
		CIniFile fcrompathini("fat:/_wfwd/lastsave.ini");
		fcPath = replaceAll(ndsPath, "fat:/", woodfat);
		fcrompathini.SetString("Save Info", "lastLoaded", fcPath);
		fcrompathini.SaveIniFile("fat:/_wfwd/lastsave.ini");
		err = runNdsFile("fat:/Wfwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	} else if (memcmp(io_dldi_data->friendlyName, "Acekard AK2", 0xB) == 0) {
		CIniFile fcrompathini("fat:/_afwd/lastsave.ini");
		fcPath = replaceAll(ndsPath, "fat:/", woodfat);
		fcrompathini.SetString("Save Info", "lastLoaded", fcPath);
		fcrompathini.SaveIniFile("fat:/_afwd/lastsave.ini");
		err = runNdsFile("fat:/Afwd.dat", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	} else if (memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0) {
		CIniFile fcrompathini("fat:/_dstwo/autoboot.ini");
		fcPath = replaceAll(ndsPath, "fat:/", dstwofat);
		fcrompathini.SetString("Dir Info", "fullName", fcPath);
		fcrompathini.SaveIniFile("fat:/_dstwo/autoboot.ini");
		err = runNdsFile("fat:/_dstwo/autoboot.nds", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	} else if ((memcmp(io_dldi_data->friendlyName, "TTCARD", 6) == 0)
			 || (memcmp(io_dldi_data->friendlyName, "DSTT", 4) == 0)
			 || (memcmp(io_dldi_data->friendlyName, "DEMON", 5) == 0)) {
		CIniFile fcrompathini("fat:/TTMenu/YSMenu.ini");
		fcPath = replaceAll(ndsPath, "fat:/", slashchar);
		fcrompathini.SetString("YSMENU", "AUTO_BOOT", fcPath);
		fcrompathini.SaveIniFile("fat:/TTMenu/YSMenu.ini");
		err = runNdsFile("fat:/YSMenu.nds", 0, NULL, true, true, true, runNds_boostCpu, runNds_boostVram);
	}

	char text[64];
	snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
	fadeType = true;	// Fade from white
	if (err == 0) {
		printLarge(false, 4, 4, STR_ERROR_FLASHCARD_UNSUPPORTED);
		printLarge(false, 4, 68, io_dldi_data->friendlyName);
	} else {
		printLarge(false, 4, 4, text);
	}
	printSmall(false, 4, 90, STR_PRESS_B_RETURN);
	int pressed = 0;
	do {
		scanKeys();
		pressed = keysDownRepeat();
		checkSdEject();
		swiWaitForVBlank();
	} while (!(pressed & KEY_B));
	fadeType = false;	// Fade to white
	for (int i = 0; i < 25; i++) {
		swiWaitForVBlank();
	}
	if (!isDSiMode()) {
		chdir("fat:/");
	} else if (sdFound()) {
		chdir("sd:/");
	}
	runNdsFile("/_nds/TWiLightMenu/dsimenu.srldr", 0, NULL, true, false, false, true, true);
	stop();
}

void unlaunchRomBoot(const char* rom) {
	snd().stopStream();
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

void unlaunchSetHiyaBoot(void) {
	snd().stopStream();
	tonccpy((u8 *)0x02000800, unlaunchAutoLoadID, 12);
	*(u16 *)(0x0200080C) = 0x3F0;			   // Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16 *)(0x0200080E) = 0;			   // Unlaunch CRC16 (empty)
	*(u32 *)(0x02000810) = (BIT(0) | BIT(1));	  // Load the title at 2000838h
							   // Use colors 2000814h
	*(u16 *)(0x02000814) = 0x7FFF;			   // Unlaunch Upper screen BG color (0..7FFFh)
	*(u16 *)(0x02000816) = 0x7FFF;			   // Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8 *)0x02000818, 0, 0x20 + 0x208 + 0x1C0); // Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < 14; i++) {
		*(u8 *)(0x02000838 + i2) =
		    hiyaNdsPath[i]; // Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	while (*(vu16 *)(0x0200080E) == 0) { // Keep running, so that CRC16 isn't 0
		*(u16 *)(0x0200080E) = swiCRC16(0xFFFF, (void *)0x02000810, 0x3F0); // Unlaunch CRC16
	}
}

/**
 * Reboot into an SD game when in DS mode.
 */
void ntrStartSdGame(void) {
	if (ms().consoleModel == 0) {
		unlaunchRomBoot("sd:/_nds/TWiLightMenu/resetgame.srldr");
	} else {
		tonccpy((u32 *)0x02000300, sr_data_srllastran, 0x020);
		DC_FlushAll();						// Make reboot not fail
		fifoSendValue32(FIFO_USER_02, 1);
		stop();
	}
}

void dsCardLaunch() {
	snd().stopStream();
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

int main(int argc, char **argv) {
	/*SetBrightness(0, 0);
	SetBrightness(1, 0);
	consoleDemoInit();*/

	defaultExceptionHandler();
	sys().initFilesystem();
	sys().initArm7RegStatuses();

	if (access(settingsinipath, F_OK) != 0 && flashcardFound()) {
		settingsinipath =
		    "fat:/_nds/TWiLightMenu/settings.ini"; // Fallback to .ini path on flashcard, if not found on
							   // SD card, or if SD access is disabled
	}

	useTwlCfg = (REG_SCFG_EXT!=0 && (*(u8*)0x02000400 & 0x0F) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0) && (*(u8*)0x02000448 != 0));

	//logInit();
	ms().loadSettings();
	widescreenEffects = (ms().consoleModel >= 2 && ms().wideScreen && access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) == 0);
	tfn(); //
	tc().loadConfig();
	tex().videoSetup(); // allocate texture pointers

	fontInit();

	if (ms().theme == 5) {
		tex().loadHBTheme();
	} else if (ms().theme == 4) {
		tex().loadSaturnTheme();
	} else if (ms().theme == 1) {
		tex().load3DSTheme();
	} else {
		tex().loadDSiTheme();
	}

	//printf("Username copied\n");
	tonccpy(usernameRendered, (useTwlCfg ? (s16*)0x02000448 : PersonalData->name), sizeof(s16) * 10);

	if (!sys().fatInitOk()) {
		graphicsInit();
		fontInit();
		whiteScreen = false;
		fadeType = true;
		for (int i = 0; i < 5; i++)
			swiWaitForVBlank();
		if (!dropDown && ms().theme == 0) {
			dropDown = true;
			for (int i = 0; i < 72; i++) 
				swiWaitForVBlank();
		} else {
			for (int i = 0; i < 25; i++)
				swiWaitForVBlank();
		}
		currentBg = 1;
		printLarge(false, 0, 32, "fatInitDefault failed!", Alignment::center);

		// Control the DSi Menu, but can't launch anything.
		int pressed = 0;

		while (1) {
			// Power saving loop. Only poll the keys once per frame and sleep the CPU if there is nothing
			// else to do
			do {
				scanKeys();
				pressed = keysDownRepeat();
				snd().updateStream();
				swiWaitForVBlank();
			} while (!pressed);

			if ((pressed & KEY_LEFT) && !titleboxXmoveleft && !titleboxXmoveright) {
				CURPOS -= 1;
				if (CURPOS >= 0)
					titleboxXmoveleft = true;
			} else if ((pressed & KEY_RIGHT) && !titleboxXmoveleft && !titleboxXmoveright) {
				CURPOS += 1;
				if (CURPOS <= 39)
					titleboxXmoveright = true;
			}
			if (CURPOS < 0) {
				CURPOS = 0;
			} else if (CURPOS > 39) {
				CURPOS = 39;
			}
		}
	}

	if (ms().theme == 4 || ms().theme == 5) {
		whiteScreen = false;
		fadeColor = false;
	}

	langInit();

	std::string filename;

	if (isDSiMode() && sdFound() && ms().consoleModel < 2 && ms().launcherApp != -1) {
		u8 setRegion = 0;

		if (ms().sysRegion == -1) {
			// Determine SysNAND region by searching region of System Settings on SDNAND
			char tmdpath[256] = {0};
			for (u8 i = 0x41; i <= 0x5A; i++) {
				snprintf(tmdpath, sizeof(tmdpath), "sd:/title/00030015/484e42%x/content/title.tmd", i);
				if (access(tmdpath, F_OK) == 0) {
					setRegion = i;
					break;
				}
			}
		} else {
			switch (ms().sysRegion) {
			case 0:
			default:
				setRegion = 0x4A; // JAP
				break;
			case 1:
				setRegion = 0x45; // USA
				break;
			case 2:
				setRegion = 0x50; // EUR
				break;
			case 3:
				setRegion = 0x55; // AUS
				break;
			case 4:
				setRegion = 0x43; // CHN
				break;
			case 5:
				setRegion = 0x4B; // KOR
				break;
			}
		}

		snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath),
			 "nand:/title/00030017/484E41%x/content/0000000%i.app", setRegion, ms().launcherApp);
	}

	if (ms().showGba == 1) {
	  if (*(u16*)(0x020000C0) != 0) {
		sysSetCartOwner(BUS_OWNER_ARM9); // Allow arm9 to access GBA ROM
	  } else {
		ms().showGba = 0;	// Hide GBA ROMs
	  }
	}

	graphicsInit();
	iconManagerInit();

	keysSetRepeat(10, 2);

	std::vector<std::string> extensionList;
	if (ms().showNds) {
		extensionList.emplace_back(".nds");
		extensionList.emplace_back(".dsi");
		extensionList.emplace_back(".ids");
		extensionList.emplace_back(".srl");
		extensionList.emplace_back(".app");
		extensionList.emplace_back(".argv");
	}
	if (memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0) {
		extensionList.emplace_back(".plg");
	}
	if (ms().showRvid) {
		extensionList.emplace_back(".rvid");
		extensionList.emplace_back(".mp4");
	}
	if (ms().showGba) {
		extensionList.emplace_back(".gba");
	}
	if (ms().showA26) {
		extensionList.emplace_back(".a26");
	}
	if (ms().showA78) {
		extensionList.emplace_back(".a78");
	}
	if (ms().showGb) {
		extensionList.emplace_back(".gb");
		extensionList.emplace_back(".sgb");
		extensionList.emplace_back(".gbc");
	}
	if (ms().showNes) {
		extensionList.emplace_back(".nes");
		extensionList.emplace_back(".fds");
	}
	if (ms().showSmsGg) {
		extensionList.emplace_back(".sms");
		extensionList.emplace_back(".gg");
	}
	if (ms().showMd) {
		extensionList.emplace_back(".gen");
	}
	if (!isDSiMode() || (isDSiMode() && (flashcardFound() || !sys().arm7SCFGLocked()))) {
		if (ms().showSnes) {
			extensionList.emplace_back(".smc");
			extensionList.emplace_back(".sfc");
		}
		if (ms().showPce) {
			extensionList.emplace_back(".pce");
		}
	}
	srand(time(NULL));

	char path[256] = {0};

	//logPrint("snd()\n");
	snd();

	if (ms().theme == 4) {
		//logPrint("snd().playStartup()\n");
		snd().playStartup();
	} else if (ms().dsiMusic != 0) {
		if ((ms().theme == 1 && ms().dsiMusic == 1) || (ms().dsiMusic == 3 && tc().playStartupJingle())) {
			//logPrint("snd().playStartup()\n");
			snd().playStartup();
			//logPrint("snd().setStreamDelay(snd().getStartupSoundLength() - tc().startupJingleDelayAdjust())\n");
			snd().setStreamDelay(snd().getStartupSoundLength() - tc().startupJingleDelayAdjust());
		}
		//logPrint("snd().beginStream()\n");
		snd().beginStream();
	}

	if (ms().consoleModel < 2 && ms().previousUsedDevice && bothSDandFlashcard()
	&& ms().launchType[ms().previousUsedDevice] == Launch::EDSiWareLaunch && !ms().dsiWareBooter) {
	  if ((access(ms().dsiWarePubPath.c_str(), F_OK) == 0 && extention(ms().dsiWarePubPath.c_str(), ".pub")) ||
	    (access(ms().dsiWarePrvPath.c_str(), F_OK) == 0 && extention(ms().dsiWarePrvPath.c_str(), ".prv"))) {
		fadeType = true; // Fade in from white
		printSmall(false, 0, 20, STR_TAKEWHILE_CLOSELID, Alignment::center);
		printLarge(false, 0, (ms().theme == 4 ? 80 : 88), STR_NOW_COPYING_DATA, Alignment::center);
		printSmall(false, 0, (ms().theme == 4 ? 96 : 104), STR_DONOT_TURNOFF_POWER, Alignment::center);
		updateText(false);
		for (int i = 0; i < 15; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		for (int i = 0; i < 20; i++) {
			snd().updateStream();
			swiWaitForVBlank();
		}
		showProgressIcon = true;
		controlTopBright = false;
		if (access(ms().dsiWarePubPath.c_str(), F_OK) == 0) {
			fcopy("sd:/_nds/TWiLightMenu/tempDSiWare.pub", ms().dsiWarePubPath.c_str());
		}
		if (access(ms().dsiWarePrvPath.c_str(), F_OK) == 0) {
			fcopy("sd:/_nds/TWiLightMenu/tempDSiWare.prv", ms().dsiWarePrvPath.c_str());
		}
		showProgressIcon = false;
		if (ms().theme != 4) {
			fadeType = false; // Fade to white
			for (int i = 0; i < 25; i++) {
				snd().updateStream();
				swiWaitForVBlank();
			}
		}
		clearText();
		updateText(false);
	  }
	}

	while (1) {

		snprintf(path, sizeof(path), "%s", ms().romfolder[ms().secondaryDevice].c_str());
		// Set directory
		chdir(path);

		// Navigates to the file to launch
		filename = browseForFile(extensionList);

		////////////////////////////////////
		// Launch the item

		if (applaunch) {
			// Delete previously used DSiWare of flashcard from SD
			if (!ms().gotosettings && ms().consoleModel < 2 && ms().previousUsedDevice &&
			    bothSDandFlashcard()) {
				if (access("sd:/_nds/TWiLightMenu/tempDSiWare.dsi", F_OK) == 0) {
					remove("sd:/_nds/TWiLightMenu/tempDSiWare.dsi");
				}
				if (access("sd:/_nds/TWiLightMenu/tempDSiWare.pub", F_OK) == 0) {
					remove("sd:/_nds/TWiLightMenu/tempDSiWare.pub");
				}
				if (access("sd:/_nds/TWiLightMenu/tempDSiWare.prv", F_OK) == 0) {
					remove("sd:/_nds/TWiLightMenu/tempDSiWare.prv");
				}
			}

			// Construct a command line
			getcwd(filePath, PATH_MAX);
			int pathLen = strlen(filePath);
			vector<char *> argarray;

			bool isArgv = false;
			if (strcasecmp(filename.c_str() + filename.size() - 5, ".argv") == 0) {
				ms().romPath[ms().secondaryDevice] = std::string(filePath) + std::string(filename);

				FILE *argfile = fopen(filename.c_str(), "rb");
				char str[PATH_MAX], *pstr;
				const char seps[] = "\n\r\t ";

				while (fgets(str, PATH_MAX, argfile)) {
					// Find comment and end string there
					if ((pstr = strchr(str, '#')))
						*pstr = '\0';

					// Tokenize arguments
					pstr = strtok(str, seps);

					while (pstr != NULL) {
						argarray.push_back(strdup(pstr));
						pstr = strtok(NULL, seps);
					}
				}
				fclose(argfile);
				filename = argarray.at(0);
				isArgv = true;
			} else {
				argarray.push_back(strdup(filename.c_str()));
			}

			// Launch DSiWare .nds via Unlaunch
			if ((isDSiMode() || sdFound()) && isDSiWare[CURPOS]) {
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

				char *name = argarray.at(0);
				strcpy(filePath + pathLen, name);
				free(argarray.at(0));
				argarray.at(0) = filePath;

				ms().dsiWareSrlPath = std::string(argarray[0]);
				ms().dsiWarePubPath = replaceAll(argarray[0], typeToReplace, ".pub");
				ms().dsiWarePrvPath = replaceAll(argarray[0], typeToReplace, ".prv");
				if (!isArgv) {
					ms().romPath[ms().secondaryDevice] = std::string(argarray[0]);
				}
				ms().launchType[ms().secondaryDevice] = (ms().consoleModel>0 ? Launch::ESDFlashcardLaunch : Launch::EDSiWareLaunch);
				ms().previousUsedDevice = ms().secondaryDevice;
				ms().saveSettings();

				sNDSHeaderExt NDSHeader;

				FILE *f_nds_file = fopen(filename.c_str(), "rb");

				fread(&NDSHeader, 1, sizeof(NDSHeader), f_nds_file);
				fclose(f_nds_file);

				fadeSpeed = true; // Fast fading

				if ((getFileSize(ms().dsiWarePubPath.c_str()) == 0) && (NDSHeader.pubSavSize > 0)) {
					if (ms().theme == 5) displayGameIcons = false;
					clearText();
					if (memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
						// Display nothing
					} else if (ms().consoleModel >= 2) {
						printSmall(false, 0, 20, STR_BARSTOPPED_PRESSHOME, Alignment::center);
					} else {
						printSmall(false, 0, 20, STR_BARSTOPPED_CLOSELID, Alignment::center);
					}
					printLarge(false, 0, (ms().theme == 4 ? 80 : 88), STR_CREATING_PUBLIC_SAVE, Alignment::center);
					updateText(false);
					if (ms().theme != 4 && !fadeType) {
						fadeType = true; // Fade in from white
						for (int i = 0; i < 35; i++) {
							swiWaitForVBlank();
						}
					}
					showProgressIcon = true;

					static const int BUFFER_SIZE = 4096;
					char buffer[BUFFER_SIZE];
					toncset(buffer, 0, sizeof(buffer));
					char savHdrPath[64];
					snprintf(savHdrPath, sizeof(savHdrPath), "nitro:/DSiWareSaveHeaders/%x.savhdr",
						 (unsigned int)NDSHeader.pubSavSize);
					FILE *hdrFile = fopen(savHdrPath, "rb");
					if (hdrFile)
						fread(buffer, 1, 0x200, hdrFile);
					fclose(hdrFile);

					FILE *pFile = fopen(ms().dsiWarePubPath.c_str(), "wb");
					if (pFile) {
						fwrite(buffer, 1, sizeof(buffer), pFile);
						fseek(pFile, NDSHeader.pubSavSize - 1, SEEK_SET);
						fputc('\0', pFile);
						fclose(pFile);
					}
					showProgressIcon = false;
					clearText();
					printLarge(false, 0, (ms().theme == 4 ? 32 : 88), STR_PUBLIC_SAVE_CREATED, Alignment::center);
					updateText(false);
					for (int i = 0; i < 60; i++) {
						swiWaitForVBlank();
					}
					if (ms().theme == 5) displayGameIcons = true;
				}

				if ((getFileSize(ms().dsiWarePrvPath.c_str()) == 0) && (NDSHeader.prvSavSize > 0)) {
					if (ms().theme == 5) displayGameIcons = false;
					clearText();
					if (memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
						// Display nothing
					} else if (ms().consoleModel >= 2) {
						printSmall(false, 0, 20, STR_BARSTOPPED_PRESSHOME, Alignment::center);
					} else {
						printSmall(false, 0, 20, STR_BARSTOPPED_CLOSELID, Alignment::center);
					}
					printLarge(false, 0, (ms().theme == 4 ? 80 : 88), STR_CREATING_PRIVATE_SAVE, Alignment::center);
					updateText(false);
					if (ms().theme != 4 && !fadeType) {
						fadeType = true; // Fade in from white
						for (int i = 0; i < 35; i++) {
							swiWaitForVBlank();
						}
					}
					showProgressIcon = true;

					static const int BUFFER_SIZE = 4096;
					char buffer[BUFFER_SIZE];
					toncset(buffer, 0, sizeof(buffer));
					char savHdrPath[64];
					snprintf(savHdrPath, sizeof(savHdrPath), "nitro:/DSiWareSaveHeaders/%x.savhdr",
						 (unsigned int)NDSHeader.prvSavSize);
					FILE *hdrFile = fopen(savHdrPath, "rb");
					if (hdrFile)
						fread(buffer, 1, 0x200, hdrFile);
					fclose(hdrFile);

					FILE *pFile = fopen(ms().dsiWarePrvPath.c_str(), "wb");
					if (pFile) {
						fwrite(buffer, 1, sizeof(buffer), pFile);
						fseek(pFile, NDSHeader.prvSavSize - 1, SEEK_SET);
						fputc('\0', pFile);
						fclose(pFile);
					}
					showProgressIcon = false;
					clearText();
					printLarge(false, 0, (ms().theme == 4 ? 32 : 88), STR_PRIVATE_SAVE_CREATED, Alignment::center);
					updateText(false);
					for (int i = 0; i < 60; i++) {
						swiWaitForVBlank();
					}
					if (ms().theme == 5) displayGameIcons = true;
				}

				if (ms().theme != 4 && ms().theme != 5 && fadeType) {
					fadeType = false; // Fade to white
					for (int i = 0; i < 25; i++) {
						swiWaitForVBlank();
					}
				}

				if (ms().dsiWareBooter || ms().consoleModel > 0) {
					// Use nds-bootstrap
					loadPerGameSettings(filename);

					bootstrapinipath = "sd:/_nds/nds-bootstrap.ini";
					CIniFile bootstrapini(bootstrapinipath);
					bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", ms().dsiWareSrlPath);
					bootstrapini.SetString("NDS-BOOTSTRAP", "SAV_PATH", ms().dsiWarePubPath);
					bootstrapini.SetString("NDS-BOOTSTRAP", "AP_FIX_PATH", "");
					bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", -1);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", true);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", true);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", true);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "DONOR_SDK_VER", 5);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "GAME_SOFT_RESET", 1);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_REGION", 0);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", 0);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "CARDENGINE_CACHED", 1);
					bootstrapini.SetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", 
						(ms().forceSleepPatch
					|| (memcmp(io_dldi_data->friendlyName, "TTCARD", 6) == 0 && !sys().isRegularDS())
					|| (memcmp(io_dldi_data->friendlyName, "DSTT", 4) == 0 && !sys().isRegularDS())
					|| (memcmp(io_dldi_data->friendlyName, "DEMON", 5) == 0 && !sys().isRegularDS())
					|| (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0 && !sys().isRegularDS()))
					);

					bootstrapini.SaveIniFile(bootstrapinipath);

					if (ms().theme == 5) {
						fadeType = false;		  // Fade to black
						for (int i = 0; i < 60; i++) {
							swiWaitForVBlank();
						}
					}

					if (isDSiMode() || !ms().secondaryDevice) {
						SetWidescreen(filename.c_str());
					}
					if (!isDSiMode() && !ms().secondaryDevice) {
						ntrStartSdGame();
					}

					bool useNightly = (perGameSettings_bootstrapFile == -1 ? ms().bootstrapFile : perGameSettings_bootstrapFile);

					char ndsToBoot[256];
					sprintf(ndsToBoot, "sd:/_nds/nds-bootstrap-%s.nds", useNightly ? "nightly" : "release");
					if(access(ndsToBoot, F_OK) != 0) {
						sprintf(ndsToBoot, "fat:/_nds/nds-bootstrap-%s.nds", useNightly ? "nightly" : "release");
					}

					argarray.at(0) = (char *)ndsToBoot;
					snd().stopStream();
					int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);
					char text[64];
					snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
					clearText();
					fadeType = true;
					printLarge(false, 4, 4, text);
					if (err == 1) {
						printLarge(false, 4, 20, useNightly ? STR_BOOTSTRAP_NIGHTLY_NOT_FOUND : STR_BOOTSTRAP_RELEASE_NOT_FOUND);
					}
					printSmall(false, 4, 20 + calcLargeFontHeight(useNightly ? STR_BOOTSTRAP_NIGHTLY_NOT_FOUND : STR_BOOTSTRAP_RELEASE_NOT_FOUND), STR_PRESS_B_RETURN);
					updateText(false);
					int pressed = 0;
					do {
						scanKeys();
						pressed = keysDownRepeat();
						checkSdEject();
						swiWaitForVBlank();
					} while (!(pressed & KEY_B));
					fadeType = false;	// Fade to white
					for (int i = 0; i < 25; i++) {
						swiWaitForVBlank();
					}
					if (!isDSiMode()) {
						chdir("fat:/");
					} else if (sdFound()) {
						chdir("sd:/");
					}
					runNdsFile("/_nds/TWiLightMenu/dsimenu.srldr", 0, NULL, true, false, false, true, true);
					stop();
				}

				if (ms().secondaryDevice) {
					clearText();
					printSmall(false, 0, 20, STR_BARSTOPPED_CLOSELID, Alignment::center);
					printLarge(false, 0, (ms().theme == 4 ? 80 : 88), STR_NOW_COPYING_DATA, Alignment::center);
					printSmall(false, 0, (ms().theme == 4 ? 96 : 104), STR_DONOT_TURNOFF_POWER, Alignment::center);
					updateText(false);
					if (ms().theme != 4) {
						fadeType = true; // Fade in from white
						for (int i = 0; i < 35; i++) {
							swiWaitForVBlank();
						}
					}
					showProgressIcon = true;
					fcopy(ms().dsiWareSrlPath.c_str(), "sd:/_nds/TWiLightMenu/tempDSiWare.dsi");
					if ((access(ms().dsiWarePubPath.c_str(), F_OK) == 0) && (NDSHeader.pubSavSize > 0)) {
						fcopy(ms().dsiWarePubPath.c_str(),
						      "sd:/_nds/TWiLightMenu/tempDSiWare.pub");
					}
					if ((access(ms().dsiWarePrvPath.c_str(), F_OK) == 0) && (NDSHeader.prvSavSize > 0)) {
						fcopy(ms().dsiWarePrvPath.c_str(),
						      "sd:/_nds/TWiLightMenu/tempDSiWare.prv");
					}
					showProgressIcon = false;
					if (ms().theme != 4 && ms().theme != 5) {
						fadeType = false; // Fade to white
						for (int i = 0; i < 25; i++) {
							swiWaitForVBlank();
						}
					}

					if ((access(ms().dsiWarePubPath.c_str(), F_OK) == 0 && (NDSHeader.pubSavSize > 0))
					 || (access(ms().dsiWarePrvPath.c_str(), F_OK) == 0 && (NDSHeader.prvSavSize > 0))) {
						clearText();
						printLarge(false, 0, ms().theme == 4 ? 16 : 72, STR_RESTART_AFTER_SAVING, Alignment::center);
						updateText(false);
						if (ms().theme != 4) {
							fadeType = true; // Fade in from white
						}
						for (int i = 0; i < 60 * 3; i++) {
							swiWaitForVBlank(); // Wait 3 seconds
						}
						if (ms().theme != 4 && ms().theme != 5) {
							fadeType = false;	   // Fade to white
							for (int i = 0; i < 25; i++) {
								swiWaitForVBlank();
							}
						}
					}
				}

				if (ms().theme == 5) {
					fadeType = false;		  // Fade to black
					for (int i = 0; i < 60; i++) {
						swiWaitForVBlank();
					}
				}

				char unlaunchDevicePath[256];
				if (ms().secondaryDevice) {
					snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath),
						 "sdmc:/_nds/TWiLightMenu/tempDSiWare.dsi");
				} else {
					snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath), "__%s",
						 ms().dsiWareSrlPath.c_str());
					unlaunchDevicePath[0] = 's';
					unlaunchDevicePath[1] = 'd';
					unlaunchDevicePath[2] = 'm';
					unlaunchDevicePath[3] = 'c';
				}

				tonccpy((u8 *)0x02000800, unlaunchAutoLoadID, 12);
				*(u16 *)(0x0200080C) = 0x3F0;   // Unlaunch Length for CRC16 (fixed, must be 3F0h)
				*(u16 *)(0x0200080E) = 0;       // Unlaunch CRC16 (empty)
				*(u32 *)(0x02000810) = 0;       // Unlaunch Flags
				*(u32 *)(0x02000810) |= BIT(0); // Load the title at 2000838h
				*(u32 *)(0x02000810) |= BIT(1); // Use colors 2000814h
				*(u16 *)(0x02000814) = 0x7FFF;  // Unlaunch Upper screen BG color (0..7FFFh)
				*(u16 *)(0x02000816) = 0x7FFF;  // Unlaunch Lower screen BG color (0..7FFFh)
				toncset((u8 *)0x02000818, 0, 0x20 + 0x208 + 0x1C0); // Unlaunch Reserved (zero)
				int i2 = 0;
				for (int i = 0; i < (int)sizeof(unlaunchDevicePath); i++) {
					*(u8 *)(0x02000838 + i2) =
					    unlaunchDevicePath[i]; // Unlaunch Device:/Path/Filename.ext (16bit
								   // Unicode,end by 0000h)
					i2 += 2;
				}
				while (*(u16 *)(0x0200080E) == 0) { // Keep running, so that CRC16 isn't 0
					*(u16 *)(0x0200080E) =
					    swiCRC16(0xFFFF, (void *)0x02000810, 0x3F0); // Unlaunch CRC16
				}
				DC_FlushAll();
				fifoSendValue32(FIFO_USER_02, 1); // Reboot into DSiWare title, booted via Unlaunch
				for (int i = 0; i < 15; i++) {
					swiWaitForVBlank();
				}
			}

			// Launch .nds directly or via nds-bootstrap
			if (extention(filename, ".nds") || extention(filename, ".dsi")
			 || extention(filename, ".ids") || extention(filename, ".srl")
			 || extention(filename, ".app")) {
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

				bool dsModeSwitch = false;
				bool dsModeDSiWare = false;

				char gameTid3[5];
				for (int i = 0; i < 3; i++) {
					gameTid3[i] = gameTid[CURPOS][i];
				}

				if (memcmp(gameTid[CURPOS], "HND", 3) == 0 || memcmp(gameTid[CURPOS], "HNE", 3) == 0) {
					dsModeSwitch = true;
					dsModeDSiWare = true;
					useBackend = false; // Bypass nds-bootstrap
					ms().homebrewBootstrap = true;
				} else if (isHomebrew[CURPOS]) {
					loadPerGameSettings(filename);
					if (perGameSettings_directBoot || (ms().useBootstrap && ms().secondaryDevice)) {
						useBackend = false; // Bypass nds-bootstrap
					} else {
						useBackend = true;
					}
					if (isDSiMode() && !perGameSettings_dsiMode) {
						dsModeSwitch = true;
					}
					ms().homebrewBootstrap = true;
				} else {
					loadPerGameSettings(filename);
					useBackend = true;
					ms().homebrewBootstrap = false;
				}

				char *name = argarray.at(0);
				strcpy(filePath + pathLen, name);
				free(argarray.at(0));
				argarray.at(0) = filePath;
				if (useBackend) {
					if (ms().useBootstrap || !ms().secondaryDevice) {
						std::string path = argarray[0];
						std::string savename = replaceAll(filename, typeToReplace, getSavExtension());
						std::string ramdiskname = replaceAll(filename, typeToReplace, getImgExtension(perGameSettings_ramDiskNo));
						std::string romFolderNoSlash = ms().romfolder[ms().secondaryDevice];
						RemoveTrailingSlashes(romFolderNoSlash);
						mkdir(isHomebrew[CURPOS] ? "ramdisks" : "saves", 0777);
						std::string savepath = romFolderNoSlash + "/saves/" + savename;
						if (sdFound() && ms().secondaryDevice && ms().fcSaveOnSd) {
							savepath = replaceAll(savepath, "fat:/", "sd:/");
						}
						std::string ramdiskpath = romFolderNoSlash + "/ramdisks/" + ramdiskname;

						if (!isHomebrew[CURPOS] && (strcmp(gameTid[CURPOS], "NTR") != 0))
						{ // Create or expand save if game isn't homebrew
							int orgsavesize = getFileSize(savepath.c_str());
							int savesize = 524288; // 512KB (default size for most games)

							for (auto i : saveMap) {
								if (i.second.find(gameTid3) != i.second.cend()) {
									savesize = i.first;
									break;
								}
							}

							bool saveSizeFixNeeded = false;

							// TODO: If the list gets large enough, switch to bsearch().
							for (unsigned int i = 0; i < sizeof(saveSizeFixList) / sizeof(saveSizeFixList[0]); i++) {
								if (memcmp(gameTid[CURPOS], saveSizeFixList[i], 3) == 0) {
									// Found a match.
									saveSizeFixNeeded = true;
									break;
								}
							}

							if ((orgsavesize == 0 && savesize > 0) || (orgsavesize < savesize && saveSizeFixNeeded)) {
								if (ms().theme == 5) displayGameIcons = false;
								if (isDSiMode() && memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
									// Display nothing
								} else if (REG_SCFG_EXT != 0 && ms().consoleModel >= 2) {
									printSmall(false, 0, 20, STR_TAKEWHILE_PRESSHOME, Alignment::center);
								} else {
									printSmall(false, 0, 20, STR_TAKEWHILE_CLOSELID, Alignment::center);
								}
								printLarge(false, 0, (ms().theme == 4 ? 80 : 88), (orgsavesize == 0) ? STR_CREATING_SAVE : STR_EXPANDING_SAVE, Alignment::center);
								updateText(false);

								if (ms().theme != 4 && ms().theme != 5) {
									fadeSpeed = true; // Fast fading
									fadeType = true; // Fade in from white
								}
								showProgressIcon = true;

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
								showProgressIcon = false;
								clearText();
								printLarge(false, 0, (ms().theme == 4 ? 32 : 88), (orgsavesize == 0) ? STR_SAVE_CREATED : STR_SAVE_EXPANDED, Alignment::center);
								updateText(false);
								for (int i = 0; i < 30; i++) {
									swiWaitForVBlank();
								}
								if (ms().theme != 4 && ms().theme != 5) {
									fadeType = false;	   // Fade to white
									for (int i = 0; i < 25; i++) {
										swiWaitForVBlank();
									}
								}
								clearText();
								updateText(false);
								if (ms().theme == 5) displayGameIcons = true;
							}
						}

						int donorSdkVer = SetDonorSDK();
						SetMPUSettings(argarray[0]);
						SetSpeedBumpExclude();

						bool useWidescreen = (perGameSettings_wideScreen == -1 ? ms().wideScreen : perGameSettings_wideScreen);

						bootstrapinipath = ((!ms().secondaryDevice || (isDSiMode() && sdFound())) ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini");
						CIniFile bootstrapini(bootstrapinipath);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", path);
						bootstrapini.SetString("NDS-BOOTSTRAP", "SAV_PATH", savepath);
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", (useWidescreen && (gameTid[CURPOS][0] == 'W' || romVersion[CURPOS] == 0x57)) ? "wide" : "");
						if (!isHomebrew[CURPOS]) {
							bootstrapini.SetString("NDS-BOOTSTRAP", "AP_FIX_PATH", setApFix(argarray[0]));
						}
						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", (perGameSettings_ramDiskNo >= 0 && !ms().secondaryDevice) ? ramdiskpath : "sd:/null.img");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", perGameSettings_language == -2 ? ms().gameLanguage : perGameSettings_language);
						if (isDSiMode() || !ms().secondaryDevice) {
							bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", perGameSettings_dsiMode == -1 ? ms().bstrap_dsiMode : perGameSettings_dsiMode);
						}
						if ((REG_SCFG_EXT != 0) || !ms().secondaryDevice) {
							bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu);
							bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram);
						}
						bootstrapini.SetInt("NDS-BOOTSTRAP", "EXTENDED_MEMORY", perGameSettings_expandRomSpace == -1 ? ms().extendedMemory : perGameSettings_expandRomSpace);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DONOR_SDK_VER", donorSdkVer);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_REGION", mpuregion);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", mpusize);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "CARDENGINE_CACHED", ceCached);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", 
							(ms().forceSleepPatch
						|| (memcmp(io_dldi_data->friendlyName, "TTCARD", 6) == 0 && !sys().isRegularDS())
						|| (memcmp(io_dldi_data->friendlyName, "DSTT", 4) == 0 && !sys().isRegularDS())
						|| (memcmp(io_dldi_data->friendlyName, "DEMON", 5) == 0 && !sys().isRegularDS())
						|| (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0 && !sys().isRegularDS()))
						);
						if (!isDSiMode() && ms().secondaryDevice && sdFound()) {
							CIniFile bootstrapiniSD("sd:/_nds/nds-bootstrap.ini");
							bootstrapini.SetInt("NDS-BOOTSTRAP", "DEBUG", bootstrapiniSD.GetInt("NDS-BOOTSTRAP", "DEBUG", 0));
							bootstrapini.SetInt("NDS-BOOTSTRAP", "LOGGING", bootstrapiniSD.GetInt("NDS-BOOTSTRAP", "LOGGING", 0)); 
						}
						bootstrapini.SaveIniFile(bootstrapinipath);

						CheatCodelist codelist;
						u32 gameCode, crc32;

						if ((isDSiMode() || !ms().secondaryDevice) && !isHomebrew[CURPOS]) {
							bool cheatsEnabled = true;
							const char* cheatDataBin = "/_nds/nds-bootstrap/cheatData.bin";
							mkdir("/_nds", 0777);
							mkdir("/_nds/nds-bootstrap", 0777);
							if(codelist.romData(path,gameCode,crc32)) {
								long cheatOffset; size_t cheatSize;
								FILE* dat=fopen(sdFound() ? "sd:/_nds/TWiLightMenu/extras/usrcheat.dat" : "fat:/_nds/TWiLightMenu/extras/usrcheat.dat","rb");
								if (dat) {
									if (codelist.searchCheatData(dat, gameCode, crc32, cheatOffset, cheatSize)) {
										codelist.parse(path);
										writeCheatsToFile(codelist.getCheats(), cheatDataBin);
										FILE* cheatData=fopen(cheatDataBin,"rb");
										if (cheatData) {
											u32 check[2];
											fread(check, 1, 8, cheatData);
											fclose(cheatData);
											if (check[1] == 0xCF000000 || getFileSize(cheatDataBin) > 0x8000) {
												cheatsEnabled = false;
											}
										}
									} else {
										cheatsEnabled = false;
									}
									fclose(dat);
								} else {
									cheatsEnabled = false;
								}
							} else {
								cheatsEnabled = false;
							}
							if (!cheatsEnabled) {
								remove(cheatDataBin);
							}
						}

						if (!isArgv) {
							ms().romPath[ms().secondaryDevice] = std::string(argarray[0]);
						}
						ms().homebrewHasWide = (isHomebrew[CURPOS] && gameTid[CURPOS][0] == 'W');
						ms().launchType[ms().secondaryDevice] = Launch::ESDFlashcardLaunch; // 1
						ms().previousUsedDevice = ms().secondaryDevice;
						ms().saveSettings();

						if (ms().theme == 5) {
							fadeType = false;		  // Fade to black
							for (int i = 0; i < 60; i++) {
								swiWaitForVBlank();
							}
						}

						if (isDSiMode() || !ms().secondaryDevice) {
							SetWidescreen(filename.c_str());
						}
						if (!isDSiMode() && !ms().secondaryDevice) {
							ntrStartSdGame();
						}

						bool useNightly = (perGameSettings_bootstrapFile == -1 ? ms().bootstrapFile : perGameSettings_bootstrapFile);

						char ndsToBoot[256];
						sprintf(ndsToBoot, "sd:/_nds/nds-bootstrap-%s%s.nds", ms().homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							sprintf(ndsToBoot, "fat:/_nds/%s-%s%s.nds", isDSiMode() ? "nds-bootstrap" : "b4ds", ms().homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
						}

						argarray.at(0) = (char *)ndsToBoot;
						snd().stopStream();
						int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], (ms().homebrewBootstrap ? false : true), true, false, true, true);
						char text[64];
						snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
						clearText();
						printLarge(false, 4, 4, text);
						if (err == 1) {
							printLarge(false, 4, 20, useNightly ? STR_BOOTSTRAP_NIGHTLY_NOT_FOUND : STR_BOOTSTRAP_RELEASE_NOT_FOUND);
						}
						printSmall(false, 4, 20 + calcLargeFontHeight(useNightly ? STR_BOOTSTRAP_NIGHTLY_NOT_FOUND : STR_BOOTSTRAP_RELEASE_NOT_FOUND), STR_PRESS_B_RETURN);
						updateText(false);
						fadeSpeed = true;
						fadeType = true;
						int pressed = 0;
						do {
							scanKeys();
							pressed = keysDownRepeat();
							checkSdEject();
							swiWaitForVBlank();
						} while (!(pressed & KEY_B));
						fadeType = false;	// Fade to white
						for (int i = 0; i < 25; i++) {
							swiWaitForVBlank();
						}
						if (!isDSiMode()) {
							chdir("fat:/");
						} else if (sdFound()) {
							chdir("sd:/");
						}
						runNdsFile("/_nds/TWiLightMenu/dsimenu.srldr", 0, NULL, true, false, false, true, true);
						stop();
					} else {
						ms().romPath[ms().secondaryDevice] = std::string(argarray[0]);
						ms().launchType[ms().secondaryDevice] = Launch::ESDFlashcardLaunch;
						ms().previousUsedDevice = ms().secondaryDevice;
						ms().saveSettings();

						if (ms().theme == 5) {
							fadeType = false;		  // Fade to black
							for (int i = 0; i < 60; i++) {
								swiWaitForVBlank();
							}
						}

						loadGameOnFlashcard(argarray[0], true);
					}
				} else {
					if (!isArgv) {
						ms().romPath[ms().secondaryDevice] = std::string(argarray[0]);
					}
					ms().homebrewHasWide = (isHomebrew[CURPOS] && (gameTid[CURPOS][0] == 'W' || romVersion[CURPOS] == 0x57));
					ms().launchType[ms().secondaryDevice] = Launch::ESDFlashcardDirectLaunch;
					ms().previousUsedDevice = ms().secondaryDevice;
					if (isDSiMode() || !ms().secondaryDevice) {
						SetWidescreen(filename.c_str());
					}
					ms().saveSettings();

					if (ms().theme == 5) {
						fadeType = false;		  // Fade to black
						for (int i = 0; i < 60; i++) {
							swiWaitForVBlank();
						}
					}

					if (!isDSiMode() && !ms().secondaryDevice) {
						ntrStartSdGame();
					}

					bool useWidescreen = (perGameSettings_wideScreen == -1 ? ms().wideScreen : perGameSettings_wideScreen);

					if (ms().consoleModel >= 2 && useWidescreen && ms().homebrewHasWide) {
						argarray.push_back((char*)"wide");
					}

					bool runNds_boostCpu = false;
					bool runNds_boostVram = false;
					if (REG_SCFG_EXT != 0 && !dsModeDSiWare) {
						loadPerGameSettings(filename);

						runNds_boostCpu = perGameSettings_boostCpu == -1 ? ms().boostCpu : perGameSettings_boostCpu;
						runNds_boostVram = perGameSettings_boostVram == -1 ? ms().boostVram : perGameSettings_boostVram;
					}
					snd().stopStream();
					int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, dsModeSwitch, runNds_boostCpu, runNds_boostVram);
					char text[64];
					snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
					printLarge(false, 4, 4, text);
					printSmall(false, 4, 20, STR_PRESS_B_RETURN);
					updateText(false);
					fadeSpeed = true;
					fadeType = true;
					int pressed = 0;
					do {
						scanKeys();
						pressed = keysDownRepeat();
						checkSdEject();
						swiWaitForVBlank();
					} while (!(pressed & KEY_B));
					fadeType = false;	// Fade to white
					for (int i = 0; i < 25; i++) {
						swiWaitForVBlank();
					}
					if (!isDSiMode()) {
						chdir("fat:/");
					} else if (sdFound()) {
						chdir("sd:/");
					}
					runNdsFile("/_nds/TWiLightMenu/dsimenu.srldr", 0, NULL, true, false, false, true, true);
					stop();
				}
			} else {
				bool useNDSB = false;
				bool dsModeSwitch = false;
				bool boostCpu = true;
				bool boostVram = false;

				std::string romfolderNoSlash = ms().romfolder[ms().secondaryDevice];
				RemoveTrailingSlashes(romfolderNoSlash);
				char ROMpath[256];
				snprintf (ROMpath, sizeof(ROMpath), "%s/%s", romfolderNoSlash.c_str(), filename.c_str());
				ms().romPath[ms().secondaryDevice] = std::string(ROMpath);
				ms().previousUsedDevice = ms().secondaryDevice;
				ms().homebrewBootstrap = true;

				const char *ndsToBoot = "sd:/_nds/nds-bootstrap-release.nds";
				if (extention(filename, ".plg")) {
					ndsToBoot = "fat:/_nds/TWiLightMenu/bootplg.srldr";
					dsModeSwitch = true;

					// Print .plg path without "fat:" at the beginning
					char ROMpathDS2[256];
					if (ms().secondaryDevice) {
						for (int i = 0; i < 252; i++) {
							ROMpathDS2[i] = ROMpath[4+i];
							if (ROMpath[4+i] == '\x00') break;
						}
					} else {
						sprintf(ROMpathDS2, "/_nds/TWiLightMenu/tempPlugin.plg");
						fcopy(ROMpath, "fat:/_nds/TWiLightMenu/tempPlugin.plg");
					}

					CIniFile dstwobootini( "fat:/_dstwo/twlm.ini" );
					dstwobootini.SetString("boot_settings", "file", ROMpathDS2);
					dstwobootini.SaveIniFile( "fat:/_dstwo/twlm.ini" );
				} else if (extention(filename, ".rvid")) {
					ms().launchType[ms().secondaryDevice] = Launch::ERVideoLaunch;

					ndsToBoot = "sd:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
						boostVram = true;
					}
				} else if (extention(filename, ".mp4")) {
					ms().launchType[ms().secondaryDevice] = Launch::EMPEG4Launch;

					ndsToBoot = "sd:/_nds/TWiLightMenu/apps/MPEG4Player.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/apps/MPEG4Player.nds";
						boostVram = true;
					}
				} else if (extention(filename, ".gba")) {
					ms().launchType[ms().secondaryDevice] = (ms().showGba == 1) ? Launch::EGBANativeLaunch : Launch::ESDFlashcardLaunch;

					if (ms().showGba == 1) {
						if (ms().theme == 5) displayGameIcons = false;
						printSmall(false, 0, 20, STR_BARSTOPPED_CLOSELID, Alignment::center);
						printLarge(false, 0, (ms().theme == 4 ? 80 : 88), STR_NOW_LOADING, Alignment::center);
						updateText(false);

						if (ms().theme != 4 && ms().theme != 5) {
							fadeSpeed = true; // Fast fading
							fadeType = true; // Fade in from white
						}
						showProgressIcon = true;
						showProgressBar = true;
						progressBarLength = 0;

						u32 ptr = 0x08000000;
						extern char copyBuf[0x8000];
						u32 romSize = getFileSize(filename.c_str());
						if (romSize > 0x2000000) romSize = 0x2000000;

						bool nor = false;
						if (*(u16*)(0x020000C0) == 0x5A45) {
							cExpansion::SetRompage(0);
							expansion().SetRampage(cExpansion::ENorPage);
							cExpansion::OpenNorWrite();
							cExpansion::SetSerialMode();
							for(u32 address=0;address<romSize&&address<0x2000000;address+=0x40000)
							{
								expansion().Block_Erase(address);
							}
							nor = true;
						} else if (*(u16*)(0x020000C0) == 0x4353 && romSize > 0x1FFFFFE) {
							romSize = 0x1FFFFFE;
						}

						FILE* gbaFile = fopen(filename.c_str(), "rb");
						for (u32 len = romSize; len > 0; len -= 0x8000) {
							if (fread(&copyBuf, 1, (len>0x8000 ? 0x8000 : len), gbaFile) > 0) {
								s2RamAccess(true);
								if (nor) {
									expansion().WriteNorFlash(ptr-0x08000000, (u8*)copyBuf, (len>0x8000 ? 0x8000 : len));
								} else {
									tonccpy((u16*)ptr, &copyBuf, (len>0x8000 ? 0x8000 : len));
								}
								s2RamAccess(false);
								ptr += 0x8000;
								progressBarLength = ((ptr-0x08000000)+0x8000)/(romSize/192);
							} else {
								break;
							}
						}
						fclose(gbaFile);

						if (*(u16*)(0x020000C0) == 0x5A45) {
							expansion().SetRampage(0);	// Switch to GBA SRAM for EZ Flash
						}

						ptr = 0x0A000000;

						std::string savename = replaceAll(filename, ".gba", ".sav");
						u32 savesize = getFileSize(savename.c_str());
						if (savesize > 0x10000) savesize = 0x10000;

						if (savesize > 0) {
							FILE* savFile = fopen(savename.c_str(), "rb");
							for (u32 len = savesize; len > 0; len -= 0x8000) {
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

						showProgressIcon = false;
						if (ms().theme != 4 && ms().theme != 5) {
							fadeType = false;	   // Fade to white
							for (int i = 0; i < 25; i++) {
								swiWaitForVBlank();
							}
						}
						clearText();
						updateText(false);
						if (ms().theme == 5) displayGameIcons = true;

						ndsToBoot = "fat:/_nds/TWiLightMenu/gbapatcher.srldr";
					} else if (ms().secondaryDevice) {
						ndsToBoot = ms().gbar2DldiAccess ? "sd:/_nds/GBARunner2_arm7dldi_ds.nds" : "sd:/_nds/GBARunner2_arm9dldi_ds.nds";
						if (REG_SCFG_EXT != 0) {
							ndsToBoot = ms().consoleModel > 0 ? "sd:/_nds/GBARunner2_arm7dldi_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_dsi.nds";
						}
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = ms().gbar2DldiAccess ? "fat:/_nds/GBARunner2_arm7dldi_ds.nds" : "fat:/_nds/GBARunner2_arm9dldi_ds.nds";
							if (REG_SCFG_EXT != 0) {
								ndsToBoot = ms().consoleModel > 0 ? "fat:/_nds/GBARunner2_arm7dldi_3ds.nds" : "fat:/_nds/GBARunner2_arm7dldi_dsi.nds";
							}
						}
						boostVram = false;
					} else {
						useNDSB = true;

						const char* gbar2Path = ms().consoleModel>0 ? "sd:/_nds/GBARunner2_arm7dldi_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_dsi.nds";
						if (isDSiMode() && sys().arm7SCFGLocked()) {
							gbar2Path = ms().consoleModel > 0 ? "sd:/_nds/GBARunner2_arm7dldi_nodsp_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_nodsp_dsi.nds";
						}

						ndsToBoot = (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", gbar2Path);
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", ROMpath);
						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", "");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				} else if (extention(filename, ".a26")) {
					ms().launchType[ms().secondaryDevice] = Launch::EStellaDSLaunch;

					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/StellaDS.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/StellaDS.nds";
						boostVram = true;
					}
				} else if (extention(filename, ".a78")) {
					ms().launchType[ms().secondaryDevice] = Launch::EA7800DSLaunch;

					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/A7800DS.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/A7800DS.nds";
						boostVram = true;
					}
				} else if (extention(filename, ".gb") || extention(filename, ".sgb") || extention(filename, ".gbc")) {
					ms().launchType[ms().secondaryDevice] = Launch::EGameYobLaunch;

					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/gameyob.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/gameyob.nds";
						dsModeSwitch = !isDSiMode();
						boostVram = true;
					}
				} else if (extention(filename, ".nes") || extention(filename, ".fds")) {
					ms().launchType[ms().secondaryDevice] = Launch::ENESDSLaunch;

					ndsToBoot = (ms().secondaryDevice ? "sd:/_nds/TWiLightMenu/emulators/nesds.nds" : "sd:/_nds/TWiLightMenu/emulators/nestwl.nds");
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/nesds.nds";
						boostVram = true;
					}
				} else if (extention(filename, ".sms") || extention(filename, ".gg")) {
					mkdir(ms().secondaryDevice ? "fat:/data" : "sd:/data", 0777);
					mkdir(ms().secondaryDevice ? "fat:/data/s8ds" : "sd:/data/s8ds", 0777);

					if (!ms().secondaryDevice && !sys().arm7SCFGLocked() && ms().smsGgInRam) {
						ms().launchType[ms().secondaryDevice] = Launch::ESDFlashcardLaunch;

						useNDSB = true;

						ndsToBoot = (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/S8DS07.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", ROMpath);
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					} else {
						ms().launchType[ms().secondaryDevice] = Launch::ES8DSLaunch;

						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/S8DS.nds";
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/S8DS.nds";
							boostVram = true;
						}
					}
				} else if (extention(filename, ".gen")) {
					bool usePicoDrive = ((isDSiMode() && sdFound() && sys().arm7SCFGLocked())
						|| ms().showMd==2 || (ms().showMd==3 && getFileSize(filename.c_str()) > 0x300000));
					ms().launchType[ms().secondaryDevice] = (usePicoDrive ? Launch::EPicoDriveTWLLaunch : Launch::ESDFlashcardLaunch);

					if (usePicoDrive || ms().secondaryDevice) {
						ndsToBoot = usePicoDrive ? "sd:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds" : "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds";
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = usePicoDrive ? "fat:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds" : "fat:/_nds/TWiLightMenu/emulators/jEnesisDS.nds";
							boostVram = true;
						}
						dsModeSwitch = !usePicoDrive;
					} else {
						useNDSB = true;

						ndsToBoot = (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/ROM.BIN");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", ROMpath);
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				} else if (extention(filename, ".smc") || extention(filename, ".sfc")) {
					ms().launchType[ms().secondaryDevice] = Launch::ESDFlashcardLaunch;

					if (ms().secondaryDevice) {
						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.nds";
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/SNEmulDS.nds";
							boostCpu = false;
							boostVram = true;
						}
						dsModeSwitch = true;
					} else {
						useNDSB = true;

						ndsToBoot = (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/snes/ROM.SMC");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", ROMpath);
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				} else if (extention(filename, ".pce")) {
					ms().launchType[ms().secondaryDevice] = Launch::ESDFlashcardLaunch;

					if (ms().secondaryDevice) {
						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/NitroGrafx.nds";
						if(access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "/_nds/TWiLightMenu/emulators/NitroGrafx.nds";
							boostVram = true;
						}
						dsModeSwitch = true;
					} else {
						useNDSB = true;

						ndsToBoot = (ms().bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", ms().gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/NitroGrafx.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", ROMpath);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", "");
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				}

				ms().saveSettings();
				if (!isDSiMode() && !ms().secondaryDevice && !extention(filename, ".plg")) {
					ntrStartSdGame();
				}

				argarray.push_back(ROMpath);
				argarray.at(0) = (char *)ndsToBoot;
				snd().stopStream();

				int err = runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], !useNDSB, true, dsModeSwitch, boostCpu, boostVram);	// Pass ROM to emulator as argument

				char text[64];
				snprintf(text, sizeof(text), STR_START_FAILED_ERROR.c_str(), err);
				printLarge(false, 4, 4, text);
				if (err == 1 && useNDSB) {
					printLarge(false, 4, 20, ms().bootstrapFile ? STR_BOOTSTRAP_NIGHTLY_NOT_FOUND : STR_BOOTSTRAP_RELEASE_NOT_FOUND);
				}
				printSmall(false, 4, 20 + calcLargeFontHeight(ms().bootstrapFile ? STR_BOOTSTRAP_NIGHTLY_NOT_FOUND : STR_BOOTSTRAP_RELEASE_NOT_FOUND), STR_PRESS_B_RETURN);
				updateText(false);
				fadeSpeed = true;
				fadeType = true;
				int pressed = 0;
				do {
					scanKeys();
					pressed = keysDownRepeat();
					checkSdEject();
					swiWaitForVBlank();
				} while (!(pressed & KEY_B));
				fadeType = false;	// Fade to white
				for (int i = 0; i < 25; i++) {
					swiWaitForVBlank();
				}
				if (!isDSiMode()) {
					chdir("fat:/");
				} else if (sdFound()) {
					chdir("sd:/");
				}
				runNdsFile("/_nds/TWiLightMenu/dsimenu.srldr", 0, NULL, true, false, false, true, true);
				stop();
			}

			while (argarray.size() != 0) {
				free(argarray.at(0));
				argarray.erase(argarray.begin());
			}
		}
	}

	return 0;
}
