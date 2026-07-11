# Architectural Analysis: Centralized System State Management

## User Decisions
1. **Presence Sensing**: Yes, integrating PIR and other sensors.
2. **Activity Logic**: Sensor Fusion.
3. **State Updates**: Reactive Pub/Sub (Recommended for event-driven ESP32 systems).

## Revised Design: `SystemStateManager` with Event Bus

### Key Components
- **SystemStateManager**: Holds the state (`Power`, `Presence`, `ActivityLevel`).
- **EventBus**: Asynchronous communication channel.
- **Sensors/Drivers**: Emit events (`PIR_ACTIVE`, `IR_ACTIVITY`, `TV_PING_FAIL`).
- **Subscribers**: `SleepStateMachine` (reacts to state changes), `MqttBridge` (publishes updates).

### Implementation Plan
1. **Define `SystemEvent` enum**: The vocabulary for our Event Bus.
2. **Implement lightweight `EventBus`**: Simple observer pattern for component decoupling.
3. **Refactor existing logic**: Migrate activity tracking to emit `SYSTEM_ACTIVITY` events.
4. **Implement `SystemStateManager`**: Listen for relevant events and update the central `SystemState` struct.
5. **Update `SleepStateMachine`**: Subscribe to `SystemStateManager` updates and make decisions based on the new, richer context.
