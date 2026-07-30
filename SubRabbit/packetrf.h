// packetrf.h
#ifndef PACKETRF_H
#define PACKETRF_H

#include <Arduino.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>

class PacketRF {
public:
    PacketRF();
    ~PacketRF();
    
    void init();
    void send(const byte* data, int len);
    bool receive(byte* buffer, int& len);
    void setRxMode();
    void setTxMode();
    void setCCMode(bool enable);
    void setModulation(int mode);
    void setFrequency(float freq);
    void setDeviation(float dev);
    void setChannel(int channel);
    void setChannelSpacing(float spacing);
    void setRxBandwidth(float bw);
    void setDataRate(float rate);
    void setPower(int power);
    void setSyncMode(int mode);
    void setSyncWord(int high, int low);
    void setAddressCheck(int mode);
    void setAddress(int addr);
    void setWhitening(bool enable);
    void setPacketFormat(int format);
    void setLengthConfig(int config);
    void setPacketLength(int length);
    void setCRC(bool enable);
    void setCRCAutoFlush(bool enable);
    void setDCFilter(bool enable);
    void setManchester(bool enable);
    void setFEC(bool enable);
    void setPreamble(int mode);
    void setPQT(int threshold);
    void setAppendStatus(bool enable);
    void setGDO0(int pin);
    void setGDO2(int pin);
    void setSpiPins(byte sck, byte miso, byte mosi, byte ss);
    bool checkReceiveFlag();
    bool checkCRC();
    int getRssi();
    int getLqi();
    void reset();
    
private:
    static const int MAX_BUFFER = 64;
    byte rxBuffer[MAX_BUFFER];
    byte txBuffer[MAX_BUFFER];
    bool ccmode;
    int gdo0pin;
    int gdo2pin;
};

#endif
