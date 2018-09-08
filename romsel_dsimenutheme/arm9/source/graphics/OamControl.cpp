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
    _topBackground = createBMP15FromFile(path);
    nocashMessage("ARM9 GDI BMP15 Created");
    if (_topBackground.width() < SCREEN_WIDTH && _topBackground.height() < SCREEN_WIDTH)
    {
        nocashMessage("BG Width too small");
        _topBackground = createBMP15(SCREEN_WIDTH, SCREEN_HEIGHT);
        zeroMemory(_topBackground.buffer(), _topBackground.height() * _topBackground.pitch());
    }

    u32 pitch = _topBackground.pitch() >> 1;

    for (size_t y = 0; y < 3; ++y)
    {
        for (size_t x = 0; x < 4; ++x)
        {
            size_t index = y * 4 + x;

            _topBackgroundSprites[index].init(&oamSub, 2 + index);
            _topBackgroundSprites[index].setSize(SS_SIZE_64);
            _topBackgroundSprites[index].setPriority(3);
            _topBackgroundSprites[index].setBufferOffset(32 + index * 64);
            _topBackgroundSprites[index].setPosition(x * 64, y * 64);

            for (size_t k = 0; k < 64; ++k)
            {
                for (size_t l = 0; l < 64; ++l)
                {

                    ((u16 *)_topBackgroundSprites[index].buffer())[k * 64 + l] =
                       ((u16 *)_topBackground.buffer())[(k + y * 64) * pitch + (l + x * 64)];
                }
            }
            _topBackgroundSprites[index].show();
        }
    }
    oamUpdate(&oamSub);
}

void OamControl::fillSprites(vector<Sprite> &sprites, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        sprites.emplace_back(Sprite());
    }
}