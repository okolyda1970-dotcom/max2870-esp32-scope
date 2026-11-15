#include "pll_controller.h"
#include <SPI.h>

PllController pll;

void PllController::init() {
    // Инициализация SPI
    SPI.begin(ADF_SCK, -1, ADF_MOSI, -1); 
    SPI.setFrequency(1000000); // 1 MHz
    
    pinMode(ADF_CE, OUTPUT);
    pinMode(ADF_LE, OUTPUT);
    
    digitalWrite(ADF_CE, HIGH);    
    digitalWrite(ADF_LE, LOW);
    
    Serial.println("PLL Controller Initialized");
}

bool PllController::isLocked() {
    return digitalRead(MUX_PIN) == HIGH;
}

// Метод для отправки одного 32-битного пакета через SPI
void PllController::writePacket(uint32_t packet) {
    digitalWrite(ADF_CE, LOW);
    delayMicroseconds(1);
    
    // Отправка MSB first (4 байта)
    for (int i = 0; i < 4; i++) {
        uint8_t byteVal = (packet >> (24 - i*8)) & 0xFF;
        SPI.transfer(byteVal);
    }
    
    delayMicroseconds(1);
    digitalWrite(ADF_CE, HIGH);
    
    // Импульс LE (Latch Enable)
    digitalWrite(ADF_LE, HIGH);
    delayMicroseconds(1);
    digitalWrite(ADF_LE, LOW);
}

void PllController::applySettings(float freqMHz, uint8_t cpCurrent, uint8_t outputPower, uint8_t ldPinMode) {
    if (freqMHz <= 0 || freqMHz > 6000.0f) return;

    // --- РАСЧЕТ ДЕЛИТЕЛЕЙ ДЛЯ FREF = 25 МГц ---
    float fpd = REF_FREQ_MHZ; // 25.0f
    uint32_t nInt = (uint32_t)(freqMHz / fpd);
    float remainder = freqMHz - (nInt * fpd);
    
    uint32_t mModulator = 4096; // Стандартное значение для низкой фазового шума
    uint32_t fracNum = (uint32_t)round((remainder / fpd) * mModulator);
    
    if (fracNum >= mModulator) {
        nInt++;
        fracNum -= mModulator;
    }

    Serial.printf("Calc for %.2f MHz: N=%lu, Frac=%lu, M=%lu\n", freqMHz, nInt, fracNum, mModulator);

    // Helper to build 32-bit packet: [MSB=1][Address(5)][Data(24)]
    auto buildPacket = [](uint8_t addr, uint32_t data24) -> uint32_t {
        return (0x80000000) | ((addr & 0x1F) << 24) | (data24 & 0xFFFFFF);
    };

    // ============================================================
    // ПОЛНАЯ ЗАПИСЬ ВСЕХ РЕГИСТРОВ (R0-R5) В ПРАВИЛЬНОМ ПОРЯДКЕ!
    // ============================================================

    // REGISTERS MUST BE WRITTEN IN ORDER FROM HIGHEST ADDRESS TO LOWEST
    // OR ACCORDING TO SPECIFIC SEQUENCE. For MAX2870, typical safe order is R5->R0 or R0->R5 depending on config.
    // Let's use the standard initialization sequence found in many drivers: R5, R4, R3, R2, R1, R0.

    // --- Register 5: Charge Pump Control & Test Modes ---
    uint32_t r5_data = 0;
    r5_data |= (0 << 23); // CP Output Enable (Active Low? Check DS. Usually 0=Enable)
    r5_data |= (cpCurrent & 0xF) << 16; // CP Current Code
    r5_data |= (0 << 15); // Shutdown VCO Calibration
    r5_data |= (0 << 14); // Shutdown Reference Clock
    r5_data |= (0 << 13); // Shutdown RF Divider
    r5_data |= (0 << 12); // Shutdown AUX Output
    r5_data |= (0 << 11); // Shutdown MUX Counter
    r5_data |= (0 << 10); // Shutdown Phase Detector
    r5_data |= (0 << 9);  // Shutdown Charge Pump
    r5_data |= (0 << 8);  // Shutdown VCO Band Select
    r5_data |= (0 << 7);  // Shutdown VCO Bias
    r5_data |= (0 << 6);  // Shutdown VCO Varactor
    r5_data |= (0 << 5);  // Shutdown VCO Switch Driver
    r5_data |= (0 << 4);  // Shutdown VCO Array
    r5_data |= (0 << 3);  // Shutdown VCO Calibrate
    r5_data |= (0 << 2);  // Shutdown VCO Tune
    r5_data |= (0 << 1);  // Shutdown VCO Loop Filter
    r5_data |= (0 << 0);  // Shutdown VCO
    
    writePacket(buildPacket(5, r5_data));
    delayMicroseconds(100);

    // --- Register 4: Modulator Control ---
    uint32_t r4_data = 0;
    r4_data |= (0 << 23); // MASH Enable (Set to 1 for better spectral purity)
    r4_data |= (0 << 22); // MASH Reset
    r4_data |= (0 << 21); // Lock Detect Precision (1=Fast, 0=Slow/Stable)
    r4_data |= (0 << 20); // ABP Mode (Auto Band Preset) - CRITICAL FOR LOCKING
    r4_data |= (0 << 19); // Charge Cancellation
    r4_data |= (0 << 18); // PD Polarity
    r4_data |= (0 << 17); // CP Tri-State
    r4_data |= (ldPinMode & 0x3) << 14; // LD Pin Mode (Bits 15-14 usually)
    r4_data |= (0 << 13); // MUX Out Select
    r4_data |= (0 << 12); // AUX Output Enable
    r4_data |= (0 << 11); // VCO Power Down
    r4_data |= (0 << 10); // RF Divider A Index Upper
    r4_data |= (0 << 9);  // RF Divider A Index Lower
    r4_data |= (0 << 8);  // RF Divider B Index
    r4_data |= (0 << 7);  // RF Divider C Index
    r4_data |= (0 << 6);  // RF Divider D Index
    r4_data |= (0 << 5);  // RF Divider E Index
    r4_data |= (0 << 4);  // RF Divider F Index
    r4_data |= (0 << 3);  // RF Divider G Index
    r4_data |= (0 << 2);  // RF Divider H Index
    r4_data |= (0 << 1);  // RF Divider I Index
    r4_data |= (0 << 0);  // RF Divider J Index
    
    // Simplified R4 based on common usage patterns:
    r4_data = 0;
    r4_data |= (1 << 20); // ABP ON (Essential!)
    r4_data |= (0 << 19); // Charge Cancel OFF
    r4_data |= (0 << 18); // PD Positive
    r4_data |= (0 << 17); // CP Active
    r4_data |= ((ldPinMode & 0x3) << 14); // LD Pin Mode
    r4_data |= (0 << 13); // MUX Normal Op
    r4_data |= (0 << 12); // Aux Disabled
    r4_data |= (0 << 11); // VCO Enabled
    
    writePacket(buildPacket(4, r4_data));
    delayMicroseconds(100);

    // --- Register 3: Frequency Synthesizer (Upper Bits + Power) ---
    uint32_t r3_data = 0;
    r3_data |= (((nInt >> 16) & 0xFF) << 16); // N[23:16]
    r3_data |= (((fracNum >> 16) & 0xFF) << 8); // Frac[23:16]
    r3_data |= ((mModulator >> 16) & 0xFF);     // M[23:16]
    r3_data |= ((outputPower & 0x3) << 19);      // Output Power Code (Check exact bit position in DS Rev E)
    
    writePacket(buildPacket(3, r3_data));
    delayMicroseconds(100);

    // --- Register 2: Frequency Synthesizer (Middle Bits) ---
    uint32_t r2_data = 0;
    r2_data |= (((nInt >> 8) & 0xFF) << 16); // N[15:8]
    r2_data |= (((fracNum >> 8) & 0xFF) << 8); // Frac[15:8]
    r2_data |= ((mModulator >> 8) & 0xFF);     // M[15:8]
    
    writePacket(buildPacket(2, r2_data));
    delayMicroseconds(100);

    // --- Register 1: Frequency Synthesizer (Lower Bits) ---
    uint32_t r1_data = 0;
    r1_data |= ((nInt & 0xFF) << 16);       // N[7:0]
    r1_data |= ((fracNum & 0xFF) << 8);     // Frac[7:0]
    r1_data |= (mModulator & 0xFF);         // M[7:0]
    
    writePacket(buildPacket(1, r1_data));
    delayMicroseconds(100);

    // --- Register 0: Global Configuration ---
    uint32_t r0_data = 0;
    r0_data |= ((cpCurrent & 0xF) << 16);   // CP Current Code (Again? Check overlap with R5/R4)
                                             // In some revisions CP is only in R0 bits 19-16.
    r0_data |= ((ldPinMode & 0x3) << 24);   // LD Pin Mode
    r0_data |= (1 << 20);                   // ABP Mode ON (Redundant but safe)
    r0_data |= (0 << 21);                   // VCO Power Down OFF (VCO On)
    
    writePacket(buildPacket(0, r0_data));
    
    Serial.println("All registers written.");
    delay(50); // Wait for lock detection circuitry
    
    bool locked = isLocked();
    Serial.printf("RESULT: %s\n", locked ? "SUCCESS: PLL LOCKED!" : "FAILURE: STILL UNLOCKED");
}