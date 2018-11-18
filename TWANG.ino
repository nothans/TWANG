/* 
  TWANG 

  Based on original code by Critters/TWANG

  https://github.com/Critters/TWANG

  Additional contributions from:
  - bdring (serial menu, more levels)
  - dskw (more levels)
  - fablab-muenchen (screensaver rework)
  - chess9876543210 (more screensavers)


  Ported to the Arduino Nano by nuess0r

  CONNECTION for Arduino Nano:
    Pins D2         LED data pin
    Pins D3         LED clock pin
    Pins D5 & 6 & 7 Life LEDs
    Pins D9 & 10    Loudspeaker
    Pins A4         Gyroscope SDA
    Pins A5         Gyroscope SCL

*/
#define VERSION "2018-11-18"

// Required libs
#include "FastLED.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "iSin.h"
#include "RunningMedian.h"

#include <stdint.h> // uint8_t type variables

// Included libs
#include "Enemy.h"
#include "Particle.h"
#include "Spawner.h"
#include "Lava.h"
#include "Boss.h"
#include "Conveyor.h"
#include "settings.h"


// what type of sound device ....pick one
#define USE_TONEAC

#if defined(USE_TONEAC)
#include "toneAC.h"
#define twangInitTone()                   do {} while(0)
#define twangPlayTone(freq, vol)          toneAC(freq, vol)
#define twangPlayToneLen(freq, vol, len)  toneAC(freq, vol, len, true)
#define twangStopTone()                   noToneAC()
#else
#define twangInitTone()                   do {} while(0)
#define twangPlayTone(freq, vol)          do {} while(0)
#define twangPlayToneLen(freq, vol, len)  do {} while(0)
#define twangStopTone()                   do {} while(0)
#endif

// GAME
long previousMillis = 0;           // Time of the last redraw
uint8_t levelNumber = 0;

iSin isin = iSin();


int8_t joystickTilt = 0;              // Stores the angle of the joystick
int joystickWobble = 0;            // Stores the max amount of wobble

// WOBBLE ATTACK
uint8_t attack_width = DEFAULT_ATTACK_WIDTH;
long attackMillis = 0;             // Time the attack started
bool attacking = 0;                // Is the attack in progress?

enum stages {
	STARTUP,
	PLAY,
	WIN,
	DEAD,
	SCREENSAVER,
	BOSS_KILLED,
	GAMEOVER
} stage;

int score;
long stageStartTime;               // Stores the time the stage changed for stages that are time based
int playerPosition;                // Stores the player position
int playerPositionModifier;        // +/- adjustment to player position

bool playerAlive;
long killTime;
uint8_t lives;
bool lastLevel = false;

#ifdef USE_LIFELEDS
    #define LIFE_LEDS 3
    const PROGMEM int lifeLEDs[LIFE_LEDS] = {7, 6, 5}; // these numbers are Arduino GPIO numbers...this is not used in the B. Dring enclosure design
#endif


// POOLS
#define ENEMY_COUNT 10
Enemy enemyPool[ENEMY_COUNT] = {
    Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy(), Enemy()
};

#if defined(ARDUINO_AVR_MEGA2560)
// Arduino Mega 2560
#define PARTICLE_COUNT 40
Particle particlePool[PARTICLE_COUNT] = {
    Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(),
    Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(),
    Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(),
    Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle()
};
#elif defined(ARDUINO_AVR_NANO)
#define PARTICLE_COUNT 24
Particle particlePool[PARTICLE_COUNT] = {
    Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(),
    Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(), Particle(),
    Particle(), Particle(), Particle(), Particle()
};
#else
#error "Please define size of particle pool for your board."
#endif


#define SPAWN_COUNT 2
Spawner spawnPool[SPAWN_COUNT] = {
    Spawner(), Spawner()
};

#define LAVA_COUNT 4
Lava lavaPool[LAVA_COUNT] = {
    Lava(), Lava(), Lava(), Lava()
};

#define CONVEYOR_COUNT 2
Conveyor conveyorPool[CONVEYOR_COUNT] = {
    Conveyor(), Conveyor()
};

Boss boss = Boss();

// MPU
MPU6050 accelgyro;
CRGB leds[MAX_LEDS]; // this is set to the max, but the actual number used is set in FastLED.addLeds below
RunningMedian<int,5> MPUAngleSamples;
RunningMedian<int,5> MPUWobbleSamples;


void setup() {

    Serial.begin(115200);
    settings_eeprom_read();
    showSetupInfo();

    // MPU
    Wire.begin();
    accelgyro.initialize();	

    // initialize sound device (if necessary)
    twangInitTone();

    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, user_settings.led_count);

    FastLED.setBrightness(user_settings.led_brightness);
    FastLED.setDither(1);	

    ScreenSaverMgr screenSaverMgr = ScreenSaverMgr();

    stage = STARTUP;
}

void loop() {
    long mm = millis();
	
    while(Serial.available()) {  // see if there are someone is trying to edit settings via serial port
        processSerial(Serial.read());				
	  }

    if (mm - previousMillis >= MIN_REDRAW_INTERVAL) {
        getInput();
		
        long frameTimer = mm;
        previousMillis = mm;

        if(abs(joystickTilt) > user_settings.joystick_deadzone){
            lastInputTime = mm;
            if(stage == SCREENSAVER){
                levelNumber = -1;
                stageStartTime = mm;
                stage = WIN;
            }
        }else{
            if(lastInputTime+TIMEOUT < mm){
				
                stage = SCREENSAVER;
            }
        }
        if(stage == SCREENSAVER){
            screenSaverTick();
		
        }else if(stage == STARTUP){
            if (stageStartTime+STARTUP_FADE_DUR > mm)
				        tickStartup(mm);
            else
            {
                SFXcomplete();
                levelNumber = 0;
                loadLevel();
            }
        }else if(stage == PLAY){
            // PLAYING
            if(attacking && attackMillis+ATTACK_DURATION < mm) attacking = 0;		
    			
            if (attacking)
    				    SFXattacking();			
    
            // If not attacking, check if they should be
            if(!attacking && joystickWobble > user_settings.attack_threshold){
                attackMillis = mm;
                attacking = 1;
            }
    
            // If still not attacking, move!
            playerPosition += playerPositionModifier;
            if(!attacking){
                SFXtilt(joystickTilt);
                int8_t moveAmount = (joystickTilt/6.0);
                if(user_settings.player_direction) moveAmount = -moveAmount;
                moveAmount = constrain(moveAmount, -MAX_PLAYER_SPEED, MAX_PLAYER_SPEED);
                playerPosition -= moveAmount;
                if(playerPosition < 0) playerPosition = 0;
                // stop player from leaving if boss is alive
                if (boss.Alive() && playerPosition >= VIRTUAL_WORLD_COUNT) // move player back
                    playerPosition = VIRTUAL_WORLD_COUNT - 1;
    
                if(playerPosition >= VIRTUAL_WORLD_COUNT && !boss.Alive()) {
                    // Reached exit!
                    levelComplete();
                    return;
                }
            }

            if(inLava(playerPosition)){
                die();
            }
    
            // Ticks and draw calls
            FastLED.clear();
            tickConveyors();
            tickSpawners();
            tickBoss();
            tickLava();
            tickEnemies();
            drawPlayer();
            drawAttack();
            drawExit();
        }else if(stage == DEAD){
            // DEAD			
            SFXdead();			
            FastLED.clear();
            tickDie(mm);
            if(!tickParticles()){
                if(lives == 0){
                    gameOver();
                } else {
                    loadLevel();
                }
            }
        }else if(stage == WIN){
            // LEVEL COMPLETE   
            tickWin(mm);
        }else if(stage == BOSS_KILLED){
            tickBossKilled(mm);            
        } else if (stage == GAMEOVER) {
            if (stageStartTime+GAMEOVER_FADE_DURATION > mm)
            {
                tickGameover(mm);
            }
            else
            {
                FastLED.clear();	
                levelNumber = 0;
                lives = user_settings.lives_per_level;
                loadLevel();
            }
        }

        FastLED.show();
    }
}

// ---------------------------------
// ------------ LEVELS -------------
// ---------------------------------
void loadLevel(){
	// leave these alone
	FastLED.setBrightness(user_settings.led_brightness);
	updateLives();
  cleanupLevel();
  playerAlive = 1;
	lastLevel = false; // this gets changed on the boss level
	
	/// Defaults...OK to change the following items in the levels below
	attack_width = DEFAULT_ATTACK_WIDTH; 
	playerPosition = 0; 
	
	/* ==== Level Editing Guide ===============
	Level creation is done by adding to or editing the switch statement below
	
	You can add as many levels as you want but you must have a "case"  
	for each level. The case numbers must start at 0 and go up without skipping any numbers.
	
	Don't edit case 0 or the last (boss) level. They are special cases and editing them could
	break the code.
	
	TWANG uses a virtual 1000 LED grid (World). It will then scale that number to your strip, so if you
	want something in the middle of your strip use the number 500. Consider the size of your strip
	when adding features. All time values are specified in milliseconds (1/1000 of a second)
	
	You can add any of the following features.
	
	Enemies: You add up to 10 enemies with the spawnEnemy(...) functions.
		spawnEnemy(position, direction, speed, wobble);
			position: Where the enemy starts 
			direction: If it moves, what direction does it go 0=down, 1=away
			speed: How fast does it move. Typically 1 to 4.
			wobble: 0=regular movement, 1=bouncing back and forth use the speed value
				to set the length of the wobble.
				
	Spawn Pools: This generates and endless source of new enemies. 2 pools max
		spawnPool[index].Spawn(position, rate, speed, direction, activate);
			index: You can have up to 2 pools, use an index of 0 for the first and 1 for the second.
			position: The location the enemies with be generated from. 
			rate: The time in milliseconds between each new enemy
			speed: How fast they move. Typically 1 to 4.
			direction: Directions they go 0=down, 1=away
			activate: The delay in milliseconds before the first enemy
			
	Lava: You can create 4 pools of lava.
		spawnLava(left, right, ontime, offtime, offset, state);
			left: the lower end of the lava pool
			right: the upper end of the lava pool
			ontime: How long the lave stays on.
			offset: the delay before the first switch
			state: does it start on or off_dir
	
	Conveyor: You can create 2 conveyors.
		spawnConveyor(startPoint, endPoint, speed)
			startPoint: The close end of the conveyor
			endPoint: The far end of the conveyor
			speed: positive = away, negative = towards you (must be less than +/- player speed)
	
	===== Other things you can adjust per level ================ 
	
		Player Start position:
			playerPosition = xxx; 
				
			
		The size of the TWANG attack
			attack_width = xxx;
					
	
	*/
    switch(levelNumber){
        case 0: // basic introduction 
            playerPosition = 200;
            spawnEnemy(1, 0, 0, 0);
            break;
        case 1:
            // Slow moving enemy
            spawnEnemy(900, 0, 1, 0);
            break;
        case 2:
            // Spawning enemies at exit every 2 seconds
            spawnPool[0].Spawn(1000, 3000, 2, 0, 0);
            break;
        case 3:
            // Lava intro
            spawnLava(400, 490, 2000, 2000, 0, Lava::OFF);
            spawnEnemy(350, 0, 1, 0);
            spawnPool[0].Spawn(1000, 5500, 3, 0, 0);
            break;
        case 4:
            // Sin enemy
            spawnEnemy(700, 1, 7, 275);
            spawnEnemy(500, 1, 5, 250);
            break;
        case 5:
            // Sin enemy swarm
            spawnEnemy(700, 1, 7, 275);
            spawnEnemy(500, 1, 5, 250);
            spawnEnemy(600, 1, 7, 200);
            spawnEnemy(800, 1, 5, 350);
            spawnEnemy(400, 1, 7, 150);
            spawnEnemy(450, 1, 5, 400);
            break;
        case 6:
            // Conveyor
            spawnConveyor(100, 600, -6);
            spawnEnemy(800, 0, 0, 0);
            break;
        case 7:
            // Drainage
            spawnConveyor(100, 600, 1);
            spawnConveyor(600, 1000, -1);
            spawnEnemy(600, 0, 0, 0);
            spawnPool[0].Spawn(1000, 5500, 3, 0, 0);
            break;
        case 8:
            // Conveyor of enemies
            spawnConveyor(50, 1000, 6);
            spawnEnemy(300, 0, 0, 0);
            spawnEnemy(400, 0, 0, 0);
            spawnEnemy(500, 0, 0, 0);
            spawnEnemy(600, 0, 0, 0);
            spawnEnemy(700, 0, 0, 0);
            spawnEnemy(800, 0, 0, 0);
            spawnEnemy(900, 0, 0, 0);
            break;
        case 9:   // spawn train;
            spawnPool[0].Spawn(900, 1300, 2, 0, 0);
            break;
        case 10:   // spawn train skinny attack width;
            attack_width = 32;
            spawnPool[0].Spawn(900, 1800, 2, 0, 0);
            break;
        case 11:  // evil fast split spawner
            spawnPool[0].Spawn(550, 1500, 2, 0, 0);
            spawnPool[1].Spawn(550, 1500, 2, 1, 0);
            break;
        case 12: // split spawner with exit blocking lava
            spawnPool[0].Spawn(500, 1200, 2, 0, 0);
            spawnPool[1].Spawn(500, 1200, 2, 1, 0);
            spawnLava(900, 950, 2200, 800, 2000, Lava::OFF);
	    break;
        case 13:
            // Lava run
            spawnLava(195, 300, 2000, 2000, 0, Lava::OFF);
            spawnLava(400, 500, 2000, 2000, 0, Lava::OFF);
            spawnLava(600, 700, 2000, 2000, 0, Lava::OFF);
            spawnPool[0].Spawn(1000, 3800, 4, 0, 0);
            break;
        case 14:
            // Sin enemy #2 practice (slow conveyor)
            spawnEnemy(700, 1, 7, 275);
            spawnEnemy(500, 1, 5, 250);
            spawnPool[0].Spawn(1000, 5500, 4, 0, 3000);
            spawnPool[1].Spawn(0, 5500, 5, 1, 10000);
            spawnConveyor(100, 900, -4);
            break;
        case 15:
            // Sin enemy #2 (fast conveyor)
            spawnEnemy(800, 1, 7, 275);
            spawnEnemy(700, 1, 7, 275);
            spawnEnemy(500, 1, 5, 250);
            spawnPool[0].Spawn(1000, 3000, 4, 0, 3000);
            spawnPool[1].Spawn(0, 5500, 5, 1, 10000);
            spawnConveyor(100, 900, -6);
            break;
        case 16:
            // less lava, more enemies
            spawnLava(350, 455, 2000, 2000, 0, false);
            spawnLava(660, 760, 2000, 2000, 0, false);
            spawnPool[0].Spawn(1000, 3800, 4, 0, 270);
            spawnEnemy(800, 0, 0, 0);
            break;
        case 17:
            // pushed towards lava
            spawnConveyor(100, 800, 1);
            spawnLava(800, 850, 1000, 2000, 0, false);
            spawnPool[0].Spawn(1000, 2000, 4, 0, 0);
            break;
        case 18:
            // quick lava
            spawnPool[0].Spawn(0, 2300, 6, 1, 7000);
            spawnLava(200, 400, 1000, 2000, 0, false);
            spawnLava(600, 800, 1000, 2000, 0, false);
            spawnPool[1].Spawn(1000, 2500, 6, 0, 1000);
            break;
        case 19: // (don't edit last level)
            // Boss this should always be the last level			
            spawnBoss();
            break;
    }
    stageStartTime = millis();
    stage = PLAY;
}

void spawnBoss(){
    lastLevel = true;
    boss.Spawn();	
    moveBoss();
}

void moveBoss(){
    int spawnSpeed = 1800;
    if(boss._lives == 2) spawnSpeed = 1600;
    if(boss._lives == 1) spawnSpeed = 1000;
    spawnPool[0].Spawn(boss._pos, spawnSpeed, 3, 0, 0);
    spawnPool[1].Spawn(boss._pos, spawnSpeed, 3, 1, 0);
}

/* ======================== spawn Functions =====================================

   The following spawn functions add items to pools by looking for an inactive
   item in the pool. You can only add as many as the ..._COUNT. Additional attempts 
   to add will be ignored.

   ==============================================================================
*/
void spawnEnemy(int pos, bool dir, uint8_t speed, uint8_t wobble){
    for(uint8_t e = 0; e<ENEMY_COUNT; e++){  // look for one that is not alive for a place to add one
        if(!enemyPool[e].Alive()){
            enemyPool[e].Spawn(pos, dir, speed, wobble);
            enemyPool[e].playerSide = pos > playerPosition?1:-1;
            return;
        }
    }
}

void spawnLava(int left, int right, int ontime, int offtime, int offset, bool state){
    for(uint8_t i = 0; i<LAVA_COUNT; i++){
        if(!lavaPool[i].Alive()){
            lavaPool[i].Spawn(left, right, ontime, offtime, offset, state);
            return;
        }
    }
}

void spawnConveyor(int startPoint, int endPoint, uint8_t speed){
    for(uint8_t i = 0; i<CONVEYOR_COUNT; i++){
        if(!conveyorPool[i]._alive){
            conveyorPool[i].Spawn(startPoint, endPoint, speed);
            return;
        }
    }
}

void cleanupLevel(){
    for(uint8_t i = 0; i<ENEMY_COUNT; i++){
        enemyPool[i].Kill();
    }
    for(uint8_t i = 0; i<PARTICLE_COUNT; i++){
        particlePool[i].Kill();
    }
    for(uint8_t i = 0; i<SPAWN_COUNT; i++){
        spawnPool[i].Kill();
    }
    for(uint8_t i = 0; i<LAVA_COUNT; i++){
        lavaPool[i].Kill();
    }
    for(uint8_t i = 0; i<CONVEYOR_COUNT; i++){
        conveyorPool[i].Kill();
    }
    boss.Kill();
}

void levelComplete(){
    stageStartTime = millis();
    stage = WIN;
    //if(levelNumber == LEVEL_COUNT){
    if (lastLevel) {
        stage = BOSS_KILLED;
    }
    if (levelNumber != 0)  // no points for the first level
    {
        score = score + (lives * 10);  //
    }
}

void nextLevel(){

    levelNumber ++;
    if(lastLevel)
        levelNumber = 0;
    lives = user_settings.lives_per_level;
    loadLevel();
}

void gameOver(){

    levelNumber = 0;
    loadLevel();
}

void die(){
    playerAlive = 0;
    if(levelNumber > 0) 
        lives--;

    if(lives == 0){
       stage = GAMEOVER;
       stageStartTime = millis();
    }
    else
    {
        for(uint8_t p = 0; p < PARTICLE_COUNT; p++){
            particlePool[p].Spawn(playerPosition);
        }
        stageStartTime = millis();
        stage = DEAD;
    }
    killTime = millis();
}

// ----------------------------------
// -------- TICKS & RENDERS ---------
// ----------------------------------
void tickStartup(long mm)
{
    FastLED.clear();
    if(stageStartTime+STARTUP_WIPEUP_DUR > mm) // fill to the top with green
    {
        int n = min(map(((mm-stageStartTime)), 0, STARTUP_WIPEUP_DUR, 0, user_settings.led_count), user_settings.led_count);  // fill from top to bottom
        for(int i = 0; i<= n; i++){			
            leds[i] = CRGB(0, 255, 0);
        }		
    }
    else if(stageStartTime+STARTUP_SPARKLE_DUR > mm) // sparkle the full green bar		
    {
        for(int i = 0; i< user_settings.led_count; i++){		 
            if(random8(30) < 28)
            leds[i] = CRGB(0, 255, 0);  // most are green
            else {
                int flicker = random8(250);
                leds[i] = CRGB(flicker, 150, flicker); // some flicker brighter
            }
        }
    }
    else if (stageStartTime+STARTUP_FADE_DUR > mm) // fade it out to bottom
    {
        int n = max(map(((mm-stageStartTime)), STARTUP_SPARKLE_DUR, STARTUP_FADE_DUR, 0, user_settings.led_count), 0);  // fill from top to bottom
        int brightness = max(map(((mm-stageStartTime)), STARTUP_SPARKLE_DUR, STARTUP_FADE_DUR, 255, 0), 0);
		
        for(int i = n; i< user_settings.led_count; i++){					 
            leds[i] = CRGB(0, brightness, 0);
        }
    }
    SFXFreqSweepWarble(STARTUP_FADE_DUR, millis()-stageStartTime, 40, 400, 20);	
}

void tickEnemies(){
    for(uint8_t i = 0; i<ENEMY_COUNT; i++){
        if(enemyPool[i].Alive()){
            enemyPool[i].Tick();
            // Hit attack?
            if(attacking){
                if(enemyPool[i]._pos > playerPosition-(attack_width/2) && enemyPool[i]._pos < playerPosition+(attack_width/2)){
                   enemyPool[i].Kill();
                   SFXkill();
                }
            }
            if(inLava(enemyPool[i]._pos)){
                enemyPool[i].Kill();
                SFXkill();
            }
            // Draw (if still alive)
            if(enemyPool[i].Alive()) {
                leds[getLED(enemyPool[i]._pos)] = CRGB(255, 0, 0);
            }
            // Hit player?
            if(
                (enemyPool[i].playerSide == 1 && enemyPool[i]._pos <= playerPosition) ||
                (enemyPool[i].playerSide == -1 && enemyPool[i]._pos >= playerPosition)
            ){
                die();
                return;
            }
        }
    }
}

void tickBoss(){
    // DRAW
    if(boss.Alive()){
        boss._ticks ++;
        for(int i = getLED(boss._pos-BOSS_WIDTH/2); i<=getLED(boss._pos+BOSS_WIDTH/2); i++){
            leds[i] = CRGB::DarkRed;
            leds[i] %= 100;
        }
        // CHECK COLLISION
        if(getLED(playerPosition) > getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPosition) < getLED(boss._pos + BOSS_WIDTH)){
            die();
            return;
        }
        // CHECK FOR ATTACK
        if(attacking){
            if(
              (getLED(playerPosition+(attack_width/2)) >= getLED(boss._pos - BOSS_WIDTH/2) && getLED(playerPosition+(attack_width/2)) <= getLED(boss._pos + BOSS_WIDTH/2)) ||
              (getLED(playerPosition-(attack_width/2)) <= getLED(boss._pos + BOSS_WIDTH/2) && getLED(playerPosition-(attack_width/2)) >= getLED(boss._pos - BOSS_WIDTH/2))
            ){
               boss.Hit();
               if(boss.Alive()){
                   moveBoss();
               }else{
                   spawnPool[0].Kill();
                   spawnPool[1].Kill();
               }
            }
        }
    }
}

void drawPlayer(){
    leds[getLED(playerPosition)] = CRGB(0, 255, 0);
}

void drawExit(){
    if(!boss.Alive()){
        leds[user_settings.led_count-1] = CRGB(0, 0, 255);
    }
}

void tickSpawners(){
    long mm = millis();
    for(uint8_t s = 0; s<SPAWN_COUNT; s++){
        if(spawnPool[s].Alive() && spawnPool[s]._activate < mm){
            if(spawnPool[s]._lastSpawned + spawnPool[s]._rate < mm || spawnPool[s]._lastSpawned == 0){
                spawnEnemy(spawnPool[s]._pos, spawnPool[s]._dir, spawnPool[s]._speed, 0);
                spawnPool[s]._lastSpawned = mm;
            }
        }
    }
}

void tickLava(){
    int A, B, p;
    uint8_t i, brightness, flicker;
    long mm = millis();
		const uint8_t lava_off_brightness = WS2812_LAVA_OFF_BRIGHTNESS;

    Lava LP;
    for(i = 0; i<LAVA_COUNT; i++){        
        LP = lavaPool[i];
        if(LP.Alive()){
            A = getLED(LP._left);
            B = getLED(LP._right);
            if(LP._state == Lava::OFF){
                if(LP._lastOn + LP._offtime < mm){
                    LP._state = Lava::ON;
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    flicker = random8(lava_off_brightness);
                    leds[p] = CRGB(lava_off_brightness+flicker, (lava_off_brightness+flicker)/1.5, 0);
                }
            }else if(LP._state == Lava::ON){
                if(LP._lastOn + LP._ontime < mm){
                    LP._state = Lava::OFF;
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    if(random8(30) < 29)
                        leds[p] = CRGB(150, 0, 0);
                    else
                        leds[p] = CRGB(180, 100, 0);
                }
            }
        }
        lavaPool[i] = LP;
    }
}

bool tickParticles(){
    bool stillActive = false;
    uint8_t brightness;
    for(uint8_t p = 0; p < PARTICLE_COUNT; p++){
        if(particlePool[p].Alive()){
            particlePool[p].Tick(user_settings.gravity, user_settings.bend_point);
			
        if (particlePool[p]._power < 5)
        {
            brightness = (5 - particlePool[p]._power) * 10;
            leds[getLED(particlePool[p]._pos)] += CRGB(brightness, brightness/2, brightness/2);\
        }
        else			
            leds[getLED(particlePool[p]._pos)] += CRGB(particlePool[p]._power, 0, 0);\
            stillActive = true;
        }
    }
    return stillActive;
}

void tickConveyors(){
    int b, n, ss, ee, led;
    uint8_t speed, i;
    long m = 10000+millis();
    playerPositionModifier = 0;
		const uint8_t conveyor_brightness = WS2812_CONVEYOR_BRIGHTNES;
	
    uint8_t levels = 5; // brightness levels in conveyor 	
	

    for(i = 0; i<CONVEYOR_COUNT; i++){
        if(conveyorPool[i]._alive){
            speed = constrain(conveyorPool[i]._speed, -MAX_PLAYER_SPEED+1, MAX_PLAYER_SPEED-1);
            ss = getLED(conveyorPool[i]._startPoint);
            ee = getLED(conveyorPool[i]._endPoint);
            for(led = ss; led<ee; led++){
                
                n = (-led + (m/100)) % levels;
                if(speed < 0) 
                    n = (led + (m/100)) % levels;
				
                b = map(n, 5, 0, 0, conveyor_brightness);
                if(b > 0) 
                    leds[led] = CRGB(0, 0, b);
            }

            if(playerPosition > conveyorPool[i]._startPoint && playerPosition < conveyorPool[i]._endPoint){
                playerPositionModifier = speed;
            }
        }
    }
}

void tickBossKilled(long mm) // boss funeral
{
    static uint8_t gHue = 0; 
	
    FastLED.setBrightness(255); // super bright!
	
    uint8_t brightness = 0;
    FastLED.clear();	
	
    if(stageStartTime+6500 > mm){
        gHue++;
        fill_rainbow( leds, user_settings.led_count, gHue, 7); // FastLED's built in rainbow
        if( random8() < 200) {  // add glitter
            leds[ random16(user_settings.led_count) ] += CRGB::White;
        }
        SFXbosskilled();
    }else if(stageStartTime+7000 > mm){
        int n = max(map(((mm-stageStartTime)), 5000, 5500, user_settings.led_count, 0), 0);
        for(int i = 0; i< n; i++){
            brightness = (sin(((i*10)+mm)/500.0)+1)*255;
            leds[i].setHSV(brightness, 255, 50);
        }
        SFXcomplete();
    }else{		
        nextLevel();
    }
}

void tickDie(long mm) { // a short bright explosion...particles persist after it.
    const int duration = 200; // milliseconds
    const int width = 10;     // half width of the explosion

    if(stageStartTime+duration > mm) {// Spread red from player position up and down the width

        uint8_t brightness = map((mm-stageStartTime), 0, duration, 255, 50); // this allows a fade from white to red

        // fill up
        int n = max(map(((mm-stageStartTime)), 0, duration, getLED(playerPosition), getLED(playerPosition)+width), 0);
        for(int i = getLED(playerPosition); i<= n; i++){
            leds[i] = CRGB(255, brightness, brightness);
        }

        // fill to down
        n = max(map(((mm-stageStartTime)), 0, duration, getLED(playerPosition), getLED(playerPosition)-width), 0);
        for(int i = getLED(playerPosition); i>= n; i--){
            leds[i] = CRGB(255, brightness, brightness);
        }
    }
}

void tickGameover(long mm) {
    uint8_t brightness = 0;
    FastLED.clear();
    if(stageStartTime+GAMEOVER_SPREAD_DURATION > mm) // Spread red from player position to top and bottom
    {
      // fill to top
      int n = max(map(((mm-stageStartTime)), 0, GAMEOVER_SPREAD_DURATION, getLED(playerPosition), user_settings.led_count), 0);
      for(int i = getLED(playerPosition); i<= n; i++){
            leds[i] = CRGB(255, 0, 0);
      }
      // fill to bottom
      n = max(map(((mm-stageStartTime)), 0, GAMEOVER_SPREAD_DURATION, getLED(playerPosition), 0), 0);
      for(int i = getLED(playerPosition); i>= n; i--){
            leds[i] = CRGB(255, 0, 0);
      }
      SFXgameover();
    }
    else if(stageStartTime+GAMEOVER_FADE_DURATION > mm)  // fade down to bottom and fade brightness
    {
      int n = max(map(((mm-stageStartTime)), GAMEOVER_FADE_DURATION, GAMEOVER_SPREAD_DURATION, 0, user_settings.led_count), 0);
      brightness =  map(((mm-stageStartTime)), GAMEOVER_SPREAD_DURATION, GAMEOVER_FADE_DURATION, 255, 0);

      for(int i = 0; i<= n; i++){
            leds[i] = CRGB(brightness, 0, 0);
      }
      SFXcomplete();
    }

}

void tickWin(long mm) {
    FastLED.clear();
    if(stageStartTime+WIN_FILL_DURATION > mm){
        int n = max(map(((mm-stageStartTime)), 0, WIN_FILL_DURATION, user_settings.led_count, 0), 0);  // fill from top to bottom
        for(int i = user_settings.led_count; i>= n; i--){
            leds[i] = CRGB(0, 255, 0);
        }
        SFXwin();
    }else if(stageStartTime+WIN_CLEAR_DURATION > mm){
        int n = max(map(((mm-stageStartTime)), WIN_FILL_DURATION, WIN_CLEAR_DURATION, user_settings.led_count, 0), 0);  // clear from top to bottom
        for(int i = 0; i< n; i++){
            leds[i] = CRGB(0, 255, 0);
        }
        SFXwin();
    }else if(stageStartTime+WIN_OFF_DURATION > mm){   // wait a while with leds off
        leds[0] = CRGB(0, 255, 0);
    }else{
        nextLevel();
    }
}


void drawLives()
{
    // show how many lives are left by drawing a short line of green leds for each life
    SFXcomplete();  // stop any sounds
    FastLED.clear();

    int pos = 0;
    for (uint8_t i = 0; i < lives; i++)
    {
            for (uint8_t j=0; j<4; j++)
            {
                leds[pos++] = CRGB(0, 255, 0);
                FastLED.show();
            }
            leds[pos++] = CRGB(0, 0, 0);
    }
    FastLED.show();
    delay(1000);
    FastLED.clear();
}

void drawAttack(){
    if(!attacking) return;
    int n = map(millis() - attackMillis, 0, ATTACK_DURATION, 100, 5);
    for(int i = getLED(playerPosition-(attack_width/2))+1; i<=getLED(playerPosition+(attack_width/2))-1; i++){
        leds[i] = CRGB(0, 0, n);
    }
    if(n > 90) {
        n = 255;
        leds[getLED(playerPosition)] = CRGB(255, 255, 255);
    }else{
        n = 0;
        leds[getLED(playerPosition)] = CRGB(0, 255, 0);
    }
    leds[getLED(playerPosition-(attack_width/2))] = CRGB(n, n, 255);
    leds[getLED(playerPosition+(attack_width/2))] = CRGB(n, n, 255);
}

int getLED(int pos){
    // The world is 1000 pixels wide, this converts world units into an LED number
    return constrain((int)map(pos, 0, VIRTUAL_WORLD_COUNT, 0, user_settings.led_count-1), 0, user_settings.led_count-1);
}

bool inLava(int pos){
    // Returns if the player is in active lava
    int i;
    Lava LP;
    for(i = 0; i<LAVA_COUNT; i++){
        LP = lavaPool[i];
        if(LP.Alive() && LP._state == Lava::ON){
            if(LP._left < pos && LP._right > pos) return true;
        }
    }
    return false;
}

void initLifeLEDs(){
#ifdef USE_LIFELEDS  
    // Life LEDs
    for(uint8_t i = 0; i<LIFE_LEDS; i++){
        pinMode(pgm_read_byte_near(lifeLEDs[i]), OUTPUT);
        digitalWrite(pgm_read_byte_near(lifeLEDs[i]), HIGH);
    }
#endif
}

void updateLives(){
    #ifdef USE_LIFELEDS
        // Updates the life LEDs to show how many lives the player has left
        for(uint8_t i = 0; i<LIFE_LEDS; i++){
           digitalWrite(pgm_read_byte_near(lifeLEDs[i]), lives>i?HIGH:LOW);
        }
    #endif

    drawLives();
}

// ---------------------------------
// --------- SCREENSAVER -----------
// ---------------------------------
void screenSaverTick(){
    int n, c, i;
    long mm = millis();
    uint8_t mode = (mm/30000)%5;

    SFXcomplete(); // make sure there is not sound...play testing showed this to be a problem

    if(mode == 0){
        // Marching green <> orange
        for(i = 0; i<user_settings.led_count; i++){
            leds[i].nscale8(250);
        }

        n = (mm/250)%10;
        c = 20+((sin(mm/5000.00)+1)*33);
        for(i = 0; i<user_settings.led_count; i++){
            if(i%10 == n){
                leds[i] = CHSV( c, 255, 150);
            }
        }
    }else if(mode == 1){
        // Random flashes
        for(i = 0; i<user_settings.led_count; i++){
            leds[i].nscale8(250);
        }
        randomSeed(mm);
        for(i = 0; i<user_settings.led_count; i++){
            if(random8(20) == 0){
                leds[i] = CHSV( 25, 255, 100);
            }
        }
    }else if(mode == 2){
        const uint8_t dotspeed = 22;
        const uint8_t dotsInBowlsCount = 3;
        const uint16_t dotDistance = 65535/dotsInBowlsCount;
        const uint8_t dotBrightness = 255;

        for (i = 0; i < user_settings.led_count; i ++)
        {
            leds[i] = CRGB::Black;
        }

        for (i = 0; i < dotsInBowlsCount; i ++)
        {
            mm = ((i*dotDistance) + millis()*dotspeed);
            n = map(sin16_avr(mm),-32767,32767,2,user_settings.led_count-3);
            c = mm % 255;
            leds[n-2] += CHSV(c, 255, dotBrightness/4);
            leds[n-1] += CHSV(c, 255, dotBrightness/2);
            leds[n]   += CHSV(c, 255, dotBrightness);
            leds[n+1] += CHSV(c, 255, dotBrightness/2);
            leds[n+2] += CHSV(c, 255, dotBrightness/4);
        }
    }else if(mode == 3){
        for(i = 0; i<user_settings.led_count; i++)
        {
            leds[i].fadeToBlackBy(128);
        }
        randomSeed(mm);
        c = mm%800;
        if (c < 240)
        {
            n = 121 - c/2;
        }
        else
        {
            n = 1;
        }
        for(i = 0; i<user_settings.led_count; i++)
        {
            if ((random8() <= n))
            {
                leds[i] = CRGB(100,100,100);
            }
        }
    }else if(mode >= 4){
        for (i = 0; i < user_settings.led_count; i ++)
        {
            if (((i+(int)(mm/100))%5)==0)
            {
                leds[i] = CRGB(100,100,100);
            }
            else
            {
                leds[i] = CRGB::Black;
            }
        }
    }
}

// ---------------------------------
// ----------- JOYSTICK ------------
// ---------------------------------
void getInput(){
    // This is responsible for the player movement speed and attacking.
    // You can replace it with anything you want that passes a -90>+90 value to joystickTilt
    // and any value to joystickWobble that is greater than ATTACK_THRESHOLD (defined at start)
    // For example you could use 3 momentary buttons:
        // if(digitalRead(leftButtonPinNumber) == HIGH) joystickTilt = -90;
        // if(digitalRead(rightButtonPinNumber) == HIGH) joystickTilt = 90;
        // if(digitalRead(attackButtonPinNumber) == HIGH) joystickWobble = ATTACK_THRESHOLD;
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    int a = (JOYSTICK_ORIENTATION == 0?ax:(JOYSTICK_ORIENTATION == 1?ay:az))/166;
    int g = (JOYSTICK_ORIENTATION == 0?gx:(JOYSTICK_ORIENTATION == 1?gy:gz));

    if(abs(a) < user_settings.joystick_deadzone) a = 0;
    if(a > 0) a -= user_settings.joystick_deadzone;
    if(a < 0) a += user_settings.joystick_deadzone;
    MPUAngleSamples.add(a);
    MPUWobbleSamples.add(g);

    MPUAngleSamples.getMedian(a);
    joystickTilt = (int8_t)a;
    if(JOYSTICK_DIRECTION == 1) {
        joystickTilt = 0-joystickTilt;
    }
    MPUWobbleSamples.getHighest(a);
    joystickWobble = abs(a);
}

// ---------------------------------
// -------------- SFX --------------
// ---------------------------------

/*
   This is used sweep across (up or down) a frequency range for a specified duration.
   A sin based warble is added to the frequency. This function is meant to be called
   on each frame to adjust the frequency in sync with an animation

   duration     = over what time period is this mapped
   elapsedTime  = how far into the duration are we in
   freqStart    = the beginning frequency
   freqEnd      = the ending frequency
   warble       = the amount of warble added (0 disables)


*/
void SFXFreqSweepWarble(int duration, int elapsedTime, int freqStart, int freqEnd, int warble)
{
    int freq = map_constrain(elapsedTime, 0, duration, freqStart, freqEnd);
    if (warble)
        warble = map(sin(millis()/20.0)*1000.0, -1000, 1000, 0, warble);

    twangPlayTone(freq + warble, user_settings.audio_volume);
}

/*

   This is used sweep across (up or down) a frequency range for a specified duration.
   Random noise is optionally added to the frequency. This function is meant to be called
   on each frame to adjust the frequency in sync with an animation

   duration     = over what time period is this mapped
   elapsedTime  = how far into the duration are we in
   freqStart    = the beginning frequency
   freqEnd      = the ending frequency
   noiseFactor  = the amount of noise to added/subtracted (0 disables)


*/
void SFXFreqSweepNoise(int duration, int elapsedTime, int freqStart, int freqEnd, uint8_t noiseFactor)
{
    int freq;

    if (elapsedTime > duration)
        freq = freqEnd;
    else
        freq = map(elapsedTime, 0, duration, freqStart, freqEnd);

    if (noiseFactor)
        noiseFactor = noiseFactor - random8(noiseFactor / 2);

    twangPlayTone(freq + noiseFactor, user_settings.audio_volume);
}


void SFXtilt(int amount){
    int f = map(abs(amount), 0, 90, 80, 900)+random8(100);
    if(playerPositionModifier < 0) f -= 500;
    if(playerPositionModifier > 0) f += 200;
    twangPlayTone(f, min(min(abs(amount)/9, 5), user_settings.audio_volume));

}
void SFXattacking(){
    int freq = map(sin(millis()/2.0)*1000.0, -1000, 1000, 500, 600);
    if(random8(5)== 0){
      freq *= 3;
    }
    twangPlayTone(freq, user_settings.audio_volume);
}
void SFXdead(){
    SFXFreqSweepNoise(1000, millis()-killTime, 1000, 10, 200);
}

void SFXgameover(){
    SFXFreqSweepWarble(GAMEOVER_SPREAD_DURATION, millis()-killTime, 440, 20, 60);
}

void SFXkill(){
    twangPlayToneLen(2000, user_settings.audio_volume, 1000);
}
void SFXwin(){
    SFXFreqSweepWarble(WIN_OFF_DURATION, millis()-stageStartTime, 40, 400, 20);
}

void SFXbosskilled()
{
    SFXFreqSweepWarble(7000, millis()-stageStartTime, 75, 1100, 60);
}

void SFXcomplete(){
    twangStopTone();
}

/*
    This works just like the map function except x is constrained to the range of in_min and in_max
*/
int map_constrain(int x, int in_min, int in_max, int out_min, int out_max)
{
    // constrain the x value to be between in_min and in_max
    if (in_max > in_min){   // map allows min to be larger than max, but constrain does not
        x = constrain(x, in_min, in_max);
    }
    else {
        x = constrain(x, in_max, in_min);
    }
    
    return map(x, in_min, in_max, out_min, out_max);
}

void showSetupInfo()
{
    Serial.print(F("\r\nTWANG VERSION: ")); Serial.println(F(VERSION));
    show_settings_menu();
}
