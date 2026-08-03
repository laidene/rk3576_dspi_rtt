/*
 * RK3576 GPIO internal interfaces.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __GPIO_INTERNAL_H__
#define __GPIO_INTERNAL_H__

#include <rtthread.h>

rt_err_t rk3576_gpio_clock_enable(rt_uint32_t bank);

void rk3576_gpio_hw_set_direction(rt_uint32_t bank, rt_uint32_t pin, rt_bool_t output);
void rk3576_gpio_hw_set_value(rt_uint32_t bank, rt_uint32_t pin, rt_bool_t high);
int rk3576_gpio_hw_get_value(rt_uint32_t bank, rt_uint32_t pin);

#endif /* __GPIO_INTERNAL_H__ */
