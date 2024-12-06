#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include <cmocka.h>

#include <cbor.h>

#include "utils.h"

static void test_utils_iso_timestamp(void **state) {
  uint64_t unix_seconds = 1733393632;
  char iso_string[64];
  utils_timestamp_to_iso8601(unix_seconds, iso_string, sizeof(iso_string));
  assert_string_equal(iso_string, "2024-12-05T11:13:52Z");
}

static void test_utils_bytes_to_hex(void **state) {
  char *bytes = "Hello World!";
  char hex[100] = {0};
  bytes_to_hex_heading_0x(bytes, strlen(bytes), hex);

  assert_string_equal(hex, "0x48656c6c6f20576f726c6421");
}

// Main function to run tests
int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_utils_iso_timestamp),
      cmocka_unit_test(test_utils_bytes_to_hex),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
