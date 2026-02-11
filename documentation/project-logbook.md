# Project Logbook — Motorized Window Shade Controller

**ECET230 Prototyping Class | NJIT**

---

## Week 1 — Project Selection & Initial Research

**Date:** [Insert start date]

### What I Did
- Selected motorized window shade as project topic
- Researched existing DIY motorized shade projects online
- Identified core requirements: motor control, phone connectivity, light sensing, physical buttons, LED indicators

### Design Decisions Made
- Chose ESP32 over Arduino Uno for built-in WiFi/BLE (eliminates need for separate WiFi shield)
- Decided on 28BYJ-48 stepper motor — lightweight shade doesn't need NEMA 17 torque, and 5V operation simplifies power design
- Selected WiFi web server approach over BLE for phone control (no app install needed, better for demos)

### Evaluation / Thought Process
The main tradeoff was between simplicity (Arduino + BLE) and functionality (ESP32 + WiFi). ESP32 wins because the web UI makes it accessible from any device on the network without installing an app. The cost difference is negligible (~$8 vs ~$5).

For the motor, I considered NEMA 17 but it requires 12V power supply and current-limiting driver (A4988). The 28BYJ-48 runs on the same 5V USB supply as the ESP32. For a standard roller shade (~200g), 3.4 N·cm of torque is sufficient.

### Issues / Blockers
- None this week

---

## Week 2 — Stepper Motor Prototyping

**Date:** [Insert date]

### What I Did
- Wired 28BYJ-48 stepper motor to Arduino with ULN2003 driver
- Wrote initial motor control code using `Stepper.h` library
- Added two buttons (UP/DOWN) with internal pull-ups
- Tested basic CW/CCW rotation with button hold-to-move

### Code Snapshot (Working)
```cpp
#include <Stepper.h>
Stepper myStepper(250, 8, 10, 9, 11);
// Buttons on pin 6 and 7 with INPUT_PULLUP
// Step 10 at a time while button held
```

### Evaluation / Thought Process
The `Stepper.h` library works but has a critical limitation: `myStepper.step(10)` is **blocking**. While the motor executes those 10 steps, nothing else can run — no button reads, no sensor checks, no WiFi handling. This will be a problem when I add more features.

Found AccelStepper library online — it uses non-blocking `run()` calls and has built-in acceleration/deceleration. Decided to migrate to AccelStepper before adding more complexity.

### Issues / Blockers
- Motor gets warm when holding position. Need to de-energize coils when stopped.
- Need to determine total steps for full shade travel (calibration needed)

---

## Week 3 — AccelStepper Migration & Position Tracking

**Date:** [Insert date]

### What I Did
- Replaced `Stepper.h` with `AccelStepper` library
- Implemented non-blocking motor control with `moveTo()` and `run()`
- Added position tracking using `currentPosition()`
- Calibrated MAX_POSITION by counting revolutions to fully open shade

### Key Changes
- `myStepper.step(10)` → `stepper.moveTo(targetPosition)` + `stepper.run()` in loop
- Motor now accelerates/decelerates smoothly (setAcceleration = 50.0)
- Buttons now use `moveTo()` for incremental positioning while held

### Evaluation / Thought Process
Non-blocking control is a game-changer. The main loop now runs freely — I tested button response time and it's instant even while the motor is moving. This architecture supports adding WiFi and sensor reads without any timing conflicts.

Calibration process: held UP button while counting revolutions. My test shade needed approximately 5 full revolutions = 5 × 2048 = ~10,240 steps. Set MAX_POSITION to 10,000 with some margin.

### Issues / Blockers
- Position resets to 0 on power cycle. Need to implement NVS storage (will do when moving to ESP32).

---

## Week 4 — Adding Sensors & LEDs (Arduino Stage)

**Date:** [Insert date]

### What I Did
- Added BH1750 light sensor via I2C (SDA/SCL)
- Implemented auto mode: shade opens when lux > 300, closes when lux < 50
- Added hysteresis band to prevent rapid open/close cycling near thresholds
- Wired 4x WS2812B NeoPixels on single data pin
- Implemented LED color scheme for mode/state indication
- Added MODE button to toggle between Manual and Auto

### Evaluation / Thought Process
Light sensor thresholds needed tuning. Initial values (100/20 lux) were too sensitive — shade would react to clouds passing. Increased to 300/50 lux with a 5-second check interval. Added hysteresis so the shade must cross the threshold by a margin before triggering (prevents oscillation).

NeoPixel choice validated — 4 LEDs on 1 pin vs. 12 pins for 4 individual RGB LEDs. Library handles timing-critical protocol internally.

### Issues / Blockers
- BH1750 occasionally returns 0 lux on first read. Added a 100ms delay after initialization.
- NeoPixel data line needs to be close to ESP32 pin — long wires cause signal degradation.

---

## Week 5 — ESP32 Migration & WiFi Integration

**Date:** [Insert date]

### What I Did
- Migrated all hardware from Arduino to ESP32 DevKit V1
- Updated pin assignments (Arduino pins → ESP32 GPIOs)
- Implemented WiFi web server with HTML/CSS/JS UI
- Added REST API endpoints: /api/status, /api/move, /api/mode, /api/position
- Implemented NVS position storage using Preferences library
- Position now persists across power cycles

### Pin Migration Table
| Function | Arduino Pin | ESP32 GPIO |
|----------|------------|------------|
| Stepper IN1-IN4 | 8,10,9,11 | 19,18,5,17 |
| Button UP | 7 | 32 |
| Button DOWN | 6 | 33 |
| Button MODE | — | 25 |
| NeoPixel | — | 16 |
| I2C SDA | A4 | 21 |
| I2C SCL | A5 | 22 |

### Evaluation / Thought Process
ESP32 migration was mostly straightforward since AccelStepper and NeoPixel libraries are ESP32-compatible. The main gotchas:
1. GPIO 34/35 (limit switches) are input-only with NO internal pull-up — needed external 10kΩ resistors to 3.3V
2. ESP32 WiFi only supports 2.4GHz — had to make sure my test network wasn't 5GHz-only
3. Some ESP32 USB cables are charge-only (no data). Wasted 20 minutes troubleshooting before swapping cables.

Web server UI built with embedded HTML in the sketch. AJAX calls update the page without refresh. Phone can control shade from anywhere on the local network.

### Issues / Blockers
- Web UI occasionally disconnects during motor movement (WiFi task competes with motor timing). Added `yield()` calls in motor loop.

---

## Week 6 — Integration Testing & Refinement

**Date:** [Insert date]

### What I Did
- Added limit switches (top/bottom) for safety stops
- Full system integration test: buttons + WiFi + auto mode + LEDs + position memory
- Stress tested: 50 full open/close cycles without failure
- Tuned motor speed and acceleration for smooth operation
- Documented final pin assignments and wiring

### Evaluation / Thought Process
Limit switches are essential safety feature. Without them, a software bug could drive the motor past physical limits and damage the shade or mechanism. Hardware stops work regardless of software state.

Final system meets all requirements:
- ✅ Motor control (up/down with position tracking)
- ✅ Phone control (WiFi web UI)
- ✅ Light sensor (auto open/close)
- ✅ Physical buttons (manual override)
- ✅ LED indicators (mode/state feedback)
- ✅ Position memory (NVS persistence)
- ✅ Safety (limit switches)

### Final Reflections
Biggest lesson: start with non-blocking architecture from the beginning. The AccelStepper migration early on saved massive headaches when adding WiFi and sensors. If I'd kept the blocking `Stepper.h` library, the whole system would need restructuring.

If I were to do this again, I'd add OTA (over-the-air) firmware updates so I don't have to plug in USB every time I tweak the code. The ESP32 supports this natively.
