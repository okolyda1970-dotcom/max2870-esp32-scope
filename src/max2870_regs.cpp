#include "max2870_regs.h"

StructR0 tipDef_0;
StructR1 tipDef_1;
StructR2 tipDef_2;
StructR3 tipDef_3;
StructR4 tipDef_4;
StructR5 tipDef_5;

uint32_t initReg0() {
  return ((uint32_t)(tipDef_0.INT & 1) << 31)
       | ((uint32_t)(tipDef_0.N & 0xFFFF) << 15)
       | ((uint32_t)(tipDef_0.FRAC & 0xFFF) << 3)
       | (tipDef_0.ADDR & 7);
}
uint32_t initReg1() {
  return ((uint32_t)(tipDef_1.CPOC & 1) << 31)
       | ((uint32_t)(tipDef_1.CPL & 3) << 29)
       | ((uint32_t)(tipDef_1.CPT & 3) << 27)
       | ((uint32_t)(tipDef_1.P & 0xFFF) << 15)
       | ((uint32_t)(tipDef_1.M & 0xFFF) << 3)
       | (tipDef_1.ADDR & 7);
}
uint32_t initReg2() {
  return ((uint32_t)(tipDef_2.LDS & 1) << 31)
       | ((uint32_t)(tipDef_2.SDN & 3) << 29)
       | ((uint32_t)(tipDef_2.MUX & 7) << 26)
       | ((uint32_t)(tipDef_2.DBR & 1) << 25)
       | ((uint32_t)(tipDef_2.RDIV2 & 1) << 24)
       | ((uint32_t)(tipDef_2.R & 0x3FF) << 14)
       | ((uint32_t)(tipDef_2.REG4DB & 1) << 13)
       | ((uint32_t)(tipDef_2.CP & 0xF) << 9)
       | ((uint32_t)(tipDef_2.LDF & 1) << 8)
       | ((uint32_t)(tipDef_2.LDP & 1) << 7)
       | ((uint32_t)(tipDef_2.PDP & 1) << 6)
       | ((uint32_t)(tipDef_2.SHDN & 1) << 5)
       | ((uint32_t)(tipDef_2.TRI & 1) << 4)
       | ((uint32_t)(tipDef_2.RST & 1) << 3)
       | (tipDef_2.ADDR & 7);
}
uint32_t initReg3() {
  return ((uint32_t)(tipDef_3.VCO & 0x3F) << 26)
       | ((uint32_t)(tipDef_3.VAS_SHDN & 1) << 25)
       | ((uint32_t)(tipDef_3.RETUNE & 1) << 24)
       | ((uint32_t)(tipDef_3.CDM & 3) << 15)
       | ((uint32_t)(tipDef_3.CDIV & 0xFFF) << 3)
       | (tipDef_3.ADDR & 7);
}
uint32_t initReg4() {
  return ((uint32_t)(tipDef_4.RESERVED_31_26 & 0x3F) << 26)
       | ((uint32_t)(tipDef_4.BS_MSB & 3) << 24)
       | ((uint32_t)(tipDef_4.FB & 1) << 23)
       | ((uint32_t)(tipDef_4.DIVA & 7) << 20)
       | ((uint32_t)(tipDef_4.BS & 0xFF) << 12)
       | ((uint32_t)(tipDef_4.BDIV & 1) << 9)
       | ((uint32_t)(tipDef_4.RFB_EN & 1) << 8)
       | ((uint32_t)(tipDef_4.BPWR & 3) << 6)
       | ((uint32_t)(tipDef_4.RFA_EN & 1) << 5)
       | ((uint32_t)(tipDef_4.APWR & 3) << 3)
       | (tipDef_4.ADDR & 7);
}
uint32_t initReg5() {
  return ((uint32_t)(tipDef_5.F01 & 1) << 24)
       | ((uint32_t)(tipDef_5.LD & 3) << 22)
       | ((uint32_t)(tipDef_5.MUX_MSB & 1) << 18)
       | (tipDef_5.ADDR & 7);
}