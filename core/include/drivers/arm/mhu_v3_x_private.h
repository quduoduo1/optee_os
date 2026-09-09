/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 */

#ifndef __DRIVERS_ARM_MHU_V3_X_PRIVATE_H
#define __DRIVERS_ARM_MHU_V3_X_PRIVATE_H

#include <stdint.h>

/* MHUv3 control-page registers */
#define MHU_V3_X_AIDR_OFFSET			UINT32_C(0xfcc)
#define MHU_V3_X_DBCH_CFG0_OFFSET		UINT32_C(0x20)
#define MHU_V3_X_CTRL_OFFSET			UINT32_C(0x100)

/* Doorbell channel windows */
#define MHU_V3_X_DBCH_WINDOW_OFFSET		UINT32_C(0x1000)
#define MHU_V3_X_DBCH_STRIDE			UINT32_C(0x20)

/* PBX doorbell channel window registers */
#define MHU_V3_X_PBX_DBCH_ST_OFFSET		UINT32_C(0x00)
#define MHU_V3_X_PBX_DBCH_SET_OFFSET		UINT32_C(0x0c)
#define MHU_V3_X_PBX_DBCH_INT_ST_OFFSET	UINT32_C(0x10)
#define MHU_V3_X_PBX_DBCH_INT_CLR_OFFSET	UINT32_C(0x14)
#define MHU_V3_X_PBX_DBCH_INT_EN_OFFSET	UINT32_C(0x18)
#define MHU_V3_X_PBX_DBCH_CTRL_OFFSET		UINT32_C(0x1c)

/* MBX doorbell channel window registers */
#define MHU_V3_X_MBX_DBCH_ST_OFFSET		UINT32_C(0x00)
#define MHU_V3_X_MBX_DBCH_ST_MSK_OFFSET	UINT32_C(0x04)
#define MHU_V3_X_MBX_DBCH_CLR_OFFSET		UINT32_C(0x08)
#define MHU_V3_X_MBX_DBCH_MSK_ST_OFFSET	UINT32_C(0x10)
#define MHU_V3_X_MBX_DBCH_MSK_SET_OFFSET	UINT32_C(0x14)
#define MHU_V3_X_MBX_DBCH_MSK_CLR_OFFSET	UINT32_C(0x18)
#define MHU_V3_X_MBX_DBCH_CTRL_OFFSET		UINT32_C(0x1c)

/* Interrupt/control bits */
#define MHU_V3_X_PDBCW_INT_X_TFR_ACK		UINT32_C(0x1)
#define MHU_V3_X_PDBCW_CTRL_PBX_COMB_EN	UINT32_C(0x1)
#define MHU_V3_X_MDBCW_CTRL_MBX_COMB_EN	UINT32_C(0x1)

#endif /* __DRIVERS_ARM_MHU_V3_X_PRIVATE_H */