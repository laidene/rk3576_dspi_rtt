/*
 * Shell tests for the four onboard WS2812 RGB LEDs.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rtthread.h>

#ifdef BSP_USING_WS2812

#include <ws2812/drv_ws2812.h>

#define WS2812_TEST_BRIGHTNESS 0x20U

static int ws2812_test(int argc, char **argv)
{
    static const struct ws2812_rgb colors[WS2812_LED_COUNT] = {
        { WS2812_TEST_BRIGHTNESS, 0U,                      WS2812_TEST_BRIGHTNESS },  /* Purple */
        { WS2812_TEST_BRIGHTNESS, 0U,                      0U                      }, /* Red */
        { 0U,                      WS2812_TEST_BRIGHTNESS, 0U                      }, /* Green */
        { 0U,                      0U,                      WS2812_TEST_BRIGHTNESS }, /* Blue */
    };
    (void)argc;
    (void)argv;

    return ws2812_write(colors, WS2812_LED_COUNT);
}
MSH_CMD_EXPORT(ws2812_test, set onboard LEDs to purple red green and blue);

static int ws2812_off(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    return ws2812_clear();
}
MSH_CMD_EXPORT(ws2812_off, turn off all onboard RGB LEDs);

#endif /* BSP_USING_WS2812 */
