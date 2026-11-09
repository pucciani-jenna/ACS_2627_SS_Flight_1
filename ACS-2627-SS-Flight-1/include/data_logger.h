#pragma once

#include <stddef.h>
#include <stdint.h>

enum class ImuChannel {
    LowG,
    HighG,
    Gyro,
};

// Every path is populated only when a new read succeeds.
struct PressureReading {
    uint32_t tMs = 0;
    float pressurePa = 0;
    float temperatureC = 0;
    bool fresh = false;
};

struct VectorReading {
    uint32_t tMs = 0;
    float x = 0;
    float y = 0;
    float z = 0;
    bool fresh = false;
};

struct ImuReading {
    VectorReading lowG;
    VectorReading highG;
    VectorReading gyro;
};

struct LoggedTelemetrySample {
    PressureReading barometers[3];  // A1, A2, A3
    ImuReading imus[3];              // I1, I2, I3
};

struct DataLoggerDiagnostics {
    bool initialized = false;
    bool syncPending = false;
    size_t bufferedBytes = 0;
    uint32_t appendFailures = 0;
    uint32_t writeFailures = 0;
    uint32_t syncFailures = 0;
};

bool DataLoggerLogTelemetry(const LoggedTelemetrySample& sample);
bool DataLoggerService();   // Call regularly from loop, even with no fresh data.
bool DataLoggerForceSync();
bool DataLoggerIsInitialized();
DataLoggerDiagnostics DataLoggerGetDiagnostics();

// Start once per boot. DataLoggerFilename() reports the active file after success.
bool DataLoggerBegin();
const char* DataLoggerFilename();

// The following are called after the read is cussessful. A1 through A3 are passed as barometers, A1 is top barometer and A3 is bottom

// IMU is passed in as I1 through I3, I1 is top IMU and I3 is bottom IMU. The channel is passed in as LowG, HighG, or Gyro. The x, y, z values are in m/s^2 for LowG and HighG, and rad/s for Gyro.
bool DataLoggerLogPressure(uint32_t tMs, const char* sensor,
                    float pressurePa, float temperatureC);
bool DataLoggerLogImu(uint32_t tMs, const char* sensor, ImuChannel channel,
               float x, float y, float z);

// Sync and close at the end of a bench run, before removing power.
bool DataLoggerClose();
