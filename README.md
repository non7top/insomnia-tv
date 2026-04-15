# insomniaTV

> The cure for watching TV when you should be sleeping

ESP32-based smart IR sleep controller that monitors remote activity, gradually decreases TV volume after inactivity, verifies TV availability, and sends a power-off sequence.

## Quick Start

### Prerequisites
- [PlatformIO](https://platformio.org/) CLI or VS Code extension
- ESP32 development board
- TSOP382 IR receiver + IR LED

### Build Firmware
```bash
pio run -e esp32dev
```

### Run Native Tests
```bash
pio test -e native
```

### Upload to ESP32
```bash
pio run -e esp32dev -t upload
```

## Architecture

```
[IR Receiver] → ActivityTracker → SleepStateMachine → RampScheduler
    → TvVerifier → PowerOffDb → ConfigManager ↔ AsyncWebServer / MQTT
```

See `project.md` for full specification and phase delivery plan.

## Phases

| Phase | Branch | Status |
|-------|--------|--------|
| 1. Foundation | `feature/phase-1-foundation` | 🚧 In Progress |
| 2. IR Core | `feature/phase-2-ir-core` | Planned |
| 3. Sleep Ramp | `feature/phase-3-sleep-ramp` | Planned |
| 4. TV Verify | `feature/phase-4-tv-verify` | Planned |
| 5. Web/MQTT | `feature/phase-5-web-mqtt` | Planned |
| 6. Vexio/QEMU | `feature/phase-6-vexio-qemu` | Planned |
| 7. Release v1.0 | `release/v1.0.0` | Planned |

## Configuration

Edit `/config/insomnia_tv.json` on the device filesystem. Schema validation is in `config/insomnia_tv_schema.json`.

## License

MIT
