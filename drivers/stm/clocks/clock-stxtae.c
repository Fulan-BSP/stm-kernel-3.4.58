/*****************************************************************************
 *
 * File name   : clock-fli7610.c
 * Description : Low Level API - HW specific implementation
 * Author: Srinivas Kandagatla <srinivas.kandagatla@st.com>
 *
 * COPYRIGHT (C) 2011 STMicroelectronics - All Rights Reserved
 * May be copied or modified under the terms of the GNU General Public
 * License V2 __ONLY__.  See linux/COPYING for more information.
 *
 *****************************************************************************/

/* ----- Modification history (most recent first)----
 * Original code from stx7111 platform
*/

/* Includes ----------------------------------------------------------------- */

#include <linux/stm/fli7610.h>
#include <linux/stm/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "clock-stxtae.h"
#include "clock-regs-stxtae.h"

#include "clock-oslayer.h"
#include "clock-common.h"

static void *cga_base;

static int clkgena_set_parent(clk_t *clk_p, clk_t *src_p);
static int clkgena_set_rate(clk_t *clk_p, unsigned long freq);
static int clkgena_set_div(clk_t *clk_p, unsigned long *div_p);
static int clkgena_recalc(clk_t *clk_p);
static int clkgena_enable(clk_t *clk_p);
static int clkgena_disable(clk_t *clk_p);
static int clkgena_init(clk_t *clk_p);

#define SYSA_CLKIN			30	/* FE osc */

_CLK_OPS(clkgena,
	"Clockgen A",
	clkgena_init,
	clkgena_set_parent,
	clkgena_set_rate,
	clkgena_recalc,
	clkgena_enable,
	clkgena_disable,
	NULL,
	NULL,
	NULL
);

/* Physical clocks description */
clk_t clk_clocks[] = {

_CLK(CLKA_REF, &clkgena, 0,
	  CLK_RATE_PROPAGATES | CLK_ALWAYS_ENABLED),
_CLK_P(CLKA_PLL0HS, &clkgena, 900000000,
	CLK_RATE_PROPAGATES, &clk_clocks[CLKA_REF]),
_CLK_P(CLKA_PLL0LS, &clkgena, 450000000,
	CLK_RATE_PROPAGATES, &clk_clocks[CLKA_PLL0HS]),
_CLK_P(CLKA_PLL1, &clkgena, 800000000,
	CLK_RATE_PROPAGATES, &clk_clocks[CLKA_REF]),


_CLK(CLKA_VTAC_0_PHY, 	&clkgena, 900000000, 0),
_CLK(CLKA_VTAC_1_PHY, 	&clkgena, 900000000, 0),
_CLK(CLKA_STAC_PHY, 	&clkgena, 800000000, 0),
_CLK(CLKA_DCE_PHY, 	&clkgena, 200000000, 0),
_CLK(CLKA_STAC_DIGITAL, &clkgena, 400000000, 0),
_CLK(CLKA_AIP, 		&clkgena, 225000000, 0),
_CLK(CLKA_RESV0, 	&clkgena, 0	   , 0),
_CLK(CLKA_FDMA, 	&clkgena, 400000000, CLK_ALWAYS_ENABLED),
_CLK(CLKA_RESV1, 	&clkgena, 0	   , 0),
_CLK(CLKA_AATV, 	&clkgena, 200000000, 0),
_CLK(CLKA_EMI, 		&clkgena, 133000000, 0),
_CLK(CLKA_GMAC_LPM,	&clkgena,  75000000, 0),
_CLK(CLKA_GMAC, 	&clkgena,  75000000, 0),
_CLK(CLKA_PCI,  	&clkgena, 100000000, CLK_ALWAYS_ENABLED),
_CLK(CLKA_IC_100, 	&clkgena, 100000000, CLK_ALWAYS_ENABLED),
_CLK(CLKA_IC_150, 	&clkgena, 150000000, 0),
_CLK(CLKA_ETHERNET, 	&clkgena,  25000000, CLK_ALWAYS_ENABLED),
_CLK(CLKA_IC_200, 	&clkgena, 200000000, CLK_ALWAYS_ENABLED),

};




/******************************************************************************
CLOCKGEN A (CPU+interco+comms) clocks group
******************************************************************************/

/* ========================================================================
Name:	     clkgena_get_index
Description: Returns index of given clockgenA clock and source reg infos
Returns:     idx==-1 if error, else >=0
======================================================================== */

static int clkgena_get_index(int clkid, unsigned long *srcreg, int *shift)
{
	int idx;

	/* Warning: This function assumes clock IDs are perfectly
	   following real implementation order. Each "hole" has therefore
	   to be filled with "CLKx_NOT_USED" */
	if (clkid < CLKA_STAC_DIGITAL || clkid > CLK_NOT_USED_2)
		return -1;

	idx = (clkid - CLKA_STAC_DIGITAL) % 16;

	*srcreg = CKGA_CLKOPSRC_SWITCH_CFG + (0x10 * (idx / 16));
	*shift = 2 * (idx % 16);

	return idx;
}

/* ========================================================================
   Name:	clkgena_set_parent
   Description: Set clock source for clockgenA when possible
   Returns:     0=NO error
   ======================================================================== */

static int clkgena_set_parent(clk_t *clk_p, clk_t *src_p)
{
	unsigned long clk_src, val;
	int idx, shift;
	unsigned long srcreg;

	if (!clk_p || !src_p)
		return CLK_ERR_BAD_PARAMETER;
	if (clk_p->id < CLKA_STAC_DIGITAL || clk_p->id > CLK_NOT_USED_2)
		return CLK_ERR_BAD_PARAMETER;

	switch (src_p->id) {
	case CLKA_REF:
		clk_src = 0;
		break;
	case CLKA_PLL0LS:
	case CLKA_PLL0HS:
		clk_src = 1;
		break;
	case CLKA_PLL1:
		clk_src = 2;
		break;
	default:
		return CLK_ERR_BAD_PARAMETER;
	}

	idx = clkgena_get_index(clk_p->id, &srcreg, &shift);
	if (idx == -1)
		return CLK_ERR_BAD_PARAMETER;

	val = CLK_READ(cga_base + srcreg) & ~(0x3 << shift);
	val = val | (clk_src << shift);
	CLK_WRITE(cga_base + srcreg, val);

	clk_p->parent = &clk_clocks[src_p->id];

	return clkgena_recalc(clk_p);
}

/* ========================================================================
   Name:	clkgena_identify_parent
   Description: Identify parent clock for clockgen A clocks.
   Returns:     'clk_err_t' error code
   ======================================================================== */

static int clkgena_identify_parent(clk_t *clk_p)
{
	int idx;
	unsigned long src_sel;
	unsigned long srcreg;
	int shift;

	if (!clk_p)
		return CLK_ERR_BAD_PARAMETER;

	if ((clk_p->id >= CLKA_REF && clk_p->id <= CLKA_PLL1))
		return 0;

	/* Which divider to setup ? */
	idx = clkgena_get_index(clk_p->id, &srcreg, &shift);
	if (idx == -1)
		return CLK_ERR_BAD_PARAMETER;

	/* Identifying source */
	src_sel = (CLK_READ(cga_base + srcreg) >> shift) & 0x3;
	switch (src_sel) {
	case 0:
		clk_p->parent = &clk_clocks[CLKA_REF];
		break;
	case 1:
		if (idx <= 3)
			clk_p->parent = &clk_clocks[CLKA_PLL0HS];
		else
			clk_p->parent = &clk_clocks[CLKA_PLL0LS];
		break;
	case 2:
		clk_p->parent = &clk_clocks[CLKA_PLL1];
		break;
	case 3:
		clk_p->parent = NULL;
		clk_p->rate = 0;
		break;
	}

	return 0;
}

/* ========================================================================
   Name:	clkgena_xable_pll
   Description: Enable/disable PLL
   Returns:     'clk_err_t' error code
   ======================================================================== */

static int clkgena_xable_pll(clk_t *clk_p, int enable)
{
	unsigned long val;
	int bit, err = 0;

	if (!clk_p)
		return CLK_ERR_BAD_PARAMETER;
	if (clk_p->id != CLKA_PLL0HS && clk_p->id != CLKA_PLL1)
		return CLK_ERR_BAD_PARAMETER;

	bit = (clk_p->id == CLKA_PLL0HS ? 0 : 1);
	val = CLK_READ(cga_base + CKGA_POWER_CFG);
	if (enable)
		val &= ~(1 << bit);
	else
		val |= (1 << bit);
	CLK_WRITE(cga_base + CKGA_POWER_CFG, val);

	if (enable)
		err = clkgena_recalc(clk_p);
	else
		clk_p->rate = 0;

	return err;
}

/* ========================================================================
   Name:	clkgena_enable
   Description: Enable clock
   Returns:     'clk_err_t' error code
   ======================================================================== */

static int clkgena_enable(clk_t *clk_p)
{
	int err;

	if (!clk_p)
		return CLK_ERR_BAD_PARAMETER;
	if (!clk_p->parent)
		/* Unsupported. Init must be called first. */
		return CLK_ERR_BAD_PARAMETER;

	/* PLL power up */
	if (clk_p->id >= CLKA_PLL0HS && clk_p->id <= CLKA_PLL1)
		return clkgena_xable_pll(clk_p, 1);

	err = clkgena_set_parent(clk_p, clk_p->parent);
	/* clkgena_set_parent() is performing also a recalc() */

	return err;
}

/* ========================================================================
   Name:	clkgena_disable
   Description: Disable clock
   Returns:     'clk_err_t' error code
   ======================================================================== */

static int clkgena_disable(clk_t *clk_p)
{
	unsigned long val;
	int idx, shift;
	unsigned long srcreg;

	if (!clk_p)
		return CLK_ERR_BAD_PARAMETER;
	if (clk_p->id < CLKA_PLL0HS || clk_p->id > CLK_NOT_USED_2)
		return CLK_ERR_BAD_PARAMETER;

	/* Can this clock be disabled ? */
	if (clk_p->flags & CLK_ALWAYS_ENABLED)
		return 0;

	/* PLL power down */
	if (clk_p->id >= CLKA_PLL0HS && clk_p->id <= CLKA_PLL1)
		return clkgena_xable_pll(clk_p, 0);

	idx = clkgena_get_index(clk_p->id, &srcreg, &shift);
	if (idx == -1)
		return CLK_ERR_BAD_PARAMETER;

	/* Disabling clock */
	val = CLK_READ(cga_base + srcreg) & ~(0x3 << shift);
	val = val | (3 << shift);	 /* 3 = STOP clock */
	CLK_WRITE(cga_base + srcreg, val);
	clk_p->rate = 0;

	return 0;
}

static int clkgena_set_div(clk_t *clk_p, unsigned long *div_p)
{
	int idx;
	unsigned long div_cfg = 0;
	unsigned long srcreg, offset;
	int shift;

	if (!clk_p || !clk_p->parent)
		return CLK_ERR_BAD_PARAMETER;

	/* Computing divider config */
	div_cfg = (*div_p - 1) & 0x1F;

	/* Which divider to setup ? */
	idx = clkgena_get_index(clk_p->id, &srcreg, &shift);
	if (idx == -1)
		return CLK_ERR_BAD_PARAMETER;

	/* Now according to parent, let's write divider ratio */
	offset = CKGA_SOURCE_CFG(clk_p->parent->id - CLKA_REF);
	CLK_WRITE(cga_base + offset + (4 * idx), div_cfg);

	return 0;
}

static int clkgena_set_rate(clk_t *clk_p, unsigned long freq)
{
	unsigned long div;
	int err;

	if (!clk_p)
		return CLK_ERR_BAD_PARAMETER;
	if (clk_p->id < CLKA_PLL0HS || clk_p->id > CLK_NOT_USED_2)
		return CLK_ERR_BAD_PARAMETER;

	/* PLL set rate: to be completed */
	if ((clk_p->id >= CLKA_PLL0HS) && (clk_p->id <= CLKA_PLL1))
		return CLK_ERR_BAD_PARAMETER;

	/* We need a parent for these clocks */
	if (!clk_p->parent)
		return CLK_ERR_INTERNAL;

	div = clk_p->parent->rate / freq;
	err = clkgena_set_div(clk_p, &div);
	if (!err)
		clk_p->rate = clk_p->parent->rate / div;

	return err;
}

static int clkgena_recalc(clk_t *clk_p)
{
	unsigned long data, ratio;
	int idx;
	unsigned long srcreg, offset;
	int shift, err;

	if (!clk_p)
		return CLK_ERR_BAD_PARAMETER;
	if (!clk_p->parent)
		return CLK_ERR_INTERNAL;

	/* Reading clock programmed value */
	switch (clk_p->id) {
	case CLKA_REF:  /* Clockgen A reference clock */
		clk_p->rate = clk_p->parent->rate;
		break;
	case CLKA_PLL0HS:
		data = CLK_READ(cga_base + CKGA_PLL0_CFG);
		err = clk_pll1600_get_rate(clk_p->parent->rate, data & 0x7,
				(data >> 8) & 0xff, &clk_p->rate);
		return err;
	case CLKA_PLL0LS:
		clk_p->rate = clk_p->parent->rate / 2;
		return 0;
	case CLKA_PLL1:
		data = CLK_READ(cga_base + CKGA_PLL1_CFG);
		return clk_pll800_get_rate(clk_p->parent->rate, data & 0xff,
			(data >> 8) & 0xff, (data >> 16) & 0x7, &clk_p->rate);

	default:
		idx = clkgena_get_index(clk_p->id, &srcreg, &shift);
		if (idx == -1)
			return CLK_ERR_BAD_PARAMETER;

		/* Now according to source, let's get divider ratio */
		offset = CKGA_SOURCE_CFG(clk_p->parent->id - CLKA_REF);
		data = CLK_READ(cga_base + offset + (4 * idx));

		ratio = (data & 0x1F) + 1;

		clk_p->rate = clk_p->parent->rate / ratio;
		break;
	}

	return 0;
}

/* ========================================================================
   Name:	clkgena_init
   Description: Read HW status to initialize 'clk_t' structure.
   Returns:     'clk_err_t' error code.
   ======================================================================== */

static int clkgena_init(clk_t *clk_p)
{
	int err;

	if (!clk_p)
		return CLK_ERR_BAD_PARAMETER;

	err = clkgena_identify_parent(clk_p);
	if (!err)
		err = clkgena_recalc(clk_p);

	return err;
}

int tae_clk_init(clk_t *_sys_clk_in)
{
	int ret = 0;
	clk_clocks[CLKA_REF].parent = _sys_clk_in;
	cga_base = ioremap_nocache(CKGA_BASE_ADDRESS , 0x1000);
	ret = clk_register_table(clk_clocks, ARRAY_SIZE(clk_clocks), 0);
	if (ret)
		return ret;
	return ret;
}
