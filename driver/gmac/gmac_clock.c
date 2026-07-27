/*
 * RK3576 GMAC0 clock, reset, and power-domain control.
 *
 * 参考linux驱动
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drv_cru.h>

#include "gmac_internal.h"

#define SDGMAC_GRF_GMAC0_CON                0x0020U /* GMAC0接口模式、时钟源和速率分频控制寄存器偏移 */

#define PMU_PWR_GATE_SFTCON0                0x0210U /* PMU_PWR_GATE_CON1     电源控制的寄存器偏移 */
#define PMU_BUS_IDLE_REQ                    0x0114U /* PMU_BIU_IDLE_SFTCON1  SDGMAC总线空闲请求的软件控制寄存器 */
#define PMU_BUS_IDLE_ACK                    0x0120U /* PMU_BIU_IDLE_ACK_STS */
#define PMU_BUS_IDLE_ST                     0x0128U /* PMU_BIU_IDLE_STS */
#define PMU_CLK_UNGATE                      0x0144U /* PMU_BIU_GATMASK_SFTCON1 是否允许 开启或控制对应信号时钟 的寄存器操作 */
#define PMU_REPAIR_STATUS                   0x0570U /* PMU_BISR_PWR_REPAIR_STATUS0 */
#define PMU_SDGMAC_PWR_BIT                  GMAC_BIT(7)
#define PMU_SDGMAC_IDLE_REQ_BIT             GMAC_BIT(1)
#define PMU_SDGMAC_IDLE_STATUS_BIT          GMAC_BIT(17)
#define PMU_SDGMAC_CLK_MASK                 (GMAC_BIT(1) | GMAC_BIT(2))

rt_err_t rk3576_gmac_clock_enable(void)
{
    /* CPLL / 8 = 125 MHz rgmii源时钟 */
    rk3576_cru_hiword_update(RK3576_CLKSEL_CON(30), 0x1fU << 10, 7U << 10);
    rk3576_cru_gate_enable(RK3576_CLKGATE_CON(3), 6);

    /* ACLK_GMAC0 and its parent dma时钟 */
    rk3576_cru_gate_enable(RK3576_CLKGATE_CON(42), 1);
    rk3576_cru_gate_enable(RK3576_CLKGATE_CON(42), 7);

    /* PCLK_GMAC0 and its parent are used 访问寄存器时钟 */
    rk3576_cru_gate_enable(RK3576_CLKGATE_CON(42), 2);
    rk3576_cru_gate_enable(RK3576_CLKGATE_CON(42), 9);

    /* GPIO0 复位引脚时钟 */
    rk3576_cru_gate_enable(RK3576_PMU_CLKGATE_CON(7), 6);

    return RT_EOK;
}

rt_err_t rk3576_gmac_power_on(void)
{
    rt_tick_t timeout;
    /* 打开控制，让寄存器可以控制上电 */
    HWREG32(PMU_MMIO_BASE + PMU_CLK_UNGATE) = GMAC_HIWORD_UPDATE(PMU_SDGMAC_CLK_MASK, PMU_SDGMAC_CLK_MASK);

    /* 若PD_SDGMAC尚未上电，则清除关断请求，并等待上电完成。 */
    if ((HWREG32(PMU_MMIO_BASE + PMU_REPAIR_STATUS) & PMU_SDGMAC_PWR_BIT) == 0U) {
        /* 上电 */
        HWREG32(PMU_MMIO_BASE + PMU_PWR_GATE_SFTCON0) = GMAC_HIWORD_UPDATE(PMU_SDGMAC_PWR_BIT, 0);

        /* 读取结果直至超时 */
        timeout = rt_tick_get() + rt_tick_from_millisecond(100);
        while ((HWREG32(PMU_MMIO_BASE + PMU_REPAIR_STATUS) & PMU_SDGMAC_PWR_BIT) == 0U) {
            if ((rt_int32_t)(rt_tick_get() - timeout) >= 0) {
                return -RT_ETIMEOUT;
            }
            rt_thread_mdelay(1);
        }
    }

    /* 取消SDGMAC总线空闲请求，使总线恢复工作。 */
    HWREG32(PMU_MMIO_BASE + PMU_BUS_IDLE_REQ) = GMAC_HIWORD_UPDATE(PMU_SDGMAC_IDLE_REQ_BIT, 0);

    /* 等待idle应答和idle状态均清零，超时表示总线未能退出空闲状态。 */
    timeout = rt_tick_get() + rt_tick_from_millisecond(100);
    while (((HWREG32(PMU_MMIO_BASE + PMU_BUS_IDLE_ACK) & PMU_SDGMAC_IDLE_STATUS_BIT) != 0U) ||
           ((HWREG32(PMU_MMIO_BASE + PMU_BUS_IDLE_ST) & PMU_SDGMAC_IDLE_STATUS_BIT) != 0U)) {
        if ((rt_int32_t)(rt_tick_get() - timeout) >= 0) {
            return -RT_ETIMEOUT;
        }
        rt_thread_mdelay(1);
    }

    /* 关闭控制 */
    HWREG32(PMU_MMIO_BASE + PMU_CLK_UNGATE) = GMAC_HIWORD_UPDATE(PMU_SDGMAC_CLK_MASK, 0);

    return RT_EOK;
}

void rk3576_gmac_clock_reset(void)
{
    /* SRST_A_GMAC0 is reset id 679: SOFTRST_CON42 bit 7. */
    rk3576_cru_reset_assert(RK3576_SOFTRST_CON(42), 7);
    rt_thread_mdelay(1);
    rk3576_cru_reset_deassert(RK3576_SOFTRST_CON(42), 7);
    rt_thread_mdelay(1);
}

/* 更新gmac0的时钟速率 */
void rk3576_gmac_clock_set_speed(rt_uint32_t speed)
{
    rt_uint32_t value;

    if (speed == 1000U) {
        value = 0U;
    } else if (speed == 100U) {
        value = GMAC_BIT(6) | GMAC_BIT(5);
    } else {
        value = GMAC_BIT(6);
    }

    HWREG32(SDGMAC_GRF_MMIO_BASE + SDGMAC_GRF_GMAC0_CON) = GMAC_HIWORD_UPDATE(GMAC_BIT(6) | GMAC_BIT(5), value);
}
