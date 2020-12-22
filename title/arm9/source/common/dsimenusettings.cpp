
#include "dsimenusettings.h"
#include "bootstrappaths.h"
#include "systemdetails.h"
#include "common/inifile.h"
#include <string.h>

extern const char *settingsinipath;

const char *charUnlaunchBg;
bool removeLauncherPatches = true;

TWLSettings::TWLSettings()
{
    romfolder = "";
    pagenum = 0;
    cursorPosition = 0;
    startMenu_cursorPosition = 0;
    consoleModel = -1;

    guiLanguage = ELangDefault;
    colorMode = 0;
    blfLevel = 0;
    dsiWareExploit = 0;
    wifiLed = -1;
    useGbarunner = false;
    showMainMenu = false;
    theme = 0;
    subtheme = 0;

	showGba = 1 + isDSiMode();
	showMd = 3;
    showDirectories = true;
    showBoxArt = 1 + isDSiMode();
    animateDsiIcons = true;
    sysRegion = -1;
    launcherApp = -1;
    secondaryAccess = false;
    previousUsedDevice = false;
    secondaryDevice = false;
	fcSaveOnSd = false;

    flashcard = EDSTTClone;

    slot1LaunchMethod = EDirect;

    useBootstrap = true;
    bootstrapFile = EReleaseBootstrap;

    gameLanguage = ELangDefault;
    boostCpu = false;
    boostVram = false;
    bstrap_dsiMode = false;
    slot1SCFGUnlock = false;

    show12hrClock = true;

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
    unlaunchBg = "default.gif";

	dsiSplash = isDSiMode();
	nintendoLogoColor = 1;
    showlogo = true;
    autorun = false;
	autostartSlot1 = false;

	wideScreen = false;
}

void TWLSettings::loadSettings()
{
    CIniFile settingsini(settingsinipath);

    // UI settings.
    romfolder = settingsini.GetString("SRLOADER", "ROM_FOLDER", romfolder);

    pagenum = settingsini.GetInt("SRLOADER", "PAGE_NUMBER", pagenum);
    cursorPosition = settingsini.GetInt("SRLOADER", "CURSOR_POSITION", cursorPosition);
    startMenu_cursorPosition = settingsini.GetInt("SRLOADER", "STARTMENU_CURSOR_POSITION", startMenu_cursorPosition);
    consoleModel = settingsini.GetInt("SRLOADER", "CONSOLE_MODEL", consoleModel);

	showGba = settingsini.GetInt("SRLOADER", "SHOW_GBA", showGba);
	if (!sys().isRegularDS() && showGba != 0) {
		showGba = 2;
	}
	showMd = settingsini.GetInt("SRLOADER", "SHOW_MDGEN", showMd);

    // Customizable UI settings.
	colorMode = settingsini.GetInt("SRLOADER", "COLOR_MODE", colorMode);
	blfLevel = settingsini.GetInt("SRLOADER", "BLUE_LIGHT_FILTER_LEVEL", blfLevel);
	dsiWareExploit = settingsini.GetInt("SRLOADER", "DSIWARE_EXPLOIT", dsiWareExploit);
	wifiLed = settingsini.GetInt("SRLOADER", "WIFI_LED", wifiLed);
    guiLanguage = settingsini.GetInt("SRLOADER", "LANGUAGE", guiLanguage);
    useGbarunner = settingsini.GetInt("SRLOADER", "USE_GBARUNNER2", useGbarunner);
    if (!sys().isRegularDS()) {
        useGbarunner = true;
    }

    dsiSplash = settingsini.GetInt("SRLOADER", "DSI_SPLASH", dsiSplash);
    nintendoLogoColor = settingsini.GetInt("SRLOADER", "NINTENDO_LOGO_COLOR", nintendoLogoColor);
    showlogo = settingsini.GetInt("SRLOADER", "SHOWLOGO", showlogo);

	secondaryAccess = settingsini.GetInt("SRLOADER", "SECONDARY_ACCESS", secondaryAccess);
	previousUsedDevice = settingsini.GetInt("SRLOADER", "PREVIOUS_USED_DEVICE", previousUsedDevice);
    secondaryDevice = settingsini.GetInt("SRLOADER", "SECONDARY_DEVICE", secondaryDevice);
	fcSaveOnSd = settingsini.GetInt("SRLOADER", "FC_SAVE_ON_SD", fcSaveOnSd);

   	romPath[0] = settingsini.GetString("SRLOADER", "ROM_PATH", romPath[0]);
   	romPath[1] = settingsini.GetString("SRLOADER", "SECONDARY_ROM_PATH", romPath[1]);
    showMainMenu = settingsini.GetInt("SRLOADER", "SHOW_MAIN_MENU", showMainMenu);
    theme = settingsini.GetInt("SRLOADER", "THEME", theme);
    subtheme = settingsini.GetInt("SRLOADER", "SUB_THEME", subtheme);
    showDirectories = settingsini.GetInt("SRLOADER", "SHOW_DIRECTORIES", showDirectories);
    showBoxArt = settingsini.GetInt("SRLOADER", "SHOW_BOX_ART", showBoxArt);
    animateDsiIcons = settingsini.GetInt("SRLOADER", "ANIMATE_DSI_ICONS", animateDsiIcons);
	if (consoleModel < 2) {
		sysRegion = settingsini.GetInt("SRLOADER", "SYS_REGION", sysRegion);
		launcherApp = settingsini.GetInt("SRLOADER", "LAUNCHER_APP", launcherApp);
	}

    slot1LaunchMethod = settingsini.GetInt("SRLOADER", "SLOT1_LAUNCHMETHOD", slot1LaunchMethod);
    bootstrapFile = settingsini.GetInt("SRLOADER", "BOOTSTRAP_FILE", bootstrapFile);
    useBootstrap = settingsini.GetInt("SRLOADER", "USE_BOOTSTRAP", useBootstrap);

    // Default nds-bootstrap settings
    gameLanguage = settingsini.GetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
    boostCpu = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", boostCpu);
    boostVram = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_VRAM", boostVram);
    bstrap_dsiMode = settingsini.GetInt("NDS-BOOTSTRAP", "DSI_MODE", bstrap_dsiMode);

    ak_viewMode = settingsini.GetInt("SRLOADER", "AK_VIEWMODE", ak_viewMode);
    ak_scrollSpeed = settingsini.GetInt("SRLOADER", "AK_SCROLLSPEED", ak_scrollSpeed);
    ak_theme = settingsini.GetString("SRLOADER", "AK_THEME", ak_theme);
    ak_zoomIcons = settingsini.GetInt("SRLOADER", "AK_ZOOM_ICONS", ak_zoomIcons);

    dsiWareSrlPath = settingsini.GetString("SRLOADER", "DSIWARE_SRL", dsiWareSrlPath);
    dsiWarePubPath = settingsini.GetString("SRLOADER", "DSIWARE_PUB", dsiWarePubPath);
    dsiWarePrvPath = settingsini.GetString("SRLOADER", "DSIWARE_PRV", dsiWarePrvPath);
	slot1Launched = settingsini.GetInt("SRLOADER", "SLOT1_LAUNCHED", slot1Launched);
    launchType[0] = settingsini.GetInt("SRLOADER", "LAUNCH_TYPE", launchType[0]);
    launchType[1] = settingsini.GetInt("SRLOADER", "SECONDARY_LAUNCH_TYPE", launchType[1]);
    homebrewArg[0] = settingsini.GetString("SRLOADER", "HOMEBREW_ARG", homebrewArg[0]);
    homebrewArg[1] = settingsini.GetString("SRLOADER", "SECONDARY_HOMEBREW_ARG", homebrewArg[0]);
    homebrewBootstrap = settingsini.GetInt("SRLOADER", "HOMEBREW_BOOTSTRAP", homebrewBootstrap);
    homebrewHasWide = settingsini.GetInt("SRLOADER", "HOMEBREW_HAS_WIDE", homebrewHasWide);

    unlaunchBg = settingsini.GetString("SRLOADER", "UNLAUNCH_BG", unlaunchBg);
	charUnlaunchBg = unlaunchBg.c_str();
	removeLauncherPatches = settingsini.GetInt("SRLOADER", "UNLAUNCH_PATCH_REMOVE", removeLauncherPatches);

    show12hrClock =  settingsini.GetInt("SRLOADER", "SHOW_12H_CLOCK", show12hrClock);

    autorun = settingsini.GetInt("SRLOADER", "AUTORUNGAME", autorun);
    autostartSlot1 = settingsini.GetInt("SRLOADER", "AUTORUNSLOT1", autostartSlot1);

    wideScreen = settingsini.GetInt("SRLOADER", "WIDESCREEN", wideScreen);
}

void TWLSettings::saveSettings()
{
    CIniFile settingsini(settingsinipath);

    settingsini.SetString("SRLOADER", "ROM_FOLDER", romfolder);

    settingsini.SetInt("SRLOADER", "PAGE_NUMBER", pagenum);
    settingsini.SetInt("SRLOADER", "CURSOR_POSITION", cursorPosition);
    settingsini.SetInt("SRLOADER", "STARTMENU_CURSOR_POSITION", startMenu_cursorPosition);
    settingsini.SetInt("SRLOADER", "CONSOLE_MODEL", consoleModel);
    settingsini.SetInt("SRLOADER", "AUTORUNGAME", autorun);
	settingsini.SetInt("SRLOADER", "WIFI_LED", wifiLed);
    // Customizable UI settings.
    settingsini.SetInt("SRLOADER", "LANGUAGE", guiLanguage);
    settingsini.SetInt("SRLOADER", "USE_GBARUNNER2", useGbarunner);

    settingsini.SetInt("SRLOADER", "SHOWLOGO", showlogo);

    settingsini.SetInt("SRLOADER", "SECONDARY_ACCESS", secondaryAccess);
    settingsini.SetInt("SRLOADER", "SHOW_MAIN_MENU", showMainMenu);
    settingsini.SetInt("SRLOADER", "THEME", theme);
    settingsini.SetInt("SRLOADER", "SUB_THEME", subtheme);
    settingsini.SetInt("SRLOADER", "SHOW_DIRECTORIES", showDirectories);
    settingsini.SetInt("SRLOADER", "SHOW_BOX_ART", showBoxArt);
    settingsini.SetInt("SRLOADER", "ANIMATE_DSI_ICONS", animateDsiIcons);
	if (consoleModel < 2) {
		settingsini.SetInt("SRLOADER", "SYS_REGION", sysRegion);
		settingsini.SetInt("SRLOADER", "LAUNCHER_APP", launcherApp);
	}

    settingsini.SetInt("SRLOADER", "SLOT1_LAUNCHMETHOD", slot1LaunchMethod);
    settingsini.SetInt("SRLOADER", "BOOTSTRAP_FILE", bootstrapFile);
    settingsini.SetInt("SRLOADER", "USE_BOOTSTRAP", useBootstrap);

    // Default nds-bootstrap settings
    settingsini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
    settingsini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", boostCpu);
    settingsini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", boostVram);
    settingsini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", bstrap_dsiMode);

    settingsini.SetInt("SRLOADER", "AK_VIEWMODE", ak_viewMode);
    settingsini.SetInt("SRLOADER", "AK_SCROLLSPEED", ak_scrollSpeed);
    settingsini.SetString("SRLOADER", "AK_THEME", ak_theme);
    settingsini.SetInt("SRLOADER", "AK_ZOOM_ICONS", ak_zoomIcons);

    settingsini.SetInt("SRLOADER", "SHOW_12H_CLOCK", show12hrClock);

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