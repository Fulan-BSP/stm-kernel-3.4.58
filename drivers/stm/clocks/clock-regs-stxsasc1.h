/*****************************************************************************
 *
 * File name   : clock-regs-stxSASC1.h
 * Description : Low Level API - Base addresses & register definitions.
 *
 * COPYRIGHT (C) 2012 STMicroelectronics - All Rights Reserved
 * May be copied or modified under the terms of the GNU General Public
 * License V2 _ONLY_.  See linux/COPYING for more information.
 *
 *****************************************************************************/

#ifndef __CLOCK_LLA_REGS_SASC1_H
#define __CLOCK_LLA_REGS_SASC1_H

/* --- CKGA registers --- */
#define CKGA_PLL_CFG(_pll_id, _reg_id)	((_reg_id) * 4 + (_pll_id) * 0xc)
#define CKGA_POWER_CFG	                0x018
#define CKGA_CLKOPSRC_SWITCH_CFG        0x01c
#define CKGA_CLKOPSRC_SWITCH_CFG2       0x020

#define CKGA_CLKOBS_MUX0_CFG            0x030
#define CKGA_CLKOBS_MASTER_MAXCOUNT     0x034
#define CKGA_CLKOBS_CMD                 0x038
#define CKGA_CLKOBS_STATUS              0x03c
#define CKGA_CLKOBS_SLAVE0_COUNT        0x040
#define CKGA_OSCMUX_DEBUG               0x044
#define CKGA_CLKOBS_MUX1_CFG            0x048
#define CKGA_LOW_POWER_CTRL             0x04C
#define CKGA_LOW_POWER_CFG              0x050

#define CKGA_PLL0_REG3_CFG              0x054
#define CKGA_PLL1_REG3_CFG              0x058

/*
 * The CKGA_SOURCE_CFG(..) replaces the
 * - CKGA_OSC_DIV0_CFG
 * - CKGA_PLL0HS_DIV0_CFG
 * - CKGA_PLL1HS_DIV0_CFG
 * - CKGA_PLL0LS_DIV0_CFG
 * - CKGA_PLL1LS_DIV0_CFG
 * macros.
 * The _parent_id identifies the parent as:
 * - 0: OSC
 * - 1: PLL0_HS
 * - 2: PLL0_LS
 * - 3: PLL1_HS
 * - 4: PLL1_LS
 */
#define CKGA_SOURCE_CFG(_parent_id)	(0x800 + (_parent_id) * 0x100 +	 \
					((_parent_id) == 2 ? 0x80 : 0) - \
					((_parent_id) >= 2 ? 0x100 : 0))

#define CKGA_OSC_DIV0_CFG				0x800
#define CKGA_PLL0HS_DIV0_CFG			0x900
#define CKGA_PLL0LS_DIV0_CFG			0xA08
#define CKGA_PLL1HS_DIV0_CFG			0x980
#define CKGA_PLL1LS_DIV0_CFG			0xAD8

/* ---  QFS registers (common to clockgen B/C/D/E/F--- */

#define CKG_FS_SETUP					0x000
#define CKG_FS_CFG(_chan)				(0x04 + (0x4 * _chan))
#define CKG_FS_ENPRGMSK					0x014
#define CKG_FS_EXCLKCMN					0x018
#define CKG_FS_OPCLKCMN					0x01C
#define CKG_FS_PWR						0x020
#define CKG_FS_OPCLKSTP					0x024
#define CKG_FS_REFCLKSEL				0x028

#define CKG_FS_GPOUT(_chan)				(0x060 + (0x4 * _chan))
#define CKG_FS_GPOUT_CTRL				0x074

#define CKG_FS_JE_CTRL0					0x040

#endif  /* End __CLOCK_LLA_REGS_SASC1_H */
