#include "BitmapFont.h"
#include "memory.h"
#include <cstdio>

#define CHAR_WIDTH(c) texcoords[(c * 4) + 2]

Font::Font(const unsigned int *bitmap, const unsigned short *palette, const unsigned short int *mapping, const unsigned int *texcoords, 
        const unsigned int imageCount, const int bmpWidth)
    : bitmap(bitmap), palette(palette), mapping(mapping), texcoords(texcoords), imageCount(imageCount), bmpWidth(bmpWidth)
{

}

/**
 * Get the index in the UV coordinate array where the letter appears
 */
unsigned int Font::getSpriteIndex(const u16 letter) {
	unsigned int spriteIndex = 0;
	long int left = 0;
	long int right = imageCount;
	long int mid = 0;

	while (left <= right)
	{
		mid = left + ((right - left) / 2);
		if (mapping[mid] == letter) {
			spriteIndex = mid;
			break;
		}

		if (mapping[mid] < letter) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}
	return spriteIndex;
}

inline u8 getHalfByteAtPos(u16 pitch, u16 hbX, u16 bY, unsigned char* array) {
	u8 byte = array[(bY * pitch) + (hbX >> 1)];
	// We have to mask either the left or right halfbyte off.
	return hbX % 2 == 0 ? byte & 0x0F : (byte & 0xF0) >> 4;
}

unsigned short Font::printCharacter(int x, int y, u16 charIndex, u16 *surface) {
    int u_off = texcoords[(charIndex * 4)]; // horizontal half-bytes
    int v_off = texcoords[(charIndex * 4) + 1]; // vertical halfbytes
    int width = texcoords[(charIndex * 4) + 2];  // width in bytes
    int height = texcoords[(charIndex * 4) + 3];
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
			u8 pal = getHalfByteAtPos(bmpWidth, u_off + j, i + v_off, (unsigned char*)bitmap);
			if (pal != 0) {
				surface[((y + i) * 256) + (j + x)] = (palette[pal] | BIT(15));
			}
        } 
    }
    return width;
}


void Font::print(int x, int y, const char *text, u16 *surface)
{
	unsigned short int fontChar;
	unsigned char lowBits;
	unsigned char highBits;
	while (*text)
	{
		lowBits = *(unsigned char*) text++;
		if (lowBits != UTF16_SIGNAL_BYTE) { // check if the lower bits is the signal bits.
			fontChar = getSpriteIndex(lowBits);
		} else {
			lowBits = *(unsigned char*) text++; // LSB
			highBits = *(unsigned char*) text++; // HSB
			u16 assembled = (u16)(lowBits | highBits << 8);
			
			fontChar = getSpriteIndex(assembled);
		}
	
		x += printCharacter(x, y, fontChar, surface);
	}
}


int Font::calcWidth(const char *text)
{
	unsigned short int fontChar;
	unsigned char lowBits;
	unsigned char highBits;
	int x = 0;

	while (*text)
	{
		lowBits = *(unsigned char*) text++;
		if (lowBits != UTF16_SIGNAL_BYTE) {
			fontChar = getSpriteIndex(lowBits);
		} else {
			lowBits = *(unsigned char*) text++;
			highBits = *(unsigned char*) text++;
			fontChar = getSpriteIndex((u16)(lowBits | highBits << 8));
		}

		x += CHAR_WIDTH(fontChar);
	}
	return x;
}


int Font::getCenteredX(const char *text)
{
	unsigned short int fontChar;
	unsigned char lowBits;
	unsigned char highBits;
	int total_width = 0;
	while (*text)
	{
		lowBits = *(unsigned char*) text++;
		if (lowBits != UTF16_SIGNAL_BYTE) {
			fontChar = getSpriteIndex(lowBits);
		} else {
			lowBits = *(unsigned char*) text++;
			highBits = *(unsigned char*) text++;
			fontChar = getSpriteIndex((u16)(lowBits | highBits << 8));
		}

		total_width += CHAR_WIDTH(fontChar);
	}
	return (SCREEN_WIDTH - total_width) / 2;
}

void Font::printCentered(int y, const char *text,  u16 *surface)
{
	unsigned short int fontChar;
	unsigned char lowBits;
	unsigned char highBits;	

	int x = getCenteredX(text);
	while (*text)
	{
		lowBits = *(unsigned char*) text++;
		if (lowBits != UTF16_SIGNAL_BYTE) {
			fontChar = getSpriteIndex(lowBits);
		} else {
			lowBits = *(unsigned char*) text++;
			highBits = *(unsigned char*) text++;
			fontChar = getSpriteIndex((u16)(lowBits | highBits << 8));
		}

		fontChar = getSpriteIndex(*(unsigned char*) text++);
		x += printCharacter(x, y, fontChar, surface);
	}
}
