#include "Font.h"



Font::Font(unsigned int *bitmap, unsigned short *palette, unsigned short int *mapping, unsigned int *texcoords, 
    unsigned int imageCount, int bmpWidth)
    : bitmap(bitmap), palette(palette), mapping(mapping), texcoords(texcoords), imageCount(imageCount), bmpWidth(bmpWidth >> 1)
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

unsigned short Font::printCharacter(int x, int y, u16 charIndex, u16 *surface) {
    int u_off = texcoords[charIndex]; // horizontal half-bytes
    int v_off = texcoords[charIndex + 1]; // vertical halfbytes
    int width = texcoords[charIndex + 2];  // width in bytes
    int height = texcoords[charIndex + 3];

    // int x1 = x;
	// int y1 = y;
	// int x2 = x + width; // bits to blit
	// int y2 = y + height;

//(u_off >> 1)
    unsigned char* start = (((unsigned char*)bitmap) + ((bmpWidth * v_off) + (u_off >> 1)));
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < (width >> 1); j++) {
            // row = bmpWidth * (i + v_off)
            // bytecol = (u_off >> 1) +  
            u8 colorByte = ((unsigned char*)bitmap)[(bmpWidth * (i + v_off)) + ((u_off >> 1) + j)];
            u8 rBit = (colorByte & 0x0F);
            u8 lBit = (colorByte & 0xF0) >> 4;
            if (lBit != 0) {
                surface[y * 256 + (i + x)] = palette[lBit];
            }
            if (++x > width) break;

            if (rBit != 0) {
                surface[y * 256 + (i + x)] = palette[rBit];
            }
            x++;
            // surface[y * 256 + (i + x)] = 
        } 
    }

    return x;
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
