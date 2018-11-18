

#include "ScreenSaver.h"

class ScrSvrMarchingGreenOrange: public ScreenSaver
{
  public:
    virtual void Tick(long millis);
};

void ScrSvrMarchingGreenOrange::Tick(long mm)
{
    // Marching green <> orange
    int n, b, c, i;
    for(i = 0; i<MAX_LEDS; i++){
        leds[i].nscale8(250);
    }
    n = (mm/250)%10;
    b = 10+((sin(mm/500.00)+1)*20.00);
    c = 20+((sin(mm/5000.00)+1)*33);
    for(i = 0; i<MAX_LEDS; i++){
        if(i%10 == n){
            leds[i] = CHSV( c, 255, 150);
        }
    }
}
