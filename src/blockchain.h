#pragma once
#include "error.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Initializes the blockchain module by checking if all needed
 * resources are available
 *
 * \param network_endpoint endpoint to connect to
 * \returns success upon finding all dependencies, error otherwise
 */
error_code blockchain_init(const char *network_endpoint);

/**
 * Inserts given data on the blockchain as tagged payload (see IOTA tagged
 * payload)
 *
 * \param buffer binary data to insert in the blockchain
 * \param buffer_size size of the data
 * \param[out] block_id created block id
 * \param[out] block_id_size size of the block id
 * \returns succes on insertion of the block, error otherwise
 */
error_code blockchain_insert_block(const uint8_t *buffer, size_t buffer_size,
                                   char *block_id, size_t block_id_size);

/**
 * Deinitializes the blockchain
 */
void blockchain_deinit(void);