// packetrf.cpp
#include "packetrf.h"

PacketRF::PacketRF() : ccmode(true), gdo0pin(2), gdo2pin(4) {
    memset(rxBuffer, 0, MAX_BUFFER);
    memset(txBuffer, 0, MAX_BUFFER);
}

PacketRF::~PacketRF() {}

void PacketRF::setGDO0(int pin) {
    gdo0pin = pin;
}

void PacketRF::setGDO2(int pin) {
    gdo2pin = pin;
}

void PacketRF::setSpiPins(byte sck, byte miso, byte mosi, byte ss) {
    ELECHOUSE_cc1101.setSpiPin(sck, miso, mosi, ss);
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

void PacketRF::setCCMode(bool enable) {
    ccmode = enable;
    ELECHOUSE_cc1101.setCCMode(enable ? 1 : 0);
}

void PacketRF::setModulation(int mode) {
    ELECHOUSE_cc1101.setModulation(mode);
}

void PacketRF::setFrequency(float freq) {
    ELECHOUSE_cc1101.setMHZ(freq);
}

void PacketRF::setDeviation(float dev) {
    ELECHOUSE_cc1101.setDeviation(dev);
}

void PacketRF::setChannel(int channel) {
    ELECHOUSE_cc1101.setChannel(channel);
}

void PacketRF::setChannelSpacing(float spacing) {
    ELECHOUSE_cc1101.setChsp(spacing);
}

void PacketRF::setRxBandwidth(float bw) {
    ELECHOUSE_cc1101.setRxBW(bw);
}

void PacketRF::setDataRate(float rate) {
    ELECHOUSE_cc1101.setDRate(rate);
}

void PacketRF::setPower(int power) {
    ELECHOUSE_cc1101.setPA(power);
}

void PacketRF::setSyncMode(int mode) {
    ELECHOUSE_cc1101.setSyncMode(mode);
}

void PacketRF::setSyncWord(int high, int low) {
    ELECHOUSE_cc1101.setSyncWord(high, low);
}

void PacketRF::setAddressCheck(int mode) {
    ELECHOUSE_cc1101.setAdrChk(mode);
}

void PacketRF::setAddress(int addr) {
    ELECHOUSE_cc1101.setAddr(addr);
}

void PacketRF::setWhitening(bool enable) {
    ELECHOUSE_cc1101.setWhiteData(enable ? 1 : 0);
}

void PacketRF::setPacketFormat(int format) {
    ELECHOUSE_cc1101.setPktFormat(format);
}

void PacketRF::setLengthConfig(int config) {
    ELECHOUSE_cc1101.setLengthConfig(config);
}

void PacketRF::setPacketLength(int length) {
    ELECHOUSE_cc1101.setPacketLength(length);
}

void PacketRF::setCRC(bool enable) {
    ELECHOUSE_cc1101.setCrc(enable ? 1 : 0);
}

void PacketRF::setCRCAutoFlush(bool enable) {
    ELECHOUSE_cc1101.setCRC_AF(enable ? 1 : 0);
}

void PacketRF::setDCFilter(bool enable) {
    ELECHOUSE_cc1101.setDcFilterOff(enable ? 0 : 1);
}

void PacketRF::setManchester(bool enable) {
    ELECHOUSE_cc1101.setManchester(enable ? 1 : 0);
}

void PacketRF::setFEC(bool enable) {
    ELECHOUSE_cc1101.setFEC(enable ? 1 : 0);
}

void PacketRF::setPreamble(int mode) {
    ELECHOUSE_cc1101.setPRE(mode);
}

void PacketRF::setPQT(int threshold) {
    ELECHOUSE_cc1101.setPQT(threshold);
}

void PacketRF::setAppendStatus(bool enable) {
    ELECHOUSE_cc1101.setAppendStatus(enable ? 1 : 0);
}

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

void PacketRF::setRxMode() {
    ELECHOUSE_cc1101.SetRx();
}

void PacketRF::setTxMode() {
    ELECHOUSE_cc1101.SetTx();
}

bool PacketRF::checkReceiveFlag() {
    return ELECHOUSE_cc1101.CheckReceiveFlag();
}

bool PacketRF::checkCRC() {
    return ELECHOUSE_cc1101.CheckCRC();
}

int PacketRF::getRssi() {
    return ELECHOUSE_cc1101.getRssi();
}

int PacketRF::getLqi() {
    return ELECHOUSE_cc1101.getLqi();
}

void PacketRF::reset() {
    ELECHOUSE_cc1101.setSidle();
    init();
}
