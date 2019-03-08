#pragma once
#ifndef __TWLMENU_FONT_H_
#define __TWLMENU_FONT_H_

#define FONT_SX 8
#define FONT_SY 10
#define UTF16_SIGNAL_BYTE 0x0F

#include <nds.h>

class Font 
{
private:
	const unsigned int *bitmap;
	const unsigned short *palette;
	const unsigned short int *mapping;
	const unsigned int *texcoords;
	unsigned int imageCount;
	const int bmpWidth;

public:
	u16 printCharacter(int x, int y, u16 charIndex, u16 *surface);
	void print(int x, int y, const char *text, u16 *surface);
	void printCentered(int y, const char *text, u16 *surface);
	
	int getCenteredX(const char *text);
	int calcWidth(const char *text);

	Font(const unsigned int *bitmap, const unsigned short *palette, const unsigned short int *mapping,
	     const unsigned int *texcoords, const unsigned int imageCount, const int bmpWidth);
private:
	unsigned int getSpriteIndex(const u16 letter);
};
#endif