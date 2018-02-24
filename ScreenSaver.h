
#ifndef _SCREENSAVER_H_
#define _SCREENSAVER_H_

class ScreenSaver
{
  public:
    virtual void Tick(long millis) = 0;
};

#endif
