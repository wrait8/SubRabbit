// analyzer.h
#ifndef ANALYZER_H
#define ANALYZER_H

#include <Arduino.h>
#include <SmartRC_CC1101.h>

class FrequencyAnalyzer {
public:
    FrequencyAnalyzer();
    ~FrequencyAnalyzer();
    
    void init();
    void scan();
    void setRssiThreshold(int threshold);
    int getRssi();
    int getLqi();
    void setFrequency(float freq);
    void setRxBandwidth(float bw);
    
private:
    static const uint32_t FREQUENCY_LIST[];
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

#endif
