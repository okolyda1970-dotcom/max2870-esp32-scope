#include <Arduino.h>
#include <SPI.h>
#include <cmath>
#include "config.h"
#include "max2870_regs.h"

static void safePrint(const String& s)   { if (Serial && Serial.availableForWrite() >= (int)s.length() + 2) Serial.print(s); }
static void safePrintln(const String& s) { if (Serial && Serial.availableForWrite() >= (int)s.length() + 4) Serial.println(s); }

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
  tipDef_0.ADDR = 0;
  tipDef_1.ADDR = 1;  tipDef_1.CPT = 0;  tipDef_1.P = 1;  tipDef_1.M = 4095;
  tipDef_2.ADDR = 2;
  tipDef_2.LDS = 0; tipDef_2.SDN = 0; tipDef_2.MUX = 6;
  tipDef_2.DBR = 0; tipDef_2.RDIV2 = 0; tipDef_2.R = 1;
  tipDef_2.REG4DB = 0; tipDef_2.CP = 4;
  tipDef_2.LDP = 0; tipDef_2.PDP = 0;
  tipDef_2.SHDN = 0; tipDef_2.TRI = 0; tipDef_2.RST = 0;
  tipDef_3.ADDR = 3;
  tipDef_3.VCO = 0; tipDef_3.VAS_SHDN = 0; tipDef_3.RETUNE = 1;
  tipDef_3.CDM = 0; tipDef_3.CDIV = 2;
  tipDef_4.ADDR = 4;
  tipDef_4.RESERVED_31_26 = 0b011000;
  tipDef_4.BS_MSB = (500 >> 8) & 3; tipDef_4.BS = 500 & 0xFF;
  tipDef_4.FB = 1;
  tipDef_4.BDIV = 0; tipDef_4.RFB_EN = 0; tipDef_4.BPWR = 0;
  tipDef_4.RFA_EN = 1; tipDef_4.APWR = 3;
  tipDef_5.ADDR = 5;
  tipDef_5.F01 = 1; tipDef_5.LD = 0b01; tipDef_5.MUX_MSB = 0;
}

bool setFrequency(float freqMHz) {
  uint8_t range = 0; uint32_t divCode = 0;
  if      (freqMHz >= 23.5  && freqMHz < 46.875) { range = 128; divCode = 7; }
  else if (freqMHz >= 46.875&& freqMHz < 93.75 ) { range = 64;  divCode = 6; }
  else if (freqMHz >= 93.75 && freqMHz < 187.5 ) { range = 32;  divCode = 5; }
  else if (freqMHz >= 187.5 && freqMHz < 375   ) { range = 16;  divCode = 4; }
  else if (freqMHz >= 375   && freqMHz < 750   ) { range = 8;   divCode = 3; }
  else if (freqMHz >= 750   && freqMHz < 1500  ) { range = 4;   divCode = 2; }
  else if (freqMHz >= 1500  && freqMHz < 3000  ) { range = 2;   divCode = 1; }
  else if (freqMHz >= 3000  && freqMHz <= 6000 ) { range = 1;   divCode = 0; }
  else return false;
  float fVCO  = freqMHz * range;
  float nReal = fVCO / REF_FREQ_MHZ;
  int N = (int)floor(nReal);
  const int M = 4095;
  int F = (int)((nReal - N) * M + 0.5f);
  if (F >= M) { N++; F -= M; }
  bool isInt = (F == 0);
  tipDef_0.INT = isInt ? 1 : 0;  tipDef_0.N = N;  tipDef_0.FRAC = F;
  tipDef_2.LDF = isInt ? 1 : 0;
  tipDef_1.CPOC = isInt ? 1 : 0; tipDef_1.CPL = isInt ? 0 : 1;
  tipDef_4.DIVA = divCode;
  return true;
}

void applySettings(bool print) {
  uint32_t r[6] = { initReg0(), initReg1(), initReg2(), initReg3(), initReg4(), initReg5() };
  if (print) safePrintln("R0: " + String(r[0], HEX) + "  R5: " + String(r[5], HEX));
  for (int k = 0; k < 2; k++) {
    for (int a = 5; a >= 0; a--) sendRegister(r[a]);
    delay(20);
  }
}

bool checkLockStatus() {
  unsigned long start = micros();
  int highs = 0;
  while (micros() - start < 10000) {
    if (digitalRead(MUX_PIN) == HIGH) highs++;
    delayMicroseconds(10);
  }
  return highs > 5;
}

// ================= SCOPE: BURST SNAPSHOTS =================
#define SNAP_MAX 250
static uint16_t snapBuf[SNAP_MAX];
static uint8_t  snapPkt[2 + SNAP_MAX * 2];
static uint32_t snapFs     = 14000;
static uint16_t snapN      = 250;
static uint16_t snapPeriod = 100;
static bool     scopeOn    = false;
static unsigned long lastFrameMs = 0;

static uint32_t adcUsPerSample = 56;

static void measureAdcSpeed() {
  uint32_t t = micros();
  for (int i = 0; i < 100; i++) analogRead(ADC_PIN);
  uint32_t d = micros() - t;
  adcUsPerSample = d / 100;
  if (adcUsPerSample == 0) adcUsPerSample = 1;
}

static void sendFrameChunked(const uint8_t* p, int len) {
  if (!Serial) return;
  int sent = 0;
  uint32_t t0 = millis();
  while (sent < len && (millis() - t0) < 200) {
    int space = Serial.availableForWrite();
    if (space > 0) {
      int n = len - sent;
      if (n > space) n = space;
      Serial.write(p + sent, n);
      sent += n;
    } else {
      delay(1);
    }
  }
}

static void captureFrame() {
  uint32_t interval = 1000000UL / snapFs;
  uint32_t t0 = micros();
  for (uint16_t i = 0; i < snapN; i++) {
    while ((uint32_t)(micros() - t0) < (uint32_t)i * interval) { }
    snapBuf[i] = analogRead(ADC_PIN);
  }
  snapPkt[0] = DATA_HEADER;
  snapPkt[1] = (uint8_t)snapN;
  for (uint16_t i = 0; i < snapN; i++) {
    snapPkt[2 + i * 2]     = snapBuf[i] & 0xFF;
    snapPkt[2 + i * 2 + 1] = (snapBuf[i] >> 8) & 0xFF;
  }
  sendFrameChunked(snapPkt, 2 + snapN * 2);
}

// ================= SWEEP (Goertzel — частотно-избирательный) =================
#define SWEEP_SETTLE_MS   25
#define SWEEP_MEAS_FS     8000
#define SWEEP_MEAS_N      64
#define SWEEP_IF_HZ       200.0f

static bool sweepActive = false;
static uint8_t sweepPhase = 0;              // 0 = пауза на захват, 1 = замер
static float sweepCur = 0, sweepStopF = 0, sweepStep = 0;
static unsigned long sweepPhaseStart = 0;
static uint16_t measBuf[SWEEP_MEAS_N];
static uint16_t measIdx = 0;
static uint32_t measNextUs = 0;
static float bestF = 0; static int bestL = -1;

static int goertzelLevel() {
  uint32_t sum = 0;
  for (int i = 0; i < SWEEP_MEAS_N; i++) sum += measBuf[i];
  double mean = sum / (double)SWEEP_MEAS_N;
  double coeff = 2.0 * cos(2.0 * PI * SWEEP_IF_HZ / SWEEP_MEAS_FS);
  double s1 = 0, s2 = 0;
  for (int i = 0; i < SWEEP_MEAS_N; i++) {
    double s0 = (measBuf[i] - mean) + coeff * s1 - s2;
    s2 = s1; s1 = s0;
  }
  double power = s1 * s1 + s2 * s2 - coeff * s1 * s2;
  if (power < 0) power = 0;
  return (int)(sqrt(power) * 2.0 / SWEEP_MEAS_N);
}

// ================= протокол =================
static uint8_t rxBuf[CTRL_SIZE];
static uint8_t rxIdx = 0;
static unsigned long lastByteMs = 0;
static String textBuf = "";

static void handlePacket() {
  uint32_t sum = 0;
  for (int i = 0; i < CTRL_SIZE - 4; i++) sum += rxBuf[i];
  uint32_t got; memcpy(&got, rxBuf + CTRL_SIZE - 4, 4);
  if (sum != got) { safePrintln("CHECKSUM FAIL"); return; }
  ControlPacket pkt; memcpy(&pkt, rxBuf, sizeof(pkt));
  if (pkt.frequency > 0) {
    if (sweepActive) { sweepActive = false; safePrintln("SWEEP: stopped"); }
    tipDef_2.CP   = pkt.cp_current & 0xF;
    tipDef_4.APWR = pkt.output_power & 3;
    tipDef_5.LD   = pkt.ld_pin_mode & 3;
    if (setFrequency(pkt.frequency)) {
      applySettings(true);
      delay(60);
      safePrintln("APPLIED " + String(pkt.frequency, 6) + " MHz LD=" + String(pkt.ld_pin_mode)
                  + " CP=" + String(pkt.cp_current) + " -> " + (checkLockStatus() ? "LOCKED" : "UNLOCKED"));
    } else safePrintln("FREQ OUT OF RANGE");
  } else {
    scopeOn = true; lastFrameMs = 0;
    safePrintln(">>> SCOPE ON (burst)");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  pinMode(STATUS_LED, OUTPUT); pinMode(MUX_PIN, INPUT);
  pinMode(ADF_CE, OUTPUT); pinMode(ADF_LE, OUTPUT);
  digitalWrite(ADF_CE, HIGH); digitalWrite(ADF_LE, LOW);
  SPI.begin(ADF_SCK, -1, ADF_MOSI, -1);
  SPI.setFrequency(1000000); SPI.setDataMode(SPI_MODE0); SPI.setBitOrder(MSBFIRST);
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);
  measureAdcSpeed();
  defaultSet();
  if (setFrequency(1000.0)) applySettings(true);
  safePrintln("=== MAX2870 PRODUCTION (burst scope + Goertzel sweep) ===");
}

void loop() {
  if (rxIdx > 0 && (millis() - lastByteMs) > 50) rxIdx = 0;
  while (Serial.available()) {
    uint8_t b = (uint8_t)Serial.read();
    lastByteMs = millis();
    if (b == STOP_HEADER) { rxIdx = 0; textBuf = ""; scopeOn = false; sweepActive = false; safePrintln(">>> STOP"); continue; }
    if (rxIdx == 0) {
      if (b == PACKET_HEADER) { rxBuf[0] = b; rxIdx = 1; }
      else if (b == '\n') {
        if (textBuf.length()) {
          if (textBuf.startsWith("SCOPE_CFG:")) {
            String s = textBuf.substring(10);
            int c1 = s.indexOf(','), c2 = s.indexOf(',', c1 + 1);
            if (c1 > 0 && c2 > c1) {
              uint32_t fs = s.substring(0, c1).toInt();
              int n = s.substring(c1 + 1, c2).toInt();
              int p = s.substring(c2 + 1).toInt();
              if (fs >= 1000 && fs <= 100000 && n >= 20 && n <= SNAP_MAX && p >= 20 && p <= 2000) {
                uint32_t maxFs = 1000000UL / adcUsPerSample;
                uint32_t allowed = (uint32_t)(maxFs * 0.8);
                if (fs > allowed) fs = allowed;
                snapFs = fs; snapN = n; snapPeriod = p;
                safePrintln("SCOPE_CFG ok: fs=" + String(snapFs) + " N=" + String(snapN) + " T=" + String(snapPeriod)
                            + " load=" + String((int)(snapN * 2 * 1000 / snapPeriod)) + " B/s"
                            + " adc_us=" + String(adcUsPerSample) + " max_fs=" + String(maxFs));
              } else safePrintln("SCOPE_CFG bad range");
            }
          }
          else if (textBuf.startsWith("SWEEP_START:")) {
            String s = textBuf.substring(12);
            int c1 = s.indexOf(','), c2 = s.indexOf(',', c1 + 1);
            if (c1 > 0 && c2 > c1) {
              float a = s.substring(0, c1).toFloat();
              float bnd = s.substring(c1 + 1, c2).toFloat();
              float stk = s.substring(c2 + 1).toFloat();
              if (a > 0 && bnd >= a && stk > 0) {
                scopeOn = false;
                sweepCur = a; sweepStopF = bnd; sweepStep = stk / 1000.0f;
                bestL = -1; bestF = a;
                sweepActive = true; sweepPhase = 0; sweepPhaseStart = millis();
                setFrequency(sweepCur + SWEEP_IF_HZ / 1000000.0f); applySettings(false);
                safePrintln("SWEEP: started " + String(a, 3) + "->" + String(bnd, 3) + " MHz, step " + String(stk) + " kHz");
              }
            }
          }
          else if (textBuf.startsWith("SWEEP_STOP")) { sweepActive = false; safePrintln("SWEEP: stopped"); }
          else if (textBuf.startsWith("CP:"))  { int v = textBuf.substring(3).toInt(); if (v >= 0 && v <= 15) { tipDef_2.CP = v; applySettings(true); safePrintln("CP set to " + String(v)); } }
          else if (textBuf.startsWith("SDN:")) { int v = textBuf.substring(4).toInt(); if (v >= 0 && v <= 3)  { tipDef_2.SDN = v; applySettings(true); safePrintln("SDN set to " + String(v)); } }
          else if (textBuf.startsWith("ADC_BENCH")) {
            uint32_t interval = 1000000UL / snapFs;
            uint32_t t1 = micros();
            for (int i = 0; i < 100; i++) analogRead(ADC_PIN);
            uint32_t perSample = (micros() - t1) / 100;
            if (perSample == 0) perSample = 1;

            uint16_t mn = 4095, mx = 0;
            uint32_t t0 = micros();
            for (uint16_t i = 0; i < snapN; i++) {
              while ((uint32_t)(micros() - t0) < (uint32_t)i * interval) { }
              uint16_t v = analogRead(ADC_PIN);
              if (v < mn) mn = v;
              if (v > mx) mx = v;
            }
            uint32_t dur = micros() - t0;
            if (dur == 0) dur = 1;
            uint32_t realFs = (uint32_t)((uint64_t)snapN * 1000000ULL / dur);

            safePrintln("ADC_BENCH: us_per_sample=" + String(perSample)
                        + " max_fs=" + String(1000000UL / perSample)
                        + " | N=" + String(snapN)
                        + " dur_us=" + String(dur)
                        + " expect_us=" + String((uint32_t)snapN * interval)
                        + " real_fs=" + String(realFs)
                        + " | min=" + String(mn) + " max=" + String(mx));
          }
          else { float f = textBuf.toFloat(); if (f > 0 && setFrequency(f)) { sweepActive = false; applySettings(true); safePrintln("TXT freq updated"); } }
          textBuf = "";
        }
      }
      else if (b != '\r') { textBuf += (char)b; if (textBuf.length() > 64) textBuf = ""; }
      continue;
    }
    rxBuf[rxIdx++] = b;
    if (rxIdx == CTRL_SIZE) { rxIdx = 0; handlePacket(); }
  }

  // --- Sweep (Goertzel, неблокирующий) ---
  if (sweepActive) {
    if (sweepPhase == 0) {
      if (millis() - sweepPhaseStart >= SWEEP_SETTLE_MS) {
        sweepPhase = 1; measIdx = 0; measNextUs = micros();
      }
    } else {
      if (measIdx < SWEEP_MEAS_N) {
        if ((int32_t)(micros() - measNextUs) >= 0) {
          measBuf[measIdx++] = analogRead(ADC_PIN);
          measNextUs += 1000000UL / SWEEP_MEAS_FS;
        }
      } else {
        int level = goertzelLevel();
        if (level > bestL) { bestL = level; bestF = sweepCur; }
        safePrintln("SCAN_RESULT:" + String(sweepCur, 4) + "," + String(level));
        sweepCur += sweepStep;
        if (sweepCur > sweepStopF + 1e-7) {
          sweepActive = false;
          safePrintln("SWEEP: done");
          safePrintln("SWEEP_PEAK:" + String(bestF, 4) + "," + String(bestL));
        } else {
          setFrequency(sweepCur + SWEEP_IF_HZ / 1000000.0f); applySettings(false);
          sweepPhase = 0; sweepPhaseStart = millis();
        }
      }
    }
  }

  // --- Burst scope ---
  if (scopeOn && !sweepActive && (millis() - lastFrameMs >= snapPeriod)) {
    lastFrameMs = millis();
    captureFrame();
  }

  // --- статус ---
  static unsigned long t = 0;
  if (millis() - t > 500) { t = millis(); digitalWrite(STATUS_LED, checkLockStatus() ? HIGH : LOW); }
  static unsigned long p = 0;
  if (millis() - p > 1000) {
    p = millis();
    if (!scopeOn && !sweepActive) {
      if (digitalRead(STATUS_LED) == HIGH) safePrintln(".");
      else safePrintln("WARNING: UNLOCKED!");
    }
  }
  yield();
}