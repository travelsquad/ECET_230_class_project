# Tasks — Motorized Window Shade Controller

**ECET230 Prototyping Class | NJIT**

---

## Task Overview

```
Priority: 🔴 Critical  🟡 High  🟢 Medium  ⚪ Optional
Status:   ✅ Done  🔄 In Progress  ⬜ Not Started
```

---

## Phase 1: Core Motor Control

| # | Task | Priority | Status | Depends On | Notes |
|---|------|----------|--------|------------|-------|
| 1.1 | Wire 28BYJ-48 stepper + ULN2003 to Arduino | 🔴 | ✅ | — | Pins 8,10,9,11 (IN1-IN3-IN2-IN4 order) |
| 1.2 | Basic CW/CCW rotation test | 🔴 | ✅ | 1.1 | Verify motor direction matches shade up/down |
| 1.3 | Add UP/DOWN buttons with pull-ups | 🔴 | ✅ | 1.1 | GPIO with INPUT_PULLUP, active LOW |
| 1.4 | Migrate from Stepper.h to AccelStepper | 🔴 | ✅ | 1.2 | Non-blocking run(), acceleration support |
| 1.5 | Calibrate MAX_POSITION | 🔴 | ✅ | 1.4 | Count revolutions × 2048 steps/rev |
| 1.6 | Implement position limits in software | 🟡 | ✅ | 1.5 | Clamp between 0 and MAX_POSITION |

**Phase 1 Deliverable:** Motor moves shade up/down with button control and stops at limits.

---

## Phase 2: Sensors & Feedback

| # | Task | Priority | Status | Depends On | Notes |
|---|------|----------|--------|------------|-------|
| 2.1 | Wire BH1750 light sensor (I2C) | 🟡 | ✅ | 1.1 | SDA/SCL, 3.3V power |
| 2.2 | Read + display lux values on Serial | 🟡 | ✅ | 2.1 | Verify sensor readings make sense |
| 2.3 | Implement auto mode logic | 🟡 | ✅ | 1.4, 2.2 | Open >300 lux, close <50 lux, hysteresis |
| 2.4 | Add MODE button (Manual/Auto toggle) | 🟡 | ✅ | 2.3 | Third button, debounced |
| 2.5 | Wire WS2812B NeoPixels | 🟢 | ✅ | 1.1 | Single data pin, 5V power |
| 2.6 | Implement LED color states | 🟢 | ✅ | 2.5 | Blue=manual, Green=auto, White=moving |

**Phase 2 Deliverable:** Shade reacts to light automatically. LEDs show system state.

---

## Phase 3: ESP32 Migration & WiFi

| # | Task | Priority | Status | Depends On | Notes |
|---|------|----------|--------|------------|-------|
| 3.1 | Move all wiring to ESP32 DevKit V1 | 🔴 | ✅ | Phase 2 | New GPIO mapping required |
| 3.2 | Update pin definitions in code | 🔴 | ✅ | 3.1 | Arduino pins → ESP32 GPIOs |
| 3.3 | Verify all sensors/motor work on ESP32 | 🔴 | ✅ | 3.2 | I2C, stepper, NeoPixel, buttons |
| 3.4 | Implement WiFi connection | 🔴 | ✅ | 3.1 | 2.4GHz only, serial print IP address |
| 3.5 | Build web server with HTML UI | 🔴 | ✅ | 3.4 | Embedded HTML, AJAX updates |
| 3.6 | Create REST API endpoints | 🟡 | ✅ | 3.5 | /api/status, /move, /mode, /position |
| 3.7 | Implement NVS position storage | 🟡 | ✅ | 3.2 | Preferences library, persist across reboot |
| 3.8 | Test phone control end-to-end | 🔴 | ✅ | 3.5, 3.6 | Open browser → control shade |

**Phase 3 Deliverable:** Full WiFi control from phone. Position persists across power cycles.

---

## Phase 4: Safety & Polish

| # | Task | Priority | Status | Depends On | Notes |
|---|------|----------|--------|------------|-------|
| 4.1 | Wire limit switches (top/bottom) | 🔴 | ✅ | 3.1 | GPIO 34/35, external 10kΩ pull-ups |
| 4.2 | Implement hardware stop logic | 🔴 | ✅ | 4.1 | Motor stops immediately on limit trigger |
| 4.3 | Add motor de-energize on stop | 🟢 | ✅ | 1.4 | Prevents motor overheating when idle |
| 4.4 | Stress test (50 cycles) | 🟡 | ✅ | Phase 3, 4.1 | No mechanical/electrical failures |
| 4.5 | Tune acceleration/speed values | 🟢 | ✅ | 4.4 | Smooth operation, no jams |
| 4.6 | Final code cleanup & comments | 🟢 | ✅ | All | Production-ready sketch |

**Phase 4 Deliverable:** Reliable, safe system ready for demo.

---

## Phase 5: Documentation & Submission

| # | Task | Priority | Status | Depends On | Notes |
|---|------|----------|--------|------------|-------|
| 5.1 | Create GitHub repository | 🔴 | 🔄 | — | Public or class-accessible |
| 5.2 | Write design decision document | 🔴 | ✅ | All design choices | Include block diagram |
| 5.3 | Complete project logbook | 🔴 | ✅ | Weekly entries | Options, evaluations, decisions |
| 5.4 | Document task breakdown | 🔴 | ✅ | This document | Dependencies and priorities |
| 5.5 | Write project definition | 🔴 | ✅ | All | Full engineering spec |
| 5.6 | Upload all files to repo | 🔴 | ⬜ | 5.1-5.5 | Verify folder structure |
| 5.7 | Submit repo link | 🔴 | ⬜ | 5.6 | On Canvas/LMS |

---

## Dependency Graph

```
1.1 ──► 1.2 ──► 1.4 ──► 1.5 ──► 1.6
  │       │              │
  │       ▼              │
  │     1.3              │
  │                      │
  ├──► 2.1 ──► 2.2 ──► 2.3 ──► 2.4
  │                      │
  ├──► 2.5 ──► 2.6       │
  │                      │
  └──► Phase 2 Complete ─┤
                         │
                         ▼
                  3.1 ──► 3.2 ──► 3.3
                    │              │
                    ▼              ▼
                  3.4 ──► 3.5 ──► 3.6 ──► 3.8
                           │
                           ▼
                         3.7
                           │
                           ▼
                  4.1 ──► 4.2
                           │
                         4.3, 4.4 ──► 4.5 ──► 4.6
                                               │
                                               ▼
                                      5.1 through 5.7
```

---

## Time Estimate Summary

| Phase | Est. Hours | Actual |
|-------|-----------|--------|
| Phase 1: Core Motor Control | 4-6 hrs | [Fill in] |
| Phase 2: Sensors & Feedback | 4-6 hrs | [Fill in] |
| Phase 3: ESP32 & WiFi | 6-8 hrs | [Fill in] |
| Phase 4: Safety & Polish | 3-4 hrs | [Fill in] |
| Phase 5: Documentation | 3-4 hrs | [Fill in] |
| **Total** | **20-28 hrs** | |
