#include "utils.h"
#include <cbor.h>
#include <string.h>
#include <time.h>

#define IS_NULL(x) ((x) == NULL)

void utils_timestamp_to_iso8601(uint64_t timestamp, char *buffer,
                                size_t buffer_size) {
  time_t raw_time = (time_t)timestamp;
  struct tm timeinfo;

  if (localtime_r(&raw_time, &timeinfo) == NULL) {
    snprintf(buffer, buffer_size, "Invalid timestamp");
    return;
  }

  if (strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%SZ", &timeinfo) == 0) {
    snprintf(buffer, buffer_size, "Formatting error");
    return;
  }
}

void bytes_to_hex_heading_0x(const uint8_t *buffer, size_t size, char *out) {
  out[0] = '0';
  out[1] = 'x';

  for (size_t i = 0; i < size; ++i) {
    sprintf(out + 2 + i * 2, "%02x", buffer[i]);
  }

  out[2 + size * 2] = '\0';
}