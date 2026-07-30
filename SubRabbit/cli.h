// cli.h
#ifndef CLI_H
#define CLI_H

#include <Arduino.h>
#include "packetrf.h"
#include "recorder.h"
#include "jammer.h"
#include "analyzer.h"
#include "rcswitch_handler.h"

class CommandHandler {
public:
    CommandHandler();
    ~CommandHandler();
    
    void init();
    void processLine(const char* line);
    void handleRx();
    void handleJam();
    void handleChat();
    bool isChatMode() const;
    void setChatMode(bool enabled);
    void printHelp();
    
private:
    PacketRF rf;
    Recorder recorder;
    Jammer jammer;
    FrequencyAnalyzer analyzer;
    RCSwitchHandler rcSwitch;
    
    bool receivingMode;
    bool jammingMode;
    bool recordingMode;
    bool chatMode;
    bool echoEnabled;
    
    static const int BUFFER_SIZE = 128;
    char textBuffer[BUFFER_SIZE];
    byte byteBuffer[64];
    
    unsigned long lastSigCode;
    unsigned int lastSigBits;
    unsigned int lastSigDelay;
    unsigned int lastSigProtocol;
    
    // Command handlers
    void cmdHelp();
    void cmdSetModulation(const char* args);
    void cmdSetFrequency(const char* args);
    void cmdSetDeviation(const char* args);
    void cmdSetChannel(const char* args);
    void cmdSetChannelSpacing(const char* args);
    void cmdSetRxBandwidth(const char* args);
    void cmdSetDataRate(const char* args);
    void cmdSetPower(const char* args);
    void cmdSetSyncMode(const char* args);
    void cmdSetSyncWord(const char* args);
    void cmdSetAddressCheck(const char* args);
    void cmdSetAddress(const char* args);
    void cmdSetWhitening(const char* args);
    void cmdSetPacketFormat(const char* args);
    void cmdSetLengthConfig(const char* args);
    void cmdSetPacketLength(const char* args);
    void cmdSetCRC(const char* args);
    void cmdSetCRCAutoFlush(const char* args);
    void cmdSetDCFilter(const char* args);
    void cmdSetManchester(const char* args);
    void cmdSetFEC(const char* args);
    void cmdSetPreamble(const char* args);
    void cmdSetPQT(const char* args);
    void cmdSetAppendStatus(const char* args);
    void cmdGetRSSI();
    void cmdAnalyze();
    void cmdStartChat();
    void cmdStopChat();
    void cmdStartRx();
    void cmdStopRx();
    void cmdTransmit(const char* args);
    void cmdStartJam();
    void cmdStopJam();
    void cmdBruteForce(const char* args);
    void cmdStartRec();
    void cmdStopRec();
    void cmdAddFrame(const char* args);
    void cmdAddRaw(const char* args);
    void cmdShowFrames();
    void cmdShowRaw();
    void cmdShowBits();
    void cmdFlush();
    void cmdPlay(const char* args);
    void cmdPlayAll();
    void cmdPlayRaw(const char* args);
    void cmdRxRaw(const char* args);
    void cmdRecRaw(const char* args);
    unsigned long findMostCommonPulse(unsigned long* pulses, int count);
    void cmdRecSig();
    void cmdPlaySig();
    void cmdSave();
    void cmdLoad();
    void cmdSaveRaw();
    void cmdLoadRaw();
    void cmdSaveSig();
    void cmdLoadSig();
    void cmdShowSig(); 
    void cmdEcho(const char* args);
    void cmdStop();
    void cmdReset();
    void cmdInit();
    void cmdStatus();
    
    void hexToAscii(byte* ascii, const byte* hex, int len);
    void asciiToHex(byte* hex, const byte* ascii, int len);
};

#endif
