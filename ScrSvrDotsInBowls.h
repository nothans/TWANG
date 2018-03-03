#include "ScreenSaver.h"

class ScrSvrDotsInBowls: public ScreenSaver
{
    public:
        virtual void Tick(long millis);
        virtual void Draw(int pos, uint8_t hue);
};
void ScrSvrDotsInBowls::Tick(long millis)
{
    const uint8_t dotspeed = 22;
    const uint8_t dotsInBowlsCount = 3;
    const uint16_t dotDistance = 65535/dotsInBowlsCount;

    for (int i = 0; i < NUM_LEDS; i ++)
    {
        leds[i] = CRGB::Black;
    }

    for (int i = 0; i < dotsInBowlsCount; i ++)
    {
        long k = ((i*dotDistance) + millis*dotspeed);
        int dotPosition = map(sin16_avr(k),-32767,32767,2,NUM_LEDS-3);
        Draw(
            dotPosition,
            (uint8_t) (k % 255)
        );
    }
}
void ScrSvrDotsInBowls::Draw(int pos, uint8_t hue)
{
    leds[pos-2] += CHSV(hue, 255, BRIGHTNESS/4);
    leds[pos-1] += CHSV(hue, 255, BRIGHTNESS/2);
    leds[pos]   += CHSV(hue, 255, BRIGHTNESS);
    leds[pos+1] += CHSV(hue, 255, BRIGHTNESS/2);
    leds[pos+2] += CHSV(hue, 255, BRIGHTNESS/4);
} 
