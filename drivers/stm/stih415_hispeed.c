/*
 * (c) 2011 STMicroelectronics Limited
 *
 *   Author: Srinivas Kandagatla <srinivas.kandagatla@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/ethtool.h>
#include <linux/dma-mapping.h>
#include <linux/phy.h>
#include <linux/stm/miphy.h>
#include <linux/stm/device.h>

#include <linux/stm/emi.h>
#include <linux/stm/pad.h>
#include <linux/stm/sysconf.h>
#include <linux/stm/stih415.h>
#include <linux/stm/stih415-periphs.h>
#include <linux/delay.h>
#include <asm/irq-ilc.h>
#include "pio-control.h"

/* --------------------------------------------------------------------
 *           Ethernet MAC resources (PAD and Retiming)
 * --------------------------------------------------------------------*/

#define DATA_IN(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_in, \
		.function = _func, \
		.priv = &(struct stih415_pio_config) { \
			.retime = _retiming, \
		}, \
	}

#define DATA_OUT(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_out, \
		.function = _func, \
		.priv = &(struct stih415_pio_config) { \
			.retime = _retiming, \
		}, \
	}

/* On some boards MDIO line is missing Pull-up resistor, Enabling weak
 * internal PULL-UP overcomes the issue */
#define DATA_OUT_PU(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_bidir, \
		.function = _func, \
		.priv = &(struct stih415_pio_config) { \
			.retime = _retiming, \
			.mode = &(struct stm_pio_control_mode_config) { \
				.oe = 0, \
				.pu = 1, \
				.od = 0, \
			}, \
		}, \
	}

#define CLOCK_IN(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_in, \
		.function = _func, \
		.priv = &(struct stih415_pio_config) { \
			.retime = _retiming, \
		}, \
	}

#define CLOCK_OUT(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_out, \
		.function = _func, \
		.priv = &(struct stih415_pio_config) { \
			.retime = _retiming, \
		}, \
	}

#define MDC_CLOCK_OUT(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_out, \
		.function = _func, \
		.priv = &(struct stih415_pio_config) { \
			.retime = _retiming, \
		}, \
	}

#define MY_PHY_CLOCK(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_out, \
		.function = _func, \
		.name = "PHYCLK", \
		.priv = &(struct stih415_pio_config) { \
		.retime = _retiming, \
		}, \
	}
#define PHY_CLOCK(_port, _pin, _func, _retiming) \
	{ \
		.gpio = stm_gpio(_port, _pin), \
		.direction = stm_pad_gpio_direction_unknown, \
		.function = _func, \
		.name = "PHYCLK", \
		.priv = &(struct stih415_pio_config) { \
		.retime = _retiming, \
		}, \
	}


/* FIXME MII_PHY_REF_CLK ??
 * MII_CLK_125 */



static struct stm_pad_config stih415_ethernet_mii_pad_configs[] = {
	[0] =  {
		.gpios_num = 20,
		.gpios = (struct stm_pad_gpio []) {
			DATA_OUT(14, 0, 2, RET_SE_NICLK_IO(0, 0)),/* TXD[0] */
			DATA_OUT(14, 1, 2, RET_SE_NICLK_IO(0, 0)),/* TXD[1] */
			DATA_OUT(14, 2, 2, RET_SE_NICLK_IO(0, 1)),/* TXD[2] */
			DATA_OUT(14, 3, 2, RET_SE_NICLK_IO(0, 1)),/* TXD[3] */
			DATA_OUT(15, 1, 2, RET_SE_NICLK_IO(0, 0)),/* TXER */
			DATA_OUT(13, 7, 2, RET_SE_NICLK_IO(0, 0)),/* TXEN */
			CLOCK_IN(15, 0, 2, RET_NICLK(0)),/* TXCLK */
			DATA_IN(15, 3, 2, RET_BYPASS(2)),/* COL */
			DATA_OUT(15, 4, 2, RET_BYPASS(3)),/* MDIO*/
			CLOCK_OUT(15, 5, 2, RET_NICLK(1)),/* MDC */
			DATA_IN(15, 2, 2, RET_BYPASS(2)),/* CRS */
			DATA_IN(13, 6, 2, RET_BYPASS(0)),/* MDINT */
			DATA_IN(16, 0, 2, RET_SE_NICLK_IO(0, 0)),/* 5 RXD[0] */
			DATA_IN(16, 1, 2, RET_SE_NICLK_IO(0, 0)),/* RXD[1] */
			DATA_IN(16, 2, 2, RET_SE_NICLK_IO(0, 0)),/* RXD[2] */
			DATA_IN(16, 3, 2, RET_SE_NICLK_IO(0, 0)),/* RXD[3] */
			DATA_IN(15, 6, 2, RET_SE_NICLK_IO(0, 0)),/* RXDV */
			DATA_IN(15, 7, 2, RET_SE_NICLK_IO(0, 0)),/* RX_ER */
			CLOCK_IN(17, 0, 2, RET_NICLK(0)),/* RXCLK */
			PHY_CLOCK(13, 5, 2, RET_NICLK(1)),/* PHYCLK */
		},
		.sysconfs_num = 5,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* ETH0_GMAC_EN */
			STM_PAD_SYSCONF(SYSCONF(166), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(382), 2, 4, 0),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(382), 5, 5, 1),
			/* TXCLK_NOT_CLK125 */
			STM_PAD_SYSCONF(SYSCONF(382), 6, 6, 1),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(382), 8, 8, 0),
		},
	},
	[1] =  {
		.gpios_num = 20,
		.gpios = (struct stm_pad_gpio []) {
			DATA_OUT(0, 0, 1, RET_SE_NICLK_IO(0, 0)),/* TXD[0] */
			DATA_OUT(0, 1, 1, RET_SE_NICLK_IO(0, 0)),/* TXD[1] */
			DATA_OUT(0, 2, 1, RET_SE_NICLK_IO(0, 0)),/* TXD[2] */
			DATA_OUT(0, 3, 1, RET_SE_NICLK_IO(0, 0)),/* TXD[3] */
			DATA_OUT(0, 4, 1, RET_SE_NICLK_IO(0, 0)),/* TXER */
			DATA_OUT(0, 5, 1, RET_SE_NICLK_IO(0, 0)),/* TXEN */
			CLOCK_IN(0, 6, 1, RET_NICLK(0)),/* TXCLK */
			DATA_IN(0, 7, 1, RET_BYPASS(2)),/* COL */

			DATA_OUT(1, 0, 1, RET_BYPASS(0)),/* MDIO */
			CLOCK_OUT(1, 1, 1, RET_NICLK(0)),/* MDC */
			DATA_IN(1, 2, 1, RET_BYPASS(2)),/* CRS */
			DATA_IN(1, 3, 1, RET_BYPASS(0)),/* MDINT */
			DATA_IN(1, 4, 1, RET_SE_NICLK_IO(0, 0)),/* RXD[0] */
			DATA_IN(1, 5, 1, RET_SE_NICLK_IO(0, 0)),/* RXD[1] */
			DATA_IN(1, 6, 1, RET_SE_NICLK_IO(0, 0)),/* RXD[2] */
			DATA_IN(1, 7, 1, RET_SE_NICLK_IO(0, 0)),/* RXD[3] */

			DATA_IN(2, 0, 1, RET_SE_NICLK_IO(0, 0)),/* RXDV */
			DATA_IN(2, 1, 1, RET_SE_NICLK_IO(0, 0)),/* RX_ER */
			CLOCK_IN(2, 2, 1, RET_NICLK(0)),/* RXCLK */
			PHY_CLOCK(2, 3, 1, RET_NICLK(1)),/* PHYCLK */

		},
		.sysconfs_num = 5,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC1 */
			STM_PAD_SYSCONF(SYSCONF(31), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(29), 2, 4, 0),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(29), 5, 5, 1),
			/* TXCLK_NOT_CLK125 */
			STM_PAD_SYSCONF(SYSCONF(29), 6, 6, 1),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(29), 8, 8, 0),
		},
	},
};

static struct stm_pad_config stih415_ethernet_gmii_pad_configs[] = {
	[0] =  {
		.gpios_num = 29,
		.gpios = (struct stm_pad_gpio []) {
			PHY_CLOCK(13, 5, 4, RET_NICLK(1)),/* GTXCLK */
			DATA_IN(13, 6, 2, RET_BYPASS(0)),/* MDINT */
			DATA_OUT(13, 7, 2, RET_SE_NICLK_IO(3, 0)),/* TXEN */

			DATA_OUT(14, 0, 2, RET_SE_NICLK_IO(3, 0)),/* TXD[0] */
			DATA_OUT(14, 1, 2, RET_SE_NICLK_IO(3, 0)),/* TXD[1] */
			DATA_OUT(14, 2, 2, RET_SE_NICLK_IO(3, 1)),/* TXD[2] */
			DATA_OUT(14, 3, 2, RET_SE_NICLK_IO(3, 1)),/* TXD[3] */
			DATA_OUT(14, 4, 2, RET_SE_NICLK_IO(3, 1)),/* TXD[4] */
			DATA_OUT(14, 5, 2, RET_SE_NICLK_IO(3, 1)),/* TXD[5] */
			DATA_OUT(14, 6, 2, RET_SE_NICLK_IO(3, 1)),/* TXD[6] */
			DATA_OUT(14, 7, 2, RET_SE_NICLK_IO(3, 1)),/* TXD[7] */

			CLOCK_IN(15, 0, 2, RET_NICLK(0)),/* TXCLK */
			DATA_OUT(15, 1, 2, RET_SE_NICLK_IO(3, 0)),/* TXER */
			DATA_IN(15, 2, 2, RET_BYPASS(2)),/* CRS */
			DATA_IN(15, 3, 2, RET_BYPASS(2)),/* COL */
			DATA_OUT_PU(15, 4, 2, RET_BYPASS(3)),/* MDIO */
			CLOCK_OUT(15, 5, 2, RET_NICLK(1)),/* MDC */
			DATA_IN(15, 6, 2, RET_SE_NICLK_IO(3, 0)),/* RXDV */
			DATA_IN(15, 7, 2, RET_SE_NICLK_IO(3, 0)),/* RX_ER */

			DATA_IN(16, 0, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[0] */
			DATA_IN(16, 1, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[1] */
			DATA_IN(16, 2, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[2] */
			DATA_IN(16, 3, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[3] */
			DATA_IN(16, 4, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[4] */
			DATA_IN(16, 5, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[5] */
			DATA_IN(16, 6, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[6] */
			DATA_IN(16, 7, 2, RET_SE_NICLK_IO(3, 0)),/* RXD[7] */

			CLOCK_IN(17, 0, 2, RET_NICLK(0)),/* RXCLK */
			CLOCK_IN(17, 6, 1, RET_NICLK(0)),/* 125MHZ i/p clk */
		},
		.sysconfs_num = 5,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC0 */
			STM_PAD_SYSCONF(SYSCONF(166), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(382), 2, 4, 0),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(382), 5, 5, 1),
			/* TXCLK_NOT_CLK125 */
			STM_PAD_SYSCONF(SYSCONF(382), 6, 6, 0),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(382), 8, 8, 0),
		},
	},
	[1] =  {
		.gpios_num = 29,
		.gpios = (struct stm_pad_gpio []) {
			DATA_OUT(0, 0, 1, RET_SE_NICLK_IO(3, 0)),/* TXD[0] */
			DATA_OUT(0, 1, 1, RET_SE_NICLK_IO(3, 0)),/* TXD[1] */
			DATA_OUT(0, 2, 1, RET_SE_NICLK_IO(3, 0)),/* TXD[2] */
			DATA_OUT(0, 3, 1, RET_SE_NICLK_IO(3, 0)),/* TXD[3] */
			DATA_OUT(0, 4, 1, RET_SE_NICLK_IO(3, 0)),/* TXER */
			DATA_OUT(0, 5, 1, RET_SE_NICLK_IO(3, 0)),/* TXEN */
			CLOCK_IN(0, 6, 1, RET_NICLK(0)),/* TXCLK  FIXME*/
			DATA_IN(0, 7, 1, RET_BYPASS(2)),/* COL */


			DATA_OUT_PU(1, 0, 1, RET_BYPASS(3)),/* MDIO */
			CLOCK_OUT(1, 1, 1, RET_NICLK(0)),/* MDC */
			DATA_IN(1, 2, 1, RET_BYPASS(2)),/* CRS */
			DATA_IN(1, 3, 1, RET_BYPASS(0)),/* MDINT */
			DATA_IN(1, 4, 1, RET_SE_NICLK_IO(3, 0)),/* RXD[0] */
			DATA_IN(1, 5, 1, RET_SE_NICLK_IO(3, 0)),/* RXD[1] */
			DATA_IN(1, 6, 1, RET_SE_NICLK_IO(3, 0)),/* RXD[2] */
			DATA_IN(1, 7, 1, RET_SE_NICLK_IO(3, 0)),/* RXD[3] */

			DATA_IN(2, 0, 1, RET_SE_NICLK_IO(3, 0)),/* RXDV */
			DATA_IN(2, 1, 1, RET_SE_NICLK_IO(3, 0)),/* RX_ER */
			CLOCK_IN(2, 2, 1, RET_NICLK(0)),/* RXCLK */
			PHY_CLOCK(2, 3, 4, RET_NICLK(1)), /* GTXCLK */
			DATA_OUT(2, 6, 4, RET_SE_NICLK_IO(3, 0)),/* TXD[4] */
			DATA_OUT(2, 7, 4, RET_SE_NICLK_IO(3, 0)),/* TXD[5] */

			DATA_IN(3, 0, 4, RET_SE_NICLK_IO(3, 0)),/* RXD[4] */
			DATA_IN(3, 1, 4, RET_SE_NICLK_IO(3, 0)),/* RXD[5] */
			DATA_IN(3, 2, 4, RET_SE_NICLK_IO(3, 0)),/* RXD[6] */
			DATA_IN(3, 3, 4, RET_SE_NICLK_IO(3, 0)),/* RXD[7] */

			CLOCK_IN(3, 7, 4, RET_NICLK(0)),/* 125MHZ input clock */

			DATA_OUT(4, 1, 4, RET_SE_NICLK_IO(3, 1)),/* TXD[6] */
			DATA_OUT(4, 2, 4, RET_SE_NICLK_IO(3, 1)),/* TXD[7] */
		},
		.sysconfs_num = 5,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC1 */
			STM_PAD_SYSCONF(SYSCONF(31), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(29), 2, 4, 0),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(29), 5, 5, 1),
			/* TXCLK_NOT_CLK125 */
			STM_PAD_SYSCONF(SYSCONF(29), 6, 6, 0),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(29), 8, 8, 0),
		},
	},
};

static struct stm_pad_config stih415_ethernet_rgmii_pad_configs[] = {
	[0] =  {
		.gpios_num = 18,
		.gpios = (struct stm_pad_gpio []) {
			PHY_CLOCK(13, 5, 2, RET_NICLK(1)),/* GTXCLK */
			DATA_IN(13, 6, 2, RET_BYPASS(0)), /* MDINT */
			DATA_OUT(13, 7, 2, RET_DE_IO(0, 0)),/* TXEN */
			DATA_OUT(14, 0, 2, RET_DE_IO(0, 0)),/* TXD[0] */
			DATA_OUT(14, 1, 2, RET_DE_IO(0, 0)),/* TXD[1] */
			DATA_OUT(14, 2, 2, RET_DE_IO(0, 0)),/* TXD[2] */
			DATA_OUT(14, 3, 2, RET_DE_IO(0, 0)),/* TXD[3] */

			/* TX Clock inversion is not set for 1000Mbps */
			CLOCK_IN(15, 0, 2, RET_ICLK(-1)),/* TXCLK */
			DATA_IN(15, 2, 2, RET_BYPASS(0)), /* CRS */
			DATA_IN(15, 3, 2, RET_BYPASS(0)),/* COL */
			DATA_OUT_PU(15, 4, 2, RET_BYPASS(3)),/* MDIO */
			CLOCK_OUT(15, 5, 2, RET_NICLK(0)),/* MDC */

			DATA_IN(15, 6, 2, RET_DE_IO(0, 0)),/* RXDV */
			DATA_IN(16, 0, 2, RET_DE_IO(0, 0)),/* RXD[0] */
			DATA_IN(16, 1, 2, RET_DE_IO(0, 0)),/* RXD[1] */
			DATA_IN(16, 2, 2, RET_DE_IO(0, 0)),/* RXD[2] */
			DATA_IN(16, 3, 2, RET_DE_IO(0, 0)),/* RXD[3] */
			CLOCK_IN(17, 0, 2, RET_NICLK(-1)),/* RXCLK */
		},
		.sysconfs_num = 3,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC0 */
			STM_PAD_SYSCONF(SYSCONF(166), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(382), 2, 4, 1),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(382), 5, 5, 1),
			/* Extra retime config base on speed */
		},
	},
	[1] =  {
		.gpios_num = 18,
		.gpios = (struct stm_pad_gpio []) {
			DATA_OUT(0, 0, 1, RET_DE_IO(0, 1)),/* TXD[0] */
			DATA_OUT(0, 1, 1, RET_DE_IO(0, 1)),/* TXD[1] */
			DATA_OUT(0, 2, 1, RET_DE_IO(0, 1)),/* TXD[2] */
			DATA_OUT(0, 3, 1, RET_DE_IO(0, 1)),/* TXD[3] */
			DATA_OUT(0, 5, 1, RET_DE_IO(0, 1)),/* TXEN */
			/* TX Clock inversion is not set for 1000Mbps */
			CLOCK_IN(0, 6, 1, RET_ICLK(-1)),/* TXCLK */
			DATA_IN(0, 7, 1, RET_BYPASS(0)),/* COL */

			DATA_OUT_PU(1, 0, 1, RET_BYPASS(2)),/* MDIO */
			CLOCK_OUT(1, 1, 1, RET_NICLK(1)),/* MDC */
			DATA_IN(1, 2, 1, RET_BYPASS(0)),/* CRS */
			DATA_IN(1, 3, 1, RET_BYPASS(0)),/* MDINT */
			DATA_IN(2, 0, 1, RET_DE_IO(0, 1)),/* RXDV */
			CLOCK_IN(2, 2, 1, RET_NICLK(-1)),/* RXCLK */
			PHY_CLOCK(2, 3, 1, RET_NICLK(1)), /* GTXCLK*/

			DATA_IN(3, 0, 1, RET_DE_IO(0, 1)),/* RXD[0] */
			DATA_IN(3, 1, 1, RET_DE_IO(0, 1)),/* RXD[1] */
			DATA_IN(3, 2, 1, RET_DE_IO(0, 1)),/* RXD[2] */
			DATA_IN(3, 3, 1, RET_DE_IO(0, 1)),/* RXD[3] */
		},
		.sysconfs_num = 3,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC1 */
			STM_PAD_SYSCONF(SYSCONF(31), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(29), 2, 4, 1),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(29), 5, 5, 1),
			/* Extra retime config base on speed */
		},
	},
};
static struct stm_pad_config stih415_ethernet_rmii_pad_configs[] = {
	[0] = {
		.gpios_num = 12,
		.gpios = (struct stm_pad_gpio []) {
			PHY_CLOCK(13, 5, 3, RET_NICLK(0)),/* PHYCLK */
			DATA_IN(13, 6, 2, RET_BYPASS(0)),/* MDINT */
			DATA_OUT(13, 7, 2, RET_SE_NICLK_IO(0, 1)),/* TXEN */
			DATA_OUT(14, 0, 2, RET_SE_NICLK_IO(0, 1)),/* TXD[0] */
			DATA_OUT(14, 1, 2, RET_SE_NICLK_IO(0, 1)),/* TXD[1] */
			DATA_IN(16, 0, 2, RET_SE_NICLK_IO(2, 1)),/* RXD.0 */
			DATA_IN(16, 1, 2, RET_SE_NICLK_IO(2, 1)),/* RXD.1 */
			DATA_OUT(15, 1, 2, RET_SE_NICLK_IO(0, 1)),/* TXER */
			DATA_OUT_PU(15, 4, 2, RET_BYPASS(3)),/* MDIO */
			CLOCK_OUT(15, 5, 2, RET_NICLK(0)),/* MDC */
			DATA_IN(15, 6, 2, RET_SE_NICLK_IO(2, 1)),/* RXDV */
			DATA_IN(15, 7, 2, RET_SE_NICLK_IO(2, 1)),/* RX_ER */
		},
		.sysconfs_num = 4,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC0 */
			STM_PAD_SYSCONF(SYSCONF(166), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(382), 2, 4, 4),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(382), 5, 5, 1),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(382), 8, 8, 1),
		},
	},
	[1] =  {
		.gpios_num = 12,
		.gpios = (struct stm_pad_gpio []) {
			PHY_CLOCK(2, 3, 2, RET_NICLK(0)),/* PHYCLK */
			DATA_IN(1, 3, 1, RET_BYPASS(0)),/* MDINT */
			DATA_OUT(0, 5, 1, RET_SE_NICLK_IO(0, 0)),/* TXEN */
			DATA_OUT(0, 0, 1, RET_SE_NICLK_IO(0, 0)),/* TXD[0] */
			DATA_OUT(0, 1, 1, RET_SE_NICLK_IO(0, 0)),/* TXD[1] */
			DATA_OUT(0, 4, 1, RET_SE_NICLK_IO(0, 0)),/* TXER */
			DATA_OUT_PU(1, 0, 1, RET_BYPASS(2)),/* MDIO */
			CLOCK_OUT(1, 1, 1, RET_NICLK(1)),/* MDC */
			DATA_IN(2, 0, 1, RET_SE_NICLK_IO(2, 0)),/* RXDV */
			DATA_IN(2, 1, 1, RET_SE_NICLK_IO(2, 0)),/* RX_ER */
			DATA_IN(1, 4, 1, RET_SE_NICLK_IO(2, 0)),/* RXD[0] */
			DATA_IN(1, 5, 1, RET_SE_NICLK_IO(2, 0)),/* RXD[1] */
		},
		.sysconfs_num = 4,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC1 */
			STM_PAD_SYSCONF(SYSCONF(31), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(29), 2, 4, 4),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(29), 5, 5, 1),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(29), 8, 8, 1),
		},
	},
};

static struct stm_pad_config stih415_ethernet_reverse_mii_pad_configs[] = {
	[0] = {
		.gpios_num = 20,
		.gpios = (struct stm_pad_gpio []) {
			DATA_OUT(14, 0, 2, RET_BYPASS(0)),/* TXD[0] */
			DATA_OUT(14, 1, 2, RET_BYPASS(0)),/* TXD[1] */
			DATA_OUT(14, 2, 2, RET_BYPASS(0)),/* TXD[2] */
			DATA_OUT(14, 3, 2, RET_BYPASS(0)),/* TXD[3] */
			DATA_OUT(15, 1, 2, RET_SE_NICLK_IO(0, 0)),/* TXER */
			DATA_OUT(13, 7, 2, RET_SE_NICLK_IO(0, 0)),/* TXEN */
			CLOCK_IN(15, 0, 2, RET_NICLK(-1)),/* TXCLK */

			DATA_OUT(15, 3, 3, RET_BYPASS(0)),/* COL */
			DATA_OUT_PU(15, 4, 2, RET_BYPASS(2)),/* MDIO*/
			CLOCK_IN(15, 5, 3, RET_NICLK(0)),/* MDC */
			DATA_OUT(15, 2, 3, RET_BYPASS(0)),/* CRS */

			DATA_IN(13, 6, 2, RET_BYPASS(0)),/* MDINT */
			DATA_IN(16, 0, 2, RET_SE_NICLK_IO(2, 0)),/* RXD[0] */
			DATA_IN(16, 1, 2, RET_SE_NICLK_IO(2, 0)),/* RXD[1] */
			DATA_IN(16, 2, 2, RET_SE_NICLK_IO(2, 0)),/* RXD[2] */
			DATA_IN(16, 3, 2, RET_SE_NICLK_IO(2, 0)),/* RXD[3] */
			DATA_IN(15, 6, 2, RET_SE_NICLK_IO(2, 0)),/* RXDV */
			DATA_IN(15, 7, 2, RET_SE_NICLK_IO(2, 0)),/* RX_ER */
			CLOCK_IN(17, 0, 2, RET_NICLK(-1)),/* RXCLK */
			PHY_CLOCK(13, 5, 2, RET_NICLK(0)),/* PHYCLK */
		},
		.sysconfs_num = 5,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC0 */
			STM_PAD_SYSCONF(SYSCONF(166), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(382), 2, 4, 0),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(382), 5, 5, 0),
			/* TXCLK_NOT_CLK125 */
			STM_PAD_SYSCONF(SYSCONF(382), 6, 6, 1),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(382), 8, 8, 0),
		},
	},
	[1] =  {
		.gpios_num = 20,
		.gpios = (struct stm_pad_gpio []) {
			DATA_OUT(0, 0, 1, RET_SE_NICLK_IO(0, 1)),/* TXD[0] */
			DATA_OUT(0, 1, 1, RET_SE_NICLK_IO(0, 1)),/* TXD[1] */
			DATA_OUT(0, 2, 1, RET_SE_NICLK_IO(0, 1)),/* TXD[2] */
			DATA_OUT(0, 3, 1, RET_SE_NICLK_IO(0, 1)),/* TXD[3] */
			DATA_OUT(0, 4, 1, RET_SE_NICLK_IO(0, 1)),/* TXER */
			DATA_OUT(0, 5, 1, RET_SE_NICLK_IO(0, 1)),/* TXEN */
			CLOCK_IN(0, 6, 1, RET_NICLK(-1)),/* TXCLK */
			DATA_OUT(0, 7, 2, RET_BYPASS(0)),/* COL */


			DATA_IN(2, 0, 1, RET_SE_NICLK_IO(2, 1)),/* RXDV */
			DATA_IN(2, 1, 1, RET_SE_NICLK_IO(2, 1)),/* RX_ER */
			CLOCK_IN(2, 2, 1, RET_NICLK(-1)),/* RXCLK */
			PHY_CLOCK(2, 3, 1, RET_NICLK(0)),/* PHYCLK */

			DATA_OUT_PU(1, 0, 1, RET_BYPASS(2)),/* MDIO */
			CLOCK_IN(1, 1, 2, RET_NICLK(1)),/* MDC */
			DATA_OUT(1, 2, 2, RET_BYPASS(0)),/* CRS */
			DATA_IN(1, 3, 1, RET_BYPASS(0)),/* MDINT */
			DATA_IN(1, 4, 1, RET_SE_NICLK_IO(2, 1)),/* RXD[0] */
			DATA_IN(1, 5, 1, RET_SE_NICLK_IO(2, 1)),/* RXD[1] */
			DATA_IN(1, 6, 1, RET_SE_NICLK_IO(2, 1)),/* RXD[2] */
			DATA_IN(1, 7, 1, RET_SE_NICLK_IO(2, 1)),/* RXD[3] */
		},
		.sysconfs_num = 5,
		.sysconfs = (struct stm_pad_sysconf []) {
			/* EN_GMAC1 */
			STM_PAD_SYSCONF(SYSCONF(31), 0, 0, 1),
			/* MIIx_PHY_SEL */
			STM_PAD_SYSCONF(SYSCONF(29), 2, 4, 0),
			/* ENMIIx */
			STM_PAD_SYSCONF(SYSCONF(29), 5, 5, 1),
			/* TXCLK_NOT_CLK125 */
			STM_PAD_SYSCONF(SYSCONF(29), 6, 6, 1),
			/* TX_RETIMING_CLK */
			STM_PAD_SYSCONF(SYSCONF(29), 8, 8, 0),
		},
	},
};
#if 0
static void stih415_ethernet_rmii_speed(void *bsp_priv, unsigned int speed)
{
	struct sysconf_field *mac_speed_sel = bsp_priv;

	sysconf_write(mac_speed_sel, (speed == SPEED_100) ? 1 : 0);
}
#endif
static void stih415_ethernet_gtx_speed(void *priv, unsigned int speed)
{
	void (*txclk_select)(int txclk_250_not_25_mhz) = priv;

	if (txclk_select)
		txclk_select(speed == SPEED_1000);
}

static void stih415_ethernet_rgmii0_gtx_speed(void *priv, unsigned int speed)
{
	struct sysconf_field *sc = sysconf_claim(2, 82, 6, 8, "rgmmi");
	/* TX Clock inversion is not set for 1000Mbps */
	if (speed == SPEED_1000) {
		/* output clock driver by MII_TXCLK
		 * 125Mhz Clock from PHY is used for retiming
		 * and also to drive GTXCLK
		 * */
		sysconf_write(sc, 1);
		stm_pio_control_config_retime(15, 0, RET_NICLK(-1));
	} else {
		/* output clock driver by Clockgen
		 * 125MHz clock provided by PHY is not suitable for retiming.
		 * So TXPIO retiming must therefore be clocked by an
		 * internal 2.5/25Mhz clock generated by Clockgen.
		 * */
		sysconf_write(sc, 6);
		stm_pio_control_config_retime(15, 0, RET_ICLK(-1));
	}

	sysconf_release(sc);
	stih415_ethernet_gtx_speed(priv, speed);
}

static void stih415_ethernet_rgmii1_gtx_speed(void *priv, unsigned int speed)
{
	struct sysconf_field *sc = sysconf_claim(0, 29, 6, 8, "rgmmi");
	/* TX Clock inversion is not set for 1000Mbps */
	if (speed == SPEED_1000) {
		sysconf_write(sc, 1);
		stm_pio_control_config_retime(0, 6, RET_NICLK(-1));
	} else {
		sysconf_write(sc, 6);
		stm_pio_control_config_retime(0, 6, RET_ICLK(-1));
	}
	sysconf_release(sc);
	stih415_ethernet_gtx_speed(priv, speed);
}

static struct plat_stmmacenet_data stih415_ethernet_platform_data[] = {
	{
		.pbl = 32,
		.has_gmac = 1,
		.enh_desc = 1,
		.tx_coe = 0,
		.force_sf_dma_mode = 1,
		.bugged_jumbo = 1,
		.pmt = 1,
		.init = &stmmac_claim_resource,
	}, {
		.pbl = 32,
		.has_gmac = 1,
		.enh_desc = 1,
		.tx_coe = 0,
		.force_sf_dma_mode = 1,
		.bugged_jumbo = 1,
		.pmt = 1,
		.init = &stmmac_claim_resource,
	}
};

#if defined(CONFIG_SUPERH)
#define STIH415_IRQ(irq) ILC_IRQ(irq)
#elif defined(CONFIG_ARM)
#define STIH415_IRQ(irq) ((irq)+32)
#endif

static u64 stih415_usb_dma_mask = DMA_BIT_MASK(32);
static struct platform_device stih415_ethernet_devices[] = {
	{
		.name = "stmmaceth",
		.id = 0,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0xFE810000, 0x8000),
			STM_PLAT_RESOURCE_IRQ_NAMED("macirq", STIH415_IRQ(147), -1),
		},
		.dev = {
			.dma_mask = &stih415_usb_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
			.platform_data = &stih415_ethernet_platform_data[0],
		},
	}, {
		.name = "stmmaceth",
		.id = 1,
		.num_resources = 2,
		.resource = (struct resource[]) {
			STM_PLAT_RESOURCE_MEM(0xFEF08000, 0x8000),
			STM_PLAT_RESOURCE_IRQ_NAMED("macirq", STIH415_IRQ(150), -1),
		},

		.dev = {
			.dma_mask = &stih415_usb_dma_mask,
			.coherent_dma_mask = DMA_BIT_MASK(32),
			.platform_data = &stih415_ethernet_platform_data[1],
		},
	}
};

#define GMAC_AHB_CONFIG         0x2000
static void stih415_ethernet_bus_setup(void __iomem *ioaddr)
{
	/* Configure the bridge to generate more efficient STBus traffic.
	 *
	 * Cut Version	| Ethernet AD_CONFIG[21:0]
	 *	1.0	|	0x00264207
	 */
	writel(0x00264207, ioaddr + GMAC_AHB_CONFIG);
}

/* ONLY MII mode on GMAC1 is tested */
void __init stih415_configure_ethernet(int port,
		struct stih415_ethernet_config *config)
{
	static int configured[ARRAY_SIZE(stih415_ethernet_devices)];
	struct stih415_ethernet_config default_config;
	struct plat_stmmacenet_data *plat_data;
	struct stm_pad_config *pad_config;
	int interface;

	BUG_ON(port < 0 || port >= ARRAY_SIZE(stih415_ethernet_devices));
	BUG_ON(configured[port]++);

	if (!config)
		config = &default_config;

	plat_data = &stih415_ethernet_platform_data[port];

	switch (config->mode) {
	case stih415_ethernet_mode_mii:
		pad_config = &stih415_ethernet_mii_pad_configs[port];
		if (config->ext_clk)
			stm_pad_set_pio_ignored(pad_config, "PHYCLK");
		else
			stm_pad_set_pio_out(pad_config, "PHYCLK", 1 + port);
		interface = PHY_INTERFACE_MODE_MII;
		break;
	case stih415_ethernet_mode_gmii:
#if 0
		pad_config = &stih415_ethernet_gmii_pad_configs[port];
		stm_pad_set_pio_ignored(pad_config, "PHYCLK");
		interface = PHY_INTERFACE_MODE_GMII;
		break;
#endif
	case stih415_ethernet_mode_gmii_gtx:
		pad_config = &stih415_ethernet_gmii_pad_configs[port];
		stm_pad_set_pio_out(pad_config, "PHYCLK", 4); /*FIXME */
		plat_data->fix_mac_speed = stih415_ethernet_gtx_speed;
		plat_data->bsp_priv = config->txclk_select;
		interface = PHY_INTERFACE_MODE_GMII;
		break;
	case stih415_ethernet_mode_rgmii_gtx:
		/* This mode is similar to GMII (GTX) except the data
		 * buses are reduced down to 4 bits and the 2 error
		 * signals are removed. The data rate is maintained by
		 * using both edges of the clock. This also explains
		 * the different retiming configuration for this mode.
		 */
		pad_config = &stih415_ethernet_rgmii_pad_configs[port];
		stm_pad_set_pio_out(pad_config, "PHYCLK", 1 + port);
		if (port == 0)
			plat_data->fix_mac_speed =
				stih415_ethernet_rgmii0_gtx_speed;
		else
			plat_data->fix_mac_speed =
				stih415_ethernet_rgmii1_gtx_speed;

		plat_data->bsp_priv = config->txclk_select;
		interface = PHY_INTERFACE_MODE_RGMII;
		break;
	case stih415_ethernet_mode_rmii:
		pad_config = &stih415_ethernet_rmii_pad_configs[port];
		if (config->ext_clk) {
			stm_pad_set_pio_in(pad_config, "PHYCLK", 2 + port);
			/* SEL_INTERNAL_NO_EXT_PHYCLK */
			if (!port)
				stm_pad_config_add_sysconf(pad_config,
							2, 82, 7, 7, 0);
			else
				stm_pad_config_add_sysconf(pad_config,
							0, 29, 7, 7, 0);
		} else {
			stm_pad_set_pio_out(pad_config, "PHYCLK", 1 + port);
			/* SEL_INTERNAL_NO_EXT_PHYCLK */
			if (!port)
				stm_pad_config_add_sysconf(pad_config,
							2, 82, 7, 7, 1);
			else
				stm_pad_config_add_sysconf(pad_config,
							0, 29, 7, 7, 1);
		}
#if 0
		plat_data->fix_mac_speed = stih415_ethernet_rmii_speed;
#endif
		break;
	case stih415_ethernet_mode_reverse_mii:
		pad_config = &stih415_ethernet_reverse_mii_pad_configs[port];
		if (config->ext_clk)
			stm_pad_set_pio_ignored(pad_config, "PHYCLK");
		else
			stm_pad_set_pio_out(pad_config, "PHYCLK", 1 + port);
		interface = PHY_INTERFACE_MODE_MII;
		break;
	default:
		BUG();
		return;
	}

	stih415_ethernet_platform_data[port].bus_setup =
			stih415_ethernet_bus_setup;

	plat_data->custom_cfg = (void *) pad_config;
	plat_data->interface = interface;
	plat_data->bus_id = config->phy_bus;
	plat_data->phy_addr = config->phy_addr;
	plat_data->mdio_bus_data = config->mdio_bus_data;

	platform_device_register(&stih415_ethernet_devices[port]);
}
