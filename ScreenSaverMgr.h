

#include "ScreenSaver.h"

class ScreenSaverMgr
{
  public:
    ScreenSaverMgr();
    void Tick();

  private:
    int _mode;    
    ScreenSaver** _screenSavers;
    int _screenSaverCount;
};



