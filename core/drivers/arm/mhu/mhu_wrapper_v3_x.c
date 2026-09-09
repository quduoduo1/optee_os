/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024-2026, Arm Limited and Contributors. All rights reserved.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <drivers/arm/mhu.h>

#include <drivers/arm/mhu_v3_x.h>

#define MHU_NOTIFY_VALUE	UINT32_C(1234)

static struct mhu_v3_x_dev_t mhu_sender_dev = {
	.frame = MHU_V3_X_PBX_FRAME,
};

static struct mhu_v3_x_dev_t mhu_receiver_dev = {
	.frame = MHU_V3_X_MBX_FRAME,
};

static enum mhu_error_t map_error(
	enum mhu_v3_x_error_t error)
{
	switch (error) {
	case MHU_V_3_X_ERR_NONE:
		return MHU_ERR_NONE;
	case MHU_V_3_X_ERR_NOT_INIT:
		return MHU_ERR_NOT_INIT;
	case MHU_V_3_X_ERR_UNSUPPORTED_VERSION:
		return MHU_ERR_UNSUPPORTED_VERSION;
	case MHU_V_3_X_ERR_UNSUPPORTED:
		return MHU_ERR_UNSUPPORTED;
	case MHU_V_3_X_ERR_INVALID_PARAM:
		return MHU_ERR_INVALID_ARG;
	default:
		return MHU_ERR_GENERAL;
	}
}

static enum mhu_error_t get_num_channels(
	struct mhu_v3_x_dev_t *dev,
	uint8_t *num_channels)
{
	enum mhu_v3_x_error_t error;

	error = mhu_v3_x_get_num_channel_implemented(
		dev, MHU_V3_X_CHANNEL_TYPE_DBCH, num_channels);

	return map_error(error);
}

static enum mhu_error_t signal_and_wait_for_clear(
	struct mhu_v3_x_dev_t *dev,
	uint32_t value)
{
	enum mhu_v3_x_error_t error;
	uint8_t num_channels;
	uint32_t read_value;

	if (dev == NULL || dev->base == 0)
		return MHU_ERR_INVALID_ARG;

	if (get_num_channels(dev, &num_channels) != MHU_ERR_NONE)
		return MHU_ERR_GENERAL;

	if (num_channels < 2)
		return MHU_ERR_UNSUPPORTED;

	/*
	 * Wait until the previous acknowledgement has disappeared.
	 */
	do {
		error = mhu_v3_x_doorbell_read(
			dev, num_channels - 1, &read_value);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);
	} while ((read_value & value) == value);

	/*
	 * Notify the receiver that the current transfer window is ready.
	 */
	error = mhu_v3_x_doorbell_write(
		dev, num_channels - 1, value);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	/*
	 * Wait until the receiver acknowledges the transfer.
	 */
	do {
		error = mhu_v3_x_doorbell_read(
			dev, num_channels - 1, &read_value);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);
	} while ((read_value & value) == value);

	return MHU_ERR_NONE;
}

static enum mhu_error_t wait_for_signal(
	struct mhu_v3_x_dev_t *dev,
	uint32_t value)
{
	enum mhu_v3_x_error_t error;
	uint8_t num_channels;
	uint32_t read_value;

	if (dev == NULL || dev->base == 0)
		return MHU_ERR_INVALID_ARG;

	error = mhu_v3_x_get_num_channel_implemented(
		dev, MHU_V3_X_CHANNEL_TYPE_DBCH, &num_channels);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	do {
		error = mhu_v3_x_doorbell_read(
			dev, num_channels - 1, &read_value);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);
	} while ((read_value & value) != value);

	return MHU_ERR_NONE;
}

static enum mhu_error_t clear_and_wait_for_signal(
	struct mhu_v3_x_dev_t *dev,
	uint32_t value)
{
	enum mhu_v3_x_error_t error;
	uint8_t num_channels;
	uint8_t channel;

	if (dev == NULL || dev->base == 0)
		return MHU_ERR_INVALID_ARG;

	error = mhu_v3_x_get_num_channel_implemented(
		dev, MHU_V3_X_CHANNEL_TYPE_DBCH, &num_channels);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	for (channel = 0; channel < num_channels; channel++) {
		error = mhu_v3_x_doorbell_clear(
			dev, channel, UINT32_MAX);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);
	}

	return wait_for_signal(dev, value);
}

static enum mhu_error_t validate_buffer(
	const void *buffer)
{
	uintptr_t address = (uintptr_t)buffer;

	if (address == 0 ||
	    (address & (sizeof(uint32_t) - 1U)) != 0)
		return MHU_ERR_INVALID_ARG;

	return MHU_ERR_NONE;
}

enum mhu_error_t mhu_init_sender(uintptr_t mhu_sender_base)
{
	enum mhu_v3_x_error_t error;
	uint8_t num_channels;
	uint8_t channel;

	if (mhu_sender_base == 0)
		return MHU_ERR_INVALID_ARG;

	mhu_sender_dev = (struct mhu_v3_x_dev_t) {
		.base = mhu_sender_base,
		.frame = MHU_V3_X_PBX_FRAME,
	};

	error = mhu_v3_x_driver_init(&mhu_sender_dev);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	error = mhu_v3_x_get_num_channel_implemented(
		&mhu_sender_dev,
		MHU_V3_X_CHANNEL_TYPE_DBCH,
		&num_channels);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	/*
	 * Channel 0 is initially used for the message length.
	 * The last channel is reserved for transfer notification.
	 */
	if (num_channels < 3)
		return MHU_ERR_UNSUPPORTED;

	for (channel = 0; channel < num_channels; channel++) {
		error = mhu_v3_x_channel_interrupt_disable(
			&mhu_sender_dev,
			channel,
			MHU_V3_X_CHANNEL_TYPE_DBCH);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);
	}

	return MHU_ERR_NONE;
}

enum mhu_error_t mhu_init_receiver(uintptr_t mhu_receiver_base)
{
	enum mhu_v3_x_error_t error;
	uint8_t num_channels;
	uint8_t channel;

	if (mhu_receiver_base == 0)
		return MHU_ERR_INVALID_ARG;

	mhu_receiver_dev = (struct mhu_v3_x_dev_t) {
		.base = mhu_receiver_base,
		.frame = MHU_V3_X_MBX_FRAME,
	};

	error = mhu_v3_x_driver_init(&mhu_receiver_dev);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	error = mhu_v3_x_get_num_channel_implemented(
		&mhu_receiver_dev,
		MHU_V3_X_CHANNEL_TYPE_DBCH,
		&num_channels);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	if (num_channels < 3)
		return MHU_ERR_UNSUPPORTED;

	/*
	 * Data channels are interrupt masked. Only the final notification
	 * channel contributes to the combined mailbox interrupt.
	 */
	for (channel = 0; channel < num_channels - 1; channel++) {
		error = mhu_v3_x_doorbell_mask_set(
			&mhu_receiver_dev, channel, UINT32_MAX);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);
	}

	error = mhu_v3_x_doorbell_mask_clear(
		&mhu_receiver_dev, num_channels - 1, UINT32_MAX);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	error = mhu_v3_x_channel_interrupt_enable(
		&mhu_receiver_dev,
		num_channels - 1,
		MHU_V3_X_CHANNEL_TYPE_DBCH);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	return MHU_ERR_NONE;
}

enum mhu_error_t mhu_send_data(
	const uint8_t *send_buffer,
	size_t size)
{
	enum mhu_v3_x_error_t error;
	enum mhu_error_t wrapper_error;
	uint8_t num_channels;
	uint8_t channel;
	size_t offset;

	if (size == 0)
		return MHU_ERR_NONE;

	if (size > UINT32_MAX)
		return MHU_ERR_INVALID_ARG;

	wrapper_error = validate_buffer(send_buffer);
	if (wrapper_error != MHU_ERR_NONE)
		return wrapper_error;

	error = mhu_v3_x_get_num_channel_implemented(
		&mhu_sender_dev,
		MHU_V3_X_CHANNEL_TYPE_DBCH,
		&num_channels);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	if (num_channels < 3)
		return MHU_ERR_UNSUPPORTED;

	/*
	 * The first word carries the total payload size.
	 */
	error = mhu_v3_x_doorbell_write(
		&mhu_sender_dev, 0, (uint32_t)size);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	channel = 1;
	offset = 0;

	while (offset < size) {
		uint32_t word = 0;
		size_t chunk;
		size_t remaining = size - offset;

		chunk = remaining < sizeof(word) ? remaining : sizeof(word);
		memcpy(&word, send_buffer + offset, chunk);

		error = mhu_v3_x_doorbell_write(
			&mhu_sender_dev, channel, word);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);

		offset += chunk;
		channel++;

		if (channel == num_channels - 1) {
			wrapper_error = signal_and_wait_for_clear(
				&mhu_sender_dev, MHU_NOTIFY_VALUE);
			if (wrapper_error != MHU_ERR_NONE)
				return wrapper_error;

			/*
			 * After the first window channel 0 is available for
			 * payload data again.
			 */
			channel = 0;
		}
	}

	if (channel != 0) {
		wrapper_error = signal_and_wait_for_clear(
			&mhu_sender_dev, MHU_NOTIFY_VALUE);
		if (wrapper_error != MHU_ERR_NONE)
			return wrapper_error;
	}

	return MHU_ERR_NONE;
}

enum mhu_error_t mhu_receive_data(
	uint8_t *receive_buffer,
	size_t *size)
{
	enum mhu_v3_x_error_t error;
	enum mhu_error_t wrapper_error;
	uint8_t num_channels;
	uint8_t channel;
	uint32_t message_length;
	size_t offset;
	size_t bytes_left;

	if (size == NULL || receive_buffer == NULL)
		return MHU_ERR_INVALID_ARG;

	wrapper_error = validate_buffer(receive_buffer);
	if (wrapper_error != MHU_ERR_NONE)
		return wrapper_error;

	error = mhu_v3_x_get_num_channel_implemented(
		&mhu_receiver_dev,
		MHU_V3_X_CHANNEL_TYPE_DBCH,
		&num_channels);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	if (num_channels < 3)
		return MHU_ERR_UNSUPPORTED;

	wrapper_error = wait_for_signal(
		&mhu_receiver_dev, MHU_NOTIFY_VALUE);
	if (wrapper_error != MHU_ERR_NONE)
		return wrapper_error;

	error = mhu_v3_x_doorbell_read(
		&mhu_receiver_dev, 0, &message_length);
	if (error != MHU_V_3_X_ERR_NONE)
		return map_error(error);

	if (*size < message_length) {
		*size = message_length;
		return MHU_ERR_BUFFER_TOO_SMALL;
	}

	channel = 1;
	offset = 0;
	bytes_left = message_length;

	while (bytes_left != 0) {
		uint32_t word = 0;
		size_t chunk;

		error = mhu_v3_x_doorbell_read(
			&mhu_receiver_dev, channel, &word);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);

		chunk = bytes_left < sizeof(word) ? bytes_left : sizeof(word);
		memcpy(receive_buffer + offset, &word, chunk);

		offset += chunk;
		bytes_left -= chunk;
		channel++;

		if (channel == num_channels - 1 && bytes_left != 0) {
			wrapper_error = clear_and_wait_for_signal(
				&mhu_receiver_dev, MHU_NOTIFY_VALUE);
			if (wrapper_error != MHU_ERR_NONE)
				return wrapper_error;

			channel = 0;
		}
	}

	for (channel = 0; channel < num_channels; channel++) {
		error = mhu_v3_x_doorbell_clear(
			&mhu_receiver_dev, channel, UINT32_MAX);
		if (error != MHU_V_3_X_ERR_NONE)
			return map_error(error);
	}

	*size = message_length;

	return MHU_ERR_NONE;
}

size_t mhu_get_max_message_size(void)
{
	uint8_t num_channels;

	if (mhu_v3_x_get_num_channel_implemented(
		    &mhu_receiver_dev,
		    MHU_V3_X_CHANNEL_TYPE_DBCH,
		    &num_channels) != MHU_V_3_X_ERR_NONE)
		return 0;

	if (num_channels < 2)
		return 0;

	return (size_t)(num_channels - 1) * sizeof(uint32_t);
}