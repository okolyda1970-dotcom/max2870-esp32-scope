#ifndef SCOPE_STREAMER_H
#define SCOPE_STREAMER_H
#include "config.h"

class ScopeStreamer {
public:
    void init();
    void startStreaming();
    void stopStreaming();
    bool isActive() const { return active; }
    void sendDataBlock(); // Вызывается из loop()

private:
    bool active = false;
    const uint8_t SAMPLE_COUNT = 20;
};
extern ScopeStreamer scope;
#endif