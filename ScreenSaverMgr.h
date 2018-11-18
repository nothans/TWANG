

#include "ScreenSaver.h"
#include <stdint.h>

#define SCREEN_SAVER_DURATION 10000 // Duration every screen saver will run (ms)

class ScreenSaverMgr
{
  public:
    ScreenSaverMgr();
    void Tick();

  private:
    uint8_t _mode;    
    ScreenSaver** _screenSavers;
    uint8_t _screenSaverCount;
};



