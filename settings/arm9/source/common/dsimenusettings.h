
#include <nds.h>
#include <string>
#include "common/singleton.h"

#pragma once
#ifndef _DSIMENUPPSETTINGS_H_
#define _DSIMENUPPSETTINGS_H_

/**
 * Multi use class for DSiMenuPlusPlus INI file.
 * 
 * Try not to change settings that are not related to the current theme.
 */
class TWLSettings
{
  public:
    enum TScrollSpeed
    {
        EScrollFast = 4,
        EScrollMedium = 10,
        EScrollSlow = 16
    };

    enum TViewMode
    {
        EViewList = 0,
        EViewIcon = 1,
        EViewInternal = 2
    };

    /*
        0: DSTT/R4i Gold/R4i-SDHC/R4 SDHC Dual-Core/R4 SDHC Upgrade/SC DSONE
        1: R4DS (Original Non-SDHC version)/ M3 Simply
        2: R4iDSN/R4i Gold RTS/R4 Ultra
        3: Acekard 2(i)/Galaxy Eagle/M3DS Real
        4: Acekard RPG
        5: Ace 3DS+/Gateway Blue Card/R4iTT
        6: SuperCard DSTWO
    */

    enum TFlashCard
    {
        EDSTTClone = 0,
        ER4Original = 1,
        ER4iGoldClone = 2,
        EAcekard2i = 3,
        EAcekardRPG = 4,
        EGatewayBlue = 5,
        ESupercardDSTWO = 6
    };

    // Do not reorder these, just add to the end
    enum TLanguage
    {
        ELangDefault = -1,
        ELangJapanese = 0,
        ELangEnglish = 1,
        ELangFrench = 2,
        ELangGerman = 3,
        ELangItalian = 4,
        ELangSpanish = 5,
        ELangChineseS = 6,
        ELangKorean = 7,
        ELangChineseT = 8,
        ELangPolish = 9,
        ELangPortuguese = 10,
        ELangRussian = 11,
        ELangSwedish = 12,
        ELangDanish = 13,
        ELangTurkish = 14,
        ELangUkrainian = 15,
        ELangHungarian = 16,
    };

    enum TRunIn
    {
        EDSMode = 0,
        EDSiMode = 1,
        EDSiModeForced = 2
    };

    enum TSlot1LaunchMethod
    {
        EReboot = 0,
        EDirect = 1
    };

    enum TBootstrapFile
    {
        EReleaseBootstrap = false,
        ENightlyBootstrap = true
    };

    /*	0 = Nintendo DSi (Retail)
    1 = Nintendo DSi (Dev/Panda)
    2 = Nintendo 3DS
    3 = New Nintendo 3DS	*/
    enum TConsoleModel
    {
        EDSiRetail = 0,
        EDSiDebug = 1,
        E3DSOriginal = 2,
        E3DSNew = 3
    };

    enum TSoundFreq
    {
        EFreq32KHz = 0,
        EFreq47KHz = 1
    };

  public:
    TWLSettings();
    ~TWLSettings();

  public:
    void loadSettings();
    void saveSettings();

    TLanguage getGuiLanguage();
    const char* getAppName();
  public:
    std::string romfolder;
    int pagenum;
    int cursorPosition;
    int startMenu_cursorPosition;
    int consoleModel;
    int guiLanguage;
    int titleLanguage;
    int fps;
    int colorMode;
    int blfLevel;
    int dsiWareExploit;
    bool wifiLed;
    bool sdRemoveDetect;
    bool showMicroSd;
    bool gbar2DldiAccess;
    bool showMainMenu;
    bool showSelectMenu;
    int theme;
    int subtheme;
    int settingsMusic;
    int dsiMusic;
	bool boxArtColorDeband;
    bool showNds;
    int showGba;
    bool showRvid;
    bool showA26;
	bool showA78;
    bool showNes;
    bool showGb;
    bool showSmsGg;
    int showMd;
    bool showSnes;
    bool showPce;
    bool updateRecentlyPlayedList;
    int sortMethod;
    bool showDirectories;
    bool showHidden;
    bool preventDeletion;
    int showBoxArt;
    bool animateDsiIcons;
    int sysRegion;
    int launcherApp;
    bool secondaryAccess;
    bool previousUsedDevice;
    bool secondaryDevice;
    bool fcSaveOnSd;

    int flashcard;
    int slot1LaunchMethod;
    bool useBootstrap;
    bool bootstrapFile;

    int gameLanguage;
    bool boostCpu;
    bool boostVram;
    int bstrap_dsiMode;
    int extendedMemory;

    bool forceSleepPatch;
    bool slot1AccessSD;
    bool slot1SCFGUnlock;
    bool dsiWareBooter;
    bool autorun;
	bool autostartSlot1;
    bool show12hrClock;
    //bool snesEmulator;
    bool smsGgInRam;

    int ak_viewMode;
    int ak_scrollSpeed;
    bool ak_zoomIcons;
    std::string ak_theme;

    std::string dsiWareSrlPath;
    std::string dsiWarePubPath;
    std::string dsiWarePrvPath;

    int launchType;
    std::string homebrewArg;
    bool homebrewBootstrap;
    bool soundFreq;
    int dsiSplash;
	int nintendoLogoColor;
    bool showlogo;

    std::string r4_theme;
    std::string dsi_theme;
    std::string _3ds_theme;
    std::string gbaBorder;
    std::string unlaunchBg;
    bool removeLauncherPatches;

    //int screenScaleSize;
    bool wideScreen;
};

typedef singleton<TWLSettings> menuSettings_s;
inline TWLSettings &ms() { return menuSettings_s::instance(); }

#endif //_DSIMENUPPSETTINGS_H_
