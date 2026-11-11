#pragma once

#include <stddef.h>
#include <stdint.h>

#ifndef DATA_LOGGER_BUFFER_SIZE
#define DATA_LOGGER_BUFFER_SIZE 32768
#endif

#ifndef DATA_LOGGER_FLUSH_INTERVAL_US
#define DATA_LOGGER_FLUSH_INTERVAL_US 200000
#endif

#ifndef DATA_LOGGER_SYNC_INTERVAL_US
#define DATA_LOGGER_SYNC_INTERVAL_US 2000000
#endif

#ifndef DATA_LOGGER_MIN_FLUSH_BYTES
#define DATA_LOGGER_MIN_FLUSH_BYTES 4096
#endif

#ifndef DATA_LOGGER_PREALLOCATE_BYTES
#define DATA_LOGGER_PREALLOCATE_BYTES 1048576
#endif

namespace settings {
namespace build {

constexpr size_t kDataLoggerBufferSize = DATA_LOGGER_BUFFER_SIZE;
constexpr uint32_t kDataLoggerFlushIntervalUs = DATA_LOGGER_FLUSH_INTERVAL_US;
constexpr uint32_t kDataLoggerSyncIntervalUs = DATA_LOGGER_SYNC_INTERVAL_US;
constexpr size_t kDataLoggerMinFlushBytes = DATA_LOGGER_MIN_FLUSH_BYTES;
constexpr uint32_t kDataLoggerPreallocateBytes = DATA_LOGGER_PREALLOCATE_BYTES;

static_assert(kDataLoggerBufferSize >= 96, "Buffer must hold a CSV row");
static_assert(kDataLoggerMinFlushBytes > 0 &&
              kDataLoggerMinFlushBytes <= kDataLoggerBufferSize,
              "Flush threshold must fit the buffer");

}  // namespace build
}  // namespace settings