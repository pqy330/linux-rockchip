/*
 * Copyright (c) 2015 Heiko Stuebner <heiko@sntech.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/syscore_ops.h>
#include <dt-bindings/clock/rk3368-cru.h>
#include "clk.h"

#define RK3368_GRF_SOC_STATUS0	0x480

enum rk3368_plls {
	apllb, aplll, dpll, cpll, gpll, npll,
};

//static struct rockchip_pll_rate_table rk3368_pll_rates[] = {
//	{ /* sentinel */ },
//};
#define rk3368_pll_rates NULL


PNAME(mux_pll_p)		= { "xin24m", "xin32k" };
PNAME(mux_ddrphy_p)		= { "dpll_ddr", "gpll_ddr" };
PNAME(mux_aclk_bus_src_p)	= { "cpll_aclk_bus", "gpll_aclk_bus" };

PNAME(mux_pll_src_cpll_gpll_p)		= { "cpll", "gpll" };
PNAME(mux_pll_src_cpll_gpll_npll_p)	= { "cpll", "gpll", "npll" };
PNAME(mux_pll_src_npll_cpll_gpll_p)	= { "npll", "cpll", "gpll" };
PNAME(mux_pll_src_cpll_gll_usb_usb_p)	= { "cpll", "gpll", "usbphy480m_src", "usbphy480m_src" };

PNAME(mux_uart0_p)	= { "uart0_src", "uart0_frac", "xin24m" };
PNAME(mux_uart1_p)	= { "uart1_src", "uart1_frac", "xin24m" };
PNAME(mux_uart2_p)	= { "uart2_src", "xin24m" };
PNAME(mux_uart3_p)	= { "uart3_src", "uart3_frac", "xin24m" };
PNAME(mux_uart4_p)	= { "uart4_src", "uart4_frac", "xin24m" };


PNAME(mux_mac_p)	= { "mac_pll_src", "ext_gmac" };
PNAME(mux_mmc_src_p)	= { "cpll", "gpll", "usbphy480m_src", "xin24m" };


/*
 * Clock-Architecture Diagram 1
 */
static struct rockchip_pll_clock rk3368_pll_clks[] __initdata = {
	[apllb] = PLL(pll_rk3066, PLL_APLLB, "apllb", mux_pll_p, 0, RK3368_PLL_CON(0),
		     RK3368_PLL_CON(3), 8, 1, 0, rk3368_pll_rates),
	[aplll] = PLL(pll_rk3066, PLL_APLLL, "aplll", mux_pll_p, 0, RK3368_PLL_CON(4),
		     RK3368_PLL_CON(7), 8, 0, 0, rk3368_pll_rates),
	[dpll] = PLL(pll_rk3066, PLL_DPLL, "dpll", mux_pll_p, 0, RK3368_PLL_CON(8),
		     RK3368_PLL_CON(11), 8, 2, 0, NULL),
	[cpll] = PLL(pll_rk3066, PLL_CPLL, "cpll", mux_pll_p, 0, RK3368_PLL_CON(12),
		     RK3368_PLL_CON(15), 8, 3, ROCKCHIP_PLL_SYNC_RATE, rk3368_pll_rates),
	[gpll] = PLL(pll_rk3066, PLL_GPLL, "gpll", mux_pll_p, 0, RK3288_PLL_CON(16),
		     RK3368_PLL_CON(19), 8, 4, ROCKCHIP_PLL_SYNC_RATE, rk3368_pll_rates),
	[npll] = PLL(pll_rk3066, PLL_NPLL, "npll",  mux_pll_p, 0, RK3288_PLL_CON(20),
		     RK3368_PLL_CON(23), 8, 5, ROCKCHIP_PLL_SYNC_RATE, rk3368_pll_rates),
};

static struct clk_div_table div_ddrphy_t[] = {
	{ .val = 0, .div = 1 },
	{ .val = 1, .div = 2 },
	{ .val = 3, .div = 4 },
	{ /* sentinel */},
};

#define MFLAGS CLK_MUX_HIWORD_MASK
#define DFLAGS CLK_DIVIDER_HIWORD_MASK
#define GFLAGS (CLK_GATE_HIWORD_MASK | CLK_GATE_SET_TO_DISABLE)

static struct rockchip_clk_branch rk3368_clk_branches[] __initdata = {
	/*
	 * Clock-Architecture Diagram 2
	 */


	GATE(0, "dpll_ddr", "dpll", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(1), 8, GFLAGS),
	GATE(0, "gpll_ddr", "gpll", 0,
			RK3368_CLKGATE_CON(1), 9, GFLAGS),
	COMPOSITE_NOGATE_DIVTBL(0, "ddrphy_src", mux_ddrphy_p, CLK_IGNORE_UNUSED,
			RK3368_CLKSEL_CON(13), 4, 1, MFLAGS, 0, 2, DFLAGS, div_ddrphy_t),

	/* FIXME: provide a FIXED_GATE branch type? */
	GATE(0, "sclk_ddr", "ddrphy_div4", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(6), 14, GFLAGS),
	GATE(0, "sclk_ddr4x", "ddrphy_src", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(6), 15, GFLAGS),

	GATE(0, "gpll_aclk_bus", "gpll", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(1), 10, GFLAGS),
	GATE(0, "cpll_aclk_bus", "cpll", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(1), 11, GFLAGS),
	COMPOSITE_NOGATE(0, "aclk_bus_src", mux_aclk_bus_src_p, CLK_IGNORE_UNUSED,
			RK3288_CLKSEL_CON(8), 7, 1, MFLAGS, 0, 5, DFLAGS),

	GATE(ACLK_BUS, "aclk_bus", "aclk_bus_src", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(1), 0, GFLAGS),
	COMPOSITE_NOMUX(PCLK_BUS, "pclk_bus", "aclk_bus_src", CLK_IGNORE_UNUSED,
			RK3368_CLKSEL_CON(8), 12, 3, DFLAGS,
			RK3368_CLKGATE_CON(1), 2, GFLAGS),
	COMPOSITE_NOMUX(HCLK_BUS, "hclk_bus", "aclk_bus_src", CLK_IGNORE_UNUSED,
			RK3368_CLKSEL_CON(8), 8, 2, DFLAGS,
			RK3368_CLKGATE_CON(1), 1, GFLAGS),
	COMPOSITE_NOMUX(0, "sclk_crypto", "aclk_bus_src", 0,
			RK3368_CLKSEL_CON(10), 14, 2, DFLAGS,
			RK3368_CLKGATE_CON(7), 2, GFLAGS),

	COMPOSITE(0, "fclk_mcu_src", mux_pll_src_cpll_gpll_p, CLK_IGNORE_UNUSED,
			RK3368_CLKSEL_CON(12), 7, 1, MFLAGS, 0, 5, DFLAGS,
			RK3368_CLKGATE_CON(1), 3, GFLAGS),
	/*
	 * stclk_mcu gate is listed as child of fclk_mcu_src in diagram 5,
	 * but stclk_mcu has its own divider in diagram2
	 */
	COMPOSITE_NOMUX(0, "stclk_mcu", "fclk_mcu_src", 0,
			RK3368_CLKSEL_CON(12), 8, 3, DFLAGS,
			RK3368_CLKGATE_CON(13), 13, GFLAGS),




	MUX(0, "uart_src", mux_pll_src_cpll_gpll_p, 0,
			RK3368_CLKSEL_CON(35), 12, 1, MFLAGS),
	COMPOSITE_NOMUX(0, "uart2_src", "uart_src", 0,
			RK3368_CLKSEL_CON(37), 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(2), 4, GFLAGS),
	MUX(SCLK_UART2, "sclk_uart2", mux_uart2_p, CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(37), 8, 1, MFLAGS),

	/*
	 * Clock-Architecture Diagram 3
	 */

	COMPOSITE(DCLK_VOP0, "dclk_vop0", mux_pll_src_cpll_gpll_npll_p, 0,
			RK3368_CLKSEL_CON(20), 8, 2, MFLAGS, 0, 8, DFLAGS,
			RK3368_CLKGATE_CON(4), 1, GFLAGS),

	GATE(SCLK_VOP0_PWM, "sclk_vop0_pwm", "xin24m", 0,
			RK3368_CLKGATE_CON(4), 2, GFLAGS),



	GATE(SCLK_HDMI_HDCP, "sclk_hdmi_hdcp", "xin24m", 0,
			RK3368_CLKGATE_CON(4), 13, GFLAGS),
	GATE(SCLK_HDMI_CEC, "sclk_hdmi_cec", "xin32k", 0,
			RK3368_CLKGATE_CON(5), 12, GFLAGS),


	DIV(0, "pclk_pd_alive", "gpll", 0,
			RK3368_CLKSEL_CON(10), 8, 5, DFLAGS),
	/* FIXME: sclk_timer has a gate in the sgrf */

	COMPOSITE_NOMUX(0, "pclk_pd_pmu", "gpll", CLK_IGNORE_UNUSED,
			RK3368_CLKSEL_CON(10), 0, 5, DFLAGS,
			RK3368_CLKGATE_CON(7), 9, GFLAGS),
	GATE(0, "sclk_pvtm_pmu", "xin24m", 0, RK3368_CLKGATE_CON(7), 3, GFLAGS),


	COMPOSITE(0, "aclk_peri_src", mux_pll_src_cpll_gpll_p, CLK_IGNORE_UNUSED,
			RK3368_CLKSEL_CON(9), 7, 1, MFLAGS, 0, 5, DFLAGS,
			RK3368_CLKGATE_CON(3), 0, GFLAGS),
	COMPOSITE_NOMUX(PCLK_PERI, "pclk_peri", "aclk_peri_src", 0,
			RK3368_CLKSEL_CON(9), 12, 2, DFLAGS | CLK_DIVIDER_POWER_OF_TWO,
			RK3368_CLKGATE_CON(3), 3, GFLAGS),
	COMPOSITE_NOMUX(HCLK_PERI, "hclk_peri", "aclk_peri_src", CLK_IGNORE_UNUSED,
			RK3368_CLKSEL_CON(9), 8, 2, DFLAGS | CLK_DIVIDER_POWER_OF_TWO,
			RK3368_CLKGATE_CON(3), 2, GFLAGS),
	GATE(ACLK_PERI, "aclk_peri", "aclk_peri_src", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(3), 1, GFLAGS),

	GATE(0, "sclk_mipidsi_24m", "xin24m", 0, RK3368_CLKGATE_CON(4), 14, GFLAGS),

	/*
	 * Clock-Architecture Diagram 4
	 */

	COMPOSITE(SCLK_SPI0, "sclk_spi0", mux_pll_src_cpll_gpll_p, 0,
			RK3368_CLKSEL_CON(45), 7, 1, MFLAGS, 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(3), 7, GFLAGS),
	COMPOSITE(SCLK_SPI1, "sclk_spi1", mux_pll_src_cpll_gpll_p, 0,
			RK3368_CLKSEL_CON(45), 15, 1, MFLAGS, 8, 7, DFLAGS,
			RK3368_CLKGATE_CON(3), 8, GFLAGS),
	COMPOSITE(SCLK_SPI2, "sclk_spi2", mux_pll_src_cpll_gpll_p, 0,
			RK3368_CLKSEL_CON(46), 15, 1, MFLAGS, 8, 7, DFLAGS,
			RK3368_CLKGATE_CON(3), 9, GFLAGS),


	COMPOSITE(SCLK_SDMMC, "sclk_sdmmc", mux_mmc_src_p, 0,
			RK3368_CLKSEL_CON(50), 8, 2, MFLAGS, 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(7), 12, GFLAGS),
	COMPOSITE(SCLK_SDIO0, "sclk_sdio0", mux_mmc_src_p, 0,
			RK3368_CLKSEL_CON(48), 8, 2, MFLAGS, 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(7), 13, GFLAGS),
	COMPOSITE(SCLK_EMMC, "sclk_emmc", mux_mmc_src_p, 0,
			RK3368_CLKSEL_CON(51), 8, 2, MFLAGS, 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(7), 15, GFLAGS),

	MMC(SCLK_SDMMC_DRV,    "sdmmc_drv",    "sclk_sdmmc", RK3368_SDMMC_CON0, 1),
	MMC(SCLK_SDMMC_SAMPLE, "sdmmc_sample", "sclk_sdmmc", RK3368_SDMMC_CON1, 0),

	MMC(SCLK_SDIO0_DRV,    "sdio0_drv",    "sclk_sdio0", RK3368_SDIO0_CON0, 1),
	MMC(SCLK_SDIO0_SAMPLE, "sdio0_sample", "sclk_sdio0", RK3368_SDIO0_CON1, 0),

	MMC(SCLK_EMMC_DRV,     "emmc_drv",     "sclk_emmc",  RK3368_EMMC_CON0,  1),
	MMC(SCLK_EMMC_SAMPLE,  "emmc_sample",  "sclk_emmc",  RK3368_EMMC_CON1,  0),

	/* FIXME: restructure 480m handling [the 480m come from the pll inside the phy] */
	GATE(SCLK_OTGPHY0, "sclk_otgphy0", "usb480m", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(8), 1, GFLAGS),

	/* FIXME: pmu_grf_soc_con0[6] allows to select between xin32k and pvtm_pmu */
	GATE(SCLK_OTG_ADP, "sclk_otg_adp", "xin32k", CLK_IGNORE_UNUSED,
			RK3368_CLKGATE_CON(8), 4, GFLAGS),

	/* FIXME: pmu_grf_soc_con0[6] allows to select between xin32k and pvtm_pmu */
	COMPOSITE_NOMUX(SCLK_TSADC, "sclk_tsadc", "xin32k", 0,
			RK3368_CLKSEL_CON(25), 0, 6, DFLAGS,
			RK3368_CLKGATE_CON(3), 5, GFLAGS),

	COMPOSITE_NOMUX(SCLK_SARADC, "sclk_saradc", "xin24m", 0,
			RK3368_CLKSEL_CON(25), 8, 8, DFLAGS,
			RK3368_CLKGATE_CON(3), 6, GFLAGS),

	COMPOSITE(SCLK_NANDC0, "sclk_nandc0", mux_pll_src_cpll_gpll_p, 0,
			RK3368_CLKSEL_CON(47), 7, 1, MFLAGS, 0, 5, DFLAGS,
			RK3368_CLKGATE_CON(7), 8, GFLAGS),

	COMPOSITE(SCLK_SFC, "sclk_sfc", mux_pll_src_cpll_gpll_p, 0,
			RK3368_CLKSEL_CON(52), 7, 1, MFLAGS, 0, 5, DFLAGS,
			RK3368_CLKGATE_CON(6), 7, GFLAGS),

	COMPOSITE(0, "uart0_src", mux_pll_src_cpll_gll_usb_usb_p, 0,
			RK3368_CLKSEL_CON(33), 12, 2, MFLAGS, 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(2), 0, GFLAGS),
	COMPOSITE_FRAC(0, "uart0_frac", "uart0_src", CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(34), 0,
			RK3368_CLKGATE_CON(2), 1, GFLAGS),
	MUX(SCLK_UART0, "sclk_uart0", mux_uart0_p, CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(33), 8, 2, MFLAGS),

	COMPOSITE_NOMUX(0, "uart1_src", "uart_src", 0,
			RK3368_CLKSEL_CON(35), 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(2), 2, GFLAGS),
	COMPOSITE_FRAC(0, "uart1_frac", "uart1_src", CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(36), 0,
			RK3368_CLKGATE_CON(2), 3, GFLAGS),
	MUX(SCLK_UART1, "sclk_uart1", mux_uart1_p, CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(35), 8, 2, MFLAGS),

	COMPOSITE_NOMUX(0, "uart3_src", "uart_src", 0,
			RK3368_CLKSEL_CON(39), 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(2), 6, GFLAGS),
	COMPOSITE_FRAC(0, "uart3_frac", "uart3_src", CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(40), 0,
			RK3368_CLKGATE_CON(2), 7, GFLAGS),
	MUX(SCLK_UART3, "sclk_uart3", mux_uart3_p, CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(39), 8, 2, MFLAGS),

	COMPOSITE_NOMUX(0, "uart4_src", "uart_src", 0,
			RK3368_CLKSEL_CON(41), 0, 7, DFLAGS,
			RK3368_CLKGATE_CON(2), 8, GFLAGS),
	COMPOSITE_FRAC(0, "uart4_frac", "uart4_src", CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(42), 0,
			RK3368_CLKGATE_CON(2), 9, GFLAGS),
	MUX(SCLK_UART4, "sclk_uart4", mux_uart4_p, CLK_SET_RATE_PARENT,
			RK3368_CLKSEL_CON(41), 8, 2, MFLAGS),




	COMPOSITE(0, "mac_pll_src", mux_pll_src_npll_cpll_gpll_p, 0,
			RK3368_CLKSEL_CON(43), 6, 2, MFLAGS, 0, 5, DFLAGS,
			RK3368_CLKGATE_CON(3), 4, GFLAGS),
	MUX(SCLK_MAC, "mac_clk", mux_mac_p, 0,
			RK3368_CLKSEL_CON(43), 8, 1, MFLAGS),
	GATE(SCLK_MACREF_OUT, "sclk_macref_out", "mac_clk", 0,
			RK3368_CLKGATE_CON(7), 7, GFLAGS),
	GATE(SCLK_MACREF, "sclk_macref", "mac_clk", 0,
			RK3368_CLKGATE_CON(7), 6, GFLAGS),
	GATE(SCLK_MAC_RX, "sclk_mac_rx", "mac_clk", 0,
			RK3368_CLKGATE_CON(7), 4, GFLAGS),
	GATE(SCLK_MAC_TX, "sclk_mac_tx", "mac_clk", 0,
			RK3368_CLKGATE_CON(7), 5, GFLAGS),


	/*
	 * Clock-Architecture Diagram 5
	 */


	/* aclk_bus gates */
	GATE(0, "aclk_strc_sys", "aclk_bus", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(12), 12, GFLAGS),
	GATE(ACLK_DMAC_BUS, "aclk_dmac_bus", "aclk_bus", 0, RK3368_CLKGATE_CON(12), 11, GFLAGS),
	GATE(0, "sclk_intmem1", "aclk_bus", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(12), 6, GFLAGS),
	GATE(0, "sclk_intmem0", "aclk_bus", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(12), 5, GFLAGS),
	GATE(0, "aclk_intmem", "aclk_bus", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(12), 4, GFLAGS),
	GATE(0, "aclk_gic400", "aclk_bus", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(13), 9, GFLAGS),

	/* sclk_ddr gates */
	GATE(0, "nclk_ddrupctl", "sclk_ddr", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(13), 2, GFLAGS),

	/* clk_hsadc_tsp is part of diagram2 */

	/* fclk_mcu_src gates */
	GATE(0, "hclk_noc_mcu", "fclk_mcu_src", 0, RK3368_CLKGATE_CON(13), 14, GFLAGS),
	GATE(0, "fclk_mcu", "fclk_mcu_src", 0, RK3368_CLKGATE_CON(13), 12, GFLAGS),
	GATE(0, "hclk_mcu", "fclk_mcu_src", 0, RK3368_CLKGATE_CON(13), 11, GFLAGS),

	/* hclk_cpu gates */
	GATE(HCLK_SPDIF, "hclk_spdif", "hclk_bus", 0, RK3368_CLKGATE_CON(12), 10, GFLAGS),
	GATE(HCLK_ROM, "hclk_rom", "hclk_bus", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(12), 9, GFLAGS),
	GATE(HCLK_I2S_2CH, "hclk_i2s_2ch", "hclk_bus", 0, RK3368_CLKGATE_CON(12), 8, GFLAGS),
	GATE(HCLK_I2S_8CH, "hclk_i2s_8ch", "hclk_bus", 0, RK3368_CLKGATE_CON(12), 7, GFLAGS),
	GATE(HCLK_TSP, "hclk_tsp", "hclk_bus", 0, RK3368_CLKGATE_CON(13), 10, GFLAGS),
	GATE(HCLK_CRYPTO, "hclk_crypto", "hclk_bus", 0, RK3368_CLKGATE_CON(13), 4, GFLAGS),
	GATE(MCLK_CRYPTO, "mclk_crypto", "hclk_bus", 0, RK3368_CLKGATE_CON(13), 3, GFLAGS),

	/* pclk_cpu gates */
	GATE(PCLK_DDRPHY, "pclk_ddrphy", "pclk_bus", 0, RK3368_CLKGATE_CON(12), 14, GFLAGS),
	GATE(PCLK_DDRUPCTL, "pclk_ddrupctl", "pclk_bus", 0, RK3368_CLKGATE_CON(12), 13, GFLAGS),
	GATE(PCLK_I2C1, "pclk_i2c1", "pclk_bus", 0, RK3368_CLKGATE_CON(12), 3, GFLAGS),
	GATE(PCLK_I2C0, "pclk_i2c0", "pclk_bus", 0, RK3368_CLKGATE_CON(12), 2, GFLAGS),
	GATE(PCLK_MAILBOX, "pclk_mailbox", "pclk_bus", 0, RK3368_CLKGATE_CON(12), 1, GFLAGS),
	GATE(PCLK_PWM0, "pclk_pwm0", "pclk_bus", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(12), 0, GFLAGS),
	GATE(PCLK_SIM, "pclk_sim", "pclk_bus", 0, RK3368_CLKGATE_CON(13), 8, GFLAGS),
	GATE(PCLK_PWM1, "pclk_pwm1", "pclk_bus", 0, RK3368_CLKGATE_CON(13), 6, GFLAGS),
	GATE(PCLK_UART2, "pclk_uart2", "pclk_bus", 0, RK3368_CLKGATE_CON(13), 5, GFLAGS),
	GATE(0, "pclk_efuse_256", "pclk_bus", 0, RK3368_CLKGATE_CON(13), 1, GFLAGS),
	GATE(0, "pclk_efuse_1024", "pclk_bus", 0, RK3368_CLKGATE_CON(13), 0, GFLAGS),





	/* aclk_peri gates */
	GATE(ACLK_DMAC_PERI, "aclk_dmac_peri", "aclk_peri", 0, RK3368_CLKGATE_CON(19), 3, GFLAGS),
	GATE(0, "aclk_peri_axi_matrix", "aclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(19), 2, GFLAGS),
	GATE(HCLK_SFC, "hclk_sfc", "aclk_peri", 0, RK3368_CLKGATE_CON(20), 15, GFLAGS),
	GATE(ACLK_GMAC, "aclk_gmac", "aclk_peri", 0, RK3368_CLKGATE_CON(20), 13, GFLAGS),
	GATE(0, "aclk_peri_niu", "aclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(20), 8, GFLAGS),
	GATE(ACLK_PERI_MMU, "aclk_peri_mmu", "aclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(21), 4, GFLAGS),

	/* hclk_peri gates */
	GATE(0, "hclk_peri_axi_matrix", "hclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(19), 0, GFLAGS),
	GATE(HCLK_NANDC0, "hclk_nandc0", "hclk_peri", 0, RK3368_CLKGATE_CON(20), 11, GFLAGS),
	GATE(0, "hclk_mmc_peri", "hclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(20), 10, GFLAGS),
	GATE(0, "hclk_emem_peri", "hclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(20), 9, GFLAGS),
	GATE(0, "hclk_peri_ahb_arbi", "hclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(20), 7, GFLAGS),
	GATE(0, "hclk_usb_peri", "hclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(20), 6, GFLAGS),
	GATE(HCLK_HSIC, "hclk_hsic", "hclk_peri", 0, RK3368_CLKGATE_CON(20), 5, GFLAGS),
	GATE(HCLK_HOST1, "hclk_host1", "hclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(20), 4, GFLAGS),
	GATE(HCLK_HOST0, "hclk_host0", "hclk_peri", 0, RK3368_CLKGATE_CON(20), 3, GFLAGS),
	GATE(0, "pmu_hclk_otg0", "hclk_peri", 0, RK3368_CLKGATE_CON(20), 2, GFLAGS),
	GATE(HCLK_OTG0, "hclk_otg0", "hclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(20), 1, GFLAGS),
	GATE(HCLK_HSADC, "hclk_hsadc", "hclk_peri", 0, RK3368_CLKGATE_CON(21), 3, GFLAGS),
	GATE(HCLK_EMMC, "hclk_emmc", "hclk_peri", 0, RK3368_CLKGATE_CON(21), 2, GFLAGS),
	GATE(HCLK_SDIO0, "hclk_sdio0", "hclk_peri", 0, RK3368_CLKGATE_CON(21), 1, GFLAGS),
	GATE(HCLK_SDMMC, "hclk_sdmmc", "hclk_peri", 0, RK3368_CLKGATE_CON(21), 0, GFLAGS),

	/* pclk_peri gates */
	GATE(PCLK_SARADC, "pclk_saradc", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 15, GFLAGS),
	GATE(PCLK_I2C5, "pclk_i2c5", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 14, GFLAGS),
	GATE(PCLK_I2C4, "pclk_i2c4", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 13, GFLAGS),
	GATE(PCLK_I2C3, "pclk_i2c3", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 12, GFLAGS),
	GATE(PCLK_I2C2, "pclk_i2c2", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 11, GFLAGS),
	GATE(PCLK_UART4, "pclk_uart4", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 10, GFLAGS),
	GATE(PCLK_UART3, "pclk_uart3", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 9, GFLAGS),
	GATE(PCLK_UART1, "pclk_uart1", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 8, GFLAGS),
	GATE(PCLK_UART0, "pclk_uart0", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 7, GFLAGS),
	GATE(PCLK_SPI2, "pclk_spi2", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 6, GFLAGS),
	GATE(PCLK_SPI1, "pclk_spi1", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 5, GFLAGS),
	GATE(PCLK_SPI0, "pclk_spi0", "pclk_peri", 0, RK3368_CLKGATE_CON(19), 4, GFLAGS),
	GATE(0, "pclk_peri_axi_matrix", "pclk_peri", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(19), 1, GFLAGS),
	GATE(PCLK_GMAC, "pclk_gmac", "pclk_peri", 0, RK3368_CLKGATE_CON(20), 14, GFLAGS),
	GATE(PCLK_TSADC, "pclk_tsadc", "pclk_peri", 0, RK3368_CLKGATE_CON(20), 0, GFLAGS),




	/* pclk_pd_alive gates */
	GATE(PCLK_TIMER1, "pclk_timer1", "pclk_pd_alive", 0, RK3368_CLKGATE_CON(14), 8, GFLAGS),
	GATE(PCLK_TIMER0, "pclk_timer0", "pclk_pd_alive", 0, RK3368_CLKGATE_CON(14), 7, GFLAGS),
	GATE(0, "pclk_alive_niu", "pclk_pd_alive", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(14), 12, GFLAGS),
	GATE(PCLK_GRF, "pclk_grf", "pclk_pd_alive", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(14), 11, GFLAGS),
	GATE(PCLK_GPIO3, "pclk_gpio3", "pclk_pd_alive", 0, RK3368_CLKGATE_CON(14), 3, GFLAGS),
	GATE(PCLK_GPIO2, "pclk_gpio2", "pclk_pd_alive", 0, RK3368_CLKGATE_CON(14), 2, GFLAGS),
	GATE(PCLK_GPIO1, "pclk_gpio1", "pclk_pd_alive", 0, RK3368_CLKGATE_CON(14), 1, GFLAGS),
	//PCLK_WDT sgrf_soc_con3[7]
	//PCLK_WDT_M3 sgrf_soc_con3[9]
	//PCLK_STIMER sgrf_soc_con3[5]


	/* pclk_vio gates */
	GATE(0, "pclk_dphyrx", "pclk_vio", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(14), 8, GFLAGS),
	GATE(0, "pclk_dphytx", "pclk_vio", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(14), 8, GFLAGS),


	/* pclk_pd_pmu gates */
	GATE(PCLK_PMUGRF, "pclk_pmugrf", "pclk_pd_pmu", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(17), 0, GFLAGS),
	GATE(PCLK_GPIO0, "pclk_gpio0", "pclk_pd_pmu", 0, RK3368_CLKGATE_CON(17), 4, GFLAGS),
	GATE(PCLK_SGRF, "pclk_sgrf", "pclk_pd_pmu", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(17), 3, GFLAGS),
	GATE(0, "pclk_pmu_noc", "pclk_pd_pmu", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(17), 2, GFLAGS),
	GATE(0, "pclk_intmem1", "pclk_pd_pmu", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(17), 1, GFLAGS),
	GATE(PCLK_PMU, "pclk_pmu", "pclk_pd_pmu", CLK_IGNORE_UNUSED, RK3368_CLKGATE_CON(17), 2, GFLAGS),




/*
	GATE(PCLK_PS2C, "pclk_ps2c", "pclk_peri", 0, RK3288_CLKGATE_CON(6), 7, GFLAGS),
	GATE(PCLK_SIM, "pclk_sim", "pclk_peri", 0, RK3288_CLKGATE_CON(7), 3, GFLAGS),
	GATE(0, "hclk_mem", "hclk_peri", CLK_IGNORE_UNUSED, RK3288_CLKGATE_CON(7), 13, GFLAGS),
	GATE(HCLK_NANDC1, "hclk_nandc1", "hclk_peri", 0, RK3288_CLKGATE_CON(7), 15, GFLAGS),
	GATE(HCLK_TSP, "hclk_tsp", "hclk_peri", 0, RK3288_CLKGATE_CON(8), 8, GFLAGS),
	GATE(HCLK_SDIO1, "hclk_sdio1", "hclk_peri", 0, RK3288_CLKGATE_CON(8), 5, GFLAGS),
*/

};


static void __init rk3368_clk_init(struct device_node *np)
{
	void __iomem *reg_base;
	struct clk *clk;

	reg_base = of_iomap(np, 0);
	if (!reg_base) {
		pr_err("%s: could not map cru region\n", __func__);
		return;
	}

	rockchip_clk_init(np, reg_base, CLK_NR_CLKS);

	/* xin12m is created by an cru-internal divider */
	clk = clk_register_fixed_factor(NULL, "xin12m", "xin24m", 0, 1, 2);
	if (IS_ERR(clk))
		pr_warn("%s: could not register clock xin12m: %ld\n",
			__func__, PTR_ERR(clk));

	/* xin12m is created by an cru-internal divider */
	clk = clk_register_fixed_factor(NULL, "ddrphy_div4", "ddrphy_src", 0, 1, 4);
	if (IS_ERR(clk))
		pr_warn("%s: could not register clock xin12m: %ld\n",
			__func__, PTR_ERR(clk));


	rockchip_clk_register_plls(rk3368_pll_clks,
				   ARRAY_SIZE(rk3368_pll_clks),
				   RK3368_GRF_SOC_STATUS0);
	rockchip_clk_register_branches(rk3368_clk_branches,
				  ARRAY_SIZE(rk3368_clk_branches));

	rockchip_register_softrst(np, 15, reg_base + RK3368_SOFTRST_CON(0),
				  ROCKCHIP_SOFTRST_HIWORD_MASK);

	rockchip_register_restart_notifier(RK3368_GLB_SRST_FST);
}
CLK_OF_DECLARE(rk3368_cru, "rockchip,rk3368-cru", rk3368_clk_init);
