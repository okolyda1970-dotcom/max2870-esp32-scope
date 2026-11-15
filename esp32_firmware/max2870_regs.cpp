#include "max2870_regs.h"

// Реализация функций формирования слов регистров
// Точная копия логики из regAdf.c вашего STM32 проекта

uint32_t initReg0(void) {
    return ((uint32_t)tipDef_0.adres << 0) | 
           ((uint32_t)tipDef_0.frac << 3) | 
           ((uint32_t)tipDef_0.integ << 15) | 
           ((uint32_t)tipDef_0.reserved << 31);
}

uint32_t initReg1(void) {
    return ((uint32_t)tipDef_1.adres << 0) | 
           ((uint32_t)tipDef_1.modulsValue << 3) | 
           ((uint32_t)tipDef_1.phaseValue << 15) | 
           ((uint32_t)tipDef_1.prescaler << 27) |
           ((uint32_t)tipDef_1.fazeAdj << 28) | 
           ((uint32_t)tipDef_1.reserved << 29);
}

uint32_t initReg2(void) {
    return ((uint32_t)tipDef_2.adres << 0) | 
           ((uint32_t)tipDef_2.counterReset << 3) | 
           ((uint32_t)tipDef_2.CP_freeState << 4) |
           ((uint32_t)tipDef_2.powerDown << 5) | 
           ((uint32_t)tipDef_2.PD_polarity << 6) |  
           ((uint32_t)tipDef_2.ldp << 7) | 
           ((uint32_t)tipDef_2.ldf << 8) |
           ((uint32_t)tipDef_2.chargePumpCurrent << 9) | 
           ((uint32_t)tipDef_2.doubleBufferR4 << 13) | 
           ((uint32_t)tipDef_2.R_counter << 14) |
           ((uint32_t)tipDef_2.Rdiv2 << 24) | 
           ((uint32_t)tipDef_2.referenceDoubler << 25) | 
           ((uint32_t)tipDef_2.muxOut << 26) | 
           ((uint32_t)tipDef_2.noiseMode << 29) |
           ((uint32_t)tipDef_2.reserved << 31);
}

uint32_t initReg3(void) {
    return ((uint32_t)tipDef_3.adres << 0) | 
           ((uint32_t)tipDef_3.clockDividerValue << 3) | 
           ((uint32_t)tipDef_3.CLK_divMode << 15) | 
           ((uint32_t)tipDef_3.reserved3 << 17) |
           ((uint32_t)tipDef_3.CSR << 18) | 
           ((uint32_t)tipDef_3.reserved2 << 19) | 
           ((uint32_t)tipDef_3.chargeCancel << 21) | 
           ((uint32_t)tipDef_3.ABP << 22) |
           ((uint32_t)tipDef_3.bandSelect << 23) | 
           ((uint32_t)tipDef_3.reserved << 24);
}

uint32_t initReg4(void) {
    return ((uint32_t)tipDef_4.adres << 0) | 
           ((uint32_t)tipDef_4.outputPower << 3) | 
           ((uint32_t)tipDef_4.RFoutputEnable << 5) | 
           ((uint32_t)tipDef_4.AUXoutputPower << 6) |
           ((uint32_t)tipDef_4.AUXoutputEnable << 8) | 
           ((uint32_t)tipDef_4.AUXoutputSelect << 9) | 
           ((uint32_t)tipDef_4.MTLD << 10) | 
           ((uint32_t)tipDef_4.VCOpowerDown << 11) |
           ((uint32_t)tipDef_4.bandSelectclockDividerValue << 12) | 
           ((uint32_t)tipDef_4.RFdividerSelect << 20) | 
           ((uint32_t)tipDef_4.FBselect << 23) |
           ((uint32_t)tipDef_4.reserved << 24);
}

uint32_t initReg5(void) {
    return ((uint32_t)tipDef_5.adres << 0) | 
           ((uint32_t)tipDef_5.reserved4 << 3) | 
           ((uint32_t)tipDef_5.reserved3 << 19) |
           ((uint32_t)tipDef_5.reserved2 << 21) | 
           ((uint32_t)tipDef_5.LDpinMode << 22) | 
           ((uint32_t)tipDef_5.reserved << 24);
}