// recorder.cpp
#include "recorder.h"
#include <SmartRC_CC1101.h>

Recorder::Recorder() : recording(false), bufferPos(0), frameCount(0) {
    memset(recordingBuffer, 0, RECORDING_BUFFER_SIZE);
    memset(tempBuffer, 0, CC_BUFFER_SIZE);
}

Recorder::~Recorder() {}

void Recorder::init() {
    EEPROM.begin(EEPROM_SIZE);
    flush();
}

void Recorder::startRecording() {
    recording = true;
    flush();
    Serial.println("Recording started");
}

void Recorder::stopRecording() {
    recording = false;
    Serial.printf("Recording stopped. Frames stored: %d\n", frameCount);
}

bool Recorder::isRecording() const {
    return recording;
}

void Recorder::ensureBufferSpace(int needed) {
    if (bufferPos + needed >= RECORDING_BUFFER_SIZE) {
        Serial.println("Buffer full! Stopping recording.");
        recording = false;
    }
}

void Recorder::addFrame(const byte* data, int len) {
    if (!recording) return;
    if (len > CC_BUFFER_SIZE) len = CC_BUFFER_SIZE;
    
    int totalNeeded = len + 1; // +1 for length byte
    ensureBufferSpace(totalNeeded);
    if (!recording) return;
    
    recordingBuffer[bufferPos++] = len;
    memcpy(&recordingBuffer[bufferPos], data, len);
    bufferPos += len;
    frameCount++;
}

void Recorder::addRaw(const byte* data, int len) {
    // Remove the recording check - we want to add raw data anytime
    // if (!recording) return;  // REMOVE THIS LINE
    
    // Make sure we don't overflow
    if (bufferPos + len >= RECORDING_BUFFER_SIZE) {
        len = RECORDING_BUFFER_SIZE - bufferPos;
        if (len <= 0) {
            Serial.println("Buffer full!");
            return;
        }
    }
    
    // Copy the data to the buffer
    memcpy(&recordingBuffer[bufferPos], data, len);
    bufferPos += len;
    
    // Debug output to confirm data is being added
    Serial.printf("Added %d bytes to buffer. Total: %d bytes\n", len, bufferPos);
}

void Recorder::hexToAscii(byte* ascii, const byte* hex, int len) {
    for (int i = 0; i < len; i++) {
        byte high = hex[i] >> 4;
        byte low = hex[i] & 0x0F;
        ascii[2*i] = high > 9 ? (high - 10 + 'A') : (high + '0');
        ascii[2*i+1] = low > 9 ? (low - 10 + 'A') : (low + '0');
    }
    ascii[2*len] = '\0';
}

void Recorder::asciiToHex(byte* hex, const byte* ascii, int len) {
    for (int i = 0; i < len/2; i++) {
        byte val = 0;
        byte high = ascii[2*i];
        byte low = ascii[2*i+1];
        
        if (high >= '0' && high <= '9') val = (high - '0') * 16;
        else if (high >= 'A' && high <= 'F') val = (high - 'A' + 10) * 16;
        else if (high >= 'a' && high <= 'f') val = (high - 'a' + 10) * 16;
        
        if (low >= '0' && low <= '9') val += (low - '0');
        else if (low >= 'A' && low <= 'F') val += (low - 'A' + 10);
        else if (low >= 'a' && low <= 'f') val += (low - 'a' + 10);
        
        hex[i] = val;
    }
}

void Recorder::playFrame(int index) {
    if (index < 0 || index >= frameCount) {
        Serial.println("Invalid frame index");
        return;
    }
    
    int pos = 0;
    for (int i = 0; i < index; i++) {
        int len = recordingBuffer[pos];
        pos += len + 1;
    }
    
    int len = recordingBuffer[pos++];
    memcpy(tempBuffer, &recordingBuffer[pos], len);
    ELECHOUSE_cc1101.SendData(tempBuffer, len);
}

void Recorder::playAll() {
    if (frameCount == 0) {
        Serial.println("No frames to play");
        return;
    }
    
    int pos = 0;
    for (int i = 0; i < frameCount; i++) {
        int len = recordingBuffer[pos++];
        memcpy(tempBuffer, &recordingBuffer[pos], len);
        ELECHOUSE_cc1101.SendData(tempBuffer, len);
        pos += len;
        delay(10);
    }
    Serial.printf("Played %d frames\n", frameCount);
}

void Recorder::playRaw(int interval) {
    if (interval <= 0) {
        Serial.println("Invalid interval");
        return;
    }
    
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    pinMode(2, OUTPUT);
    
    for (int i = 0; i < RECORDING_BUFFER_SIZE; i++) {
        byte data = recordingBuffer[i];
        for (int j = 7; j >= 0; j--) {
            digitalWrite(2, bitRead(data, j));
            delayMicroseconds(interval);
        }
    }
    
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setPktFormat(0);
    ELECHOUSE_cc1101.SetTx();
}

void Recorder::showFrames() {
    if (frameCount == 0) {
        Serial.println("No frames in buffer");
        return;
    }
    
    int pos = 0;
    for (int i = 0; i < frameCount; i++) {
        int len = recordingBuffer[pos++];
        hexToAscii(tempBuffer, &recordingBuffer[pos], len);
        Serial.printf("Frame %d: %s\n", i+1, tempBuffer);
        pos += len;
    }
}

void Recorder::showRaw() {
    for (int i = 0; i < RECORDING_BUFFER_SIZE; i += 32) {
        hexToAscii(tempBuffer, &recordingBuffer[i], 32);
        Serial.print((char*)tempBuffer);
    }
    Serial.println();
}

void Recorder::showBits() {
    byte hexBuffer[64];
    for (int i = 0; i < RECORDING_BUFFER_SIZE; i += 32) {
        hexToAscii(hexBuffer, &recordingBuffer[i], 32);
        for (int j = 0; j < 64; j++) {
            byte nibble = hexBuffer[j];
            if (nibble >= '0' && nibble <= '9') nibble -= '0';
            else if (nibble >= 'A' && nibble <= 'F') nibble -= 'A' - 10;
            else if (nibble >= 'a' && nibble <= 'f') nibble -= 'a' - 10;
            
            for (int k = 3; k >= 0; k--) {
                Serial.print((nibble & (1 << k)) ? "-" : "_");
            }
        }
        Serial.println();
    }
}

void Recorder::flush() {
    memset(recordingBuffer, 0, RECORDING_BUFFER_SIZE);
    bufferPos = 0;
    frameCount = 0;
    Serial.println("Buffer flushed");
}

void Recorder::saveToEEPROM(int offset, const byte* data, int len) {
    int maxLen = EEPROM_SIZE - offset;
    if (len > maxLen) len = maxLen;
    
    for (int i = 0; i < len; i++) {
        EEPROM.write(offset + i, data[i]);
    }
    EEPROM.commit();
}

void Recorder::loadFromEEPROM(int offset, byte* data, int len) {
    int maxLen = EEPROM_SIZE - offset;
    if (len > maxLen) len = maxLen;
    
    for (int i = 0; i < len; i++) {
        data[i] = EEPROM.read(offset + i);
    }
}

void Recorder::save() {
    int len = bufferPos;
    if (len > EEPROM_SIZE) len = EEPROM_SIZE;
    saveToEEPROM(0, recordingBuffer, len);
    Serial.printf("Saved %d bytes to EEPROM\n", len);
}

void Recorder::load() {
    flush();
    loadFromEEPROM(0, recordingBuffer, EEPROM_SIZE);
    bufferPos = EEPROM_SIZE;
    // Rebuild frame count
    int pos = 0;
    frameCount = 0;
    while (pos < EEPROM_SIZE) {
        int len = recordingBuffer[pos];
        if (len == 0 || len > CC_BUFFER_SIZE) break;
        pos += len + 1;
        frameCount++;
        if (pos >= EEPROM_SIZE) break;
    }
    bufferPos = pos;
    Serial.printf("Loaded %d frames from EEPROM\n", frameCount);
}

void Recorder::saveRaw() {
    int len = bufferPos;
    if (len > EEPROM_SIZE) len = EEPROM_SIZE;
    saveToEEPROM(0, recordingBuffer, len);
    Serial.printf("Saved %d raw bytes to EEPROM\n", len);
}

void Recorder::loadRaw() {
    flush();
    loadFromEEPROM(0, recordingBuffer, EEPROM_SIZE);
    bufferPos = EEPROM_SIZE;
    Serial.printf("Loaded %d raw bytes from EEPROM\n", bufferPos);
}

int Recorder::getFrameCount() const {
    return frameCount;
}

int Recorder::getBufferPos() const {
    return bufferPos;
}
