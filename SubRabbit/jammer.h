// jam.h
#ifndef JAM_H
#define JAM_H

#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

class Jammer {
public:
    Jammer();
    ~Jammer();
    
    void start();
    void stop();
    bool isActive() const;
    void update();
    
private:
    bool active;
    static const int JAM_BUFFER_SIZE = 60;
    byte jambuffer[JAM_BUFFER_SIZE];
};

#endif
