# ECET230 Prototyping Project: Motorized Window Shade Controller

## Project Overview

An ESP32-based motorized window shade system that automatically opens and closes based on ambient light levels, with manual override via physical buttons and a WiFi-hosted web interface for phone control.

## Team Members

- Ezra Lynn
- Qosai Awawda

## Repository Structure

```
├── docs/
│   ├── design-decision-document.md   # Design decisions + block diagram
│   ├── project-logbook.md            # Weekly progress, evaluations, decisions
│   └── project-definition.md         # Full project specification
├── src/
│   └── motorized_shade.ino           # Main Arduino/ESP32 sketch
├── tasks/
│   └── tasks.md                      # Task breakdown with dependencies
├── diagrams/
│   └── block-diagram.svg             # System block diagram
└── README.md
```

## Key Components

| Component             | Part               | Purpose |
|-----------            |------              |---------|
| MCU                   | ESP32 DevKit V1    | WiFi + BLE, motor control, sensor reading |
| Motor                 | 28BYJ-48 + ULN2003 | Rolls shade up/down |
| Light Sensor          | BH1750 (I2C)       | Ambient light detection for auto mode |
| LEDs                  | WS2812B NeoPixels  | Status indication (mode, WiFi, movement) |
| Buttons               | 3x Momentary       | Up, Down, Mode select |
| Limit Switches        | 2x Microswitch     | Prevents over-travel at top/bottom |

## Features

- **Manual Control**: Physical buttons for up/down/mode
- **Auto Mode**: Opens/closes shade based on light sensor thresholds
- **WiFi Web UI**: Control from any phone browser on local network
- **Position Memory**: Remembers shade position across power cycles (NVS)
- **Safety**: Limit switches prevent motor over-driving
- **LED Status**: Color-coded NeoPixels for mode/state indication
