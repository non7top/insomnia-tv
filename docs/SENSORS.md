# Sensor Framework

The insomniaTV Sensor Framework provides a unified, extensible way to aggregate readings from various hardware and software sources.

## Core Components

### `Sensor` Base Class
All sensors must inherit from the `Sensor` abstract base class.

```cpp
class Sensor {
public:
    virtual String getId() const = 0;
    virtual String getType() const = 0;
    virtual bool read() = 0;
    virtual JsonDocument getConfig() = 0;
    virtual void setConfig(const JsonDocument& cfg) = 0;
    virtual bool isAvailable() const;
    virtual ~Sensor() = default;
};
```

### `SensorManager`
A singleton registry that manages sensor instances and provides thread-safe access.

## Adding New Sensors

1. Create a new class in `src/sensors/` that inherits from `Sensor`.
2. Implement all required virtual methods.
3. Register your sensor in `SensorManager`.
4. Add any required UI templates for configuration.

## Built-in Sensors

- **GPIO Input**: Digital input with debounce.
- **GPIO Analog**: ADC reading with scaling.
- **Ping**: Network reachability check.
- **HTTP**: Remote service status check.
- **UPnP**: Device discovery and monitoring.
