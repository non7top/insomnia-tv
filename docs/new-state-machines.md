# Plan: Implementing Dual State Machines for System Management

## Objective
Implement two new state machines to centralize and robustly manage system-wide logic:
1.  **Connectivity State Machine**: Manages the WiFi and MQTT connection lifecycle.
2.  **Operational State Machine**: Orchestrates high-level system modes (Active, Dimming, Sleep, Override).

---

## 1. Connectivity State Machine (`ConnectivityManager`)

### Problem
Network logic is currently distributed across `WifiSetup`, `WebServer`, and `MqttBridge`, leading to complex event handling and potential race conditions during reconnection or OTA.

### Proposed States
- `DISCONNECTED`: No network active.
- `CONNECTING_WIFI`: Attempting to join the configured Access Point.
- `WIFI_PORTAL`: WiFiManager Configuration portal is active (AP mode).
- `WIFI_CONNECTED`: WiFi IP acquired, MQTT not yet active.
- `CONNECTING_MQTT`: Attempting to reach the MQTT broker.
- `ONLINE`: Fully functional (WiFi + MQTT).
- `ERROR`: Persistent failure state requiring intervention or reboot.

### Transitions
- **Event: WiFi Lost** -> Move to `CONNECTING_WIFI`.
- **Event: MQTT Fail** -> Move to `CONNECTING_MQTT`.
- **Event: Button Hold (10s)** -> Move to `WIFI_PORTAL`.

---

## 2. Operational Mode State Machine (`SystemModeManager`)

### Problem
The current `SleepStateMachine` is focused only on power transitions. We need a higher-level manager to handle "dimming" grace periods, user overrides, and maintenance modes.

### Proposed States
- `IDLE`: TV is OFF. Monitoring for PIR/Activity to trigger "Wake".
- `MONITORING`: TV is ON. Tracking activity levels.
- `WARN_DIMMING`: Inactivity detected. Ramping volume/brightness down to warn the user.
- `SLEEP_TRANSITION`: Actively turning off the TV (multiple IR retries).
- `FORCED_SLEEP`: TV is OFF. Presence is detected but user is considered "sleeping" (no wake until reset).
- `MANUAL_OVERRIDE`: Auto-off disabled for 1 hour (triggered via Web UI).

### Transitions
- **Event: Activity Detected** while in `WARN_DIMMING` -> Revert to `MONITORING` (Cancel ramp).
- **Event: Timer Expired** in `WARN_DIMMING` -> Move to `SLEEP_TRANSITION`.
- **Event: Web Command "Keep On"** -> Move to `MANUAL_OVERRIDE`.

---

## Implementation Steps

### Phase 1: Define Enums and Interfaces
- Create `src/state/ConnectivityState.h` and `src/state/OperationalState.h`.
- Update `SystemStateManager` to host these new sub-state machines.

### Phase 2: Implement Connectivity Manager
- Refactor `WifiSetup` to feed events into the state machine.
- Update `WebServer` status endpoint to reflect the unified `ConnectivityState`.

### Phase 3: Implement Operational Mode Manager
- Migrate and expand `SleepStateMachine` logic into `SystemModeManager`.
- Integrate `RampScheduler` directly into the `WARN_DIMMING` state.

### Phase 4: Verification
- Unit tests for state transitions (Native).
- Integration tests for WiFi/MQTT reconnect cycles (ESP32).

---

## Verification & Testing
- **Native Tests**: Mock WiFi/MQTT events to verify transition logic.
- **Hardware Tests**: Verify LED feedback (if applicable) for each state.
- **Web UI**: Update the "Status" page to show the textual description of both state machines.
