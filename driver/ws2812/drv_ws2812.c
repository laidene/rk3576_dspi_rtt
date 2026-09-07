/*
 * WS2812C-2020 driver for the four onboard RGB LEDs.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtdevice.h>
#include <rtthread.h>

#include <board.h>
#include <drv_iomux.h>
#include <gpio/drv_gpio.h>

#include "drv_ws2812.h"

#define WS2812_DATA_PIN       RK3576_GPIO_PIN(4U, RK_PC7)
#define WS2812_GPIO_DR_H      0x0004U
#define WS2812_GPIO_HIGH_BIT  (1U << (RK_PC7 - 16U))
#define WS2812_T0H_DELAY_NS   220U
#define WS2812_T1H_DELAY_NS   650U
#define WS2812_TL_DELAY_NS    650U
#define WS2812_RESET_TIME_MS  1U

struct ws2812_timing {
    rt_uint64_t t0h_ticks;
    rt_uint64_t t1h_ticks;
    rt_uint64_t tl_ticks;
};

static inline __attribute__((always_inline)) rt_uint64_t ws2812_counter_get(void)
{
    rt_uint64_t value;

    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(value));
    return value;
}

static inline __attribute__((always_inline)) rt_uint64_t ws2812_counter_frequency_get(void)
{
    rt_uint64_t value;

    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

static inline __attribute__((always_inline)) void ws2812_wait_until(rt_uint64_t deadline)
{
    while ((rt_int64_t)(ws2812_counter_get() - deadline) < 0) {
        __asm__ volatile("nop");
    }
}

static inline __attribute__((always_inline)) void ws2812_data_write(rt_bool_t high)
{
    HWREG32(GPIO4_MMIO_BASE + WS2812_GPIO_DR_H) =
        (WS2812_GPIO_HIGH_BIT << 16) | (high ? WS2812_GPIO_HIGH_BIT : 0U);
}

static rt_uint64_t ws2812_ns_to_ticks(rt_uint64_t frequency, rt_uint32_t nanoseconds)
{
    return (frequency * nanoseconds + 999999999ULL) / 1000000000ULL;
}

static rt_err_t __attribute__((optimize("O2")))
ws2812_send(const struct ws2812_rgb *colors, rt_size_t count)
{
    struct ws2812_timing timing;
    rt_uint64_t frequency;
    rt_uint64_t deadline;
    rt_uint64_t high_ticks;
    rt_base_t level;
    rt_uint32_t value;
    rt_uint32_t mask;
    rt_size_t led;

    frequency = ws2812_counter_frequency_get();
    timing.t0h_ticks = ws2812_ns_to_ticks(frequency, WS2812_T0H_DELAY_NS);
    timing.t1h_ticks = ws2812_ns_to_ticks(frequency, WS2812_T1H_DELAY_NS);
    timing.tl_ticks = ws2812_ns_to_ticks(frequency, WS2812_TL_DELAY_NS);
    if ((timing.t0h_ticks == 0U) || (timing.t1h_ticks == 0U) ||
        (timing.tl_ticks == 0U)) {
        return -RT_ERROR;
    }

    level = rt_hw_interrupt_disable();
    for (led = 0; led < count; led++) {
        value = ((rt_uint32_t)colors[led].green << 16) |
                ((rt_uint32_t)colors[led].red << 8) |
                colors[led].blue;

        for (mask = 1U << 23; mask != 0U; mask >>= 1) {
            high_ticks = (value & mask) != 0U ? timing.t1h_ticks : timing.t0h_ticks;
            deadline = ws2812_counter_get() + high_ticks;
            ws2812_data_write(RT_TRUE);
            ws2812_wait_until(deadline);
            deadline = ws2812_counter_get() + timing.tl_ticks;
            ws2812_data_write(RT_FALSE);
            ws2812_wait_until(deadline);
        }
    }
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

rt_err_t ws2812_write(const struct ws2812_rgb *colors, rt_size_t count)
{
    rt_err_t ret;

    if ((colors == RT_NULL) || (count == 0U) || (count > WS2812_LED_COUNT)) {
        return -RT_EINVAL;
    }

    rt_pin_mode(WS2812_DATA_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(WS2812_DATA_PIN, PIN_LOW);
    rt_thread_mdelay(WS2812_RESET_TIME_MS);

    ret = ws2812_send(colors, count);
    rt_thread_mdelay(WS2812_RESET_TIME_MS);
    return ret;
}

rt_err_t ws2812_clear(void)
{
    static const struct ws2812_rgb colors[WS2812_LED_COUNT] = { 0 };

    return ws2812_write(colors, WS2812_LED_COUNT);
}
