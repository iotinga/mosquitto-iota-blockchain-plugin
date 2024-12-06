#include "blockchain.h"
#include "mosquitto.h"
#include "mosquitto_broker.h"
#include "utils.h"
#include <python3.12/Python.h>

static PyObject *iota_client_instance;

static int check_node_health(const char *network_endpoint) {
  PyObject *get_health =
      PyObject_GetAttrString(iota_client_instance, "get_health");
  if (!get_health || !PyCallable_Check(get_health)) {
    Py_XDECREF(get_health);
    return 0; // Not healthy if the function is invalid
  }

  PyObject *args = PyTuple_New(1);
  PyObject *endpoint = PyUnicode_FromString(network_endpoint);
  PyTuple_SetItem(args, 0, endpoint); // Do NOT decrement endpoint after this

  PyObject *result = PyObject_CallObject(get_health, args);
  int is_healthy = result ? PyObject_IsTrue(result) : 0;

  Py_XDECREF(result);
  Py_DECREF(args);
  Py_DECREF(get_health);

  return is_healthy;
}

static error_code build_and_post_block(const char *hex_tag,
                                       const char *hex_data, char *block_id,
                                       size_t block_id_size) {
  PyObject *tag = PyUnicode_FromString(hex_tag);
  PyObject *data = PyUnicode_FromString(hex_data);
  PyObject *p_build_and_post_block =
      PyObject_GetAttrString(iota_client_instance, "build_and_post_block");

  if (!p_build_and_post_block || !PyCallable_Check(p_build_and_post_block)) {
    Py_XDECREF(tag);
    Py_XDECREF(data);
    Py_XDECREF(p_build_and_post_block);
    return ERROR_NOT_FOUND;
  }

  PyObject *args = PyTuple_New(0);
  PyObject *kwargs = PyDict_New();
  PyDict_SetItemString(kwargs, "tag", tag);
  PyDict_SetItemString(kwargs, "data", data);

  PyObject *block = PyObject_Call(p_build_and_post_block, args, kwargs);
  error_code error = SUCCESS;

  if (!block) {
    PyErr_Print();
    error = ERROR_UNKNOWN;
  } else {
    PyObject *p_block_id = PyList_GetItem(block, 0); // Borrowed reference
    if (!p_block_id) {
      error = ERROR_UNKNOWN;
    } else {
      PyObject *block_id_str = PyObject_Str(p_block_id);
      if (!block_id_str) {
        error = ERROR_UNKNOWN;
      } else {
        snprintf(block_id, block_id_size, "%s", PyUnicode_AsUTF8(block_id_str));
        Py_DECREF(block_id_str);
      }
    }
  }

  Py_DECREF(tag);
  Py_DECREF(data);
  Py_DECREF(p_build_and_post_block);
  Py_DECREF(args);
  Py_DECREF(kwargs);

  if (block) {
    Py_DECREF(block); // Only if block is not NULL
  }

  return error;
}

error_code blockchain_init(const char *network_endpoint) {
  Py_Initialize();
  PyObject *iota_module = PyImport_ImportModule("iota_sdk");
  if (iota_module == NULL) {
    mosquitto_log_printf(MOSQ_LOG_ERR, "IOTA SDK not found!");
    return ERROR_NOT_FOUND;
  }

  PyObject *iota_client_class = PyObject_GetAttrString(iota_module, "Client");
  if ((iota_client_class == NULL) || !PyCallable_Check(iota_client_class)) {
    Py_DECREF(iota_module);
    return ERROR_NOT_FOUND;
  }

  PyObject *nodes = PyList_New(1);
  PyObject *node_url = PyUnicode_FromString(network_endpoint);
  PyList_SetItem(nodes, 0, node_url);

  PyObject *localPow = PyBool_FromLong(1);

  PyObject *kwargs = PyDict_New();
  PyDict_SetItemString(kwargs, "nodes", nodes);
  PyDict_SetItemString(kwargs, "local_pow", localPow);

  PyObject *args = PyTuple_New(0); // No positional arguments
  iota_client_instance = PyObject_Call(iota_client_class, args, kwargs);

  error_code error = SUCCESS;
  if (iota_client_instance == NULL || !check_node_health(network_endpoint)) {
    PyErr_Print();
    error = ERROR_UNKNOWN;
  }

  Py_DECREF(iota_module);
  Py_DECREF(iota_client_class);
  Py_DECREF(kwargs);
  Py_DECREF(args);
  return error;
}

error_code blockchain_insert_block(const uint8_t *buffer, size_t buffer_size,
                                   char *block_id, size_t block_id_size) {
  const char tag[] = "SENSOR_DATA";
  char hex_tag[(sizeof(tag) - 1u) * 2 + 1u] = {0};
  char hex_data[1024] = {0};

  bytes_to_hex_heading_0x((const uint8_t *)tag, sizeof(tag) - 1u, hex_tag);
  bytes_to_hex_heading_0x(buffer, buffer_size, hex_data);

  return build_and_post_block(hex_tag, hex_data, block_id, block_id_size);
}

void blockchain_deinit(void) {
  if (Py_IsInitialized()) {
    Py_XDECREF(iota_client_instance);
    iota_client_instance = NULL;
    Py_Finalize();
  }
}