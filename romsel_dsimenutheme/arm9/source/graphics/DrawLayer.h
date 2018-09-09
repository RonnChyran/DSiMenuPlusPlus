#include <nds.h>
#include <string>
#include "common/singleton.h"
#include "sprite.h"
#include "bmp15.h"
#include <memory>
#include <nds/arm9/background.h>
#include <nds/arm9/cache.h>
#include "ndsheaderbanner.h"
#include <vector>
#include <memory>
#ifndef __DSIMENUPP_DRAWLAYER_H_
#define __DSIMENUPP_DRAWLAYER_H_

class DrawLayer
{
  public:
    DrawLayer();
    ~DrawLayer() {}
    
    void  drawPixel(u8 x, u8 y, u16 color); 
    void presentBuffer(int id);
    void invalidate();
    void setIconBanner(int num, const sNDSBannerExt* _banner)
    {
         auto bmpPtr = std::make_unique<u8[]>(512);
        memcpy(bmpPtr.get(), _banner->icon, 512);
        _bitmapArray[num] = std::move(bmpPtr);

        auto palPtr = std::make_unique<u16[]>(16);
        memcpy(palPtr.get(), _banner->palette, 32);
        _paletteArray[num] = std::move(palPtr);
    //    
       /// bannerPtr.get()
  //      _headerArray[num] = (std::move(bannerPtr));
        //_headerArray[num] = _banner;
      
    }

    void drawIcon(u8 x, u8 y, int num);


  private:
    //std::unique_ptr<u16[]> _backBuffer;
    std::array<std::unique_ptr<u8[]>, 6> _bitmapArray;
    std::array<std::unique_ptr<u16[]>, 6> _paletteArray;
    bool drawNext;

};

typedef singleton<DrawLayer> drawLayer_s;
inline DrawLayer &drw() { return drawLayer_s::instance(); }
#endif