#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>

#include "blockchain.h"
#include <cmocka.h>

static void multiple_insertions(void **state) {
  error_code error = blockchain_init("https://api.testnet.iotaledger.net");
  assert_int_equal(SUCCESS, error);

  char block_id[256] = {0};
  char heading0x[] = "0x";

  char message0[] = "Hello World!";
  error = blockchain_insert_block(message0, sizeof(message0), block_id,
                                  sizeof(block_id));
  assert_int_equal(SUCCESS, error);
  assert_memory_equal(heading0x, block_id, 2u);

  char message1[] = "Blockchain";
  error = blockchain_insert_block(message1, sizeof(message1), block_id,
                                  sizeof(block_id));
  assert_int_equal(SUCCESS, error);
  assert_memory_equal(heading0x, block_id, 2u);

  char message3[] = "Running tests";
  error = blockchain_insert_block(message3, sizeof(message3), block_id,
                                  sizeof(block_id));
  assert_int_equal(SUCCESS, error);
  assert_memory_equal(heading0x, block_id, 2u);

  blockchain_deinit();
}

// Main function to run tests
int main(void) {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(multiple_insertions),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}