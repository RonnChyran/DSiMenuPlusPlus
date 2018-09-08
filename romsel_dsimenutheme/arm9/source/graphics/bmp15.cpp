/*
    bmp15.cpp
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

//�

#include <list>
#include <string>
#include "bmp15.h"

BMP15::BMP15() : _width(0), _height(0), _pitch(0), _buffer(NULL)
{
}

BMP15::BMP15(u32 width, u32 height) : _width(0), _height(0), _pitch(0), _buffer(NULL)
{
    _width = width;
    _height = height;
    _pitch = (width + (width & 1)) << 1;
}

BMP15::~BMP15()
{
}

BMP15 createBMP15(u32 width, u32 height)
{
    BMP15 bmp(width, height);

    u32 pitch = bmp.pitch();
   

    u32 bufferSize = height * pitch;
    if (bufferSize & 3)
        bufferSize += 4 - (bufferSize & 3);
    bmp._buffer = new u32[bufferSize >> 2];
    return bmp;
}

typedef std::pair<std::string, BMP15> str_bmp_pair;
typedef std::list<str_bmp_pair> str_bmp_list;
static str_bmp_list _bmpPool;

BMP15 createBMP15FromMem(void *mem)
{
    return BMP15();
}

BMP15 createBMP15FromFile(const std::string &filename)
{
    str_bmp_list::iterator it;
    for (it = _bmpPool.begin(); it != _bmpPool.end(); ++it)
    {
        if (filename == it->first)
        {
            return it->second;
        }
    }

    FILE *f = fopen(filename.c_str(), "rb");
    if (NULL == f)
    {
        return BMP15();
    }

    fseek(f, 0, SEEK_END);

    u16 bmMark = 0;
    fseek(f, 0, SEEK_SET);
    fread(&bmMark, 1, 2, f);
    if (bmMark != 0x4d42)
    { // 'B' 'M' header
        fclose(f);
        return BMP15();
    }

    u32 width = 0;
    u32 height = 0;
    fseek(f, 0x12, SEEK_SET);
    fread(&width, 1, 4, f);
    fseek(f, 0x16, SEEK_SET);
    fread(&height, 1, 4, f);

    BMP15 bmp = createBMP15(width, height);

    u32 bmpDataOffset = 0;
    fseek(f, 0x0a, SEEK_SET);
    fread(&bmpDataOffset, 1, 4, f);

    long position = bmpDataOffset;
    fseek(f, position, SEEK_SET);
    u16 *pbuffer = ((u16 *)bmp.buffer()) + (bmp.pitch() >> 1) * height - (bmp.pitch() >> 1);

    for (u32 i = 0; i < height; ++i)
    {
        fread(pbuffer, 1, bmp.pitch(), f);
        position += bmp.pitch();
        pbuffer -= bmp.pitch() >> 1;
        fseek(f, position, SEEK_SET);
    }
    fclose(f);

    pbuffer = (u16 *)bmp.buffer();
    for (u32 i = 0; i < height; ++i)
    {
        for (u32 j = 0; j < (bmp.pitch() >> 1); ++j)
        {
            u16 pixelColor = pbuffer[i * (bmp.pitch() >> 1) + j];
            pixelColor = ((pixelColor & 0x7C00) >> 10) | ((pixelColor & 0x03E0)) | ((pixelColor & 0x1F) << 10);
            pbuffer[i * (bmp.pitch() >> 1) + j] = pixelColor | (pixelColor ? BIT(15) : 0);
        }
    }

    str_bmp_pair bmpPoolItem(std::string(filename), bmp);
    _bmpPool.push_back(bmpPoolItem);

    return bmp;
}
