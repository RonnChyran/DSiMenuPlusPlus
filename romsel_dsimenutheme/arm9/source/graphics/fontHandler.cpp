/*-----------------------------------------------------------------
 Copyright (C) 2015
	Matthew Scholefield

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

#include "common/gl2d.h"
#include <list>
#include <stdio.h>
#include <nds/interrupts.h>
#include "fontHandler.h"
#include "TextEntry.h"
#include <nds.h>

// GRIT auto-genrated arrays of images
#include "small_font.h"
#include "large_font.h"

// Texture UV coords
#include "uvcoord_small_font.h"
#include "uvcoord_large_font.h"
#include "TextPane.h"
#include "ThemeTextures.h"
#include "BitmapFont.h"
#include <memory>

extern int theme;
extern int fontTextureID[2];

using namespace std;

using std::unique_ptr;

list<TextEntry> topText, bottomText;
list<TextPane> panes;

Font largeFont(large_fontBitmap, large_fontPal, large_utf16_lookup_table, large_font_texcoords, 
		LARGE_FONT_NUM_IMAGES, LARGE_FONT_BITMAP_WIDTH);
Font smallFont(small_fontBitmap, small_fontPal, small_utf16_lookup_table, small_font_texcoords, 
		SMALL_FONT_NUM_IMAGES, SMALL_FONT_BITMAP_WIDTH);


void fontInit() {
	// largeFont = new Font;
	// smallFont = new Font;
}

TextPane &createTextPane(int startX, int startY, int shownElements)
{
	if (panes.size() > 2)
		panes.pop_front();
	panes.emplace_back(startX, startY, shownElements);
	return panes.back();
}

static list<TextEntry> &getTextQueue(bool top)
{
	return top ? topText : bottomText;
}

Font &getFont(bool large)
{
	return large ? largeFont : smallFont;
}

void updateText(bool top)
{
	auto &text = getTextQueue(top);
	for (auto it = text.begin(); it != text.end(); ++it)
	{
		if (it->update())
		{
			it = text.erase(it);
			--it;
			continue;
		}
		int alpha = it->calcAlpha();
		if (alpha > 0)
		{
			// glPolyFmt(POLY_ALPHA(alpha) | POLY_CULL_NONE | POLY_ID(1));
			// if (top)
			// 	glColor(RGB15(0, 0, 0));
			getFont(it->large).print(it->x / TextEntry::PRECISION, it->y / TextEntry::PRECISION, it->message, tex().bottomTextSurface());
		}
	}
	for (auto it = panes.begin(); it != panes.end(); ++it)
	{
		if (it->update(top))
		{
			it = panes.erase(it);
			--it;
			continue;
		}
	}
}

void clearText(bool top)
{
	list<TextEntry> &text = getTextQueue(top);
	for (auto it = text.begin(); it != text.end(); ++it)
	{
		if (it->immune)
			continue;
		it = text.erase(it);
		--it;
	}
}

void clearText()
{
	clearText(true);
	clearText(false);
}

void printSmall(bool top, int x, int y, const char *message)
{
	getTextQueue(top).emplace_back(false, x, y, message);
}

void printSmallCentered(bool top, int y, const char *message)
{
	getTextQueue(top).emplace_back(false, smallFont.getCenteredX(message), y, message);
}

void printLarge(bool top, int x, int y, const char *message)
{
	getTextQueue(top).emplace_back(true, x, y, message);
}

void printLargeCentered(bool top, int y, const char *message)
{
	getTextQueue(top).emplace_back(true, largeFont.getCenteredX(message), y, message);
}

int calcSmallFontWidth(const char *text)
{
	return smallFont.calcWidth(text);
}

int calcLargeFontWidth(const char *text)
{
	return largeFont.calcWidth(text);
}

TextEntry *getPreviousTextEntry(bool top)
{
	return &getTextQueue(top).back();
}

void waitForPanesToClear()
{
	while (panes.size() > 0)
		swiWaitForVBlank();
}