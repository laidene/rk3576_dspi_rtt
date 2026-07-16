/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_CRU_H__
#define __DRV_CRU_H__

#include <rtthread.h>

/* PLL的值是直接利用uboot初始化的，并没有手动设置到频率 */
#define RK3576_OSC_HZ      24000000UL
#define RK3576_GPLL_HZ     1188000000UL
#define RK3576_CPLL_HZ     1000000000UL
#define RK3576_AUPLL_HZ    786432000UL

#define RK3576_CLKSEL_CON(n)      (0x0300U + ((n) * 4U))        /* CRU_CLKSEL_CON<X> */
#define RK3576_CLKGATE_CON(n)     (0x0800U + ((n) * 4U))        /* CRU_GATE_CON<X> */
#define RK3576_SOFTRST_CON(n)     (0x0a00U + ((n) * 4U))        /* CRU_SOFTRST_CON<X> */

#define RK3576_PMU1CRU_OFFSET     0x20000U
#define RK3576_PMU_CLKSEL_CON(n)  (RK3576_PMU1CRU_OFFSET + 0x0300U + ((n) * 4U))
#define RK3576_PMU_CLKGATE_CON(n) (RK3576_PMU1CRU_OFFSET + 0x0800U + ((n) * 4U))

void rk3576_cru_init(void);

rt_uint32_t rk3576_cru_read         (rt_uint32_t offset);
void        rk3576_cru_write        (rt_uint32_t offset, rt_uint32_t value);
void        rk3576_cru_hiword_update(rt_uint32_t offset, rt_uint32_t mask, rt_uint32_t value);

void rk3576_cru_gate_enable   (rt_uint32_t offset, rt_uint32_t bit);
void rk3576_cru_gate_disable  (rt_uint32_t offset, rt_uint32_t bit);
void rk3576_cru_reset_assert  (rt_uint32_t offset, rt_uint32_t bit);
void rk3576_cru_reset_deassert(rt_uint32_t offset, rt_uint32_t bit);

rt_err_t   rk3576_cru_uart_enable  (rt_uint32_t id);
rt_uint32_t rk3576_cru_get_uart_clk(rt_uint32_t id);
rt_uint32_t rk3576_cru_set_uart_clk(rt_uint32_t id, rt_uint32_t rate);

rt_err_t rk3576_cru_gmac0_enable(void);
void     rk3576_cru_gmac0_reset(void);

#endif /* __DRV_CRU_H__ */
