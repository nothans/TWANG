
#include "ScreenSaverMgr.h"
#include "Arduino.h"

#include "FastLED.h"
#include "defines.h"

extern CRGB leds[NUM_LEDS];

// Screensavers
#include "ScrSvrPulseFlashes.h"
#include "ScrSvrMarchingGreenOrange.h"
#include "ScrSvrRandomFlashes.h"
#include "ScrSvrDef.h" //AddScreenSaver

ScreenSaverMgr::ScreenSaverMgr()
{
    _screenSaverCount = 4; //AddScreenSaver
    _screenSavers = new ScreenSaver*[_screenSaverCount] 
    {
        new ScrSvrPulseFlashes(),
        new ScrSvrMarchingGreenOrange(),
        new ScrSvrRandomFlashes(),
        new ScrSvrDef(), //AddScreenSaver
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

