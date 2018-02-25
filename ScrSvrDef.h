#include "ScreenSaver.h"

class ScrSvrDef: public ScreenSaver
{
    public:
        virtual void Tick(long millis);
};
void ScrSvrDef::Tick(long millis)
{
    int i;
    //NUM_LEDS; LED count
    //leds; FastLED object
    //millis; how long this will run in (1/1000)seconds

    for (i = 0; i < NUM_LEDS; i ++)
    {
        if (((i+(int)(millis/10)%5)==0)
        {
            leds[i] = CRGB::White;
        }
        else
        {
            leds[i] = CRGB::Black;
        }
    }
}
