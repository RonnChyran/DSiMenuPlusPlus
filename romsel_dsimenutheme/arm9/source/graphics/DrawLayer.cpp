#include "DrawLayer.h"
u32 _backBuffer[256 * 192];
u16 *backBuffer = (u16 *)_backBuffer;

DrawLayer::DrawLayer()
{
    invalidate();
    //backBuffer = std::move(std::make_unique<u16[]>(192 * 256));
}

void DrawLayer::invalidate()
{
    drawNext = true;
}

void DrawLayer::drawPixel(u8 x, u8 y, u16 color)
{
    //backBuffer[0] = color | BIT(15);
    //backBuffer[(y + 1) * 256 + (x)] = ((color >> 10) & 0x1f) | ((color) & (0x1f << 5)) | (color & 0x1f) << 10 | BIT(15);
     backBuffer[((u32)y << 8) + x] = color | BIT(15);
}

void DrawLayer::presentBuffer(int id)
{
    if (!drawNext) return;
    DC_FlushRange((void *)backBuffer, 256 * 192 * 2);
    dmaCopyWords(3, (void *)backBuffer, bgGetGfxPtr(id), 256 * 192 * 2);
    DC_InvalidateRange(bgGetGfxPtr(id), 256 * 192 * 2);
    swiFastCopy((void*)(0xffffffff), backBuffer, (0x18000>>2) | COPY_MODE_WORD | COPY_MODE_FILL);
    drawNext = false;
   // swiFastCopy((void*)(0xffffffff), backBuffer, (0x18000>>2) | COPY_MODE_WORD | COPY_MODE_FILL);
}

void DrawLayer::drawIcon(u8 x, u8 y, int num)
{
    //if (_headerArray.empty()) return;
    // if (_headerArray[num] == nullptr) return;
    // sNDSBannerExt _banner = *_headerArray[num];

    // // if (!_banner.icon) return;
    invalidate();
    for (int tile = 0; tile < 16; tile++)
    {
        for (int pixel = 0; pixel < 32; pixel++)
        {
            u8 a_byte = _bitmapArray[num].get()[(tile << 5) + pixel];
            // sprintf(draw_buf, "Read %x", a_byte);
            int px = ((tile & 3) << 3) + ((pixel << 1) & 7);
            int py = ((tile >> 2) << 3) + (pixel >> 2);

            u8 idx1 = (a_byte & 0xf0) >> 4;
            if (0 != idx1)
            {
                // setPenColor(_banner.palette[idx1]);
                // setPenColor(_banner.palette[idx1]);
                drawPixel(px + 1 + x, py + y, _paletteArray[num].get()[idx1]);
            }

            u8 idx2 = (a_byte & 0x0f);
            if (0 != idx2)
            {
                // setPenColor(_banner.palette[idx2]);
               drawPixel(px + x, py + y, _paletteArray[num].get()[idx2]);
            }
        }
    }
}