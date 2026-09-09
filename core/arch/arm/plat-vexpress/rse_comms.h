/* SPDX-License-Identifier: BSD-3-Clause */
#ifndef VEXPRESS_RSE_COMMS_H
#define VEXPRESS_RSE_COMMS_H

#include <stddef.h>
#include <stdint.h>

typedef int32_t psa_status_t;
typedef int32_t psa_handle_t;

typedef struct {
	const void *base;
	size_t len;
} psa_invec;

typedef struct {
	void *base;
	size_t len;
} psa_outvec;

#define PSA_MAX_IOVEC				4U
#define PSA_SUCCESS				0
#define PSA_ERROR_NOT_SUPPORTED			(-134)
#define PSA_ERROR_INVALID_ARGUMENT		(-135)
#define PSA_ERROR_COMMUNICATION_FAILURE		(-145)

psa_status_t psa_call(psa_handle_t handle, int32_t type,
		     const psa_invec *in_vec, size_t in_len,
		     psa_outvec *out_vec, size_t out_len);

#endif /* VEXPRESS_RSE_COMMS_H */