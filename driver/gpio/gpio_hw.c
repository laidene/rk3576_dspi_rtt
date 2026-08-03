/*
 * RK3576 GPIO v2 register access.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>

#include <board.h>

#include "drv_gpio.h"
#include "gpio_internal.h"

#define GPIO_SWPORT_DR_L   0x0000U
#define GPIO_SWPORT_DDR_L  0x0008U
#define GPIO_EXT_PORT      0x0070U
#define GPIO_PINS_PER_REG  16U

static const rt_ubase_t gpio_bases[RK3576_GPIO_BANK_COUNT] = {
    GPIO0_MMIO_BASE,
    GPIO1_MMIO_BASE,
    GPIO2_MMIO_BASE,
    GPIO3_MMIO_BASE,
    GPIO4_MMIO_BASE,
};

static void rk3576_gpio_write_bit(rt_uint32_t bank, rt_uint32_t reg, rt_uint32_t pin, rt_bool_t set)
{
    rt_uint32_t bit;

    reg += (pin / GPIO_PINS_PER_REG) * sizeof(rt_uint32_t);
    bit = 1U << (pin % GPIO_PINS_PER_REG);
    HWREG32(gpio_bases[bank] + reg) = (bit << 16) | (set ? bit : 0U);
}

void rk3576_gpio_hw_set_direction(rt_uint32_t bank, rt_uint32_t pin, rt_bool_t output)
{
    rk3576_gpio_write_bit(bank, GPIO_SWPORT_DDR_L, pin, output);
}

void rk3576_gpio_hw_set_value(rt_uint32_t bank, rt_uint32_t pin, rt_bool_t high)
{
    rk3576_gpio_write_bit(bank, GPIO_SWPORT_DR_L, pin, high);
}

int rk3576_gpio_hw_get_value(rt_uint32_t bank, rt_uint32_t pin)
{
    return (HWREG32(gpio_bases[bank] + GPIO_EXT_PORT) & (1U << pin)) != 0U ? 1 : 0;
}
