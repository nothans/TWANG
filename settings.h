#include "HardwareSerial.h"
#include <avr/wdt.h>
#include <EEPROM.h>

#include "boards.h"

/// Defaults

// change this whenever the saved settings are not compatible with a change
// it force a load from defaults.
#define SETTINGS_VERSION 5

// LED Strip Setup
#define NUM_LEDS        120
#define MIN_LEDS				60

#define BRIGHTNESS          150
#define MIN_BRIGHTNESS			10
#define MAX_BRIGHTNESS 			255

// the strips have different low level brightness.  WS2812 tends to fade out faster at the low end
#define WS2812_CONVEYOR_BRIGHTNES  40
#define WS2812_LAVA_OFF_BRIGHTNESS 15

#define USE_LIFELEDS  // uncomment this to make Life LEDs available (not used in the B. Dring enclosure)

// JOYSTICK
#define JOYSTICK_ORIENTATION 1     // 0, 1 or 2 to set the angle of the joystick
#define JOYSTICK_DIRECTION   1     // 0/1 to flip joystick direction

#define ATTACK_THRESHOLD     30000 // The threshold that triggers an attack
#define MIN_ATTACK_THRESHOLD     20000
#define MAX_ATTACK_THRESHOLD     30000

#define JOYSTICK_DEADZONE    7     // Angle to ignore
#define MIN_JOYSTICK_DEADZONE 3
#define MAX_JOYSTICK_DEADZONE 12


// AUDIO
#define MAX_VOLUME           10
#define MIN_VOLUME              0
#define MAX_VOLUME              10


// LEVELS
#define VIRTUAL_WORLD_COUNT 1000

#define MAX_PLAYER_SPEED    10     // Max move speed of the player

#define LIVES_PER_LEVEL    3
#define MIN_LIVES_PER_LEVEL 3
#define MAX_LIVES_PER_LEVEL 9

#define DEFAULT_ATTACK_WIDTH 70  // Width of the wobble attack, world is 1000 wide
#define ATTACK_DURATION     500    // Duration of a wobble attack (ms)

#define BOSS_WIDTH          40

// Animation durations used in main loop and some sounds too.
#define MIN_REDRAW_INTERVAL  16    // Min redraw interval (ms) 33 = 30fps / 16 = 63fps
#define STARTUP_WIPEUP_DUR 200
#define STARTUP_SPARKLE_DUR 1300
#define STARTUP_FADE_DUR 1500

#define GAMEOVER_SPREAD_DURATION 1000
#define GAMEOVER_FADE_DURATION 1500

#define WIN_FILL_DURATION 500     // sound has a freq effect that might need to be adjusted
#define WIN_CLEAR_DURATION 1000
#define WIN_OFF_DURATION 1200

// SCREEN SAVER
#define TIMEOUT              30000  // time until screen saver

enum stripTypes{
	strip_APA102 = 0,
	strip_WS2812 = 1
};

enum ErrorNums{
	ERR_SETTING_NUM,
	ERR_SETTING_RANGE
};

typedef struct {
	uint8_t settings_version; // stores the settings format version 	
	
	uint16_t led_count;
	uint8_t led_brightness;
	
	uint8_t joystick_deadzone;
	uint16_t attack_threshold;
	
	uint8_t audio_volume;
	
	uint8_t lives_per_level;

  uint8_t player_direction;
  
  uint8_t gravity;
  uint16_t bend_point;
	
}settings_t;

settings_t user_settings;

#define READ_BUFFER_LEN 8
#define CARRIAGE_RETURN 13
char readBuffer[READ_BUFFER_LEN];
uint8_t readIndex = 0;

long lastInputTime = 0;

void reset_cpu();
void processSerial(char inChar);
void show_settings_menu();
void reset_settings();
void change_setting(char *line);
void settings_eeprom_write();
void settings_eeprom_write();
void printError(int reason);


void reset_cpu()
{
  wdt_enable(WDTO_15MS);
  while(1)
  {
    // wait for it...boom
  }
}

void processSerial(char inChar)
{
	readBuffer[readIndex] = inChar;
	
		switch(readBuffer[readIndex]) {
			case '?':// show settings
				readIndex = 0;
				Serial.print(F("TWANG VERSION: ")); Serial.println(F(VERSION));
				show_settings_menu();
				return;
			break;
			
			case 'R': // reset to defaults
				readIndex = 0;
				reset_settings();
				return;
			break;
			
			case '!': // reboot
				reset_cpu();
			break;
			
			default:
			break;
		}
	
		if (readBuffer[readIndex] == CARRIAGE_RETURN) {
			if (readIndex < 3) {
				// not enough characters
				readIndex = 0;
			}
			else {				
				readBuffer[readIndex] = 0; // mark it as the end of the string
				change_setting(readBuffer);	
				readIndex = 0;
			}
		}
		else if (readIndex >= READ_BUFFER_LEN) {
			readIndex = 0; // too many characters. Reset and try again
		}
		else
			readIndex++;
}

void show_settings_menu() {
	
	
	Serial.println(F("\r\n====== TWANG Settings Menu ========"));
	Serial.println(F("=    Current values are shown     ="));
	Serial.println(F("=   Send new values like B=150    ="));
	Serial.println(F("=     with a carriage return      ="));
	Serial.println(F("==================================="));
	
	Serial.print(F("\r\nC="));
	Serial.print(user_settings.led_count);
	Serial.println(F(" (LED Count 60-1000 forces restart if increased above initial val.)"));
	
	Serial.print(F("B="));	
	Serial.print(user_settings.led_brightness);
	Serial.println(F(" (LED Brightness 10-255)"));

  Serial.print(F("G="));
  Serial.print(user_settings.gravity);
  Serial.println(F(" (Use gravity (LED strip going up wall) 0 = off, 1 = on)"));

  Serial.print(F("P="));
  Serial.print(user_settings.bend_point);
  Serial.println(F(" (Point at which the LED strip goes up the wall 0-1000)"));

	Serial.print(F("S="));
	Serial.print(user_settings.audio_volume);
	Serial.println(F(" (Sound Volume 0-10)"));
	
	Serial.print(F("J="));
	Serial.print(user_settings.joystick_deadzone);
	Serial.println(F(" (Joystick Deadzone 3-12)"));

  Serial.print(F("D="));
  Serial.print(user_settings.player_direction);
  Serial.println(F(" (Player movment direction 0 = right to left, 1 = left to right)"));
	
	Serial.print(F("A="));
	Serial.print(user_settings.attack_threshold);
	Serial.println(F(" (Attack Sensitivity 20000-35000)"));	
	
	Serial.print(F("L="));
	Serial.print(user_settings.lives_per_level);
	Serial.println(F(" (Lives per Level (3-9))"));		
	
	Serial.println(F("\r\n(Send...)"));
	Serial.println(F("  ? to show current settings"));
	Serial.println(F("  R to reset everything to defaults"));
  Serial.println(F("  ! to reboot\r\n"));
	
}

void reset_settings() {
	user_settings.settings_version = SETTINGS_VERSION;
	
	user_settings.led_count = NUM_LEDS;
	user_settings.led_brightness = BRIGHTNESS;
	
	user_settings.joystick_deadzone = JOYSTICK_DEADZONE;
	user_settings.attack_threshold = ATTACK_THRESHOLD;
	
	user_settings.audio_volume = MAX_VOLUME;
	
	user_settings.lives_per_level = LIVES_PER_LEVEL;

  user_settings.player_direction = 1;
  
  user_settings.gravity = 0;
  user_settings.bend_point = VIRTUAL_WORLD_COUNT/2;
	
	settings_eeprom_write();
	
}

void change_setting(char *line) {
  
	
	char setting_val[6];
  char param;
  uint16_t newValue;
	
	if (readBuffer[1] != '='){  // check if the equals sign is there
		Serial.print(F("Missing '=' in command"));
		readIndex = 0;
		return;
  }
	
	// move the value characters into a char array while verifying they are digits
  for(int i=0; i<5; i++) {
	if (i+2 < readIndex) {
		if (isDigit(readBuffer[i+2]))
			setting_val[i] = readBuffer[i+2];
		else {
			Serial.println(F("Invalid setting value"));
			return;
			
		}			
	}
	else
		setting_val[i] = 0;
  }
	
	param = readBuffer[0];
  newValue = atoi(setting_val); // convert the val section to an integer
	
	switch (param) {		 
		
		lastInputTime = millis(); // reset screensaver count		
		
		case 'C': // LED Count
				user_settings.led_count = constrain(newValue, MIN_LEDS, MAX_LEDS);
				settings_eeprom_write();
				if (FastLED.size() < user_settings.led_count)
					reset_cpu(); // reset required to prevent overrun the 
		break;	
			
		case 'B': // brightness
			user_settings.led_brightness = constrain(newValue, MIN_BRIGHTNESS, MAX_BRIGHTNESS);
			FastLED.setBrightness(user_settings.led_brightness);
			settings_eeprom_write();			
		break;
		
		case 'S': // sound
			user_settings.audio_volume = constrain(newValue, MIN_VOLUME, MAX_VOLUME);
			settings_eeprom_write();
		break;
		
		case 'J': // deadzone, joystick
			user_settings.joystick_deadzone = constrain(newValue, MIN_JOYSTICK_DEADZONE, MAX_JOYSTICK_DEADZONE);
			settings_eeprom_write();		
		break;
		
		case 'A': // attack threshold, joystick
			user_settings.attack_threshold = constrain(newValue, MIN_ATTACK_THRESHOLD, MAX_ATTACK_THRESHOLD);
			settings_eeprom_write();
		break;
		
		case 'L': // lives per level
			user_settings.lives_per_level = constrain(newValue, MIN_LIVES_PER_LEVEL, MAX_LIVES_PER_LEVEL);
			settings_eeprom_write();
		break;

    case 'D': // player direction
      user_settings.player_direction = constrain(newValue, 0, 1);
      settings_eeprom_write();
    break;

    case 'G': // gravity
      user_settings.gravity = constrain(newValue, 0, 1);
      settings_eeprom_write();
    break;

    case 'P': // bend point
      user_settings.bend_point = constrain(newValue, 0, VIRTUAL_WORLD_COUNT);
      settings_eeprom_write();
    break;
		
		default:
			Serial.print(F("Command Error: "));
			Serial.println(readBuffer[0]);
			return;
		break;
	
  } 	
	show_settings_menu(); 
  
}

void settings_eeprom_read()
{
	uint8_t ver = EEPROM.read(0);
	uint8_t temp[sizeof(user_settings)];
	bool read_fail = false;

	if (ver != SETTINGS_VERSION) {
		Serial.println(F("Error: Reading EEPROM settings failed"));
		Serial.println(F("Loading defaults"));
		reset_settings();		
		return;
	}				
	
	for (int i=0; i<sizeof(user_settings); i++)
	{
		temp[i] = EEPROM.read(i);
	}	
	
	memcpy((char*)&user_settings, temp, sizeof(user_settings));
	
	// if any values are out of range, reset them all	
	
	if (user_settings.led_count < MIN_LEDS || user_settings.led_count > MAX_LEDS)
		read_fail = true;	
	
	if (user_settings.led_brightness < MIN_BRIGHTNESS || user_settings.led_brightness > MAX_BRIGHTNESS)
		read_fail = true;
	
	if (user_settings.joystick_deadzone < MIN_JOYSTICK_DEADZONE || user_settings.joystick_deadzone > MAX_JOYSTICK_DEADZONE)
		read_fail = true;
	
	if (user_settings.attack_threshold < MIN_ATTACK_THRESHOLD || user_settings.attack_threshold > MAX_ATTACK_THRESHOLD )
		read_fail = true;
	
	if (user_settings.audio_volume < MIN_VOLUME || user_settings.audio_volume > MAX_VOLUME)
		read_fail = true;
	
	if (user_settings.lives_per_level < MIN_LIVES_PER_LEVEL || user_settings.lives_per_level > MAX_LIVES_PER_LEVEL)
		read_fail = true;

  if (user_settings.player_direction > 1)
    read_fail = true; 

  if (user_settings.gravity > 1)
    read_fail = true;

  if (user_settings.bend_point > VIRTUAL_WORLD_COUNT)
    read_fail = true;
	
	if (read_fail) {
		reset_settings();
		
	}
	
}

void settings_eeprom_write() {
	uint8_t temp[sizeof(user_settings)];	
	memcpy(temp, (char*)&user_settings, sizeof(user_settings));
	
	for (int i=0; i<sizeof(user_settings); i++)
	{
		EEPROM.write(i, temp[i]);
	}	
}

void printError(int reason) {
	switch(reason) {
		case ERR_SETTING_NUM:
			Serial.print(F("Error: Invalid setting number"));
		break;
		case ERR_SETTING_RANGE:
			Serial.print(F("Error: Setting out of range"));
		break;
		default:
			Serial.print(F("Error:"));Serial.println(reason);
		break;
	}
}

