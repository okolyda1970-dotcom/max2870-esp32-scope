#ifndef MAX2870_REGS_H
#define MAX2870_REGS_H

#include <stdint.h>

// ============================================================
// СТРУКТУРЫ РЕГИСТРОВ (Точная копия из regAdf.h)
// ============================================================

typedef struct __structR0 {
    uint8_t reserved : 1;       // Bit 31
    uint16_t integ : 16;        // Bits 30:15
    uint16_t frac : 12;         // Bits 14:3
    uint8_t adres : 3;          // Bits 2:0
} StructR0;

typedef struct __structR1 {
    uint8_t reserved : 3;       // Bits 31:29
    uint8_t fazeAdj : 1;        // Bit 28
    uint8_t prescaler : 1;      // Bit 27
    uint16_t phaseValue : 12;   // Bits 26:15
    uint16_t modulsValue : 12;  // Bits 14:3
    uint8_t adres : 3;          // Bits 2:0
} StructR1;

typedef struct __structR2 {
    uint8_t reserved : 1;       // Bit 31
    uint8_t noiseMode : 2;      // Bits 30:29
    uint8_t muxOut : 3;         // Bits 28:26
    uint8_t referenceDoubler : 1;// Bit 25
    uint8_t Rdiv2 : 1;          // Bit 24
    uint16_t R_counter : 10;    // Bits 23:14
    uint8_t doubleBufferR4 : 1; // Bit 13
    uint8_t chargePumpCurrent : 4;// Bits 12:9
    uint8_t ldf : 1;            // Bit 8
    uint8_t ldp : 1;            // Bit 7
    uint8_t PD_polarity : 1;    // Bit 6
    uint8_t powerDown : 1;      // Bit 5
    uint8_t CP_freeState : 1;   // Bit 4
    uint8_t counterReset : 1;   // Bit 3
    uint8_t adres : 3;          // Bits 2:0
} StructR2;

typedef struct __structR3 {
    uint8_t reserved : 8;       // Bits 31:24
    uint8_t bandSelect : 1;     // Bit 23
    uint8_t ABP : 1;            // Bit 22
    uint8_t chargeCancel : 1;   // Bit 21
    uint8_t reserved2 : 2;      // Bits 20:19
    uint8_t CSR : 1;            // Bit 18
    uint8_t reserved3 : 1;      // Bit 17
    uint8_t CLK_divMode : 2;    // Bits 16:15
    uint16_t clockDividerValue : 12; // Bits 14:3
    uint8_t adres : 3;          // Bits 2:0
} StructR3;

typedef struct __structR4 {
    uint8_t reserved : 8;       // Bits 31:24
    uint8_t FBselect : 1;       // Bit 23
    uint8_t RFdividerSelect : 3;// Bits 22:20
    uint8_t bandSelectclockDividerValue : 8; // Bits 19:12
    uint8_t VCOpowerDown : 1;   // Bit 11
    uint8_t MTLD : 1;           // Bit 10
    uint8_t AUXoutputSelect : 1;// Bit 9
    uint8_t AUXoutputEnable : 1;// Bit 8
    uint8_t AUXoutputPower : 2; // Bits 7:6
    uint8_t RFoutputEnable : 1; // Bit 5
    uint8_t outputPower : 2;    // Bits 4:3
    uint8_t adres : 3;          // Bits 2:0
} StructR4;

typedef struct __structR5 {
    uint8_t reserved : 8;       // Bits 31:24
    uint8_t LDpinMode : 2;      // Bits 23:22
    uint8_t reserved2 : 1;      // Bit 21
    uint8_t reserved3 : 2;      // Bits 20:19
    uint16_t reserved4 : 16;    // Bits 18:3
    uint8_t adres : 3;          // Bits 2:0
} StructR5;

// Объявление глобальных экземпляров (определяются в main.cpp)
extern StructR0 tipDef_0;
extern StructR1 tipDef_1;
extern StructR2 tipDef_2;
extern StructR3 tipDef_3;
extern StructR4 tipDef_4;
extern StructR5 tipDef_5;

// Прототипы функций формирования слов регистров
uint32_t initReg0(void);
uint32_t initReg1(void);
uint32_t initReg2(void);
uint32_t initReg3(void);
uint32_t initReg4(void);
uint32_t initReg5(void);

#endif // MAX2870_REGS_H