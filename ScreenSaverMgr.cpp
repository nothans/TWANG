
#include "ScreenSaverMgr.h"
#include "Arduino.h"

#include "FastLED.h"
#include "defines.h"

extern CRGB leds[NUM_LEDS];

// Screensavers
#include "ScrSvrPulseFlashes.h"
#include "ScrSvrMarchingGreenOrange.h"
#include "ScrSvrRandomFlashes.h"

ScreenSaverMgr::ScreenSaverMgr()
{
    _screenSaverCount = 3;
    _screenSavers = new ScreenSaver*[_screenSaverCount] 
    {
        new ScrSvrPulseFlashes(),
        new ScrSvrMarchingGreenOrange(),
        new ScrSvrRandomFlashes(),
    };
}

void ScreenSaverMgr::Tick()
{
    long mm = millis();
    long modeDuration = 10000;
    int mode = (mm/modeDuration)%_screenSaverCount;
    long modeTime = mm%modeDuration;

    _mode = mode;
    _screenSavers[_mode]->Tick(modeTime);
}

