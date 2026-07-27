/*
 * Motorcomm YT8521S PHY driver for RK3576 GMAC0.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gmac_internal.h"
#include "gmac_mdio.h"
#include "yt8521s.h"

#define PHY_SPEC_STATUS          0x11U

/* 读取PHY ID并启动自动协商 */
rt_err_t yt8521s_init(struct rk3576_gmac *gmac)
{
    enum {
        PHY_BMCR            = 0x00U,
        PHY_ID1             = 0x02U,
        PHY_ID2             = 0x03U,
        PHY_BMCR_AN_ENABLE  = GMAC_BIT(12),
        PHY_BMCR_AN_RESTART = GMAC_BIT(9),
    };
    int id1;
    int id2;
    int value;

    id1 = gmac_mdio_read(gmac, YT8521S_PHY_ADDR, PHY_ID1);
    id2 = gmac_mdio_read(gmac, YT8521S_PHY_ADDR, PHY_ID2);
    if ((id1 < 0) || (id2 < 0) || ((id1 == 0xffff) && (id2 == 0xffff))) {
        return -RT_EIO;
    }

    value = gmac_mdio_read(gmac, YT8521S_PHY_ADDR, PHY_BMCR);
    if (value < 0) {
        return -RT_EIO;
    }
    if (gmac_mdio_write(gmac, YT8521S_PHY_ADDR, PHY_BMCR, (rt_uint16_t)value | PHY_BMCR_AN_ENABLE | PHY_BMCR_AN_RESTART) != RT_EOK) {
        return -RT_EIO;
    }

    rt_kprintf("gmac0: PHY id %04x:%04x, autonegotiation started\n", id1, id2);
    return RT_EOK;
}

/* 读取phy寄存器11 */
int yt8521s_read_status(struct rk3576_gmac *gmac)
{
    return gmac_mdio_read(gmac, YT8521S_PHY_ADDR, PHY_SPEC_STATUS);
}

/* 解析寄存器11的状态 */
rt_err_t yt8521s_get_link(struct rk3576_gmac *gmac, rt_bool_t *up, rt_uint32_t *speed, rt_bool_t *full_duplex)
{
    enum {
        PHY_STATUS_SPEED_MASK   = 3U << 14,
        PHY_STATUS_DUPLEX       = GMAC_BIT(13),
        PHY_STATUS_LINK         = GMAC_BIT(10),
    };
    int value;
    rt_uint32_t speed_mode;

    value = yt8521s_read_status(gmac);
    if (value < 0) {
        return -RT_EIO;
    }

    *up             = ((rt_uint32_t)value & PHY_STATUS_LINK) != 0U;
    *full_duplex    = ((rt_uint32_t)value & PHY_STATUS_DUPLEX) != 0U;
    speed_mode      = ((rt_uint32_t)value & PHY_STATUS_SPEED_MASK) >> 14;
    if (speed_mode == 2U) {
        *speed = 1000U;
    } else if (speed_mode == 1U) {
        *speed = 100U;
    } else {
        *speed = 10U;
    }

    return RT_EOK;
}
