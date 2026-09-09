/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#ifndef __DRIVERS_ARM_MHU_V3_X_H
#define __DRIVERS_ARM_MHU_V3_X_H

#include <stdbool.h>
#include <stdint.h>

/*
 * MHUv3 AIDR encoding.
 * The architectural major revision value for MHUv3 is 2.
 */
#define MHU_MAJOR_REV_V3			UINT32_C(0x2)
#define MHU_MINOR_REV_3_0			UINT32_C(0x0)

#define MHU_ARCH_MAJOR_REV_OFF			4U
#define MHU_ARCH_MAJOR_REV_MASK			(UINT32_C(0xf) << \
						 MHU_ARCH_MAJOR_REV_OFF)

#define MHU_ARCH_MINOR_REV_OFF			0U
#define MHU_ARCH_MINOR_REV_MASK			(UINT32_C(0xf) << \
						 MHU_ARCH_MINOR_REV_OFF)

#define MHU_V3_OP_REQ				UINT32_C(0x1)

/*
 * The control page is followed by the doorbell channel window page.
 * This size is also sufficient for the MHUv3 frame used by the wrapper.
 */
#define MHU_V3_X_FRAME_SIZE			UINT32_C(0x2000)

enum mhu_v3_x_error_t {
	MHU_V_3_X_ERR_NONE = 0,
	MHU_V_3_X_ERR_NOT_INIT,
	MHU_V_3_X_ERR_ALREADY_INIT,
	MHU_V_3_X_ERR_UNSUPPORTED_VERSION,
	MHU_V_3_X_ERR_UNSUPPORTED,
	MHU_V_3_X_ERR_INVALID_PARAM,
	MHU_V_3_X_ERR_GENERAL,
};

enum mhu_v3_x_channel_type_t {
	MHU_V3_X_CHANNEL_TYPE_DBCH = 0,
	MHU_V3_X_CHANNEL_TYPE_COUNT,
};

enum mhu_v3_x_frame_t {
	MHU_V3_X_PBX_FRAME = 0,
	MHU_V3_X_MBX_FRAME,
};

struct mhu_v3_x_dev_t {
	/*
	 * Runtime virtual address of the MHU frame.
	 * The platform is responsible for registering/mapping the MMIO region.
	 */
	uintptr_t base;

	enum mhu_v3_x_frame_t frame;

	uint32_t subversion;
	uint8_t num_dbch;

	bool is_initialized;
};

enum mhu_v3_x_error_t mhu_v3_x_driver_init(
	struct mhu_v3_x_dev_t *dev);

enum mhu_v3_x_error_t mhu_v3_x_get_num_channel_implemented(
	const struct mhu_v3_x_dev_t *dev,
	enum mhu_v3_x_channel_type_t ch_type,
	uint8_t *num_ch);

enum mhu_v3_x_error_t mhu_v3_x_doorbell_clear(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags);

enum mhu_v3_x_error_t mhu_v3_x_doorbell_write(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags);

enum mhu_v3_x_error_t mhu_v3_x_doorbell_read(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t *flags);

enum mhu_v3_x_error_t mhu_v3_x_doorbell_mask_set(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags);

enum mhu_v3_x_error_t mhu_v3_x_doorbell_mask_clear(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags);

enum mhu_v3_x_error_t mhu_v3_x_doorbell_mask_get(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t *flags);

enum mhu_v3_x_error_t mhu_v3_x_channel_interrupt_enable(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	enum mhu_v3_x_channel_type_t ch_type);

enum mhu_v3_x_error_t mhu_v3_x_channel_interrupt_disable(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	enum mhu_v3_x_channel_type_t ch_type);

enum mhu_v3_x_error_t mhu_v3_x_channel_interrupt_clear(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	enum mhu_v3_x_channel_type_t ch_type);

#endif /* __DRIVERS_ARM_MHU_V3_X_H */