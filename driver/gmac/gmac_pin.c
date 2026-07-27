/*
 * RK3576 GMAC0 RGMII pin and PHY-reset configuration.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drv_iomux.h>

#include "gmac_internal.h"

#define SDGMAC_GRF_GMAC0_CON   0x0020U  /* GMAC0接口模式、时钟源和速率分频控制寄存器偏移 */
#define VCCIO_IOC_MISC_CON2    0x6408U  /* GMAC0 M0的RGMII RX/TX时钟延时控制寄存器偏移 */
#define VCCIO_IOC_MISC_CON3    0x640cU  /* GMAC0 M1的RGMII RX/TX时钟延时控制寄存器偏移 */
#define GMAC0_TX_DELAY         0x1bU    /* TX时钟延时链的档位值：27 */

#define GPIO_SWPORT_DR_H       0x0004U      /* GPIO 16~31输出电平寄存器偏移，GPIO0_C2使用数据位2 */
#define GPIO_SWPORT_DDR_H      0x000cU      /* GPIO 16~31方向寄存器偏移：1为输出，0为输入 */
#define GPIO0_C2_HIGH_REG_BIT  GMAC_BIT(2)  /* GPIO0_C2编号为18，在高半区寄存器中对应位2 */

static void gmac_gpio0_c2_set(rt_bool_t high)
{
    rt_uint32_t value = GPIO0_C2_HIGH_REG_BIT << 16;

    if (high) {
        value |= GPIO0_C2_HIGH_REG_BIT;
    }

    HWREG32(GPIO0_MMIO_BASE + GPIO_SWPORT_DR_H) = value;
}

/* 设置rgmii模式 设置收发延迟 */
static void gmac_rgmii_configure(void)
{
    rt_uint32_t grf_mask    = GMAC_BIT(7) | GMAC_BIT(6) | GMAC_BIT(5) | GMAC_BIT(3);
    rt_uint32_t delay_mask  = GMAC_BIT(15) | (0x7fU << 8) | GMAC_BIT(7) | 0x7fU;        /* 全部生效 */
    rt_uint32_t delay_value = GMAC_BIT(7) | GMAC0_TX_DELAY;                             /* 禁用rx_delay | delayline=0 | 使能tx_delay | delayline=GMAC0_TX_DELAY */

    /* RGMII mode, clock from CRU, and initial 1000 Mbit/s divider. */
    HWREG32(SDGMAC_GRF_MMIO_BASE + SDGMAC_GRF_GMAC0_CON) = GMAC_HIWORD_UPDATE(grf_mask, 0);         /* 时钟来自cru | 25M时钟 | rgmii模式 */
    
    /* TX delay is supplied by the SoC; RX delay is supplied by the PHY. */
    HWREG32(IOC_GRF_MMIO_BASE + VCCIO_IOC_MISC_CON2) = GMAC_HIWORD_UPDATE(delay_mask, delay_value); /* 设置gmac0 m0 */
}

rt_err_t rk3576_gmac_pins_init(void)
{
    static const rt_uint8_t pins[] = {
        RK_PB6, /* TXCLK */
        RK_PB3, /* TXCTL */
        RK_PB5, /* TXD0 */
        RK_PB4, /* TXD1 */
        RK_PC3, /* TXD2 */
        RK_PC2, /* TXD3 */
        RK_PD1, /* RXCLK */
        RK_PA7, /* RXCTL */
        RK_PB2, /* RXD0 */
        RK_PB1, /* RXD1 */
        RK_PD3, /* RXD2 */
        RK_PD2, /* RXD3 */
        RK_PA6, /* MDC */
        RK_PA5, /* MDIO */
    };
    rt_size_t i;
    rt_err_t ret;

    /* gmac0复用配置 */
    for (i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        ret = rk3576_iomux_set(3, pins[i], 3);
        if (ret != RT_EOK) {
            return ret;
        }

        ret = rk3576_pull_set(3, pins[i], RK3576_PULL_NONE);
        if (ret != RT_EOK) {
            return ret;
        }
    }

    /* 复位引脚复用为gpio */
    ret = rk3576_iomux_set(0, RK_PC2, 0);
    if (ret != RT_EOK) {
        return ret;
    }

    gmac_rgmii_configure();
    return RT_EOK;
}

/* 重启复位引脚 */
void rk3576_gmac_phy_reset(void)
{
    /* Preload low before switching the pin to output to avoid a high glitch. */
    gmac_gpio0_c2_set(RT_FALSE);
    HWREG32(GPIO0_MMIO_BASE + GPIO_SWPORT_DDR_H) = (GPIO0_C2_HIGH_REG_BIT << 16) | GPIO0_C2_HIGH_REG_BIT;
    rt_thread_mdelay(20);

    /* ETH0_RESET_N is active-low; drive high to release the PHY reset. */
    gmac_gpio0_c2_set(RT_TRUE);
    rt_thread_mdelay(100);
}
