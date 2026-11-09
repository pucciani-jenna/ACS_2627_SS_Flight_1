#include "data_logger.h"
#include "settings.h"

#include <Arduino.h>
#include <SdFat.h>
#include <stdio.h>
#include <string.h>

namespace {
SdFs card;
FsFile logFile;
char filename[13];
uint32_t lastSyncUs = 0;
uint32_t bufferStartedUs = 0;
bool sessionStarted = false;
bool preallocated = false;
bool syncPending = false;
char buffer[settings::build::kDataLoggerBufferSize];
DataLoggerDiagnostics diagnostics;

constexpr char kHeader[] =
    "t_ms,sensor,channel,x,y,z,pressure_pa,temperature_c\n";

void failLogger() {
    logFile.close();
    diagnostics.bufferedBytes = 0;
    syncPending = false;
}

bool flushBuffer() {
    const size_t bytes = diagnostics.bufferedBytes;
    if (bytes == 0) {
        return true;
    }
    if (logFile.write(buffer, bytes) != bytes) {
        ++diagnostics.writeFailures;
        failLogger();
        return false;
    }
    diagnostics.bufferedBytes = 0;
    syncPending = true;
    return true;
}

bool writeRow(const char* row, int length, size_t capacity) {
    if (!logFile || length < 0 || static_cast<size_t>(length) >= capacity ||
        static_cast<size_t>(length) > sizeof(buffer)) {
        ++diagnostics.appendFailures;
        return false;
    }

    const size_t bytes = static_cast<size_t>(length);
    if (bytes > sizeof(buffer) - diagnostics.bufferedBytes && !flushBuffer()) {
        ++diagnostics.appendFailures;
        return false;
    }

    if (diagnostics.bufferedBytes == 0) {
        bufferStartedUs = micros();
    }
    memcpy(buffer + diagnostics.bufferedBytes, row, bytes);
    diagnostics.bufferedBytes += bytes;
    return true;
}
}  // namespace

bool DataLoggerBegin() {
    if (sessionStarted || !card.begin(SdioConfig(FIFO_SDIO))) {
        return false;
    }

    for (unsigned index = 0; index < 1000; ++index) {
        snprintf(filename, sizeof(filename), "SS1%03u.CSV", index);
        if (card.exists(filename)) {
            continue;
        }
        if (!logFile.open(filename, O_WRONLY | O_CREAT | O_EXCL)) {
            return false;
        }

        preallocated = settings::build::kDataLoggerPreallocateBytes != 0 &&
                       logFile.preAllocate(settings::build::kDataLoggerPreallocateBytes);

        if (logFile.write(kHeader, sizeof(kHeader) - 1) != sizeof(kHeader) - 1 ||
            !logFile.sync()) {
            logFile.close();
            return false;
        }

        lastSyncUs = micros();
        sessionStarted = true;
        return true;
    }
    return false;
}

const char* DataLoggerFilename() {
    return logFile ? filename : "";
}

bool DataLoggerLogPressure(uint32_t tMs, const char* sensor,
                           float pressurePa, float temperatureC) {
    char row[96];
    const int length = snprintf(row, sizeof(row), "%lu,%s,baro,,,,%.8g,%.8g\n",
                                static_cast<unsigned long>(tMs), sensor,
                                pressurePa, temperatureC);
    return writeRow(row, length, sizeof(row));
}

bool DataLoggerLogImu(uint32_t tMs, const char* sensor, ImuChannel channel,
                      float x, float y, float z) {
    const char* name;
    switch (channel) {
        case ImuChannel::LowG:
            name = "low_g_mps2";
            break;
        case ImuChannel::HighG:
            name = "high_g_mps2";
            break;
        case ImuChannel::Gyro:
            name = "gyro_radps";
            break;
        default:
            return false;
    }

    char row[96];
    const int length = snprintf(row, sizeof(row), "%lu,%s,%s,%.8g,%.8g,%.8g,,\n",
                                static_cast<unsigned long>(tMs), sensor,
                                name, x, y, z);
    return writeRow(row, length, sizeof(row));
}

bool DataLoggerLogTelemetry(const LoggedTelemetrySample& sample) {
    constexpr const char* barometerIds[] = {"A1", "A2", "A3"};
    constexpr const char* imuIds[] = {"I1", "I2", "I3"};
    constexpr ImuChannel channels[] = {
        ImuChannel::LowG, ImuChannel::HighG, ImuChannel::Gyro,
    };

    if (!logFile) {
        ++diagnostics.appendFailures;
        return false;
    }

    for (size_t i = 0; i < 3; ++i) {
        const PressureReading& pressure = sample.barometers[i];
        if (pressure.fresh &&
            !DataLoggerLogPressure(pressure.tMs, barometerIds[i],
                                   pressure.pressurePa, pressure.temperatureC)) {
            return false;
        }

        const ImuReading& imu = sample.imus[i];
        const VectorReading* paths[] = {&imu.lowG, &imu.highG, &imu.gyro};
        for (size_t path = 0; path < 3; ++path) {
            const VectorReading& reading = *paths[path];
            if (reading.fresh &&
                !DataLoggerLogImu(reading.tMs, imuIds[i], channels[path],
                                  reading.x, reading.y, reading.z)) {
                return false;
            }
        }
    }
    return true;
}

bool DataLoggerService() {
    if (!logFile) {
        return false;
    }

    const uint32_t nowUs = micros();
    if (nowUs - lastSyncUs >= settings::build::kDataLoggerSyncIntervalUs) {
        return DataLoggerForceSync();
    }
    if (diagnostics.bufferedBytes != 0 &&
        (diagnostics.bufferedBytes >= settings::build::kDataLoggerMinFlushBytes ||
         nowUs - bufferStartedUs >= settings::build::kDataLoggerFlushIntervalUs)) {
        return flushBuffer();
    }
    return true;
}

bool DataLoggerForceSync() {
    if (!logFile || !flushBuffer()) {
        return false;
    }
    if (syncPending && !logFile.sync()) {
        ++diagnostics.syncFailures;
        failLogger();
        return false;
    }

    syncPending = false;
    lastSyncUs = micros();
    return true;
}

bool DataLoggerClose() {
    if (!DataLoggerForceSync()) {
        return false;
    }

    // Trim any unused space at the end of the preallocated file.
    const bool trimmed = !preallocated || logFile.truncate();
    const bool closed = logFile.close();
    return trimmed && closed;
}

bool DataLoggerIsInitialized() {
    return static_cast<bool>(logFile);
}

DataLoggerDiagnostics DataLoggerGetDiagnostics() {
    DataLoggerDiagnostics status = diagnostics;
    status.initialized = DataLoggerIsInitialized();
    status.syncPending = syncPending || diagnostics.bufferedBytes != 0;
    return status;
}
