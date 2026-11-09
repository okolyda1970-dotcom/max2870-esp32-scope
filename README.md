# MAX2870 Controller & Digital Scope via ESP32-C3

This project allows controlling the Analog Devices MAX2870 RF synthesizer using an ESP32-C3 microcontroller and a custom Java Swing GUI with a built-in digital oscilloscope.

## Features

### Hardware Side (ESP32-C3)
- **Binary Protocol Communication:** Reliable control over UART using fixed-size packets (24 bytes).
- **PLL Control:** Adjust frequency, charge pump current, output power, LD pin mode, and fine-tuning steps.
- **ADC Streaming:** Real-time data acquisition from GPIO0 (connected to mixer IF output) sent back to PC during scope operation.
- **Status Monitoring:** Periodic lock/unlock status updates via text markers (`.` for locked).

### Software Side (Java GUI)
- **Modern Interface:** Clean Swing-based UI for easy parameter adjustment.
- **Integrated Oscilloscope:**
  - Real-time waveform display.
  - Adjustable Time/Div (horizontal zoom).
  - Adjustable Volt/Div (vertical gain/sensitivity).
  - Trigger Level control.
  - Vertical Position (Pos Y) shift.
  - Pause/Resume functionality for signal analysis.
  - Auto-scale option.
- **Frequency Stepping:** Quick buttons (+/- 1kHz, 10kHz, 100kHz, 1MHz) for precise tuning without sliders.
- **Live Parameter Application:** Change PLL settings while the oscilloscope is running (no stream interruption).

## Prerequisites

1. **Hardware:**
   - ESP32-C3 Dev Board.
   - MAX2870 Module (or discrete circuit).
   - Mixer (e.g., IAM81008) connected to ADC input.
   - USB Cable for Serial communication.

2. **Software:**
   - **PlatformIO / Arduino IDE:** To flash the firmware.
   - **Java JDK 17+:** To run the client application.
   - **jSerialComm Library:** Required for serial port access in Java.

## Installation & Usage

### 1. Flash Firmware
Copy the contents of `esp32_firmware/main.cpp` into your PlatformIO or Arduino project. Ensure pin definitions match your wiring:
- `ADF_SCK`: 6
- `ADF_MOSI`: 7
- `ADF_CE`: 1
- `ADF_LE`: 3
- `MUX_PIN`: 5
- `STATUS_LED`: 10
- `ADC_PIN`: 0

Upload the sketch to your ESP32-C3.

### 2. Run Java Client
1. Import the `java_gui` folder into Eclipse or IntelliJ IDEA.
2. Add `jSerialComm-x.x.x.jar` to your Build Path.
3. Run `Max2870Gui.java`.
4. Select your COM Port and click **Connect**.
5. Click **START SCOPE** to begin streaming ADC data.
6. Use the Frequency buttons and **APPLY ALL SETTINGS** to tune the synthesizer.
7. Adjust Scope Controls (Time/Div, Volt/Div, Pos Y) to analyze the signal shape.

## License
MIT License
