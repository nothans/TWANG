
#include "ScreenSaver.h"

class ScrSvrRandomFlashes: public ScreenSaver
{
  public:
    virtual void Tick(long millis);
};

void ScrSvrRandomFlashes::Tick(long mm)
{
    // Random flashes
    int n, b, c, i;
    for(i = 0; i<NUM_LEDS; i++){
        leds[i].nscale8(250);
    }
    randomSeed(mm);
    for(i = 0; i<NUM_LEDS; i++){
        if(random8(200) == 0){
            leds[i] = CHSV( 25, 255, 100);
        }
    }
}
