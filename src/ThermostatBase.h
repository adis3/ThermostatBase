#ifndef THERMOSTAT_BASE_SRC_THERMOSTAT_BASE_H_
#define THERMOSTAT_BASE_SRC_THERMOSTAT_BASE_H_

#include <stdint.h>

#include <ILogger.h>
#include <TemperatureBase.h>

// Shared temperature-control contract. Output types belong to concrete
// variants. Use from one loop/task; dependencies and name are non-owning.
class ThermostatBase {
 public:
  enum class Mode : uint8_t { kHeating, kCooling };
  enum class Status : uint8_t {
    kNotStarted,
    kStarting,
    kDisabled,
    kOk,
    kNoData,
    kSensorError,
    kStaleReading,
    kInitializationFailed
  };

  struct Config {
    Mode mode = Mode::kHeating;
    bool enabled = true;
    float target_c = 21.0f;
    float minimum_target_c = 5.0f;
    float maximum_target_c = 35.0f;
    uint32_t maximum_reading_age_seconds = 30;
  };

  // Limit conversion to a bounded unsigned millisecond interval.
  static constexpr uint32_t kMaximumDurationSeconds = 0x7FFFFFFFUL / 1000UL;

  explicit ThermostatBase(TemperatureBase& sensor, ILogger* logger = nullptr,
                          const char* name = nullptr);
  virtual ~ThermostatBase();
  ThermostatBase(const ThermostatBase&) = delete;
  ThermostatBase& operator=(const ThermostatBase&) = delete;

  // Clock arguments use the SAME millis() clock as TemperatureBase.
  // All configuration durations and reported thermostat timestamps use seconds.
  virtual bool Begin(uint32_t now_ms) = 0;
  virtual void Update(uint32_t now_ms) = 0;

  const char* GetName() const;
  virtual Status GetStatus() const = 0;
  // The reference is owned by the thermostat. Copy it before editing settings.
  virtual const Config& GetConfig() const = 0;

  // Concrete variants preserve their own settings when applying common config.
  virtual bool SetConfig(const Config& config) = 0;
  bool SetTargetC(float target_c);  // Clamp finite values to setpoint limits.
  bool SetEnabled(bool enabled);    // Applied by the next Update().

  static bool IsValidConfig(const Config& config);
  static const char* StatusText(Status status);

 protected:
  Status ReadTemperature(uint32_t now_ms, float& temperature_c) const;
  static bool SameConfig(const Config& a, const Config& b);
  static uint32_t Milliseconds(uint32_t seconds);
  void Log(LogLevel level, const char* message) const;

 private:
  TemperatureBase& sensor_;
  ILogger* logger_;
  const char* name_;
};

#endif  // THERMOSTAT_BASE_SRC_THERMOSTAT_BASE_H_
