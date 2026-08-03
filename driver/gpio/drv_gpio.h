/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_GPIO_H__
#define __DRV_GPIO_H__

#include <rtthread.h>

#define RK3576_GPIO_BANK_COUNT     5U
#define RK3576_GPIO_PINS_PER_BANK 32U
#define RK3576_GPIO_PIN(bank, pin) (((bank) * RK3576_GPIO_PINS_PER_BANK) + (pin))

int rt_hw_gpio_init(void);

#endif /* __DRV_GPIO_H__ */
