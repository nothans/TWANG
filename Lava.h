#include "Arduino.h"

/*
startPoint: 0 to 1000
endPoint: 0 to 1000, combined with startPoint this sets the location and size of the lava
ontime: How long (ms) the lava is ON for
offtime: How long (ms) the lava is OFF for
offset: How long (ms) after the level starts before the lava turns on, use this to create patterns with multiple lavas
*/
class Lava
{
  public:
    void Spawn(int left, int right, int ontime, int offtime, int offset, int state);
    void Kill();
    int Alive();
    int _left;
    int _right;
    int _ontime;
    int _offtime;
    int _offset;
    long _lastOn;
    int _state;
    static const int OFF = 0;
    static const int ON = 1;
  private:
    int _alive;
};

void Lava::Spawn(int left, int right, int ontime, int offtime, int offset, int state){
    _left = left;
    _right = right;
    _ontime = ontime;
    _offtime = offtime;
    _offset = offset;
    _alive = 1;
    _lastOn = millis()-offset;
    _state = state;
}

void Lava::Kill(){
    _alive = 0;
}

int Lava::Alive(){
    return _alive;
}
