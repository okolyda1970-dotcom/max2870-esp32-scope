#ifndef SWEEPER_H
#define SWEEPER_H
#include "config.h"
#include "pll_controller.h"

class Sweeper {
public:
    void init();
    void startScan(float start, float stop, float stepKhz);
    void stopScan();
    bool isActive() const { return params.active; }
    void processStep(); // Выполняет один шаг сканирования

private:
    struct ScanParams {
        float startFreqMHz;
        float stopFreqMHz;
        float stepKHz;
        float currentFreqMHz;
        bool active;
    };
    ScanParams params;
    unsigned long lastTime = 0;
};
extern Sweeper sweeper;
#endif