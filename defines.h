
// GAME
#define TIMEOUT              20000
#define LEVEL_COUNT          9
#define MAX_VOLUME           10

// LED setup
#define NUM_LEDS             300
#define DATA_PIN             3
//#define CLOCK_PIN            4
#define LED_COLOR_ORDER      BGR//GBR
#define BRIGHTNESS           255
#define DIRECTION            1     // 0 = right to left, 1 = left to right
#define MIN_REDRAW_INTERVAL  16    // Min redraw interval (ms) 33 = 30fps / 16 = 63fps
#define USE_GRAVITY          1     // 0/1 use gravity (LED strip going up wall)
#define BEND_POINT           0   // 0/1000 point at which the LED strip goes up the wall

// JOYSTICK
#define JOYSTICK_ORIENTATION 1     // 0, 1 or 2 to set the angle of the joystick
#define JOYSTICK_DIRECTION   0     // 0/1 to flip joystick direction
#define ATTACK_THRESHOLD     30000 // The threshold that triggers an attack
#define JOYSTICK_DEADZONE    5     // Angle to ignore

// WOBBLE ATTACK
#define ATTACK_WIDTH        70     // Width of the wobble attack, world is 1000 wide
#define ATTACK_DURATION     500    // Duration of a wobble attack (ms)
#define BOSS_WIDTH          40

// PLAYER
#define MAX_PLAYER_SPEED    10     // Max move speed of the player

