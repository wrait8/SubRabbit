// cli.cpp
#include "cli.h"
#include <EEPROM.h>

CommandHandler::CommandHandler() 
    : receivingMode(false), jammingMode(false), recordingMode(false), 
      chatMode(false), echoEnabled(true) {
    memset(textBuffer, 0, BUFFER_SIZE);
    memset(byteBuffer, 0, 64);
}

CommandHandler::~CommandHandler() {}

void CommandHandler::init() {
    rf.init();
    recorder.init();
    analyzer.init();
    rcSwitch.init(2);
    
    if (ELECHOUSE_cc1101.getCC1101()) {
        Serial.println("CC1101 initialized. Connection OK");
    } else {
        Serial.println("CC1101 connection error! Check wiring.");
    }
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
        "=== Void Recon ===\n\n"
        "MODULATION & RF SETTINGS:\n"
        "  mod <0-4>      - Set modulation (0=2FSK,1=GFSK,2=ASK/OOK,3=4FSK,4=MSK)\n"
        "  freq <MHz>     - Set frequency (300-348, 387-464, 779-928 MHz)\n"
        "  dev <kHz>      - Set frequency deviation (1.58-380.85 kHz)\n"
        "  chan <0-255>   - Set channel number\n"
        "  chsp <kHz>     - Set channel spacing (25.39-405.45 kHz)\n"
        "  bw <kHz>       - Set receive bandwidth (58.03-812.50 kHz)\n"
        "  rate <kbaud>   - Set data rate (0.02-1621.83 kBaud)\n"
        "  power <dBm>    - Set transmit power (-30 to 12 dBm)\n\n"
        "SYNC & PACKET SETTINGS:\n"
        "  syncmode <0-7> - Set sync mode\n"
        "  sync <hi,lo>   - Set sync word (decimal)\n"
        "  adrchk <0-3>   - Set address check mode\n"
        "  addr <0-255>   - Set device address\n"
        "  whitening <0/1>- Enable/disable data whitening\n"
        "  pktfmt <0-3>   - Set packet format\n"
        "  lenconf <0-3>  - Set length configuration\n"
        "  pktlen <bytes> - Set packet length\n"
        "  crc <0/1>      - Enable/disable CRC\n"
        "  crcaf <0/1>    - Enable/disable CRC auto-flush\n"
        "  dcfilter <0/1> - Enable/disable DC filter\n"
        "  manchester <0/1>- Enable/disable Manchester encoding\n"
        "  fec <0/1>      - Enable/disable FEC\n"
        "  pre <0-7>      - Set preamble length\n"
        "  pqt <0-255>    - Set preamble quality threshold\n"
        "  append <0/1>   - Enable/disable status byte append\n\n"
        "OPERATIONS:\n"
        "  rx             - Start/stop packet sniffer\n"
        "  tx <hex>       - Transmit packet (hex values)\n"
        "  jam            - Start/stop jamming\n"
        "  chat           - Enter/exit chat mode\n"
        "  brute <us> <bits> - Brute force garage gate\n\n"
        "RECORDING & REPLAY:\n"
        "  rec            - Start/stop frame recording\n"
        "  add <hex>      - Manually add frame to buffer\n"
        "  show           - Show recorded frames\n"
        "  flush          - Clear recording buffer\n"
        "  play [N]       - Replay frame N or all frames\n"
        "  save           - Save buffer to EEPROM\n"
        "  load           - Load buffer from EEPROM\n\n"
        "RAW MODE:\n"
        "  rxraw <us>     - Sniff raw signal with sampling interval\n"
        "  recraw <us>    - Record raw signal\n"
        "  addraw <hex>   - Add raw data to buffer\n"
        "  showraw        - Show raw buffer content\n"
        "  showbit        - Show raw buffer as bits\n"
        "  playraw <us>   - Replay raw data\n"
        "  saveraw        - Save raw buffer to EEPROM\n"
        "  loadraw        - Load raw buffer from EEPROM\n\n"
        "  showsig        - Show current signal in memory\n" 
        "FIXED CODE:\n"
        "  recsig         - Record fixed code remote\n"
        "  playsig        - Play fixed code from buffer\n"
        "  savesig        - Save fixed code to EEPROM\n"
        "  loadsig        - Load fixed code from EEPROM\n\n"
        "UTILITY:\n"
        "  rssi           - Show RSSI and LQI\n"
        "  analyze        - Scan for strongest signal\n"
        "  echo <0/1>     - Enable/disable command echo\n"
        "  stop           - Stop all active modes\n"
        "  reset          - Reset CC1101\n"
        "  init           - Reinitialize CC1101\n"
        "  status         - Show current status\n"
        "  help           - Show this help\n"
    ));
}

void CommandHandler::processLine(const char* line) {
    char cmd[32];
    char args[96];
    cmd[0] = '\0';
    args[0] = '\0';
    
    // Parse command and arguments
    const char* space = strchr(line, ' ');
    if (space) {
        int cmdLen = space - line;
        if (cmdLen > 31) cmdLen = 31;
        strncpy(cmd, line, cmdLen);
        cmd[cmdLen] = '\0';
        strcpy(args, space + 1);
    } else {
        strcpy(cmd, line);
    }
    
    // Handle commands
    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        cmdHelp();
    }
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
    else if (strcmp(cmd, "chat") == 0) {
        if (chatMode) cmdStopChat();
        else cmdStartChat();
    }
    else if (strcmp(cmd, "rx") == 0) {
        if (receivingMode) cmdStopRx();
        else cmdStartRx();
    }
    else if (strcmp(cmd, "tx") == 0) cmdTransmit(args);
    else if (strcmp(cmd, "jam") == 0) {
        if (jammingMode) cmdStopJam();
        else cmdStartJam();
    }
    else if (strcmp(cmd, "brute") == 0) cmdBruteForce(args);
    else if (strcmp(cmd, "rec") == 0) {
        if (recordingMode) cmdStopRec();
        else cmdStartRec();
    }
    else if (strcmp(cmd, "add") == 0) cmdAddFrame(args);
    else if (strcmp(cmd, "show") == 0) cmdShowFrames();
    else if (strcmp(cmd, "flush") == 0) cmdFlush();
    else if (strcmp(cmd, "play") == 0) {
        if (strlen(args) == 0) cmdPlayAll();
        else cmdPlay(args);
    }
    else if (strcmp(cmd, "save") == 0) cmdSave();
    else if (strcmp(cmd, "load") == 0) cmdLoad();
    else if (strcmp(cmd, "rxraw") == 0) cmdRxRaw(args);
    else if (strcmp(cmd, "recraw") == 0) cmdRecRaw(args);
    else if (strcmp(cmd, "addraw") == 0) cmdAddRaw(args);
    else if (strcmp(cmd, "playraw") == 0) cmdPlayRaw(args);
    else if (strcmp(cmd, "saveraw") == 0) cmdSaveRaw();
    else if (strcmp(cmd, "loadraw") == 0) cmdLoadRaw();
    else if (strcmp(cmd, "showraw") == 0) cmdShowRaw();
    else if (strcmp(cmd, "showbit") == 0) cmdShowBits();
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
        Serial.printf("Unknown command: %s\n", cmd);
        Serial.println("Type 'help' for available commands");
    }
}

// Command implementations
void CommandHandler::cmdHelp() { printHelp(); }

void CommandHandler::cmdSetModulation(const char* args) {
    int mode = atoi(args);
    rf.setModulation(mode);
    const char* modes[] = {"2-FSK", "GFSK", "ASK/OOK", "4-FSK", "MSK"};
    Serial.printf("Modulation: %s\n", modes[mode]);
}

void CommandHandler::cmdSetFrequency(const char* args) {
    float freq = atof(args);
    rf.setFrequency(freq);
    Serial.printf("Frequency: %.2f MHz\n", freq);
}

void CommandHandler::cmdSetDeviation(const char* args) {
    float dev = atof(args);
    rf.setDeviation(dev);
    Serial.printf("Deviation: %.2f kHz\n", dev);
}

void CommandHandler::cmdSetChannel(const char* args) {
    int chan = atoi(args);
    rf.setChannel(chan);
    Serial.printf("Channel: %d\n", chan);
}

void CommandHandler::cmdSetChannelSpacing(const char* args) {
    float spacing = atof(args);
    rf.setChannelSpacing(spacing);
    Serial.printf("Channel spacing: %.2f kHz\n", spacing);
}

void CommandHandler::cmdSetRxBandwidth(const char* args) {
    float bw = atof(args);
    rf.setRxBandwidth(bw);
    Serial.printf("Rx bandwidth: %.2f kHz\n", bw);
}

void CommandHandler::cmdSetDataRate(const char* args) {
    float rate = atof(args);
    rf.setDataRate(rate);
    Serial.printf("Data rate: %.2f kBaud\n", rate);
}

void CommandHandler::cmdSetPower(const char* args) {
    int power = atoi(args);
    rf.setPower(power);
    Serial.printf("TX power: %d dBm\n", power);
}

void CommandHandler::cmdSetSyncMode(const char* args) {
    int mode = atoi(args);
    rf.setSyncMode(mode);
    Serial.printf("Sync mode: %d\n", mode);
}

void CommandHandler::cmdSetSyncWord(const char* args) {
    int high, low;
    sscanf(args, "%d,%d", &high, &low);
    rf.setSyncWord(high, low);
    Serial.printf("Sync word: hi=%d, lo=%d\n", high, low);
}

void CommandHandler::cmdSetAddressCheck(const char* args) {
    int mode = atoi(args);
    rf.setAddressCheck(mode);
    Serial.printf("Address check: %d\n", mode);
}

void CommandHandler::cmdSetAddress(const char* args) {
    int addr = atoi(args);
    rf.setAddress(addr);
    Serial.printf("Address: %d\n", addr);
}

void CommandHandler::cmdSetWhitening(const char* args) {
    bool enable = atoi(args) == 1;
    rf.setWhitening(enable);
    Serial.printf("Whitening: %s\n", enable ? "ON" : "OFF");
}

void CommandHandler::cmdSetPacketFormat(const char* args) {
    int format = atoi(args);
    rf.setPacketFormat(format);
    Serial.printf("Packet format: %d\n", format);
}

void CommandHandler::cmdSetLengthConfig(const char* args) {
    int config = atoi(args);
    rf.setLengthConfig(config);
    Serial.printf("Length config: %d\n", config);
}

void CommandHandler::cmdSetPacketLength(const char* args) {
    int len = atoi(args);
    rf.setPacketLength(len);
    Serial.printf("Packet length: %d\n", len);
}

void CommandHandler::cmdSetCRC(const char* args) {
    bool enable = atoi(args) == 1;
    rf.setCRC(enable);
    Serial.printf("CRC: %s\n", enable ? "ON" : "OFF");
}

void CommandHandler::cmdSetCRCAutoFlush(const char* args) {
    bool enable = atoi(args) == 1;
    rf.setCRCAutoFlush(enable);
    Serial.printf("CRC auto-flush: %s\n", enable ? "ON" : "OFF");
}

void CommandHandler::cmdSetDCFilter(const char* args) {
    bool enable = atoi(args) == 1;
    rf.setDCFilter(enable);
    Serial.printf("DC filter: %s\n", enable ? "ON" : "OFF");
}

void CommandHandler::cmdSetManchester(const char* args) {
    bool enable = atoi(args) == 1;
    rf.setManchester(enable);
    Serial.printf("Manchester: %s\n", enable ? "ON" : "OFF");
}

void CommandHandler::cmdSetFEC(const char* args) {
    bool enable = atoi(args) == 1;
    rf.setFEC(enable);
    Serial.printf("FEC: %s\n", enable ? "ON" : "OFF");
}

void CommandHandler::cmdSetPreamble(const char* args) {
    int mode = atoi(args);
    rf.setPreamble(mode);
    Serial.printf("Preamble: %d\n", mode);
}

void CommandHandler::cmdSetPQT(const char* args) {
    int threshold = atoi(args);
    rf.setPQT(threshold);
    Serial.printf("PQT: %d\n", threshold);
}

void CommandHandler::cmdSetAppendStatus(const char* args) {
    bool enable = atoi(args) == 1;
    rf.setAppendStatus(enable);
    Serial.printf("Append status: %s\n", enable ? "ON" : "OFF");
}

void CommandHandler::cmdGetRSSI() {
    Serial.printf("RSSI: %d dBm\n", rf.getRssi());
    Serial.printf("LQI: %d\n", rf.getLqi());
}

void CommandHandler::cmdAnalyze() {
    Serial.println("Waiting for signals [SEND ANY KEY TO STOP]...");
    analyzer.scan();
}

void CommandHandler::cmdStartChat() {
    chatMode = true;
    receivingMode = false;
    jammingMode = false;
    recordingMode = false;
    Serial.println("Chat mode enabled. Type messages to send.");
}

void CommandHandler::cmdStopChat() {
    chatMode = false;
    Serial.println("Chat mode disabled");
}

void CommandHandler::cmdStartRx() {
    receivingMode = true;
    jammingMode = false;
    recordingMode = false;
    chatMode = false;
    rf.setRxMode();
    Serial.println("Receiving enabled");
}

void CommandHandler::cmdStopRx() {
    receivingMode = false;
    Serial.println("Receiving disabled");
}

void CommandHandler::cmdTransmit(const char* args) {
    int len = strlen(args);
    if (len > 0 && len <= 120) {
        asciiToHex(byteBuffer, (const byte*)args, len);
        int dataLen = len / 2;
        rf.send(byteBuffer, dataLen);
        Serial.printf("Transmitted %d bytes\n", dataLen);
    } else {
        Serial.println("Invalid hex data");
    }
}

void CommandHandler::cmdStartJam() {
    jammingMode = true;
    receivingMode = false;
    recordingMode = false;
    chatMode = false;
    jammer.start();
}

void CommandHandler::cmdStopJam() {
    jammingMode = false;
    jammer.stop();
}

void CommandHandler::cmdBruteForce(const char* args) {
    // Parse arguments: microseconds and bits
    int us = atoi(args);
    const char* bitsStr = strchr(args, ' ');
    int bits = bitsStr ? atoi(bitsStr + 1) : 0;
    
    if (us <= 0 || bits <= 0) {
        Serial.println("Usage: brute <microseconds> <number-of-bits>");
        return;
    }
    
    Serial.printf("Brute forcing %d bits with %d us symbol time...\n", bits, us);
    Serial.println("This may take a while...");
    
    // Generate all possible combinations
    unsigned long maxCode = (1UL << bits) - 1;
    Serial.printf("Total combinations: %lu\n", maxCode + 1);
    
    rf.setCCMode(false);
    rf.setPacketFormat(3);
    rf.setTxMode();
    pinMode(2, OUTPUT);
    
    for (unsigned long code = 0; code <= maxCode; code++) {
        // Convert code to signal
        for (int bit = bits - 1; bit >= 0; bit--) {
            digitalWrite(2, (code & (1UL << bit)) ? HIGH : LOW);
            delayMicroseconds(us);
        }
        delay(50); // Small delay between attempts
        
        // Check if any key pressed to stop
        if (Serial.available()) {
            Serial.println("\nBrute force stopped by user");
            break;
        }
    }
    
    rf.setCCMode(true);
    rf.setPacketFormat(0);
    rf.setTxMode();
    
    Serial.println("Brute force complete");
}

void CommandHandler::cmdStartRec() {
    recordingMode = true;
    receivingMode = false;
    jammingMode = false;
    chatMode = false;
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
        int dataLen = len / 2;
        recorder.addFrame(byteBuffer, dataLen);
        Serial.printf("Added frame. Total: %d frames\n", recorder.getFrameCount());
    } else {
        Serial.println("Invalid hex data");
    }
}

void CommandHandler::cmdShowFrames() {
    recorder.showFrames();
}

void CommandHandler::cmdFlush() {
    recorder.flush();
}

void CommandHandler::cmdPlay(const char* args) {
    int index = atoi(args);
    recorder.playFrame(index - 1);
}

void CommandHandler::cmdPlayAll() {
    recorder.playAll();
}

void CommandHandler::cmdSave() {
    recorder.save();
}

void CommandHandler::cmdLoad() {
    recorder.load();
}

void CommandHandler::cmdRxRaw(const char* args) {
    int interval = atoi(args);
    if (interval <= 0) {
        Serial.println("Invalid interval");
        return;
    }
    
    rf.setCCMode(false);
    rf.setPacketFormat(3);
    rf.setRxMode();
    pinMode(2, INPUT);
    
    Serial.println("Raw sniffer enabled...");
    while (!Serial.available()) {
        // Use a local buffer for raw data
        byte rawBuffer[4096];
        for (int i = 0; i < 4096; i++) {
            byte data = 0;
            for (int j = 7; j >= 0; j--) {
                bitWrite(data, j, digitalRead(2));
                delayMicroseconds(interval);
            }
            rawBuffer[i] = data;
        }
        // Output in hex
        byte hexOutput[128];
        for (int i = 0; i < 4096; i += 32) {
            hexToAscii(hexOutput, &rawBuffer[i], 32);
            Serial.print((char*)hexOutput);
        }
    }
    
    Serial.println("\nStopping sniffer");
    rf.setCCMode(true);
    rf.setPacketFormat(0);
    rf.setRxMode();
}
void CommandHandler::cmdRecRaw(const char* args) {
    int interval = atoi(args);
    if (interval <= 0) {
        Serial.println("Invalid interval");
        return;
    }
    
    rf.setCCMode(false);
    rf.setPacketFormat(3);
    rf.setRxMode();
    pinMode(2, INPUT);
    
    Serial.println("Waiting for signal to start recording [SEND ANY KEY TO STOP]...");
    delayMicroseconds(1000);
    while (digitalRead(2) == LOW) {
        if (Serial.available()) {
            Serial.println("Recording cancelled");
            return;
        }
    }
    
    Serial.println("Recording to the buffer...");
    byte rawBuffer[4096];
    int i = 0;
    for (i = 0; i < 4096; i++) {
        byte data = 0;
        for (int j = 7; j >= 0; j--) {
            bitWrite(data, j, digitalRead(2));
            delayMicroseconds(interval);
        }
        rawBuffer[i] = data;
        if (i % 10 == 0 && Serial.available()) {
            Serial.println("Recording stopped by user.");
            break;
        }
    }
    
    recorder.addRaw(rawBuffer, i);
    Serial.println("Recording complete.");
    rf.setCCMode(true);
    rf.setPacketFormat(0);
    rf.setRxMode();
}

void CommandHandler::cmdAddRaw(const char* args) {
    int len = strlen(args);
    if (len > 0 && len <= 120) {
        asciiToHex(byteBuffer, (const byte*)args, len);
        int dataLen = len / 2;
        recorder.addRaw(byteBuffer, dataLen);
        Serial.printf("Added %d raw bytes\n", dataLen);
    } else {
        Serial.println("Invalid hex data");
    }
}

void CommandHandler::cmdShowRaw() {
    recorder.showRaw();
}

void CommandHandler::cmdShowBits() {
    recorder.showBits();
}

void CommandHandler::cmdPlayRaw(const char* args) {
    int interval = atoi(args);
    if (interval <= 0) {
        Serial.println("Invalid interval");
        return;
    }
    recorder.playRaw(interval);
}

void CommandHandler::cmdSaveRaw() {
    recorder.saveRaw();
}

void CommandHandler::cmdLoadRaw() {
    recorder.loadRaw();
}

void CommandHandler::cmdRecSig() {
    // Setup like original code
    ELECHOUSE_cc1101.Init();
    ELECHOUSE_cc1101.SetRx();
    rcSwitch.enableReceive();
    
    Serial.println("Waiting for radio signal [SEND ANY KEY TO STOP]...");
    
    while (!Serial.available()) {
        if (rcSwitch.available()) {
            unsigned long code = rcSwitch.getValue();
            unsigned int bits = rcSwitch.getBits();
            unsigned int delay = rcSwitch.getDelay();
            unsigned int protocol = rcSwitch.getProtocol();
            
            // Format output like original
            const char* b = RCSwitchHandler::dec2binWzerofill(code, bits);
            Serial.print("Decimal: ");
            Serial.print(code);
            Serial.print(" (");
            Serial.print(bits);
            Serial.print("Bit) Binary: ");
            Serial.print(b);
            Serial.print(" Tri-State: ");
            Serial.print(RCSwitchHandler::bin2tristate(b));
            Serial.print(" PulseLength: ");
            Serial.print(delay);
            Serial.print(" microseconds");
            Serial.print(" Protocol: ");
            Serial.println(protocol);
            
            // Get raw data
            unsigned int raw[128];
            int rawLen = rcSwitch.getReceivedRawData(raw);
            Serial.print("Raw data: ");
            for (int i = 0; i < rawLen && raw[i] != 0; i++) {
                Serial.print(raw[i]);
                Serial.print(",");
            }
            Serial.println();
            Serial.println();
            
            // Store in memory (but NOT EEPROM - user must save manually)
            lastSigCode = code;
            lastSigBits = bits;
            lastSigDelay = delay;
            lastSigProtocol = protocol;
            
            Serial.println("Signal received! Use 'savesig' to save to EEPROM.");
            
            rcSwitch.resetAvailable();
            break;
        }
    }
    
    rcSwitch.disable();
}

void CommandHandler::cmdSaveSig() {
    if (lastSigCode == 0 && lastSigBits == 0) {
        Serial.println("No signal in memory. Use 'recsig' first.");
        return;
    }
    
    Serial.println("Saving signal to EEPROM...");
    
    EEPROM.write(0, (byte)(lastSigCode >> 24));
    EEPROM.write(1, (byte)(lastSigCode >> 16));
    EEPROM.write(2, (byte)(lastSigCode >> 8));
    EEPROM.write(3, (byte)(lastSigCode));
    EEPROM.write(4, (byte)(lastSigBits >> 8));
    EEPROM.write(5, (byte)(lastSigBits));
    EEPROM.write(6, (byte)(lastSigDelay >> 8));
    EEPROM.write(7, (byte)(lastSigDelay));
    EEPROM.write(8, (byte)(lastSigProtocol >> 8));
    EEPROM.write(9, (byte)(lastSigProtocol));
    EEPROM.commit();
    
    Serial.printf("Saved: Code=%lu, Bits=%u, Delay=%u, Protocol=%u\n", 
                  lastSigCode, lastSigBits, lastSigDelay, lastSigProtocol);
    Serial.println("Saved successfully!");
}

void CommandHandler::cmdLoadSig() {
    // Load from EEPROM
    unsigned long code = 0;
    unsigned int bits = 0, delay = 0, protocol = 0;
    
    code = ((unsigned long)EEPROM.read(0) << 24) |
           ((unsigned long)EEPROM.read(1) << 16) |
           ((unsigned long)EEPROM.read(2) << 8) |
           (unsigned long)EEPROM.read(3);
    bits = ((unsigned int)EEPROM.read(4) << 8) |
           (unsigned int)EEPROM.read(5);
    delay = ((unsigned int)EEPROM.read(6) << 8) |
            (unsigned int)EEPROM.read(7);
    protocol = ((unsigned int)EEPROM.read(8) << 8) |
               (unsigned int)EEPROM.read(9);
    
    if (code == 0 && bits == 0) {
        Serial.println("No signal saved in EEPROM. Use 'recsig' then 'savesig' first.");
        return;
    }
    
    // Store in memory
    lastSigCode = code;
    lastSigBits = bits;
    lastSigDelay = delay;
    lastSigProtocol = protocol;
    
    Serial.println("Loaded signal from EEPROM:");
    Serial.printf("Code: %lu, Bits: %u, Delay: %u, Protocol: %u\n", 
                  code, bits, delay, protocol);
}

void CommandHandler::cmdPlaySig() {
    if (lastSigCode == 0 && lastSigBits == 0) {
        Serial.println("No signal loaded. Use 'recsig' or 'loadsig' first.");
        return;
    }
    
    Serial.print("Sending signal: ");
    Serial.printf("Code: %lu, Bits: %u, Delay: %u, Protocol: %u\n", 
                  lastSigCode, lastSigBits, lastSigDelay, lastSigProtocol);
    
    // Setup CC1101 like original
    ELECHOUSE_cc1101.setCCMode(0);
    ELECHOUSE_cc1101.setPktFormat(3);
    ELECHOUSE_cc1101.SetTx();
    rcSwitch.enableTransmit();
    delay(200);
    
    rcSwitch.setProtocol(lastSigProtocol);
    rcSwitch.setPulseLength(lastSigDelay);
    rcSwitch.setRepeatTransmit(5);
    rcSwitch.send(lastSigCode, lastSigBits);
    
    // Restore CC1101
    ELECHOUSE_cc1101.setCCMode(1);
    ELECHOUSE_cc1101.setPktFormat(0);
    ELECHOUSE_cc1101.SetTx();
    
    Serial.println("Signal sent!");
}
void CommandHandler::cmdShowSig() {
    if (lastSigCode == 0 && lastSigBits == 0) {
        Serial.println("No signal loaded in memory.");
        Serial.println("Use 'recsig' to receive a signal, or 'loadsig' to load from EEPROM.");
        return;
    }
    
    Serial.println("=== Current Signal in Memory ===");
    Serial.printf("  Code:     %lu\n", lastSigCode);
    Serial.printf("  Bits:     %u\n", lastSigBits);
    Serial.printf("  Delay:    %u us\n", lastSigDelay);
    Serial.printf("  Protocol: %u\n", lastSigProtocol);
    
    // Show binary representation
    const char* b = RCSwitchHandler::dec2binWzerofill(lastSigCode, lastSigBits);
    Serial.printf("  Binary:   %s\n", b);
    Serial.printf("  Tri-State: %s\n", RCSwitchHandler::bin2tristate(b));
    
    // Show raw data (if available)
    Serial.println("  Raw data: (not stored in this format)");
    Serial.println("================================");
}
void CommandHandler::cmdEcho(const char* args) {
    echoEnabled = atoi(args) == 1;
    Serial.printf("Echo: %s\n", echoEnabled ? "ON" : "OFF");
}

void CommandHandler::cmdStop() {
    receivingMode = false;
    jammingMode = false;
    recordingMode = false;
    chatMode = false;
    jammer.stop();
    Serial.println("All modes stopped");
}

void CommandHandler::cmdReset() {
    rf.reset();
    Serial.println("CC1101 reset");
}

void CommandHandler::cmdInit() {
    rf.init();
    Serial.println("CC1101 reinitialized");
}

void CommandHandler::cmdStatus() {
    Serial.println("=== CC1101 Status ===");
    Serial.printf("Receiving: %s\n", receivingMode ? "ON" : "OFF");
    Serial.printf("Jamming: %s\n", jammingMode ? "ON" : "OFF");
    Serial.printf("Recording: %s\n", recordingMode ? "ON" : "OFF");
    Serial.printf("Chat: %s\n", chatMode ? "ON" : "OFF");
    Serial.printf("Frames recorded: %d\n", recorder.getFrameCount());
    Serial.printf("Buffer position: %d\n", recorder.getBufferPos());
    Serial.printf("Echo: %s\n", echoEnabled ? "ON" : "OFF");
}

void CommandHandler::handleRx() {
    if (!receivingMode && !recordingMode) return;
    
    if (rf.checkReceiveFlag()) {
        byte buffer[64];
        int len;
        if (rf.receive(buffer, len)) {
            if (receivingMode) {
                byte hexOutput[128];
                hexToAscii(hexOutput, buffer, len);
                Serial.print((char*)hexOutput);
            }
            if (recordingMode) {
                recorder.addFrame(buffer, len);
            }
            rf.setRxMode();
        }
    }
}

void CommandHandler::handleJam() {
    if (jammingMode) {
        jammer.update();
    }
}

void CommandHandler::handleChat() {
    if (!chatMode) return;
    
    // Handle incoming serial data (sending)
    while (Serial.available()) {
        int i = 0;
        byte chatBuffer[64];
        while (Serial.available() && i < 63) {
            byte data = Serial.read();
            chatBuffer[i++] = data;
            // Echo back
            Serial.write(data);
            if (data == '\r') {
                chatBuffer[i++] = '\n';
            }
        }
        if (i > 0) {
            rf.send(chatBuffer, i);
        }
    }
    
    // Handle incoming RF data (receiving)
    if (rf.checkReceiveFlag()) {
        byte buffer[64];
        int len;
        if (rf.receive(buffer, len)) {
            buffer[len] = '\0';
            Serial.print((char*)buffer);
            rf.setRxMode();
        }
    }
}

bool CommandHandler::isChatMode() const {
    return chatMode;
}

void CommandHandler::setChatMode(bool enabled) {
    chatMode = enabled;
}
