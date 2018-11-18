#include "ScreenSaver.h"

class ScrSvrWhiteFlow: public ScreenSaver
{
    public:
        virtual void Tick(long millis);
};
void ScrSvrWhiteFlow::Tick(long millis)
{
    int i;

    for (i = 0; i < NUM_LEDS; i ++)
    {
        if (((i+(int)(millis/100))%5)==0)
        {
            leds[i] = CRGB::White;
        }
        else
        {
            leds[i] = CRGB::Black;
        }
    }
}
