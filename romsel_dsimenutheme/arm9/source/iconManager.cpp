#include "iconManager.h"
#include <nds.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/stat.h>
#include <gl2d.h>
#include "icon_unk.h"

// [num] [frame]
//static int _iconTexID[6];
//static glImage _ndsIcon[6][8];

bool initialized;

int _iconTexID[6];
glImage _ndsIcon[6][8];

/**
 * Gets the current icon stored at the specified index.
 * If the index is out of bounds or the icon manager is not
 * initialized, returns null.
 */
const glImage *getIcon(int num)
{
    //     if (num < 0 || num > 5 || !initialized) return NULL;
    return _ndsIcon[num];
}

/**
 * Copied glLoadTileSet directly from gl2d.c
 * 
 * With the added feature of specifitying the preallocated textureID.
 * The texture ID is intended to have already been initialized, as well
 * as palette already set. 
 * 
 * Do not call this directly, instead, use only loadIcon.
 */
void glLoadTileSetIntoSlot(
    int num,
    int tile_wid,
    int tile_hei,
    int bmp_wid,
    int bmp_hei,
    GL_TEXTURE_TYPE_ENUM type,
    int sizeX,
    int sizeY,
    int param,
    int pallette_width,
    const u16 *_palette,
    const uint8 *_texture,
    bool init)
{
    int textureID = _iconTexID[num];
    glImage *sprite = _ndsIcon[num];

    glBindTexture(0, textureID);
    glTexImage2D(0, 0, type, sizeX, sizeY, 0, param, _texture);

    if (!init)
    {
        glColorSubTableEXT(0, 0, pallette_width, 0, 0, _palette);
    }
    else
    {
        glColorTableEXT(0, 0, pallette_width, 0, 0, _palette);
    }

    int i = 0;
    int x, y;

    // init sprites texture coords and texture ID
    for (y = 0; y < (bmp_hei / tile_hei); y++)
    {
        for (x = 0; x < (bmp_wid / tile_wid); x++)
        {
            sprite[i].width = tile_wid;
            sprite[i].height = tile_hei;
            sprite[i].u_off = x * tile_wid;
            sprite[i].v_off = y * tile_hei;
            sprite[i].textureID = textureID;
            i++;
        }
    }
}

/**
 * Loads an icon into one of 6 existing banks, overwritting 
 * the previous data.
 * num must be in the range [0, 5] else this function
 * does nothing.
 * 
 * If init is true, then the palettes will be copied into
 * texture memory before being bound with
 * glColorTableEXT. 
 * 
 * Otherwise, they will be replacing the existing palette
 * using glColorTableSubEXT at the same memory location.
 */
void glLoadIcon(int num, const u16 *_palette, const u8 *_tiles, bool init)
{
    glLoadTileSetIntoSlot(
        num,
        32,               // sprite width
        32,               // sprite height
        32,               // bitmap image width
        256,              // bitmap image height
        GL_RGB16,         // texture type for glTexImage2D() in videoGL.h
        TEXTURE_SIZE_32,  // sizeX for glTexImage2D() in videoGL.h
        TEXTURE_SIZE_256, // sizeY for glTexImage2D() in videoGL.h
        GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
        16,              // Length of the palette to use (16 colors)
        (u16 *)_palette, // Image palette
        (u8 *)_tiles,    // Raw image data
        init     
    );
}

void glLoadIcon(int num, const u16 *palette, const u8 *tiles)
{
    glLoadIcon(num, palette, tiles, false);
}

/**
 * Allocates and initializes the VRAM locations for
 * icons. Must be called before the icon manager is used.
 */
void iconManagerInit()
{
    // Allocate texture memory for 6 textures.
    glGenTextures(6, _iconTexID);

    // Initialize empty data for the 6 textures.
    for (int i = 0; i < 6; i++)
    {
        printf("%i\n", _iconTexID[i]);
        glLoadIcon(i, (u16 *)icon_unkPal, (u8 *)icon_unkBitmap, true);
    }

    // set initialized.
    initialized = true;
}

