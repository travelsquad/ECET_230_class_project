/*
 * ============================================
 *  MOTORIZED WINDOW SHADE — STAGE 3 (FINAL)
 *  ESP32 + WiFi + AccelStepper + Everything
 * ============================================
 * 
 * This is the FINAL version. Only move to this after 
 * Stage 1 and Stage 2 work on your Arduino.
 * 
 * What's new in Stage 3:
 *  - Runs on ESP32 (pin changes from Arduino!)
 *  - WiFi web server — control from your phone's browser
 *  - Position saved to flash (survives power cycles)
 *  - Limit switch support
 *  - Nice mobile UI with slider, live status
 * 
 * ESP32 BOARD SETUP (one-time):
 *  1. Arduino IDE → File → Preferences
 *  2. "Additional Board Manager URLs" paste:
 *     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *  3. Tools → Board → Board Manager → search "esp32" → Install
 *  4. Tools → Board → "ESP32 Dev Module"
 *  5. Tools → Port → select your ESP32's COM port
 * 
 * ESP32 PIN MAPPING (different from Arduino!):
 *  Stepper:    GPIO 19, 18, 5, 17
 *  Buttons:    GPIO 32 (UP), 33 (DOWN), 25 (MODE)
 *  NeoPixels:  GPIO 16
 *  BH1750:     GPIO 21 (SDA), GPIO 22 (SCL)
 *  Limit SW:   GPIO 34 (TOP), GPIO 35 (BOTTOM) — need external pull-ups!
 * 
 * Libraries needed:
 *  - AccelStepper (same as before)
 *  - BH1750 by Christopher Laws (same as before)
 *  - Adafruit NeoPixel (same as before)
 *  - WiFi, WebServer, Preferences (built into ESP32 package)
 */

#include <AccelStepper.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  CONFIGURATION — EDIT THESE
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// WiFi — YOUR network credentials
const char* WIFI_SSID     = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// Position limits — USE THE VALUE YOU FOUND IN STAGE 1
const long MAX_POSITION = 10240;   // Steps for fully open
const long MIN_POSITION = 0;       // Fully closed

// Motor speed
const float MAX_SPEED    = 800.0;
const float ACCELERATION = 200.0;

// Light thresholds — USE VALUES YOU FOUND IN STAGE 2
const float LUX_OPEN_ABOVE  = 300.0;
const float LUX_CLOSE_BELOW = 50.0;
const unsigned long AUTO_CHECK_MS = 5000;

// NeoPixel count
#define NUM_PIXELS 4

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  ESP32 PIN DEFINITIONS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

// Stepper (ULN2003): IN1, IN3, IN2, IN4
#define STEPPER_PIN1  19
#define STEPPER_PIN2  18
#define STEPPER_PIN3  5
#define STEPPER_PIN4  17

// Buttons
#define BTN_UP    32
#define BTN_DOWN  33
#define BTN_MODE  25

// NeoPixel
#define NEOPIXEL_PIN  16

// I2C for BH1750
#define SDA_PIN  21
#define SCL_PIN  22

// Limit switches (GPIO 34/35 are INPUT ONLY, no internal pull-up)
// Wire: 10kΩ from pin to 3.3V, switch from pin to GND
#define LIMIT_TOP     34
#define LIMIT_BOTTOM  35

// Set to true if you wired limit switches, false if not
#define USE_LIMIT_SWITCHES false

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  OBJECTS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

#define MotorInterfaceType 4
AccelStepper stepper(MotorInterfaceType, STEPPER_PIN1, STEPPER_PIN3, STEPPER_PIN2, STEPPER_PIN4);

BH1750 lightSensor;
Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
WebServer server(80);
Preferences prefs;

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  STATE VARIABLES
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

enum Mode { MANUAL, AUTO_LIGHT };
Mode currentMode = MANUAL;

float currentLux = 0;
bool wifiConnected = false;

// Button tracking
bool btnUpPrev   = false;
bool btnDownPrev = false;
bool btnModePrev = false;
unsigned long lastModePress = 0;
unsigned long lastAutoCheck = 0;

// LED animation
int pulseVal = 0;
int pulseDir = 6;

// Track if motor was commanded by web (vs button)
bool webCommandActive = false;

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  SETUP
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void setup() {
  Serial.begin(115200);  // ESP32 uses 115200, not 9600
  Serial.println("\n=== Window Shade Controller — Stage 3 (ESP32) ===\n");

  // Buttons (ESP32 GPIO 32/33/25 support INPUT_PULLUP)
  pinMode(BTN_UP,   INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);

  // Limit switches
  if (USE_LIMIT_SWITCHES) {
    pinMode(LIMIT_TOP,    INPUT);  // External pull-up required
    pinMode(LIMIT_BOTTOM, INPUT);  // External pull-up required
  }

  // Stepper
  stepper.setMaxSpeed(MAX_SPEED);
  stepper.setAcceleration(ACCELERATION);

  // Load saved position from flash
  prefs.begin("shade", false);
  long savedPos = prefs.getLong("position", 0);
  int savedMode = prefs.getInt("mode", 0);
  stepper.setCurrentPosition(savedPos);
  currentMode = (savedMode == 1) ? AUTO_LIGHT : MANUAL;
  Serial.printf("Loaded: position=%ld (%d%%), mode=%s\n",
    savedPos, getPositionPercent(),
    currentMode == MANUAL ? "MANUAL" : "AUTO");

  // I2C + light sensor
  Wire.begin(SDA_PIN, SCL_PIN);
  if (lightSensor.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("BH1750 sensor: OK");
  } else {
    Serial.println("BH1750 sensor: NOT FOUND");
  }

  // NeoPixels
  pixels.begin();
  pixels.setBrightness(40);

  // WiFi
  Serial.printf("Connecting to WiFi: %s ", WIFI_SSID);
  pixels.setPixelColor(0, pixels.Color(255, 165, 0));  // Orange = connecting
  pixels.show();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\n\n╔══════════════════════════════════════╗\n");
    Serial.printf("║  WiFi Connected!                     ║\n");
    Serial.printf("║  Open in your phone browser:         ║\n");
    Serial.printf("║  http://%-28s║\n", WiFi.localIP().toString().c_str());
    Serial.printf("╚══════════════════════════════════════╝\n\n");
    flashPixels(0, 255, 0, 3);  // Green flash
  } else {
    wifiConnected = false;
    Serial.println("\nWiFi FAILED — buttons still work");
    flashPixels(255, 0, 0, 3);  // Red flash
  }

  // Web server
  setupWebServer();

  Serial.println("System ready.\n");
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  MAIN LOOP
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void loop() {
  // Handle web requests
  server.handleClient();

  // Handle physical buttons
  handleButtons();

  // Auto mode
  if (currentMode == AUTO_LIGHT && millis() - lastAutoCheck > AUTO_CHECK_MS) {
    lastAutoCheck = millis();
    handleAutoMode();
  }

  // Limit switches
  if (USE_LIMIT_SWITCHES) {
    checkLimitSwitches();
  }

  // Position limits (software)
  if (stepper.currentPosition() >= MAX_POSITION && stepper.distanceToGo() > 0) {
    stepper.stop();
    stepper.setCurrentPosition(MAX_POSITION);
    Serial.println("Reached TOP limit");
  }
  if (stepper.currentPosition() <= MIN_POSITION && stepper.distanceToGo() < 0) {
    stepper.stop();
    stepper.setCurrentPosition(MIN_POSITION);
    Serial.println("Reached BOTTOM limit");
  }

  // Run motor (non-blocking)
  stepper.run();

  // De-energize coils when stopped (saves power, reduces heat)
  if (!stepper.isRunning()) {
    deEnergizeMotor();
  }

  // Save position when motor stops
  static bool wasStopped = true;
  if (!stepper.isRunning() && !wasStopped) {
    savePosition();
    wasStopped = true;
    webCommandActive = false;
  }
  if (stepper.isRunning()) wasStopped = false;

  // Update LEDs
  updateLEDs();

  // Prevent watchdog timeout on ESP32
  yield();
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  BUTTON HANDLING
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void handleButtons() {
  bool upNow   = (digitalRead(BTN_UP)   == LOW);
  bool downNow = (digitalRead(BTN_DOWN) == LOW);
  bool modeNow = (digitalRead(BTN_MODE) == LOW);

  // Mode toggle (debounced)
  if (modeNow && !btnModePrev && millis() - lastModePress > 300) {
    lastModePress = millis();
    currentMode = (currentMode == MANUAL) ? AUTO_LIGHT : MANUAL;
    prefs.putInt("mode", (int)currentMode);
    Serial.printf("Mode: %s\n", currentMode == MANUAL ? "MANUAL" : "AUTO");
    flashPixels(currentMode == MANUAL ? 0 : 0,
                currentMode == MANUAL ? 0 : 200,
                currentMode == MANUAL ? 200 : 100, 2);
  }
  btnModePrev = modeNow;

  // UP button
  if (upNow && !downNow) {
    webCommandActive = false;
    stepper.moveTo(MAX_POSITION);
  }
  // DOWN button
  else if (downNow && !upNow) {
    webCommandActive = false;
    stepper.moveTo(MIN_POSITION);
  }
  // Released
  else if (!upNow && !downNow && !webCommandActive) {
    if (btnUpPrev || btnDownPrev) {
      stepper.stop();
      Serial.printf("Button released. Position: %d%%\n", getPositionPercent());
    }
  }

  btnUpPrev   = upNow;
  btnDownPrev = downNow;
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  AUTO MODE
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void handleAutoMode() {
  currentLux = lightSensor.readLightLevel();
  if (currentLux < 0) return;  // Read error

  if (!stepper.isRunning()) {
    if (currentLux > LUX_OPEN_ABOVE && stepper.currentPosition() < MAX_POSITION) {
      Serial.printf("Auto: %.0f lux > %.0f threshold → Opening\n", currentLux, LUX_OPEN_ABOVE);
      webCommandActive = true;  // Don't let button-release stop this
      stepper.moveTo(MAX_POSITION);
    }
    else if (currentLux < LUX_CLOSE_BELOW && stepper.currentPosition() > MIN_POSITION) {
      Serial.printf("Auto: %.0f lux < %.0f threshold → Closing\n", currentLux, LUX_CLOSE_BELOW);
      webCommandActive = true;
      stepper.moveTo(MIN_POSITION);
    }
  }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  LIMIT SWITCHES
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void checkLimitSwitches() {
  // TOP limit switch: stop if moving up
  if (digitalRead(LIMIT_TOP) == LOW && stepper.distanceToGo() > 0) {
    stepper.stop();
    stepper.setCurrentPosition(MAX_POSITION);
    Serial.println("TOP limit switch triggered");
  }
  // BOTTOM limit switch: stop if moving down
  if (digitalRead(LIMIT_BOTTOM) == LOW && stepper.distanceToGo() < 0) {
    stepper.stop();
    stepper.setCurrentPosition(MIN_POSITION);
    Serial.println("BOTTOM limit switch triggered");
  }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  HELPER FUNCTIONS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

int getPositionPercent() {
  return map(stepper.currentPosition(), MIN_POSITION, MAX_POSITION, 0, 100);
}

void savePosition() {
  prefs.putLong("position", stepper.currentPosition());
  Serial.printf("Position saved: %ld (%d%%)\n", stepper.currentPosition(), getPositionPercent());
}

void deEnergizeMotor() {
  // Turn off all stepper coils to save power and prevent heat
  digitalWrite(STEPPER_PIN1, LOW);
  digitalWrite(STEPPER_PIN2, LOW);
  digitalWrite(STEPPER_PIN3, LOW);
  digitalWrite(STEPPER_PIN4, LOW);
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  LED STATUS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void updateLEDs() {
  // Pixel 0: WiFi
  if (wifiConnected) {
    pixels.setPixelColor(0, pixels.Color(0, 80, 0));
  } else {
    bool blink = (millis() / 500) % 2;
    pixels.setPixelColor(0, blink ? pixels.Color(80, 0, 0) : 0);
  }

  // Pixel 1: Mode
  pixels.setPixelColor(1, currentMode == MANUAL ?
    pixels.Color(0, 0, 80) :   // Blue = manual
    pixels.Color(0, 80, 40));   // Teal = auto

  // Pixel 2-3: Motor state
  if (stepper.isRunning()) {
    pulseVal += pulseDir;
    if (pulseVal >= 180 || pulseVal <= 5) pulseDir = -pulseDir;
    
    if (stepper.distanceToGo() > 0) {
      pixels.setPixelColor(2, pixels.Color(0, pulseVal, 0));
      pixels.setPixelColor(3, pixels.Color(0, pulseVal, 0));
    } else {
      pixels.setPixelColor(2, pixels.Color(pulseVal, pulseVal / 3, 0));
      pixels.setPixelColor(3, pixels.Color(pulseVal, pulseVal / 3, 0));
    }
  } else {
    int b = map(getPositionPercent(), 0, 100, 5, 60);
    pixels.setPixelColor(2, pixels.Color(b, b, b));
    pixels.setPixelColor(3, pixels.Color(b, b, b));
  }

  pixels.show();
}

void flashPixels(uint8_t r, uint8_t g, uint8_t b, int times) {
  for (int t = 0; t < times; t++) {
    for (int i = 0; i < NUM_PIXELS; i++)
      pixels.setPixelColor(i, pixels.Color(r, g, b));
    pixels.show();
    delay(120);
    for (int i = 0; i < NUM_PIXELS; i++)
      pixels.setPixelColor(i, 0);
    pixels.show();
    delay(120);
  }
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//  WEB SERVER
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void setupWebServer() {
  server.on("/",           HTTP_GET,  handleRoot);
  server.on("/api/status", HTTP_GET,  handleStatus);
  server.on("/api/up",     HTTP_POST, handleUp);
  server.on("/api/down",   HTTP_POST, handleDown);
  server.on("/api/stop",   HTTP_POST, handleStopWeb);
  server.on("/api/set",    HTTP_POST, handleSetPosition);
  server.on("/api/mode",   HTTP_POST, handleSetMode);
  server.on("/api/calibrate", HTTP_POST, handleCalibrate);
  server.begin();
  Serial.println("Web server started on port 80");
}

// ── API Handlers ──

void handleStatus() {
  currentLux = lightSensor.readLightLevel();
  
  String state = "stopped";
  if (stepper.isRunning()) {
    state = (stepper.distanceToGo() > 0) ? "opening" : "closing";
  }

  String json = "{";
  json += "\"position\":" + String(getPositionPercent()) + ",";
  json += "\"state\":\"" + state + "\",";
  json += "\"lux\":" + String(currentLux, 1) + ",";
  json += "\"mode\":" + String((int)currentMode) + ",";
  json += "\"steps\":" + String(stepper.currentPosition()) + ",";
  json += "\"maxSteps\":" + String(MAX_POSITION);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleUp() {
  webCommandActive = true;
  stepper.moveTo(MAX_POSITION);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleDown() {
  webCommandActive = true;
  stepper.moveTo(MIN_POSITION);
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleStopWeb() {
  stepper.stop();
  webCommandActive = false;
  server.send(200, "application/json", "{\"ok\":true}");
}

void handleSetPosition() {
  if (server.hasArg("position")) {
    int pct = constrain(server.arg("position").toInt(), 0, 100);
    long target = map(pct, 0, 100, MIN_POSITION, MAX_POSITION);
    webCommandActive = true;
    stepper.moveTo(target);
    server.send(200, "application/json", "{\"ok\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"missing position\"}");
  }
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    int m = server.arg("mode").toInt();
    currentMode = (m == 1) ? AUTO_LIGHT : MANUAL;
    prefs.putInt("mode", (int)currentMode);
    server.send(200, "application/json", "{\"ok\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"missing mode\"}");
  }
}

void handleCalibrate() {
  stepper.setCurrentPosition(0);
  savePosition();
  server.send(200, "application/json", "{\"ok\":true}");
  Serial.println("Calibrated: position reset to 0");
}

// ── Web Page ──

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>Window Shade</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: #0f0f1a;
      color: #e0e0e0;
      min-height: 100vh;
      padding: 16px;
      -webkit-tap-highlight-color: transparent;
    }
    .wrap { max-width: 420px; margin: 0 auto; }
    h1 { text-align: center; font-size: 1.3rem; padding: 12px 0 16px; color: #c8c8ff; }
    .card {
      background: #1a1a2e;
      border-radius: 16px;
      padding: 18px;
      margin-bottom: 14px;
      border: 1px solid #2a2a4a;
    }

    /* Shade visual */
    .shade-box {
      position: relative;
      width: 100%;
      height: 130px;
      border-radius: 10px;
      overflow: hidden;
      border: 2px solid #333;
      background: #87CEEB;
    }
    .shade-fill {
      position: absolute;
      bottom: 0;
      width: 100%;
      background: #3a3a5a;
      transition: height 0.6s ease;
    }
    .shade-pct {
      position: absolute;
      width: 100%;
      text-align: center;
      top: 50%;
      transform: translateY(-50%);
      font-size: 2.2rem;
      font-weight: 700;
      text-shadow: 0 2px 10px rgba(0,0,0,0.7);
      z-index: 1;
    }

    /* Status row */
    .srow { display: flex; justify-content: space-between; padding: 6px 0; font-size: 0.9rem; }
    .srow .label { color: #888; }
    .srow .val { font-weight: 600; }
    .dot { display: inline-block; width: 8px; height: 8px; border-radius: 50%; margin-right: 5px; }
    .dot-g { background: #4caf50; }
    .dot-b { background: #2196f3; }
    .dot-o { background: #ff9800; }
    @keyframes pulse { 0%,100%{opacity:1}50%{opacity:.3} }
    .anim { animation: pulse .8s infinite; }

    /* Buttons */
    .btns { display: flex; gap: 10px; margin: 12px 0; }
    .btn {
      flex: 1;
      padding: 15px 0;
      border: none;
      border-radius: 12px;
      font-size: 1rem;
      font-weight: 700;
      cursor: pointer;
      color: #fff;
      transition: transform .1s;
    }
    .btn:active { transform: scale(.95); }
    .btn-up { background: #2e7d32; }
    .btn-stop { background: #c62828; }
    .btn-down { background: #1565c0; }

    /* Slider */
    .slider-wrap { padding: 8px 0; }
    .slider-labels { display: flex; justify-content: space-between; font-size: .8rem; color: #666; margin-bottom: 6px; }
    input[type=range] {
      width: 100%; height: 6px; border-radius: 3px;
      background: #2a2a4a; outline: none; -webkit-appearance: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none; width: 26px; height: 26px;
      border-radius: 50%; background: #e94560; cursor: pointer;
    }

    /* Mode toggle */
    .modes { display: flex; border-radius: 12px; overflow: hidden; border: 2px solid #2a2a4a; }
    .mbtn {
      flex: 1; padding: 12px; border: none; font-size: .9rem;
      font-weight: 600; cursor: pointer; background: #1a1a2e; color: #666;
      transition: all .2s;
    }
    .mbtn.on { background: #2a2a4a; color: #fff; }

    .small-btn {
      width: 100%; padding: 10px; background: transparent;
      border: 1px solid #333; color: #555; border-radius: 10px;
      font-size: .8rem; cursor: pointer;
    }
    .info { text-align: center; font-size: .78rem; color: #555; margin-top: 6px; }
  </style>
</head>
<body>
<div class="wrap">
  <h1>&#127967; Window Shade</h1>

  <div class="card">
    <div class="shade-box">
      <div class="shade-fill" id="shadeFill" style="height:50%"></div>
      <div class="shade-pct" id="shadePct">50%</div>
    </div>
    <div class="srow" style="margin-top:10px">
      <span class="label">Status</span>
      <span class="val"><span class="dot dot-g" id="sDot"></span><span id="sLabel">Stopped</span></span>
    </div>
    <div class="srow">
      <span class="label">Light</span>
      <span class="val" id="luxVal">--</span>
    </div>
    <div class="srow">
      <span class="label">Mode</span>
      <span class="val" id="modeVal">Manual</span>
    </div>
  </div>

  <div class="card">
    <div class="btns">
      <button class="btn btn-up" onclick="cmd('up')">&#9650; Open</button>
      <button class="btn btn-stop" onclick="cmd('stop')">Stop</button>
      <button class="btn btn-down" onclick="cmd('down')">&#9660; Close</button>
    </div>
    <div class="slider-wrap">
      <div class="slider-labels">
        <span>Closed</span>
        <span id="sliderPct">50%</span>
        <span>Open</span>
      </div>
      <input type="range" id="slider" min="0" max="100" value="50"
        oninput="document.getElementById('sliderPct').innerText=this.value+'%'"
        onchange="setPos(this.value)">
    </div>
  </div>

  <div class="card">
    <div class="modes">
      <button class="mbtn on" id="btnManual" onclick="setMode(0)">&#128075; Manual</button>
      <button class="mbtn" id="btnAuto" onclick="setMode(1)">&#9728;&#65039; Auto</button>
    </div>
    <div class="info" id="autoTip">Opens &gt;300 lux &middot; Closes &lt;50 lux</div>
  </div>

  <div class="card">
    <button class="small-btn" onclick="if(confirm('Reset current position as CLOSED (0%)?'))cmd('calibrate')">
      &#128295; Recalibrate (set current = fully closed)
    </button>
  </div>
</div>

<script>
  const $ = id => document.getElementById(id);
  let polling;

  function fetchStatus() {
    fetch('/api/status').then(r=>r.json()).then(d => {
      // Shade visual (inverted: 0% open = 100% fill)
      $('shadeFill').style.height = (100 - d.position) + '%';
      $('shadePct').innerText = d.position + '%';
      $('slider').value = d.position;
      $('sliderPct').innerText = d.position + '%';

      // Status dot
      const dot = $('sDot');
      const lbl = $('sLabel');
      dot.className = 'dot';
      if (d.state === 'opening') { dot.classList.add('dot-g','anim'); lbl.innerText = 'Opening...'; }
      else if (d.state === 'closing') { dot.classList.add('dot-b','anim'); lbl.innerText = 'Closing...'; }
      else { dot.classList.add('dot-g'); lbl.innerText = 'Stopped'; }

      // Lux
      $('luxVal').innerText = d.lux.toFixed(0) + ' lux';

      // Mode
      $('btnManual').className = 'mbtn' + (d.mode===0?' on':'');
      $('btnAuto').className   = 'mbtn' + (d.mode===1?' on':'');
      $('modeVal').innerText = d.mode===0 ? 'Manual' : 'Auto (Light)';
    }).catch(()=>{});
  }

  function cmd(a) { fetch('/api/'+a,{method:'POST'}).then(()=>setTimeout(fetchStatus,300)); }
  function setPos(v) {
    fetch('/api/set',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'position='+v})
    .then(()=>setTimeout(fetchStatus,300));
  }
  function setMode(m) {
    fetch('/api/mode',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'mode='+m})
    .then(()=>setTimeout(fetchStatus,300));
  }

  polling = setInterval(fetchStatus, 2000);
  fetchStatus();
</script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}
