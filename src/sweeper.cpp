#include "sweeper.h"
#include <SPI.h>

Sweeper sweeper;

void Sweeper::init() {
    params.active = false;
}

void Sweeper::startScan(float start, float stop, float stepKhz) {
    params.startFreqMHz = start;
    params.stopFreqMHz = stop;
    params.stepKHz = stepKhz;
    params.currentFreqMHz = start;
    params.active = true;
    lastTime = 0;
}

void Sweeper::stopScan() {
    params.active = false;
}

void Sweeper::processStep() {
    if (!params.active) return;
    
    unsigned long now = millis();
    // Интервал между точками сканирования (мс)
    // Зависит от времени установления PLL (~5-10ms) + чтение ADC
    if (now - lastTime < 10) return; 
    lastTime = now;

    if (params.currentFreqMHz > params.stopFreqMHz) {
        stopScan();
        Serial.println(">>> SWEEP FINISHED");
        return;
    }

    // 1. Меняем частоту PLL
    // Используем глобальный экземпляр pll из pll_controller.cpp
    extern PllController pll;
    // Предполагаем дефолтные параметры CP и Power для скана
    pll.applySettings(params.currentFreqMHz, 4, 3, 1); 
    
    // 2. Читаем уровень сигнала
    uint16_t adcVal = (uint16_t)analogRead(ADC_PIN);
    
    // 3. Отправляем результат Java-клиенту
    // Формат: SCAN_RESULT:<freq>,<level>\n
    char msg[64];
    sprintf(msg, "SCAN_RESULT:%.3f,%u\n", params.currentFreqMHz, adcVal);
    Serial.print(msg);
    
    // 4. Следующая точка
    params.currentFreqMHz += (params.stepKHz / 1000.0f);
}