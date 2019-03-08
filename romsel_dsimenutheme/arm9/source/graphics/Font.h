#pragma once

#include <nds.h>
#include "large_font.h"
#include "small_font.h"
// Texture UV coords
#include "uvcoord_small_font.h"
#include "uvcoord_large_font.h"

#define FONT_SX 8
#define FONT_SY 10
#define UTF16_SIGNAL_BYTE  0x0F

class Font
{
private:
	const unsigned short int *mapping;
    const unsigned int *texcoords;
	unsigned int imageCount;	
	unsigned int getSpriteIndex(const u16 letter);
    const unsigned int *bitmap;
    const unsigned short *palette;
    const int bmpWidth;

public:
    u16 printCharacter(int x, int y, u16 charIndex, u16 *surface);
    void print(int x, int y, const char *text, u16 *surface);

    Font(unsigned int *bitmap, unsigned short *palette, unsigned short int *mapping, unsigned int *texcoords, 
        unsigned int imageCount, int bmpWidth);
}

