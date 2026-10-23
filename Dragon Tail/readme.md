# 🐉 Dragon Tail Halloween Prop Controller (v2.0)

An ESP32-C3 based continuous hardware controller designed for an animatronic or wearable Dragon Tail Halloween prop. Features dual-zone continuous rotation (360°) servo motion paired with hardware I2S digital audio streaming directly from internal LittleFS storage.

---

## 🛠️ Hardware Requirements

* **Microcontroller:** Seeed Studio XIAO ESP32-C3
* **Servo Motor:** SG90 360° Continuous Rotation Servo
* **Audio DAC:** MAX98357A I2S 3.2W Amplifier / Line-Out Board
* **Input:** 10kΩ Linear Potentiometer
* **Speaker:** 4Ω / 8Ω Passive Speaker or Portable Aux Speaker
* **Power Source:** 5V USB Power Supply / Power Bank (via Microcontroller VBUS)

---

## 📌 Wiring Diagram & Pinout

All pins are assigned to ensure separation between ADC sampling, hardware PWM, and I2S audio data channels:

```text
                  +-----------------------------------+
                  |   Seeed Studio XIAO ESP32-C3      |
                  |                                   |
[3.3V Rail] <-----+ 3V3                     (5V VBUS) +-----> [5V Rail]
[GND Rail]  <-----+ GND                         GND   +-----> [GND Rail]
                  |                                   |
                  | D0/GPIO2                          |
                  | D1/GPIO3                 D10/CS   |
[Pot. Wiper] <----+ D2/GPIO4/A2            D9/GPIO9   |
[I2S LRC/WS] <----+ D3/GPIO5/A3            D8/GPIO8   |
[I2S BCLK]   <----+ D4/GPIO6/SDA         D7/GPIO21    +-----> [Servo Signal]
[I2S DOUT]   <----+ D5/GPIO7/SCL         D6/GPIO20    |
                  +-----------------------------------+
```

| Component Wire | XIAO ESP32-C3 Pin | Header Location | Description |
| :--- | :--- | :--- | :--- |
| **Potentiometer Outer 1** | **`3.3V`** | Left Side (1st down) | Potentiometer Power (3.3V) |
| **Potentiometer Wiper (Middle)** | **`D2`** | Left Side (3rd down) | Analog Input (`GPIO4`) |
| **Potentiometer Outer 2** | **`GND`** | Right Side (2nd down) | Common Ground |
| **Servo Signal (Yellow/Orange)** | **`D7` / `GPIO21`** | Right Side (7th down) | Servo PWM Signal (`D10` alias) |
| **Servo Power (Red)** | **`5V` / `VBUS`** | Right Side (1st down) | 5V Main Power Supply |
| **Servo Ground (Black/Brown)** | **`GND`** | Right Side (2nd down) | Common Ground |
| **MAX98357A LRC (WS)** | **`D3`** | Left Side (4th down) | I2S Word Select (`GPIO5`) |
| **MAX98357A BCLK** | **`D4`** | Left Side (5th down) | I2S Bit Clock (`GPIO6`) |
| **MAX98357A DOUT** | **`D5`** | Left Side (6th down) | I2S Serial Data (`GPIO7`) |
| **MAX98357A VIN** | **`5V` / `VBUS`** | Right Side (1st down) | 5V Audio Amplifier Power |
| **MAX98357A GND** | **`GND`** | Right Side (2nd down) | Common Ground |

> ⚠️ **Important:** Connect both the **Servo Red Wire** and **MAX98357A VIN** to **5V / VBUS**, not 3.3V. Servos and amplifiers require 5V to prevent microcontroller brownouts during motor operation.

---

## 📂 LittleFS Audio Directory Structure

Before uploading firmware, place your sound files inside a `data/sounds/` directory in your Arduino sketch folder:

```text
DragonTailProp/
  ├── DragonTailProp.ino
  └── data/
      └── sounds/
          ├── roar_01.wav
          ├── roar_02.wav
          ├── growl.wav
          └── hiss.wav
```

### 🔊 Recommended Audio Format
To achieve clean audio quality while preserving LittleFS flash memory space:
* **Format:** Uncompressed PCM `.wav`
* **Sample Rate:** `22,050 Hz`
* **Channels:** `Mono` (1 Channel)
* **Bit Depth:** `16-bit`

---

## ⚙️ How It Works

1. **Parallel Operation:** Audio streaming (`audio.loop()`) runs continuously in the background via hardware DMA channels, ensuring 50 Hz PWM servo updates (`myServo.writeMicroseconds()`) execute simultaneously without motor stutter.
2. **Round-Robin Sound Loop:** The firmware automatically scans `/sounds/`, plays each `.wav` file sequentially, and rewinds back to the beginning upon reaching the end of the folder.
3. **Calibrated Dual-Zone Servo Speed:**
   * **Full Left:** Tail wags **Left (CCW)** at maximum speed (`1200µs` pulse).
   * **Center (5kΩ Midpoint / ADC 2048):** **OFF / STOPPED** (`1500µs` pulse with deadband).
   * **Full Right:** Tail wags **Right (CW)** at maximum speed (`1800µs` pulse).

---

## 🚀 Software Installation

1. **Required Arduino Libraries:**
   * **ESP32Servo** by Kevin Harrington
   * **ESP32-audioI2S** by Wolle (wollewald)
2. **Board Configuration:**
   * Select **XIAO_ESP32C3** under `Tools -> Board -> ESP32 Arduino`.
   * Set Partition Scheme to **Default 4MB with spiffs/littlefs**.
3. **Upload Steps:**
   * Upload audio assets using **Tools -> ESP32 LittleFS Data Upload**.
   * Flash `DragonTailProp.ino` to the board and open the Serial Monitor at **115200 baud**.

---

## 📜 License

MIT License. Free to use and modify for personal Halloween prop projects!