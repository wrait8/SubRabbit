// jam.cpp
#include "jammer.h"

Jammer::Jammer() : active(false) {}

Jammer::~Jammer() {}

void Jammer::start() {
    active = true;
    Serial.println("Jamming started");
}

void Jammer::stop() {
    active = false;
    Serial.println("Jamming stopped");
}

bool Jammer::isActive() const {
    return active;
}

void Jammer::update() {
    if (!active) return;
    
    randomSeed(analogRead(0));
    for (int i = 0; i < JAM_BUFFER_SIZE; i++) {
        jambuffer[i] = (byte)random(255);
    }
    ELECHOUSE_cc1101.SendData(jambuffer, JAM_BUFFER_SIZE);
}
