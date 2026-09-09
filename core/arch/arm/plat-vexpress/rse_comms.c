// SPDX-License-Identifier: BSD-3-Clause
/*
 * RSE COMMS wire layouts based on TF-M:
 * Copyright (c) 2022-2024, Arm Limited. All rights reserved.
 */
#include <assert.h>
#include <compiler.h>
#include <drivers/arm/mhu.h>
#include <initcall.h>
#include <inttypes.h>
#include <kernel/mutex.h>
#include <string.h>
#include <tee_api_defines.h>
#include <trace.h>

#include "rse_comms.h"

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
#error "This RSE COMMS implementation requires a little-endian AP"
#endif

#define RSE_PROTOCOL_EMBED	0U
#define RSE_CLIENT_ID		1U

/* Local limit for the initial communication test. */
#define RSE_PAYLOAD_MAX		256U

struct rse_header {
	uint8_t protocol_ver;
	uint8_t seq_num;
	uint16_t client_id;
} __packed;

struct rse_request {
	struct rse_header header;
	int32_t handle;
	uint32_t ctrl_param;
	uint16_t io_size[PSA_MAX_IOVEC];
	uint8_t payload[RSE_PAYLOAD_MAX];
} __packed;

struct rse_reply {
	struct rse_header header;
	int32_t status;
	uint16_t out_size[PSA_MAX_IOVEC];
	uint8_t payload[RSE_PAYLOAD_MAX];
} __packed;

static_assert(offsetof(struct rse_request, payload) == 20,
	      "request layout");
static_assert(offsetof(struct rse_reply, payload) == 16,
	      "reply layout");

static struct mutex rse_lock = MUTEX_INITIALIZER;
static uint8_t next_seq = 1;

psa_status_t psa_call(psa_handle_t handle, int32_t type,
		     const psa_invec *in_vec, size_t in_len,
		     psa_outvec *out_vec, size_t out_len)
{
	union {
		struct rse_request request;
		struct rse_reply reply;
	} io __aligned(4) = { };
	psa_status_t status = PSA_ERROR_COMMUNICATION_FAILURE;
	size_t total = 0;
	size_t offset = 0;
	size_t tx_size = 0;
	size_t rx_size = sizeof(io.reply);
	size_t payload_size = 0;
	size_t i = 0;
	uint8_t seq = 0;

	if (handle <= 0 || type < 0 || type > INT16_MAX ||
	    in_len > PSA_MAX_IOVEC || out_len > PSA_MAX_IOVEC ||
	    in_len + out_len > PSA_MAX_IOVEC ||
	    (in_len && !in_vec) || (out_len && !out_vec))
		return PSA_ERROR_INVALID_ARGUMENT;

	for (i = 0; i < in_len; i++) {
		if (in_vec[i].len && !in_vec[i].base)
			return PSA_ERROR_INVALID_ARGUMENT;
		if (in_vec[i].len > RSE_PAYLOAD_MAX - total)
			return PSA_ERROR_NOT_SUPPORTED;
		total += in_vec[i].len;
	}

	/*
	 * TF-M's Embed receiver reserves space for both the inputs and
	 * the requested output capacities in its parameter buffer.
	 */
	for (i = 0; i < out_len; i++) {
		if (out_vec[i].len && !out_vec[i].base)
			return PSA_ERROR_INVALID_ARGUMENT;
		if (out_vec[i].len > RSE_PAYLOAD_MAX - total)
			return PSA_ERROR_NOT_SUPPORTED;
		total += out_vec[i].len;
	}

	io.request.header.protocol_ver = RSE_PROTOCOL_EMBED;
	io.request.header.client_id = RSE_CLIENT_ID;
	io.request.handle = handle;

	/*
	 * TF-M PARAM_PACK:
	 * type    : bits [15:0]
	 * out_len : bits [18:16]
	 * in_len  : bits [26:24]
	 */
	io.request.ctrl_param = (uint32_t)type |
				((uint32_t)in_len << 24) |
				((uint32_t)out_len << 16);

	for (i = 0; i < in_len; i++) {
		io.request.io_size[i] = in_vec[i].len;
		if (in_vec[i].len)
			memcpy(io.request.payload + offset, in_vec[i].base,
			       in_vec[i].len);
		offset += in_vec[i].len;
	}

	/* Output sizes follow the input sizes in io_size[]. */
	for (i = 0; i < out_len; i++)
		io.request.io_size[in_len + i] = out_vec[i].len;

	tx_size = offsetof(struct rse_request, payload) + offset;

	/* Serialize the complete transaction on this MHU pair. */
	mutex_lock(&rse_lock);
	seq = next_seq++;
	io.request.header.seq_num = seq;

	if (mhu_send_data((const uint8_t *)&io.request, tx_size) !=
	    MHU_ERR_NONE)
		goto out;

	memset(&io, 0, sizeof(io));
	if (mhu_receive_data((uint8_t *)&io.reply, &rx_size) != MHU_ERR_NONE)
		goto out;

	if (rx_size < offsetof(struct rse_reply, payload) ||
	    rx_size > sizeof(io.reply) ||
	    io.reply.header.protocol_ver != RSE_PROTOCOL_EMBED ||
	    io.reply.header.seq_num != seq ||
	    io.reply.header.client_id != RSE_CLIENT_ID)
		goto out;

	payload_size = rx_size - offsetof(struct rse_reply, payload);
	offset = 0;

	/* Validate every output before modifying the caller's buffers. */
	for (i = 0; i < out_len; i++) {
		size_t len = io.reply.out_size[i];

		if (len > out_vec[i].len || len > payload_size - offset)
			goto out;
		offset += len;
	}
	if (offset != payload_size)
		goto out;

	offset = 0;
	for (i = 0; i < out_len; i++) {
		size_t len = io.reply.out_size[i];

		if (len)
			memcpy(out_vec[i].base, io.reply.payload + offset, len);
		out_vec[i].len = len;
		offset += len;
	}

	status = io.reply.status;
out:
	mutex_unlock(&rse_lock);
	return status;
}

#ifdef CFG_RSE_COMMS_TEST
/* TF-M's generated psa_manifest/sid.h and tfm_attest_defs.h. */
#define TFM_ATTESTATION_SERVICE_HANDLE	0x40000103
#define TFM_ATTEST_GET_TOKEN_SIZE	1002

static TEE_Result rse_comms_test(void)
{
	uint32_t challenge_size = 32;
	uint32_t token_size = 0;
	psa_invec in = { &challenge_size, sizeof(challenge_size) };
	psa_outvec out = { &token_size, sizeof(token_size) };
	psa_status_t status = PSA_SUCCESS;

	status = psa_call(TFM_ATTESTATION_SERVICE_HANDLE,
			  TFM_ATTEST_GET_TOKEN_SIZE, &in, 1, &out, 1);
	if (status != PSA_SUCCESS) {
		EMSG("RSE token-size query failed: %" PRId32, status);
		return TEE_ERROR_GENERIC;
	}

	if (out.len != sizeof(token_size) || !token_size) {
		EMSG("Invalid RSE token-size reply");
		return TEE_ERROR_BAD_FORMAT;
	}

	IMSG("RSE communication OK: token_size=%" PRIu32, token_size);

	return TEE_SUCCESS;
}
driver_init_late(rse_comms_test);
#endif /* CFG_RSE_COMMS_TEST */