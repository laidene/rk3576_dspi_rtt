/*
 * DWC GMAC MDIO controller for RK3576 GMAC0.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gmac_internal.h"
#include "gmac_mdio.h"

#define GMAC_MDIO_ADDRESS        0x0200U /* GMAC_MAC_MDIO_ADDRESS */
#define GMAC_MDIO_DATA           0x0204U /* GMAC_MAC_MDIO_DATA */

#define GMAC_MDIO_PA_SHIFT       21U
#define GMAC_MDIO_RDA_SHIFT      16U
#define GMAC_MDIO_CR_SHIFT       8U
#define GMAC_MDIO_GOC_SHIFT      2U
#define GMAC_MDIO_BUSY           GMAC_BIT(0)
#define GMAC_MDIO_CR_100_150_MHZ 1U

/* 等待MDIO总线空闲 */
static rt_err_t gmac_mdio_wait_idle(struct rk3576_gmac *gmac)
{
    rt_uint32_t count;

    for (count = 0; count < 1000000U; count++) {
        if ((gmac_read(gmac, GMAC_MDIO_ADDRESS) & GMAC_MDIO_BUSY) == 0U) {
            return RT_EOK;
        }
        __asm__ volatile("nop");
    }

    return -RT_ETIMEOUT;
}

/* 通过MDIO读取PHY标准寄存器 */
int gmac_mdio_read(struct rk3576_gmac *gmac, rt_uint32_t phy, rt_uint32_t reg)
{
    enum {
        GMAC_MDIO_GOC_READ = 3U,
    };
    rt_uint32_t command;

    if (gmac_mdio_wait_idle(gmac) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    command = (phy                      << GMAC_MDIO_PA_SHIFT)  |
              (reg                      << GMAC_MDIO_RDA_SHIFT) |
              (GMAC_MDIO_CR_100_150_MHZ << GMAC_MDIO_CR_SHIFT)  |
              (GMAC_MDIO_GOC_READ       << GMAC_MDIO_GOC_SHIFT) |
               GMAC_MDIO_BUSY;
    gmac_write(gmac, GMAC_MDIO_ADDRESS, command);

    if (gmac_mdio_wait_idle(gmac) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    return (int)(gmac_read(gmac, GMAC_MDIO_DATA) & 0xffffU);
}

/* 通过MDIO写入PHY标准寄存器 */
rt_err_t gmac_mdio_write(struct rk3576_gmac *gmac, rt_uint32_t phy, rt_uint32_t reg, rt_uint16_t data)
{
    enum {
        GMAC_MDIO_GOC_WRITE = 1U,
    };
    rt_uint32_t command;

    if (gmac_mdio_wait_idle(gmac) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    gmac_write(gmac, GMAC_MDIO_DATA, data);
    command = (phy                      << GMAC_MDIO_PA_SHIFT)  |
              (reg                      << GMAC_MDIO_RDA_SHIFT) |
              (GMAC_MDIO_CR_100_150_MHZ << GMAC_MDIO_CR_SHIFT)  |
              (GMAC_MDIO_GOC_WRITE      << GMAC_MDIO_GOC_SHIFT) |
               GMAC_MDIO_BUSY;
    gmac_write(gmac, GMAC_MDIO_ADDRESS, command);

    return gmac_mdio_wait_idle(gmac);
}
