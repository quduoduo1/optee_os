/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#include <io.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/arm/mhu.h>
#include <drivers/arm/mhu_v3_x.h>
#include <drivers/arm/mhu_v3_x_private.h>

static bool valid_frame(enum mhu_v3_x_frame_t frame)
{
	return frame == MHU_V3_X_PBX_FRAME ||
	       frame == MHU_V3_X_MBX_FRAME;
}

static enum mhu_v3_x_error_t validate_device(
	const struct mhu_v3_x_dev_t *dev)
{
	if (dev == NULL || dev->base == 0)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	if (!dev->is_initialized)
		return MHU_V_3_X_ERR_NOT_INIT;

	if (!valid_frame(dev->frame) || dev->num_dbch == 0)
		return MHU_V_3_X_ERR_UNSUPPORTED;

	return MHU_V_3_X_ERR_NONE;
}

static enum mhu_v3_x_error_t validate_channel(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel)
{
	if (channel >= dev->num_dbch)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	return MHU_V_3_X_ERR_NONE;
}

static vaddr_t mhu_reg(const struct mhu_v3_x_dev_t *dev,
		       uint32_t offset)
{
	return (vaddr_t)dev->base + offset;
}

static vaddr_t dbch_reg(const struct mhu_v3_x_dev_t *dev,
			uint32_t channel,
			uint32_t offset)
{
	return mhu_reg(dev, MHU_V3_X_DBCH_WINDOW_OFFSET +
		       channel * MHU_V3_X_DBCH_STRIDE + offset);
}

enum mhu_v3_x_error_t mhu_v3_x_driver_init(
	struct mhu_v3_x_dev_t *dev)
{
	vaddr_t base;
	uint32_t aidr;
	uint32_t dbch_cfg;
	uint32_t major;
	uint32_t minor;
	uint32_t ctrl;

	if (dev == NULL || dev->base == 0)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	if (dev->is_initialized)
		return MHU_V_3_X_ERR_NONE;

	if (!valid_frame(dev->frame))
		return MHU_V_3_X_ERR_UNSUPPORTED;

	base = (vaddr_t)dev->base;
	aidr = io_read32(base + MHU_V3_X_AIDR_OFFSET);

	major = (aidr & MHU_ARCH_MAJOR_REV_MASK) >>
		MHU_ARCH_MAJOR_REV_OFF;
	if (major != MHU_MAJOR_REV_V3)
		return MHU_V_3_X_ERR_UNSUPPORTED_VERSION;

	minor = (aidr & MHU_ARCH_MINOR_REV_MASK) >>
		MHU_ARCH_MINOR_REV_OFF;
	if (minor != MHU_MINOR_REV_3_0)
		return MHU_V_3_X_ERR_UNSUPPORTED_VERSION;

	dev->subversion = minor;

	dbch_cfg = io_read32(base + MHU_V3_X_DBCH_CFG0_OFFSET);

	/*
	 * The public interface stores the channel count in uint8_t.
	 * Reject values that cannot be represented safely.
	 */
	if (dbch_cfg >= UINT8_MAX)
		return MHU_V_3_X_ERR_UNSUPPORTED;

	dev->num_dbch = (uint8_t)(dbch_cfg + 1U);
	if (dev->num_dbch == 0)
		return MHU_V_3_X_ERR_UNSUPPORTED;

	/*
	 * Keep the frame operational.
	 */
	ctrl = io_read32(base + MHU_V3_X_CTRL_OFFSET);
	io_write32(base + MHU_V3_X_CTRL_OFFSET,
		   ctrl | MHU_V3_OP_REQ);

	dev->is_initialized = true;

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_get_num_channel_implemented(
	const struct mhu_v3_x_dev_t *dev,
	enum mhu_v3_x_channel_type_t ch_type,
	uint8_t *num_ch)
{
	enum mhu_v3_x_error_t err;

	if (num_ch == NULL)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (ch_type != MHU_V3_X_CHANNEL_TYPE_DBCH)
		return MHU_V_3_X_ERR_UNSUPPORTED;

	*num_ch = dev->num_dbch;

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_doorbell_clear(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags)
{
	enum mhu_v3_x_error_t err;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame != MHU_V3_X_MBX_FRAME)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	/*
	 * MDBCW clear is a write-only register.
	 */
	io_write32(dbch_reg(dev, channel,
			    MHU_V3_X_MBX_DBCH_CLR_OFFSET),
		   flags);

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_doorbell_write(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags)
{
	enum mhu_v3_x_error_t err;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame != MHU_V3_X_PBX_FRAME)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	/*
	 * PDBCW set is a write-only register.
	 */
	io_write32(dbch_reg(dev, channel,
			    MHU_V3_X_PBX_DBCH_SET_OFFSET),
		   flags);

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_doorbell_read(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t *flags)
{
	enum mhu_v3_x_error_t err;

	if (flags == NULL)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame == MHU_V3_X_PBX_FRAME) {
		*flags = io_read32(dbch_reg(
			dev, channel, MHU_V3_X_PBX_DBCH_ST_OFFSET));
	} else if (dev->frame == MHU_V3_X_MBX_FRAME) {
		*flags = io_read32(dbch_reg(
			dev, channel, MHU_V3_X_MBX_DBCH_ST_OFFSET));
	} else {
		return MHU_V_3_X_ERR_UNSUPPORTED;
	}

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_doorbell_mask_set(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags)
{
	enum mhu_v3_x_error_t err;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame != MHU_V3_X_MBX_FRAME)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	io_write32(dbch_reg(dev, channel,
			    MHU_V3_X_MBX_DBCH_MSK_SET_OFFSET),
		   flags);

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_doorbell_mask_clear(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t flags)
{
	enum mhu_v3_x_error_t err;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame != MHU_V3_X_MBX_FRAME)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	io_write32(dbch_reg(dev, channel,
			    MHU_V3_X_MBX_DBCH_MSK_CLR_OFFSET),
		   flags);

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_doorbell_mask_get(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	uint32_t *flags)
{
	enum mhu_v3_x_error_t err;

	if (flags == NULL)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame != MHU_V3_X_MBX_FRAME)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	*flags = io_read32(dbch_reg(
		dev, channel, MHU_V3_X_MBX_DBCH_MSK_ST_OFFSET));

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_channel_interrupt_enable(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	enum mhu_v3_x_channel_type_t ch_type)
{
	enum mhu_v3_x_error_t err;
	uint32_t value;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (ch_type != MHU_V3_X_CHANNEL_TYPE_DBCH)
		return MHU_V_3_X_ERR_UNSUPPORTED;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame == MHU_V3_X_PBX_FRAME) {
		io_write32(dbch_reg(dev, channel,
				    MHU_V3_X_PBX_DBCH_INT_EN_OFFSET),
			   MHU_V3_X_PDBCW_INT_X_TFR_ACK);

		value = io_read32(dbch_reg(
			dev, channel, MHU_V3_X_PBX_DBCH_CTRL_OFFSET));
		io_write32(dbch_reg(
			dev, channel, MHU_V3_X_PBX_DBCH_CTRL_OFFSET),
			   value | MHU_V3_X_PDBCW_CTRL_PBX_COMB_EN);
	} else if (dev->frame == MHU_V3_X_MBX_FRAME) {
		value = io_read32(dbch_reg(
			dev, channel, MHU_V3_X_MBX_DBCH_CTRL_OFFSET));
		io_write32(dbch_reg(
			dev, channel, MHU_V3_X_MBX_DBCH_CTRL_OFFSET),
			   value | MHU_V3_X_MDBCW_CTRL_MBX_COMB_EN);
	} else {
		return MHU_V_3_X_ERR_UNSUPPORTED;
	}

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_channel_interrupt_disable(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	enum mhu_v3_x_channel_type_t ch_type)
{
	enum mhu_v3_x_error_t err;
	uint32_t value;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (ch_type != MHU_V3_X_CHANNEL_TYPE_DBCH)
		return MHU_V_3_X_ERR_UNSUPPORTED;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (dev->frame == MHU_V3_X_PBX_FRAME) {
		io_write32(dbch_reg(dev, channel,
				    MHU_V3_X_PBX_DBCH_INT_CLR_OFFSET),
			   MHU_V3_X_PDBCW_INT_X_TFR_ACK);

		value = io_read32(dbch_reg(
			dev, channel, MHU_V3_X_PBX_DBCH_INT_EN_OFFSET));
		io_write32(dbch_reg(
			dev, channel, MHU_V3_X_PBX_DBCH_INT_EN_OFFSET),
			   value & ~MHU_V3_X_PDBCW_INT_X_TFR_ACK);

		value = io_read32(dbch_reg(
			dev, channel, MHU_V3_X_PBX_DBCH_CTRL_OFFSET));
		io_write32(dbch_reg(
			dev, channel, MHU_V3_X_PBX_DBCH_CTRL_OFFSET),
			   value & ~MHU_V3_X_PDBCW_CTRL_PBX_COMB_EN);
	} else if (dev->frame == MHU_V3_X_MBX_FRAME) {
		value = io_read32(dbch_reg(
			dev, channel, MHU_V3_X_MBX_DBCH_CTRL_OFFSET));
		io_write32(dbch_reg(
			dev, channel, MHU_V3_X_MBX_DBCH_CTRL_OFFSET),
			   value & ~MHU_V3_X_MDBCW_CTRL_MBX_COMB_EN);
	} else {
		return MHU_V_3_X_ERR_UNSUPPORTED;
	}

	return MHU_V_3_X_ERR_NONE;
}

enum mhu_v3_x_error_t mhu_v3_x_channel_interrupt_clear(
	const struct mhu_v3_x_dev_t *dev,
	uint32_t channel,
	enum mhu_v3_x_channel_type_t ch_type)
{
	enum mhu_v3_x_error_t err;

	err = validate_device(dev);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	if (ch_type != MHU_V3_X_CHANNEL_TYPE_DBCH)
		return MHU_V_3_X_ERR_UNSUPPORTED;

	if (dev->frame != MHU_V3_X_PBX_FRAME)
		return MHU_V_3_X_ERR_INVALID_PARAM;

	err = validate_channel(dev, channel);
	if (err != MHU_V_3_X_ERR_NONE)
		return err;

	io_write32(dbch_reg(dev, channel,
			    MHU_V3_X_PBX_DBCH_INT_CLR_OFFSET),
		   MHU_V3_X_PDBCW_INT_X_TFR_ACK);

	return MHU_V_3_X_ERR_NONE;
}