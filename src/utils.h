#pragma once
#include "error.h"
#include <cbor.h>
#include <stddef.h>

/**
 * Converts unix timestamp (in seconds) into ISO8601 string
 *
 * \param timestamp unix seconds
 * \param buffer out buffer for the ISO8601 string
 * \param buffer_size size of the buffer
 */
void utils_timestamp_to_iso8601(uint64_t timestamp, char *buffer,
                                size_t buffer_size);

/**
 * Converts bytes to hex string with heading 0x
 *
 * \param buffer bytes to convert
 * \param size size of buffer
 * \param out out string
 */
void bytes_to_hex_heading_0x(const uint8_t *buffer, size_t size, char *out);
