#include "ScreenSaver.h"
#include "iSin.h"

class ScrSvrDotInBowl: public ScreenSaver
{
    public:
        virtual void Tick(long millis);
};
void ScrSvrDotInBowl::Tick(long millis)
{
    int pos = map(isin.convert(millis/10),-128,128,1,NUM_LEDS-1);


    for (int i = 0; i < NUM_LEDS; i ++)
    {
        if (i == pos || i + 1 == pos || i - 1 == pos)
        {
            leds[i] = CRGB::White;
        }
        else
        {
            leds[i] = CRGB::Black;
        }
    }
}
