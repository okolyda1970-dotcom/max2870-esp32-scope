#include <Arduino.h>
#include <SPI.h>
#include <cmath>
#include <cstring>
#include "max2870_regs.h"

// ============================================================
// ПИНЫ
// ============================================================
#define ADF_SCK   6
#define ADF_MOSI  7 
#define ADF_CE    1
#define ADF_LE    3
#define MUX_PIN   5 
#define STATUS_LED 10
#define ADC_PIN   0 // GPIO0 для считывания с выхода смесителя IAM81008

// Глобальные структуры
StructR0 tipDef_0;
StructR1 tipDef_1;
StructR2 tipDef_2;
StructR3 tipDef_3;
StructR4 tipDef_4;
StructR5 tipDef_5;

// ============================================================
// ПРОТОКОЛ УПРАВЛЕНИЯ (Binary)
// ============================================================
#define PKT_CTRL_HEADER  0xAA
#define PKT_DATA_HEADER  0xCC // Заголовок для пакетов с данными АЦП
#define PKT_STOP_HEADER  0xDD // Команда остановки потока

#define CTRL_PKT_SIZE    24
#define DATA_PKT_SIZE    64  // Размер блока данных (может содержать N сэмплов uint16_t)
#define SAMPLES_PER_BLOCK ((DATA_PKT_SIZE - 6) / 2) // 6 байт служебных, остальные данные

#pragma pack(push, 1)
struct ControlPacket {
    uint8_t  header;         // 0xAA
    float    frequency;      // 1-4
    uint8_t  cp_current;     // 5
    uint8_t  pd_polarity;    // 6
    uint8_t  ldf_mode;       // 7
    uint8_t  mux_out_sel;    // 8
    uint8_t  band_scan_div;  // 9
    uint8_t  rf_div_select;  // 10
    uint8_t  output_power;   // 11
    uint8_t  aux_enable;     // 12
    uint8_t  vco_pd;         // 13
    uint8_t  abp_mode;       // 14
    uint8_t  charge_cancel;  // 15
    int16_t  tuning_step;    // 16-17
    uint8_t  ld_pin_mode;    // 18
    uint8_t  reserved;       // 19
    uint32_t checksum;       // 20-23
};

struct DataPacket {
    uint8_t  header;         // 0xCC
    uint16_t seq_num;        // 1-2 Номер последовательности (для синхронизации)
    uint8_t  sample_count;   // 3 Количество сэмплов в этом блоке
    uint8_t  adc_resolution_bits; // 4 Разрешение (обычно 12)
    uint8_t  reserved[2];    // 5-6 Резерв
    uint16_t samples[SAMPLES_PER_BLOCK]; // 7... Данные АЦП (Big Endian or Little? Let's use LE for consistency with ESP32 native, but Java expects specific order. We'll send raw uint16_t in LE)
    // Note: The rest of the packet is filled with zeros if samples count is less than max capacity, 
    // OR we just fill exactly what fits. For simplicity, let's assume fixed block size transmission.
};
#pragma pack(pop)

ControlPacket currentCmd;
bool streaming_active = false;
uint16_t stream_seq = 0;

// Буферы приема
uint8_t rxBuf[CTRL_PKT_SIZE];
uint8_t rxIdx = 0;

// ============================================================
// ФУНКЦИИ SPI И РЕГИСТРОВ (Без изменений, сокращенно)
// ============================================================
void sendRegister(uint32_t reg) {
  digitalWrite(ADF_LE, LOW);
  SPI.transfer((reg >> 24) & 0xFF);
  SPI.transfer((reg >> 16) & 0xFF);
  SPI.transfer((reg >> 8)  & 0xFF);
  SPI.transfer(reg & 0xFF);
  digitalWrite(ADF_LE, HIGH);
  delayMicroseconds(2);
  digitalWrite(ADF_LE, LOW);
}

void defaultSet() {
    // ... (ваш код defaultSet без изменений) ...
    tipDef_0.adres = 0; tipDef_0.reserved = 0;
    tipDef_1.adres = 0b001; tipDef_1.fazeAdj = 0; tipDef_1.reserved = 0;
    tipDef_2.adres = 0b010; tipDef_2.counterReset = 0; tipDef_2.CP_freeState = 0; tipDef_2.powerDown = 0;
    tipDef_2.PD_polarity = 0; tipDef_2.ldp = 1; tipDef_2.ldf = 0; tipDef_2.chargePumpCurrent = 4;
    tipDef_2.doubleBufferR4 = 0; tipDef_2.R_counter = 1; tipDef_2.Rdiv2 = 0; tipDef_2.referenceDoubler = 0;
    tipDef_2.muxOut = 6; tipDef_2.noiseMode = 0; tipDef_2.reserved = 0;
    tipDef_3.adres = 0b011; tipDef_3.clockDividerValue = 0; tipDef_3.CLK_divMode = 0; tipDef_3.reserved3 = 0;
    tipDef_3.CSR = 0; tipDef_3.reserved2 = 0; tipDef_3.chargeCancel = 1; tipDef_3.ABP = 0; tipDef_3.bandSelect = 1; tipDef_3.reserved = 0;
    tipDef_4.adres = 0b100; tipDef_4.outputPower = 3; tipDef_4.RFoutputEnable = 1; tipDef_4.AUXoutputPower = 0;
    tipDef_4.AUXoutputEnable = 0; tipDef_4.AUXoutputSelect = 0; tipDef_4.MTLD = 0; tipDef_4.VCOpowerDown = 0;
    tipDef_4.bandSelectclockDividerValue = 200; tipDef_4.RFdividerSelect = 0b010; tipDef_4.FBselect = 1; tipDef_4.reserved = 0;
    tipDef_5.adres = 0b101; tipDef_5.reserved4 = 0; tipDef_5.reserved3 = 0b11; tipDef_5.reserved2 = 0;
    tipDef_5.LDpinMode = 0b01; tipDef_5.reserved = 0;
}

bool applyFromPacket(const ControlPacket& cmd) {
    // ... (ваша логика применения параметров без изменений) ...
    tipDef_2.chargePumpCurrent = cmd.cp_current;
    tipDef_2.PD_polarity = cmd.pd_polarity;
    tipDef_2.ldf = cmd.ldf_mode;
    tipDef_2.muxOut = cmd.mux_out_sel;
    tipDef_4.bandSelectclockDividerValue = cmd.band_scan_div;
    tipDef_4.outputPower = cmd.output_power;
    tipDef_4.AUXoutputEnable = cmd.aux_enable;
    tipDef_4.VCOpowerDown = cmd.vco_pd;
    tipDef_3.ABP = cmd.abp_mode;
    tipDef_3.chargeCancel = cmd.charge_cancel;
    tipDef_5.LDpinMode = cmd.ld_pin_mode;

    float freqMHz = cmd.frequency;
    uint8_t range = 0; uint32_t divCode = 0;

    if (cmd.rf_div_select == 0xFF) {
        if      (freqMHz >= 23.5 && freqMHz < 46.875) { range = 128; divCode = 7; }
        else if (freqMHz >= 46.875 && freqMHz < 93.75) { range = 64; divCode = 6; }
        else if (freqMHz >= 93.75 && freqMHz < 187.5) { range = 32; divCode = 5; }
        else if (freqMHz >= 187.5 && freqMHz < 375) { range = 16; divCode = 4; }
        else if (freqMHz >= 375 && freqMHz < 750) { range = 8; divCode = 3; }
        else if (freqMHz >= 750 && freqMHz < 1500) { range = 4; divCode = 2; }
        else if (freqMHz >= 1500 && freqMHz < 3000) { range = 2; divCode = 1; }
        else if (freqMHz >= 3000 && freqMHz <= 6000) { range = 1; divCode = 0; }
        else return false;
        tipDef_4.RFdividerSelect = divCode;
    } else {
        divCode = cmd.rf_div_select;
        switch(divCode) { case 7: range=128; break; case 6: range=64; break; case 5: range=32; break; case 4: range=16; break; case 3: range=8; break; case 2: range=4; break; case 1: range=2; break; case 0: range=1; break; default: return false; }
        tipDef_4.RFdividerSelect = divCode;
    }

    float fVCO = freqMHz * range;
    float valueN = fVCO / 25.0f;
    int intValue = (int)floorf(valueN);
    float fracPart = valueN - intValue;
    const int MOD_VALUE = 4095;
    int baseFrac = (int)(fracPart * MOD_VALUE);
    long totalFrac = (long)baseFrac + (long)cmd.tuning_step;
    
    while(totalFrac < 0) { intValue--; totalFrac += MOD_VALUE; }
    while(totalFrac > MOD_VALUE) { intValue++; totalFrac -= MOD_VALUE; }
    if(intValue < 0 || intValue > 65535) return false;

    tipDef_0.integ = intValue;
    tipDef_0.frac = totalFrac;
    tipDef_1.modulsValue = MOD_VALUE;
    tipDef_1.prescaler = 1;
    tipDef_1.phaseValue = 0;

    uint32_t r0=initReg0(), r1=initReg1(), r2=initReg2(), r3=initReg3(), r4=initReg4(), r5=initReg5();
    for(int i=0;i<2;i++) { sendRegister(r5); sendRegister(r4); sendRegister(r3); sendRegister(r2); sendRegister(r1); sendRegister(r0); delay(20); }
    return true;
}

uint32_t calcChecksum(const uint8_t* data, size_t len) {
    uint32_t s = 0; for(size_t i=0; i<len; i++) s+=data[i]; return s;
}

// ============================================================
// ЛОГИКА ПОТОКОВОЙ ПЕРЕДАЧИ ДАННЫХ АЦП
// ============================================================

void startStreaming() {
    Serial.println("STREAMING_STARTED");
    streaming_active = true;
    stream_seq = 0;
    analogReadResolution(12); // 12-bit ADC
    pinMode(ADC_PIN, ANALOG);
}

void stopStreaming() {
    Serial.println("STREAMING_STOPPED");
    streaming_active = false;
}

void sendAdcBlock() {
    if (!streaming_active) return;

    const uint8_t SAMPLE_COUNT = 20; // Количество точек в одном пакете
    uint8_t buffer[2 + SAMPLE_COUNT * 2]; 
    
    buffer[0] = 0xCC; // Header
    buffer[1] = SAMPLE_COUNT; // Count
    
    analogReadResolution(12);
    pinMode(ADC_PIN, ANALOG);

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        uint16_t val = (uint16_t)analogRead(ADC_PIN);
        buffer[2 + i*2] = val & 0xFF;       // Low Byte
        buffer[2 + i*2 + 1] = (val >> 8) & 0xFF; // High Byte
    }

    Serial.write(buffer, sizeof(buffer));
    
    // Небольшая пауза, чтобы UART успел передать данные перед следующим циклом loop()
    delayMicroseconds(200); 
}

// ============================================================
// ПРИЕМ КОМАНД
// ============================================================
void pollSerialCommands() {
    while (Serial.available()) {
        uint8_t b = (uint8_t)Serial.read();

        // 1. Проверка на команду СТОП (приоритет №1)
        if (b == PKT_STOP_HEADER) {
            streaming_active = false;
            rxIdx = 0; // Сброс индекса приема контрольного пакета
            return;    // Выход из функции, чтобы начать цикл loop заново без стрима
        }

        // 2. Если стрим активен, мы НЕ собираем Control Packets (чтобы не мешать потоку данных)
        // Но! Мы должны позволять менять частоту. 
        // Решение: Разрешаем сбор Control Packet ТОЛЬКО если header 0xAA найден явно.
        
        if (streaming_active) {
             // Во время стрима игнорируем все байты, кроме начала нового Control Packet?
             // Нет, это сложно синхронизировать. 
             // ЛУЧШИЙ ВАРИАНТ: Останавливать стрим на долю секунды, применять настройки, возобновлять.
             
             // Но так как вы хотите "плавно", давайте сделаем так:
             // ESP32 принимает команду смены частоты, применяет её, и продолжает слать данные АЦП.
             // Для этого нам нужно читать Control Packet параллельно со стримом? Нет, UART один канал.
             
             // КОМПРОМИСС: При получении 0xAA во время стрима - временно пауза стрима, обработка команды, возврат к стриму.
             
             if (rxIdx == 0 && b == PKT_CTRL_HEADER) {
                 rxBuf[0] = b;
                 rxIdx = 1;
                 // Пауза в передаче данных АЦП будет происходить естественно, 
                 // пока мы читаем этот пакет в этом цикле while(Serial.available()).
                 // Как только пакет прочитан (ниже), стрим продолжится в следующем витке loop().
             } else if (rxIdx > 0) {
                 rxBuf[rxIdx++] = b;
                 
                 if (rxIdx == CTRL_PKT_SIZE) {
                     uint32_t calc = calcChecksum(rxBuf, CTRL_PKT_SIZE - 4);
                     uint32_t got; memcpy(&got, rxBuf + 20, 4);

                     if (calc == got) {
                         memcpy(&currentCmd, rxBuf, sizeof(ControlPacket));
                         
                         // Применяем настройки ЧАСТОТЫ, но НЕ трогаем флаг streaming_active!
                         applyFromPacket(currentCmd); 
                         
                         // Опционально: можно вывести подтверждение, но лучше тихо
                     }
                     rxIdx = 0; // Сброс для следующего пакета
                 }
             }
             // Если rxIdx==0 и байт не 0xAA и не 0xDD -> просто мусор или часть потока данных (если бы он шел сюда, но он идет через sendAdcBlock)
             continue;
        }

        // 3. Обычный режим (без стрима): Полный разбор протокола
        if (rxIdx == 0) {
            if (b == PKT_CTRL_HEADER) {
                rxBuf[0] = b;
                rxIdx = 1;
            }
            continue;
        }

        rxBuf[rxIdx++] = b;

        if (rxIdx == CTRL_PKT_SIZE) {
            uint32_t calc = calcChecksum(rxBuf, CTRL_PKT_SIZE - 4);
            uint32_t got; memcpy(&got, rxBuf + 20, 4);

            if (calc == got) {
                memcpy(&currentCmd, rxBuf, sizeof(ControlPacket));
                
                if (currentCmd.frequency < 0) {
                    startStreaming();
                } else {
                    applyFromPacket(currentCmd);
                }
            }
            rxIdx = 0;
        }
    }
}

// ============================================================
// SETUP AND LOOP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  pinMode(STATUS_LED, OUTPUT);
  pinMode(MUX_PIN, INPUT_PULLDOWN);
  pinMode(ADF_CE, OUTPUT);
  pinMode(ADF_LE, OUTPUT);
  digitalWrite(ADF_CE, HIGH);    
  digitalWrite(ADF_LE, LOW);
  
  SPI.begin(ADF_SCK, -1, ADF_MOSI, -1); 
  SPI.setFrequency(1000000);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);

  Serial.println("=== MAX2870 Controller Ready (With Scope) ===");
  
  defaultSet();
  
  memset(&currentCmd, 0, sizeof(ControlPacket));
  currentCmd.header = PKT_CTRL_HEADER;
  currentCmd.frequency = 1000.0f;
  currentCmd.cp_current = 4;
  currentCmd.pd_polarity = 0;
  currentCmd.ldf_mode = 0;
  currentCmd.mux_out_sel = 6;
  currentCmd.band_scan_div = 200;
  currentCmd.rf_div_select = 0xFF;
  currentCmd.output_power = 3;
  currentCmd.aux_enable = 0;
  currentCmd.vco_pd = 0;
  currentCmd.abp_mode = 0;
  currentCmd.charge_cancel = 1;
  currentCmd.tuning_step = 0;
  currentCmd.ld_pin_mode = 1;
  
  applyFromPacket(currentCmd);
}

void loop() {
  // 1. Статус блокировки (только если не стримим, чтобы не мешать)
  if (!streaming_active) {
      int muxState = digitalRead(MUX_PIN);
      static unsigned long lastPrint = 0;
      if (millis() - lastPrint > 1000) {
        lastPrint = millis();
        if (muxState == HIGH) {
          Serial.print("."); 
          digitalWrite(STATUS_LED, HIGH);
        } else {
          Serial.println("\nUNLOCKED");
          digitalWrite(STATUS_LED, LOW);
        }
      }
  }

  // 2. Обработка команд
  pollSerialCommands();

  // 3. Передача данных АЦП
  if (streaming_active) {
      sendAdcBlock();
      // Небольшая задержка, чтобы не забить буфер Serial полностью, 
      // если скорость чтения Java ниже скорости генерации.
      // При 115200 бод ~11KB/s. Блок 64 байта. 
      // Можно слать несколько блоков подряд.
      delayMicroseconds(100); 
  }
}