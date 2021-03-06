/*
 * Copyright (C) 2013 STMicroelectronics Limited
 *
 * Author(s): Giuseppe Cavallaro <peppe.cavallaro@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *****************************************************************************
 * NOTE: THIS FILE IS AN INTERMEDIATE TRANSISSION FROM NON-DEVICE TREES
 * TO DEVICE TREES. IDEALLY THIS FILE SHOULD NOT EXIST IN FULL DEVICE TREE
 * SUPPORTED KERNEL.
 *
 * WITH THE ASSUMPTION THAT SDK2 WILL MOVE TO FULL DEVICE TREES AT
 * SOME POINT, AT WHICH THIS FILEIS NOT REQUIRED ANYMORE
 *
 * ALSO BOARD SUPPORT WITH THIS APPROCH IS IS DONE IN TWO PLACES
 * 1. IN THIS FILE
 * 2. arch/arm/boot/dts/stid128xxZ-b2112.dtsp
 *	THIS FILECONFIGURES ALL THE DRIVERS WHICH SUPPORT DEVICE TREES.
 *
 * please do not optimize this file or try adding any level of abstraction
 * due to reasons above.
 *****************************************************************************
 */

#include <linux/of_platform.h>
#include <linux/stm/stig125.h>
#include <linux/stm/soc.h>
#include <linux/stmfp.h>

#include <linux/stm/core_of.h>
#include <linux/stm/stm_device_of.h>

#include <asm/hardware/gic.h>
#include <asm/mach/time.h>

#include <mach/common-dt.h>
#include <mach/hardware.h>
#include <mach/soc-stig125.h>

struct of_dev_auxdata stig125_auxdata_lookup[] __initdata = {
	OF_DEV_AUXDATA("st,fdma", 0xfe2c0000, "stm-fdma.0", NULL),
	OF_DEV_AUXDATA("st,fdma", 0xfe2e0000, "stm-fdma.1", NULL),
	OF_DEV_AUXDATA("st,fdma", 0xfeb80000, "stm-fdma.2", NULL),
	OF_DEV_AUXDATA("st,usb", 0xfe800000, "stm-usb.0", NULL),
	{}
};

static void __init b2112_dt_init(void)
{
	of_platform_populate(NULL, of_default_bus_match_table,
			     stig125_auxdata_lookup, NULL);

	stig125_configure_fp(&(struct stig125_fp_config) {
			.if_cfg[DEVID_GIGE0] = &(struct fpif_config)
						{ .phy_addr = 0x4 },
		});

	/* PCIe0 (CN15) */
	stig125_configure_miphy(&(struct stig125_miphy_config){
			.id = 1,
			.mode = PCIE_MODE,
			.iface = UPORT_IF,
			});
	stig125_configure_pcie(0);

	/* PCIe1 (CN16) */
	stig125_configure_miphy(&(struct stig125_miphy_config){
			.id = 2,
			.mode = PCIE_MODE,
			.iface = UPORT_IF,
			});
	stig125_configure_pcie(1);
}

/* Setup the Timer */
static void __init stig125_of_timer_init(void)
{
	stig125_plat_clk_init();
	stig125_plat_clk_alias_init();
	stm_of_timer_init();
}

struct sys_timer stig125_of_timer = {
	.init	= stig125_of_timer_init,
};

static const char *stig125_dt_match[] __initdata = {
	"st,stig125-b2112",
	NULL
};

DT_MACHINE_START(STM, " STiD128ZYZ SoC with Flattened Device Tree")
	.map_io		= stig125_map_io,
	.init_early	= core_of_early_device_init,
	.timer		= &stig125_of_timer,
	.handle_irq	= gic_handle_irq,
	.init_machine	= b2112_dt_init,
	.init_irq	= stm_of_gic_init,
	.dt_compat	= stig125_dt_match,
MACHINE_END
