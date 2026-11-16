#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <SPI.h>

// --- PIN DEFINITIONS ---
#define ADF_SCK   6
#define ADF_MOSI  7 
#define ADF_CE    1
#define ADF_LE    3
#define MUX_PIN   5 
#define STATUS_LED 10
#define ADC_PIN   0 
//#define ADC_PIN   4

// --- PROTOCOL CONSTANTS ---
#define PACKET_HEADER 0xAA
#define DATA_HEADER   0xCC
#define STOP_HEADER   0xDD
#define CTRL_SIZE     24

// --- PLL HARDWARE CONSTANTS ---
// !!! ВАЖНО: Проверьте частоту вашего кварцевого резонатора на плате MAX2870 !!!
// Обычно это 10.0 MHz или 25.0 MHz.
#define REF_FREQ_MHZ  25.0f  

#pragma pack(push, 1)
struct ControlPacket {
    uint8_t header; float frequency; uint8_t cp_current; uint8_t pd_polarity;
    uint8_t ldf_mode; uint8_t mux_out_sel; uint8_t band_scan_div; uint8_t rf_div_select;
    uint8_t output_power; uint8_t aux_enable; uint8_t vco_pd; uint8_t abp_mode;
    uint8_t charge_cancel; int16_t tuning_step; uint8_t ld_pin_mode; uint8_t reserved;
    uint32_t checksum;
};
#pragma pack(pop)

#endif