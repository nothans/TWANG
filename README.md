# TWANG (FORK)
An Arduino-based, 1D, LED loving, dungeon crawler. inspired by Line Wobbler by Robin B

This fork was done to incorporate feedback after several field tests. Details on the fork can be found at the [Buildlog.net Blog](http://www.buildlog.net/blog?s=twang)



## Video playlist
A playlist that shows the development of TWANG and the game in both a desktop and house-sized form can be [found here](https://www.youtube.com/watch?v=9yf_VINmbTE&list=PL1_Z89_x_Dff-XhOxlx6sQ38wJqe1X2M0)


## Required libraries:
* [FastLED](http://fastled.io/)
* I2Cdev
* [MPU6050](https://github.com/jrowberg/i2cdevlib/tree/master/Arduino/MPU6050)
* [ToneAC](http://forum.arduino.cc/index.php?topic=142097.0)
* [RunningMedian](http://playground.arduino.cc/Main/RunningMedian)

## Hardware used:
* Arduino MEGA
* 3 LEDs for life indicator (optional) This fork displays the remaining lives on the LED strip.
* LED light strip. The more the better, maximum of 1000. Tested with 1x & 2x 144/meter, 12x 60/meter and 5m x 114/meter strips. This has been tested with APA102C and Neopixel type strips. Anything compatible wiht the FastLED library should work.
* MPU6050 accelerometer
* Spring doorstop, I used [these](http://smile.amazon.com/gp/product/B00J4Y5BU2)

See [Buildlog.net Blog](http://www.buildlog.net/blog?s=twang) for more details.

Super easy to use kits and ready to play units are available on [Tindie](https://www.tindie.com/products/33366583/twang-shield-kit/)

## Enclosure
Files to print the original enclosure can be found [here](http://www.thingiverse.com/thing:1116899)
The Tindie kit version version of the enclosure can be found [here](https://www.thingiverse.com/thing:2770292)


## Overview
The following is a quick overview of the code to help you understand and tweak the game to your needs.

The game is played on a 1000 unit line, the position of enemies, the player, lava etc range from 0 to 1000 and the LEDs that represent them are derived using the getLED() function. You don't need to worry about this but it's good to know for things like the width of the attack and player max move speed. Regardles of the number of LEDs, everything takes place in this 1000 unit wide line.

**//LED SETUP** Defines the quantity of LEDs as well as the data and clock pins used. I've tested several APA102-C strips and the color order sometimes changes from BGR to GBR, if the player is not blue, the exit green and the enemies red, this is the bit you want to change. Brightness should range from 50 to 255, use a lower number if playing at night or wanting to use a smaller power supply. "DIRECTION" can be set to 0 or 1 to flip the game orientation. In setup() there is a "FastLED.addLeds()" line, in there you could change it to another brand of LED strip like the cheaper WS2812.

The game also has 3 regular LEDs for life indicators (the player gets 3 lives which reset each time they level up). The pins for these LEDs are stored in lifeLEDs[] and are updated in the updateLives() function

**//JOYSTICK SETUP** All parameters are commented in the code, you can set it to work in both forward/backward as well as side-to-side mode by changing JOYSTICK_ORIENTATION. Adjust the ATTACK_THRESHOLD if the "Twaning" is overly sensitive and the JOYSTICK_DEADZONE  if the player slowly drifts when there is no input (because it's hard to get the MPU6050 dead level)

**//WOBBLE ATTACK** Sets the width, duration (ms) of the attack

**//POOLS** These are the object pools for enemies, particles, lava, conveyors etc. You can modify the quanity of any of them if your levels use more or if you want to save some memory, just remember to update the respective counts to avoid errors.

**//USE_GRAVITY** 0/1 to set if particles created by the player getting killed should fall towards the start point, the BEND_POINT variable can be set to mark the point at which the strip of LEDs goes from been horizontal to vertical. The game is 1000 units wide (regardless of number of LED's) so 500 would be the mid point. If this is confusing just set USE_GRAVITY to 0

##  Serial Port Options Menu (New!) ##
Some very basic options can be used to adjust some values via the serial port over USB. An 
easy to use serial terminal is the Serial Console of the Arduino IDE. Open the Com Port that appears when you plug in the TWANG USB. 
It typically will identify itself as an "Arduino Mega 2560". Set it up for 115200 baud and have it add a carriage return when sending. You
should see this menu.
```
====== TWANG Settings Menu ========
=    Current values are shown     =
=   Send new values like 13=66    =
=     with a carriage return      =
===================================

10=200 (LED Brightness 5-255)
20=7 (Joystick Deadzone 3-12)
21=30000 (Attack Sensitivity 20000-35000)
30=10 (Audio Volume 0-10)
40=3 (Lives per Level (3-9))
```
You can change any of these settings. Send 10=225 to change the LED brightness to 225 (on a 5-255 scale). These settings will be saved in
the EEPROM memory of the TWANG and not be lost when you power down the TWANG.

## Modifying / Creating levels
Find the loadLevel() function, in there you can see a switch statment with the 10 levels I created. 
They all call different functions and variables to setup the level. Each one is described below:

**playerPosition;** Where the player starts on the 0 to 1000 line. If not set it defaults to 0. I set it to 200 in the first level so the player can see movement even if the first action they take is to push the joystick left

**spawnEnemy(position, direction, speed, wobble);** (10 enemies max)
* position: 0 to 1000
* direction: 0/1, initial direction of travel
* speed: >=0, speed of the enemy, remember the game is 1000 wide and runs at 60fps. I recommend between 1 and 4
* wobble: 0=regular moving enemy, 1=sine wave enemy, in this case speed sets the width of the wave

**spawnPool[poolNumber].Spawn(position, rate, speed, direction);** (2 spawners max)
* A spawn pool is a point which spawns enemies forever
* position: 0 to 1000
* rate: milliseconds between spawns, 1000 = 1 second
* speed: speed of the enemis it spawns
* direction: 0=towards start, 1=away from start

**spawnLava(startPoint, endPoint, ontime, offtime, offset);** (4 lava pools max)
* startPoint: 0 to 1000
* endPoint: 0 to 1000, combined with startPoint this sets the location and size of the lava
* ontime: How long (ms) the lava is ON for
* offtime: How long the lava is ON for
* offset: How long (ms) after the level starts before the lava turns on, use this to create patterns with multiple lavas

**spawnConveyor(startPoint, endPoint, direction);** (2 conveyors max)
* startPoint, endPoint: Same as lava
* direction: the direction of travel 0/1

**spawnBoss(); ** (only one, don't edit boss level)
* There are no parameters for a boss, they always spawn in the same place and have 3 lives. Tweak the values of Boss.h to modify

Feel free to edit, comment on the YouTube video (link at top) if you have any questions.