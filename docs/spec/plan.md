# insomniaTV plan

How the current code is organised and what is left. Issues are the task list; this file groups them. Behaviour is in `spec.md`.

## Architecture

```
sensors ──► SensorManager ──► TvStateMachine ──► SleepStateMachine ──► power-off
 ping/http/upnp/                (weighted fusion:     (MONITORING ->        Samsung network
 samsung_tv/gpio                 ON / OFF / UNKNOWN)   RAMPING -> VERIFYING)  or IR (stub)
                                                          ▲
                              activity (IR stub, phone ping) ┘
ConfigManager (LittleFS JSON) ◄──► WebServer (REST + embedded UI), WifiSetup, OTA
```

- Components expose their own typed callback or a direct getter (`TvStateMachine::subscribe`, `SensorManager::subscribeValueChange`, `getPowerState()`); there is no global event bus (an earlier EventBus design was removed).
- Hardware and network access sit behind small interfaces (`src/hal/`, `ISamsungTvTransport`) so the logic runs in native tests.
- Single TV per device. `device_id` on sensors is groundwork only.

## Milestones

**1. Prove the core on the device**
- Samsung pairing and power-off verified on hardware (fix in #92 pending).
- One full natural cycle observed: idle, ramp, verify, power-off, back to MONITORING.

**2. Harden and secure (before calling it v1)**
- #51 credentials and the unauthenticated `/update`.
- #70 OTA rollback; #6 watchdog and recovery; #76 ping timeout.

**3. Presence and sensors**
- #60 Hall/current and PIR as fusion inputs, after #61 (continuous values in the `Sensor` interface), then #62 (presence as a second fusion target, replacing the `presence_phone` stand-in).
- #57 per-sensor weights exist in the JSON; a UI to edit them is missing.

**4. Actuators**
- #48 IR send/receive, blocked on hardware.
- #58 further power-off targets (relay, MQTT); the Samsung network target is done.

**5. Structure and UX**
- #44 / UI split out of `WebServer.cpp`; #55 remaining discovery-API work; #39 SSDP announcement.
- #50 `SystemModeManager` / `ConnectivityManager`, only if the extra states earn their cost (`docs/new-state-machines.md`).

## Open decision: foundation (#80)

Keep the bespoke Arduino firmware, or move to ESPHome. For (per #80): maintained base, real IR TX/RX, `ping`/`http_request` components. Against (not yet evaluated in depth): the weighted fusion and sleep state machines may not map onto ESPHome's YAML/lambda model, and the Samsung pairing would likely need a custom component. Needs a decision before milestones 3 and 4 grow further.

## Issue housekeeping

Phase issues #5 and #8 describe the abandoned phase plan; #1, #21, #23, #33 are loose notes. Close or fold them into the milestones above.
