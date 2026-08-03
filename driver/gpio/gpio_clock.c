/*
 * RK3576 GPIO clocks.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drv_cru.h>

#include "gpio_internal.h"

rt_err_t rk3576_gpio_clock_enable(rt_uint32_t bank)
{
    switch (bank) {
    case 0U:
        rk3576_cru_gate_enable(RK3576_PMU_CLKGATE_CON(7), 6);
        rk3576_cru_gate_enable(RK3576_PMU_CLKGATE_CON(7), 7);
        break;
    case 1U:
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(17), 15);
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(18), 0);
        break;
    case 2U:
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(18), 1);
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(18), 2);
        break;
    case 3U:
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(18), 3);
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(18), 4);
        break;
    case 4U:
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(18), 5);
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(18), 6);
        break;
    default:
        return -RT_EINVAL;
    }

    return RT_EOK;
}
