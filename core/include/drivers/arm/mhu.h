/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, Arm Limited and Contributors. All rights reserved.
 */

#ifndef __DRIVERS_ARM_MHU_H
#define __DRIVERS_ARM_MHU_H

#include <stddef.h>
#include <stdint.h>

enum mhu_error_t {
	MHU_ERR_NONE = 0,
	MHU_ERR_NOT_INIT = -1,
	MHU_ERR_ALREADY_INIT = -2,
	MHU_ERR_UNSUPPORTED_VERSION = -3,
	MHU_ERR_UNSUPPORTED = -4,
	MHU_ERR_INVALID_ARG = -5,
	MHU_ERR_BUFFER_TOO_SMALL = -6,
	MHU_ERR_GENERAL = -7,
};

/*
 * The platform provides the two mapped MHU frame addresses.
 */
struct mhu_addr {
	uintptr_t sender_base;
	uintptr_t receiver_base;
};

enum mhu_error_t mhu_init_sender(uintptr_t mhu_sender_base);

enum mhu_error_t mhu_init_receiver(uintptr_t mhu_receiver_base);

enum mhu_error_t mhu_send_data(
	const uint8_t *send_buffer,
	size_t size);

enum mhu_error_t mhu_receive_data(
	uint8_t *receive_buffer,
	size_t *size);

size_t mhu_get_max_message_size(void);

#endif /* __DRIVERS_ARM_MHU_H */