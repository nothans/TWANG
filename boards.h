// This file contains the board specific definitions like max led count

#if defined(ARDUINO_AVR_MEGA2560)
// Arduino Mega 2560
#define DATA_PIN             3
#define CLOCK_PIN            4   // ignored for Neopixel

#define MAX_LEDS 1000
#elif defined(ARDUINO_AVR_NANO)
// Arduino Mega 328
#define DATA_PIN             2
#define CLOCK_PIN            3   // ignored for Neopixel

#define MAX_LEDS 240
#else
#error "Please define DATA_PIN and CLOCK_PIN for your board."
#endif
