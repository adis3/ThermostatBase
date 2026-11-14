#include "ThermostatBase.h"

#include <math.h>
#include <stdio.h>

constexpr uint32_t ThermostatBase::kMaximumDurationSeconds;

ThermostatBase::ThermostatBase(TemperatureBase& sensor, ILogger* logger,
                               const char* name)
    : sensor_(sensor), logger_(logger), name_(name) {
  // No logging or hardware access during construction.
}

ThermostatBase::~ThermostatBase() = default;

const char* ThermostatBase::GetName() const {
  return name_ && name_[0] ? name_ : "missing name";
}

bool ThermostatBase::IsValidConfig(const Config& c) {
  return (c.mode == Mode::kHeating || c.mode == Mode::kCooling) &&
         isfinite(c.target_c) && isfinite(c.minimum_target_c) &&
         isfinite(c.maximum_target_c) &&
         c.minimum_target_c <= c.maximum_target_c &&
         c.target_c >= c.minimum_target_c && c.target_c <= c.maximum_target_c &&
         c.maximum_reading_age_seconds <= kMaximumDurationSeconds;
}

bool ThermostatBase::SameConfig(const Config& a, const Config& b) {
  return a.mode == b.mode && a.enabled == b.enabled &&
         a.target_c == b.target_c && a.minimum_target_c == b.minimum_target_c &&
         a.maximum_target_c == b.maximum_target_c &&
         a.maximum_reading_age_seconds == b.maximum_reading_age_seconds;
}

bool ThermostatBase::SetTargetC(float target_c) {
  if (!isfinite(target_c)) {
    return false;
  }
  Config config = GetConfig();
  if (target_c < config.minimum_target_c) {
    target_c = config.minimum_target_c;
  }
  if (target_c > config.maximum_target_c) {
    target_c = config.maximum_target_c;
  }
  config.target_c = target_c;
  return SetConfig(config);
}

bool ThermostatBase::SetEnabled(bool enabled) {
  Config config = GetConfig();
  config.enabled = enabled;
  return SetConfig(config);
}

uint32_t ThermostatBase::Milliseconds(uint32_t seconds) {
  return seconds * 1000UL;
}

ThermostatBase::Status ThermostatBase::ReadTemperature(
    uint32_t now_ms, float& temperature_c) const {
  if (!GetConfig().enabled) {
    return Status::kDisabled;
  }
  const TemperatureBase::State sensor = sensor_.GetState();
  if (sensor.status == TemperatureBase::Status::NoData) {
    return Status::kNoData;
  }
  if (sensor.status != TemperatureBase::Status::Ok) {
    return Status::kSensorError;
  }
  if (!sensor.hasReading) {
    return Status::kNoData;
  }
  if (!isfinite(sensor.reading.temperatureC)) {
    return Status::kSensorError;
  }
  if (static_cast<uint32_t>(now_ms - sensor.reading.sampledAtMs) >
      Milliseconds(GetConfig().maximum_reading_age_seconds)) {
    return Status::kStaleReading;
  }
  temperature_c = sensor.reading.temperatureC;
  return Status::kOk;
}

const char* ThermostatBase::StatusText(Status status) {
  switch (status) {
    case Status::kNotStarted:
      return "NotStarted";
    case Status::kStarting:
      return "Starting";
    case Status::kDisabled:
      return "Disabled";
    case Status::kOk:
      return "Ok";
    case Status::kNoData:
      return "NoData";
    case Status::kSensorError:
      return "SensorError";
    case Status::kStaleReading:
      return "StaleReading";
    case Status::kInitializationFailed:
      return "InitializationFailed";
    default:
      return "Unknown";
  }
}

void ThermostatBase::Log(LogLevel level, const char* message) const {
  if (!logger_) {
    return;
  }
  char source[80];
  snprintf(source, sizeof(source), "Thermostat '%.50s'", GetName());
  logger_->Log(level, source, message);
}
