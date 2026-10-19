# insomniaTV specification

Status key: **Built** (in firmware, exercised by tests and/or run on a device), **Stub** (code exists but does nothing real), **Planned** (an open issue, not in the code).

## Purpose

An ESP32 that stops a TV from staying on all night. After a period with no activity it lowers the volume in steps as a warning, checks the TV is really on, and switches it off.

## Behaviour

### Sleep cycle — Built (`src/state/`)

`SleepStateMachine`, ticked from `loop()`:

| State | Entered when | Leaves when |
|-------|--------------|-------------|
| MONITORING | boot, any activity, end of a cycle | idle time reaches `inactivity_timeout_min` (default 15) |
| RAMPING | idle timeout | ramp finishes (`onRampComplete`), or activity |
| VERIFYING | ramp finished | the next tick, by TV state (below) |
| POWERING_OFF / FALLBACK_OFF | VERIFYING with TV ON/TRANSITIONING (or no TV state machine) / UNKNOWN | immediately: the power-off callback fires and the machine returns to MONITORING in the same tick, so a poll never observes them |

In VERIFYING a TV state of OFF goes straight back to MONITORING without sending anything; ON, TRANSITIONING and UNKNOWN all fire the power-off callback once (UNKNOWN is the "fallback" case). `RampScheduler` fires the volume-down callback every `ramp_interval_min` (default 2) minutes, `max_ramp_steps_before_poweroff` (default 10) times. Any activity returns to MONITORING and cancels the ramp. The idle clock is `SleepStateMachine::msSinceReset()` (`msSinceSleepReset` in `/api/status`).

### Activity detection — Stub / interim

- IR remote activity: **Stub**. `IrDriver` never decodes anything (`hasDecoded()` is always false) and `send()` returns true without sending (#48; no IR hardware attached).
- Interim: a ping sensor with the reserved id `presence_phone` going from unreachable to reachable counts as activity (generalization planned, #62).

### TV state — Built (`src/tv/TvStateMachine`, `src/sensors/`)

Each detection sensor votes `+weight` (true) or `-weight` (false). Confidence is the weighted sum scaled to -10..+10: above +3 is ON, below -3 is OFF, otherwise UNKNOWN, with a hysteresis of 2 (the same reading on 2 consecutive evaluations before the state changes). Default weights: `samsung_tv` 5, `upnp` 4, `ping` 3, anything else 2; a sensor's JSON `weight` overrides. Sensor types: `ping`, `http`, `upnp`, `samsung_tv` (PowerState from the TV's REST API, the only one that sees standby), `gpio_input`, `gpio_analog`. Sensors are polled on their own task so a slow target can't stall the main loop. Sensors can carry a `device_id` (metadata only; runtime is single-TV).

### Power-off — Built for Samsung, Stub for IR

- Samsung Tizen TV paired: `KEY_POWER` over the TV's WebSocket on port 8002, sent only if `PowerState` is `on` (`docs/SAMSUNG_TV.md`). Pairing is one-time from the web UI and the token is stored in the config.
- Otherwise: the IR power code via `IrDriver` (**Stub**).
- Not possible: switching the TV back on; in deep standby it leaves the network.

### Configuration — Built (`src/config/`)

JSON at `/config/insomnia_tv.json` on LittleFS (`behavior`, `tv_verification`, `ir_codes`, `tv_control`, `sensors`, `web`, `mqtt`, `config_version`). Out-of-range values are replaced by defaults at boot (not written back). The three timeouts (`inactivity_timeout_min`, `ramp_interval_min`, `max_ramp_steps_before_poweroff`) apply live when the file is saved from the web editor; other fields apply at boot. `mqtt.*`, `web.auth_enabled` and `tv_verification.*` are stored but unused by the firmware.

### Web UI and API — Built (`src/web/WebServer.cpp`)

Tabs: Status (TV and sleep state, pulsing while transitional, live refresh), Config (Samsung pairing and test), Sensors (registry, Add Smart TV wizard with discovery), Files (browse, edit, upload, delete), plus a System tab (reboot, firmware upload) once PR #91 is merged. Routes: `/api/status`, `/api/sensors`, `/api/tv-config`, `/api/tv/control|pair|power-off`, `/api/scan`, `/api/discovery`, `/api/probe`, `/api/files*`, `/update`. Authentication is HTTP auth with the fixed credentials in `SystemCredentials.h` (#51); `/update` has no authentication check.

### Network and device — Built

First boot (no saved WiFi) opens a captive portal AP named `insomnia-<CHIPID>`; holding BOOT (GPIO 9) for 10 s clears WiFi settings. ArduinoOTA is enabled on `insomnia-<CHIPID>.local` (password = the web password). Status LED: slow breathing normally, fast blink while the portal waits. Target board: ESP32-C3 (`esp32c3` env). SSDP announcement of the device itself: Planned (#39). MQTT: Planned (config fields and `IMqttClient` exist, no implementation).

## Known gaps

IR send/receive (#48), credentials and open `/update` (#51), OTA rollback (#70), ping timeout not honoured (#76), UI embedded in `WebServer.cpp` (#44).
