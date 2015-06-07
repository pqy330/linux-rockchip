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

	rockchip_clk_register_plls(rk3368_pll_clks,
				   ARRAY_SIZE(rk3368_pll_clks),
				   RK3368_GRF_SOC_STATUS0);

	rockchip_register_softrst(np, 15, reg_base + RK3368_SOFTRST_CON(0),
				  ROCKCHIP_SOFTRST_HIWORD_MASK);

	rockchip_register_restart_notifier(RK3368_GLB_SRST_FST);
}
CLK_OF_DECLARE(rk3368_cru, "rockchip,rk3368-cru", rk3368_clk_init);
