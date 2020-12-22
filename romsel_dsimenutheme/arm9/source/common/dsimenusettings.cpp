#include <nds/arm9/dldi.h>
#include "dsimenusettings.h"
#include "bootstrappaths.h"
#include "systemdetails.h"
#include "common/inifile.h"
#include "flashcard.h"
#include <string.h>

extern const char *settingsinipath;
const char *charUnlaunchBg;
bool removeLauncherPatches = true;

TWLSettings::TWLSettings()
{
	
	romfolder[0] = "sd:/";
	romfolder[1] = "fat:/";

	pagenum[0] = 0;
	pagenum[1] = 0;

	cursorPosition[0] = 0;
	cursorPosition[1] = 0;

	startMenu_cursorPosition = 0;
	consoleModel = 0;

	gotosettings = false;

	guiLanguage = ELangDefault;
	titleLanguage = -1;
	fps = 60;
	colorMode = 0;
	blfLevel = 0;
	sdRemoveDetect = true;
	showMicroSd = false;
	gbar2DldiAccess = false;
	showSelectMenu = false;
	theme = 0;
	subtheme = 0;
	dsiMusic = 1;
	boxArtColorDeband = true;

	showNds = true;
	showGba = 1 + isDSiMode();
	showRvid = true;
	showA26 = true;
	showA78 = true;
	showNes = true;
	showGb = true;
	showSmsGg = true;
	showMd = 3;
	showSnes = true;
	showPce = true;
	showDirectories = true;
	showHidden = false;
	showBoxArt = 1 + isDSiMode();
	animateDsiIcons = true;
	preventDeletion = false;
	sysRegion = -1;
	launcherApp = -1;
	secondaryAccess = false;
	previousUsedDevice = false;
	secondaryDevice = false;
	fcSaveOnSd = false;
	updateRecentlyPlayedList = true;
	sortMethod = 0;

	flashcard = EDSTTClone;

	slot1LaunchMethod = EDirect;

	useBootstrap = isDSiMode();
	bootstrapFile = EReleaseBootstrap;

	gameLanguage = ELangDefault;
	boostCpu = false;
	boostVram = false;
	bstrap_dsiMode = EDSMode;
    extendedMemory = 0;

	forceSleepPatch = false;
	slot1SCFGUnlock = false;
	dsiWareBooter = false;

	show12hrClock = true;

	//snesEmulator = true;
	smsGgInRam = false;

	ak_viewMode = EViewInternal;
	ak_scrollSpeed = EScrollFast;
	ak_theme = "zelda";
	ak_zoomIcons = true;

	slot1Launched = false;
	launchType[0] = ENoLaunch;
	launchType[1] = ENoLaunch;
	homebrewBootstrap = EReleaseBootstrap;
	homebrewHasWide = false;

	r4_theme = "unused";
	dsi_theme = "dark";
	_3ds_theme = "light";
	
    gbaBorder = "default.png";
    unlaunchBg = "default.gif";

	soundfreq = EFreq32KHz;
	showlogo = true;
	autorun = false;

	wideScreen = false;
}

void TWLSettings::loadSettings()
{
	//printf("ms().loadSettings()\n");
	CIniFile settingsini(settingsinipath);

	// UI settings.
	romfolder[0] = settingsini.GetString("SRLOADER", "ROM_FOLDER", romfolder[0]);
	romfolder[1] = settingsini.GetString("SRLOADER", "SECONDARY_ROM_FOLDER", romfolder[1]);

	pagenum[0] = settingsini.GetInt("SRLOADER", "PAGE_NUMBER", pagenum[0]);
	pagenum[1] = settingsini.GetInt("SRLOADER", "SECONDARY_PAGE_NUMBER", pagenum[1]);

	cursorPosition[0] = settingsini.GetInt("SRLOADER", "CURSOR_POSITION", cursorPosition[0]);
	cursorPosition[1] = settingsini.GetInt("SRLOADER", "SECONDARY_CURSOR_POSITION", cursorPosition[1]);

	startMenu_cursorPosition = settingsini.GetInt("SRLOADER", "STARTMENU_CURSOR_POSITION", startMenu_cursorPosition);
	consoleModel = settingsini.GetInt("SRLOADER", "CONSOLE_MODEL", consoleModel);

	showNds = settingsini.GetInt("SRLOADER", "SHOW_NDS", showNds);
	showGba = settingsini.GetInt("SRLOADER", "SHOW_GBA", showGba);
	if (!sys().isRegularDS() && showGba != 0) {
		showGba = 2;
	}
	showRvid = settingsini.GetInt("SRLOADER", "SHOW_RVID", showRvid);
	showA26 = settingsini.GetInt("SRLOADER", "SHOW_A26", showA26);
	showA78 = settingsini.GetInt("SRLOADER", "SHOW_A78", showA78);
	showNes = settingsini.GetInt("SRLOADER", "SHOW_NES", showNes);
	showGb = settingsini.GetInt("SRLOADER", "SHOW_GB", showGb);
	showSmsGg = settingsini.GetInt("SRLOADER", "SHOW_SMSGG", showSmsGg);
	showMd = settingsini.GetInt("SRLOADER", "SHOW_MDGEN", showMd);
	showSnes = settingsini.GetInt("SRLOADER", "SHOW_SNES", showSnes);
	showPce = settingsini.GetInt("SRLOADER", "SHOW_PCE", showPce);

	updateRecentlyPlayedList = settingsini.GetInt("SRLOADER", "UPDATE_RECENTLY_PLAYED_LIST", updateRecentlyPlayedList);
	sortMethod = settingsini.GetInt("SRLOADER", "SORT_METHOD", sortMethod);

	// Customizable UI settings.
	fps = settingsini.GetInt("SRLOADER", "FRAME_RATE", fps);
	colorMode = settingsini.GetInt("SRLOADER", "COLOR_MODE", colorMode);
	blfLevel = settingsini.GetInt("SRLOADER", "BLUE_LIGHT_FILTER_LEVEL", blfLevel);
	guiLanguage = settingsini.GetInt("SRLOADER", "LANGUAGE", guiLanguage);
	titleLanguage = settingsini.GetInt("SRLOADER", "TITLELANGUAGE", titleLanguage);
	sdRemoveDetect = settingsini.GetInt("SRLOADER", "SD_REMOVE_DETECT", sdRemoveDetect);
	showMicroSd = settingsini.GetInt("SRLOADER", "SHOW_MICROSD", showMicroSd);
	gbar2DldiAccess = settingsini.GetInt("SRLOADER", "GBAR2_DLDI_ACCESS", gbar2DldiAccess);

	soundfreq = settingsini.GetInt("SRLOADER", "SOUND_FREQ", soundfreq);

	secondaryAccess = settingsini.GetInt("SRLOADER", "SECONDARY_ACCESS", secondaryAccess);
	previousUsedDevice = settingsini.GetInt("SRLOADER", "PREVIOUS_USED_DEVICE", previousUsedDevice);
	romPath[0] = settingsini.GetString("SRLOADER", "ROM_PATH", romPath[0]);
	romPath[1] = settingsini.GetString("SRLOADER", "SECONDARY_ROM_PATH", romPath[1]);

	secondaryDevice = bothSDandFlashcard() ? settingsini.GetInt("SRLOADER", "SECONDARY_DEVICE", secondaryDevice) : flashcardFound();
	fcSaveOnSd = settingsini.GetInt("SRLOADER", "FC_SAVE_ON_SD", fcSaveOnSd);
	
	showSelectMenu = settingsini.GetInt("SRLOADER", "SHOW_SELECT_MENU", showSelectMenu);
	theme = settingsini.GetInt("SRLOADER", "THEME", theme);
	subtheme = settingsini.GetInt("SRLOADER", "SUB_THEME", subtheme);
	dsiMusic = settingsini.GetInt("SRLOADER", "DSI_MUSIC", dsiMusic);
	boxArtColorDeband = settingsini.GetInt("SRLOADER", "PHOTO_BOXART_COLOR_DEBAND", boxArtColorDeband);
	showDirectories = settingsini.GetInt("SRLOADER", "SHOW_DIRECTORIES", showDirectories);
	showHidden = settingsini.GetInt("SRLOADER", "SHOW_HIDDEN", showHidden);
	showBoxArt = settingsini.GetInt("SRLOADER", "SHOW_BOX_ART", showBoxArt);
	animateDsiIcons = settingsini.GetInt("SRLOADER", "ANIMATE_DSI_ICONS", animateDsiIcons);
	preventDeletion = settingsini.GetInt("SRLOADER", "PREVENT_ROM_DELETION", preventDeletion);
	if (consoleModel < 2) {
		sysRegion = settingsini.GetInt("SRLOADER", "SYS_REGION", sysRegion);
		launcherApp = settingsini.GetInt("SRLOADER", "LAUNCHER_APP", launcherApp);
	}


	slot1LaunchMethod = settingsini.GetInt("SRLOADER", "SLOT1_LAUNCHMETHOD", slot1LaunchMethod);
	bootstrapFile = settingsini.GetInt("SRLOADER", "BOOTSTRAP_FILE", bootstrapFile);
	useBootstrap = settingsini.GetInt("SRLOADER", "USE_BOOTSTRAP", useBootstrap);
	if (io_dldi_data->ioInterface.features & FEATURE_SLOT_GBA) {
		useBootstrap = true;
	}

	// Default nds-bootstrap settings
	gameLanguage = settingsini.GetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
	boostCpu = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", boostCpu);
	boostVram = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_VRAM", boostVram);
	bstrap_dsiMode = settingsini.GetInt("NDS-BOOTSTRAP", "DSI_MODE", bstrap_dsiMode);
	extendedMemory = settingsini.GetInt("NDS-BOOTSTRAP", "EXTENDED_MEMORY", extendedMemory);

	forceSleepPatch = settingsini.GetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", forceSleepPatch);
	dsiWareBooter = settingsini.GetInt("SRLOADER", "DSIWARE_BOOTER", dsiWareBooter);

	dsiWareSrlPath = settingsini.GetString("SRLOADER", "DSIWARE_SRL", dsiWareSrlPath);
	dsiWarePubPath = settingsini.GetString("SRLOADER", "DSIWARE_PUB", dsiWarePubPath);
	dsiWarePrvPath = settingsini.GetString("SRLOADER", "DSIWARE_PRV", dsiWarePrvPath);
	launchType[0] = settingsini.GetInt("SRLOADER", "LAUNCH_TYPE", launchType[0]);
	launchType[1] = settingsini.GetInt("SRLOADER", "SECONDARY_LAUNCH_TYPE", launchType[1]);
	homebrewBootstrap = settingsini.GetInt("SRLOADER", "HOMEBREW_BOOTSTRAP", homebrewBootstrap);

	show12hrClock = settingsini.GetInt("SRLOADER", "SHOW_12H_CLOCK", show12hrClock);

	dsi_theme = settingsini.GetString("SRLOADER", "DSI_THEME", dsi_theme);
	_3ds_theme = settingsini.GetString("SRLOADER", "3DS_THEME", _3ds_theme);
    unlaunchBg = settingsini.GetString("SRLOADER", "UNLAUNCH_BG", unlaunchBg);
    gbaBorder = settingsini.GetString("SRLOADER", "GBA_BORDER", gbaBorder);
	charUnlaunchBg = unlaunchBg.c_str();
	removeLauncherPatches = settingsini.GetInt("SRLOADER", "UNLAUNCH_PATCH_REMOVE", removeLauncherPatches);

	//snesEmulator = settingsini.GetInt("SRLOADER", "SNES_EMULATOR", snesEmulator);
	smsGgInRam = settingsini.GetInt("SRLOADER", "SMS_GG_IN_RAM", smsGgInRam);
	
	wideScreen = settingsini.GetInt("SRLOADER", "WIDESCREEN", wideScreen);
}

void TWLSettings::saveSettings()
{
	CIniFile settingsini(settingsinipath);

	settingsini.SetString("SRLOADER", "ROM_FOLDER", romfolder[0]);
	settingsini.SetString("SRLOADER", "SECONDARY_ROM_FOLDER", romfolder[1]);
	
	settingsini.SetInt("SRLOADER", "PAGE_NUMBER", pagenum[0]);
	settingsini.SetInt("SRLOADER", "SECONDARY_PAGE_NUMBER", pagenum[1]);

	settingsini.SetInt("SRLOADER", "CURSOR_POSITION", cursorPosition[0]);
	settingsini.SetInt("SRLOADER", "SECONDARY_CURSOR_POSITION", cursorPosition[1]);

	settingsini.SetInt("SRLOADER", "SECONDARY_ACCESS", secondaryAccess);

	if (bothSDandFlashcard()) {
		settingsini.SetInt("SRLOADER", "SECONDARY_DEVICE", secondaryDevice);
	}

	if (!gotosettings) {
		settingsini.SetInt("SRLOADER", "PREVIOUS_USED_DEVICE", previousUsedDevice);
		settingsini.SetString("SRLOADER", "ROM_PATH", romPath[0]);
		settingsini.SetString("SRLOADER", "SECONDARY_ROM_PATH", romPath[1]);
		settingsini.SetString("SRLOADER", "DSIWARE_SRL", dsiWareSrlPath);
		settingsini.SetString("SRLOADER", "DSIWARE_PUB", dsiWarePubPath);
		settingsini.SetString("SRLOADER", "DSIWARE_PRV", dsiWarePrvPath);
		settingsini.SetInt("SRLOADER", "SLOT1_LAUNCHED", slot1Launched);
		settingsini.SetInt("SRLOADER", "LAUNCH_TYPE", launchType[0]);
		settingsini.SetInt("SRLOADER", "SECONDARY_LAUNCH_TYPE", launchType[1]);
		settingsini.SetString("SRLOADER", ms().secondaryDevice ? "SECONDARY_HOMEBREW_ARG" : "HOMEBREW_ARG", homebrewArg);
		settingsini.SetInt("SRLOADER", "HOMEBREW_BOOTSTRAP", homebrewBootstrap);
		settingsini.SetInt("SRLOADER", "HOMEBREW_HAS_WIDE", homebrewHasWide);
	}
	
	settingsini.SetInt("SRLOADER", "SORT_METHOD", sortMethod);

	settingsini.SaveIniFile(settingsinipath);
}

TWLSettings::TLanguage TWLSettings::getGuiLanguage()
{
	if (guiLanguage == ELangDefault)
	{
		extern bool useTwlCfg;
		return (TLanguage)(useTwlCfg ? *(u8*)0x02000406 : PersonalData->language);
	}
	return (TLanguage)guiLanguage;
}
