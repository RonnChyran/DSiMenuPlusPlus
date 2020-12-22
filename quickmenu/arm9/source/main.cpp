#include <nds.h>
#include <nds/arm9/dldi.h>
#include "io_m3_common.h"
#include "io_g6_common.h"
#include "io_sc_common.h"
#include "exptools.h"

#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>
#include <gl2d.h>
#include <maxmod9.h>

#include "date.h"
#include "fileCopy.h"
#include "nand/nandio.h"

#include "graphics/graphics.h"

#include "common/tonccpy.h"
#include "common/nitrofs.h"
#include "read_card.h"
#include "flashcard.h"
#include "ndsheaderbanner.h"
#include "gbaswitch.h"
#include "nds_loader_arm9.h"
#include "perGameSettings.h"
#include "errorScreen.h"

#include "iconTitle.h"
#include "graphics/fontHandler.h"

#include "common/inifile.h"
#include "tool/stringtool.h"

#include "language.h"

#include "cheat.h"
#include "crc.h"

#include "soundbank.h"
#include "soundbank_bin.h"

#include "donorMap.h"
#include "mpuMap.h"
#include "speedBumpExcludeMap.h"
#include "saveMap.h"

#include "sr_data_srllastran.h"	// For rebooting into the game

#define gbamodeText "Start GBA game."
#define featureUnavailableText "This feature is unavailable."

bool useTwlCfg = false;

bool whiteScreen = true;
bool fadeType = false;		// false = out, true = in
bool fadeSpeed = true;		// false = slow (for DSi launch effect), true = fast
bool controlTopBright = true;
bool controlBottomBright = true;
bool widescreenEffects = false;
int fps = 60;
int colorMode = 0;
int blfLevel = 0;

extern bool showProgressBar;
extern int progressBarLength;

bool cardEjected = false;
static bool cardRefreshed = false;

extern void ClearBrightness();
extern int boxArtType[2];

const char* settingsinipath = "sd:/_nds/TWiLightMenu/settings.ini";
const char* bootstrapinipath = "sd:/_nds/nds-bootstrap.ini";

std::string romPath[2];
std::string dsiWareSrlPath;
std::string dsiWarePubPath;
std::string dsiWarePrvPath;

const char *charUnlaunchBg;
std::string gbaBorder = "default.png";
std::string unlaunchBg = "default.gif";
bool removeLauncherPatches = true;

const char *unlaunchAutoLoadID = "AutoLoadInfo";
static char hiyaNdsPath[14] = {'s','d','m','c',':','/','h','i','y','a','.','d','s','i'};
char unlaunchDevicePath[256];

static char pictochatPath[256];
static char dlplayPath[256];

bool arm7SCFGLocked = false;
int consoleModel = 0;
/*	0 = Nintendo DSi (Retail)
	1 = Nintendo DSi (Dev/Panda)
	2 = Nintendo 3DS
	3 = New Nintendo 3DS	*/
bool isRegularDS = true;
bool isDSLite = false;

extern bool showdialogbox;

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
void RemoveTrailingSlashes(std::string& path)
{
	while (!path.empty() && path[path.size()-1] == '/') {
		path.resize(path.size()-1);
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

std::string romfolder[2];

// These are used by flashcard functions and must retain their trailing slash.
static const std::string slashchar = "/";
static const std::string woodfat = "fat0:/";
static const std::string dstwofat = "fat1:/";

int mpuregion = 0;
int mpusize = 0;
bool ceCached = true;

bool applaunch = false;
bool showCursor = true;
bool startMenu = false;
bool gotosettings = false;

bool slot1Launched = false;
int launchType[2] = {0};	// 0 = Slot-1, 1 = SD/Flash card, 2 = SD/Flash card (Direct boot), 3 = DSiWare, 4 = NES, 5 = (S)GB(C), 6 = SMS/GG
int slot1LaunchMethod = 1;	// 0 == Reboot, 1 == Direct, 2 == Unlaunch
bool useBootstrap = true;
bool bootstrapFile = false;
bool homebrewBootstrap = false;
bool homebrewHasWide = false;
bool show12hrClock = true;
//bool snesEmulator = true;
bool smsGgInRam = false;
bool fcSaveOnSd = false;
bool wideScreen = false;

bool pictochatFound = false;
bool dlplayFound = false;
bool pictochatReboot = false;
bool dlplayReboot = false;
bool sdRemoveDetect = true;
bool gbar2DldiAccess = false;	// false == ARM9, true == ARM7
int theme = 0;
int subtheme = 0;
int showGba = 2;
int showMd = 3;
int cursorPosition[2] = {0};
int startMenu_cursorPosition = 0;
int pagenum[2] = {0};
bool showDirectories = true;
int showBoxArt = 1 + isDSiMode();
bool animateDsiIcons = false;
int launcherApp = -1;
int sysRegion = -1;

int guiLanguage = -1;
int gameLanguage = -1;
int titleLanguage = -1;
bool boostCpu = false;	// false == NTR, true == TWL
bool boostVram = false;
int bstrap_dsiMode = 0;
int bstrap_extendedMemory = 0;
bool forceSleepPatch = false;
bool dsiWareBooter = false;

void LoadSettings(void) {
	useBootstrap = isDSiMode();
	showGba = 1 + isDSiMode();

	// GUI
	CIniFile settingsini( settingsinipath );

	// UI settings.
	consoleModel = settingsini.GetInt("SRLOADER", "CONSOLE_MODEL", 0);

	showGba = settingsini.GetInt("SRLOADER", "SHOW_GBA", showGba);
	if (!isRegularDS && showGba != 0) {
		showGba = 2;
	}
	showMd = settingsini.GetInt("SRLOADER", "SHOW_MDGEN", showMd);

	// Customizable UI settings.
	fps = settingsini.GetInt("SRLOADER", "FRAME_RATE", fps);
	colorMode = settingsini.GetInt("SRLOADER", "COLOR_MODE", 0);
	blfLevel = settingsini.GetInt("SRLOADER", "BLUE_LIGHT_FILTER_LEVEL", 0);
	guiLanguage = settingsini.GetInt("SRLOADER", "LANGUAGE", -1);
	titleLanguage = settingsini.GetInt("SRLOADER", "TITLELANGUAGE", titleLanguage);
	sdRemoveDetect = settingsini.GetInt("SRLOADER", "SD_REMOVE_DETECT", 1);
	gbar2DldiAccess = settingsini.GetInt("SRLOADER", "GBAR2_DLDI_ACCESS", gbar2DldiAccess);
	theme = settingsini.GetInt("SRLOADER", "THEME", 0);
	subtheme = settingsini.GetInt("SRLOADER", "SUB_THEME", 0);
	showDirectories = settingsini.GetInt("SRLOADER", "SHOW_DIRECTORIES", 1);
	showBoxArt = settingsini.GetInt("SRLOADER", "SHOW_BOX_ART", showBoxArt);
	animateDsiIcons = settingsini.GetInt("SRLOADER", "ANIMATE_DSI_ICONS", 0);
	if (consoleModel < 2) {
		launcherApp = settingsini.GetInt("SRLOADER", "LAUNCHER_APP", launcherApp);
	}

	previousUsedDevice = settingsini.GetInt("SRLOADER", "PREVIOUS_USED_DEVICE", previousUsedDevice);
	secondaryDevice = bothSDandFlashcard() ? settingsini.GetInt("SRLOADER", "SECONDARY_DEVICE", secondaryDevice) : flashcardFound();
	fcSaveOnSd = settingsini.GetInt("SRLOADER", "FC_SAVE_ON_SD", fcSaveOnSd);

	slot1LaunchMethod = settingsini.GetInt("SRLOADER", "SLOT1_LAUNCHMETHOD", slot1LaunchMethod);
	useBootstrap = settingsini.GetInt("SRLOADER", "USE_BOOTSTRAP", useBootstrap);
	if (isRegularDS && (io_dldi_data->ioInterface.features & FEATURE_SLOT_GBA)) {
		useBootstrap = true;
	}
	bootstrapFile = settingsini.GetInt("SRLOADER", "BOOTSTRAP_FILE", 0);
    //snesEmulator = settingsini.GetInt("SRLOADER", "SNES_EMULATOR", snesEmulator);
    smsGgInRam = settingsini.GetInt("SRLOADER", "SMS_GG_IN_RAM", smsGgInRam);

    show12hrClock = settingsini.GetInt("SRLOADER", "SHOW_12H_CLOCK", show12hrClock);

    gbaBorder = settingsini.GetString("SRLOADER", "GBA_BORDER", gbaBorder);
    unlaunchBg = settingsini.GetString("SRLOADER", "UNLAUNCH_BG", unlaunchBg);
	charUnlaunchBg = unlaunchBg.c_str();
	removeLauncherPatches = settingsini.GetInt("SRLOADER", "UNLAUNCH_PATCH_REMOVE", removeLauncherPatches);

	// Default nds-bootstrap settings
	gameLanguage = settingsini.GetInt("NDS-BOOTSTRAP", "LANGUAGE", -1);
	boostCpu = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
	boostVram = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);
	bstrap_dsiMode = settingsini.GetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
	bstrap_extendedMemory = settingsini.GetInt("NDS-BOOTSTRAP", "EXTENDED_MEMORY", 0);

	forceSleepPatch = settingsini.GetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", 0);
	dsiWareBooter = settingsini.GetInt("SRLOADER", "DSIWARE_BOOTER", dsiWareBooter);

	dsiWareSrlPath = settingsini.GetString("SRLOADER", "DSIWARE_SRL", dsiWareSrlPath);
	dsiWarePubPath = settingsini.GetString("SRLOADER", "DSIWARE_PUB", dsiWarePubPath);
	dsiWarePrvPath = settingsini.GetString("SRLOADER", "DSIWARE_PRV", dsiWarePrvPath);
	slot1Launched = settingsini.GetInt("SRLOADER", "SLOT1_LAUNCHED", slot1Launched);
	launchType[0] = settingsini.GetInt("SRLOADER", "LAUNCH_TYPE", launchType[0]);
	launchType[1] = settingsini.GetInt("SRLOADER", "SECONDARY_LAUNCH_TYPE", launchType[1]);
	romPath[0] = settingsini.GetString("SRLOADER", "ROM_PATH", romPath[0]);
	romPath[1] = settingsini.GetString("SRLOADER", "SECONDARY_ROM_PATH", romPath[1]);

	wideScreen = settingsini.GetInt("SRLOADER", "WIDESCREEN", wideScreen);
}

void SaveSettings(void) {
	// GUI
	CIniFile settingsini( settingsinipath );

	// UI settings.
	if (!gotosettings) {
		settingsini.SetString("SRLOADER", "DSIWARE_SRL", dsiWareSrlPath);
		settingsini.SetString("SRLOADER", "DSIWARE_PUB", dsiWarePubPath);
		settingsini.SetString("SRLOADER", "DSIWARE_PRV", dsiWarePrvPath);
		settingsini.SetInt("SRLOADER", "SLOT1_LAUNCHED", slot1Launched);
		settingsini.SetInt("SRLOADER", "LAUNCH_TYPE", launchType[0]);
		settingsini.SetInt("SRLOADER", "SECONDARY_LAUNCH_TYPE", launchType[1]);
		settingsini.SetInt("SRLOADER", "HOMEBREW_BOOTSTRAP", homebrewBootstrap);
		settingsini.SetInt("SRLOADER", "HOMEBREW_HAS_WIDE", homebrewHasWide);
	}
	//settingsini.SetInt("SRLOADER", "THEME", theme);
	//settingsini.SetInt("SRLOADER", "SUB_THEME", subtheme);
	settingsini.SaveIniFile(settingsinipath);
}

bool isDSPhat(void) {
	return (isRegularDS && !isDSLite);
}

bool useBackend = false;

using namespace std;

bool showbubble = false;
bool showSTARTborder = false;

bool titleboxXmoveleft = false;
bool titleboxXmoveright = false;

bool applaunchprep = false;

int spawnedtitleboxes = 0;

//char usernameRendered[10];
//bool usernameRenderedDone = false;

touchPosition touch;

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

/**
 * Set donor SDK version for a specific game.
 */
int SetDonorSDK(const char* filename) {
	FILE *f_nds_file = fopen(filename, "rb");

	char game_TID[5];
	grabTID(f_nds_file, game_TID);
	fclose(f_nds_file);
	game_TID[4] = 0;
	game_TID[3] = 0;

	for (auto i : donorMap) {
		if (i.first == 5 && game_TID[0] == 'V')
			return 5;

		if (i.second.find(game_TID) != i.second.cend())
			return i.first;
	}

	return 0;
}

/**
 * Set MPU settings for a specific game.
 */
void SetMPUSettings(const char* filename) {
	FILE *f_nds_file = fopen(filename, "rb");

	char game_TID[5];
	fseek(f_nds_file, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
	fread(game_TID, 1, 4, f_nds_file);
	game_TID[4] = 0;
	game_TID[3] = 0;
	fclose(f_nds_file);

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

	if(pressed & KEY_RIGHT) {
		mpusize = 3145728;
	} else if(pressed & KEY_LEFT) {
		mpusize = 1;
	} else {
		mpusize = 0;
	}

	// Check for games that need an MPU size of 3 MB.
	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(mpu_3MB_list)/sizeof(mpu_3MB_list[0]); i++) {
		if (!memcmp(game_TID, mpu_3MB_list[i], 3)) {
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
void SetSpeedBumpExclude(const char* filename) {
	if (!isDSiMode() || (perGameSettings_heapShrink >= 0 && perGameSettings_heapShrink < 2)) {
		ceCached = perGameSettings_heapShrink;
		return;
	}

	FILE *f_nds_file = fopen(filename, "rb");

	char game_TID[5];
	fseek(f_nds_file, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
	fread(game_TID, 1, 4, f_nds_file);
	fclose(f_nds_file);

	// TODO: If the list gets large enough, switch to bsearch().
	for (unsigned int i = 0; i < sizeof(sbeList2)/sizeof(sbeList2[0]); i++) {
		if (memcmp(game_TID, sbeList2[i], 3) == 0) {
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
		FILE *f_nds_file = fopen(filename, "rb");

		char game_TID[5];
		u16 headerCRC16 = 0;
		fseek(f_nds_file, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
		fread(game_TID, 1, 4, f_nds_file);
		fseek(f_nds_file, offsetof(sNDSHeaderExt, headerCRC16), SEEK_SET);
		fread(&headerCRC16, sizeof(u16), 1, f_nds_file);
		fclose(f_nds_file);
		game_TID[4] = 0;

		snprintf(ipsPath, sizeof(ipsPath), "%s:/_nds/TWiLightMenu/apfix/%s-%X.ips", sdFound() ? "sd" : "fat", game_TID, headerCRC16);
		ipsFound = (access(ipsPath, F_OK) == 0);
	}

	if (ipsFound) {
		if (secondaryDevice && sdFound()) {
			mkdir("fat:/_nds", 0777);
			mkdir("fat:/_nds/nds-bootstrap", 0777);
			fcopy(ipsPath, "fat:/_nds/nds-bootstrap/apFix.ips");
			return "fat:/_nds/nds-bootstrap/apFix.ips";
		}
		return ipsPath;
	}

	return "";
}

/**
 * Enable widescreen for some games.
 */
void SetWidescreen(const char *filename) {
	remove("/_nds/nds-bootstrap/wideCheatData.bin");

	bool useWidescreen = (perGameSettings_wideScreen == -1 ? wideScreen : perGameSettings_wideScreen);

	if ((isDSiMode() && arm7SCFGLocked) || consoleModel < 2 || !useWidescreen
	|| (access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) != 0)) {
		homebrewHasWide = false;
		return;
	}
	
	bool wideCheatFound = false;
	char wideBinPath[256];
	if (launchType[secondaryDevice] == 1) {
		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s.bin", filename);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	}

	if (slot1Launched) {
		// Reset Slot-1 to allow reading card header
		sysSetCardOwner (BUS_OWNER_ARM9);
		disableSlot1();
		for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
		enableSlot1();
		for(int i = 0; i < 15; i++) { swiWaitForVBlank(); }

		cardReadHeader((uint8*)&ndsCardHeader);

		char game_TID[5];
		tonccpy(game_TID, ndsCardHeader.gameCode, 4);
		game_TID[4] = 0;

		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s-%X.bin", game_TID, ndsCardHeader.headerCRC16);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	} else if (!wideCheatFound) {
		FILE *f_nds_file = fopen(filename, "rb");

		char game_TID[5];
		u16 headerCRC16 = 0;
		fseek(f_nds_file, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
		fread(game_TID, 1, 4, f_nds_file);
		fseek(f_nds_file, offsetof(sNDSHeaderExt, headerCRC16), SEEK_SET);
		fread(&headerCRC16, sizeof(u16), 1, f_nds_file);
		fclose(f_nds_file);
		game_TID[4] = 0;

		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s-%X.bin", game_TID, headerCRC16);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	}

	if (isHomebrew[secondaryDevice]) {
		FILE *f_nds_file = fopen(filename, "rb");

		char game_TID[5];
		u8 romVersion = 0;
		fseek(f_nds_file, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
		fread(game_TID, 1, 4, f_nds_file);
		fseek(f_nds_file, offsetof(sNDSHeaderExt, romversion), SEEK_SET);
		fread(&romVersion, sizeof(u8), 1, f_nds_file);
		fclose(f_nds_file);
		game_TID[4] = 0;

		homebrewHasWide = (game_TID[0] == 'W' || romVersion == 0x57);
		return;
	}

	if (wideCheatFound) {
		const char* resultText1;
		const char* resultText2;
		mkdir("/_nds", 0777);
		mkdir("/_nds/nds-bootstrap", 0777);
		if (fcopy(wideBinPath, "/_nds/nds-bootstrap/wideCheatData.bin") == 0) {
			return;
		} else {
			resultText1 = "Failed to copy widescreen";
			resultText2 = "code for the game.";
		}
		remove("/_nds/nds-bootstrap/wideCheatData.bin");
		int textXpos[2] = {0};
		textXpos[0] = 72;
		textXpos[1] = 84;
		clearText();
		printSmallCentered(false, textXpos[0], resultText1);
		printSmallCentered(false, textXpos[1], resultText2);
		fadeType = true; // Fade in from white
		for (int i = 0; i < 60 * 3; i++) {
			swiWaitForVBlank(); // Wait 3 seconds
		}
		fadeType = false;	   // Fade to white
		for (int i = 0; i < 25; i++) {
			swiWaitForVBlank();
		}
	}
}

char filePath[PATH_MAX];

//---------------------------------------------------------------------------------
void doPause() {
//---------------------------------------------------------------------------------
	// iprintf("Press start...\n");
	// printSmall(false, x, y, "Press start...");
	while(1) {
		scanKeys();
		if(keysDown() & KEY_START)
			break;
		swiWaitForVBlank();
	}
	scanKeys();
}

mm_sound_effect snd_launch;
mm_sound_effect snd_select;
mm_sound_effect snd_stop;
mm_sound_effect snd_wrong;
mm_sound_effect snd_back;
mm_sound_effect snd_switch;
mm_sound_effect snd_backlight;

void InitSound() {
	mmInitDefaultMem((mm_addr)soundbank_bin);
	
	mmLoadEffect( SFX_LAUNCH );
	mmLoadEffect( SFX_SELECT );
	mmLoadEffect( SFX_STOP );
	mmLoadEffect( SFX_WRONG );
	mmLoadEffect( SFX_BACK );
	mmLoadEffect( SFX_SWITCH );
	mmLoadEffect( SFX_BACKLIGHT );

	snd_launch = {
		{ SFX_LAUNCH } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
	snd_select = {
		{ SFX_SELECT } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
	snd_stop = {
		{ SFX_STOP } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
	snd_wrong = {
		{ SFX_WRONG } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
	snd_back = {
		{ SFX_BACK } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
	snd_switch = {
		{ SFX_SWITCH } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
	snd_backlight = {
		{ SFX_BACKLIGHT } ,			// id
		(int)(1.0f * (1<<10)),	// rate
		0,		// handle
		255,	// volume
		128,	// panning
	};
}

void loadGameOnFlashcard (const char* ndsPath, bool dsGame) {
	bool runNds_boostCpu = false;
	bool runNds_boostVram = false;
	std::string filename = ndsPath;
	const size_t last_slash_idx = filename.find_last_of("/");
	if (std::string::npos != last_slash_idx) {
		filename.erase(0, last_slash_idx + 1);
	}

	loadPerGameSettings(filename);

	if ((REG_SCFG_EXT != 0) && dsGame) {
		runNds_boostCpu = perGameSettings_boostCpu == -1 ? boostCpu : perGameSettings_boostCpu;
		runNds_boostVram = perGameSettings_boostVram == -1 ? boostVram : perGameSettings_boostVram;
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
		std::string romFolderNoSlash = romfolder[true];
		RemoveTrailingSlashes(romFolderNoSlash);
		std::string saveFolder = romFolderNoSlash + "/saves";
		mkdir(saveFolder.c_str(), 0777);
		std::string savepath = romFolderNoSlash + "/saves/" + savename;
		std::string savepathFc = romFolderNoSlash + "/" + savenameFc;
		rename(savepath.c_str(), savepathFc.c_str());
	}

	std::string fcPath;
	int err = 0;
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

	char text[32];
	snprintf(text, sizeof(text), "Start failed. Error %i", err);
	ClearBrightness();
	printSmall(false, 4, 4, text);
	if (err == 0) {
		printSmall(false, 4, 20, "Flashcard may be unsupported.");
		printSmall(false, 4, 52, "Flashcard name:");
		printSmall(false, 4, 68, io_dldi_data->friendlyName);
	}
	stop();
}

void loadROMselect()
{
	if (!isDSiMode()) {
		chdir("fat:/");
	} else if (sdFound()) {
		chdir("sd:/");
	}
	/*if (theme == 3)
	{
		runNdsFile("/_nds/TWiLightMenu/akmenu.srldr", 0, NULL, true, false, false, true, true);
	}
	else */if (theme == 2 || theme == 6)
	{
		runNdsFile("/_nds/TWiLightMenu/r4menu.srldr", 0, NULL, true, false, false, true, true);
	}
	else
	{
		runNdsFile("/_nds/TWiLightMenu/dsimenu.srldr", 0, NULL, true, false, false, true, true);
	}
}

void unlaunchRomBoot(const char* rom) {
	char unlaunchDevicePath[256] = {0};
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
	tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
	*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
	*(u32*)(0x02000810) |= BIT(0);		// Load the title at 2000838h
	*(u32*)(0x02000810) |= BIT(1);		// Use colors 2000814h
	*(u16*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(u16*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)
	toncset((u8*)0x02000818, 0, 0x20+0x208+0x1C0);		// Unlaunch Reserved (zero)
	int i2 = 0;
	for (int i = 0; i < 14; i++) {
		*(u8*)(0x02000838+i2) = hiyaNdsPath[i];		// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
		i2 += 2;
	}
	while (*(u16*)(0x0200080E) == 0) {	// Keep running, so that CRC16 isn't 0
		*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
	}
}

/**
 * Reboot into an SD game when in DS mode.
 */
void ntrStartSdGame(void) {
	if (consoleModel == 0) {
		unlaunchRomBoot("sd:/_nds/TWiLightMenu/resetgame.srldr");
	} else {
		tonccpy((u32 *)0x02000300, sr_data_srllastran, 0x020);
		DC_FlushAll();						// Make reboot not fail
		fifoSendValue32(FIFO_USER_02, 1);
		stop();
	}
}

void dsCardLaunch() {
	*(u32*)(0x02000300) = 0x434E4C54;	// Set "CNLT" warmboot flag
	*(u16*)(0x02000304) = 0x1801;
	*(u32*)(0x02000308) = 0x43415254;	// "CART"
	*(u32*)(0x0200030C) = 0x00000000;
	*(u32*)(0x02000310) = 0x43415254;	// "CART"
	*(u32*)(0x02000314) = 0x00000000;
	*(u32*)(0x02000318) = 0x00000013;
	*(u32*)(0x0200031C) = 0x00000000;
	while (*(u16*)(0x02000306) == 0) {	// Keep running, so that CRC16 isn't 0
		*(u16*)(0x02000306) = swiCRC16(0xFFFF, (void*)0x02000308, 0x18);
	}
	
	unlaunchSetHiyaBoot();

	DC_FlushAll();						// Make reboot not fail
	fifoSendValue32(FIFO_USER_02, 1);	// Reboot into DSiWare title, booted via Launcher
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

void directCardLaunch() {
	/*if (memcmp(ndsCardHeader.gameCode, "ALXX", 4) == 0) {
		u16 alxxBannerCrc = 0;
		extern u32 arm9StartSig[4];
		cardRead(0x75600, &arm9StartSig, 0x10);
		cardRead(0x174602, &alxxBannerCrc, sizeof(u16));
		if ((arm9StartSig[0] == 0xE58D0008
		 && arm9StartSig[1] == 0xE1500005
		 && arm9StartSig[2] == 0xBAFFFFC5
		 && arm9StartSig[3] == 0xE59D100C)
		 || alxxBannerCrc != 0xBA52)
		{
			if (sdFound()) {
				chdir("sd:/");
			}
			int err = runNdsFile ("/_nds/TWiLightMenu/dstwoLaunch.srldr", 0, NULL, true, true, true, boostCpu, boostVram);
			char text[32];
			snprintf(text, sizeof(text), "Start failed. Error %i", err);
			ClearBrightness();
			printSmall(false, 4, 4, text);
			stop();
		}
	}*/
	SetWidescreen(NULL);
	if (sdFound()) {
		chdir("sd:/");
	}
	int err = runNdsFile ("/_nds/TWiLightMenu/slot1launch.srldr", 0, NULL, true, true, false, true, true);
	char text[32];
	snprintf(text, sizeof(text), "Start failed. Error %i", err);
	ClearBrightness();
	printSmall(false, 4, 4, text);
	stop();
}

void printLastPlayedText(int num) {
	printSmallCentered(false, 24, iconYpos[num]+BOX_PY+BOX_PY_spacing2, "Last-played game");
	printSmallCentered(false, 24, iconYpos[num]+BOX_PY+BOX_PY_spacing3, "will appear here.");
}

void refreshNdsCard() {
	if (cardRefreshed) return;

	if (arm7SCFGLocked && showBoxArt) {
		loadBoxArt("nitro:/graphics/boxart_unknown.png");
	} else if ((cardInit(true) == 0) && showBoxArt) {
		char game_TID[5] = {0};
		tonccpy(&game_TID, ndsCardHeader.gameCode, 4);

		char boxArtPath[256];
		sprintf (boxArtPath, (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png" : "fat:/_nds/TWiLightMenu/boxart/%s.png"), game_TID);
		loadBoxArt(boxArtPath);	// Load box art
	} else if (showBoxArt) {
		loadBoxArt("nitro:/graphics/boxart_unknown.png");
	}

	getGameInfo(1, false, "slot1");
	iconUpdate (1, false, "slot1");
	bnrRomType[1] = 0;
	boxArtType[1] = 0;

	// Power off after done retrieving info
	disableSlot1();

	cardRefreshed = true;
	cardEjected = false;
}

void printNdsCartBannerText() {
	if (cardEjected) {
		printSmallCentered(false, 24, iconYpos[0]+BOX_PY+BOX_PY_spacing2, "There is no Game Card");
		printSmallCentered(false, 24, iconYpos[0]+BOX_PY+BOX_PY_spacing3, "inserted.");
	} else if (arm7SCFGLocked) {
		printSmallCentered(false, 24, iconYpos[0]+BOX_PY+BOX_PY_spacing1, "Start Game Card");
	} else {
		titleUpdate(1, false, "slot1");
	}
}

void printGbaBannerText() {
	printSmallCentered(false, 24, iconYpos[3]+BOX_PY+BOX_PY_spacing1, isRegularDS ? gbamodeText : featureUnavailableText);
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	defaultExceptionHandler();

	useTwlCfg = (REG_SCFG_EXT!=0 && (*(u8*)0x02000400 & 0x0F) && (*(u8*)0x02000401 == 0) && (*(u8*)0x02000402 == 0) && (*(u8*)0x02000404 == 0) && (*(u8*)0x02000448 != 0));

	extern const DISC_INTERFACE __my_io_dsisd;

	*(u32*)(0x2FFFD0C) = 0x54494D52;	// Run reboot timer
	fatMountSimple("sd", &__my_io_dsisd);
	fatMountSimple("fat", dldiGetInternal());
    bool fatInited = (sdFound() || flashcardFound());
	*(u32*)(0x2FFFD0C) = 0;
	chdir(sdFound()&&isDSiMode() ? "sd:/" : "fat:/");

	// Read user name
	/*char *username = (char*)PersonalData->name;
		
	// text
	for (int i = 0; i < 10; i++) {
		if (username[i*2] == 0x00)
			username[i*2/2] = 0;
		else
			username[i*2/2] = username[i*2];
	}*/
	
	if (!fatInited) {
		graphicsInit();
		fontInit();
		whiteScreen = true;
		printSmall(false, 64, 32, "fatinitDefault failed!");
		fadeType = true;
		stop();
	}

	nitroFSInit("/_nds/TWiLightMenu/mainmenu.srldr");

	if (access(settingsinipath, F_OK) != 0 && flashcardFound()) {
		settingsinipath = "fat:/_nds/TWiLightMenu/settings.ini";		// Fallback to .ini path on flashcard, if not found on SD card, or if SD access is disabled
	}

	langInit();

	std::string filename[2];

	fifoWaitValue32(FIFO_USER_06);
	if (fifoGetValue32(FIFO_USER_03) == 0) arm7SCFGLocked = true;	// If TWiLight Menu++ is being run from DSiWarehax or flashcard, then arm7 SCFG is locked.
	u16 arm7_SNDEXCNT = fifoGetValue32(FIFO_USER_07);
	if (arm7_SNDEXCNT != 0) isRegularDS = false;	// If sound frequency setting is found, then the console is not a DS Phat/Lite
	isDSLite = fifoGetValue32(FIFO_USER_04);
	fifoSendValue32(FIFO_USER_07, 0);

	LoadSettings();
	widescreenEffects = (consoleModel >= 2 && wideScreen && access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) == 0);
	
	snprintf(pictochatPath, sizeof(pictochatPath), "/_nds/pictochat.nds");
	pictochatFound = (access(pictochatPath, F_OK) == 0);

	if (isDSiMode() && arm7SCFGLocked) {
		if (consoleModel < 2 && !pictochatFound) {
			pictochatFound = true;
			pictochatReboot = true;
		}
		dlplayFound = true;
		dlplayReboot = true;
	} else {
		bool nandInited = false;
		char srcPath[256];
		u8 regions[3] = {0x41, 0x43, 0x4B};

		if (!pictochatFound && consoleModel == 0) {
			for (int i = 0; i < 3; i++)
			{
				snprintf(pictochatPath, sizeof(pictochatPath), "/title/00030005/484e45%x/content/00000000.app", regions[i]);
				if (access(pictochatPath, F_OK) == 0)
				{
					pictochatFound = true;
					break;
				}
			}
		}
		if (!pictochatFound && isDSiMode() && sdFound() && consoleModel == 0) {
			if (!nandInited) {
				fatMountSimple("nand", &io_dsi_nand);
				nandInited = true;
			}
			if (access("nand:/", F_OK) == 0) {
				for (int i = 0; i < 3; i++)
				{
					snprintf(srcPath, sizeof(srcPath), "nand:/title/00030005/484e45%x/content/00000000.app", regions[i]);
					if (access(srcPath, F_OK) == 0)
					{
						snprintf(pictochatPath, sizeof(pictochatPath), "/_nds/pictochat.nds");
						remove(pictochatPath);
						fcopy(srcPath, pictochatPath);	// Copy from NAND
						pictochatFound = true;
						break;
					}
				}
			}
		}

		snprintf(dlplayPath, sizeof(dlplayPath), "/_nds/dlplay.nds");
		dlplayFound = (access(dlplayPath, F_OK) == 0);
		if (!dlplayFound && consoleModel == 0) {
			for (int i = 0; i < 3; i++)
			{
				snprintf(dlplayPath, sizeof(dlplayPath), "/title/00030005/484e44%x/content/00000000.app", regions[i]);
				if (access(dlplayPath, F_OK) == 0) {
					dlplayFound = true;
					break;
				} else if (regions[i] != 0x43 && regions[i] != 0x4B) {
					snprintf(dlplayPath, sizeof(dlplayPath), "/title/00030005/484e4441/content/00000001.app");
					if (access(dlplayPath, F_OK) == 0) {
						dlplayFound = true;
						break;
					}
				}
			}
		}
		if (!dlplayFound && isDSiMode() && sdFound() && consoleModel == 0) {
			if (!nandInited) {
				fatMountSimple("nand", &io_dsi_nand);
				nandInited = true;
			}
			if (access("nand:/", F_OK) == 0) {
				for (int i = 0; i < 3; i++)
				{
					snprintf(srcPath, sizeof(srcPath), "nand:/title/00030005/484e44%x/content/00000000.app", regions[i]);
					if (access(srcPath, F_OK) == 0) {
						snprintf(dlplayPath, sizeof(dlplayPath), "/_nds/dlplay.nds");
						remove(dlplayPath);
						fcopy(srcPath, dlplayPath);	// Copy from NAND
						dlplayFound = true;
						break;
					} else if (regions[i] != 0x43 && regions[i] != 0x4B) {
						snprintf(srcPath, sizeof(srcPath), "nand:/title/00030005/484e4441/content/00000001.app");
						if (access(srcPath, F_OK) == 0) {
							snprintf(dlplayPath, sizeof(dlplayPath), "/_nds/dlplay.nds");
							remove(dlplayPath);
							fcopy(srcPath, dlplayPath);	// Copy from NAND
							dlplayFound = true;
							break;
						}
					}
				}
			}
		}
		if (!dlplayFound && consoleModel >= 2) {
			dlplayFound = true;
			dlplayReboot = true;
		}
	}

	if (isDSiMode() && sdFound() && consoleModel < 2 && launcherApp != -1) {
		u8 setRegion = 0;
		if (sysRegion == -1) {
			// Determine SysNAND region by searching region of System Settings on SDNAND
			char tmdpath[256];
			for (u8 i = 0x41; i <= 0x5A; i++)
			{
				snprintf(tmdpath, sizeof(tmdpath), "sd:/title/00030015/484e42%x/content/title.tmd", i);
				if (access(tmdpath, F_OK) == 0)
				{
					setRegion = i;
					break;
				}
			}
		} else {
			switch(sysRegion) {
				case 0:
				default:
					setRegion = 0x4A;	// JAP
					break;
				case 1:
					setRegion = 0x45;	// USA
					break;
				case 2:
					setRegion = 0x50;	// EUR
					break;
				case 3:
					setRegion = 0x55;	// AUS
					break;
				case 4:
					setRegion = 0x43;	// CHN
					break;
				case 5:
					setRegion = 0x4B;	// KOR
					break;
			}
		}

		snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath), "nand:/title/00030017/484E41%x/content/0000000%i.app", setRegion, launcherApp);
	}

	graphicsInit();
	fontInit();

	iconTitleInit();

	InitSound();

	keysSetRepeat(25,5);

	srand(time(NULL));
	
	bool menuButtonPressed = false;
	
	if (showGba == 1) {
	  if (*(u16*)(0x020000C0) != 0) {
		sysSetCartOwner(BUS_OWNER_ARM9); // Allow arm9 to access GBA ROM
	  } else {
		showGba = 0;	// Hide GBA ROMs
	  }
	}

	if (consoleModel < 2
	 && previousUsedDevice
	 && bothSDandFlashcard()
	 && launchType[previousUsedDevice] == 3
	 && !dsiWareBooter
	 && (
		(access(dsiWarePubPath.c_str(), F_OK) == 0 && extention(dsiWarePubPath.c_str(), ".pub"))
	     || (access(dsiWarePrvPath.c_str(), F_OK) == 0 && extention(dsiWarePrvPath.c_str(), ".prv"))
	    )
	) {
		controlTopBright = false;
		whiteScreen = true;
		fadeType = true;	// Fade in from white
		printSmallCentered(false, 24, "If this takes a while, close and open");
		printSmallCentered(false, 38, "the console's lid.");
		printSmallCentered(false, 86, "Now copying data...");
		printSmallCentered(false, 100, "Do not turn off the power.");
		for (int i = 0; i < 30; i++) swiWaitForVBlank();
		if (access(dsiWarePubPath.c_str(), F_OK) == 0) {
			fcopy("sd:/_nds/TWiLightMenu/tempDSiWare.pub", dsiWarePubPath.c_str());
		}
		if (access(dsiWarePrvPath.c_str(), F_OK) == 0) {
			fcopy("sd:/_nds/TWiLightMenu/tempDSiWare.prv", dsiWarePrvPath.c_str());
		}
		fadeType = false;	// Fade to white
		for (int i = 0; i < 30; i++) swiWaitForVBlank();
		clearText(false);
		whiteScreen = false;
		controlTopBright = true;
	}

	topBgLoad();
	bottomBgLoad();
	
	bool romFound[2] = {false};
	char boxArtPath[2][256];

	// SD card
	if (sdFound() && romPath[0] != "" && access(romPath[0].c_str(), F_OK) == 0) {
		romFound[0] = true;

		romfolder[0] = romPath[0];
		while (!romfolder[0].empty() && romfolder[0][romfolder[0].size()-1] != '/') {
			romfolder[0].resize(romfolder[0].size()-1);
		}
		chdir(romfolder[0].c_str());

		filename[0] = romPath[0];
		const size_t last_slash_idx = filename[0].find_last_of("/");
		if (std::string::npos != last_slash_idx)
		{
			filename[0].erase(0, last_slash_idx + 1);
		}

		if (extention(filename[0], ".nds") || extention(filename[0], ".dsi") || extention(filename[0], ".ids") || extention(filename[0], ".app") || extention(filename[0], ".srl") || extention(filename[0], ".argv")) {
			getGameInfo(0, false, filename[0].c_str());
			iconUpdate (0, false, filename[0].c_str());
			bnrRomType[0] = 0;
			boxArtType[0] = 0;
		} else if (extention(filename[0], ".pce")) {
			bnrRomType[0] = 11;
			boxArtType[0] = 0;
		} else if (extention(filename[0], ".a26") || extention(filename[0], ".a78")) {
			bnrRomType[0] = 10;
			boxArtType[0] = 0;
		} else if (extention(filename[0], ".plg") || extention(filename[0], ".rvid") || extention(filename[0], ".mp4")) {
			bnrRomType[0] = 9;
			boxArtType[0] = 0;
		} else if (extention(filename[0], ".gba")) {
			bnrRomType[0] = 1;
			boxArtType[0] = 1;
		} else if (extention(filename[0], ".gb") || extention(filename[0], ".sgb")) {
			bnrRomType[0] = 2;
			boxArtType[0] = 1;
		} else if (extention(filename[0], ".gbc")) {
			bnrRomType[0] = 3;
			boxArtType[0] = 1;
		} else if (extention(filename[0], ".nes")) {
			bnrRomType[0] = 4;
			boxArtType[0] = 2;
		} else if (extention(filename[0], ".fds")) {
			bnrRomType[0] = 4;
			boxArtType[0] = 1;
		} else if (extention(filename[0], ".sms")) {
			bnrRomType[0] = 5;
			boxArtType[0] = 2;
		} else if (extention(filename[0], ".gg")) {
			bnrRomType[0] = 6;
			boxArtType[0] = 2;
		} else if (extention(filename[0], ".gen")) {
			bnrRomType[0] = 7;
			boxArtType[0] = 2;
		} else if (extention(filename[0], ".smc")) {
			bnrRomType[0] = 8;
			boxArtType[0] = 3;
		} else if (extention(filename[0], ".sfc")) {
			bnrRomType[0] = 8;
			boxArtType[0] = 2;
		}

		if (showBoxArt) {
			// Store box art path
			std::string temp_filename = filename[0];
			sprintf (boxArtPath[0], (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png" : "fat:/_nds/TWiLightMenu/boxart/%s.png"), filename[0].c_str());
			if ((access(boxArtPath[0], F_OK) != 0) && (bnrRomType[0] == 0)) {
				if(extention(filename[0], ".argv")) {
					vector<char*> argarray;

					FILE *argfile = fopen(filename[0].c_str(),"rb");
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
					temp_filename = argarray.at(0);
				}
				// Get game's TID
				FILE *f_nds_file = fopen(temp_filename.c_str(), "rb");
				char game_TID[5];
				grabTID(f_nds_file, game_TID);
				game_TID[4] = 0;
				fclose(f_nds_file);

				sprintf (boxArtPath[0], (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png" : "fat:/_nds/TWiLightMenu/boxart/%s.png"), game_TID);
			}
		}
	}

	// Flashcard (Secondary device)
	if (flashcardFound() && romPath[1] != "" && access(romPath[1].c_str(), F_OK) == 0) {
		romFound[1] = true;

		romfolder[1] = romPath[1];
		while (!romfolder[1].empty() && romfolder[1][romfolder[1].size()-1] != '/') {
			romfolder[1].resize(romfolder[1].size()-1);
		}
		chdir(romfolder[1].c_str());

		filename[1] = romPath[1];
		const size_t last_slash_idx = filename[1].find_last_of("/");
		if (std::string::npos != last_slash_idx)
		{
			filename[1].erase(0, last_slash_idx + 1);
		}

		if (extention(filename[1], ".nds") || extention(filename[1], ".dsi") || extention(filename[1], ".ids") || extention(filename[1], ".app") || extention(filename[1], ".srl") || extention(filename[1], ".argv")) {
			getGameInfo(1, false, filename[1].c_str());
			iconUpdate (1, false, filename[1].c_str());
			bnrRomType[1] = 0;
			boxArtType[1] = 0;
		} else if (extention(filename[1], ".pce")) {
			bnrRomType[1] = 11;
			boxArtType[1] = 0;
		} else if (extention(filename[1], ".a26") || extention(filename[1], ".a78")) {
			bnrRomType[1] = 10;
			boxArtType[1] = 0;
		} else if (extention(filename[1], ".plg") || extention(filename[1], ".rvid") || extention(filename[1], ".mp4")) {
			bnrRomType[1] = 9;
			boxArtType[1] = 0;
		} else if (extention(filename[1], ".gba")) {
			bnrRomType[1] = 1;
			boxArtType[1] = 1;
		} else if (extention(filename[1], ".gb") || extention(filename[1], ".sgb")) {
			bnrRomType[1] = 2;
			boxArtType[1] = 1;
		} else if (extention(filename[1], ".gbc")) {
			bnrRomType[1] = 3;
			boxArtType[1] = 1;
		} else if (extention(filename[1], ".nes")) {
			bnrRomType[1] = 4;
			boxArtType[1] = 2;
		} else if (extention(filename[1], ".fds")) {
			bnrRomType[1] = 4;
			boxArtType[1] = 1;
		} else if (extention(filename[1], ".sms")) {
			bnrRomType[1] = 5;
			boxArtType[1] = 2;
		} else if (extention(filename[1], ".gg")) {
			bnrRomType[1] = 6;
			boxArtType[1] = 2;
		} else if (extention(filename[1], ".gen")) {
			bnrRomType[1] = 7;
			boxArtType[1] = 2;
		} else if (extention(filename[1], ".smc")) {
			bnrRomType[1] = 8;
			boxArtType[1] = 3;
		} else if (extention(filename[1], ".sfc")) {
			bnrRomType[1] = 8;
			boxArtType[1] = 2;
		}

		if (showBoxArt) {
			// Store box art path
			std::string temp_filename = filename[1];
			sprintf (boxArtPath[1], (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png" : "fat:/_nds/TWiLightMenu/boxart/%s.png"), filename[0].c_str());
			if ((access(boxArtPath[1], F_OK) != 0) && (bnrRomType[1] == 0)) {
				if(extention(filename[0], ".argv")) {
					vector<char*> argarray;

					FILE *argfile = fopen(filename[0].c_str(),"rb");
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
					temp_filename = argarray.at(0);
				}
				// Get game's TID
				FILE *f_nds_file = fopen(temp_filename.c_str(), "rb");
				char game_TID[5];
				grabTID(f_nds_file, game_TID);
				game_TID[4] = 0;
				fclose(f_nds_file);

				sprintf (boxArtPath[1], (sdFound() ? "sd:/_nds/TWiLightMenu/boxart/%s.png" : "fat:/_nds/TWiLightMenu/boxart/%s.png"), game_TID);
			}
		}
	}

	if (showBoxArt && !slot1Launched) {
		loadBoxArt(boxArtPath[previousUsedDevice]);	// Load box art
	}

	if (isDSiMode() && !flashcardFound() && slot1Launched) {
		if (REG_SCFG_MC == 0x11) {
			cardEjected = true;
			if (showBoxArt) loadBoxArt("nitro:/graphics/boxart_unknown.png");
		} else {
			refreshNdsCard();
		}
	}

	whiteScreen = false;
	fadeType = true;	// Fade in from white
	for (int i = 0; i < 30; i++) {
		swiWaitForVBlank();
	}
	topBarLoad();
	startMenu = true;	// Show bottom screen graphics
	fadeSpeed = false;

	while(1) {

		if (startMenu) {
			int pressed = 0;

			do {
				clearText();
				printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
				printSmallCentered(false, 112, 6, RetTime().c_str());
				if (flashcardFound()) {
					if (romFound[1]) {
						titleUpdate(1, false, filename[1].c_str());
					} else {
						printLastPlayedText(0);
					}
				} else if (isDSiMode() && !flashcardFound()) {
					printNdsCartBannerText();
				}
				if (!sdFound()) {
					printGbaBannerText();
				} else if (romFound[0]) {
					titleUpdate(0, false, filename[0].c_str());
				} else {
					printLastPlayedText(3);
				}

				if (isDSiMode() && !flashcardFound()) {
					if (REG_SCFG_MC == 0x11) {
						if (cardRefreshed && showBoxArt) {
							loadBoxArt(slot1Launched ? "nitro:/graphics/boxart_unknown.png" : boxArtPath[previousUsedDevice]);
						}
						cardRefreshed = false;
						cardEjected = true;
					} else if (cardEjected) {
						refreshNdsCard();
					}
				}

				scanKeys();
				pressed = keysDownRepeat();
				touchRead(&touch);
				checkSdEject();
				swiWaitForVBlank();
			} while (!pressed);

			if (pressed & KEY_UP) {
				if (startMenu_cursorPosition == 2 || startMenu_cursorPosition == 3 || startMenu_cursorPosition == 5) {
					startMenu_cursorPosition -= 2;
					mmEffectEx(&snd_select);
				} else if (startMenu_cursorPosition == 6) {
					startMenu_cursorPosition -= 3;
					mmEffectEx(&snd_select);
				} else {
					startMenu_cursorPosition--;
					mmEffectEx(&snd_select);
				}
			}

			if (pressed & KEY_DOWN) {
				if (startMenu_cursorPosition == 1 || startMenu_cursorPosition == 3) {
					startMenu_cursorPosition += 2;
					mmEffectEx(&snd_select);
				} else if (startMenu_cursorPosition >= 0 && startMenu_cursorPosition <= 3) {
					startMenu_cursorPosition++;
					mmEffectEx(&snd_select);
				}
			}

			if (pressed & KEY_LEFT) {
				if (startMenu_cursorPosition == 2 || (startMenu_cursorPosition == 5 && isDSiMode() && consoleModel < 2)
				|| startMenu_cursorPosition == 6) {
					startMenu_cursorPosition--;
					mmEffectEx(&snd_select);
				}
			}

			if (pressed & KEY_RIGHT) {
				if (startMenu_cursorPosition == 1 || startMenu_cursorPosition == 4 || startMenu_cursorPosition == 5) {
					startMenu_cursorPosition++;
					mmEffectEx(&snd_select);
				}
			}

			if (pressed & KEY_TOUCH) {
				if (touch.px >= 33 && touch.px <= 221 && touch.py >= 25 && touch.py <= 69) {
					startMenu_cursorPosition = 0;
					menuButtonPressed = true;
				} else if (touch.px >= 33 && touch.px <= 125 && touch.py >= 73 && touch.py <= 117) {
					startMenu_cursorPosition = 1;
					menuButtonPressed = true;
				} else if (touch.px >= 129 && touch.px <= 221 && touch.py >= 73 && touch.py <= 117) {
					startMenu_cursorPosition = 2;
					menuButtonPressed = true;
				} else if (touch.px >= 33 && touch.px <= 221 && touch.py >= 121 && touch.py <= 165) {
					startMenu_cursorPosition = 3;
					menuButtonPressed = true;
				} else if (touch.px >= 10 && touch.px <= 20 && touch.py >= 175 && touch.py <= 185
							&& isDSiMode() && consoleModel < 2)
				{
					startMenu_cursorPosition = 4;
					menuButtonPressed = true;
				} else if (touch.px >= 117 && touch.px <= 137 && touch.py >= 170 && touch.py <= 190) {
					startMenu_cursorPosition = 5;
					menuButtonPressed = true;
				} else if (touch.px >= 235 && touch.px <= 244 && touch.py >= 175 && touch.py <= 185) {
					startMenu_cursorPosition = 6;
					menuButtonPressed = true;
				}
			}

			if (pressed & KEY_A) {
				menuButtonPressed = true;
			}

			if (startMenu_cursorPosition < 0) startMenu_cursorPosition = 0;
			if (startMenu_cursorPosition > 6) startMenu_cursorPosition = 6;

			if (menuButtonPressed) {
				switch (startMenu_cursorPosition) {
					case -1:
					default:
						break;
					case 0:
						if (flashcardFound()) {
							// Launch last-run ROM (Secondary)
						  if (launchType[1] == 0) {
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[0] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								if (romFound[1]) {
									titleUpdate(1, false, filename[1].c_str());
								} else {
									printLastPlayedText(0);
								}
								if (!sdFound()) {
									printGbaBannerText();
								} else if (romFound[0]) {
									titleUpdate(0, false, filename[0].c_str());
								} else {
									printLastPlayedText(3);
								}
								swiWaitForVBlank();
							}
							loadROMselect();
						  } else if (launchType[1] > 0) {
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[0] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								if (romFound[1]) {
									titleUpdate(1, false, filename[1].c_str());
								} else {
									printLastPlayedText(0);
								}
								if (!sdFound()) {
									printGbaBannerText();
								} else if (romFound[0]) {
									titleUpdate(0, false, filename[0].c_str());
								} else {
									printLastPlayedText(3);
								}
								swiWaitForVBlank();
							}
							if (romFound[1]) {
								applaunch = true;
							} else {
								loadROMselect();
							}
						  }
						  secondaryDevice = true;
						} else if (!flashcardFound() && REG_SCFG_MC != 0x11) {
							// Launch Slot-1
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[0] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								printNdsCartBannerText();
								if (romFound[0]) {
									titleUpdate(0, false, filename[0].c_str());
								} else {
									printLastPlayedText(3);
								}
								swiWaitForVBlank();
							}
							slot1Launched = true;
							SaveSettings();

							if (slot1LaunchMethod == 0 || arm7SCFGLocked) {
								dsCardLaunch();
							} else if (slot1LaunchMethod == 2) {
								unlaunchRomBoot("cart:");
							} else {
								directCardLaunch();
							}
						} else {
							mmEffectEx(&snd_wrong);
						}
						break;
					case 1:
						if (pictochatFound) {
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[1] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								if (!sdFound()) {
									printGbaBannerText();
								} else if (romFound[0]) {
									titleUpdate(0, false, filename[0].c_str());
								} else {
									printLastPlayedText(3);
								}
								swiWaitForVBlank();
							}

							// Clear screen with white
							whiteScreen = true;
							controlTopBright = false;
							clearText();

							if (pictochatReboot) {
								*(u32 *)(0x02000300) = 0x434E4C54; // Set "CNLT" warmboot flag
								*(u16 *)(0x02000304) = 0x1801;

								switch (sysRegion) {
									case 4:
										*(u32 *)(0x02000308) = 0x484E4543;
										*(u32 *)(0x0200030C) = 0x00030005;
										*(u32 *)(0x02000310) = 0x484E4543;
										break;
									case 5:
										*(u32 *)(0x02000308) = 0x484E454B;
										*(u32 *)(0x0200030C) = 0x00030005;
										*(u32 *)(0x02000310) = 0x484E454B;
										break;
									default:
										*(u32 *)(0x02000308) = 0x484E4541;	// "HNEA"
										*(u32 *)(0x0200030C) = 0x00030005;
										*(u32 *)(0x02000310) = 0x484E4541;	// "HNEA"
								}

								*(u32 *)(0x02000314) = 0x00030005;
								*(u32 *)(0x02000318) = 0x00000017;
								*(u32 *)(0x0200031C) = 0x00000000;
								while (*(u16 *)(0x02000306) == 0x0000)
								{ // Keep running, so that CRC16 isn't 0
									*(u16 *)(0x02000306) = swiCRC16(0xFFFF, (void *)0x02000308, 0x18);
								}

								if (consoleModel < 2) {
									unlaunchSetHiyaBoot();
								}

								fifoSendValue32(FIFO_USER_02, 1); // Reboot into DSiWare title, booted via Launcher
								for (int i = 0; i < 15; i++) swiWaitForVBlank();
							} else {
								if (sdFound()) {
									chdir("sd:/");
								}
								int err = runNdsFile (pictochatPath, 0, NULL, true, true, true, false, false);
								char text[32];
								snprintf (text, sizeof(text), "Start failed. Error %i", err);
								clearText();
								ClearBrightness();
								printSmall(false, 4, 80, text);
								stop();
							}
						} else {
							mmEffectEx(&snd_wrong);
						}
						break;
					case 2:
						if (dlplayFound) {
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[2] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								if (!sdFound()) {
									printGbaBannerText();
								} else if (romFound[0]) {
									titleUpdate(0, false, filename[0].c_str());
								} else {
									printLastPlayedText(3);
								}
								swiWaitForVBlank();
							}

							// Clear screen with white
							whiteScreen = true;
							controlTopBright = false;
							clearText();

							if (dlplayReboot) {
								*(u32 *)(0x02000300) = 0x434E4C54; // Set "CNLT" warmboot flag
								*(u16 *)(0x02000304) = 0x1801;

								switch (sysRegion) {
									case 4:
										*(u32 *)(0x02000308) = 0x484E4443;
										*(u32 *)(0x0200030C) = 0x00030005;
										*(u32 *)(0x02000310) = 0x484E4443;
										break;
									case 5:
										*(u32 *)(0x02000308) = 0x484E444B;
										*(u32 *)(0x0200030C) = 0x00030005;
										*(u32 *)(0x02000310) = 0x484E444B;
										break;
									default:
										*(u32 *)(0x02000308) = 0x484E4441;	// "HNDA"
										*(u32 *)(0x0200030C) = 0x00030005;
										*(u32 *)(0x02000310) = 0x484E4441;	// "HNDA"
								}

								*(u32 *)(0x02000314) = 0x00030005;
								*(u32 *)(0x02000318) = 0x00000017;
								*(u32 *)(0x0200031C) = 0x00000000;
								while (*(u16 *)(0x02000306) == 0x0000)
								{ // Keep running, so that CRC16 isn't 0
									*(u16 *)(0x02000306) = swiCRC16(0xFFFF, (void *)0x02000308, 0x18);
								}

								if (consoleModel < 2) {
									unlaunchSetHiyaBoot();
								}

								fifoSendValue32(FIFO_USER_02, 1); // Reboot into DSiWare title, booted via Launcher
								for (int i = 0; i < 15; i++) swiWaitForVBlank();
							} else {
								if (sdFound()) {
									chdir("sd:/");
								}
								int err = runNdsFile (dlplayPath, 0, NULL, true, true, true, false, false);
								char text[32];
								snprintf (text, sizeof(text), "Start failed. Error %i", err);
								clearText();
								ClearBrightness();
								printSmall(false, 4, 80, text);
								stop();
							}
						} else {
							mmEffectEx(&snd_wrong);
						}
						break;
					case 3:
						if (sdFound()) {
							// Launch last-run ROM (SD)
						  if (launchType[0] == 0) {
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[3] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								if (romFound[0]) {
									titleUpdate(0, false, filename[0].c_str());
								} else {
									printLastPlayedText(3);
								}
								swiWaitForVBlank();
							}
							loadROMselect();
						  } else if (launchType[0] > 0) {
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[3] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								if (romFound[0]) {
									titleUpdate(0, false, filename[0].c_str());
								} else {
									printLastPlayedText(3);
								}
								swiWaitForVBlank();
							}
							if (romFound[0]) {
								applaunch = true;
							} else {
								loadROMselect();
							}
						  }
						  secondaryDevice = false;
						} else if (isRegularDS) {
							// Switch to GBA mode
							showCursor = false;
							fadeType = false;	// Fade to white
							mmEffectEx(&snd_launch);
							for (int i = 0; i < 50; i++) {
								iconYpos[3] -= 6;
								clearText();
								printSmall(false, 6, 6, "\u2428 Back");	// "(B) Back"
								printSmallCentered(false, 112, 6, RetTime().c_str());
								printGbaBannerText();
								swiWaitForVBlank();
							}
							gbaSwitch();
						} else {
							mmEffectEx(&snd_wrong);
						} 
						break;
					case 4:
						// Adjust backlight level
						if (isDSiMode() && consoleModel < 2) {
							fifoSendValue32(FIFO_USER_04, 1);
							mmEffectEx(&snd_backlight);
						}
						break;
					case 5:
						// Launch settings
						showCursor = false;
						fadeType = false;	// Fade to white
						mmEffectEx(&snd_launch);
						for (int i = 0; i < 50; i++) {
							iconYpos[5] -= 6;
							swiWaitForVBlank();
						}

						gotosettings = true;
						//SaveSettings();
						if (!isDSiMode()) {
							chdir("fat:/");
						} else if (sdFound()) {
							chdir("sd:/");
						}
						int err = runNdsFile ("/_nds/TWiLightMenu/settings.srldr", 0, NULL, true, false, false, true, true);
						iprintf ("Start failed. Error %i\n", err);
						break;
				}
				if (startMenu_cursorPosition == 6) {
					// Open manual
					showCursor = false;
					fadeType = false;	// Fade to white
					mmEffectEx(&snd_launch);
					for (int i = 0; i < 50; i++) {
						iconYpos[6] -= 6;
						swiWaitForVBlank();
					}
					if (!isDSiMode()) {
						chdir("fat:/");
					} else if (sdFound()) {
						chdir("sd:/");
					}
					int err = runNdsFile ("/_nds/TWiLightMenu/manual.srldr", 0, NULL, true, false, false, true, true);
					iprintf ("Start failed. Error %i\n", err);
				}

				menuButtonPressed = false;
			}

			if (pressed & KEY_B) {
				mmEffectEx(&snd_back);
				fadeType = false;	// Fade to white
				for (int i = 0; i < 50; i++) swiWaitForVBlank();
				loadROMselect();
			}

			if ((pressed & KEY_X) && !isRegularDS) {
				mmEffectEx(&snd_back);
				fadeType = false;	// Fade to white
				for (int i = 0; i < 50; i++) swiWaitForVBlank();
				if (!isDSiMode() || launcherApp == -1) {
					*(u32*)(0x02000300) = 0x434E4C54;	// Set "CNLT" warmboot flag
					*(u16*)(0x02000304) = 0x1801;
					*(u32*)(0x02000310) = 0x4D454E55;	// "MENU"
					unlaunchSetHiyaBoot();
				} else {
					tonccpy((u8*)0x02000800, unlaunchAutoLoadID, 12);
					*(u16*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
					*(u16*)(0x0200080E) = 0;			// Unlaunch CRC16 (empty)
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
				}
				fifoSendValue32(FIFO_USER_02, 1);	// ReturntoDSiMenu
			}

		}

		////////////////////////////////////
		// Launch the item

		if (applaunch) {
			// Clear screen with white
			whiteScreen = true;
			fadeSpeed = true;
			controlTopBright = false;
			clearText();

			// Delete previously used DSiWare of flashcard from SD
			if (!gotosettings && consoleModel < 2 && previousUsedDevice && bothSDandFlashcard()) {
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
			getcwd (filePath, PATH_MAX);
			int pathLen = strlen(filePath);
			vector<char*> argarray;

			if (extention(filename[secondaryDevice], ".argv"))
			{
				FILE *argfile = fopen(filename[secondaryDevice].c_str(),"rb");
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
				filename[secondaryDevice] = argarray.at(0);
			} else {
				argarray.push_back(strdup(filename[secondaryDevice].c_str()));
			}

			// Launch DSiWare .nds via Unlaunch
			if ((isDSiMode() || sdFound()) && isDSiWare[secondaryDevice]) {
				const char *typeToReplace = ".nds";
				if (extention(filename[secondaryDevice], ".dsi")) {
					typeToReplace = ".dsi";
				} else if (extention(filename[secondaryDevice], ".ids")) {
					typeToReplace = ".ids";
				} else if (extention(filename[secondaryDevice], ".srl")) {
					typeToReplace = ".srl";
				} else if (extention(filename[secondaryDevice], ".app")) {
					typeToReplace = ".app";
				}

				char *name = argarray.at(0);
				strcpy (filePath + pathLen, name);
				free(argarray.at(0));
				argarray.at(0) = filePath;

				dsiWareSrlPath = argarray[0];
				dsiWarePubPath = replaceAll(argarray[0], typeToReplace, ".pub");
				dsiWarePrvPath = replaceAll(argarray[0], typeToReplace, ".prv");
				launchType[secondaryDevice] = (consoleModel>0 ? 1 : 3);
				SaveSettings();

				sNDSHeaderExt NDSHeader;

				FILE *f_nds_file = fopen(filename[secondaryDevice].c_str(), "rb");

				fread(&NDSHeader, 1, sizeof(NDSHeader), f_nds_file);
				fclose(f_nds_file);

				whiteScreen = true;

				if ((getFileSize(dsiWarePubPath.c_str()) == 0) && (NDSHeader.pubSavSize > 0)) {
					clearText();
					if (memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
						// Display nothing
					} else if (consoleModel >= 2) {
						printSmallCentered(false, 20, "If the bar stopped, press HOME,");
						printSmallCentered(false, 34, "then press B.");
					} else {
						printSmallCentered(false, 20, "If the bar stopped, close and open");
						printSmallCentered(false, 34, "the console's lid.");
					}
					printSmall(false, 2, 80, "Creating public save file...");
					if (!fadeType) {
						fadeType = true;	// Fade in from white
						for (int i = 0; i < 35; i++) swiWaitForVBlank();
					}

					static const int BUFFER_SIZE = 4096;
					char buffer[BUFFER_SIZE];
					memset(buffer, 0, sizeof(buffer));
					char savHdrPath[64];
					snprintf(savHdrPath, sizeof(savHdrPath), "nitro:/DSiWareSaveHeaders/%x.savhdr", (unsigned int)NDSHeader.pubSavSize);
					FILE *hdrFile = fopen(savHdrPath, "rb");
					if (hdrFile) fread(buffer, 1, 0x200, hdrFile);
					fclose(hdrFile);

					FILE *pFile = fopen(dsiWarePubPath.c_str(), "wb");
					if (pFile) {
						fwrite(buffer, 1, sizeof(buffer), pFile);
						fseek(pFile, NDSHeader.pubSavSize - 1, SEEK_SET);
						fputc('\0', pFile);
						fclose(pFile);
					}
					printSmall(false, 2, 88, "Public save file created!");
					for (int i = 0; i < 60; i++) swiWaitForVBlank();
				}

				if ((getFileSize(dsiWarePrvPath.c_str()) == 0) && (NDSHeader.prvSavSize > 0)) {
					clearText();
					if (memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
						// Display nothing
					} else if (consoleModel >= 2) {
						printSmallCentered(false, 20, "If the bar stopped, press HOME,");
						printSmallCentered(false, 34, "then press B.");
					} else {
						printSmallCentered(false, 20, "If the bar stopped, close and open");
						printSmallCentered(false, 34, "the console's lid.");
					}
					printSmall(false, 2, 80, "Creating private save file...");
					if (!fadeType) {
						fadeType = true;	// Fade in from white
						for (int i = 0; i < 35; i++) swiWaitForVBlank();
					}

					static const int BUFFER_SIZE = 4096;
					char buffer[BUFFER_SIZE];
					memset(buffer, 0, sizeof(buffer));
					char savHdrPath[64];
					snprintf(savHdrPath, sizeof(savHdrPath), "nitro:/DSiWareSaveHeaders/%x.savhdr", (unsigned int)NDSHeader.prvSavSize);
					FILE *hdrFile = fopen(savHdrPath, "rb");
					if (hdrFile) fread(buffer, 1, 0x200, hdrFile);
					fclose(hdrFile);

					FILE *pFile = fopen(dsiWarePrvPath.c_str(), "wb");
					if (pFile) {
						fwrite(buffer, 1, sizeof(buffer), pFile);
						fseek(pFile, NDSHeader.prvSavSize - 1, SEEK_SET);
						fputc('\0', pFile);
						fclose(pFile);
					}
					printSmall(false, 2, 88, "Private save file created!");
					for (int i = 0; i < 60; i++) swiWaitForVBlank();
				}

				if (fadeType) {
					fadeType = false;	// Fade to white
					for (int i = 0; i < 25; i++) swiWaitForVBlank();
				}

				if (dsiWareBooter || consoleModel > 0) {
					// Use nds-bootstrap
					loadPerGameSettings(filename[secondaryDevice]);

					bootstrapinipath = "sd:/_nds/nds-bootstrap.ini";
					CIniFile bootstrapini(bootstrapinipath);
					bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", dsiWareSrlPath);
					bootstrapini.SetString("NDS-BOOTSTRAP", "SAV_PATH", dsiWarePubPath);
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
						(forceSleepPatch
					|| (memcmp(io_dldi_data->friendlyName, "TTCARD", 6) == 0 && !isRegularDS)
					|| (memcmp(io_dldi_data->friendlyName, "DSTT", 4) == 0 && !isRegularDS)
					|| (memcmp(io_dldi_data->friendlyName, "DEMON", 5) == 0 && !isRegularDS)
					|| (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0 && !isRegularDS))
					);
					bootstrapini.SaveIniFile(bootstrapinipath);

					if (isDSiMode() || !secondaryDevice) {
						SetWidescreen(filename[secondaryDevice].c_str());
					}
					if (!isDSiMode() && !secondaryDevice) {
						ntrStartSdGame();
					}

					bool useNightly = (perGameSettings_bootstrapFile == -1 ? bootstrapFile : perGameSettings_bootstrapFile);

					char ndsToBoot[256];
					sprintf(ndsToBoot, "sd:/_nds/nds-bootstrap-%s.nds", useNightly ? "nightly" : "release");
					if(access(ndsToBoot, F_OK) != 0) {
						sprintf(ndsToBoot, "fat:/_nds/nds-bootstrap-%s.nds", useNightly ? "nightly" : "release");
					}

					argarray.at(0) = (char *)ndsToBoot;
					int err = runNdsFile(argarray[0], argarray.size(), (const char **)&argarray[0], true, true, false, true, true);
					char text[32];
					snprintf (text, sizeof(text), "Start failed. Error %i", err);
					clearText();
					ClearBrightness();
					printSmall(false, 4, 80, text);
					if (err == 1) {
						printSmall(false, 4, 88, useNightly ? "nds-bootstrap (Nightly)" : "nds-bootstrap (Release)");
						printSmall(false, 4, 96, "not found.");
					}
					stop();
				}

				if (secondaryDevice) {
					clearText();
					printSmallCentered(false, 20, "If the bar stopped, close and open");
					printSmallCentered(false, 34, "the console's lid.");
					printSmallCentered(false, 86, "Now copying data...");
					printSmallCentered(false, 100, "Do not turn off the power.");
					fadeType = true;	// Fade in from white
					for (int i = 0; i < 35; i++) swiWaitForVBlank();
					fcopy(dsiWareSrlPath.c_str(), "sd:/_nds/TWiLightMenu/tempDSiWare.dsi");
					if ((access(dsiWarePubPath.c_str(), F_OK) == 0) && (NDSHeader.pubSavSize > 0)) {
						fcopy(dsiWarePubPath.c_str(), "sd:/_nds/TWiLightMenu/tempDSiWare.pub");
					}
					if ((access(dsiWarePrvPath.c_str(), F_OK) == 0) && (NDSHeader.prvSavSize > 0)) {
						fcopy(dsiWarePrvPath.c_str(), "sd:/_nds/TWiLightMenu/tempDSiWare.prv");
					}
					fadeType = false;	// Fade to white
					for (int i = 0; i < 25; i++) swiWaitForVBlank();

					if ((access(dsiWarePubPath.c_str(), F_OK) == 0 && (NDSHeader.pubSavSize > 0))
					 || (access(dsiWarePrvPath.c_str(), F_OK) == 0 && (NDSHeader.prvSavSize > 0))) {
						clearText();
						printSmallCentered(false, 8, "After saving, please re-start");
						printSmallCentered(false, 20, "TWiLight Menu++ to transfer your");
						printSmallCentered(false, 32, "save data back.");
						fadeType = true;	// Fade in from white
						for (int i = 0; i < 60*3; i++) swiWaitForVBlank();		// Wait 3 seconds
						fadeType = false;	// Fade to white
						for (int i = 0; i < 25; i++) swiWaitForVBlank();
					}
				}

				char unlaunchDevicePath[256];
				if (secondaryDevice) {
					snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath), "sdmc:/_nds/TWiLightMenu/tempDSiWare.dsi");
				} else {
					snprintf(unlaunchDevicePath, sizeof(unlaunchDevicePath), "__%s", dsiWareSrlPath.c_str());
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
				*(u16*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16

				fifoSendValue32(FIFO_USER_02, 1);	// Reboot into DSiWare title, booted via Launcher
				for (int i = 0; i < 15; i++) swiWaitForVBlank();
			}

			// Launch .nds directly or via nds-bootstrap
			if (extention(filename[secondaryDevice], ".nds") || extention(filename[secondaryDevice], ".dsi")
			 || extention(filename[secondaryDevice], ".ids") || extention(filename[secondaryDevice], ".srl")
			 || extention(filename[secondaryDevice], ".app")) {
				const char *typeToReplace = ".nds";
				if (extention(filename[secondaryDevice], ".dsi")) {
					typeToReplace = ".dsi";
				} else if (extention(filename[secondaryDevice], ".ids")) {
					typeToReplace = ".ids";
				} else if (extention(filename[secondaryDevice], ".srl")) {
					typeToReplace = ".srl";
				} else if (extention(filename[secondaryDevice], ".app")) {
					typeToReplace = ".app";
				}

				bool dsModeSwitch = false;
				bool dsModeDSiWare = false;

				char game_TID[5];

				FILE *f_nds_file = fopen(argarray[0], "rb");

				fseek(f_nds_file, offsetof(sNDSHeaderExt, gameCode), SEEK_SET);
				fread(game_TID, 1, 4, f_nds_file);
				fclose(f_nds_file);
				game_TID[4] = 0;
				game_TID[3] = 0;

				if (strcmp(game_TID, "HND") == 0 || strcmp(game_TID, "HNE") == 0) {
					dsModeSwitch = true;
					dsModeDSiWare = true;
					useBackend = false;	// Bypass nds-bootstrap
					homebrewBootstrap = true;
				} else if (isHomebrew[secondaryDevice]) {
					loadPerGameSettings(filename[secondaryDevice]);
					if (perGameSettings_directBoot || (useBootstrap && secondaryDevice)) {
						useBackend = false;	// Bypass nds-bootstrap
					} else {
						useBackend = true;
					}
					if (isDSiMode() && !perGameSettings_dsiMode) {
						dsModeSwitch = true;
					}
					homebrewBootstrap = true;
				} else {
					loadPerGameSettings(filename[secondaryDevice]);
					useBackend = true;
					homebrewBootstrap = false;
				}

				char *name = argarray.at(0);
				strcpy (filePath + pathLen, name);
				free(argarray.at(0));
				argarray.at(0) = filePath;
				if(useBackend) {
					if(useBootstrap || !secondaryDevice) {
						std::string path = argarray[0];
						std::string savename = replaceAll(filename[secondaryDevice], typeToReplace, getSavExtension());
						std::string ramdiskname = replaceAll(filename[secondaryDevice], typeToReplace, getImgExtension());
						std::string romFolderNoSlash = romfolder[secondaryDevice];
						RemoveTrailingSlashes(romFolderNoSlash);
						mkdir (isHomebrew[secondaryDevice] ? "ramdisks" : "saves", 0777);
						std::string savepath = romFolderNoSlash+"/saves/"+savename;
						if (sdFound() && secondaryDevice && fcSaveOnSd) {
							savepath = replaceAll(savepath, "fat:/", "sd:/");
						}
						std::string ramdiskpath = romFolderNoSlash+"/ramdisks/"+ramdiskname;

						if (!isHomebrew[secondaryDevice] && (strcmp(game_TID, "NTR") != 0)) {
							// Create or expand save if game isn't homebrew
							int orgsavesize = getFileSize(savepath.c_str());
							int savesize = 524288;	// 512KB (default size)

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
								clearText();
								ClearBrightness();
								if (isDSiMode() && memcmp(io_dldi_data->friendlyName, "CycloDS iEvolution", 18) == 0) {
									// Display nothing
								} else if (REG_SCFG_EXT != 0 && consoleModel >= 2) {
									printSmallCentered(false, 20, "If this takes a while, press HOME,");
									printSmallCentered(false, 34, "then press B.");
								} else {
									printSmallCentered(false, 20, "If this takes a while, close and open");
									printSmallCentered(false, 34, "the console's lid.");
								}
								printSmallCentered(false, 88, (orgsavesize == 0) ? "Creating save file..." : "Expanding save file...");

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
								clearText();
								printSmallCentered(false, 88, (orgsavesize == 0) ? "Save file created!" : "Save file expanded!");
								for (int i = 0; i < 30; i++) swiWaitForVBlank();
							}
						}

						int donorSdkVer = SetDonorSDK(argarray[0]);
						SetMPUSettings(argarray[0]);
						SetSpeedBumpExclude(argarray[0]);

						bool useWidescreen = (perGameSettings_wideScreen == -1 ? wideScreen : perGameSettings_wideScreen);

						bootstrapinipath = ((!secondaryDevice || (isDSiMode() && sdFound())) ? "sd:/_nds/nds-bootstrap.ini" : "fat:/_nds/nds-bootstrap.ini");
						CIniFile bootstrapini( bootstrapinipath );
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", path);
						bootstrapini.SetString("NDS-BOOTSTRAP", "SAV_PATH", savepath);
						if (!isHomebrew[secondaryDevice]) {
							bootstrapini.SetString("NDS-BOOTSTRAP", "AP_FIX_PATH", setApFix(argarray[0]));
						}
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", (useWidescreen && game_TID[0] == 'W') ? "wide" : "");
						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", (perGameSettings_ramDiskNo >= 0 && !secondaryDevice) ? ramdiskpath : "sd:/null.img");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", perGameSettings_language == -2 ? gameLanguage : perGameSettings_language);
						if (isDSiMode() || !secondaryDevice) {
							bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", perGameSettings_dsiMode == -1 ? bstrap_dsiMode : perGameSettings_dsiMode);
						}
						if ((REG_SCFG_EXT != 0) || !secondaryDevice) {
							bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", perGameSettings_boostCpu == -1 ? boostCpu : perGameSettings_boostCpu);
							bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", perGameSettings_boostVram == -1 ? boostVram : perGameSettings_boostVram);
						}
						bootstrapini.SetInt("NDS-BOOTSTRAP", "EXTENDED_MEMORY", perGameSettings_expandRomSpace == -1 ? bstrap_extendedMemory : perGameSettings_expandRomSpace);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DONOR_SDK_VER", donorSdkVer);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_REGION", mpuregion);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "PATCH_MPU_SIZE", mpusize);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "CARDENGINE_CACHED", ceCached);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", 
							(forceSleepPatch
						|| (memcmp(io_dldi_data->friendlyName, "TTCARD", 6) == 0 && !isRegularDS)
						|| (memcmp(io_dldi_data->friendlyName, "DSTT", 4) == 0 && !isRegularDS)
						|| (memcmp(io_dldi_data->friendlyName, "DEMON", 5) == 0 && !isRegularDS)
						|| (memcmp(io_dldi_data->friendlyName, "R4iDSN", 6) == 0 && !isRegularDS))
						);
						bootstrapini.SaveIniFile( bootstrapinipath );

                        			CheatCodelist codelist;
						u32 gameCode,crc32;

						if ((isDSiMode() || !secondaryDevice) && !isHomebrew[secondaryDevice]) {
							bool cheatsEnabled = true;
							const char* cheatDataBin = "/_nds/nds-bootstrap/cheatData.bin";
							mkdir("/_nds", 0777);
							mkdir("/_nds/nds-bootstrap", 0777);
							if(codelist.romData(path,gameCode,crc32)) {
								long cheatOffset; size_t cheatSize;
								FILE* dat=fopen(sdFound() ? "sd:/_nds/TWiLightMenu/extras/usrcheat.dat" : "fat:/_nds/TWiLightMenu/extras/usrcheat.dat","rb");
								if (dat) {
									if (codelist.searchCheatData(dat, gameCode,
												     crc32, cheatOffset,
												     cheatSize)) {
										codelist.parse(path);
										writeCheatsToFile(codelist.getCheats(), cheatDataBin);
										FILE* cheatData=fopen(cheatDataBin,"rb");
										if (cheatData) {
											u32 check[2];
											fread(check, 1, 8, cheatData);
											fclose(cheatData);
											if (check[1] == 0xCF000000
											|| getFileSize(cheatDataBin) > 0x8000) {
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

						launchType[secondaryDevice] = 1;
						previousUsedDevice = secondaryDevice;
						SaveSettings();

						if (isDSiMode() || !secondaryDevice) {
							SetWidescreen(filename[secondaryDevice].c_str());
						}
						if (!isDSiMode() && !secondaryDevice) {
							ntrStartSdGame();
						}

						bool useNightly = (perGameSettings_bootstrapFile == -1 ? bootstrapFile : perGameSettings_bootstrapFile);

						char ndsToBoot[256];
						sprintf(ndsToBoot, "sd:/_nds/nds-bootstrap-%s%s.nds", homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							sprintf(ndsToBoot, "fat:/_nds/%s-%s%s.nds", isDSiMode() ? "nds-bootstrap" : "b4ds", homebrewBootstrap ? "hb-" : "", useNightly ? "nightly" : "release");
						}

						argarray.at(0) = (char *)ndsToBoot;
						int err = runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], (homebrewBootstrap ? false : true), true, false, true, true);
						char text[32];
						snprintf (text, sizeof(text), "Start failed. Error %i", err);
						clearText();
						ClearBrightness();
						printSmall(false, 4, 80, text);
						if (err == 1) {
							printSmall(false, 4, 88, useNightly ? "nds-bootstrap (Nightly)" : "nds-bootstrap (Release)");
							printSmall(false, 4, 96, "not found.");
						}
						stop();
					} else {
						launchType[secondaryDevice] = 1;
						previousUsedDevice = secondaryDevice;
						SaveSettings();
						loadGameOnFlashcard(argarray[0], true);
					}
				} else {
					if (isDSiMode() || !secondaryDevice) {
						SetWidescreen(filename[secondaryDevice].c_str());
					}
					launchType[secondaryDevice] = 2;
					previousUsedDevice = secondaryDevice;
					SaveSettings();
					if (!isDSiMode() && !secondaryDevice) {
						ntrStartSdGame();
					}

					bool useWidescreen = (perGameSettings_wideScreen == -1 ? wideScreen : perGameSettings_wideScreen);

					if (consoleModel >= 2 && useWidescreen && homebrewHasWide) {
						argarray.push_back((char*)"wide");
					}

					bool runNds_boostCpu = false;
					bool runNds_boostVram = false;
					if (REG_SCFG_EXT != 0 && !dsModeDSiWare) {
						loadPerGameSettings(filename[secondaryDevice]);

						runNds_boostCpu = perGameSettings_boostCpu == -1 ? boostCpu : perGameSettings_boostCpu;
						runNds_boostVram = perGameSettings_boostVram == -1 ? boostVram : perGameSettings_boostVram;
					}
					//iprintf ("Running %s with %d parameters\n", argarray[0], argarray.size());
					int err = runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], true, true, dsModeSwitch, runNds_boostCpu, runNds_boostVram);
					char text[32];
					snprintf (text, sizeof(text), "Start failed. Error %i", err);
					clearText();
					ClearBrightness();
					printSmall(false, 4, 4, text);
					stop();
				}
			} else {
				bool useNDSB = false;
				bool dsModeSwitch = false;
				bool boostCpu = true;
				bool boostVram = false;

				std::string romfolderNoSlash = romfolder[secondaryDevice];
				RemoveTrailingSlashes(romfolderNoSlash);
				char ROMpath[256];
				snprintf (ROMpath, sizeof(ROMpath), "%s/%s", romfolderNoSlash.c_str(), filename[secondaryDevice].c_str());
				romPath[secondaryDevice] = ROMpath;
				previousUsedDevice = secondaryDevice;
				homebrewBootstrap = true;

				const char *ndsToBoot = "sd:/_nds/nds-bootstrap-release.nds";
				if (extention(filename[secondaryDevice], ".plg")) {
					ndsToBoot = "fat:/_nds/TWiLightMenu/bootplg.srldr";
					dsModeSwitch = true;

					// Print .plg path without "fat:" at the beginning
					char ROMpathDS2[256];
					if (secondaryDevice) {
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
				} else if (extention(filename[secondaryDevice], ".rvid")) {
					launchType[secondaryDevice] = 7;

					ndsToBoot = "sd:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/apps/RocketVideoPlayer.nds";
						boostVram = true;
					}
				} else if (extention(filename[secondaryDevice], ".mp4")) {
					launchType[secondaryDevice] = 8;

					ndsToBoot = "sd:/_nds/TWiLightMenu/apps/MPEG4Player.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/apps/MPEG4Player.nds";
						boostVram = true;
					}
				} else if (extention(filename[secondaryDevice], ".gba")) {
					launchType[secondaryDevice] = (showGba == 1) ? 11 : 1;

					if (showGba == 1) {
						clearText();
						ClearBrightness();
						printSmallCentered(false, 20, "If the bar stopped, close and open");
						printSmallCentered(false, 34, "the console's lid.");
						printSmallCentered(false, 88, "Now Loading...");

						showProgressBar = true;
						progressBarLength = 0;

						u32 ptr = 0x08000000;
						extern char copyBuf[0x8000];
						u32 romSize = getFileSize(filename[secondaryDevice].c_str());
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

						FILE* gbaFile = fopen(filename[secondaryDevice].c_str(), "rb");
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

						std::string savename = replaceAll(filename[secondaryDevice], ".gba", ".sav");
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

						ndsToBoot = "fat:/_nds/TWiLightMenu/gbapatcher.srldr";
					} else if (secondaryDevice) {
						ndsToBoot = gbar2DldiAccess ? "sd:/_nds/GBARunner2_arm7dldi_ds.nds" : "sd:/_nds/GBARunner2_arm9dldi_ds.nds";
						if (REG_SCFG_EXT != 0) {
							ndsToBoot = consoleModel>0 ? "sd:/_nds/GBARunner2_arm7dldi_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_dsi.nds";
						}
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = gbar2DldiAccess ? "fat:/_nds/GBARunner2_arm7dldi_ds.nds" : "fat:/_nds/GBARunner2_arm9dldi_ds.nds";
							if (REG_SCFG_EXT != 0) {
								ndsToBoot = consoleModel>0 ? "fat:/_nds/GBARunner2_arm7dldi_3ds.nds" : "fat:/_nds/GBARunner2_arm7dldi_dsi.nds";
							}
						}
						boostVram = false;
					} else {
						useNDSB = true;

						const char* gbar2Path = consoleModel>0 ? "sd:/_nds/GBARunner2_arm7dldi_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_dsi.nds";
						if (isDSiMode() && arm7SCFGLocked) {
							gbar2Path = consoleModel>0 ? "sd:/_nds/GBARunner2_arm7dldi_nodsp_3ds.nds" : "sd:/_nds/GBARunner2_arm7dldi_nodsp_dsi.nds";
						}

						ndsToBoot = (bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", gbar2Path);
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", ROMpath);
						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", "");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				} else if (extention(filename[secondaryDevice], ".a26")) {
					launchType[secondaryDevice] = 9;
					
					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/StellaDS.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/StellaDS.nds";
						boostVram = true;
					}
				} else if (extention(filename[secondaryDevice], ".a78")) {
					launchType[secondaryDevice] = 12;
					
					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/A7800DS.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/A7800DS.nds";
						boostVram = true;
					}
				} else if (extention(filename[secondaryDevice], ".gb") || extention(filename[secondaryDevice], ".sgb") || extention(filename[secondaryDevice], ".gbc")) {
					launchType[secondaryDevice] = 5;
					
					ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/gameyob.nds";
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/gameyob.nds";
						dsModeSwitch = !isDSiMode();
						boostVram = true;
					}
				} else if (extention(filename[secondaryDevice], ".nes") || extention(filename[secondaryDevice], ".fds")) {
					launchType[secondaryDevice] = 4;

					ndsToBoot = (secondaryDevice ? "sd:/_nds/TWiLightMenu/emulators/nesds.nds" : "sd:/_nds/TWiLightMenu/emulators/nestwl.nds");
					if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
						ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/nesds.nds";
						boostVram = true;
					}
				} else if (extention(filename[secondaryDevice], ".sms") || extention(filename[secondaryDevice], ".gg")) {
					mkdir(secondaryDevice ? "fat:/data" : "sd:/data", 0777);
					mkdir(secondaryDevice ? "fat:/data/s8ds" : "sd:/data/s8ds", 0777);

					if (!secondaryDevice && !arm7SCFGLocked && smsGgInRam) {
						launchType[secondaryDevice] = 1;

						useNDSB = true;

						ndsToBoot = (bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/S8DS07.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", ROMpath);
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					} else {
						launchType[secondaryDevice] = 6;

						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/S8DS.nds";
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/S8DS.nds";
							boostVram = true;
						}
					}
				} else if (extention(filename[secondaryDevice], ".gen")) {
					bool usePicoDrive = ((isDSiMode() && sdFound() && arm7SCFGLocked)
						|| showMd==2 || (showMd==3 && getFileSize(filename[secondaryDevice].c_str()) > 0x300000));
					launchType[secondaryDevice] = (usePicoDrive ? 10 : 1);

					if (usePicoDrive || secondaryDevice) {
						ndsToBoot = usePicoDrive ? "sd:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds" : "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds";
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = usePicoDrive ? "fat:/_nds/TWiLightMenu/emulators/PicoDriveTWL.nds" : "fat:/_nds/TWiLightMenu/emulators/jEnesisDS.nds";
							boostVram = true;
						}
						dsModeSwitch = !usePicoDrive;
					} else {
						useNDSB = true;

						ndsToBoot = (bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/jEnesisDS.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/ROM.BIN");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 1);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", ROMpath);
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				} else if (extention(filename[secondaryDevice], ".smc") || extention(filename[secondaryDevice], ".sfc")) {
					launchType[secondaryDevice] = 1;

					if (secondaryDevice) {
						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.nds";
						if(!isDSiMode() || access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "fat:/_nds/TWiLightMenu/emulators/SNEmulDS.nds";
							boostCpu = false;
							boostVram = true;
						}
						dsModeSwitch = true;
					} else {
						useNDSB = true;

						ndsToBoot = (bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/SNEmulDS.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/snes/ROM.SMC");
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", ROMpath);
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				} else if (extention(filename[secondaryDevice], ".pce")) {
					launchType[secondaryDevice] = 1;

					if (secondaryDevice) {
						ndsToBoot = "sd:/_nds/TWiLightMenu/emulators/NitroGrafx.nds";
						if(access(ndsToBoot, F_OK) != 0) {
							ndsToBoot = "/_nds/TWiLightMenu/emulators/NitroGrafx.nds";
							boostVram = true;
						}
						dsModeSwitch = true;
					} else {
						useNDSB = true;

						ndsToBoot = (bootstrapFile ? "sd:/_nds/nds-bootstrap-hb-nightly.nds" : "sd:/_nds/nds-bootstrap-hb-release.nds");
						CIniFile bootstrapini("sd:/_nds/nds-bootstrap.ini");

						bootstrapini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", 0);
						bootstrapini.SetString("NDS-BOOTSTRAP", "NDS_PATH", "sd:/_nds/TWiLightMenu/emulators/NitroGrafx.nds");
						bootstrapini.SetString("NDS-BOOTSTRAP", "HOMEBREW_ARG", ROMpath);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", 0);
						bootstrapini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", 0);

						bootstrapini.SetString("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", "");
						bootstrapini.SaveIniFile("sd:/_nds/nds-bootstrap.ini");
					}
				}

				SaveSettings();
				if (!isDSiMode() && !secondaryDevice && !extention(filename[secondaryDevice], ".plg")) {
					ntrStartSdGame();
				}
				argarray.push_back(ROMpath);
				argarray.at(0) = (char *)ndsToBoot;

				int err = runNdsFile (argarray[0], argarray.size(), (const char **)&argarray[0], !useNDSB, true, dsModeSwitch, boostCpu, boostVram);	// Pass ROM to emulator as argument

				char text[32];
				snprintf (text, sizeof(text), "Start failed. Error %i", err);
				clearText();
				ClearBrightness();
				printLarge(false, 4, 80, text);
				if (err == 1 && useNDSB) {
					printSmall(false, 4, 88, bootstrapFile ? "nds-bootstrap (Nightly)" : "nds-bootstrap (Release)");
					printSmall(false, 4, 96, "not found.");
				}
				stop();

				while(argarray.size() !=0 ) {
					free(argarray.at(0));
					argarray.erase(argarray.begin());
				}
			}
		}
	}

	return 0;
}
