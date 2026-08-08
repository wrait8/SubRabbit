#include "rcswitch_handler.h"
#include <SmartRC_CC1101.h>

RCSwitchHandler::RCSwitchHandler() 
    : initialized(false), gdoPin(2), lastCode(0), lastBits(0), lastDelay(0), lastProtocol(0) {
    memset(rawData, 0, sizeof(rawData));
}

RCSwitchHandler::~RCSwitchHandler() {}

void RCSwitchHandler::init(int gdoPin) {
    this->gdoPin = gdoPin;
    initialized = true;
}

void RCSwitchHandler::enableReceive() {
    if (!initialized) return;
    
    // Setup CC1101 for RCSwitch (like original code)
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.SetRx();
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetRx();
    pinMode(gdoPin, INPUT);
    
    mySwitch.enableReceive(gdoPin);
}

void RCSwitchHandler::enableTransmit() {
    if (!initialized) return;
    
    // Setup CC1101 for transmit
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    
    mySwitch.enableTransmit(gdoPin);
}

void RCSwitchHandler::disable() {
    mySwitch.disableReceive();
    mySwitch.disableTransmit();
}

bool RCSwitchHandler::available() {
    if (!initialized) return false;
    return mySwitch.available();
}

unsigned long RCSwitchHandler::getValue() {
    lastCode = mySwitch.getReceivedValue();
    return lastCode;
}

unsigned int RCSwitchHandler::getBits() {
    lastBits = mySwitch.getReceivedBitlength();
    return lastBits;
}

unsigned int RCSwitchHandler::getDelay() {
    lastDelay = mySwitch.getReceivedDelay();
    return lastDelay;
}

unsigned int RCSwitchHandler::getProtocol() {
    lastProtocol = mySwitch.getReceivedProtocol();
    return lastProtocol;
}

void RCSwitchHandler::resetAvailable() {
    mySwitch.resetAvailable();
}

void RCSwitchHandler::send(unsigned long code, unsigned int bits) {
    if (!initialized) return;
    
    // Setup CC1101 for transmission
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    mySwitch.enableTransmit(gdoPin);
    delay(200);
    
    mySwitch.send(code, bits);
    
    // Restore CC1101
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setPktFormat(0);
    ELECHOUSE_cc1101.SetTx();
}

void RCSwitchHandler::send(unsigned long code, unsigned int bits, unsigned int delay) {
    if (!initialized) return;
    mySwitch.setPulseLength(delay);
    send(code, bits);
}

void RCSwitchHandler::send(unsigned long code, unsigned int bits, unsigned int delay, unsigned int protocol) {
    if (!initialized) return;
    mySwitch.setProtocol(protocol);
    mySwitch.setPulseLength(delay);
    send(code, bits);
}

void RCSwitchHandler::setProtocol(unsigned int protocol) {
    mySwitch.setProtocol(protocol);
}

void RCSwitchHandler::setPulseLength(unsigned int delay) {
    mySwitch.setPulseLength(delay);
}

void RCSwitchHandler::setRepeatTransmit(int repeats) {
    mySwitch.setRepeatTransmit(repeats);
}

int RCSwitchHandler::getReceivedRawData(unsigned int* raw) {
    unsigned int* data = mySwitch.getReceivedRawdata();
    for (int i = 0; i < 128; i++) {
        raw[i] = data[i];
        if (data[i] == 0) {
            return i;
        }
    }
    return 128;
}

const char* RCSwitchHandler::bin2tristate(const char* bin) {
    static char returnValue[50];
    int pos = 0;
    int pos2 = 0;
    while (bin[pos] != '\0' && bin[pos+1] != '\0') {
        if (bin[pos] == '0' && bin[pos+1] == '0') {
            returnValue[pos2] = '0';
        } else if (bin[pos] == '1' && bin[pos+1] == '1') {
            returnValue[pos2] = '1';
        } else if (bin[pos] == '0' && bin[pos+1] == '1') {
            returnValue[pos2] = 'F';
        } else {
            return "not applicable";
        }
        pos += 2;
        pos2++;
    }
    returnValue[pos2] = '\0';
    return returnValue;
}

char* RCSwitchHandler::dec2binWzerofill(unsigned long Dec, unsigned int bitLength) {
    static char bin[64];
    unsigned int i = 0;
    
    while (Dec > 0) {
        bin[32+i++] = ((Dec & 1) > 0) ? '1' : '0';
        Dec = Dec >> 1;
    }
    
    for (unsigned int j = 0; j < bitLength; j++) {
        if (j >= bitLength - i) {
            bin[j] = bin[31 + i - (j - (bitLength - i))];
        } else {
            bin[j] = '0';
        }
    }
    bin[bitLength] = '\0';
    return bin;
}
