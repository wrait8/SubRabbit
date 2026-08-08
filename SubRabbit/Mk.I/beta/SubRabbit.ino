
// ================================================================
// Sub Rabbit Mk.I - ATmega32U4
// ================================================================

#include <Arduino.h>
#include <EEPROM.h>
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <SmartRC_CC1101.h>
#include <RCSwitch.h>
#include <avr/pgmspace.h>

// =========================== PINS ================================
#define SCK_PIN   15
#define MISO_PIN  14
#define MOSI_PIN  16
#define SS_PIN    10
#define GDO0_PIN  3
#define GDO2_PIN  9

// =========================== CONSTANTS ===========================
#define BUF_LENGTH 128
#define CC_BUFFER_SIZE 64
#define RECORDING_BUFFER_SIZE 1024   // shared buffer for frames & raw
#define EEPROM_SIZE 1024

// =========================== GLOBALS =============================
byte byteBuffer[64];
char textBuffer[128];   // now large enough for hex conversion (64*2+1)
byte bigBuffer[RECORDING_BUFFER_SIZE];   // the one and only buffer

// =========================== CLASS DECLARATIONS ==================

class PacketRF {
public:
    PacketRF();
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

class Recorder {
public:
    Recorder();
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
    bool recording;
    int bufferPos;
    int frameCount;
    byte tempBuffer[CC_BUFFER_SIZE];

    void ensureBufferSpace(int needed);
    void hexToAscii(byte* ascii, const byte* hex, int len);
    void saveToEEPROM(int offset, const byte* data, int len);
    void loadFromEEPROM(int offset, byte* data, int len);
};

class Jammer {
public:
    Jammer();
    void start();
    void stop();
    bool isActive() const;
    void update();
private:
    bool active;
    static const int JAM_BUFFER_SIZE = 60;
    byte jambuffer[JAM_BUFFER_SIZE];
};

class FrequencyAnalyzer {
public:
    FrequencyAnalyzer();
    void init();
    void scan();
    void setRssiThreshold(int threshold);
    int getRssi();
    int getLqi();
    void setFrequency(float freq);
    void setRxBandwidth(float bw);

private:
    static const uint32_t FREQUENCY_LIST[] PROGMEM;
    static const size_t FREQUENCY_LIST_SIZE;
    int rssi_threshold;

    struct FrequencyRSSI {
        uint32_t frequency_coarse;
        int rssi_coarse;
        uint32_t frequency_fine;
        int rssi_fine;
    };
    void coarseScan(FrequencyRSSI& result);
    void fineScan(FrequencyRSSI& result);
};

class RCSwitchHandler {
public:
    RCSwitchHandler();
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
    unsigned long lastCode;
    unsigned int lastBits;
    unsigned int lastDelay;
    unsigned int lastProtocol;
};

class CommandHandler {
public:
    CommandHandler();
    void init();
    void processLine(const char* line);
    void handleRx();
    void handleJam();
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
    bool echoEnabled;

    unsigned long lastSigCode;
    unsigned int lastSigBits;
    unsigned int lastSigDelay;
    unsigned int lastSigProtocol;

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
    void cmdStartRx();
    void cmdStopRx();
    void cmdTransmit(const char* args);
    void cmdStartJam();
    void cmdStopJam();
    void cmdBruteForce(const char* args);
    void cmdStartRec();
    void cmdStopRec();
    void cmdAddFrame(const char* args);
    void cmdShowFrames();
    void cmdFlush();
    void cmdPlay(const char* args);
    void cmdPlayAll();
    void cmdSave();
    void cmdLoad();
    void cmdRecSig();
    void cmdPlaySig();
    void cmdSaveSig();
    void cmdLoadSig();
    void cmdShowSig();
    // RAW commands
    void cmdRxRaw(const char* args);
    void cmdRecRaw(const char* args);
    void cmdAddRaw(const char* args);
    void cmdShowRaw();
    void cmdShowBits();
    void cmdPlayRaw(const char* args);
    void cmdSaveRaw();
    void cmdLoadRaw();
    void cmdEcho(const char* args);
    void cmdStop();
    void cmdReset();
    void cmdInit();
    void cmdStatus();

    void hexToAscii(byte* ascii, const byte* hex, int len);
    void asciiToHex(byte* hex, const byte* ascii, int len);
};

// =========================== CLASS IMPLEMENTATIONS ===============

// ---------- PacketRF ----------
PacketRF::PacketRF() : ccmode(true), gdo0pin(GDO0_PIN), gdo2pin(GDO2_PIN) {
    memset(rxBuffer, 0, MAX_BUFFER);
    memset(txBuffer, 0, MAX_BUFFER);
}

void PacketRF::init() {
    ELECHOUSE_cc1101.setGDO(gdo0pin, gdo2pin);
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.setGDO0(gdo0pin);
    setCCMode(true);
    setModulation(2);
    setFrequency(433.92);
    setDeviation(47.60);
    setChannel(0);
    setChannelSpacing(199.95);
    setRxBandwidth(812.50);
    setDataRate(9.6);
    setPower(10);
    setSyncMode(2);
    setSyncWord(211, 145);
    setAddressCheck(0);
    setAddress(0);
    setWhitening(false);
    setPacketFormat(0);
    setLengthConfig(1);
    setPacketLength(0);
    setCRC(false);
    setCRCAutoFlush(false);
    setDCFilter(true);
    setManchester(false);
    setFEC(false);
    setPreamble(0);
    setPQT(0);
    setAppendStatus(false);
}

void PacketRF::setCCMode(bool enable) { ccmode = enable; ELECHOUSE_cc1101.setCCMode(enable ? 1 : 0); }
void PacketRF::setModulation(int mode) { ELECHOUSE_cc1101.setModulation(mode); }
void PacketRF::setFrequency(float freq) { ELECHOUSE_cc1101.setMHZ(freq); }
void PacketRF::setDeviation(float dev) { ELECHOUSE_cc1101.setDeviation(dev); }
void PacketRF::setChannel(int channel) { ELECHOUSE_cc1101.setChannel(channel); }
void PacketRF::setChannelSpacing(float spacing) { ELECHOUSE_cc1101.setChsp(spacing); }
void PacketRF::setRxBandwidth(float bw) { ELECHOUSE_cc1101.setRxBW(bw); }
void PacketRF::setDataRate(float rate) { ELECHOUSE_cc1101.setDRate(rate); }
void PacketRF::setPower(int power) { ELECHOUSE_cc1101.setPA(power); }
void PacketRF::setSyncMode(int mode) { ELECHOUSE_cc1101.setSyncMode(mode); }
void PacketRF::setSyncWord(int high, int low) { ELECHOUSE_cc1101.setSyncWord(high, low); }
void PacketRF::setAddressCheck(int mode) { ELECHOUSE_cc1101.setAdrChk(mode); }
void PacketRF::setAddress(int addr) { ELECHOUSE_cc1101.setAddr(addr); }
void PacketRF::setWhitening(bool enable) { ELECHOUSE_cc1101.setWhiteData(enable ? 1 : 0); }
void PacketRF::setPacketFormat(int format) { ELECHOUSE_cc1101.setPktFormat(format); }
void PacketRF::setLengthConfig(int config) { ELECHOUSE_cc1101.setLengthConfig(config); }
void PacketRF::setPacketLength(int length) { ELECHOUSE_cc1101.setPacketLength(length); }
void PacketRF::setCRC(bool enable) { ELECHOUSE_cc1101.setCrc(enable ? 1 : 0); }
void PacketRF::setCRCAutoFlush(bool enable) { ELECHOUSE_cc1101.setCRC_AF(enable ? 1 : 0); }
void PacketRF::setDCFilter(bool enable) { ELECHOUSE_cc1101.setDcFilterOff(enable ? 0 : 1); }
void PacketRF::setManchester(bool enable) { ELECHOUSE_cc1101.setManchester(enable ? 1 : 0); }
void PacketRF::setFEC(bool enable) { ELECHOUSE_cc1101.setFEC(enable ? 1 : 0); }
void PacketRF::setPreamble(int mode) { ELECHOUSE_cc1101.setPRE(mode); }
void PacketRF::setPQT(int threshold) { ELECHOUSE_cc1101.setPQT(threshold); }
void PacketRF::setAppendStatus(bool enable) { ELECHOUSE_cc1101.setAppendStatus(enable ? 1 : 0); }

void PacketRF::send(const byte* data, int len) {
    if (len > MAX_BUFFER) len = MAX_BUFFER;
    memcpy(txBuffer, data, len);
    ELECHOUSE_cc1101.SendData(txBuffer, len);
}

bool PacketRF::receive(byte* buffer, int& len) {
    if (checkReceiveFlag() && checkCRC()) {
        len = ELECHOUSE_cc1101.ReceiveData(rxBuffer);
        memcpy(buffer, rxBuffer, len);
        return true;
    }
    return false;
}

void PacketRF::setRxMode() { ELECHOUSE_cc1101.SetRx(); }
void PacketRF::setTxMode() { ELECHOUSE_cc1101.SetTx(); }
bool PacketRF::checkReceiveFlag() { return ELECHOUSE_cc1101.CheckReceiveFlag(); }
bool PacketRF::checkCRC() { return ELECHOUSE_cc1101.CheckCRC(); }
int PacketRF::getRssi() { return ELECHOUSE_cc1101.getRssi(); }
int PacketRF::getLqi() { return ELECHOUSE_cc1101.getLqi(); }
void PacketRF::reset() { ELECHOUSE_cc1101.setSidle(); init(); }

// ---------- Recorder ----------
Recorder::Recorder() : recording(false), bufferPos(0), frameCount(0) {
    memset(bigBuffer, 0, RECORDING_BUFFER_SIZE);
    memset(tempBuffer, 0, CC_BUFFER_SIZE);
}

void Recorder::init() { flush(); }

void Recorder::startRecording() {
    recording = true;
    flush();
}

void Recorder::stopRecording() { recording = false; }

bool Recorder::isRecording() const { return recording; }

void Recorder::ensureBufferSpace(int needed) {
    if (bufferPos + needed >= RECORDING_BUFFER_SIZE) {
        recording = false;
    }
}

void Recorder::addFrame(const byte* data, int len) {
    if (!recording) return;
    if (len > CC_BUFFER_SIZE) len = CC_BUFFER_SIZE;
    int totalNeeded = len + 1;
    ensureBufferSpace(totalNeeded);
    if (!recording) return;
    bigBuffer[bufferPos++] = len;
    memcpy(&bigBuffer[bufferPos], data, len);
    bufferPos += len;
    frameCount++;
}

void Recorder::addRaw(const byte* data, int len) {
    if (bufferPos + len >= RECORDING_BUFFER_SIZE) {
        len = RECORDING_BUFFER_SIZE - bufferPos;
        if (len <= 0) return;
    }
    memcpy(&bigBuffer[bufferPos], data, len);
    bufferPos += len;
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

void Recorder::playFrame(int index) {
    if (index < 0 || index >= frameCount) return;
    int pos = 0;
    for (int i = 0; i < index; i++) {
        int len = bigBuffer[pos];
        pos += len + 1;
    }
    int len = bigBuffer[pos++];
    memcpy(tempBuffer, &bigBuffer[pos], len);
    ELECHOUSE_cc1101.SendData(tempBuffer, len);
}

void Recorder::playAll() {
    if (frameCount == 0) return;
    int pos = 0;
    for (int i = 0; i < frameCount; i++) {
        int len = bigBuffer[pos++];
        memcpy(tempBuffer, &bigBuffer[pos], len);
        ELECHOUSE_cc1101.SendData(tempBuffer, len);
        pos += len;
        delay(10);
    }
}

void Recorder::playRaw(int interval) {
    if (interval <= 0) return;
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    pinMode(GDO0_PIN, OUTPUT);

    for (int i = 0; i < RECORDING_BUFFER_SIZE; i++) {
        byte data = bigBuffer[i];
        for (int j = 7; j >= 0; j--) {
            digitalWrite(GDO0_PIN, bitRead(data, j));
            delayMicroseconds(interval);
        }
    }

    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setPktFormat(0);
    ELECHOUSE_cc1101.SetTx();
}

void Recorder::showFrames() {
    if (frameCount == 0) {
        Serial.println(F("No frames"));
        return;
    }
    int pos = 0;
    for (int i = 0; i < frameCount; i++) {
        int len = bigBuffer[pos++];
        hexToAscii(tempBuffer, &bigBuffer[pos], len);
        Serial.print(i+1);
        Serial.print(F(":"));
        Serial.println((char*)tempBuffer);
        pos += len;
    }
}

void Recorder::showRaw() {
    for (int i = 0; i < RECORDING_BUFFER_SIZE; i += 32) {
        hexToAscii(tempBuffer, &bigBuffer[i], 32);
        Serial.print((char*)tempBuffer);
    }
    Serial.println();
}

void Recorder::showBits() {
    byte hexBuffer[64];
    for (int i = 0; i < RECORDING_BUFFER_SIZE; i += 32) {
        hexToAscii(hexBuffer, &bigBuffer[i], 32);
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
    memset(bigBuffer, 0, RECORDING_BUFFER_SIZE);
    bufferPos = 0;
    frameCount = 0;
}

void Recorder::saveToEEPROM(int offset, const byte* data, int len) {
    int maxLen = EEPROM_SIZE - offset;
    if (len > maxLen) len = maxLen;
    for (int i = 0; i < len; i++) {
        EEPROM.write(offset + i, data[i]);
    }
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
    saveToEEPROM(0, bigBuffer, len);
}

void Recorder::load() {
    flush();
    loadFromEEPROM(0, bigBuffer, EEPROM_SIZE);
    bufferPos = EEPROM_SIZE;
    int pos = 0;
    frameCount = 0;
    while (pos < EEPROM_SIZE) {
        int len = bigBuffer[pos];
        if (len == 0 || len > CC_BUFFER_SIZE) break;
        pos += len + 1;
        frameCount++;
        if (pos >= EEPROM_SIZE) break;
    }
    bufferPos = pos;
}

void Recorder::saveRaw() {
    int len = RECORDING_BUFFER_SIZE;
    if (len > EEPROM_SIZE) len = EEPROM_SIZE;
    saveToEEPROM(0, bigBuffer, len);
}

void Recorder::loadRaw() {
    flush();
    loadFromEEPROM(0, bigBuffer, EEPROM_SIZE);
    bufferPos = EEPROM_SIZE;
}

int Recorder::getFrameCount() const { return frameCount; }
int Recorder::getBufferPos() const { return bufferPos; }

// ---------- Jammer ----------
Jammer::Jammer() : active(false) {}

void Jammer::start() { active = true; }
void Jammer::stop() { active = false; }
bool Jammer::isActive() const { return active; }

void Jammer::update() {
    if (!active) return;
    randomSeed(analogRead(0));
    for (int i = 0; i < JAM_BUFFER_SIZE; i++)
        jambuffer[i] = (byte)random(255);
    ELECHOUSE_cc1101.SendData(jambuffer, JAM_BUFFER_SIZE);
}

// ---------- FrequencyAnalyzer ----------
const uint32_t FrequencyAnalyzer::FREQUENCY_LIST[] PROGMEM = {
    300000000, 302757000, 303875000, 303900000, 304250000,
    307000000, 307500000, 307800000, 309000000, 310000000,
    312000000, 312100000, 312200000, 313000000, 313850000,
    314000000, 314350000, 314980000, 315000000, 318000000,
    330000000, 345000000, 348000000, 350000000,
    387000000, 390000000, 418000000, 430000000, 430500000,
    431000000, 431500000, 433075000, 433220000, 433420000,
    433657070, 433889000, 433920000, 434075000, 434176948,
    434190000, 434390000, 434420000, 434620000, 434775000,
    438900000, 440175000, 464000000, 467750000,
    779000000, 868350000, 868400000, 868800000, 868950000,
    906400000, 915000000, 925000000, 928000000
};
const size_t FrequencyAnalyzer::FREQUENCY_LIST_SIZE = sizeof(FREQUENCY_LIST) / sizeof(FREQUENCY_LIST[0]);

FrequencyAnalyzer::FrequencyAnalyzer() : rssi_threshold(-65) {}
void FrequencyAnalyzer::init() {}
void FrequencyAnalyzer::setRssiThreshold(int threshold) { rssi_threshold = threshold; }
int FrequencyAnalyzer::getRssi() { return ELECHOUSE_cc1101.getRssi(); }
int FrequencyAnalyzer::getLqi() { return ELECHOUSE_cc1101.getLqi(); }
void FrequencyAnalyzer::setFrequency(float freq) { ELECHOUSE_cc1101.setMHZ(freq); }
void FrequencyAnalyzer::setRxBandwidth(float bw) { ELECHOUSE_cc1101.setRxBW(bw); }

void FrequencyAnalyzer::coarseScan(FrequencyRSSI& result) {
    ELECHOUSE_cc1101.setRxBW(650);
    for (size_t i = 0; i < FREQUENCY_LIST_SIZE; i++) {
        uint32_t frequency = pgm_read_dword(&FREQUENCY_LIST[i]);
        if (frequency != 467750000 && frequency != 464000000 &&
            frequency != 390000000 && frequency != 312000000 &&
            frequency != 312100000 && frequency != 312200000 &&
            frequency != 440175000) {
            ELECHOUSE_cc1101.setMHZ((float)frequency / 1000000.0);
            ELECHOUSE_cc1101.setSidle();
            ELECHOUSE_cc1101.SetRx();
            delay(2);
            int rssi = ELECHOUSE_cc1101.getRssi();
            if (result.rssi_coarse < rssi) {
                result.rssi_coarse = rssi;
                result.frequency_coarse = frequency;
            }
        }
    }
}

void FrequencyAnalyzer::fineScan(FrequencyRSSI& result) {
    if (result.rssi_coarse > rssi_threshold) {
        ELECHOUSE_cc1101.setRxBW(58);
        for (uint32_t i = result.frequency_coarse - 300000;
             i < result.frequency_coarse + 300000; i += 20000) {
            ELECHOUSE_cc1101.setMHZ((float)i / 1000000.0);
            ELECHOUSE_cc1101.setSidle();
            ELECHOUSE_cc1101.SetRx();
            delay(2);
            int rssi = ELECHOUSE_cc1101.getRssi();
            if (result.rssi_fine < rssi) {
                result.rssi_fine = rssi;
                result.frequency_fine = i;
            }
        }
    }
}

void FrequencyAnalyzer::scan() {
    while (!Serial.available()) {
        FrequencyRSSI result = {0, -100, 0, -100};
        coarseScan(result);
        fineScan(result);
        if (result.rssi_fine > rssi_threshold) {
            Serial.print(F("Fine: "));
            Serial.print(result.frequency_fine / 1000000.0, 6);
            Serial.print(F("MHz | RSSI: "));
            Serial.println(result.rssi_fine);
        } else if (result.rssi_coarse > rssi_threshold) {
            Serial.print(F("Coarse: "));
            Serial.print(result.frequency_coarse / 1000000.0, 6);
            Serial.print(F("MHz | RSSI: "));
            Serial.println(result.rssi_coarse);
        }
        delay(10);
    }
}

// ---------- RCSwitchHandler ----------
RCSwitchHandler::RCSwitchHandler() : initialized(false), gdoPin(2), lastCode(0), lastBits(0), lastDelay(0), lastProtocol(0) {}

void RCSwitchHandler::init(int gdoPin) {
    this->gdoPin = gdoPin;
    initialized = true;
}

void RCSwitchHandler::enableReceive() {
    if (!initialized) return;
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
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    mySwitch.enableTransmit(gdoPin);
}

void RCSwitchHandler::disable() { mySwitch.disableReceive(); mySwitch.disableTransmit(); }
bool RCSwitchHandler::available() { return initialized && mySwitch.available(); }
unsigned long RCSwitchHandler::getValue() { lastCode = mySwitch.getReceivedValue(); return lastCode; }
unsigned int RCSwitchHandler::getBits() { lastBits = mySwitch.getReceivedBitlength(); return lastBits; }
unsigned int RCSwitchHandler::getDelay() { lastDelay = mySwitch.getReceivedDelay(); return lastDelay; }
unsigned int RCSwitchHandler::getProtocol() { lastProtocol = mySwitch.getReceivedProtocol(); return lastProtocol; }
void RCSwitchHandler::resetAvailable() { mySwitch.resetAvailable(); }

void RCSwitchHandler::send(unsigned long code, unsigned int bits) {
    if (!initialized) return;
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    mySwitch.enableTransmit(gdoPin);
    delay(200);
    mySwitch.send(code, bits);
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setPktFormat(0);
    ELECHOUSE_cc1101.SetTx();
}

void RCSwitchHandler::send(unsigned long code, unsigned int bits, unsigned int delay) {
    mySwitch.setPulseLength(delay);
    send(code, bits);
}
void RCSwitchHandler::send(unsigned long code, unsigned int bits, unsigned int delay, unsigned int protocol) {
    mySwitch.setProtocol(protocol);
    mySwitch.setPulseLength(delay);
    send(code, bits);
}
void RCSwitchHandler::setProtocol(unsigned int protocol) { mySwitch.setProtocol(protocol); }
void RCSwitchHandler::setPulseLength(unsigned int delay) { mySwitch.setPulseLength(delay); }
void RCSwitchHandler::setRepeatTransmit(int repeats) { mySwitch.setRepeatTransmit(repeats); }

int RCSwitchHandler::getReceivedRawData(unsigned int* raw) {
    unsigned int* data = mySwitch.getReceivedRawdata();
    for (int i = 0; i < 128; i++) {
        raw[i] = data[i];
        if (data[i] == 0) return i;
    }
    return 128;
}

const char* RCSwitchHandler::bin2tristate(const char* bin) {
    static char returnValue[50];
    int pos = 0, pos2 = 0;
    while (bin[pos] != '\0' && bin[pos+1] != '\0') {
        if (bin[pos] == '0' && bin[pos+1] == '0') returnValue[pos2] = '0';
        else if (bin[pos] == '1' && bin[pos+1] == '1') returnValue[pos2] = '1';
        else if (bin[pos] == '0' && bin[pos+1] == '1') returnValue[pos2] = 'F';
        else return "not applicable";
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
        Dec >>= 1;
    }
    for (unsigned int j = 0; j < bitLength; j++) {
        if (j >= bitLength - i) bin[j] = bin[31 + i - (j - (bitLength - i))];
        else bin[j] = '0';
    }
    bin[bitLength] = '\0';
    return bin;
}

// ---------- CommandHandler ----------
CommandHandler::CommandHandler()
    : receivingMode(false), jammingMode(false), recordingMode(false), echoEnabled(true) {}

void CommandHandler::init() {
    rf.init();
    recorder.init();
    analyzer.init();
    rcSwitch.init(GDO0_PIN);
    if (ELECHOUSE_cc1101.getCC1101()) Serial.println(F("CC1101 OK"));
    else Serial.println(F("CC1101 err"));
}

void CommandHandler::hexToAscii(byte* ascii, const byte* hex, int len) {
    for (int i = 0; i < len; i++) {
        byte high = hex[i] >> 4;
        byte low = hex[i] & 0x0F;
        ascii[2*i] = high > 9 ? (high - 10 + 'A') : (high + '0');
        ascii[2*i+1] = low > 9 ? (low - 10 + 'A') : (low + '0');
    }
    ascii[2*len] = '\0';
}

void CommandHandler::asciiToHex(byte* hex, const byte* ascii, int len) {
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

void CommandHandler::printHelp() {
    Serial.println(F(
        "Commands:\n"
        "mod,freq,dev,chan,chsp,bw,rate,power\n"
        "syncmode,sync,adrchk,addr,whitening,pktfmt,lenconf\n"
        "pktlen,crc,crcaf,dcfilter,manchester,fec,pre,pqt,append\n"
        "rx,tx,jam,brute,rec,add,show,flush,play,save,load\n"
        "rxraw,recraw,addraw,showraw,showbit,playraw,saveraw,loadraw\n"
        "recsig,playsig,savesig,loadsig,showsig\n"
        "rssi,analyze,echo,stop,reset,init,status,help\n"
    ));
}

void CommandHandler::processLine(const char* line) {
    char cmd[16];
    char args[32];
    cmd[0] = '\0';
    args[0] = '\0';

    const char* space = strchr(line, ' ');
    if (space) {
        int cmdLen = space - line;
        if (cmdLen > 15) cmdLen = 15;
        strncpy(cmd, line, cmdLen);
        cmd[cmdLen] = '\0';
        strcpy(args, space + 1);
    } else {
        strcpy(cmd, line);
    }

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) cmdHelp();
    else if (strcmp(cmd, "mod") == 0) cmdSetModulation(args);
    else if (strcmp(cmd, "freq") == 0) cmdSetFrequency(args);
    else if (strcmp(cmd, "dev") == 0) cmdSetDeviation(args);
    else if (strcmp(cmd, "chan") == 0) cmdSetChannel(args);
    else if (strcmp(cmd, "chsp") == 0) cmdSetChannelSpacing(args);
    else if (strcmp(cmd, "bw") == 0) cmdSetRxBandwidth(args);
    else if (strcmp(cmd, "rate") == 0) cmdSetDataRate(args);
    else if (strcmp(cmd, "power") == 0) cmdSetPower(args);
    else if (strcmp(cmd, "syncmode") == 0) cmdSetSyncMode(args);
    else if (strcmp(cmd, "sync") == 0) cmdSetSyncWord(args);
    else if (strcmp(cmd, "adrchk") == 0) cmdSetAddressCheck(args);
    else if (strcmp(cmd, "addr") == 0) cmdSetAddress(args);
    else if (strcmp(cmd, "whitening") == 0) cmdSetWhitening(args);
    else if (strcmp(cmd, "pktfmt") == 0) cmdSetPacketFormat(args);
    else if (strcmp(cmd, "lenconf") == 0) cmdSetLengthConfig(args);
    else if (strcmp(cmd, "pktlen") == 0) cmdSetPacketLength(args);
    else if (strcmp(cmd, "crc") == 0) cmdSetCRC(args);
    else if (strcmp(cmd, "crcaf") == 0) cmdSetCRCAutoFlush(args);
    else if (strcmp(cmd, "dcfilter") == 0) cmdSetDCFilter(args);
    else if (strcmp(cmd, "manchester") == 0) cmdSetManchester(args);
    else if (strcmp(cmd, "fec") == 0) cmdSetFEC(args);
    else if (strcmp(cmd, "pre") == 0) cmdSetPreamble(args);
    else if (strcmp(cmd, "pqt") == 0) cmdSetPQT(args);
    else if (strcmp(cmd, "append") == 0) cmdSetAppendStatus(args);
    else if (strcmp(cmd, "rssi") == 0) cmdGetRSSI();
    else if (strcmp(cmd, "analyze") == 0) cmdAnalyze();
    else if (strcmp(cmd, "rx") == 0) {
        if (receivingMode) cmdStopRx(); else cmdStartRx();
    }
    else if (strcmp(cmd, "tx") == 0) cmdTransmit(args);
    else if (strcmp(cmd, "jam") == 0) {
        if (jammingMode) cmdStopJam(); else cmdStartJam();
    }
    else if (strcmp(cmd, "brute") == 0) cmdBruteForce(args);
    else if (strcmp(cmd, "rec") == 0) {
        if (recordingMode) cmdStopRec(); else cmdStartRec();
    }
    else if (strcmp(cmd, "add") == 0) cmdAddFrame(args);
    else if (strcmp(cmd, "show") == 0) cmdShowFrames();
    else if (strcmp(cmd, "flush") == 0) cmdFlush();
    else if (strcmp(cmd, "play") == 0) {
        if (strlen(args) == 0) cmdPlayAll(); else cmdPlay(args);
    }
    else if (strcmp(cmd, "save") == 0) cmdSave();
    else if (strcmp(cmd, "load") == 0) cmdLoad();
    else if (strcmp(cmd, "rxraw") == 0) cmdRxRaw(args);
    else if (strcmp(cmd, "recraw") == 0) cmdRecRaw(args);
    else if (strcmp(cmd, "addraw") == 0) cmdAddRaw(args);
    else if (strcmp(cmd, "showraw") == 0) cmdShowRaw();
    else if (strcmp(cmd, "showbit") == 0) cmdShowBits();
    else if (strcmp(cmd, "playraw") == 0) cmdPlayRaw(args);
    else if (strcmp(cmd, "saveraw") == 0) cmdSaveRaw();
    else if (strcmp(cmd, "loadraw") == 0) cmdLoadRaw();
    else if (strcmp(cmd, "recsig") == 0) cmdRecSig();
    else if (strcmp(cmd, "playsig") == 0) cmdPlaySig();
    else if (strcmp(cmd, "savesig") == 0) cmdSaveSig();
    else if (strcmp(cmd, "loadsig") == 0) cmdLoadSig();
    else if (strcmp(cmd, "showsig") == 0) cmdShowSig();
    else if (strcmp(cmd, "echo") == 0) cmdEcho(args);
    else if (strcmp(cmd, "stop") == 0) cmdStop();
    else if (strcmp(cmd, "reset") == 0) cmdReset();
    else if (strcmp(cmd, "init") == 0) cmdInit();
    else if (strcmp(cmd, "status") == 0) cmdStatus();
    else {
        Serial.print(F("? "));
        Serial.println(cmd);
    }
}

// ---- Command implementations (prints removed or minimised) ----
void CommandHandler::cmdHelp() { printHelp(); }

void CommandHandler::cmdSetModulation(const char* args) { rf.setModulation(atoi(args)); }
void CommandHandler::cmdSetFrequency(const char* args) { rf.setFrequency(atof(args)); }
void CommandHandler::cmdSetDeviation(const char* args) { rf.setDeviation(atof(args)); }
void CommandHandler::cmdSetChannel(const char* args) { rf.setChannel(atoi(args)); }
void CommandHandler::cmdSetChannelSpacing(const char* args) { rf.setChannelSpacing(atof(args)); }
void CommandHandler::cmdSetRxBandwidth(const char* args) { rf.setRxBandwidth(atof(args)); }
void CommandHandler::cmdSetDataRate(const char* args) { rf.setDataRate(atof(args)); }
void CommandHandler::cmdSetPower(const char* args) { rf.setPower(atoi(args)); }
void CommandHandler::cmdSetSyncMode(const char* args) { rf.setSyncMode(atoi(args)); }
void CommandHandler::cmdSetSyncWord(const char* args) { int h,l; sscanf(args,"%d,%d",&h,&l); rf.setSyncWord(h,l); }
void CommandHandler::cmdSetAddressCheck(const char* args) { rf.setAddressCheck(atoi(args)); }
void CommandHandler::cmdSetAddress(const char* args) { rf.setAddress(atoi(args)); }
void CommandHandler::cmdSetWhitening(const char* args) { rf.setWhitening(atoi(args)==1); }
void CommandHandler::cmdSetPacketFormat(const char* args) { rf.setPacketFormat(atoi(args)); }
void CommandHandler::cmdSetLengthConfig(const char* args) { rf.setLengthConfig(atoi(args)); }
void CommandHandler::cmdSetPacketLength(const char* args) { rf.setPacketLength(atoi(args)); }
void CommandHandler::cmdSetCRC(const char* args) { rf.setCRC(atoi(args)==1); }
void CommandHandler::cmdSetCRCAutoFlush(const char* args) { rf.setCRCAutoFlush(atoi(args)==1); }
void CommandHandler::cmdSetDCFilter(const char* args) { rf.setDCFilter(atoi(args)==1); }
void CommandHandler::cmdSetManchester(const char* args) { rf.setManchester(atoi(args)==1); }
void CommandHandler::cmdSetFEC(const char* args) { rf.setFEC(atoi(args)==1); }
void CommandHandler::cmdSetPreamble(const char* args) { rf.setPreamble(atoi(args)); }
void CommandHandler::cmdSetPQT(const char* args) { rf.setPQT(atoi(args)); }
void CommandHandler::cmdSetAppendStatus(const char* args) { rf.setAppendStatus(atoi(args)==1); }

void CommandHandler::cmdGetRSSI() {
    Serial.print(rf.getRssi()); Serial.print(F("dB ")); Serial.println(rf.getLqi());
}

void CommandHandler::cmdAnalyze() { analyzer.scan(); }

void CommandHandler::cmdStartRx() {
    receivingMode = true;
    jammingMode = recordingMode = false;
    rf.setRxMode();
}
void CommandHandler::cmdStopRx() { receivingMode = false; }

void CommandHandler::cmdTransmit(const char* args) {
    int len = strlen(args);
    if (len > 0 && len <= 120) {
        asciiToHex(byteBuffer, (const byte*)args, len);
        rf.send(byteBuffer, len/2);
    }
}

void CommandHandler::cmdStartJam() {
    jammingMode = true;
    receivingMode = recordingMode = false;
    jammer.start();
}
void CommandHandler::cmdStopJam() { jammingMode = false; jammer.stop(); }

void CommandHandler::cmdBruteForce(const char* args) {
    int us = atoi(args);
    const char* bitsStr = strchr(args, ' ');
    int bits = bitsStr ? atoi(bitsStr+1) : 0;
    if (us <= 0 || bits <= 0) {
        Serial.println(F("use: brute <us> <bits>"));
        return;
    }
    unsigned long maxCode = (1UL << bits) - 1;
    rf.setCCMode(false);
    rf.setPacketFormat(3);
    rf.setTxMode();
    pinMode(GDO0_PIN, OUTPUT);
    for (unsigned long code = 0; code <= maxCode; code++) {
        for (int bit = bits-1; bit >= 0; bit--) {
            digitalWrite(GDO0_PIN, (code & (1UL<<bit)) ? HIGH : LOW);
            delayMicroseconds(us);
        }
        delay(50);
        if (Serial.available()) { break; }
    }
    rf.setCCMode(true);
    rf.setPacketFormat(0);
    rf.setTxMode();
}

void CommandHandler::cmdStartRec() {
    recordingMode = true;
    receivingMode = jammingMode = false;
    recorder.startRecording();
    rf.setRxMode();
}
void CommandHandler::cmdStopRec() {
    recordingMode = false;
    recorder.stopRecording();
}

void CommandHandler::cmdAddFrame(const char* args) {
    int len = strlen(args);
    if (len > 0 && len <= 120) {
        asciiToHex(byteBuffer, (const byte*)args, len);
        recorder.addFrame(byteBuffer, len/2);
    }
}
void CommandHandler::cmdShowFrames() { recorder.showFrames(); }
void CommandHandler::cmdFlush() { recorder.flush(); }
void CommandHandler::cmdPlay(const char* args) { recorder.playFrame(atoi(args)-1); }
void CommandHandler::cmdPlayAll() { recorder.playAll(); }
void CommandHandler::cmdSave() { recorder.save(); }
void CommandHandler::cmdLoad() { recorder.load(); }

// ---------- RAW commands ----------
void CommandHandler::cmdRxRaw(const char* args) {
    int interval = atoi(args);
    if (interval <= 0) return;
    rf.setCCMode(false);
    rf.setPacketFormat(3);
    rf.setRxMode();
    pinMode(GDO0_PIN, INPUT);
    while (!Serial.available()) {
        for (int i = 0; i < RECORDING_BUFFER_SIZE; i++) {
            byte data = 0;
            for (int j = 7; j >= 0; j--) {
                bitWrite(data, j, digitalRead(GDO0_PIN));
                delayMicroseconds(interval);
            }
            bigBuffer[i] = data;
        }
        for (int i = 0; i < RECORDING_BUFFER_SIZE; i += 32) {
            hexToAscii(textBuffer, &bigBuffer[i], 32);
            Serial.print((char*)textBuffer);
        }
    }
    rf.setCCMode(true);
    rf.setPacketFormat(0);
    rf.setRxMode();
}

void CommandHandler::cmdRecRaw(const char* args) {
    int interval = atoi(args);
    if (interval <= 0) return;
    rf.setCCMode(false);
    rf.setPacketFormat(3);
    rf.setRxMode();
    pinMode(GDO0_PIN, INPUT);
    while (digitalRead(GDO0_PIN) == LOW) {
        if (Serial.available()) return;
    }
    int i;
    for (i = 0; i < RECORDING_BUFFER_SIZE; i++) {
        byte data = 0;
        for (int j = 7; j >= 0; j--) {
            bitWrite(data, j, digitalRead(GDO0_PIN));
            delayMicroseconds(interval);
        }
        bigBuffer[i] = data;
        if (i % 10 == 0 && Serial.available()) break;
    }
    recorder.addRaw(bigBuffer, i);
    rf.setCCMode(true);
    rf.setPacketFormat(0);
    rf.setRxMode();
}

void CommandHandler::cmdAddRaw(const char* args) {
    int len = strlen(args);
    if (len > 0 && len <= 120) {
        asciiToHex(byteBuffer, (const byte*)args, len);
        recorder.addRaw(byteBuffer, len/2);
    }
}
void CommandHandler::cmdShowRaw() { recorder.showRaw(); }
void CommandHandler::cmdShowBits() { recorder.showBits(); }
void CommandHandler::cmdPlayRaw(const char* args) {
    int interval = atoi(args);
    if (interval > 0) recorder.playRaw(interval);
}
void CommandHandler::cmdSaveRaw() { recorder.saveRaw(); }
void CommandHandler::cmdLoadRaw() { recorder.loadRaw(); }

void CommandHandler::cmdRecSig() {
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.SetRx();
    rcSwitch.enableReceive();
    while (!Serial.available()) {
        if (rcSwitch.available()) {
            lastSigCode = rcSwitch.getValue();
            lastSigBits = rcSwitch.getBits();
            lastSigDelay = rcSwitch.getDelay();
            lastSigProtocol = rcSwitch.getProtocol();
            const char* b = RCSwitchHandler::dec2binWzerofill(lastSigCode, lastSigBits);
            Serial.print(lastSigCode); Serial.print(F(" ")); Serial.print(lastSigBits);
            Serial.print(F(" ")); Serial.print(lastSigDelay); Serial.print(F(" ")); Serial.println(lastSigProtocol);
            Serial.println(b);
            rcSwitch.resetAvailable();
            break;
        }
    }
    rcSwitch.disable();
}

void CommandHandler::cmdSaveSig() {
    if (lastSigCode == 0 && lastSigBits == 0) return;
    EEPROM.write(0, (byte)(lastSigCode >> 24));
    EEPROM.write(1, (byte)(lastSigCode >> 16));
    EEPROM.write(2, (byte)(lastSigCode >> 8));
    EEPROM.write(3, (byte)lastSigCode);
    EEPROM.write(4, (byte)(lastSigBits >> 8));
    EEPROM.write(5, (byte)lastSigBits);
    EEPROM.write(6, (byte)(lastSigDelay >> 8));
    EEPROM.write(7, (byte)lastSigDelay);
    EEPROM.write(8, (byte)(lastSigProtocol >> 8));
    EEPROM.write(9, (byte)lastSigProtocol);
}

void CommandHandler::cmdLoadSig() {
    unsigned long code = ((unsigned long)EEPROM.read(0) << 24) |
                         ((unsigned long)EEPROM.read(1) << 16) |
                         ((unsigned long)EEPROM.read(2) << 8) |
                         EEPROM.read(3);
    unsigned int bits = ((unsigned int)EEPROM.read(4) << 8) | EEPROM.read(5);
    unsigned int delay = ((unsigned int)EEPROM.read(6) << 8) | EEPROM.read(7);
    unsigned int protocol = ((unsigned int)EEPROM.read(8) << 8) | EEPROM.read(9);
    if (code == 0 && bits == 0) return;
    lastSigCode = code;
    lastSigBits = bits;
    lastSigDelay = delay;
    lastSigProtocol = protocol;
}

void CommandHandler::cmdPlaySig() {
    if (lastSigCode == 0 && lastSigBits == 0) return;
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    rcSwitch.enableTransmit();
    delay(200);
    rcSwitch.setProtocol(lastSigProtocol);
    rcSwitch.setPulseLength(lastSigDelay);
    rcSwitch.setRepeatTransmit(5);
    rcSwitch.send(lastSigCode, lastSigBits);
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setPktFormat(0);
    ELECHOUSE_cc1101.SetTx();
}

void CommandHandler::cmdShowSig() {
    if (lastSigCode == 0 && lastSigBits == 0) {
        Serial.println(F("none"));
        return;
    }
    Serial.print(lastSigCode); Serial.print(F(" ")); Serial.print(lastSigBits);
    Serial.print(F(" ")); Serial.print(lastSigDelay); Serial.print(F(" ")); Serial.println(lastSigProtocol);
}

void CommandHandler::cmdEcho(const char* args) { echoEnabled = (atoi(args) == 1); }
void CommandHandler::cmdStop() {
    receivingMode = jammingMode = recordingMode = false;
    jammer.stop();
}
void CommandHandler::cmdReset() { rf.reset(); }
void CommandHandler::cmdInit() { rf.init(); }

void CommandHandler::cmdStatus() {
    Serial.print(receivingMode?"Rx ":"");
    Serial.print(jammingMode?"Jam ":"");
    Serial.print(recordingMode?"Rec ":"");
    Serial.print(F("F:")); Serial.print(recorder.getFrameCount());
    Serial.print(F(" B:")); Serial.println(recorder.getBufferPos());
}

void CommandHandler::handleRx() {
    if (!receivingMode && !recordingMode) return;
    if (rf.checkReceiveFlag()) {
        byte buffer[64];
        int len;
        if (rf.receive(buffer, len)) {
            if (receivingMode) {
                hexToAscii(textBuffer, buffer, len);
                Serial.print((char*)textBuffer);
            }
            if (recordingMode) recorder.addFrame(buffer, len);
            rf.setRxMode();
        }
    }
}

void CommandHandler::handleJam() {
    if (jammingMode) jammer.update();
}

// =========================== MAIN =================================
CommandHandler cli;

void setup() {
    Serial.begin(115200);
    Serial.println(F("\nSub Rabbit Mk.I"));

    ELECHOUSE_cc1101.setSpiPin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    ELECHOUSE_cc1101.setGDO(GDO0_PIN, GDO2_PIN);

    cli.init();
    Serial.println(F("Type help"));
    Serial.print(F("> "));
}

void loop() {
    static char buffer[BUF_LENGTH];
    static int length = 0;

    while (Serial.available()) {
        int data = Serial.read();
        if (data == '\b' || data == '\177') {
            if (length > 0) length--;
        }
        else if (data == '\r' || data == '\n') {
            buffer[length] = '\0';
            if (length == 0) {
                Serial.print(F("> "));
            } else {
                Serial.write('\n');
                cli.processLine(buffer);
                Serial.print(F("> "));
            }
            length = 0;
        }
        else if (length < BUF_LENGTH - 1) {
            buffer[length++] = data;
        }
    }

    cli.handleRx();
    cli.handleJam();
}
