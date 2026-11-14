# ThermostatBase

Common C++11 foundation for temperature controllers. This library has no binary
output, hysteresis, PID, GPIO, or relay-timing dependency.

The concrete binary implementation, example, and tests are in the sibling
[ThermostatHysteresis library](../ThermostatHysteresis/README.md).

## Dependencies and ownership

Install TemperatureBase and ILogger beside this library in the Arduino
libraries folder. The injected TemperatureBase reference, optional ILogger
pointer, and optional name string are non-owning and must outlive the controller.
The application initializes and updates the sensor; a thermostat never owns or
schedules acquisitions.

Constructing ThermostatBase does not log or access hardware. Concrete sensor
and adapter libraries may log during construction; construct logged objects
after Serial.begin() when using SerialLogger.

## Contract

ThermostatBase is abstract. A concrete implementation supplies Begin(now_ms),
Update(now_ms), GetStatus(), GetConfig(), and SetConfig(). Output representation
and output-specific notifications belong to that implementation.

| Common setting | Default | Meaning |
| --- | --- | --- |
| mode | Mode::kHeating | Heating or cooling control direction |
| enabled | true | Desired enable state |
| target_c | 21.0 | Setpoint in Celsius |
| minimum_target_c | 5.0 | Minimum permitted setpoint |
| maximum_target_c | 35.0 | Maximum permitted setpoint |
| maximum_reading_age_seconds | 30 | Inclusive reading-age limit |

Configuration validation requires a defined direction, finite temperatures,
ordered bounds, a setpoint inside those bounds, and a supported age limit.
SetTargetC() clamps finite inputs to the configured bounds and rejects NaN or
infinity. SetConfig() rejects invalid settings instead of clamping them.
SetEnabled() and SetTargetC() delegate to the concrete SetConfig() contract.

GetConfig() returns a const reference owned by the controller. Copy it before
editing or retaining a snapshot:

~~~cpp
auto config = thermostat.GetConfig();
config.target_c = 22.0f;
if (!thermostat.SetConfig(config)) {
  // Handle rejected configuration.
}
~~~

Calling these methods through ThermostatBase& preserves a concrete variant's
additional settings. Its covariant GetConfig() reference exposes the same common
values. Copying through the base copies only the common configuration.

## Input health and status

Protected ReadTemperature() gives concrete controllers a shared health check:

- Disabled takes precedence over input faults.
- NoData covers missing data and sensor configuration invalidation.
- SensorError covers unhealthy acquisitions or nonfinite readings.
- StaleReading covers an otherwise healthy reading older than the age limit.
- Ok provides a usable finite Celsius value.

These are named Status::kDisabled, Status::kNoData, Status::kSensorError,
Status::kStaleReading, and Status::kOk in code. The remaining generic lifecycle
statuses are kNotStarted, kStarting, and kInitializationFailed. StatusText()
returns their readable names. A historical good reading does not make a later
failed acquisition healthy. Fault response and output commands belong to the
concrete controller.

## Time and execution

All thermostat duration settings use seconds. Clock arguments use uint32_t
milliseconds from the same millis() clock as TemperatureBase timestamps.
kMaximumDurationSeconds is 2147483; this bounds conversion to a half-range
unsigned millisecond interval. Call Update() regularly from one loop/task.

Unsigned millisecond differences support normal millis() rollover. Reported
timestamps in concrete controllers are for display, not elapsed-time arithmetic.
The libraries are not designed for simultaneous calls from tasks or interrupts.

## Package split and migration

Version 2.0.0 separates the former combined library:

- Include ThermostatBase.h for the common contract.
- Install ThermostatHysteresis and include ThermostatHysteresis.h for binary
  control. That library owns the example and host tests.
- Mode::Heating / Cooling became Mode::kHeating / kCooling.
- Status enumerators gained k prefixes, for example Status::kOk.
- targetC became target_c.
- minimumTargetC / maximumTargetC became minimum_target_c / maximum_target_c.
- maximumReadingAgeSeconds became maximum_reading_age_seconds.

The old source filenames and method names remain. Do not keep the old
ThermostatHysteresis source files inside ThermostatBase: duplicate definitions
can result. See the sibling README for variant-specific field migrations.

## Style and review

Sources follow [Google C++ naming and formatting](https://google.github.io/styleguide/cppguide.html):
snake_case data fields, k-prefixed enumerators/constants, UpperCamelCase methods,
two-space indentation, 80-column formatting, and unique header guards.
.clang-format makes the formatting reproducible. Arduino public header names,
.cpp extensions, global API classes, C headers supported by AVR, and C++11 are
retained as explicit platform conventions.

Behavior is verified by the sibling library's CMake tests, including common
configuration through the base interface, validation, input health, freshness,
fault precedence, and rollover. See its tests/README.md for commands.

library.properties and tab-separated keywords.txt follow the
[Arduino library format](https://docs.arduino.cc/arduino-cli/library-specification/).
The project URL is intentionally blank because no repository URL was supplied.
