#ifndef PLL_CONTROLLER_H
#define PLL_CONTROLLER_H

#include "config.h"

class PllController {
public:
    void init();
    
    /**
     * Применяет полный набор настроек к MAX2870.
     */
    void applySettings(float freqMHz, uint8_t cpCurrent, uint8_t outputPower, uint8_t ldPinMode);

    /**
     * Проверяет статус блокировки PLL.
     */
    bool isLocked();

private:
    // Новый метод для отправки сырого пакета
    void writePacket(uint32_t packet);
};

extern PllController pll;

#endif