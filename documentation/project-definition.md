# Project Definition — Motorized Window Shade Controller

**ECET230 Prototyping Class | NJIT**  
**Last Updated: February 2026**

---

## 1. Project Overview

### 1.1 Problem Statement

Manual window shades require physical interaction to open and close. This creates inconvenience (getting up to adjust), energy waste (shade stays closed when natural light is available), and poor sleep/wake cycles (no gradual light adjustment in the morning).

### 1.2 Proposed Solution

A retrofit motorized controller for standard roller shades. Uses an ESP32 microcontroller to drive a stepper motor that rolls the shade up and down. Three control methods: physical buttons (manual), ambient light sensor (automatic), and WiFi web interface (phone).

### 1.3 Project Scope

**In Scope:**
- Motor-driven roller shade open/close
- Physical button control (Up, Down, Mode)
- Automatic light-based control with configurable thresholds
- WiFi web interface for phone control
- LED status indicators
- Position memory across power cycles
- Safety limit switches

**Out of Scope:**
- Voice assistant integration (Alexa/Google Home)
- Multiple shade synchronization
- Custom PCB design (breadboard prototype only)
- Weatherproof/outdoor enclosure
- Scheduling/timer functionality (future enhancement)

---

## 2. Functional Requirements

| ID | Requirement | Priority | Verification |
|----|-------------|----------|-------------|
| FR-01 | System shall roll shade fully open from closed position | Critical | Motor test: full travel in <60 seconds |
| FR-02 | System shall roll shade fully closed from open position | Critical | Motor test: full travel in <60 seconds |
| FR-03 | UP button shall move shade upward while pressed | Critical | Button hold test |
| FR-04 | DOWN button shall move shade downward while pressed | Critical | Button hold test |
| FR-05 | MODE button shall toggle between Manual and Auto modes | High | Mode toggle test |
| FR-06 | In Auto mode, shade shall open when ambient light exceeds 300 lux | High | Light source test with lux meter |
| FR-07 | In Auto mode, shade shall close when ambient light drops below 50 lux | High | Dark room test |
| FR-08 | Web UI shall display current shade position (0-100%) | High | Browser inspection |
| FR-09 | Web UI shall provide Open/Close/Stop controls | High | Remote control test |
| FR-10 | Web UI shall allow setting shade to arbitrary position (slider) | Medium | Slider input test |
| FR-11 | Web UI shall display current mode and light sensor reading | Medium | Live data verification |
| FR-12 | LEDs shall indicate current mode (blue=manual, green=auto) | Medium | Visual inspection |
| FR-13 | LEDs shall indicate motor movement (white pulse) | Medium | Visual inspection |
| FR-14 | Shade position shall persist across power cycles | High | Power cycle test |
| FR-15 | Motor shall stop when top limit switch is triggered | Critical | Limit switch test |
| FR-16 | Motor shall stop when bottom limit switch is triggered | Critical | Limit switch test |

---

## 3. Non-Functional Requirements

| ID | Requirement | Target | Verification |
|----|-------------|--------|-------------|
| NFR-01 | WiFi connection time | < 10 seconds | Timer from boot |
| NFR-02 | Button response latency | < 50 ms | Perceived instantaneous |
| NFR-03 | Web UI update rate | Every 2 seconds | Network inspector |
| NFR-04 | Motor noise level | < 50 dB at 1 meter | Qualitative (quiet room acceptable) |
| NFR-05 | Power consumption (idle) | < 500 mA at 5V | Multimeter measurement |
| NFR-06 | Operating temperature | 0°C to 45°C (indoor) | Ambient room conditions |
| NFR-07 | Position accuracy | ±2% of full travel | Repeated positioning test |
| NFR-08 | NVS write endurance | > 100,000 cycles | ESP32 spec (not tested) |

---

## 4. Hardware Specification

### 4.1 Component List

| Component | Part Number / Model | Quantity | Specifications |
|-----------|-------------------|----------|----------------|
| Microcontroller | ESP32 DevKit V1 | 1 | 240 MHz dual-core, WiFi 802.11 b/g/n, BLE 4.2, 520 KB SRAM, 4 MB Flash |
| Stepper Motor | 28BYJ-48 | 1 | 5V, 4-phase, 2048 steps/rev, ~3.4 N·cm torque |
| Motor Driver | ULN2003 breakout | 1 | 7-channel Darlington array, 500 mA per channel |
| Light Sensor | BH1750FVI | 1 | I2C, 1-65535 lux range, 1 lux resolution |
| LED Strip | WS2812B | 4 pixels | 5V, 60 mA/pixel max, single-wire protocol |
| Buttons | Momentary pushbutton | 3 | 12mm, normally open |
| Limit Switches | Micro switch with lever | 2 | Normally closed, rated 5A |
| Resistors | 10kΩ | 2 | 1/4W, pull-ups for limit switches |
| Power | USB 5V adapter | 1 | 5V 2A minimum |
| Breadboard | Full-size solderless | 1 | 830 tie points |
| Wires | Dupont jumper wires | ~30 | Male-male and male-female |

### 4.2 Wiring Diagram

See `docs/design-decision-document.md` Section 1 for full block diagram and pin assignments.

### 4.3 Power Budget

| Component | Current Draw | Notes |
|-----------|-------------|-------|
| ESP32 (WiFi active) | ~240 mA | Average with WiFi transmitting |
| 28BYJ-48 (running) | ~200 mA | All 4 coils energized |
| BH1750 | ~1 mA | Continuous measurement mode |
| WS2812B (4 LEDs, mid brightness) | ~80 mA | 20 mA × 4 at 33% |
| **Total (peak)** | **~521 mA** | Within 2A USB adapter headroom |
| **Total (idle, motor off)** | **~321 mA** | Motor de-energized when stopped |

---

## 5. Software Specification

### 5.1 Development Environment

| Tool | Version | Purpose |
|------|---------|---------|
| Arduino IDE | 2.x | Code editor and uploader |
| ESP32 Board Package | Latest | ESP32 compiler and tools |
| AccelStepper Library | 1.64+ | Non-blocking stepper control |
| BH1750 Library | 1.3+ | Light sensor I2C communication |
| Adafruit NeoPixel | 1.12+ | WS2812B LED control |
| WebServer (built-in) | — | ESP32 HTTP server |
| Preferences (built-in) | — | NVS non-volatile storage |

### 5.2 Software Architecture

```
┌──────────────────────────────────────────────┐
│                  Main Loop                    │
│                                              │
│  ┌──────────┐  ┌──────────┐  ┌───────────┐  │
│  │  Button   │  │  Motor   │  │  WiFi     │  │
│  │  Handler  │  │  Control │  │  Server   │  │
│  │           │  │  (Accel  │  │  Handler  │  │
│  │  Read     │  │  Stepper)│  │           │  │
│  │  Debounce │  │  run()   │  │  Parse    │  │
│  │  Action   │  │  moveTo()│  │  Respond  │  │
│  └─────┬─────┘  └────┬─────┘  └─────┬─────┘  │
│        │             │              │         │
│        └──────┬──────┴──────┬───────┘         │
│               │             │                 │
│        ┌──────▼──────┐  ┌───▼────────┐        │
│        │  State      │  │  Sensor    │        │
│        │  Machine    │  │  Manager   │        │
│        │             │  │            │        │
│        │  STOPPED    │  │  BH1750    │        │
│        │  MOVING_UP  │  │  Read Lux  │        │
│        │  MOVING_DOWN│  │  Auto Mode │        │
│        └──────┬──────┘  └───┬────────┘        │
│               │             │                 │
│        ┌──────▼─────────────▼──────┐          │
│        │       LED Manager         │          │
│        │  Update NeoPixels based   │          │
│        │  on current state/mode    │          │
│        └───────────────────────────┘          │
│                                              │
│        ┌───────────────────────────┐          │
│        │     NVS Storage           │          │
│        │  Save/load position       │          │
│        └───────────────────────────┘          │
└──────────────────────────────────────────────┘
```

### 5.3 Control Modes

**Manual Mode:**
- UP button → motor moves shade up (incremental while held)
- DOWN button → motor moves shade down (incremental while held)
- Release button → motor stops
- Web UI buttons → move to full open/close or specific position

**Auto Mode:**
- Light sensor reads ambient lux every 5 seconds
- If lux > 300 for 3 consecutive readings → open shade
- If lux < 50 for 3 consecutive readings → close shade
- Hysteresis prevents oscillation near thresholds
- Manual buttons still work as override

### 5.4 Web API

| Endpoint | Method | Parameters | Response |
|----------|--------|-----------|----------|
| `/` | GET | — | HTML UI page |
| `/api/status` | GET | — | JSON: position, mode, lux, state |
| `/api/move` | GET | `dir=up\|down\|stop` | JSON: ok |
| `/api/position` | GET | `value=0-100` | JSON: ok (sets position %) |
| `/api/mode` | GET | `mode=manual\|auto` | JSON: ok |
| `/api/calibrate` | GET | — | JSON: ok (reset position to 0) |

### 5.5 LED Behavior

| State | Pixel 1 | Pixel 2 | Pixel 3 | Pixel 4 |
|-------|---------|---------|---------|---------|
| Manual, Stopped | Blue | Blue | Off | WiFi indicator |
| Auto, Stopped | Green | Green | Off | WiFi indicator |
| Moving Up | White (pulsing) | White (pulsing) | Up arrow (green) | WiFi indicator |
| Moving Down | White (dim pulse) | White (dim pulse) | Down arrow (red) | WiFi indicator |
| WiFi Connected | — | — | — | Solid Cyan |
| WiFi Disconnected | — | — | — | Blinking Red |
| Limit Reached | Yellow | Yellow | Yellow | WiFi indicator |

---

## 6. Mechanical Considerations

### 6.1 Motor Mounting
- Motor attaches to existing shade roller tube via 3D-printed or friction-fit adapter
- ULN2003 driver board mounts adjacent on bracket or adhesive
- Motor shaft couples to shade roller (direct drive or belt)

### 6.2 Limit Switch Placement
- Top limit switch: activated when shade is fully open (roller at max position)
- Bottom limit switch: activated when shade is fully closed (weighted bar at bottom rail)
- Both normally closed — opening the switch signals limit reached

### 6.3 Enclosure
- Breadboard prototype: open-air on shelf/windowsill
- Future: 3D-printed enclosure for ESP32 + driver board
- Ventilation holes for motor driver heat dissipation

---

## 7. Testing Plan

| Test | Procedure | Pass Criteria |
|------|-----------|---------------|
| Motor Direction | Press UP button, verify shade moves up | Shade opens |
| Full Travel | Open from closed, measure time | < 60 seconds |
| Position Memory | Open to 50%, power cycle, check position | Resumes at 50% |
| Light Auto-Open | Shine flashlight on sensor in Auto mode | Shade opens within 15 seconds |
| Light Auto-Close | Cover sensor in Auto mode | Shade closes within 15 seconds |
| WiFi Connect | Power on, check serial for IP | Connected < 10 seconds |
| Phone Control | Open phone browser to ESP32 IP, press buttons | Shade responds |
| Limit Top | Run motor up until limit switch triggers | Motor stops, no stalling |
| Limit Bottom | Run motor down until limit switch triggers | Motor stops, no stalling |
| Stress Test | 50 full open/close cycles | No failures, no overheating |
| Button Override | In Auto mode, press button | Manual control works |

---

## 8. Known Limitations & Future Enhancements

### Current Limitations
- Single shade only (no multi-shade sync)
- Local network only (no cloud/remote access)
- No scheduling (time-based open/close)
- Breadboard prototype (not production-hardened)
- 2.4GHz WiFi only (ESP32 limitation)

### Future Enhancements
- OTA (over-the-air) firmware updates
- MQTT integration for Home Assistant / smart home platforms
- Sunrise/sunset scheduling via NTP time sync
- Multiple shade coordination
- Custom PCB for permanent installation
- 3D-printed enclosure and motor mount
- Battery backup with sleep mode
