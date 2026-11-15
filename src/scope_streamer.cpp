#include "scope_streamer.h"

ScopeStreamer scope;

void ScopeStreamer::init() {
    analogReadResolution(12);
    pinMode(ADC_PIN, ANALOG);
}

void ScopeStreamer::startStreaming() {
    active = true;
}

void ScopeStreamer::stopStreaming() {
    active = false;
}

void ScopeStreamer::sendDataBlock() {
    if (!active) return;
    
    uint8_t buf[2 + SAMPLE_COUNT * 2];
    buf[0] = DATA_HEADER;
    buf[1] = SAMPLE_COUNT;
    
    for (int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t val = (uint16_t)analogRead(ADC_PIN);
        buf[2 + i*2] = val & 0xFF;
        buf[2 + i*2 + 1] = (val >> 8) & 0xFF;
    }
    Serial.write(buf, sizeof(buf));
    // Минимальная задержка, чтобы не забить буфер UART полностью
    delayMicroseconds(50); 
}