
#include "dsimenusettings.h"
#include "bootstrappaths.h"
#include "systemdetails.h"
#include "common/inifile.h"
#include <string.h>

extern const char *settingsinipath;

TWLSettings::TWLSettings()
{
    romfolder = "";
    pagenum = 0;
    cursorPosition = 0;
    startMenu_cursorPosition = 0;
    consoleModel = 0;

    guiLanguage = ELangDefault;
    gameLanguage = ELangDefault;
    titleLanguage = ELangDefault;
    fps = 60;
    colorMode = 0;
    blfLevel = 0;
    dsiWareExploit = 0;
    wifiLed = true;
    sdRemoveDetect = true;
    showMicroSd = false;
    gbar2DldiAccess = false;
    showMainMenu = false;
    showSelectMenu = false;
    theme = 0;
    subtheme = 0;
    settingsMusic = -1;
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
    updateRecentlyPlayedList = true;
    sortMethod = 0;
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
    slot1AccessSD = false;
    slot1SCFGUnlock = false;
    dsiWareBooter = false;

    show12hrClock = true;

    //snesEmulator = true;
    smsGgInRam = false;

    ak_viewMode = EViewInternal;
    ak_scrollSpeed = EScrollFast;
    ak_theme = "zelda";
    ak_zoomIcons = true;

    launchType = -1; // ENoLaunch
    homebrewBootstrap = EReleaseBootstrap;

    r4_theme = "unused";
    
    dsi_theme = "dark";
    _3ds_theme = "light";

    gbaBorder = "default.png";
    unlaunchBg = "default.gif";
	removeLauncherPatches = true;

    soundFreq = EFreq32KHz;
    dsiSplash = isDSiMode();
	nintendoLogoColor = 1;
    showlogo = true;
    autorun = false;
    autostartSlot1 = false;

    //screenScaleSize = 0;
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
	if (isDSiMode() && (access("sd:/", F_OK) == 0) && sys().arm7SCFGLocked() && (ms().showMd == 1 || ms().showMd == 3)) {
		ms().showMd = 2;	// Use only PicoDriveTWL
	}
    showSnes = settingsini.GetInt("SRLOADER", "SHOW_SNES", showSnes);
    showPce = settingsini.GetInt("SRLOADER", "SHOW_PCE", showPce);

    // Customizable UI settings.
    fps = settingsini.GetInt("SRLOADER", "FRAME_RATE", fps);
    colorMode = settingsini.GetInt("SRLOADER", "COLOR_MODE", colorMode);
    blfLevel = settingsini.GetInt("SRLOADER", "BLUE_LIGHT_FILTER_LEVEL", blfLevel);
    dsiWareExploit = settingsini.GetInt("SRLOADER", "DSIWARE_EXPLOIT", dsiWareExploit);
    wifiLed = settingsini.GetInt("SRLOADER", "WIFI_LED", wifiLed);
    guiLanguage = settingsini.GetInt("SRLOADER", "LANGUAGE", guiLanguage);
    titleLanguage = settingsini.GetInt("SRLOADER", "TITLELANGUAGE", titleLanguage);
    sdRemoveDetect = settingsini.GetInt("SRLOADER", "SD_REMOVE_DETECT", sdRemoveDetect);
    showMicroSd = settingsini.GetInt("SRLOADER", "SHOW_MICROSD", showMicroSd);
    gbar2DldiAccess = settingsini.GetInt("SRLOADER", "GBAR2_DLDI_ACCESS", gbar2DldiAccess);

    dsiSplash = settingsini.GetInt("SRLOADER", "DSI_SPLASH", dsiSplash);
    nintendoLogoColor = settingsini.GetInt("SRLOADER", "NINTENDO_LOGO_COLOR", nintendoLogoColor);
    showlogo = settingsini.GetInt("SRLOADER", "SHOWLOGO", showlogo);

    secondaryAccess = settingsini.GetInt("SRLOADER", "SECONDARY_ACCESS", secondaryAccess);
    previousUsedDevice = settingsini.GetInt("SRLOADER", "PREVIOUS_USED_DEVICE", previousUsedDevice);
    secondaryDevice = settingsini.GetInt("SRLOADER", "SECONDARY_DEVICE", secondaryDevice);
    fcSaveOnSd = settingsini.GetInt("SRLOADER", "FC_SAVE_ON_SD", fcSaveOnSd);

    showMainMenu = settingsini.GetInt("SRLOADER", "SHOW_MAIN_MENU", showMainMenu);
    showSelectMenu = settingsini.GetInt("SRLOADER", "SHOW_SELECT_MENU", showSelectMenu);
    theme = settingsini.GetInt("SRLOADER", "THEME", theme);
    subtheme = settingsini.GetInt("SRLOADER", "SUB_THEME", subtheme);
    settingsMusic = settingsini.GetInt("SRLOADER", "SETTINGS_MUSIC", settingsMusic);
    dsiMusic = settingsini.GetInt("SRLOADER", "DSI_MUSIC", dsiMusic);
	boxArtColorDeband = settingsini.GetInt("SRLOADER", "PHOTO_BOXART_COLOR_DEBAND", boxArtColorDeband);
    updateRecentlyPlayedList = settingsini.GetInt("SRLOADER", "UPDATE_RECENTLY_PLAYED_LIST", updateRecentlyPlayedList);
    sortMethod = settingsini.GetInt("SRLOADER", "SORT_METHOD", sortMethod);
    showDirectories = settingsini.GetInt("SRLOADER", "SHOW_DIRECTORIES", showDirectories);
    showHidden = settingsini.GetInt("SRLOADER", "SHOW_HIDDEN", showHidden);
    preventDeletion = settingsini.GetInt("SRLOADER", "PREVENT_ROM_DELETION", preventDeletion);
    showBoxArt = settingsini.GetInt("SRLOADER", "SHOW_BOX_ART", showBoxArt);
    animateDsiIcons = settingsini.GetInt("SRLOADER", "ANIMATE_DSI_ICONS", animateDsiIcons);
    if (consoleModel < 2) {
        sysRegion = settingsini.GetInt("SRLOADER", "SYS_REGION", sysRegion);
        launcherApp = settingsini.GetInt("SRLOADER", "LAUNCHER_APP", launcherApp);
    }

    flashcard = settingsini.GetInt("SRLOADER", "FLASHCARD", flashcard);

    slot1LaunchMethod = settingsini.GetInt("SRLOADER", "SLOT1_LAUNCHMETHOD", slot1LaunchMethod);
    bootstrapFile = settingsini.GetInt("SRLOADER", "BOOTSTRAP_FILE", bootstrapFile);
    useBootstrap = settingsini.GetInt("SRLOADER", "USE_BOOTSTRAP", useBootstrap);

    // Default nds-bootstrap settings
    gameLanguage = settingsini.GetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
    boostCpu = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_CPU", boostCpu);
    boostVram = settingsini.GetInt("NDS-BOOTSTRAP", "BOOST_VRAM", boostVram);
    bstrap_dsiMode = settingsini.GetInt("NDS-BOOTSTRAP", "DSI_MODE", bstrap_dsiMode);
	extendedMemory = settingsini.GetInt("NDS-BOOTSTRAP", "EXTENDED_MEMORY", extendedMemory);

    forceSleepPatch = settingsini.GetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", forceSleepPatch);
    soundFreq = settingsini.GetInt("NDS-BOOTSTRAP", "SOUND_FREQ", soundFreq);
    slot1AccessSD = settingsini.GetInt("SRLOADER", "SLOT1_ENABLESD", slot1AccessSD);
    slot1SCFGUnlock = settingsini.GetInt("SRLOADER", "SLOT1_SCFG_UNLOCK", slot1SCFGUnlock);
    dsiWareBooter = settingsini.GetInt("SRLOADER", "DSIWARE_BOOTER", dsiWareBooter);

    ak_viewMode = settingsini.GetInt("SRLOADER", "AK_VIEWMODE", ak_viewMode);
    ak_scrollSpeed = settingsini.GetInt("SRLOADER", "AK_SCROLLSPEED", ak_scrollSpeed);
    ak_theme = settingsini.GetString("SRLOADER", "AK_THEME", ak_theme);
    ak_zoomIcons = settingsini.GetInt("SRLOADER", "AK_ZOOM_ICONS", ak_zoomIcons);

    dsiWareSrlPath = settingsini.GetString("SRLOADER", "DSIWARE_SRL", dsiWareSrlPath);
    dsiWarePubPath = settingsini.GetString("SRLOADER", "DSIWARE_PUB", dsiWarePubPath);
    dsiWarePrvPath = settingsini.GetString("SRLOADER", "DSIWARE_PRV", dsiWarePrvPath);
    launchType = settingsini.GetInt("SRLOADER", "LAUNCH_TYPE", launchType);
    homebrewArg = settingsini.GetString("SRLOADER", "HOMEBREW_ARG", homebrewArg);
    homebrewBootstrap = settingsini.GetInt("SRLOADER", "HOMEBREW_BOOTSTRAP", homebrewBootstrap);

    show12hrClock = settingsini.GetInt("SRLOADER", "SHOW_12H_CLOCK", show12hrClock);

    r4_theme = settingsini.GetString("SRLOADER", "R4_THEME", r4_theme);
    dsi_theme = settingsini.GetString("SRLOADER", "DSI_THEME", dsi_theme);
    _3ds_theme = settingsini.GetString("SRLOADER", "3DS_THEME", _3ds_theme);
    gbaBorder = settingsini.GetString("SRLOADER", "GBA_BORDER", gbaBorder);
    unlaunchBg = settingsini.GetString("SRLOADER", "UNLAUNCH_BG", unlaunchBg);
	removeLauncherPatches = settingsini.GetInt("SRLOADER", "UNLAUNCH_PATCH_REMOVE", removeLauncherPatches);

    //snesEmulator = settingsini.GetInt("SRLOADER", "SNES_EMULATOR", snesEmulator);
    smsGgInRam = settingsini.GetInt("SRLOADER", "SMS_GG_IN_RAM", smsGgInRam);

    autorun = settingsini.GetInt("SRLOADER", "AUTORUNGAME", autorun);
    autostartSlot1 = settingsini.GetInt("SRLOADER", "AUTORUNSLOT1", autostartSlot1);

    //screenScaleSize = settingsini.GetInt("TWL_FIRM", "SCREENSCALESIZE", screenScaleSize);
    wideScreen = settingsini.GetInt("SRLOADER", "WIDESCREEN", wideScreen);
}

void TWLSettings::saveSettings()
{
    CIniFile settingsini(settingsinipath);

    settingsini.SetString("SRLOADER", "ROM_FOLDER", romfolder);

    settingsini.SetInt("SRLOADER", "PAGE_NUMBER", pagenum);
    settingsini.SetInt("SRLOADER", "CURSOR_POSITION", cursorPosition);
    settingsini.SetInt("SRLOADER", "STARTMENU_CURSOR_POSITION", startMenu_cursorPosition);
    settingsini.SetInt("SRLOADER", "AUTORUNGAME", autorun);
    settingsini.SetInt("SRLOADER", "AUTORUNSLOT1", autostartSlot1);

    // Customizable UI settings.
    settingsini.SetInt("SRLOADER", "FRAME_RATE", fps);
    settingsini.SetInt("SRLOADER", "COLOR_MODE", colorMode);
    settingsini.SetInt("SRLOADER", "BLUE_LIGHT_FILTER_LEVEL", blfLevel);
    settingsini.SetInt("SRLOADER", "DSIWARE_EXPLOIT", dsiWareExploit);
    settingsini.SetInt("SRLOADER", "WIFI_LED", wifiLed);
    settingsini.SetInt("SRLOADER", "LANGUAGE", guiLanguage);
    settingsini.SetInt("SRLOADER", "TITLELANGUAGE", titleLanguage);
    settingsini.SetInt("SRLOADER", "GBAR2_DLDI_ACCESS", gbar2DldiAccess);
    settingsini.SetInt("SRLOADER", "SD_REMOVE_DETECT", sdRemoveDetect);
    settingsini.SetInt("SRLOADER", "SHOW_MICROSD", showMicroSd);

    settingsini.SetInt("SRLOADER", "DSI_SPLASH", dsiSplash);
    settingsini.SetInt("SRLOADER", "NINTENDO_LOGO_COLOR", nintendoLogoColor);
    settingsini.SetInt("SRLOADER", "SHOWLOGO", showlogo);

    settingsini.SetInt("SRLOADER", "SECONDARY_ACCESS", secondaryAccess);
    settingsini.SetInt("SRLOADER", "SHOW_MAIN_MENU", showMainMenu);
    settingsini.SetInt("SRLOADER", "SHOW_SELECT_MENU", showSelectMenu);
    settingsini.SetInt("SRLOADER", "THEME", theme);
    settingsini.SetInt("SRLOADER", "SUB_THEME", subtheme);
    settingsini.SetInt("SRLOADER", "SETTINGS_MUSIC", settingsMusic);
    settingsini.SetInt("SRLOADER", "DSI_MUSIC", dsiMusic);
	settingsini.SetInt("SRLOADER", "PHOTO_BOXART_COLOR_DEBAND", boxArtColorDeband);
    settingsini.SetInt("SRLOADER", "SHOW_NDS", showNds);
    settingsini.SetInt("SRLOADER", "SHOW_GBA", showGba);
    settingsini.SetInt("SRLOADER", "SHOW_RVID", showRvid);
    settingsini.SetInt("SRLOADER", "SHOW_A26", showA26);
    settingsini.SetInt("SRLOADER", "SHOW_A78", showA78);
    settingsini.SetInt("SRLOADER", "SHOW_NES", showNes);
    settingsini.SetInt("SRLOADER", "SHOW_GB", showGb);
    settingsini.SetInt("SRLOADER", "SHOW_SMSGG", showSmsGg);
    settingsini.SetInt("SRLOADER", "SHOW_MDGEN", showMd);
    settingsini.SetInt("SRLOADER", "SHOW_SNES", showSnes);
    settingsini.SetInt("SRLOADER", "SHOW_PCE", showPce);
    settingsini.SetInt("SRLOADER", "UPDATE_RECENTLY_PLAYED_LIST", updateRecentlyPlayedList);
    settingsini.SetInt("SRLOADER", "SORT_METHOD", sortMethod);
    settingsini.SetInt("SRLOADER", "SHOW_DIRECTORIES", showDirectories);
    settingsini.SetInt("SRLOADER", "SHOW_HIDDEN", showHidden);
    settingsini.SetInt("SRLOADER", "PREVENT_ROM_DELETION", preventDeletion);
    settingsini.SetInt("SRLOADER", "SHOW_BOX_ART", showBoxArt);
    settingsini.SetInt("SRLOADER", "ANIMATE_DSI_ICONS", animateDsiIcons);
    if (consoleModel < 2) {
        settingsini.SetInt("SRLOADER", "SYS_REGION", sysRegion);
        settingsini.SetInt("SRLOADER", "LAUNCHER_APP", launcherApp);
    }

    settingsini.SetInt("SRLOADER", "FLASHCARD", flashcard);

    settingsini.SetInt("SRLOADER", "SLOT1_LAUNCHMETHOD", slot1LaunchMethod);
    settingsini.SetInt("SRLOADER", "BOOTSTRAP_FILE", bootstrapFile);
    settingsini.SetInt("SRLOADER", "USE_BOOTSTRAP", useBootstrap);
    settingsini.SetInt("SRLOADER", "FC_SAVE_ON_SD", fcSaveOnSd);

    // Default nds-bootstrap settings
    settingsini.SetInt("NDS-BOOTSTRAP", "LANGUAGE", gameLanguage);
    settingsini.SetInt("NDS-BOOTSTRAP", "BOOST_CPU", boostCpu);
    settingsini.SetInt("NDS-BOOTSTRAP", "BOOST_VRAM", boostVram);
    settingsini.SetInt("NDS-BOOTSTRAP", "DSI_MODE", bstrap_dsiMode);
	settingsini.SetInt("NDS-BOOTSTRAP", "EXTENDED_MEMORY", extendedMemory);

    settingsini.SetInt("NDS-BOOTSTRAP", "FORCE_SLEEP_PATCH", forceSleepPatch);
    settingsini.SetInt("NDS-BOOTSTRAP", "SOUND_FREQ", soundFreq);
    settingsini.SetInt("SRLOADER", "SLOT1_ENABLESD", slot1AccessSD);
    settingsini.SetInt("SRLOADER", "SLOT1_SCFG_UNLOCK", slot1SCFGUnlock);
    settingsini.SetInt("SRLOADER", "DSIWARE_BOOTER", dsiWareBooter);

    settingsini.SetInt("SRLOADER", "AK_VIEWMODE", ak_viewMode);
    settingsini.SetInt("SRLOADER", "AK_SCROLLSPEED", ak_scrollSpeed);
    settingsini.SetString("SRLOADER", "AK_THEME", ak_theme);
    settingsini.SetInt("SRLOADER", "AK_ZOOM_ICONS", ak_zoomIcons);

    settingsini.SetInt("SRLOADER", "SHOW_12H_CLOCK", show12hrClock);

    settingsini.SetString("SRLOADER", "R4_THEME", r4_theme);
    settingsini.SetString("SRLOADER", "DSI_THEME", dsi_theme);
    settingsini.SetString("SRLOADER", "3DS_THEME", _3ds_theme);
    settingsini.SetString("SRLOADER", "GBA_BORDER", gbaBorder);
    settingsini.SetString("SRLOADER", "UNLAUNCH_BG", unlaunchBg);
	settingsini.SetInt("SRLOADER", "UNLAUNCH_PATCH_REMOVE", removeLauncherPatches);

    //settingsini.SetInt("SRLOADER", "SNES_EMULATOR", snesEmulator);
    settingsini.SetInt("SRLOADER", "SMS_GG_IN_RAM", smsGgInRam);

    //settingsini.SetInt("TWL_FIRM", "SCREENSCALESIZE", screenScaleSize);
    settingsini.SetInt("SRLOADER", "WIDESCREEN", wideScreen);

    settingsini.SaveIniFileModified(settingsinipath);
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
