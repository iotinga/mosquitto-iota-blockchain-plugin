#include "plugin.h"

#include <stdio.h>
#include <string.h>

#include "blockchain.h"
#include "mosquitto.h"
#include "mosquitto_broker.h"
#include "mosquitto_plugin.h"
#include "mqtt_protocol.h"
#include "utils.h"
#include <cbor.h>
#include <sys/time.h>

#define UNUSED(A) (void)(A)

static const char *ENTITY = "MOSQUITTO_MQTT_BROKER";

static mosquitto_plugin_id_t *mosq_pid = NULL;

static int error_code_to_mosquitto_error(error_code error) {
  switch (error) {
  case SUCCESS:
    return MOSQ_ERR_SUCCESS;
  case ERROR_INVALID_ARGUMENT:
    return MOSQ_ERR_INVAL;
  case ERROR_NO_MEMORY:
    return MOSQ_ERR_NOMEM;
  default:
    return MOSQ_ERR_UNKNOWN;
  }
}

static void load_configuration(plugin_config *config,
                               struct mosquitto_opt *opts, int opt_count) {
  for (size_t i = 0; i < opt_count; i++) {
    char *key = opts[i].key;
    char *value = opts[i].value;

    if (strcmp(key, "iota_network_endpoint") == 0) {
      config->iota_network_endpoint = value;
    } else {
      mosquitto_log_printf(MOSQ_LOG_WARNING,
                           "Unexpected configuration key (%s), ignoring it",
                           key);
    }
  }
}

static int callback_message(int event, void *event_data, void *userdata) {
  UNUSED(event);
  struct timeval tv;
  gettimeofday(&tv, NULL);

  plugin_config *config = (plugin_config *)userdata;
  struct mosquitto_evt_message *ed = (struct mosquitto_evt_message *)event_data;

  struct cbor_load_result load_result;
  cbor_item_t *cbor_map = cbor_load(ed->payload, ed->payloadlen, &load_result);

  if (load_result.error.code != CBOR_ERR_NONE) {
    mosquitto_log_printf(MOSQ_LOG_ERR, "Error loading CBOR data: %d",
                         load_result.error.code);
    return -1;
  }

  if (!cbor_isa_map(cbor_map)) {
    mosquitto_log_printf(MOSQ_LOG_ERR, "CBOR item is not a map");
    cbor_decref(&cbor_map);
    return -1;
  }

  if (!cbor_map_is_indefinite(cbor_map)) {
    mosquitto_log_printf(MOSQ_LOG_ERR, "CBOR map is not indefinite");
    cbor_decref(&cbor_map);
    return -1;
  }

  cbor_item_t *ingestion_time_key = cbor_build_string("INGESTION_TIME");
  cbor_item_t *ingestion_time_value =
      cbor_build_uint64(tv.tv_sec * 1000 + tv.tv_usec / 1000);
  struct cbor_pair ingestion_time_pair = {.key = ingestion_time_key,
                                          .value = ingestion_time_value};

  if (!cbor_map_add(cbor_map, ingestion_time_pair)) {
    mosquitto_log_printf(MOSQ_LOG_ERR, "Failed to add INGESTION TIME");
    cbor_decref(&cbor_map);
    cbor_decref(&ingestion_time_key);
    cbor_decref(&ingestion_time_value);
    return -1;
  }

  char block_id[128]; // It should be 64B + 2B of heading "0x" but let's be safe
                      // and double it
  error_code error = blockchain_insert_block(ed->payload, ed->payloadlen,
                                             block_id, sizeof(block_id));

  if (error != SUCCESS) {
    mosquitto_log_printf(MOSQ_LOG_ERR,
                         "Failed to make insert block on the blockchain %d",
                         error);
    cbor_decref(&cbor_map);
    return error_code_to_mosquitto_error(error);
  }

  cbor_item_t *signature_item = NULL;
  signature_item = cbor_build_string(block_id);
  if (signature_item == NULL) {
    cbor_decref(&cbor_map);
    return ERROR_NO_MEMORY;
  }

  // Add the signature to the map
  struct cbor_pair new_pair = {.key = cbor_build_string("VERIFICATION_TOKEN"),
                               .value = signature_item};
  if (!cbor_map_add(cbor_map, new_pair)) {
    cbor_decref(&signature_item);
    cbor_decref(&cbor_map);
    return ERROR_UNKNOWN;
  }

  // Serialize the updated CBOR map
  unsigned char *final_buffer = NULL;
  size_t final_size = 0;
  final_size = cbor_serialize_alloc(cbor_map, &final_buffer, &final_size);
  if (final_buffer == NULL) {
    mosquitto_log_printf(MOSQ_LOG_ERR, "Failed to serialize updated CBOR map");
    cbor_decref(&cbor_map);
    return -1;
  }

  // Allocate output buffer using mosquitto_calloc
  uint8_t *new_payload = (uint8_t *)mosquitto_calloc(1, final_size);
  if (new_payload == NULL) {
    mosquitto_log_printf(MOSQ_LOG_ERR, "Failed to allocate output buffer");
    cbor_decref(&cbor_map);
    free(final_buffer);
    return MOSQ_ERR_NOMEM;
  }

  // Copy the final serialized data to the output buffer
  memcpy(new_payload, final_buffer, final_size);

  /* Assign the new payload and payloadlen to the event data structure. You
   * must *not* free the original payload, it will be handled by the
   * broker. */
  ed->payload = new_payload;
  ed->payloadlen = final_size;

  cbor_decref(&cbor_map);
  free(final_buffer);
  return MOSQ_ERR_SUCCESS;
}

int mosquitto_plugin_version(int supported_version_count,
                             const int *supported_versions) {
  int i;

  for (i = 0; i < supported_version_count; i++) {
    if (supported_versions[i] == 5) {
      return 5;
    }
  }

  return -1;
}

int mosquitto_plugin_init(mosquitto_plugin_id_t *identifier, void **user_data,
                          struct mosquitto_opt *opts, int opt_count) {
  mosq_pid = identifier;

  /* Init session data */
  *user_data = mosquitto_malloc(sizeof(plugin_config));
  if (*user_data == NULL) {
    mosquitto_log_printf(MOSQ_LOG_ERR,
                         "Failed to allocate memory for plugin config");
    return MOSQ_ERR_NOMEM;
  }

  plugin_config *config = (plugin_config *)*user_data;
  load_configuration(config, opts, opt_count);

  error_code error = blockchain_init(config->iota_network_endpoint);
  if (error) {
    mosquitto_log_printf(MOSQ_LOG_ERR,
                         "Failed to initialize blockchain, error: %d", error);
    return MOSQ_ERR_UNKNOWN;
  }

  return mosquitto_callback_register(mosq_pid, MOSQ_EVT_MESSAGE,
                                     callback_message, NULL, config);
}

int mosquitto_plugin_cleanup(void *user_data, struct mosquitto_opt *opts,
                             int opt_count) {
  UNUSED(opts);
  UNUSED(opt_count);

  if (user_data != NULL) {
    mosquitto_free(user_data);
  }

  blockchain_deinit();

  return mosquitto_callback_unregister(mosq_pid, MOSQ_EVT_MESSAGE,
                                       callback_message, NULL);
}