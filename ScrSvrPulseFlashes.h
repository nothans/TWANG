
#include "ScreenSaver.h"

class ScrSvrPulseFlashes: public ScreenSaver
{
    public:
        virtual void Tick(long millis);
};

void ScrSvrPulseFlashes::Tick(long millis)
{
    int n, b, i;
    for(i = 0; i<MAX_LEDS; i++)
    {
        leds[i].fadeToBlackBy(128);
    }
    randomSeed(millis);
    b = millis%800;
    if (b < 240)
    {
        n = 121 - b/2;
    }
    else
    {
        n = 1;
    }
    for(i = 0; i<MAX_LEDS; i++)
    {
        if ((random8() <= n)) 
        {
            leds[i] = CRGB::White;
        }
    }
}
