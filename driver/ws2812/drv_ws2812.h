/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_WS2812_H__
#define __DRV_WS2812_H__

#include <rtthread.h>

#define WS2812_LED_COUNT 4U

struct ws2812_rgb {
    rt_uint8_t red;
    rt_uint8_t green;
    rt_uint8_t blue;
};

rt_err_t ws2812_write(const struct ws2812_rgb *colors, rt_size_t count);
rt_err_t ws2812_clear(void);

#endif /* __DRV_WS2812_H__ */
