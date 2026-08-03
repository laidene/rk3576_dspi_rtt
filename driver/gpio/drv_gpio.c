/*
 * RK3576 adapter for the RT-Thread PIN device.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtdevice.h>

#include <drv_iomux.h>

#include "drv_gpio.h"
#include "gpio_internal.h"

static rt_uint32_t gpio_open_drain[RK3576_GPIO_BANK_COUNT];

static rt_bool_t rk3576_gpio_decode(rt_base_t pin, rt_uint32_t *bank, rt_uint32_t *offset)
{
    if ((pin < 0) || ((rt_ubase_t)pin >= (RK3576_GPIO_BANK_COUNT * RK3576_GPIO_PINS_PER_BANK))) {
        return RT_FALSE;
    }

    *bank = (rt_uint32_t)pin / RK3576_GPIO_PINS_PER_BANK;
    *offset = (rt_uint32_t)pin % RK3576_GPIO_PINS_PER_BANK;
    return RT_TRUE;
}

static void rk3576_pin_mode(struct rt_device *device, rt_base_t pin, rt_base_t mode)
{
    rt_base_t level;
    rt_uint32_t bank;
    rt_uint32_t offset;
    rt_uint32_t bit;

    (void)device;
    if (!rk3576_gpio_decode(pin, &bank, &offset) ||
        ((mode != PIN_MODE_OUTPUT) && (mode != PIN_MODE_INPUT) &&
         (mode != PIN_MODE_INPUT_PULLUP) && (mode != PIN_MODE_INPUT_PULLDOWN) &&
         (mode != PIN_MODE_OUTPUT_OD))) {
        return;
    }

    if (rk3576_iomux_set(bank, offset, 0U) != RT_EOK) {
        return;
    }

    bit = 1U << offset;
    level = rt_hw_interrupt_disable();
    gpio_open_drain[bank] &= ~bit;
    rt_hw_interrupt_enable(level);

    switch (mode) {
    case PIN_MODE_OUTPUT:
        rk3576_pull_set(bank, offset, RK3576_PULL_NONE);
        rk3576_gpio_hw_set_direction(bank, offset, RT_TRUE);
        break;
    case PIN_MODE_INPUT:
        rk3576_gpio_hw_set_direction(bank, offset, RT_FALSE);
        rk3576_pull_set(bank, offset, RK3576_PULL_NONE);
        break;
    case PIN_MODE_INPUT_PULLUP:
        rk3576_gpio_hw_set_direction(bank, offset, RT_FALSE);
        rk3576_pull_set(bank, offset, RK3576_PULL_UP);
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        rk3576_gpio_hw_set_direction(bank, offset, RT_FALSE);
        rk3576_pull_set(bank, offset, RK3576_PULL_DOWN);
        break;
    case PIN_MODE_OUTPUT_OD:
        rk3576_gpio_hw_set_direction(bank, offset, RT_FALSE);
        rk3576_gpio_hw_set_value(bank, offset, RT_FALSE);
        level = rt_hw_interrupt_disable();
        gpio_open_drain[bank] |= bit;
        rt_hw_interrupt_enable(level);
        break;
    default:
        return;
    }
}

static void rk3576_pin_write(struct rt_device *device, rt_base_t pin, rt_base_t value)
{
    rt_uint32_t bank;
    rt_uint32_t offset;

    (void)device;
    if (!rk3576_gpio_decode(pin, &bank, &offset)) {
        return;
    }

    if ((gpio_open_drain[bank] & (1U << offset)) != 0U) {
        if (value == PIN_LOW) {
            rk3576_gpio_hw_set_value(bank, offset, RT_FALSE);
            rk3576_gpio_hw_set_direction(bank, offset, RT_TRUE);
        } else {
            rk3576_gpio_hw_set_direction(bank, offset, RT_FALSE);
        }
        return;
    }

    rk3576_gpio_hw_set_value(bank, offset, value != PIN_LOW);
}

static int rk3576_pin_read(struct rt_device *device, rt_base_t pin)
{
    rt_uint32_t bank;
    rt_uint32_t offset;

    (void)device;
    if (!rk3576_gpio_decode(pin, &bank, &offset)) {
        return PIN_LOW;
    }

    return rk3576_gpio_hw_get_value(bank, offset);
}

static rt_base_t rk3576_pin_get(const char *name)
{
    rt_uint32_t bank;
    rt_uint32_t pin = 0U;
    rt_size_t index;

    if ((name == RT_NULL) || ((name[0] != 'P') && (name[0] != 'p')) ||
        (name[1] < '0') || (name[1] > '4')) {
        return -RT_EINVAL;
    }

    bank = (rt_uint32_t)(name[1] - '0');
    index = name[2] == '.' ? 3U : 2U;
    if ((name[index] < '0') || (name[index] > '9')) {
        return -RT_EINVAL;
    }

    while ((name[index] >= '0') && (name[index] <= '9')) {
        pin = pin * 10U + (rt_uint32_t)(name[index] - '0');
        index++;
    }
    if ((name[index] != '\0') || (pin >= RK3576_GPIO_PINS_PER_BANK)) {
        return -RT_EINVAL;
    }

    return (rt_base_t)RK3576_GPIO_PIN(bank, pin);
}

static const struct rt_pin_ops rk3576_pin_ops = {
    .pin_mode = rk3576_pin_mode,
    .pin_write = rk3576_pin_write,
    .pin_read = rk3576_pin_read,
    .pin_attach_irq = RT_NULL,
    .pin_detach_irq = RT_NULL,
    .pin_irq_enable = RT_NULL,
    .pin_get = rk3576_pin_get,
};

int rt_hw_gpio_init(void)
{
    rt_uint32_t bank;
    rt_err_t ret;

    for (bank = 0; bank < RK3576_GPIO_BANK_COUNT; bank++) {
        ret = rk3576_gpio_clock_enable(bank);
        if (ret != RT_EOK) {
            return ret;
        }
    }

    return rt_device_pin_register("pin", &rk3576_pin_ops, RT_NULL);
}
