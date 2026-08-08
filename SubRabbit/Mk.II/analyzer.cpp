// analyzer.cpp
#include "analyzer.h"

const uint32_t FrequencyAnalyzer::FREQUENCY_LIST[] = {
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

FrequencyAnalyzer::~FrequencyAnalyzer() {}

void FrequencyAnalyzer::init() {
    // Analyzer doesn't need separate initialization
}

void FrequencyAnalyzer::setRssiThreshold(int threshold) {
    rssi_threshold = threshold;
}

int FrequencyAnalyzer::getRssi() {
    return ELECHOUSE_cc1101.getRssi();
}

int FrequencyAnalyzer::getLqi() {
    return ELECHOUSE_cc1101.getLqi();
}

void FrequencyAnalyzer::setFrequency(float freq) {
    ELECHOUSE_cc1101.setMHZ(freq);
}

void FrequencyAnalyzer::setRxBandwidth(float bw) {
    ELECHOUSE_cc1101.setRxBW(bw);
}

void FrequencyAnalyzer::coarseScan(FrequencyRSSI& result) {
    ELECHOUSE_cc1101.setRxBW(650);
    for (size_t i = 0; i < FREQUENCY_LIST_SIZE; i++) {
        uint32_t frequency = FREQUENCY_LIST[i];
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
            Serial.print("Fine: ");
            Serial.print(result.frequency_fine / 1000000.0);
            Serial.print("MHz");
            Serial.print(" | RSSI: ");
            Serial.println(result.rssi_fine);
        } else if (result.rssi_coarse > rssi_threshold) {
            Serial.print("Coarse: ");
            Serial.print(result.frequency_coarse / 1000000.0);
            Serial.print("MHz");
            Serial.print(" | RSSI: ");
            Serial.println(result.rssi_coarse);
        }
        delay(10);
    }
}
