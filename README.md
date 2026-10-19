# insomniaTV

> The cure for watching TV when you should be sleeping

ESP32-based sleep controller: after a period of inactivity it gradually lowers the TV volume, checks the TV is really on, and switches it off (over the network for Samsung TVs, via IR once that hardware exists).

## Quick Start

### Prerequisites
- [PlatformIO](https://platformio.org/) CLI or VS Code extension
- ESP32 development board
- Optional, not wired in firmware yet: TSOP382 IR receiver + IR LED

### Build Firmware
```bash
pio run -e esp32c3
```

### Run Native Tests
```bash
pio test -e native
```

### Upload to ESP32
```bash
pio run -e esp32c3 -t upload
```

## Architecture

```
sensors (ping / http / upnp / samsung_tv / gpio) -> SensorManager
    -> TvStateMachine (weighted fusion: TV ON / OFF / UNKNOWN)
    -> SleepStateMachine (MONITORING -> RAMPING -> VERIFYING -> POWERING_OFF)
    -> power-off: Samsung network command, or IR (not wired yet)
ConfigManager (LittleFS JSON) <-> WebServer (dashboard, sensors, files, OTA)
```

- What is built, stubbed and planned: `docs/spec/spec.md`; roadmap: `docs/spec/plan.md`; working principles: `docs/spec/constitution.md`.
- Sensors and the fusion weights are configured from the web UI (Add Smart TV wizard); see `docs/SENSORS.md`.
- Network power-off for Samsung Tizen TVs: `docs/SAMSUNG_TV.md`.
- IR transmit/receive is still a stub (no hardware attached yet, #48).
- `project.md` is the original spec and phase plan; the code has since moved to the sensor-fusion design above (no `TvVerifier`, `PowerOffDb`, MQTT or Velxio/QEMU).

## Status

Work is tracked in GitHub issues rather than the old phase table. Open areas: IR actuation (#48), more sensor types (#60-#62), OTA rollback (#70), credentials (#51), and the ESPHome evaluation (#80).

## Configuration

The device config lives at `/config/insomnia_tv.json` on the device filesystem and is editable from the web UI's Files tab; changes to the sleep/ramp timeouts apply without a reboot. Schema: `config/insomnia_tv_schema.json`.

## License

MIT
