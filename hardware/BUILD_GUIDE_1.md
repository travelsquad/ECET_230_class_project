# Motorized Window Shade — Step-by-Step Build Guide

## The Staged Approach (Don't Skip Ahead!)

This project is built in 3 stages. Each one builds on the last. **Don't move to the next stage until the current one works.** This way, if something breaks, you know exactly what caused it.

---

## STAGE 1: AccelStepper + Buttons (on your Arduino)

### What You're Testing
Your existing stepper motor and buttons, but switched to the AccelStepper library for smooth acceleration and non-blocking movement.

### What You Need
- Everything you already have wired up (no changes!)
- Install **AccelStepper** library: Sketch → Include Library → Manage Libraries → search "AccelStepper"

### Pin Wiring (same as your current setup)
```
Arduino Pin 7  → Button UP (other leg to GND)
Arduino Pin 6  → Button DOWN (other leg to GND)
Arduino Pin 8  → ULN2003 IN1
Arduino Pin 9  → ULN2003 IN2
Arduino Pin 10 → ULN2003 IN3
Arduino Pin 11 → ULN2003 IN4
ULN2003 VCC    → 5V
ULN2003 GND    → GND
```

### Upload & Test
1. Open `stage1_accelstepper_buttons.ino`
2. Upload to your Arduino
3. Open Serial Monitor at **9600 baud**
4. Hold UP button — motor should spin one direction with smooth acceleration
5. Release — motor decelerates and stops
6. Hold DOWN button — motor spins the other direction
7. Watch the Serial Monitor for position readings

### Key Calibration Step
**This is important for the whole project:**

1. Position your shade at the CLOSED position
2. Power on the Arduino (it starts at position 0)
3. Hold the UP button and **count how many full motor revolutions** it takes to fully open your shade
4. Multiply that by **2048** (steps per revolution for 28BYJ-48)
5. Update `MAX_POSITION` in the code with that number

Example: 5 revolutions × 2048 = 10,240 steps → `const long MAX_POSITION = 10240;`

### If Motor Spins Wrong Direction
Your UP button makes the shade go down? Just swap the button pin assignments:
```cpp
const int BTN_UP   = 6;   // Was 7
const int BTN_DOWN = 7;   // Was 6
```

### If Motor Vibrates But Won't Turn
The pin order matters for the ULN2003. Try this sequence instead:
```cpp
AccelStepper stepper(MotorInterfaceType, 8, 10, 9, 11);  // Current
AccelStepper stepper(MotorInterfaceType, 8, 9, 10, 11);  // Try this
```

### ✅ Stage 1 is done when:
- [ ] Motor moves smoothly in both directions
- [ ] Releasing button causes smooth deceleration (not a hard stop)
- [ ] Position shows in Serial Monitor
- [ ] Motor stops at MAX_POSITION and MIN_POSITION
- [ ] You've determined your MAX_POSITION value

---

## STAGE 2: Add LEDs + Light Sensor (still on Arduino)

### What's New
- NeoPixel LED strip for visual feedback
- BH1750 light sensor (or LDR as a cheaper alternative)
- Mode button to toggle between Manual and Auto

### New Libraries to Install
- **BH1750** by Christopher Laws (or skip if using LDR)
- **Adafruit NeoPixel** by Adafruit

### New Wiring (add to existing)
```
EXISTING: Keep all Stage 1 wiring the same

NEW — BH1750 Light Sensor:
  BH1750 SDA  → Arduino A4
  BH1750 SCL  → Arduino A5
  BH1750 VCC  → 3.3V (NOT 5V!)
  BH1750 GND  → GND
  BH1750 ADDR → GND (sets I2C address to 0x23)

NEW — NeoPixel Strip (4 LEDs):
  NeoPixel DIN  → Arduino Pin 3
  NeoPixel VCC  → 5V
  NeoPixel GND  → GND
  (Optional: 330Ω resistor between Pin 3 and DIN)
  (Optional: 100µF capacitor between VCC and GND of strip)

NEW — Mode Button:
  Arduino Pin 5 → Button (other leg to GND)
```

### Upload & Test
1. Open `stage2_leds_lightsensor.ino`
2. **Update MAX_POSITION** with the value from Stage 1!
3. Upload to Arduino
4. Open Serial Monitor at 9600 baud

### Test Checklist
- [ ] NeoPixels flash green on startup (3 blinks)
- [ ] Pixel 0 is blue (manual mode indicator)
- [ ] Pixel 1 changes brightness based on ambient light (cover sensor with hand)
- [ ] Pixels 2-3 pulse green when opening, orange when closing
- [ ] Serial Monitor shows light readings every 3 seconds
- [ ] Press MODE button → Pixels flash teal, mode switches to AUTO
- [ ] In AUTO mode: cover the sensor → shade closes; shine a light → shade opens
- [ ] Physical buttons override auto mode (switches back to manual)

### If BH1750 Shows "NOT FOUND"
- Check I2C wiring: SDA → A4, SCL → A5
- Make sure VCC goes to **3.3V** (not 5V) — some modules tolerate 5V but 3.3V is safer
- Make sure ADDR pin is connected to GND
- Try: Sketch → Include Library → Manage Libraries → reinstall BH1750

### If Using an LDR Instead of BH1750
See the bottom of the Stage 2 code file — there's a complete LDR alternative section with wiring and code changes.

### Tuning Light Thresholds
Watch the Serial Monitor to see what lux values you get in different conditions:
- Room with lights on: probably 200-500 lux
- Room with lights off: probably 5-30 lux
- Direct sunlight through window: 1000+ lux
- Adjust `LUX_OPEN_ABOVE` and `LUX_CLOSE_BELOW` accordingly

### ✅ Stage 2 is done when:
- [ ] All Stage 1 functions still work
- [ ] NeoPixels show meaningful status
- [ ] Light sensor gives reasonable readings
- [ ] Auto mode opens/closes based on light
- [ ] Mode button toggles correctly
- [ ] You've noted your ideal lux thresholds

---

## STAGE 3: Move to ESP32 + WiFi (Final Build)

### What's New
- Runs on ESP32 instead of Arduino
- WiFi web server — control from any phone/laptop browser
- Position saved to flash memory (survives power cycles)
- Limit switch support (optional)

### ESP32 Board Setup (One-Time)
1. Arduino IDE → **File → Preferences**
2. In "Additional Board Manager URLs" paste:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. **Tools → Board → Board Manager** → search "esp32" → Install "esp32 by Espressif"
4. **Tools → Board** → select "ESP32 Dev Module"
5. Connect ESP32 via USB, select port in **Tools → Port**

### ⚠️ ESP32 PINS ARE DIFFERENT FROM ARDUINO!
This is the #1 thing that trips people up. You need to rewire.

```
ARDUINO PIN    →    ESP32 PIN         NOTES
═══════════════════════════════════════════════════════
Stepper IN1: 8     →  GPIO 19
Stepper IN2: 9     →  GPIO 18
Stepper IN3: 10    →  GPIO 5
Stepper IN4: 11    →  GPIO 17
Button UP: 7       →  GPIO 32         Has internal pull-up ✓
Button DOWN: 6     →  GPIO 33         Has internal pull-up ✓
Button MODE: 5     →  GPIO 25         Has internal pull-up ✓
BH1750 SDA: A4     →  GPIO 21         ESP32 default I2C SDA
BH1750 SCL: A5     →  GPIO 22         ESP32 default I2C SCL
NeoPixel: 3        →  GPIO 16
Limit Top: --      →  GPIO 34         INPUT ONLY, needs external pull-up!
Limit Bottom: --   →  GPIO 35         INPUT ONLY, needs external pull-up!
```

### ESP32 vs Arduino Gotchas
1. **Serial baud rate**: ESP32 uses `115200`, not `9600`. Change your Serial Monitor!
2. **GPIO 34/35**: These pins are INPUT ONLY and do NOT have internal pull-ups. If you wire limit switches here, add a 10kΩ resistor from the pin to 3.3V.
3. **5V pin**: ESP32's 5V pin outputs whatever USB provides (~5V). This powers the ULN2003 and NeoPixels fine.
4. **3.3V logic**: ESP32 GPIO is 3.3V, but the ULN2003, BH1750, and WS2812B all work with 3.3V logic levels.
5. **WiFi is 2.4 GHz ONLY**: ESP32 does not connect to 5 GHz networks. Make sure your phone's hotspot or router has 2.4 GHz enabled.

### Configuration Before Upload
Open `stage3_esp32_wifi.ino` and update these lines:

```cpp
// Line ~28-29: Your WiFi credentials
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Line ~32: Your calibrated value from Stage 1
const long MAX_POSITION = 10240;   // <-- Your number here

// Line ~40-41: Your thresholds from Stage 2
const float LUX_OPEN_ABOVE  = 300.0;   // <-- Your value
const float LUX_CLOSE_BELOW = 50.0;    // <-- Your value
```

### Upload & Test
1. Upload to ESP32
2. Open Serial Monitor at **115200** baud
3. Watch for the WiFi connection message — it'll print an IP address like:
   ```
   ╔══════════════════════════════════════╗
   ║  WiFi Connected!                     ║
   ║  Open in your phone browser:         ║
   ║  http://192.168.1.42                 ║
   ╚══════════════════════════════════════╝
   ```
4. Open that IP address in your phone's browser (phone must be on same WiFi!)
5. Test all the web controls

### Test Checklist
- [ ] ESP32 connects to WiFi (green LED flash on startup)
- [ ] IP address appears in Serial Monitor
- [ ] Web page loads on phone
- [ ] Open/Close/Stop buttons work from phone
- [ ] Position slider works
- [ ] Physical buttons still work
- [ ] Mode toggle works on both web and physical button
- [ ] Light sensor value shows on web page
- [ ] Auto mode works via web toggle
- [ ] Unplug ESP32, plug back in → position is remembered

### WiFi Troubleshooting
| Problem | Solution |
|---------|----------|
| Won't connect to WiFi | Double-check SSID/password spelling. SSID is case-sensitive! |
| Connected but page won't load | Phone MUST be on same WiFi network as ESP32 |
| Keeps disconnecting | Try a different USB cable (some are charge-only). Try external power supply. |
| Can't find IP address | Serial Monitor must be at 115200 baud |
| Works on laptop but not phone | Make sure phone is on 2.4 GHz, not 5 GHz |

### Optional: Limit Switches
If you wire limit switches:
1. Change `#define USE_LIMIT_SWITCHES false` to `true` in the code
2. Wire each switch: one terminal to GPIO 34/35, other terminal to GND
3. Add 10kΩ resistor from GPIO 34 to 3.3V (pull-up)
4. Add 10kΩ resistor from GPIO 35 to 3.3V (pull-up)
5. When the switch is pressed, it reads LOW (0)

### ✅ Stage 3 is done when:
- [ ] Everything from Stages 1 & 2 works
- [ ] WiFi control works from your phone
- [ ] Position persists after power cycle
- [ ] Auto mode works from both web and button
- [ ] No crashes or resets during extended operation

---

## Bonus: Features to Add for Extra Credit

If everything is working and you want to go further:

1. **OTA Updates**: Upload code over WiFi instead of USB cable
   - Add `#include <ArduinoOTA.h>` and call `ArduinoOTA.handle()` in loop
   
2. **Temperature sensor (BME280)**: Show room temp on the web UI
   - Same I2C bus as BH1750, just chain the wires
   
3. **Scheduling**: Open shade at 7am, close at 9pm
   - Use NTP to get current time: `configTime(-5*3600, 0, "pool.ntp.org")`
   
4. **mDNS**: Access via `http://shade.local` instead of IP address
   - `#include <ESPmDNS.h>` → `MDNS.begin("shade")`
   
5. **Multiple shade sync**: If you build more than one unit, they can coordinate via HTTP calls to each other

---

## Parts List (Complete)

| Qty | Component | Where to Buy | ~Price |
|-----|-----------|-------------|--------|
| 1 | ESP32 DevKit V1 | Amazon/AliExpress | $8 |
| 1 | 28BYJ-48 + ULN2003 | Amazon (usually bundled) | $4 |
| 1 | BH1750 breakout board | Amazon/AliExpress | $3 |
| 4 | WS2812B NeoPixel (or small strip) | Amazon | $4 |
| 3 | Tactile push buttons | Any electronics kit | $1 |
| 2 | Micro limit switches | Amazon | $2 |
| 2 | 10kΩ resistors | Any electronics kit | $0.10 |
| 1 | Breadboard (half-size) | Amazon | $3 |
| ~20 | Jumper wires (M-M and M-F) | Amazon | $3 |
| 1 | USB-C cable (data, not charge-only) | -- | -- |

**Total: ~$25-30**

---

## Common Mistakes to Avoid

1. **Using a charge-only USB cable** — ESP32 won't show up as a COM port. Use a data cable.
2. **Not changing Serial Monitor baud rate** — Arduino uses 9600, ESP32 uses 115200.
3. **Connecting BH1750 VCC to 5V** — Use 3.3V.
4. **Forgetting to de-energize stepper coils** — The 28BYJ-48 will get HOT if you leave coils energized when stopped. The code handles this, but be aware.
5. **Trying to connect to 5 GHz WiFi** — ESP32 only does 2.4 GHz.
6. **Moving to Stage 3 before Stage 1 works** — Each stage validates the previous. Trust the process.
