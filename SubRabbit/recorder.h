// recorder.h
#ifndef RECORDER_H
#define RECORDER_H

#include <Arduino.h>
#include <EEPROM.h>

class Recorder {
public:
    Recorder();
    ~Recorder();
    
    void init();
    void startRecording();
    void stopRecording();
    bool isRecording() const;
    void addFrame(const byte* data, int len);
    void addRaw(const byte* data, int len);
    void playFrame(int index);
    void playAll();
    void playRaw(int interval);
    void showFrames();
    void showRaw();
    void showBits();
    void flush();
    void save();
    void load();
    void saveRaw();
    void loadRaw();
    int getFrameCount() const;
    int getBufferPos() const;
    
private:
    static const int CC_BUFFER_SIZE = 64;
    static const int RECORDING_BUFFER_SIZE = 4096;
    static const int EEPROM_SIZE = 512;
    
    bool recording;
    int bufferPos;
    int frameCount;
    byte recordingBuffer[RECORDING_BUFFER_SIZE];
    byte tempBuffer[CC_BUFFER_SIZE];
    
    void ensureBufferSpace(int needed);
    void hexToAscii(byte* ascii, const byte* hex, int len);
    void asciiToHex(byte* hex, const byte* ascii, int len);
    void saveToEEPROM(int offset, const byte* data, int len);
    void loadFromEEPROM(int offset, byte* data, int len);
};

#endif
