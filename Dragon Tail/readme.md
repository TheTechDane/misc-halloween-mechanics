# 🐉 Dragon Tail Halloween Prop Controller (v2.4)

An animatronic prop controller firmware built for the **Seeed Studio XIAO ESP32-C3**. It provides non-blocking, potentiometer-controlled continuous rotation servo movements alongside seamless multi-track MP3 audio playback via I2S and LittleFS.

---

## 🛠️ Hardware Requirements & Specifications

* **Microcontroller:** Seeed Studio XIAO ESP32-C3 (RISC-V single-core @ 160MHz)
* **Servo Motor:** 360° Continuous Rotation Servo (e.g., SG90 / MG996R continuous variant)
* **Audio DAC / Amp:** MAX98357A I2S 3.2W Class-D Amplifier
* **Input Control:** 10kΩ Linear Potentiometer
* **Speaker:** 4Ω or 8Ω Passive Speaker (3W+)
* **Power Source:** 5V USB Power Supply / Power Bank (connected via VBUS)

---

## 📌 Hardware Pinout & Wiring Diagram

The Seeed Studio XIAO ESP32-C3 features 7 pins on each side header:

```text
                  +-------------------+
                  |   XIAO ESP32-C3   |
                  |     [USB-C]       |
                  |                   |
       GPIO2/D0 -| D0           VBUS |- VBUS (5V Servo Red / Amp VIN)
       GPIO3/D1 -| D1            GND |- GND (Common Ground)
 (Wiper) GPIO4  -| D2       3.3V-OUT |- 3.3V-OUT (Pot. Pin 1)
  (LRC)  GPIO5  -| D3            D10 |- D10 / GPIO10
  (BCLK) GPIO6  -| D4             D9 |- D9 / GPIO9
  (DOUT) GPIO7  -| D5             D8 |- D8 / GPIO8 (Servo Signal Wire)
  (TX)  GPIO21  -| D6             D7 |- D7 / GPIO20
                  +-------------------+
```

### Exact Hardware Mapping Table

| Component | Component Pin | XIAO ESP32-C3 Pin | GPIO | Power Rail |
| :--- | :--- | :--- | :--- | :--- |
| **Potentiometer** | Pin 1 (VCC) | `3.3V-OUT` | — | 3.3V Reference |
| **Potentiometer** | Pin 2 (Wiper) | `D2` | `GPIO4` | Analog Input |
| **Potentiometer** | Pin 3 (GND) | `GND` | — | Common Ground |
| **Servo Motor** | Signal (Yellow/Orange) | `D8` | `GPIO8` | PWM Signal |
| **Servo Motor** | Power (Red) | `VBUS` | — | **5V Power** |
| **Servo Motor** | Ground (Brown/Black) | `GND` | — | Common Ground |
| **MAX98357A Amp** | LRC / WS (Word Select) | `D3` | `GPIO5` | I2S Bus |
| **MAX98357A Amp** | BCLK (Bit Clock) | `D4` | `GPIO6` | I2S Bus |
| **MAX98357A Amp** | DIN / DOUT (Data) | `D5` | `GPIO7` | I2S Bus |
| **MAX98357A Amp** | VIN | `VBUS` | — | **5V Power** |
| **MAX98357A Amp** | GND | `GND` | — | Common Ground |

> ⚠️ **Important:** Ensure both the Servo Red Wire and MAX98357A VIN connect to **VBUS (5V)**, while the potentiometer connects to **3.3V-OUT** to match the ESP32-C3 ADC reference voltage and prevent brownout resets during motor operation.

---

## 📂 Project Directory Structure

```text
Dragontail_ctrl/
├── Dragontail_ctrl.ino
├── README.md
└── data/
    └── sounds/
        ├── roar01.mp3
        ├── roar02.mp3
        └── growl.mp3
```

---

## 🎧 Audio Preparation & Batch Conversion (Audacity)

To fit multiple audio tracks into the LittleFS flash partition (~1.4 MB total), audio files are converted to **MP3 format**:

1. Open **Audacity** and drag all sound tracks into the project workspace simultaneously.
2. Select all tracks (`Ctrl + A`).
3. Open Export Dialog: **File** $\rightarrow$ **Export Audio...** (`Ctrl + Shift + E`).
4. In the top section under **Export Range**, select **`Multiple files`** (Split files based on: *Tracks*).
5. Configure format options:
   * **Format:** `MP3 Files`
   * **Bit Rate Mode:** `Constant` (128 kbps) or `Variable`
   * **Channels:** `Mono` or `Stereo`
6. Set destination folder to your sketch's `data/sounds/` directory.
7. Click **Export**.

---

## ⚡ Uploading Audio Files to ESP32-C3 LittleFS

1. In **Arduino IDE 2.x**, install the **`arduino-littlefs-upload`** plugin (`.vsix`).
2. Verify all `.mp3` files are located inside `data/sounds/`.
3. **Close the Serial Monitor** (or restart Arduino IDE) to release the COM port lock.
4. Open Command Palette (`Ctrl + Shift + P` or `Cmd + Shift + P` on Mac).
5. Select **`Upload LittleFS to ESP32`**.
6. Wait for the upload process to report `Success`.

---

## ⚙️ Software Configuration & Features (v2.4)

### Key Parameters in `Dragontail_ctrl.ino`

```cpp
const int MAX_SPEED_PERCENT = 50;  // Maximum servo speed limit (0 to 100%)
const int DEADBAND          = 50;  // ADC deadband around center position (2048)
const int STOP_PULSE        = 1500; // Microsecond pulse for continuous servo stop
```

### Core Logic Features

1. **Change-Triggered Telemetry with Hysteresis:** Serial Monitor output triggers only when the speed percentage changes by **more than 3 percentage points** (`abs(speedPercent - lastReportedPercent) > 3`), keeping output clean during steady movement or minor ADC noise.
2. **Speed Scaling:** Configured max speed limit is set to **50%**, mapping full pot deflection symmetrically to $\pm 50\%$ PWM pulse offsets ($\pm 250\mu s$ from stop pulse).
3. **Non-Blocking Architecture:** Audio decoding and servo loop timing run asynchronously without `delay()` calls, preventing motor stutter during audio streaming.

---

## 📖 Appendix: Troubleshooting & Technical Reference

### 1. No Output in Serial Monitor
* Ensure **Tools** $\rightarrow$ **USB CDC On Boot** is set to **`Enabled`** in Arduino IDE.
* Set baud rate to **`115200 baud`**.

### 2. "ERROR: No port specified, check IDE menus"
* Close **Serial Monitor** in Arduino IDE.
* Re-select the active COM port in the top dropdown bar (**Select other board and port...**).
* Restart Arduino IDE to completely reset port binding.

### 3. Audio Stutter or Distortion
* Confirm files are valid `.mp3` format placed in `data/sounds/`.
* Verify power connections; ensure MAX98357A and continuous servo share a common ground with the ESP32-C3.