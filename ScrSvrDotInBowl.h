#include "ScreenSaver.h"
#include "iSin.h"

class ScrSvrDotInBowl: public ScreenSaver
{
    public:
        virtual void Tick(long millis);

    private:
        iSin isin = iSin();
};
void ScrSvrDotInBowl::Tick(long millis)
{
    int pos = map(isin.convert(millis),-255,255,0,NUM_LEDS);


    for (int i = 0; i < NUM_LEDS; i ++)
    {
        if (i == pos)
        {
            leds[i] = CRGB::White;
        }
        else
        {
            leds[i] = CRGB::Black;
        }
    }
}
