#ifndef MAX2870_REGS_H
#define MAX2870_REGS_H
#include <Arduino.h>

// Поля = биты даташита MAX2870 (Rev 0). Упаковка в initRegX().
struct StructR0 {              // Table 4, addr 000
  uint8_t  INT;                // bit31   0=frac,1=int
  uint16_t N;                  // 30:15
  uint16_t FRAC;               // 14:3
  uint8_t  ADDR;               // 2:0 =0
};
struct StructR1 {              // Table 5, addr 001
  uint8_t  CPOC;               // bit31
  uint8_t  CPL;                // 30:29  00=int,01=frac
  uint8_t  CPT;                // 28:27  00=normal
  uint16_t P;                  // 26:15  (1 recommended)
  uint16_t M;                  // 14:3   2..4095
  uint8_t  ADDR;               // =1
};
struct StructR2 {              // Table 6, addr 010
  uint8_t  LDS;                // bit31
  uint8_t  SDN;                // 30:29  00=low-noise,10/11=low-spur
  uint8_t  MUX;                // 28:26  6=digital LD
  uint8_t  DBR;                // bit25
  uint8_t  RDIV2;              // bit24
  uint16_t R;                  // 23:14
  uint8_t  REG4DB;             // bit13
  uint8_t  CP;                 // 12:9
  uint8_t  LDF;                // bit8   0=frac,1=int
  uint8_t  LDP;                // bit7   0=10ns,1=6ns
  uint8_t  PDP;                // bit6   0=inverting,1=non-inv
  uint8_t  SHDN;               // bit5
  uint8_t  TRI;                // bit4
  uint8_t  RST;                // bit3
  uint8_t  ADDR;               // =2
};
struct StructR3 {              // Table 7, addr 011
  uint8_t  VCO;                // 31:26 (manual, when VAS off)
  uint8_t  VAS_SHDN;           // bit25  0=VAS enabled
  uint8_t  RETUNE;             // bit24  1=auto-retune
  uint8_t  CDM;                // 16:15  00=off
  uint16_t CDIV;               // 14:3
  uint8_t  ADDR;               // =3
};
struct StructR4 {              // Table 8, addr 100
  uint8_t  RESERVED_31_26;     // 31:26 =0b011000
  uint8_t  BS_MSB;             // 25:24
  uint8_t  FB;                 // bit23  1=fundamental
  uint8_t  DIVA;               // 22:20
  uint8_t  BS;                 // 19:12 (low 8 of BS)
  uint8_t  BDIV;               // bit9
  uint8_t  RFB_EN;             // bit8
  uint8_t  BPWR;               // 7:6
  uint8_t  RFA_EN;             // bit5
  uint8_t  APWR;               // 4:3
  uint8_t  ADDR;               // =4
};
struct StructR5 {              // Table 9, addr 101
  uint8_t  F01;                // bit24  1=auto int when F=0
  uint8_t  LD;                 // 23:22  01=digital LD
  uint8_t  MUX_MSB;            // bit18
  uint8_t  ADDR;               // =5
};

extern StructR0 tipDef_0;
extern StructR1 tipDef_1;
extern StructR2 tipDef_2;
extern StructR3 tipDef_3;
extern StructR4 tipDef_4;
extern StructR5 tipDef_5;

uint32_t initReg0();
uint32_t initReg1();
uint32_t initReg2();
uint32_t initReg3();
uint32_t initReg4();
uint32_t initReg5();
#endif