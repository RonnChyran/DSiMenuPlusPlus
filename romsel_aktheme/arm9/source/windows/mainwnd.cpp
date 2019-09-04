/*
    mainwnd.cpp
    Copyright (C) 2007 Acekard, www.acekard.com
    Copyright (C) 2007-2009 somebody
    Copyright (C) 2009 yellow wood goblin

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "tool/dbgtool.h"
#include "ui/windowmanager.h"
#include "mainwnd.h"
#include "ui/msgbox.h"
#include "systemfilenames.h"

#include "time/datetime.h"
#include "time/timer.h"

#include "tool/timetool.h"
#include "tool/fifotool.h"

#include "ui/progresswnd.h"
#include "common/bootstrapconfig.h"
#include "common/loaderconfig.h"
#include "common/pergamesettings.h"
#include "common/cardlaunch.h"
#include "common/systemdetails.h"
#include "common/dsargv.h"
#include "common/flashcard.h"
#include "common/flashcardlaunch.h"
#include "common/gbaswitch.h"
#include "common/unlaunchboot.h"
#include "common/files.h"
#include "common/filecopy.h"
#include "common/nds_loader_arm9.h"

#include "common/inifile.h"
#include "language.h"
#include "common/dsimenusettings.h"
#include "windows/rominfownd.h"

#include <nds/arm9/dldi.h>
#include "sound.h"
#include <sys/iosupport.h>

#include "sr_data_srllastran.h"

using namespace akui;

MainWnd::MainWnd(s32 x, s32 y, u32 w, u32 h, Window *parent, const std::string &text)
    : Form(x, y, w, h, parent, text), _mainList(NULL), _startMenu(NULL), _startButton(NULL),
      _brightnessButton(NULL), _batteryIcon(NULL), _folderUpButton(NULL),  _folderText(NULL), _processL(false)
{
}

MainWnd::~MainWnd()
{
    delete _folderText;
    delete _folderUpButton;
    delete _brightnessButton;
    delete _batteryIcon;
    delete _startButton;
    delete _startMenu;
    delete _mainList;
    windowManager().removeWindow(this);
}

void MainWnd::init()
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    bool showBatt = 0;
    COLOR color = 0;
    std::string file("");
    std::string text("");
    CIniFile ini(SFN_UI_SETTINGS);

    // self init
    dbg_printf("mainwnd init() %08x\n", this);
    loadAppearance(SFN_LOWER_SCREEN_BG);
    windowManager().addWindow(this);

    // init game file list
    _mainList = new MainList(4, 20, 248, 152, this, "main list");
    _mainList->setRelativePosition(Point(4, 20));
    _mainList->init();
    _mainList->selectChanged.connect(this, &MainWnd::listSelChange);
    _mainList->selectedRowClicked.connect(this, &MainWnd::onMainListSelItemClicked);
    _mainList->directoryChanged.connect(this, &MainWnd::onFolderChanged);
    _mainList->animateIcons.connect(this, &MainWnd::onAnimation);

    addChildWindow(_mainList);
    dbg_printf("mainlist %08x\n", _mainList);

    //waitMs( 1000 );

    // init start button
    x = ini.GetInt("start button", "x", 0);
    y = ini.GetInt("start button", "y", 172);
    w = ini.GetInt("start button", "w", 48);
    h = ini.GetInt("start button", "h", 10);
    color = ini.GetInt("start button", "textColor", 0x7fff);
    file = ini.GetString("start button", "file", "none");
    text = ini.GetString("start button", "text", "START");
    if (file != "none")
    {
        file = SFN_UI_CURRENT_DIRECTORY + file;
    }
    if (text == "ini")
    {
        text = LANG("start menu", "START");
    }
    _startButton = new Button(x, y, w, h, this, text);
    _startButton->setStyle(Button::press);
    _startButton->setRelativePosition(Point(x, y));
    _startButton->loadAppearance(file);
    _startButton->clicked.connect(this, &MainWnd::startButtonClicked);
    _startButton->setTextColor(color | BIT(15));
    if (!ini.GetInt("start button", "show", 1))
        _startButton->hide();
    addChildWindow(_startButton);

    // // init brightness button
	
	    x = ini.GetInt("battery icon", "x", 238);
	    y = ini.GetInt("battery icon", "y", 172);
	    showBatt = ini.GetInt("battery icon", "show", 0);

	
	    if(showBatt)
	    {
            if(!ini.GetInt("battery icon", "screen", true))
            {
                _batteryIcon = new Button(x, y, w, h, this, "");
                _batteryIcon->setRelativePosition(Point(x,y));

                u8 batteryLevel = sys().batteryStatus();

				if (isDSiMode()) {
					if (batteryLevel & BIT(7)) {
						_batteryIcon->loadAppearance(SFN_BATTERY_CHARGE);
					} else if (batteryLevel == 0xF) {
						_batteryIcon->loadAppearance(SFN_BATTERY4);
					} else if (batteryLevel == 0xB) {
						_batteryIcon->loadAppearance(SFN_BATTERY3);
					} else if (batteryLevel == 0x7) {
						_batteryIcon->loadAppearance(SFN_BATTERY2);
					} else if (batteryLevel == 0x3 || batteryLevel == 0x1) {
						_batteryIcon->loadAppearance(SFN_BATTERY1);
					} else {
						_batteryIcon->loadAppearance(SFN_BATTERY_CHARGE);
					}
				} else {
					if (batteryLevel & BIT(0)) {
						_batteryIcon->loadAppearance(SFN_BATTERY1);
					} else {
						_batteryIcon->loadAppearance(SFN_BATTERY4);
					}
				}
        
                addChildWindow(_batteryIcon);
            }
	    }

    x = ini.GetInt("folderup btn", "x", 0);
    y = ini.GetInt("folderup btn", "y", 2);
    w = ini.GetInt("folderup btn", "w", 32);
    h = ini.GetInt("folderup btn", "h", 16);
    _folderUpButton = new Button(x, y, w, h, this, "");
    _folderUpButton->setRelativePosition(Point(x, y));
    _folderUpButton->loadAppearance(SFN_FOLDERUP_BUTTON);
    _folderUpButton->setSize(Size(w, h));
    _folderUpButton->pressed.connect(_mainList, &MainList::backParentDir);
    addChildWindow(_folderUpButton);

    x = ini.GetInt("folder text", "x", 20);
    y = ini.GetInt("folder text", "y", 2);
    w = ini.GetInt("folder text", "w", 160);
    h = ini.GetInt("folder text", "h", 16);
    _folderText = new StaticText(x, y, w, h, this, "");
    _folderText->setRelativePosition(Point(x, y));
    _folderText->setTextColor(ini.GetInt("folder text", "color", 0));
    addChildWindow(_folderText);
    
    if (!ms().showDirectories) {
        _folderText->hide();
    }
    // init startmenu
    _startMenu = new StartMenu(160, 40, 61, 108, this, "start menu");
    _startMenu->init();
    _startMenu->itemClicked.connect(this, &MainWnd::startMenuItemClicked);
    _startMenu->hide();
    _startMenu->setRelativePosition(_startMenu->position());
    addChildWindow(_startMenu);
    dbg_printf("startMenu %08x\n", _startMenu);

    arrangeChildren();
}

void MainWnd::draw()
{
    Form::draw();
}

void MainWnd::listSelChange(u32 i)
{
    // #ifdef DEBUG
    //     //dbg_printf( "main list item %d\n", i );
    //     DSRomInfo info;
    //     if (_mainList->getRomInfo(i, info))
    //     {
    //         char title[13] = {};
    //         memcpy(title, info.saveInfo().gameTitle, 12);
    //         char code[5] = {};
    //         memcpy(code, info.saveInfo().gameCode, 4);
    //         u16 crc = swiCRC16(0xffff, ((unsigned char *)&(info.banner())) + 32, 0x840 - 32);
    //         dbg_printf("%s %s %04x %d %04x/%04x\n",
    //                    title, code, info.saveInfo().gameCRC, info.isDSRom(), info.banner().crc, crc);
    //         //dbg_printf("sizeof banner %08x\n", sizeof( info.banner() ) );
    //     }
    // #endif //DEBUG
}

void MainWnd::startMenuItemClicked(s16 i)
{
    CIniFile ini(SFN_UI_SETTINGS);
    if(!ini.GetInt("start menu", "showFileOperations", true)) i += 4;
    
    dbg_printf("start menu item %d\n", i);

    // ------------------- Copy and Paste ---
    if (START_MENU_ITEM_COPY == i)
    {
        if (_mainList->getSelectedFullPath() == "")
            return;
        struct stat st;
        stat(_mainList->getSelectedFullPath().c_str(), &st);
        if (st.st_mode & S_IFDIR)
        {
            messageBox(this, LANG("no copy dir", "title"), LANG("no copy dir", "text"), MB_YES | MB_NO);
            return;
        }
        setSrcFile(_mainList->getSelectedFullPath(), SFM_COPY);
    }

    else if (START_MENU_ITEM_CUT == i)
    {
        if (_mainList->getSelectedFullPath() == "")
            return;
        struct stat st;
        stat(_mainList->getSelectedFullPath().c_str(), &st);
        if (st.st_mode & S_IFDIR)
        {
            messageBox(this, LANG("no copy dir", "title"), LANG("no copy dir", "text"), MB_YES | MB_NO);
            return;
        }
        setSrcFile(_mainList->getSelectedFullPath(), SFM_CUT);
    }

    else if (START_MENU_ITEM_PASTE == i)
    {
        bool ret = false;
        ret = copyOrMoveFile(_mainList->getCurrentDir());
        if (ret) // refresh current directory
            _mainList->enterDir(_mainList->getCurrentDir());
    }

    else if (START_MENU_ITEM_DELETE == i)
    {
        std::string fullPath = _mainList->getSelectedFullPath();
        if (fullPath != "")
        {
            bool ret = false;
            ret = deleteFile(fullPath);
            if (ret)
                _mainList->enterDir(_mainList->getCurrentDir());
        }
    }

    if (START_MENU_ITEM_SETTING == i)
    {
        showSettings();
    }

    else if (START_MENU_ITEM_INFO == i)
    {
        showFileInfo();
    }
}

void MainWnd::startButtonClicked()
{
    if (_startMenu->isVisible())
    {
        _startMenu->hide();
    }
    else
    {
        _startMenu->show();
    }
}

Window &MainWnd::loadAppearance(const std::string &aFileName)
{
    return *this;
}

bool MainWnd::process(const Message &msg)
{
    if (_startMenu->isVisible())
        return _startMenu->process(msg);

    bool ret = false;

    ret = Form::process(msg);

    if (!ret)
    {
        if (msg.id() > Message::keyMessageStart && msg.id() < Message::keyMessageEnd)
        {
            ret = processKeyMessage((KeyMessage &)msg);
        }

        if (msg.id() > Message::touchMessageStart && msg.id() < Message::touchMessageEnd)
        {
            ret = processTouchMessage((TouchMessage &)msg);
        }
    }
    return ret;
}

bool MainWnd::processKeyMessage(const KeyMessage &msg)
{
    bool ret = false, isL = msg.shift() & KeyMessage::UI_SHIFT_L;
    if (msg.id() == Message::keyDown)
    {
        switch (msg.keyCode())
        {
        case KeyMessage::UI_KEY_DOWN:
            _mainList->selectNext();
            ret = true;
            break;
        case KeyMessage::UI_KEY_UP:
            _mainList->selectPrev();
            ret = true;
            break;

        case KeyMessage::UI_KEY_LEFT:
            _mainList->selectRow(_mainList->selectedRowId() - _mainList->visibleRowCount());
            ret = true;
            break;

        case KeyMessage::UI_KEY_RIGHT:
            _mainList->selectRow(_mainList->selectedRowId() + _mainList->visibleRowCount());
            ret = true;
            break;
        case KeyMessage::UI_KEY_A:
            onKeyAPressed();
            ret = true;
            break;
        case KeyMessage::UI_KEY_B:
            onKeyBPressed();
            ret = true;
            break;
        case KeyMessage::UI_KEY_Y:
            if (isL)
            {
                showSettings();
                _processL = false;
            }
            else
            {
                onKeyYPressed();
            }
            ret = true;
            break;

        case KeyMessage::UI_KEY_START:
            startButtonClicked();
            ret = true;
            break;
        case KeyMessage::UI_KEY_SELECT:
            if (isL)
            {
                _mainList->SwitchShowAllFiles();
                _processL = false;
            }
            else
            {
                _mainList->setViewMode((MainList::VIEW_MODE)((_mainList->getViewMode() + 1) % 3));
                ms().ak_viewMode = _mainList->getViewMode();
                ms().saveSettings();
            }
            ret = true;
            break;
        case KeyMessage::UI_KEY_L:
            _processL = true;
            ret = true;
            break;
        case KeyMessage::UI_KEY_R:
#ifdef DEBUG
            gdi().switchSubEngineMode();
            gdi().present(GE_SUB);
#endif //DEBUG
            ret = true;
            break;
        default:
        {
        }
        };
    }
    if (msg.id() == Message::keyUp)
    {
        switch (msg.keyCode())
        {
        case KeyMessage::UI_KEY_L:
            if (_processL)
            {
                _mainList->backParentDir();
                _processL = false;
            }
            ret = true;
            break;
        }
    }
    return ret;
}

bool MainWnd::processTouchMessage(const TouchMessage &msg)
{
    return false;
}

void MainWnd::onKeyYPressed()
{
    showFileInfo();
}

void MainWnd::onMainListSelItemClicked(u32 index)
{
    onKeyAPressed();
}

void MainWnd::onKeyAPressed()
{
    cwl();
    launchSelected();
}

void bootstrapSaveHandler()
{
    progressWnd().setPercent(50);
    progressWnd().update();
}

void bootstrapLaunchHandler()
{
    progressWnd().setPercent(90);
    progressWnd().update();
}

void MainWnd::bootArgv(DSRomInfo &rominfo)
{
    std::string fullPath = _mainList->getSelectedFullPath();
    std::string launchPath = fullPath;
    std::vector<const char *> cargv{};
    snd().fadeOutStream();
    if (rominfo.isArgv())
    {
        ArgvFile argv(fullPath);
        launchPath = argv.launchPath();
        for (auto &string : argv.launchArgs())
            cargv.push_back(&string.front());
    }

    LoaderConfig config(fullPath, "");
    progressWnd().setTipText(LANG("game launch", "Please wait"));
    progressWnd().update();
    progressWnd().show();

    int err = config.launch(0, cargv.data());

    if (err)
    {
        std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
        messageBox(this, LANG("game launch", "ROM Start Error"), errorString, MB_OK);
        progressWnd().hide();
        snd().cancelFadeOutStream();
    }
}

//void MainWnd::apFix(const char *filename)
std::string apFix(const char *filename, bool isHomebrew)
{
	remove("fat:/_nds/nds-bootstrap/apFix.ips");

	if (isHomebrew) {
		return "";
	}

	bool ipsFound = false;
	char ipsPath[256];
	snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s.ips", filename);
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

		snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s-%X.ips", game_TID, headerCRC16);
		ipsFound = (access(ipsPath, F_OK) == 0);
	}

	if (ipsFound) {
		if (ms().secondaryDevice) {
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

//void MainWnd::bootWidescreen(const char *filename)
void bootWidescreen(const char *filename)
{
	remove("/_nds/nds-bootstrap/wideCheatData.bin");

	if (sys().arm7SCFGLocked() || ms().consoleModel < 2 || !ms().wideScreen
	|| (access("sd:/_nds/TWiLightMenu/TwlBg/Widescreen.cxi", F_OK) != 0)) {
		return;
	}
	
	bool wideCheatFound = false;
	char wideBinPath[256];
	if (ms().launchType == DSiMenuPlusPlusSettings::ESDFlashcardLaunch) {
		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s.bin", filename);
		wideCheatFound = (access(wideBinPath, F_OK) == 0);
	}

	if (ms().launchType == DSiMenuPlusPlusSettings::ESlot1) {
		// Reset Slot-1 to allow reading card header
		sysSetCardOwner (BUS_OWNER_ARM9);
		disableSlot1();
		for(int i = 0; i < 25; i++) { swiWaitForVBlank(); }
		enableSlot1();
		for(int i = 0; i < 15; i++) { swiWaitForVBlank(); }

		cardReadHeader((uint8*)&ndsCart);

		char game_TID[5];
		memcpy(game_TID, ndsCart.gameCode, 4);
		game_TID[4] = 0;

		snprintf(wideBinPath, sizeof(wideBinPath), "sd:/_nds/TWiLightMenu/widescreen/%s-%X.bin", game_TID, ndsCart.headerCRC16);
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

	if (wideCheatFound) {
		//const char* resultText;
		mkdir("/_nds", 0777);
		mkdir("/_nds/nds-bootstrap", 0777);
		if (fcopy(wideBinPath, "/_nds/nds-bootstrap/wideCheatData.bin") == 0) {
			// Prepare for reboot into 16:10 TWL_FIRM
			mkdir("sd:/luma", 0777);
			mkdir("sd:/luma/sysmodules", 0777);
			if ((access("sd:/luma/sysmodules/TwlBg.cxi", F_OK) == 0)
			&& (rename("sd:/luma/sysmodules/TwlBg.cxi", "sd:/luma/sysmodules/TwlBg_bak.cxi") != 0)) {
				//resultText = "Failed to backup custom TwlBg.";
			} else {
				if (rename("sd:/_nds/TWiLightMenu/TwlBg/Widescreen.cxi", "sd:/luma/sysmodules/TwlBg.cxi") == 0) {
					irqDisable(IRQ_VBLANK);				// Fix the throwback to 3DS HOME Menu bug
					memcpy((u32 *)0x02000300, sr_data_srllastran, 0x020);
					fifoSendValue32(FIFO_USER_02, 1); // Reboot in 16:10 widescreen
					swiWaitForVBlank();
				} else {
					//resultText = "Failed to reboot TwlBg in widescreen.";
				}
			}
			rename("sd:/luma/sysmodules/TwlBg_bak.cxi", "sd:/luma/sysmodules/TwlBg.cxi");
		} else {
			//resultText = "Failed to copy widescreen code for the game.";
		}
		remove("/_nds/nds-bootstrap/wideCheatData.bin");
		//messageBox(this, LANG("game launch", "Widescreen Error"), resultText, MB_OK);	// Does not work
	}
}

void MainWnd::bootBootstrap(PerGameSettings &gameConfig, DSRomInfo &rominfo)
{
    snd().fadeOutStream();
    dbg_printf("%s", _mainList->getSelectedShowName().c_str());
    std::string fileName = _mainList->getSelectedShowName();
    std::string fullPath = _mainList->getSelectedFullPath();

    BootstrapConfig config(fileName, fullPath, std::string((char *)rominfo.saveInfo().gameCode), rominfo.saveInfo().gameSdkVersion);

    config.dsiMode(gameConfig.dsiMode == PerGameSettings::EDefault ? ms().bstrap_dsiMode : (int)gameConfig.dsiMode)
		  .saveNo((int)gameConfig.saveNo)
		  .ramDiskNo((int)gameConfig.ramDiskNo)
		  .cpuBoost(gameConfig.boostCpu == PerGameSettings::EDefault ? ms().boostCpu : (bool)gameConfig.boostCpu)
		  .vramBoost(gameConfig.boostVram == PerGameSettings::EDefault ? ms().boostVram : (bool)gameConfig.boostVram)
		  .nightlyBootstrap(gameConfig.bootstrapFile == PerGameSettings::EDefault ? ms().bootstrapFile : (bool)gameConfig.bootstrapFile);

    // GameConfig is default, global is not default
    if (gameConfig.language == PerGameSettings::ELangDefault && ms().bstrap_language != DSiMenuPlusPlusSettings::ELangDefault)
    {
        config.language(ms().bstrap_language);
    }
    // GameConfig is system, or global is defaut
    else if (gameConfig.language == PerGameSettings::ELangSystem || ms().bstrap_language == DSiMenuPlusPlusSettings::ELangDefault)
    {
        config.language(PersonalData->language);
    }
    else
    // gameConfig is not default
    {
        config.language(gameConfig.language);
    }

	bool hasAP = false;
    PerGameSettings settingsIni(_mainList->getSelectedShowName().c_str());

	char gameTid[5] = {0};
	snprintf(gameTid, 4, "%s", rominfo.saveInfo().gameCode);

	char ipsPath[256];
	snprintf(ipsPath, sizeof(ipsPath), "sd:/_nds/TWiLightMenu/apfix/%s-%X.ips", gameTid, rominfo.saveInfo().gameCRC);

	if (settingsIni.checkIfShowAPMsg() && (access(ipsPath, F_OK) != 0)) {
		// Check for SDK4-5 ROMs that don't have AP measures.
		if ((memcmp(rominfo.saveInfo().gameCode, "AZLJ", 4) == 0)   // Girls Mode (JAP version of Style Savvy)
		 || (memcmp(rominfo.saveInfo().gameCode, "YEEJ", 4) == 0)   // Inazuma Eleven (J)
		 || (memcmp(rominfo.saveInfo().gameCode, "VSO",  3) == 0)   // Sonic Classic Collection
		 || (memcmp(rominfo.saveInfo().gameCode, "B2D",  3) == 0)   // Doctor Who: Evacuation Earth
		 || (memcmp(rominfo.saveInfo().gameCode, "BWB",  3) == 0)	  // Plants vs Zombies
		 || (memcmp(rominfo.saveInfo().gameCode, "VDX",  3) == 0)	  // Daniel X: The Ultimate Power
		 || (memcmp(rominfo.saveInfo().gameCode, "BUD",  3) == 0)	  // River City Super Sports Challenge
		 || (memcmp(rominfo.saveInfo().gameCode, "B3X",  3) == 0)	  // River City Soccer Hooligans
		 || (memcmp(rominfo.saveInfo().gameCode, "BZX",  3) == 0)	  // Puzzle Quest 2
		 || (memcmp(rominfo.saveInfo().gameCode, "BRFP", 4) == 0)	  // Rune Factory 3 - A Fantasy Harvest Moon
		 || (memcmp(rominfo.saveInfo().gameCode, "BDX",  3) == 0)   // Minna de Taikan Dokusho DS: Choo Kowaai!: Gakkou no Kaidan
		 || (memcmp(rominfo.saveInfo().gameCode, "TFB",  3) == 0)   // Frozen: Olaf's Quest
		 || (memcmp(rominfo.saveInfo().gameCode, "B88",  3) == 0))  // DS WiFi Settings
		{
			hasAP = false;
		}
		else
		// Check for ROMs that have AP measures.
		if ((memcmp(rominfo.saveInfo().gameCode, "B", 1) == 0)
		 || (memcmp(rominfo.saveInfo().gameCode, "T", 1) == 0)
		 || (memcmp(rominfo.saveInfo().gameCode, "V", 1) == 0)) {
			hasAP = true;
		} else {
			static const char ap_list[][4] = {
				"ABT",	// Bust-A-Move DS
				"YHG",	// Houkago Shounen
				"YWV",	// Taiko no Tatsujin DS: Nanatsu no Shima no Daibouken!
				"AS7",	// Summon Night: Twin Age
				"YFQ",	// Nanashi no Geemu
				"AFX",	// Final Fantasy Crystal Chronicles: Ring of Fates
				"YV5",	// Dragon Quest V: Hand of the Heavenly Bride
				"CFI",	// Final Fantasy Crystal Chronicles: Echoes of Time
				"CCU",	// Tomodachi Collection
				"CLJ",	// Mario & Luigi: Bowser's Inside Story
				"YKG",	// Kindgom Hearts: 358/2 Days
				"COL",	// Mario & Sonic at the Olympic Winter Games
				"C24",	// Phantasy Star 0
				"AZL",	// Style Savvy
				"CS3",	// Sonic and Sega All Stars Racing
				"IPK",	// Pokemon HeartGold Version
				"IPG",	// Pokemon SoulSilver Version
				"YBU",	// Blue Dragon: Awakened Shadow
				"YBN",	// 100 Classic Books
				"YVI",	// Dragon Quest VI: Realms of Revelation
				"YDQ",	// Dragon Quest IX: Sentinels of the Starry Skies
				"C3J",	// Professor Layton and the Unwound Future
				"IRA",	// Pokemon Black Version
				"IRB",	// Pokemon White Version
				"CJR",	// Dragon Quest Monsters: Joker 2
				"YEE",	// Inazuma Eleven
				"UZP",	// Learn with Pokemon: Typing Adventure
				"IRE",	// Pokemon Black Version 2
				"IRD",	// Pokemon White Version 2
			};

			// TODO: If the list gets large enough, switch to bsearch().
			for (unsigned int i = 0; i < sizeof(ap_list)/sizeof(ap_list[0]); i++) {
				if (memcmp(rominfo.saveInfo().gameCode, ap_list[i], 3) == 0) {
					// Found a match.
					hasAP = true;
					break;
				}
			}

		}

        int optionPicked = 0;

		if (hasAP)
		{
			optionPicked = messageBox(this, "Warning", "This game may not work correctly, if it's not AP-patched. "
										"If the game freezes, does not start, or doesn't seem normal, "
										"it needs to be AP-patched.", MB_OK | MB_HOLD_X | MB_CANCEL);
		}

		scanKeys();
		int pressed = keysHeld();

		if (pressed & KEY_X || optionPicked == ID_HOLD_X)
		{
			settingsIni.dontShowAPMsgAgain();
		}
		if (!hasAP || pressed & KEY_A || pressed & KEY_X || optionPicked == ID_HOLD_X || optionPicked == ID_OK)
		{
			// Event handlers for progress window.
			config.onSaveCreated(bootstrapSaveHandler)
				.onConfigSaved(bootstrapLaunchHandler);

			progressWnd().setTipText(LANG("game launch", "Please wait"));
			progressWnd().update();
			progressWnd().show();

			int err = config.launch();
			if (err)
			{
				std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
				messageBox(this, LANG("game launch", "NDS Bootstrap Error"), errorString, MB_OK);
				progressWnd().hide();
                snd().cancelFadeOutStream();
			}
		}
	} else {
		// Event handlers for progress window.
		config.onSaveCreated(bootstrapSaveHandler)
			.onConfigSaved(bootstrapLaunchHandler);

		progressWnd().setTipText(LANG("game launch", "Please wait"));
		progressWnd().update();
		progressWnd().show();

		int err = config.launch();
		if (err)
		{
			std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
			messageBox(this, LANG("game launch", "NDS Bootstrap Error"), errorString, MB_OK);
			progressWnd().hide();
            snd().cancelFadeOutStream();
		}
	}
}

void MainWnd::bootFlashcard(const std::string &ndsPath, const std::string &filename, bool usePerGameSettings)
{
    snd().fadeOutStream();
    int err = loadGameOnFlashcard(ndsPath.c_str(), filename, usePerGameSettings);
    if (err)
    {
        std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
        messageBox(this, LANG("game launch", "Flashcard Error"), errorString, MB_OK);
        snd().cancelFadeOutStream();
    }
}

void MainWnd::bootFile(const std::string &loader, const std::string &fullPath)
{
    snd().fadeOutStream();
    LoaderConfig config(loader, "");
    std::vector<const char *> argv{};
    argv.emplace_back(loader.c_str());
    argv.emplace_back(fullPath.c_str());
    int err = config.launch(argv.size(), argv.data());
    if (err)
    {
        std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
        messageBox(this, LANG("game launch", "Launch Error"), errorString, MB_OK);
        progressWnd().hide();
        snd().cancelFadeOutStream();
    }
}

void MainWnd::launchSelected()
{
    cwl();
    dbg_printf("Launch.");
    std::string fullPath = _mainList->getSelectedFullPath();

    cwl();
    if (fullPath[fullPath.size() - 1] == '/')
    {
        cwl();
        _mainList->enterDir(fullPath);
        return;
    }

	if (!ms().gotosettings && ms().consoleModel < 2 && ms().previousUsedDevice && bothSDandFlashcard()) {
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

    ms().romfolder[ms().secondaryDevice] = _mainList->getCurrentDir();
	ms().previousUsedDevice = ms().secondaryDevice;
    ms().romPath = fullPath;
    ms().saveSettings();

    DSRomInfo rominfo;
    if (!_mainList->getRomInfo(_mainList->selectedRowId(), rominfo)) {
        return;
	}

	chdir(_mainList->getCurrentDir().c_str());

    // Launch DSiWare
    if (rominfo.isDSiWare() && rominfo.isArgv())
    {
		if (ms().consoleModel >= 2) {
			messageBox(this, LANG("game launch", "ROM Start Error"), "Cannot run this on 3DS.", MB_OK);
			return;
		}

        ms().launchType = DSiMenuPlusPlusSettings::ENoLaunch;
        ms().saveSettings();
        dsiLaunch(rominfo.saveInfo().dsiTid);
        return;
    }

    if (!rominfo.isHomebrew() && rominfo.isDSiWare() && isDSiMode())
    {
		if (ms().consoleModel >= 2) {
			messageBox(this, LANG("game launch", "ROM Start Error"), "Cannot run this on 3DS.", MB_OK);
			return;
		}

        snd().fadeOutStream();

        // Unlaunch boot here....
        UnlaunchBoot unlaunch(fullPath, rominfo.saveInfo().dsiPubSavSize, rominfo.saveInfo().dsiPrvSavSize);

        // Roughly associated with 50%, 90%
        unlaunch.onPrvSavCreated(bootstrapSaveHandler)
            .onPubSavCreated(bootstrapLaunchHandler);

            
        progressWnd().setPercent(0);
        progressWnd().setTipText(LANG("game launch", "Preparing Unlaunch Boot"));
        progressWnd().update();
        progressWnd().show();

        if (unlaunch.prepare())
        {
			progressWnd().hide();
            messageBox(this, LANG("game launch", "unlaunch boot"), LANG("game launch", "unlaunch instructions"), MB_OK);
        }
        ms().launchType = DSiMenuPlusPlusSettings::EDSiWareLaunch;
        ms().saveSettings();
        progressWnd().hide();
        unlaunch.launch();
    }

    if (rominfo.isDSRom())
    {
        PerGameSettings gameConfig(_mainList->getSelectedShowName());
        // Direct Boot for homebrew.
        if (rominfo.isDSiWare() || (gameConfig.directBoot && rominfo.isHomebrew()))
        {
			ms().launchType = DSiMenuPlusPlusSettings::ESDFlashcardDirectLaunch;
			ms().saveSettings();
            bootArgv(rominfo);
            return;
        }

        else if (ms().useBootstrap || !ms().secondaryDevice)
        {
			ms().launchType = DSiMenuPlusPlusSettings::ESDFlashcardLaunch;
			ms().saveSettings();
            bootBootstrap(gameConfig, rominfo);
            return;
        }
        else
        {
			ms().launchType = DSiMenuPlusPlusSettings::ESDFlashcardLaunch;
			ms().saveSettings();
            dbg_printf("Flashcard Launch: %s\n", fullPath.c_str());
            bootFlashcard(fullPath, _mainList->getSelectedShowName(), true);
            return;
        }
    }

    std::string extension;
    size_t lastDotPos = fullPath.find_last_of('.');
    if (fullPath.npos != lastDotPos)
        extension = fullPath.substr(lastDotPos);

    // DSTWO Plugin Launch
    if (extension == ".plg" && ms().secondaryDevice && memcmp(io_dldi_data->friendlyName, "DSTWO(Slot-1)", 0xD) == 0)
    {
        ms().launchType = DSiMenuPlusPlusSettings::ESDFlashcardLaunch;
        ms().saveSettings();

		// Print .plg path without "fat:" at the beginning
		char ROMpathDS2[256];
		for (int i = 0; i < 252; i++) {
			ROMpathDS2[i] = fullPath[4+i];
			if (fullPath[4+i] == '\x00') break;
		}

		CIniFile dstwobootini( "fat:/_dstwo/twlm.ini" );
		dstwobootini.SetString("boot_settings", "file", ROMpathDS2);
		dstwobootini.SaveIniFile( "fat:/_dstwo/twlm.ini" );

        bootFile(BOOTPLG_SRL, fullPath);
	}

	const char *ndsToBoot;

    // RVID Launch
    if (extension == ".rvid")
    {
        ms().launchType = DSiMenuPlusPlusSettings::ESDFlashcardLaunch;
        ms().saveSettings();

		ndsToBoot = RVIDPLAYER_SD;
		if(access(ndsToBoot, F_OK) != 0) {
			ndsToBoot = RVIDPLAYER_FC;
		}

        bootFile(ndsToBoot, fullPath);
    }

    // NES Launch
    if (extension == ".nes" || extension == ".fds")
    {
        ms().launchType = DSiMenuPlusPlusSettings::ENESDSLaunch;
        ms().saveSettings();

		ndsToBoot = (ms().secondaryDevice ? NESDS_SD : NESTWL_SD);
		if(access(ndsToBoot, F_OK) != 0) {
			ndsToBoot = NESDS_FC;
		}

        bootFile(ndsToBoot, fullPath);
    }

    // GB Launch
    if (extension == ".gb" || extension == ".gbc")
    {
        ms().launchType = DSiMenuPlusPlusSettings::EGameYobLaunch;
        ms().saveSettings();

		ndsToBoot = GAMEYOB_SD;
		if(access(ndsToBoot, F_OK) != 0) {
			ndsToBoot = GAMEYOB_FC;
		}

        bootFile(ndsToBoot, fullPath);
    }

    // SMS/GG Launch
    if (extension == ".sms" || extension == ".gg")
    {
        ms().launchType = DSiMenuPlusPlusSettings::ES8DSLaunch;
        ms().saveSettings();
		mkdir(ms().secondaryDevice ? "fat:/data" : "sd:/data", 0777);
		mkdir(ms().secondaryDevice ? "fat:/data/s8ds" : "sd:/data/s8ds", 0777);

		ndsToBoot = S8DS_ROM;
		if(access(ndsToBoot, F_OK) != 0) {
			ndsToBoot = S8DS_FC;
		}

        bootFile(ndsToBoot, fullPath);
    }
	
    // GEN Launch
    if (extension == ".gen")
	{
        ms().launchType = DSiMenuPlusPlusSettings::ESDFlashcardLaunch;
        ms().saveSettings();
		if (ms().secondaryDevice)
        {
			ndsToBoot = JENESISDS_ROM;
			if(access(ndsToBoot, F_OK) != 0) {
				ndsToBoot = JENESISDS_FC;
			}

            bootFile(ndsToBoot, fullPath);
		}
		else
		{
			std::string bootstrapPath = (ms().bootstrapFile ? BOOTSTRAP_NIGHTLY_HB : BOOTSTRAP_RELEASE_HB);

			std::vector<char*> argarray;
			argarray.push_back(strdup(bootstrapPath.c_str()));
			argarray.at(0) = (char*)bootstrapPath.c_str();

			LoaderConfig gen(bootstrapPath, BOOTSTRAP_INI);
			gen.option("NDS-BOOTSTRAP", "NDS_PATH", JENESISDS_ROM)
			   .option("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/ROM.BIN")
			   .option("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", fullPath)
			   .option("NDS-BOOTSTRAP", "LANGUAGE", ms().bstrap_language)
			   .option("NDS-BOOTSTRAP", "DSI_MODE", 0)
			   .option("NDS-BOOTSTRAP", "BOOST_CPU", 1)
			   .option("NDS-BOOTSTRAP", "BOOST_VRAM", 0);
			if (int err = gen.launch(argarray.size(), (const char **)&argarray[0], false))
			{
				std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
				messageBox(this, LANG("game launch", "nds-bootstrap error"), errorString, MB_OK);
			}
		}
	}

    // SNES Launch
    if (extension == ".smc" || extension == ".sfc")
	{
        ms().launchType = DSiMenuPlusPlusSettings::ESDFlashcardLaunch;
        ms().saveSettings();
		if (ms().secondaryDevice)
        {
			ndsToBoot = SNEMULDS_ROM;
			if(access(ndsToBoot, F_OK) != 0) {
				ndsToBoot = SNEMULDS_FC;
			}

            bootFile(ndsToBoot, fullPath);
		}
		else
		{
			std::string bootstrapPath = (ms().bootstrapFile ? BOOTSTRAP_NIGHTLY_HB : BOOTSTRAP_RELEASE_HB);

			std::vector<char*> argarray;
			argarray.push_back(strdup(bootstrapPath.c_str()));
			argarray.at(0) = (char*)bootstrapPath.c_str();

			LoaderConfig snes(bootstrapPath, BOOTSTRAP_INI);
			snes.option("NDS-BOOTSTRAP", "NDS_PATH", SNEMULDS_ROM)
				.option("NDS-BOOTSTRAP", "HOMEBREW_ARG", "fat:/snes/ROM.SMC")
				.option("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", fullPath)
			    .option("NDS-BOOTSTRAP", "LANGUAGE", ms().bstrap_language)
			    .option("NDS-BOOTSTRAP", "DSI_MODE", 0)
				.option("NDS-BOOTSTRAP", "BOOST_CPU", 0)
			    .option("NDS-BOOTSTRAP", "BOOST_VRAM", 0);
			if (int err = snes.launch(argarray.size(), (const char **)&argarray[0], false))
			{
				std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
				messageBox(this, LANG("game launch", "nds-bootstrap error"), errorString, MB_OK);
			}
		}
	}
}

void MainWnd::onKeyBPressed()
{
    _mainList->backParentDir();
}

void MainWnd::showSettings(void)
{
    dbg_printf("Launch settings...");
	if (sdFound()) {
		chdir("sd:/");
	}
    LoaderConfig settingsLoader(DSIMENUPP_SETTINGS_SRL, DSIMENUPP_INI);

    if (int err = settingsLoader.launch())
    {
        std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
        messageBox(this, LANG("game launch", "NDS Bootstrap Error"), errorString, MB_OK);
        snd().cancelFadeOutStream();
    }
}

void MainWnd::showManual(void)
{
    dbg_printf("Launch manual...");
	if (sdFound()) {
		chdir("sd:/");
	}
    LoaderConfig manualLoader(TWLMENUPP_MANUAL_SRL, DSIMENUPP_INI);

    if (int err = manualLoader.launch())
    {
        std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
        messageBox(this, LANG("game launch", "NDS Bootstrap Error"), errorString, MB_OK);
    }
}

void MainWnd::bootSlot1(void)
{
    snd().fadeOutStream();
    dbg_printf("Launch Slot1..\n");
    ms().launchType = DSiMenuPlusPlusSettings::ESlot1;
    ms().saveSettings();

    if (!ms().slot1LaunchMethod || sys().arm7SCFGLocked())
    {
        cardLaunch();
        return;
    }

	bootWidescreen(NULL);
	if (sdFound()) {
		chdir("sd:/");
	}
    LoaderConfig slot1Loader(SLOT1_SRL, DSIMENUPP_INI);
    if (int err = slot1Loader.launch())
    {
        std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
        messageBox(this, LANG("game launch", "nds-bootstrap error"), errorString, MB_OK);
        snd().cancelFadeOutStream();
    }
}

void MainWnd::bootGbaRunner(void)
{
    snd().fadeOutStream();
	if (ms().useGbarunner) {
	if ((access(ms().secondaryDevice ? "fat:/bios.bin" : "sd:/bios.bin", F_OK) != 0)
	&& (access(ms().secondaryDevice ? "fat:/gba/bios.bin" : "sd:/gba/bios.bin", F_OK) != 0)
	&& (access(ms().secondaryDevice ? "fat:/_gba/bios.bin" : "sd:/_gba/bios.bin", F_OK) != 0)) {
        messageBox(this, LANG("game launch", "GBARunner2 Error"), "BINF: bios.bin not found", MB_OK);
        snd().cancelFadeOutStream();
		return;
	}
	}

    if (ms().secondaryDevice && ms().useGbarunner)
    {
		if (ms().useBootstrap)
		{
            bootFile(ms().gbar2WramICache ? GBARUNNER_IWRAMCACHE_FC : GBARUNNER_FC, "");
		}
		else
		{
			bootFlashcard(ms().gbar2WramICache ? GBARUNNER_IWRAMCACHE_FC : GBARUNNER_FC, "", false);
		}
        return;
    }

    if (!isDSiMode() && !ms().useGbarunner)
    {
        gbaSwitch();
        return;
    }

	std::string bootstrapPath = (ms().bootstrapFile ? BOOTSTRAP_NIGHTLY_HB : BOOTSTRAP_RELEASE_HB);

	std::vector<char*> argarray;
	argarray.push_back(strdup(bootstrapPath.c_str()));
	argarray.at(0) = (char*)bootstrapPath.c_str();

    LoaderConfig gbaRunner(bootstrapPath, BOOTSTRAP_INI);
	gbaRunner.option("NDS-BOOTSTRAP", "NDS_PATH", ms().gbar2WramICache ? GBARUNNER_IWRAMCACHE_SD : GBARUNNER_SD)
			 .option("NDS-BOOTSTRAP", "HOMEBREW_ARG", "")
			 .option("NDS-BOOTSTRAP", "RAM_DRIVE_PATH", "")
			 .option("NDS-BOOTSTRAP", "LANGUAGE", ms().bstrap_language)
			 .option("NDS-BOOTSTRAP", "DSI_MODE", 0)
			 .option("NDS-BOOTSTRAP", "BOOST_CPU", 1)
			 .option("NDS-BOOTSTRAP", "BOOST_VRAM", 0);
    if (int err = gbaRunner.launch(argarray.size(), (const char **)&argarray[0], false))
    {
        std::string errorString = formatString(LANG("game launch", "error").c_str(), err);
        messageBox(this, LANG("game launch", "NDS Bootstrap Error"), errorString, MB_OK);
        snd().cancelFadeOutStream();
    }
}

void MainWnd::showFileInfo()
{
    DSRomInfo rominfo;
    if (!_mainList->getRomInfo(_mainList->selectedRowId(), rominfo))
    {
        return;
    }

    dbg_printf("show '%s' info\n", _mainList->getSelectedFullPath().c_str());

    CIniFile ini(SFN_UI_SETTINGS); //(256-)/2,(192-128)/2, 220, 128
    u32 w = 240;
    u32 h = 144;
    w = ini.GetInt("rom info window", "w", w);
    h = ini.GetInt("rom info window", "h", h);

    RomInfoWnd *romInfoWnd = new RomInfoWnd((256 - w) / 2, (192 - h) / 2, w, h, this, LANG("rom info", "title"));
    std::string showName = _mainList->getSelectedShowName();
    std::string fullPath = _mainList->getSelectedFullPath();
    romInfoWnd->setFileInfo(fullPath, showName);
    romInfoWnd->setRomInfo(rominfo);
    romInfoWnd->doModal();
    rominfo = romInfoWnd->getRomInfo();
    _mainList->setRomInfo(_mainList->selectedRowId(), rominfo);

    delete romInfoWnd;
}

void MainWnd::onFolderChanged()
{
    resetInputIdle();
    std::string dirShowName = _mainList->getCurrentDir();

    if (dirShowName.substr(0, 1) == SD_ROOT)
        dirShowName.replace(0, 1, "sd:/");

    if (!strncmp(dirShowName.c_str(), "^*::", 2))
    {

        if (dirShowName == SPATH_TITLEANDSETTINGS)
        {
            showSettings();
        }

        if (dirShowName == SPATH_MANUAL)
        {
            showManual();
        }

        if (dirShowName == SPATH_SLOT1)
        {
            bootSlot1();
        }

        if (dirShowName == SPATH_GBARUNNER)
        {
            bootGbaRunner();
        }

        if (dirShowName == SPATH_SYSMENU)
        {
            dsiSysMenuLaunch();
        }

        if (dirShowName == SPATH_SYSTEMSETTINGS)
        {
            dsiLaunchSystemSettings();
        }
        dirShowName.clear();
    }

    dbg_printf("%s\n", _mainList->getSelectedFullPath().c_str());

    _folderText->setText(dirShowName);
}

void MainWnd::onAnimation(bool &anAllow)
{
    if (_startMenu->isVisible())
        anAllow = false;
    else if (windowManager().currentWindow() != this)
        anAllow = false;
}

Window *MainWnd::windowBelow(const Point &p)
{
    Window *wbp = Form::windowBelow(p);
    if (_startMenu->isVisible() && wbp != _startButton)
        wbp = _startMenu;
    return wbp;
}
