/*
    sprite.cpp
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

#include "sprite.h"
#include <nds.h>

Sprite::Sprite()
{
}

Sprite::~Sprite()
{
}

SpriteEntry& Sprite::entry()
{
    return *_entry;
}
void Sprite::init(OamState *oamPtr, u16 id)
{
    _oamPtr = oamPtr;

    _id = id;

    _size = SS_SIZE_32;

    _shape = SS_SHAPE_SQUARE;

    _bufferOffset = 0;

    _priority = 2;
    
    _entry = &_oamPtr->oamMemory[_id];
    _affine = &_oamPtr->oamRotationMemory[_id];

    // initial x = 0, hidden, bitmap obj mode, square shape
    _entry->attribute[0] = ATTR0_DISABLED | ATTR0_BMP | ATTR0_SQUARE | 0;

    // initial y = 0, size = 32x32, Affine Transformation Parameter group 0
    _entry->attribute[1] = ATTR1_SIZE_32 | ATTR1_ROTDATA(_id) | 0;

    // initial alpha = 15, buffer offset = 0, display priority 0
    _entry->attribute[2] = ATTR2_ALPHA(15) | ATTR2_PRIORITY(0) | 0;

    setScale(1, 1);

    //for(int i=0;i<32*32;i++)
    //    SPRITE_GFX[i]=RGB15(0,0,27)|(1<<15); //dont forget alpha bit

    //update();
}

void Sprite::show()
{
    //oamSetHidden(&oamMain, _id, false);

    _entry->attribute[0] = (_entry->attribute[0] & (~0x0300)) | ATTR0_ROTSCALE | ATTR0_BMP;
}

void Sprite::hide()
{
    // oamSetHidden(&oamMain, _id, true);
    _entry->attribute[0] = (_entry->attribute[0] & (~0x0300)) | ATTR0_DISABLED;
}

void Sprite::setAlpha(u8 alpha)
{
    oamSetAlpha(_oamPtr, _id, alpha);
}

void Sprite::setPosition(u16 x, u8 y)
{
    oamSetXY(_oamPtr, _id, x, y);
}

void Sprite::setSize(SPRITE_SIZE size)
{
    _size = size;
    _entry->attribute[1] = (_entry->attribute[1] & (~0xC000)) | _size;
}

void Sprite::setShape(SPRITE_SHAPE shape)
{
    _shape = shape;
    _entry->attribute[1] = (_entry->attribute[0] & (~0xC000)) | _shape;
}

u16 *Sprite::buffer()
{
    return SPRITE_GFX_SUB + (_bufferOffset * 64);
}

void Sprite::setBufferOffset(u32 offset)
{
    _bufferOffset = offset;
    _entry->attribute[2] = (_entry->attribute[2] & (~0x3FF)) | offset;
}

void Sprite::setScale(float scaleX, float scaleY)
{
    _scaleX = scaleX;
    _scaleY = scaleY;

    scaleX = 1 / scaleX;
    scaleY = 1 / scaleY;

    u8 decimalX = (u8)((scaleX - (int)scaleX) * 256);
    u8 integerX = (u8)((int)scaleX) & 0x7f;
    u8 decimalY = (u8)((scaleY - (int)scaleY) * 256);
    u8 integerY = (u8)((int)scaleY) & 0x7f;

    _affine->hdx = (integerX << 8) | decimalX;
    _affine->hdy = 0;
    _affine->vdx = 0;
    _affine->vdy = (integerY << 8) | decimalY;
}

void Sprite::setPriority(u8 priority)
{
    oamSetPriority(_oamPtr, _id, priority);
    _priority = priority;
}

bool Sprite::visible()
{
    return (_entry->attribute[0] & 0x0300) != 0x0200;
}
