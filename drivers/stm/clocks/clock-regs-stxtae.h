/*****************************************************************************
 *
 * File name   : clock-regs-fli7510.h
 * Description : Low Level API - Base addresses & register definitions.
 *
 * COPYRIGHT (C) 2011 STMicroelectronics - All Rights Reserved
 * May be copied or modified under the terms of the GNU General Public
 * License V2 _ONLY_.  See linux/COPYING for more information.
 *
 *****************************************************************************/

#ifndef __CLOCK_LLA_REGS_H
#define __CLOCK_LLA_REGS_H


/* --- Base addresses ---------------------------------------- */
#define CKGA_BASE_ADDRESS		0xFEE62000

/* --- CKGA registers --- */
#define CKGA_PLL0_CFG			0x000
#define CKGA_PLL1_CFG			0x004
#define CKGA_POWER_CFG			0x010
#define CKGA_CLKOPSRC_SWITCH_CFG	0x014
#define CKGA_OSC_ENABLE_FB		0x018
#define CKGA_PLL0_ENABLE_FB		0x01c
#define CKGA_PLL1_ENABLE_FB		0x020
#define CKGA_CLKOPSRC_SWITCH_CFG2	0x024

#define CKGA_CLKOBS_MUX1_CFG		0x030
#define CKGA_CLKOBS_MASTER_MAXCOUNT	0x034
#define CKGA_CLKOBS_CMD			0x038
#define CKGA_CLKOBS_STATUS		0x03c
#define CKGA_CLKOBS_SLAVE0_COUNT	0x040
#define CKGA_OSCMUX_DEBUG		0x044
#define CKGA_CLKOBS_MUX2_CFG		0x048
#define CKGA_LOW_POWER_CTRL		0x04C


#define CLGS_BASE_ADDRESS		0xFE870000

#define  CLGS_CTL_SEL			0x00
#define  CLGS_CTL_EN			0x04

#define CLGS_FS_CTL_REG(fs, num)		(8 + ((fs - 1)*20) + (num*4))

/*
 * The CKGA_SOURCE_CFG(..) replaces the
 * - CKGA_OSC_DIV0_CFG
 * - CKGA_PLL0HS_DIV0_CFG
 * - CKGA_PLL0LS_DIV0_CFG
 * - CKGA_PLL1_DIV0_CFG
 * macros.
 * The _parent_id identifies the parent as:
 * - 0: OSC
 * - 1: PLL0_HS
 * - 2: PLL0_LS
 * - 3: PLL1
 */
#define CKGA_SOURCE_CFG(_parent_id)	(0x800 + (_parent_id) * 0x100)
#endif  /* End __CLOCK_LLA_REGS_H */
