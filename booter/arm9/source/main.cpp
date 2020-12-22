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
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>

#include <string.h>
#include <unistd.h>
#include <gl2d.h>

#include "graphics/graphics.h"

#include "nds_loader_arm9.h"

#include "graphics/fontHandler.h"

bool fadeType = false;		// false = out, true = in

using namespace std;

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

char filePath[PATH_MAX];

//---------------------------------------------------------------------------------
void doPause(int x, int y) {
//---------------------------------------------------------------------------------
	// iprintf("Press start...\n");
	printSmall(false, x, y, "Press start...");
	while(1) {
		scanKeys();
		if(keysDown() & KEY_START)
			break;
	}
	scanKeys();
}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	REG_SCFG_CLK &= (BIT(1) | BIT(2));	// Disable DSP and Camera Interface clocks

	// Turn on screen backlights if they're disabled
	powerOn(PM_BACKLIGHT_TOP);
	powerOn(PM_BACKLIGHT_BOTTOM);
	
	extern const DISC_INTERFACE __my_io_dsisd;

	if (!fatMountSimple("sd", &__my_io_dsisd)) {
		graphicsInit();
		fontInit();
		fadeType = true;

		clearText();
		printSmall(false, 4, 4, "fatinitDefault failed!");
		stop();
	}

	// Test code to extract device list
	// Only works, if booted via HiyaCFW or launched from 3DS HOME Menu
	/*FILE* deviceList = fopen("sd:/_nds/TWiLightMenu/deviceList.bin", "wb");
	fwrite((void*)0x02480000, 1, 0x400, deviceList);
	fclose(deviceList);*/

	bool runGame = (strncmp((char*)0x02FFFE0C, "SLRN", 4) == 0);

	const char* srldrPath = (runGame ? "sd:/_nds/TWiLightMenu/resetgame.srldr" : "sd:/_nds/TWiLightMenu/main.srldr");

	int err = runNdsFile (srldrPath, 0, NULL);

	graphicsInit();
	fontInit();
	fadeType = true;

	clearText();
	if (err == 1) {
		printSmall(false, 4, 4, "sd:/_nds/TWiLightMenu/");
		printSmall(false, 4, 12, runGame ? "resetgame.srldr not found." : "main.srldr not found.");
	} else {
		char errorText[16];
		snprintf(errorText, sizeof(errorText), "Error %i", err);
		printSmall(false, 4, 4, runGame ? "Unable to start resetgame.srldr" : "Unable to start main.srldr");
		printSmall(false, 4, 12, errorText);
	}
	printSmall(false, 4, 28, "Press B to return to menu.");
	
	while (1) {
		scanKeys();
		if (keysHeld() & KEY_B) fifoSendValue32(FIFO_USER_01, 1);	// Tell ARM7 to reboot into 3DS HOME Menu (power-off/sleep mode screen skipped)
	}

	return 0;
}
