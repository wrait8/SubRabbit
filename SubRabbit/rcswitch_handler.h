// rcswitch_handler.h
#ifndef RCSWITCH_HANDLER_H
#define RCSWITCH_HANDLER_H

#include <Arduino.h>
#include <RCSwitch.h>

class RCSwitchHandler {
public:
    RCSwitchHandler();
    ~RCSwitchHandler();
    
    void init(int gdoPin);
    void enableReceive();
    void enableTransmit();
    void disable();
    bool available();
    unsigned long getValue();
    unsigned int getBits();
    unsigned int getDelay();
    unsigned int getProtocol();
    void resetAvailable();
    void send(unsigned long code, unsigned int bits);
    void send(unsigned long code, unsigned int bits, unsigned int delay);
    void send(unsigned long code, unsigned int bits, unsigned int delay, unsigned int protocol);
    void setProtocol(unsigned int protocol);
    void setPulseLength(unsigned int delay);
    void setRepeatTransmit(int repeats);
    int getReceivedRawData(unsigned int* raw);
    static const char* bin2tristate(const char* bin);
    static char* dec2binWzerofill(unsigned long Dec, unsigned int bitLength);
    
private:
    RCSwitch mySwitch;
    bool initialized;
    int gdoPin;
    unsigned int rawData[128];
    unsigned long lastCode;
    unsigned int lastBits;
    unsigned int lastDelay;
    unsigned int lastProtocol;
    

};

#endif
