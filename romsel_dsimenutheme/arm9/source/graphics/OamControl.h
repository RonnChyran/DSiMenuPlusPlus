#include <nds.h>
#include <string>
#include "common/singleton.h"
#include "sprite.h"
#include "bmp15.h"
#include <vector>

#ifndef __DSIMENUPP_OAMCTRL_H_
#define __DSIMENUPP_OAMCTRL_H_

using std::vector;

class OamControl
{
  public:
    void initTopBg(const std::string path);
    void sysinitMain();
    void sysinitSub();

  private:
    void fillSprites(vector<Sprite> &sprites, int amount);

  private:
    vector<Sprite> _topBackgroundSprites;
    BMP15 _topBackground;
};

typedef singleton<OamControl> oamControl_s;
inline OamControl &oam() { return oamControl_s::instance(); }
#endif