/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_IOMUX_H__
#define __DRV_IOMUX_H__

#include <rtthread.h>

#define RK_PA0                  0U
#define RK_PA1                  1U
#define RK_PA2                  2U
#define RK_PA3                  3U
#define RK_PA4                  4U
#define RK_PA5                  5U
#define RK_PA6                  6U
#define RK_PA7                  7U
#define RK_PB0                  8U
#define RK_PB1                  9U
#define RK_PB2                  10U
#define RK_PB3                  11U
#define RK_PB4                  12U
#define RK_PB5                  13U
#define RK_PB6                  14U
#define RK_PB7                  15U
#define RK_PC0                  16U
#define RK_PC1                  17U
#define RK_PC2                  18U
#define RK_PC3                  19U
#define RK_PC4                  20U
#define RK_PC5                  21U
#define RK_PC6                  22U
#define RK_PC7                  23U
#define RK_PD0                  24U
#define RK_PD1                  25U
#define RK_PD2                  26U
#define RK_PD3                  27U
#define RK_PD4                  28U
#define RK_PD5                  29U
#define RK_PD6                  30U
#define RK_PD7                  31U

#define RK3576_GPIO_BANKS       5U
#define RK3576_GPIO_PINS        32U
#define RK3576_DRIVE_LEVEL_MAX  7U

enum rk3576_pull_mode {
    RK3576_PULL_NONE = 0,
    RK3576_PULL_UP,
    RK3576_PULL_DOWN,
};

enum rk3576_uart_mux_group {
    RK3576_UART_M0 = 0,
    RK3576_UART_M1,
    RK3576_UART_M2,
    RK3576_UART_M3,
};

rt_err_t rk3576_iomux_set  (rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t mux);
rt_err_t rk3576_pull_set   (rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t pull);
rt_err_t rk3576_drive_set  (rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t level);
rt_err_t rk3576_schmitt_set(rt_uint32_t bank, rt_uint32_t pin, rt_bool_t enable);

rt_err_t rk3576_uart_iomux_init(rt_uint32_t id, rt_uint32_t mux_group);
rt_err_t rk3576_gmac0_iomux_init(void);

#endif /* __DRV_IOMUX_H__ */
