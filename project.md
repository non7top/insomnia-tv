# 📺 insomniaTV - ESP32 Smart IR Sleep Controller

## 📋 Project Metadata & Naming Registry
```yaml
project_name: insomniaTV
repo_name: insomnia-tv
brand_tagline: "The cure for watching TV when you should be sleeping"

# Canonical Naming Registry (stored in docs/naming-conventions.yaml)
naming_conventions:
  brand_display: insomniaTV        # Web UI, docs, marketing, logo
  github_repo: insomnia-tv         # git clone, PRs, issues, releases
  cli_command: insomnia-tv         # terminal execution, --help flags
  firmware_binary: insomnia_tv_esp32.bin  # build artifact, OTA flash
  mqtt_topic_root: home/insomnia_tv       # pub/sub hierarchy, retained msgs
  config_key: insomnia_tv                 # filesystem paths, JSON keys, env vars
  cpp_namespace: InsomniaTV               # C++ scope resolution, header guards
  cpp_class_prefix: Itv                   # e.g., ItvStateMachine, ItvIrDriver (optional)
  docker_container: insomniatv-builder    # CI build environment (if used)
```

## 🏷️ C++ Namespace Clarification: `InsomniaTV`
The C++ namespace follows **PascalCase** to align with Arduino/ESP-IDF standards and prevent symbol collisions with third-party libraries (`IRremoteESP8266`, `AsyncMqttClient`, etc.).

### Usage Rules
1. **Scope Resolution**: All project-specific classes and functions live under `namespace InsomniaTV { ... }`
   ```cpp
   namespace InsomniaTV {
       class StateMachine { /* ... */ };
       class IrDriver { /* ... */ };
   }
   ```
2. **Header Guards**: Use uppercase project prefix
   ```cpp
   #ifndef INSOMNIATV_STATE_MACHINE_H
   #define INSOMNIATV_STATE_MACHINE_H
   namespace InsomniaTV { class StateMachine; }
   #endif
   ```
3. **Implementation Files**: Wrap `.cpp` files in the namespace or use explicit scoping
   ```cpp
   #include "state/StateMachine.h"
   namespace InsomniaTV {
       void StateMachine::tick() { /* ... */ }
   }
   ```
4. **Why PascalCase?**: Differentiates from `kebab-case` (repo/CLI) and `snake_case` (MQTT/FS). Prevents accidental clashes with C macros, Arduino built-ins, or dependency namespaces. IDEs and linters expect this pattern for C++ projects.

---

## 📖 Overview & Constraints
- **Core Function:** Monitors IR remote activity. After configurable inactivity, gradually decreases TV volume, verifies TV availability via ping/HTTP, and sends a power-off sequence.
- **Hardware Target:** ESP32 (IR TX via RMT/LEDC, IR RX via TSOP382, WiFi, MQTT, AsyncWebServer)
- **Development Workflow:**
  - `main` branch protected. All changes via Pull Requests only.
  - CI gates: `lint`, `unit-tests`, `build`, `velxio-integration`
  - No direct pushes to `main`. Squash-merge required.
  - Phased delivery with mandatory test coverage per phase.

---

## 🏗️ System Architecture & Data Flow
```
[IR Receiver] → Decode/Hash → ActivityTracker(last_ts, similarity_groups)
                                      ↓
[SleepStateMachine] ← FreeRTOS Timers ← (inactivity >= threshold)
         ↓
   [RampScheduler] → Queue IR VOL_DOWN → (step >= max) → [TvVerifier]
         ↓
   [PowerOffDb] → Send selected code → [State: MONITORING]
         ↓
[ConfigManager] ↔ FAT32/LittleFS ↔ AsyncWebServer ↔ AsyncMqttClient
```

### Key HAL Interfaces (`src/hal/`)
- `IIrDriver`: `receive()`, `send(protocol, code)`, `learn_raw()`
- `INetworkChecker`: `ping(ip)`, `http_get(url)`, `set_timeout()`
- `IClock`: `now_ms()`, `advance_ms()` (mockable for tests)
- `IFileSystem`: `read_json()`, `write_json()`, `upload_file()`, `download_file()`
- `IMqttClient`: `publish()`, `subscribe()`, `set_callback()`

---

## ⚙️ Configuration Schema (`/config/insomnia_tv.json`)
```json
{
  "wifi": { "ssid": "", "password": "" },
  "mqtt": {
    "enabled": true,
    "broker": "",
    "port": 1883,
    "client_id": "insomniatv-esp32",
    "topic_root": "home/insomnia_tv",
    "user": "",
    "password": ""
  },
  "behavior": {
    "inactivity_timeout_min": 15,
    "volume_step_per_ramp": 1,
    "ramp_interval_min": 2,
    "max_ramp_steps_before_poweroff": 10,
    "stay_awake": true
  },
  "tv_verification": {
    "method": "ping",
    "target": "",
    "timeout_ms": 1000,
    "retries": 3
  },
  "ir_codes": {
    "volume_up": { "protocol": "NEC", "hex": "" },
    "volume_down": { "protocol": "NEC", "hex": "" },
    "power_off_database": [],
    "learned_codes_path": "/ir_learned.json"
  },
  "web": { "port": 80, "auth_enabled": false }
}
```

---

## 🔄 State Machine Specification
| State | Entry Trigger | Exit Trigger | Actions |
|-------|---------------|--------------|---------|
| `MONITORING` | Boot / Config reload / IR activity / Power-off complete | `inactivity_ms >= timeout` | Track IR pulses, update `last_activity_ts` |
| `RAMPING` | Inactivity timeout | IR activity / Max steps reached | Schedule VOL_DOWN every `ramp_interval`, publish state |
| `VERIFYING` | Max ramp steps reached | HTTP 200 / Ping success / Timeout / IR activity | Query TV, decide power-off or fallback |
| `POWERING_OFF` | Verification success | TX complete / Retry fail / IR activity | Send power code from DB, reset to `MONITORING` |
| `FALLBACK_OFF` | Verification fail (3 retries) | Timer expire | Assume TV off, log warning, reset to `MONITORING` |

**Cancellation Rule:** Any decoded IR pulse during `RAMPING`, `VERIFYING`, or `POWERING_OFF` immediately transitions to `MONITORING`, resets ramp counter, and publishes `{"event":"activity_detected","code_hash":"..."}` to `home/insomnia_tv/log`.

---

## 📅 Multi-Phase Delivery Plan
| Phase | Branch Pattern | Deliverables | Required Tests | PR Gate |
|-------|----------------|--------------|----------------|---------|
| **1** | `feature/phase-1-foundation` | Repo scaffold, HAL interfaces, config parser, `docs/naming-conventions.yaml`, JSON schema, `platformio.ini` | Config load/save, schema validation, HAL mock instantiation | `lint` + `unit-tests` |
| **2** | `feature/phase-2-ir-core` | `IRremoteESP8266` wrapper, `ActivityTracker`, similarity hash, sliding window | Decode accuracy, inactivity threshold, hash grouping, activity reset | `unit-tests` + coverage ≥80% |
| **3** | `feature/phase-3-sleep-ramp` | FreeRTOS state machine, `RampScheduler` (`xTimer`), IR cancellation hook, MQTT state publishing | Timer tick accuracy, mid-ramp cancellation, step bounds, MQTT retain | `unit-tests` + `build` |
| **4** | `feature/phase-4-tv-verify` | `TvVerifier` (async ping/HTTP), retry logic, fallback handler, `/api/test_power_code?index=N` | HTTP 200/404/timeout, retry backoff, DB iteration, network flakiness | `unit-tests` + `velxio-integration` |
| **5** | `feature/phase-5-web-mqtt` | AsyncWebServer, FAT32/LittleFS mount, file upload/download UI, `AsyncMqttClient` bridge, OTA endpoint | Web route parsing, FS roundtrip, MQTT pub/sub, config hot-reload | `lint` + `unit-tests` + `build` |
| **6** | `feature/phase-6-velxio-qemu` | Velxio test runner scripts, QEMU RTOS timing suite, E2E simulation, coverage aggregation | Full ramp→verify→poweroff cycle, IR spam, time warp, watchdog | `velxio-integration` + `qemu-timing` |
| **7** | `release/v1.0.0` | Watchdog, error recovery, memory leak hardening, release workflow, docs | Heap trace, 24h stability run, config corruption recovery | Manual QA + all gates |

---

## 🛡️ GitHub Workflow & CI Specification
### Branch Protection
```yaml
main:
  protect: true
  require_pr_approvals: 2
  dismiss_stale_reviews: true
  required_status_checks: [lint, unit-tests, build, velxio-integration]
  allow_auto_merge: false
  restrict_pushes: true
```

### PR Template (`.github/PULL_REQUEST_TEMPLATE.md`)
```markdown
## Checklist
- [ ] `pio test -e native` passes (coverage ≥80%)
- [ ] `clang-format` & `cpplint` clean
- [ ] Velxio integration tests pass (`velxio test run`)
- [ ] Naming conventions updated if new symbols introduced
- [ ] Config schema validated & Web UI endpoints tested
- [ ] MQTT topic mapping documented (`home/insomnia_tv/*`)
- [ ] Simulation notes & edge cases tested
```

### CI Pipeline (`.github/workflows/ci.yml`)
```yaml
name: insomniaTV CI
on: [pull_request]

jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: pip install clang-format clang-tidy cpplint
      - run: cpplint --recursive src/ test/
      - run: clang-format --style=file --dry-run --Werror src/**/*.cpp test/**/*.cpp

  unit-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: platformio/platformio-action@v1
      - run: pio test -e native --coverage
      - uses: actions/upload-artifact@v4
        with: { name: coverage-report, path: .pio/build/native/coverage.info }

  build-firmware:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: platformio/platformio-action@v1
      - run: pio run -e esp32dev -e qemu-esp32
      - uses: actions/upload-artifact@v4
        with: { name: firmware, path: .pio/build/esp32dev/insomnia_tv_esp32.bin }

  velxio-integration:
    needs: [build-firmware]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: actions/download-artifact@v4
        with: { name: firmware }
      - run: npm install -g velxio-cli
      - run: velxio test run tests/velxio/ --firmware insomnia_tv_esp32.bin --headless --timeout 120
      - run: velxio coverage export > velxio-coverage.json
      - uses: actions/upload-artifact@v4
        with: { name: velxio-report, path: velxio-coverage.json }
```

---

## 🧪 Simulation & Testing Framework
| Layer | Tool | Purpose | Execution |
|-------|------|---------|-----------|
| **Native Unit** | PlatformIO + Unity + FakeIt | Logic, config, state machine, HAL mocks | `pio test -e native` |
| **Velxio Integration** | Velxio CLI + JS Runner | IR pulse injection, HTTP mock, MQTT broker, time warp | `velxio test run tests/velxio/ --headless` |
| **QEMU Timing** | `qemu-system-xtensa -M esp32` | FreeRTOS scheduler, interrupt latency, watchdog validation | `pio test -e qemu-esp32` |
| **Coverage Aggregation** | `lcov` + `velxio-coverage.json` | Unified CI badge, PR diff comment | GitHub Actions artifact merge |

### Velxio Test Example (`tests/velxio/ramp_cancellation.test.js`)
```javascript
describe("Ramp Cancellation", () => {
  beforeAll(() => velxio.firmware.load("insomnia_tv_esp32.bin"));
  beforeEach(() => { velxio.time.set(0); velxio.mqtt.clear(); });

  it("Transitions to RAMPING after 15m idle", async () => {
    await velxio.time.advance("15m");
    await velxio.wait(500);
    expect(await velxio.mqtt.last("home/insomnia_tv/state")).toBe("RAMPING");
  });

  it("Resets to MONITORING on IR activity", async () => {
    await velxio.time.advance("15m");
    await velxio.wait(500);
    velxio.ir.send("NEC", { address: 0x20DF, command: 0x40BF, bits: 32 });
    await velxio.wait(300);
    expect(await velxio.mqtt.last("home/insomnia_tv/state")).toBe("MONITORING");
  });
});
```

---

## 📁 Repository Structure
```
insomnia-tv/
├── .github/
│   ├── workflows/ci.yml
│   └── PULL_REQUEST_TEMPLATE.md
├── src/
│   ├── main.cpp
│   ├── hal/
│   │   ├── IIrDriver.h
│   │   ├── INetworkChecker.h
│   │   ├── IClock.h
│   │   ├── IFileSystem.h
│   │   └── IMqttClient.h
│   ├── config/ConfigManager.cpp|.h
│   ├── ir/ActivityTracker.cpp|.h
│   ├── state/SleepStateMachine.cpp|.h
│   ├── network/TvVerifier.cpp|.h
│   └── web/WebServer.cpp|.h
├── test/
│   ├── native/          # PlatformIO unity tests
│   ├── velxio/           # JS integration scripts
│   └── qemu/            # RTOS timing validation
├── config/
│   └── insomnia_tv_schema.json
├── docs/
│   └── naming-conventions.yaml
├── platformio.ini
└── README.md
```

---

## 🤖 Agent Execution Protocol
1. **Initialization:** Clone `insomnia-tv` → checkout `feature/phase-1-foundation`
2. **Scaffold Generation:** Output `platformio.ini`, HAL headers, config parser, `docs/naming-conventions.yaml`, CI workflow, PR template
3. **Commit & Push:** `git add -A && git commit -m "feat(phase-1): foundation, HAL, config schema, naming registry, CI gates"`
4. **Open PR:** Target `main`, attach CI run results, request 2 reviews
5. **Merge Gate:** All status checks must pass. No force-push allowed.
6. **Phase Handoff:** Upon merge, agent checks out next phase branch, loads updated context, repeats.

**Agent Output Expectations:**
- Strict C++17 / Arduino framework compliance
- All mocks implement `I*` interfaces under `namespace InsomniaTV`
- Web routes return `application/json`
- MQTT payloads use flat JSON: `{"state":"...","vol":42,"ts":1713123456}`
- No blocking `delay()`; use FreeRTOS timers or `millis()` loops
- Reference `docs/naming-conventions.yaml` for any new symbol/file/topic

---

## 🚀 Next Steps for Execution
Reply with:
```
INIT_PHASE_1
```
to receive:
- Complete `platformio.ini` (native, esp32dev, qemu envs)
- `src/hal/*.h` interfaces + mock implementations
- `config/insomnia_tv_schema.json` + parser
- `docs/naming-conventions.yaml` + `.github/workflows/ci.yml` + PR template
- Ready-to-commit Phase 1 scaffolding

All subsequent phases will follow this exact PR-gated, CI-verified structure.
