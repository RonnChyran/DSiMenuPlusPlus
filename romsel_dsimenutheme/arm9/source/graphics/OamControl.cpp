#include "OamControl.h"
#include "memtool.h"
#include "dark_top.h"

#define COLORS_PER_PALETTE 16
#define SPRITE_DMA_CHANNEL 0

void OamControl::sysinitMain()
{
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, true);

    for (int i = 0; i < 128; i++)
    {
        oamMain.oamMemory[i].attribute[0] = ATTR0_DISABLED;
        oamMain.oamMemory[i].attribute[1] = 0;
        oamMain.oamMemory[i].attribute[2] = 0;
        oamMain.oamMemory[i].filler = 0;
    }

    oamUpdate(&oamMain);
    oamEnable(&oamMain);
}

void OamControl::sysinitSub()
{
    oamInit(&oamSub, SpriteMapping_Bmp_1D_128, true);

    for (int i = 0; i < 128; i++)
    {
        oamSub.oamMemory[i].attribute[0] = ATTR0_DISABLED;
        oamSub.oamMemory[i].attribute[1] = 0;
        oamSub.oamMemory[i].attribute[2] = 0;
        oamSub.oamMemory[i].filler = 0;
    }

    oamUpdate(&oamSub);
    oamEnable(&oamSub);
}

void OamControl::initTopBg(const std::string path)
{
    fillSprites(_topBackgroundSprites, 24);
    _topBackground = createBMP15FromFile(path);

    
    nocashMessage("ARM9 GDI BMP15 Created");
    if (_topBackground.width() < SCREEN_WIDTH && _topBackground.height() < SCREEN_WIDTH)
    {
        nocashMessage("BG Width too small");
        _topBackground = createBMP15(SCREEN_WIDTH, SCREEN_HEIGHT);
        zeroMemory(_topBackground.buffer(), _topBackground.height() * _topBackground.pitch());
    }
    int index = 0;
    u32 pitch = _topBackground.pitch() >> 1;
    for (size_t y = 0; y < 6; ++y)
    {
        for (size_t x = 0; x < 8; ++x)
        {
           // size_t index = y * 8 + x;
            if (x != 0 && y != 0 && x != 7 && y != 5) continue;
            _topBackgroundSprites[index].init(&oamSub, index);
            _topBackgroundSprites[index].setSize(SS_SIZE_32);
            _topBackgroundSprites[index].setPriority(3);
            _topBackgroundSprites[index].setBufferOffset(16 + index * 16);
            _topBackgroundSprites[index].setPosition(x * 32, y * 32);

            for (size_t k = 0; k < 64; ++k)
            {
                for (size_t l = 0; l < 64; ++l)
                {
                    ((u16 *)_topBackgroundSprites[index].buffer())[k * 32 + l] =
                       ((u16 *)_topBackground.buffer())[(k + y * 32) * pitch + (l + x * 32)];
                }
            }
            _topBackgroundSprites[index].show();
            index++;
        }
    }
    oamUpdate(&oamSub);
}

void OamControl::initTopPicture()
{
    _topPhoto = createBMP15FromFile("nitro:/graphics/photo_default.bmp");

}
void OamControl::fillSprites(vector<Sprite> &sprites, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        sprites.emplace_back(Sprite());
    }
}