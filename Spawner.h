#include "Arduino.h"
/*

A spawn pool is a point which spawns enemies forever

position: 0 to 1000  (start position on the strip)
rate: milliseconds between spawns, 1000 = 1 second
speed: speed of the enemies it spawns
direction: 0=towards start, 1=away from start

*/
class Spawner
{
  public:
    void Spawn(int pos, int rate, int speed, int dir, long activate);  
    void Kill();
    int Alive();
    int _pos;
    int _rate;
    int _speed;
    int _dir;
    long _lastSpawned;
    long _activate;
  private:
    int _alive;
};

void Spawner::Spawn(int pos, int rate, int speed, int dir, long activate){
    _pos = pos;
    _rate = rate;
    _speed = speed;
    _dir = dir;
    _activate = millis()+activate;
    _alive = 1;
}

void Spawner::Kill(){
    _alive = 0;
    _lastSpawned = 0;
}

int Spawner::Alive(){
    return _alive;
}
