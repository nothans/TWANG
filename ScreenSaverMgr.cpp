
#include "ScreenSaverMgr.h"
#include "Arduino.h"

#include "FastLED.h"
#include "defines.h"

extern CRGB leds[NUM_LEDS];

// Screensavers
#include "ScrSvrPulseFlashes.h"
#include "ScrSvrMarchingGreenOrange.h"
#include "ScrSvrRandomFlashes.h"
#include "ScrSvrWhiteFlow.h"
#include "ScrSvrDotsInBowls.h"
//Include your new Screensaver here

ScreenSaverMgr::ScreenSaverMgr()
{
    _screenSaverCount = 5; //Number of Screensavers
    _screenSavers = new ScreenSaver*[_screenSaverCount] 
    {
        new ScrSvrDotsInBowls(),
        new ScrSvrWhiteFlow(),
        new ScrSvrPulseFlashes(),
        new ScrSvrMarchingGreenOrange(),
        new ScrSvrRandomFlashes(),
        //Add your new Screensaver here
    };
}

void ScreenSaverMgr::Tick()
{
    long mm = millis();
    int mode = (mm/SCREEN_SAVER_DURATION)%_screenSaverCount;
    long modeTime = mm%SCREEN_SAVER_DURATION;

    _mode = mode;
    _screenSavers[_mode]->Tick(modeTime);
}

