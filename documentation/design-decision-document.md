# Design Decision Document — Motorized Window Shade Controller

**ECET230 Prototyping Class | NJIT**  
**Last Updated: February 2026**

---

## 1. System Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        SYSTEM OVERVIEW                              │
└─────────────────────────────────────────────────────────────────────┘

                         ┌───────────────┐
                         │   POWER       │
                         │   5V USB /    │
                         │   5V Adapter  │
                         └──────┬────────┘
                                │
         ┌──────────────────────┼──────────────────────┐
         │                      │                      │
         ▼                      ▼                      ▼
┌─────────────┐    ┌────────────────────┐    ┌─────────────────┐
│  ULN2003    │    │     ESP32          │    │  WS2812B        │
│  Motor      │◄───│     DevKit V1     │───►│  NeoPixel LEDs  │
│  Driver     │    │                    │    │  (4 pixels)     │
│             │    │  ┌──────────────┐  │    └─────────────────┘
│  ┌────────┐ │    │  │ WiFi Module  │  │
│  │28BYJ-48│ │    │  │ (Web Server) │  │    ┌─────────────────┐
│  │Stepper │ │    │  └──────────────┘  │◄───│  BH1750 Light   │
│  │Motor   │ │    │                    │    │  Sensor (I2C)   │
│  └────────┘ │    │  ┌──────────────┐  │    └─────────────────┘
└─────────────┘    │  │ NVS (Flash)  │  │
                   │  │ Position Mem │  │    ┌─────────────────┐
                   │  └──────────────┘  │◄───│  Buttons (x3)   │
                   │                    │    │  UP / DOWN /    │
                   └────────┬───────────┘    │  MODE           │
                            │                └─────────────────┘
                            │
                   ┌────────┴───────────┐
                   │  Limit Switches    │
                   │  TOP / BOTTOM      │
                   └────────────────────┘


          ┌──────────┐     WiFi (HTTP)     ┌──────────────┐
          │  Phone   │◄───────────────────►│  ESP32 Web   │
          │  Browser │    192.168.x.x      │  Server :80  │
          └──────────┘                     └──────────────┘
```

### Signal Flow

```
INPUTS                          PROCESSING                    OUTPUTS
────────────────               ─────────────                 ─────────────
Button UP (GPIO 32) ──┐
Button DOWN (GPIO 33)─┤        ┌───────────┐
Button MODE (GPIO 25)─┼──────►│  ESP32     │──────────►  Stepper Motor
BH1750 Lux (I2C)─────┤        │  Control   │              (GPIO 19,18,5,17)
Limit SW Top (GPIO 34)┤       │  Logic     │──────────►  NeoPixel LEDs
Limit SW Btm (GPIO 35)┘       │            │              (GPIO 16)
WiFi Commands ─────────────►  │            │──────────►  Serial Debug
                               └───────────┘              (115200 baud)
                                    │
                                    ▼
                               NVS Storage
                               (Position)
```

---

## 2. Design Decisions

### DD-01: Microcontroller Selection — ESP32 DevKit V1

**Options Considered:**
| Option | Pros | Cons |
|--------|------|------|
| Arduino Uno | Familiar, simple | No WiFi/BLE, limited pins |
| Arduino Nano 33 IoT | WiFi + BLE | More expensive, less community support |
| ESP32 DevKit V1 | WiFi + BLE, cheap, Arduino-compatible, NVS storage | 3.3V logic (vs 5V) |
| Raspberry Pi Pico W | WiFi, dual-core | MicroPython learning curve |

**Decision:** ESP32 DevKit V1  
**Rationale:** Built-in WiFi enables phone control without extra hardware. Arduino IDE compatible means existing code transfers. Non-volatile storage (NVS) for position memory is built in. Cost: ~$8.

---

### DD-02: Motor Selection — 28BYJ-48 + ULN2003

**Options Considered:**
| Option | Torque | Cost | Complexity |
|--------|--------|------|------------|
| 28BYJ-48 + ULN2003 | ~3.4 N·cm | ~$3 | Low (5V, simple wiring) |
| NEMA 17 + A4988 | ~40 N·cm | ~$15 | Medium (12V, current tuning) |
| Servo motor | Varies | ~$10 | Low but limited rotation |

**Decision:** 28BYJ-48 with ULN2003 driver  
**Rationale:** Sufficient torque for a lightweight roller shade. Runs on 5V (same as ESP32 USB power). Simple 4-wire interface. Cheap and readily available. 2048 steps/revolution gives fine position control.

---

### DD-03: Motor Library — AccelStepper (replacing Stepper.h)

**Options Considered:**
| Option | Blocking? | Acceleration | Position Tracking |
|--------|-----------|-------------|-------------------|
| Stepper.h (built-in) | Yes | No | Manual |
| AccelStepper | No | Yes | Built-in |

**Decision:** AccelStepper library  
**Rationale:** The built-in `Stepper.h` uses blocking `step()` calls — nothing else runs while the motor moves. AccelStepper's `run()` is non-blocking: buttons, LEDs, WiFi, and light sensor all stay responsive during movement. Built-in acceleration prevents sudden starts that could jam the shade mechanism. Position tracking via `currentPosition()` eliminates manual counting.

---

### DD-04: Light Sensor — BH1750 Digital Lux Sensor

**Options Considered:**
| Option | Output | Accuracy | Interface |
|--------|--------|----------|-----------|
| LDR (photoresistor) | Analog (relative) | Low | ADC pin |
| BH1750 | Digital (lux) | High (1 lux resolution) | I2C |

**Decision:** BH1750  
**Rationale:** Returns calibrated lux values (not arbitrary analog numbers), making threshold configuration intuitive. I2C uses only 2 pins (SDA/SCL). No external resistors needed. Consistent readings regardless of supply voltage.

**Fallback:** LDR on analog pin if BH1750 is unavailable. Code includes LDR alternative.

---

### DD-05: Phone Control Method — WiFi Web Server

**Options Considered:**
| Option | Requires App? | Range | Setup |
|--------|--------------|-------|-------|
| BLE (Bluetooth) | Yes (nRF Connect) | ~30ft | Simple |
| WiFi Web Server | No (browser) | Whole network | Moderate |
| MQTT + Home Assistant | Yes | Network-wide | Complex |

**Decision:** WiFi Web Server hosted on ESP32  
**Rationale:** No app installation needed — works in any phone browser. Provides a visual UI with buttons, slider, and live sensor data. More impressive for class demos. ESP32's WebServer library handles HTTP natively.

---

### DD-06: LED Feedback — WS2812B NeoPixel Strip

**Options Considered:**
| Option | Colors | Pins Required | Addressable |
|--------|--------|---------------|-------------|
| Individual LEDs | 1 each | 1 per LED | No |
| RGB LED (common anode) | Full RGB | 3 pins | No |
| WS2812B NeoPixels | Full RGB | 1 data pin | Yes |

**Decision:** WS2812B NeoPixel strip (4 pixels)  
**Rationale:** Full RGB on single data pin. Each pixel individually addressable — can show different states simultaneously. Adafruit NeoPixel library is well-documented.

**LED Color Scheme:**
| State | Color |
|-------|-------|
| Manual mode | Blue |
| Auto mode | Green |
| Moving up | White pulse |
| Moving down | Dim white pulse |
| WiFi connected | Solid indicator |
| WiFi disconnected | Red blink |
| At limit | Yellow |

---

### DD-07: Position Persistence — ESP32 NVS (Preferences Library)

**Decision:** Store current position in ESP32's non-volatile storage using the Preferences library.  
**Rationale:** Position survives power cycles. No external EEPROM needed. ESP32 NVS has ~100,000 write cycles — more than sufficient since we only write on position change (debounced).

---

### DD-08: Safety — Limit Switches on GPIO 34/35

**Decision:** Normally-closed microswitches at top and bottom of shade travel.  
**Rationale:** Hardware safety stops that don't depend on software. GPIO 34/35 are input-only pins (no internal pull-up), so 10kΩ external pull-up resistors to 3.3V are required. Motor stops immediately when limit is triggered regardless of software state.

---

## 3. Pin Assignment Summary

| GPIO | Function | Notes |
|------|----------|-------|
| 19 | Stepper IN1 | ULN2003 |
| 18 | Stepper IN2 | ULN2003 |
| 5 | Stepper IN3 | ULN2003 |
| 17 | Stepper IN4 | ULN2003 |
| 21 | I2C SDA | BH1750 |
| 22 | I2C SCL | BH1750 |
| 32 | Button UP | Internal pull-up |
| 33 | Button DOWN | Internal pull-up |
| 25 | Button MODE | Internal pull-up |
| 16 | NeoPixel Data | WS2812B |
| 34 | Limit Switch TOP | Input-only, ext pull-up |
| 35 | Limit Switch BTM | Input-only, ext pull-up |

---

## 4. Bill of Materials

| # | Component | Qty | Est. Cost |
|---|-----------|-----|-----------|
| 1 | ESP32 DevKit V1 | 1 | $8 |
| 2 | 28BYJ-48 Stepper + ULN2003 | 1 | $3 |
| 3 | BH1750 Light Sensor Module | 1 | $3 |
| 4 | WS2812B NeoPixel Strip (4 LED) | 1 | $4 |
| 5 | Momentary Push Buttons | 3 | $2 |
| 6 | Microswitch (Limit) | 2 | $3 |
| 7 | 10kΩ Resistors | 2 | $1 |
| 8 | Breadboard + Jumper Wires | 1 set | $5 |
| 9 | USB Cable (data) | 1 | $3 |
| 10 | Roller Shade (test unit) | 1 | $10 |
| | **Total** | | **~$42** |
