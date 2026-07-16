/*
 * RK3576 GMAC0 driver for the Synopsys DWMAC 4.20a controller.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <board.h>
#include <drv_cru.h>
#include <drv_gmac.h>
#include <drv_iomux.h>

#ifdef BSP_USING_GMAC0

#include <lwip/opt.h>
#include <lwip/pbuf.h>
#include <netdev.h>
#include <netif/ethernetif.h>

#define BIT(n)                              (1U << (n))
#define HIWORD_UPDATE(mask, value)          ((((mask) & 0xffffU) << 16) | ((value) & 0xffffU))

#define GMAC_CONFIG                         0x0000U
#define GMAC_PACKET_FILTER                  0x0008U
#define GMAC_Q0_TX_FLOW_CTRL                0x0070U
#define GMAC_RX_FLOW_CTRL                   0x0090U
#define GMAC_RXQ_CTRL0                      0x00a0U
#define GMAC_HW_FEATURE1                    0x0120U
#define GMAC_MDIO_ADDRESS                   0x0200U     /* GMAC_MAC_MDIO_ADDRESS */
#define GMAC_MDIO_DATA                      0x0204U     /* GMAC_MAC_MDIO_DATA */
#define GMAC_ADDR0_HIGH                     0x0300U
#define GMAC_ADDR0_LOW                      0x0304U

#define GMAC_CONFIG_CST                     BIT(21)
#define GMAC_CONFIG_ACS                     BIT(20)
#define GMAC_CONFIG_PS                      BIT(15)
#define GMAC_CONFIG_FES                     BIT(14)
#define GMAC_CONFIG_DM                      BIT(13)
#define GMAC_CONFIG_TE                      BIT(1)
#define GMAC_CONFIG_RE                      BIT(0)

#define GMAC_PACKET_FILTER_PM               BIT(4)
#define GMAC_RXQ0_ENABLE_DCB                 2U

#define GMAC_MDIO_PA_SHIFT                  21U
#define GMAC_MDIO_RDA_SHIFT                 16U
#define GMAC_MDIO_CR_SHIFT                  8U
#define GMAC_MDIO_GOC_SHIFT                 2U
#define GMAC_MDIO_GOC_WRITE                 1U
#define GMAC_MDIO_GOC_READ                  3U
#define GMAC_MDIO_BUSY                      BIT(0)
#define GMAC_MDIO_CR_100_150_MHZ            1U

#define MTL_TXQ0_OPERATION_MODE             0x0d00U
#define MTL_TXQ0_QUANTUM_WEIGHT             0x0d18U
#define MTL_RXQ0_OPERATION_MODE             0x0d30U
#define MTL_TXQ_TQS_SHIFT                   16U
#define MTL_TXQ_TQS_MASK                    (0x1ffU << MTL_TXQ_TQS_SHIFT)
#define MTL_TXQ_TXQEN_SHIFT                 2U
#define MTL_TXQ_TXQEN_MASK                  (3U << MTL_TXQ_TXQEN_SHIFT)
#define MTL_TXQ_TXQEN_ENABLED               (2U << MTL_TXQ_TXQEN_SHIFT)
#define MTL_TXQ_TSF                         BIT(1)
#define MTL_RXQ_RQS_SHIFT                   20U
#define MTL_RXQ_RQS_MASK                    (0x3ffU << MTL_RXQ_RQS_SHIFT)
#define MTL_RXQ_RSF                         BIT(5)

#define DMA_MODE                            0x1000U
#define DMA_SYSBUS_MODE                     0x1004U
#define DMA_CH0_CONTROL                     0x1100U
#define DMA_CH0_TX_CONTROL                  0x1104U
#define DMA_CH0_RX_CONTROL                  0x1108U
#define DMA_CH0_TXDESC_HI                   0x1110U
#define DMA_CH0_TXDESC_LO                   0x1114U
#define DMA_CH0_RXDESC_HI                   0x1118U
#define DMA_CH0_RXDESC_LO                   0x111cU
#define DMA_CH0_TXDESC_TAIL                 0x1120U
#define DMA_CH0_RXDESC_TAIL                 0x1128U
#define DMA_CH0_TX_RING_LEN                 0x112cU
#define DMA_CH0_RX_RING_LEN                 0x1130U
#define DMA_CH0_INT_ENABLE                  0x1134U
#define DMA_CH0_STATUS                      0x1160U

#define DMA_MODE_SWR                        BIT(0)
#define DMA_SYSBUS_WR_OSR_LMT_SHIFT         24U
#define DMA_SYSBUS_RD_OSR_LMT_SHIFT         16U
#define DMA_SYSBUS_MB                       BIT(14)
#define DMA_SYSBUS_EAME                     BIT(11)
#define DMA_SYSBUS_BLEN16                   BIT(3)
#define DMA_SYSBUS_BLEN8                    BIT(2)
#define DMA_SYSBUS_BLEN4                    BIT(1)
#define DMA_CH_CONTROL_PBLX8                BIT(16)
#define DMA_TX_TXPBL_SHIFT                  16U
#define DMA_TX_OSP                          BIT(4)
#define DMA_TX_START                        BIT(0)
#define DMA_RX_RXPBL_SHIFT                  16U
#define DMA_RX_RBSZ_SHIFT                   1U
#define DMA_RX_START                        BIT(0)

/* DWMAC 4.10 and later channel interrupt layout. */
#define DMA_INT_NIE                         BIT(15)
#define DMA_INT_AIE                         BIT(14)
#define DMA_INT_FBE                         BIT(12)
#define DMA_INT_RBU                         BIT(7)
#define DMA_INT_RIE                         BIT(6)

#define DESC3_OWN                           BIT(31)
#define DESC3_RX_IOC                        BIT(30)
#define DESC3_FD                            BIT(29)
#define DESC3_LD                            BIT(28)
#define DESC3_RX_BUF1_VALID                 BIT(24)
#define DESC3_RX_ERROR                      BIT(15)
#define DESC3_PACKET_LEN_MASK               0x7fffU

#define PHY_ADDR                            0U
#define PHY_BMCR                            0x00U
#define PHY_ID1                             0x02U
#define PHY_ID2                             0x03U
#define PHY_SPEC_STATUS                     0x11U
#define PHY_EXT_ADDR                        0x1eU
#define PHY_EXT_DATA                        0x1fU
#define PHY_BMCR_AN_ENABLE                  BIT(12)
#define PHY_BMCR_AN_RESTART                 BIT(9)
#define PHY_STATUS_SPEED_MASK               (3U << 14)
#define PHY_STATUS_DUPLEX                   BIT(13)
#define PHY_STATUS_LINK                     BIT(10)

#define SDGMAC_GRF_GMAC_CON0                0x0020U
#define IOC_GRF_GMAC0_DELAY_M0              0x6408U
#define IOC_GRF_GMAC0_DELAY_M1              0x640cU
#define GMAC0_TX_DELAY                      0x1bU

#define PMU_PWR_CON                         0x0210U
#define PMU_BUS_IDLE_REQ                    0x0114U
#define PMU_BUS_IDLE_ACK                    0x0120U
#define PMU_BUS_IDLE_ST                     0x0128U
#define PMU_CLK_UNGATE                      0x0144U
#define PMU_REPAIR_STATUS                   0x0570U
#define PMU_SDGMAC_PWR_BIT                  BIT(7)
#define PMU_SDGMAC_IDLE_REQ_BIT             BIT(1)
#define PMU_SDGMAC_IDLE_STATUS_BIT          BIT(17)
#define PMU_SDGMAC_CLK_MASK                 (BIT(1) | BIT(2))

#define GPIO_SWPORT_DR_H                    0x0004U
#define GPIO_SWPORT_DDR_H                   0x000cU
#define GPIO0_C2_HIGH_REG_BIT               BIT(2)

#define GMAC_TX_DESC_COUNT                  4U
#define GMAC_RX_DESC_COUNT                  8U
#define GMAC_DMA_BUFFER_SIZE                1600U
#define GMAC_DESC_SIZE                      16U
#define GMAC_TX_DESC_OFFSET                 0U
#define GMAC_RX_DESC_OFFSET                 64U
#define GMAC_RX_BUFFER_OFFSET               192U
#define GMAC_TX_BUFFER_OFFSET               (GMAC_RX_BUFFER_OFFSET + GMAC_RX_DESC_COUNT * GMAC_DMA_BUFFER_SIZE)

typedef char gmac_dma_layout_must_fit[
    (GMAC_TX_BUFFER_OFFSET + GMAC_TX_DESC_COUNT * GMAC_DMA_BUFFER_SIZE <=
     RK3576_GMAC_DMA_POOL_SIZE) ? 1 : -1];

struct gmac_desc {
    rt_uint32_t des0;
    rt_uint32_t des1;
    rt_uint32_t des2;
    rt_uint32_t des3;
};

struct rk3576_gmac {
    struct eth_device parent;
    rt_ubase_t base;
    rt_uint8_t mac[6];
    volatile struct gmac_desc *tx_desc;
    volatile struct gmac_desc *rx_desc;
    rt_uint8_t *tx_buffer;
    rt_uint8_t *rx_buffer;
    rt_uint32_t tx_index;
    rt_uint32_t rx_index;
    struct rt_mutex tx_lock;
    rt_bool_t initialized;
    rt_bool_t link_up;
    rt_bool_t full_duplex;
    rt_uint32_t speed;
    rt_uint32_t fatal_bus_errors;
};

rt_uint8_t rk3576_gmac_dma_pool[RK3576_GMAC_DMA_POOL_SIZE] __attribute__((aligned(GMAC_DMA_ALIAS_SIZE)));

static struct rk3576_gmac gmac0;

static rt_uint32_t gmac_read(struct rk3576_gmac *gmac, rt_uint32_t offset)
{
    return HWREG32(gmac->base + offset);
}

static void gmac_write(struct rk3576_gmac *gmac, rt_uint32_t offset, rt_uint32_t value)
{
    HWREG32(gmac->base + offset) = value;
}

static void gmac_barrier(void)
{
    __asm__ volatile ("dsb sy" ::: "memory");
}

static rt_uint32_t gmac_dma_phys(rt_uint32_t offset)
{
    return (rt_uint32_t)((rt_ubase_t)rk3576_gmac_dma_pool + offset);
}

static void *gmac_dma_virt(rt_uint32_t offset)
{
    return (void *)(GMAC_DMA_ALIAS_BASE + offset);
}

static void gmac_gpio0_c2_set(rt_bool_t high)
{
    rt_uint32_t value = GPIO0_C2_HIGH_REG_BIT << 16;

    if (high) {
        value |= GPIO0_C2_HIGH_REG_BIT;
    }

    HWREG32(GPIO0_MMIO_BASE + GPIO_SWPORT_DR_H) = value;
}

static void gmac_phy_hardware_reset(void)
{
    gmac_gpio0_c2_set(RT_FALSE); /* 预置GPIO0_C2输出锁存值为低，避免切换为输出时产生高电平毛刺。 */
    HWREG32(GPIO0_MMIO_BASE + GPIO_SWPORT_DDR_H) = (GPIO0_C2_HIGH_REG_BIT << 16) | GPIO0_C2_HIGH_REG_BIT;   /* 设置为输出 */
    rt_thread_mdelay(20);   
    gmac_gpio0_c2_set(RT_TRUE); /* ETH0_RESET_N低有效：输出高电平，释放PHY硬件复位。 */
    rt_thread_mdelay(100);
}

static rt_err_t gmac_power_on(void)
{
    rt_tick_t timeout;

    HWREG32(PMU_MMIO_BASE + PMU_CLK_UNGATE) = HIWORD_UPDATE(PMU_SDGMAC_CLK_MASK, PMU_SDGMAC_CLK_MASK);

    /* 1'b1: power on, 1'b0: power off */
    if ((HWREG32(PMU_MMIO_BASE + PMU_REPAIR_STATUS) & PMU_SDGMAC_PWR_BIT) == 0U) {
        HWREG32(PMU_MMIO_BASE + PMU_PWR_CON) = HIWORD_UPDATE(PMU_SDGMAC_PWR_BIT, 0);

        timeout = rt_tick_get() + rt_tick_from_millisecond(100);
        while ((HWREG32(PMU_MMIO_BASE + PMU_REPAIR_STATUS) & PMU_SDGMAC_PWR_BIT) == 0U) {
            if ((rt_int32_t)(rt_tick_get() - timeout) >= 0) {
                return -RT_ETIMEOUT;
            }
            rt_thread_mdelay(1);
        }
    }

    /* disable sending irq req to biu_gmac by software */
    HWREG32(PMU_MMIO_BASE + PMU_BUS_IDLE_REQ) = HIWORD_UPDATE(PMU_SDGMAC_IDLE_REQ_BIT, 0);

    /* wait for idle_ack statue = 0 and idle_gmac = 0 */
    timeout = rt_tick_get() + rt_tick_from_millisecond(100);
    while (((HWREG32(PMU_MMIO_BASE + PMU_BUS_IDLE_ACK) & PMU_SDGMAC_IDLE_STATUS_BIT) != 0U) ||
           ((HWREG32(PMU_MMIO_BASE + PMU_BUS_IDLE_ST) & PMU_SDGMAC_IDLE_STATUS_BIT) != 0U)) {
        if ((rt_int32_t)(rt_tick_get() - timeout) >= 0) {
            return -RT_ETIMEOUT;
        }
        rt_thread_mdelay(1);
    }

    HWREG32(PMU_MMIO_BASE + PMU_CLK_UNGATE) = HIWORD_UPDATE(PMU_SDGMAC_CLK_MASK, 0);

    return RT_EOK;
}


/**
 * @brief 设置rgmii模式，设置发送接收延时
 * @param  
 */
static void gmac_rgmii_init(void)
{
    rt_uint32_t grf_mask    = BIT(7)  | BIT(6)       | BIT(5) | BIT(3);
    rt_uint32_t delay_mask  = BIT(15) | (0x7fU << 8) | BIT(7) | 0x7fU;
    rt_uint32_t delay_value = BIT(7)  | GMAC0_TX_DELAY;

    /* RGMII mode, clock from CRU, and initial 1000 Mbit/s divider. */
    HWREG32(SDGMAC_GRF_MMIO_BASE + SDGMAC_GRF_GMAC_CON0) = HIWORD_UPDATE(grf_mask, 0);              /* SDGMAC_GRF_GMAC0_CON */

    /* TX delay in the SoC; RX delay is supplied by the PHY (rgmii-rxid). */
    HWREG32(IOC_GRF_MMIO_BASE + IOC_GRF_GMAC0_DELAY_M0) = HIWORD_UPDATE(delay_mask, delay_value);   /* VCCIO_IOC_MISC_CON2 */
    HWREG32(IOC_GRF_MMIO_BASE + IOC_GRF_GMAC0_DELAY_M1) = HIWORD_UPDATE(delay_mask, delay_value);   /* VCCIO_IOC_MISC_CON3 */
}

static void gmac_set_rgmii_speed(rt_uint32_t speed)
{
    rt_uint32_t value;

    if (speed == 1000U) {
        value = 0U;
    } else if (speed == 100U) {
        value = BIT(6) | BIT(5);
    } else {
        value = BIT(6);
    }

    HWREG32(SDGMAC_GRF_MMIO_BASE + SDGMAC_GRF_GMAC_CON0) =
        HIWORD_UPDATE(BIT(6) | BIT(5), value);
}

/* mdio读取总线忙碌标识 */
static rt_err_t gmac_mdio_wait_idle(struct rk3576_gmac *gmac)
{
    rt_uint32_t count;

    for (count = 0; count < 1000000U; count++) {
        if ((gmac_read(gmac, GMAC_MDIO_ADDRESS) & GMAC_MDIO_BUSY) == 0U) {
            return RT_EOK;
        }
        __asm__ volatile ("nop");
    }

    return -RT_ETIMEOUT;
}

/* mdio读取寄存器值 */
static int gmac_mdio_read(struct rk3576_gmac *gmac, rt_uint32_t phy, rt_uint32_t reg)
{
    rt_uint32_t command;

    if (gmac_mdio_wait_idle(gmac) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    command = (phy << GMAC_MDIO_PA_SHIFT) |
              (reg << GMAC_MDIO_RDA_SHIFT) |
              (GMAC_MDIO_CR_100_150_MHZ << GMAC_MDIO_CR_SHIFT) |
              (GMAC_MDIO_GOC_READ << GMAC_MDIO_GOC_SHIFT) |
              GMAC_MDIO_BUSY;
    gmac_write(gmac, GMAC_MDIO_ADDRESS, command);

    if (gmac_mdio_wait_idle(gmac) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    return (int)(gmac_read(gmac, GMAC_MDIO_DATA) & 0xffffU);
}

/* mdio写入寄存器值 */
static rt_err_t gmac_mdio_write(struct rk3576_gmac *gmac, rt_uint32_t phy, rt_uint32_t reg, rt_uint16_t data)
{
    rt_uint32_t command;

    if (gmac_mdio_wait_idle(gmac) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    gmac_write(gmac, GMAC_MDIO_DATA, data);
    command = (phy << GMAC_MDIO_PA_SHIFT) |
              (reg << GMAC_MDIO_RDA_SHIFT) |
              (GMAC_MDIO_CR_100_150_MHZ << GMAC_MDIO_CR_SHIFT) |
              (GMAC_MDIO_GOC_WRITE << GMAC_MDIO_GOC_SHIFT) |
              GMAC_MDIO_BUSY;
    gmac_write(gmac, GMAC_MDIO_ADDRESS, command);

    return gmac_mdio_wait_idle(gmac);
}

static int gmac_phy_ext_read(struct rk3576_gmac *gmac, rt_uint16_t reg)
{
    if (gmac_mdio_write(gmac, PHY_ADDR, PHY_EXT_ADDR, reg) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    return gmac_mdio_read(gmac, PHY_ADDR, PHY_EXT_DATA);
}

static rt_err_t gmac_phy_ext_write(struct rk3576_gmac *gmac, rt_uint16_t reg,
                                   rt_uint16_t value)
{
    rt_err_t ret;

    ret = gmac_mdio_write(gmac, PHY_ADDR, PHY_EXT_ADDR, reg);
    if (ret != RT_EOK) {
        return ret;
    }

    return gmac_mdio_write(gmac, PHY_ADDR, PHY_EXT_DATA, value);
}

/* phy初始化，读id，启动自动协商 */
static rt_err_t gmac_phy_init(struct rk3576_gmac *gmac)
{
    int id1;
    int id2;
    int value;

    id1 = gmac_mdio_read(gmac, PHY_ADDR, PHY_ID1);
    id2 = gmac_mdio_read(gmac, PHY_ADDR, PHY_ID2);
    if ((id1 < 0) || (id2 < 0) || ((id1 == 0xffff) && (id2 == 0xffff))) {
        return -RT_EIO;
    }

    value = gmac_mdio_read(gmac, PHY_ADDR, PHY_BMCR);
    if (value < 0) {
        return -RT_EIO;
    }
    if (gmac_mdio_write(gmac, PHY_ADDR, PHY_BMCR,
                        (rt_uint16_t)value | PHY_BMCR_AN_ENABLE | PHY_BMCR_AN_RESTART) != RT_EOK) {
        return -RT_EIO;
    }

    rt_kprintf("gmac0: PHY id %04x:%04x, autonegotiation started\n", id1, id2);
    return RT_EOK;
}

static void gmac_set_mac_address(struct rk3576_gmac *gmac)
{
    rt_uint32_t high;
    rt_uint32_t low;

    high = BIT(31) | ((rt_uint32_t)gmac->mac[5] << 8) | gmac->mac[4];
    low = ((rt_uint32_t)gmac->mac[3] << 24) |
          ((rt_uint32_t)gmac->mac[2] << 16) |
          ((rt_uint32_t)gmac->mac[1] << 8) |
          gmac->mac[0];
    gmac_write(gmac, GMAC_ADDR0_HIGH, high);
    gmac_write(gmac, GMAC_ADDR0_LOW, low);
}

static rt_uint32_t gmac_fifo_queue_size(rt_uint32_t encoded, rt_uint32_t mask)
{
    rt_uint32_t value;

    if (encoded == 0U) {
        return 0U;
    }
    if (encoded > 15U) {
        encoded = 15U;
    }

    value = (1U << (encoded - 1U)) - 1U;
    return value > mask ? mask : value;
}

static void gmac_init_dma_memory(struct rk3576_gmac *gmac)
{
    rt_uint32_t i;

    gmac->tx_desc = (volatile struct gmac_desc *)gmac_dma_virt(GMAC_TX_DESC_OFFSET);
    gmac->rx_desc = (volatile struct gmac_desc *)gmac_dma_virt(GMAC_RX_DESC_OFFSET);
    gmac->rx_buffer = (rt_uint8_t *)gmac_dma_virt(GMAC_RX_BUFFER_OFFSET);
    gmac->tx_buffer = (rt_uint8_t *)gmac_dma_virt(GMAC_TX_BUFFER_OFFSET);
    gmac->tx_index = 0;
    gmac->rx_index = 0;

    rt_memset((void *)gmac->tx_desc, 0, GMAC_TX_DESC_COUNT * sizeof(struct gmac_desc));
    rt_memset((void *)gmac->rx_desc, 0, GMAC_RX_DESC_COUNT * sizeof(struct gmac_desc));

    for (i = 0; i < GMAC_RX_DESC_COUNT; i++) {
        gmac->rx_desc[i].des0 = gmac_dma_phys(GMAC_RX_BUFFER_OFFSET + i * GMAC_DMA_BUFFER_SIZE);
        gmac->rx_desc[i].des1 = 0;
        gmac->rx_desc[i].des2 = 0;
        gmac_barrier();
        gmac->rx_desc[i].des3 = DESC3_OWN | DESC3_RX_IOC | DESC3_RX_BUF1_VALID;
    }
    gmac_barrier();
}

/**
 * @brief 软件复位gmac的内部dma
 * @param gmac 
 * @return 
 */
static rt_err_t gmac_dma_software_reset(struct rk3576_gmac *gmac)
{
    rt_uint32_t count;

    gmac_write(gmac, DMA_MODE, gmac_read(gmac, DMA_MODE) | DMA_MODE_SWR);   /* GMAC_DMA_MODE */
    for (count = 0; count < 100U; count++) {
        if ((gmac_read(gmac, DMA_MODE) & DMA_MODE_SWR) == 0U) {
            return RT_EOK;
        }
        rt_thread_mdelay(1);
    }

    return -RT_ETIMEOUT;
}

static void gmac_dma_start(struct rk3576_gmac *gmac)
{
    rt_uint32_t feature;
    rt_uint32_t value;
    rt_uint32_t tx_queue_size;
    rt_uint32_t rx_queue_size;

    feature = gmac_read(gmac, GMAC_HW_FEATURE1);
    tx_queue_size = gmac_fifo_queue_size((feature >> 6) & 0x1fU, 0x1ffU);
    rx_queue_size = gmac_fifo_queue_size(feature & 0x1fU, 0x3ffU);

    value = gmac_read(gmac, MTL_TXQ0_OPERATION_MODE);
    value &= ~(MTL_TXQ_TQS_MASK | MTL_TXQ_TXQEN_MASK);
    value |= (tx_queue_size << MTL_TXQ_TQS_SHIFT) |
             MTL_TXQ_TXQEN_ENABLED | MTL_TXQ_TSF;
    gmac_write(gmac, MTL_TXQ0_OPERATION_MODE, value);
    gmac_write(gmac, MTL_TXQ0_QUANTUM_WEIGHT, 0x10U);

    value = gmac_read(gmac, MTL_RXQ0_OPERATION_MODE);
    value &= ~MTL_RXQ_RQS_MASK;
    value |= (rx_queue_size << MTL_RXQ_RQS_SHIFT) | MTL_RXQ_RSF;
    gmac_write(gmac, MTL_RXQ0_OPERATION_MODE, value);

    value = gmac_read(gmac, GMAC_RXQ_CTRL0);
    value = (value & ~3U) | GMAC_RXQ0_ENABLE_DCB;
    gmac_write(gmac, GMAC_RXQ_CTRL0, value);
    gmac_write(gmac, GMAC_PACKET_FILTER, GMAC_PACKET_FILTER_PM);
    gmac_write(gmac, GMAC_Q0_TX_FLOW_CTRL, (0xffffU << 16) | BIT(1));
    gmac_write(gmac, GMAC_RX_FLOW_CTRL, BIT(0));
    gmac_set_mac_address(gmac);

    gmac_init_dma_memory(gmac);
    gmac_write(gmac, DMA_CH0_TXDESC_HI, 0);
    gmac_write(gmac, DMA_CH0_TXDESC_LO, gmac_dma_phys(GMAC_TX_DESC_OFFSET));
    gmac_write(gmac, DMA_CH0_RXDESC_HI, 0);
    gmac_write(gmac, DMA_CH0_RXDESC_LO, gmac_dma_phys(GMAC_RX_DESC_OFFSET));
    gmac_write(gmac, DMA_CH0_TX_RING_LEN, GMAC_TX_DESC_COUNT - 1U);
    gmac_write(gmac, DMA_CH0_RX_RING_LEN, GMAC_RX_DESC_COUNT - 1U);

    gmac_write(gmac, DMA_SYSBUS_MODE,
               (4U << DMA_SYSBUS_WR_OSR_LMT_SHIFT) |
               (8U << DMA_SYSBUS_RD_OSR_LMT_SHIFT) |
               DMA_SYSBUS_MB | DMA_SYSBUS_EAME |
               DMA_SYSBUS_BLEN16 | DMA_SYSBUS_BLEN8 | DMA_SYSBUS_BLEN4);
    gmac_write(gmac, DMA_CH0_CONTROL, DMA_CH_CONTROL_PBLX8);
    gmac_write(gmac, DMA_CH0_TX_CONTROL, (8U << DMA_TX_TXPBL_SHIFT) | DMA_TX_OSP);
    gmac_write(gmac, DMA_CH0_RX_CONTROL,
               (8U << DMA_RX_RXPBL_SHIFT) |
               (GMAC_DMA_BUFFER_SIZE << DMA_RX_RBSZ_SHIFT));

    gmac_write(gmac, DMA_CH0_STATUS, 0xffffffffU);
    gmac_write(gmac, DMA_CH0_INT_ENABLE,
               DMA_INT_NIE | DMA_INT_AIE | DMA_INT_FBE | DMA_INT_RBU | DMA_INT_RIE);

    gmac_write(gmac, DMA_CH0_TX_CONTROL,
               gmac_read(gmac, DMA_CH0_TX_CONTROL) | DMA_TX_START);
    gmac_write(gmac, DMA_CH0_RX_CONTROL,
               gmac_read(gmac, DMA_CH0_RX_CONTROL) | DMA_RX_START);

    value = gmac_read(gmac, GMAC_CONFIG);
    value &= ~(GMAC_CONFIG_PS | GMAC_CONFIG_FES);
    value |= GMAC_CONFIG_CST | GMAC_CONFIG_ACS | GMAC_CONFIG_DM |
             GMAC_CONFIG_TE | GMAC_CONFIG_RE;
    gmac_write(gmac, GMAC_CONFIG, value);

    gmac_barrier();
    gmac_write(gmac, DMA_CH0_RXDESC_TAIL,
               gmac_dma_phys(GMAC_RX_DESC_OFFSET + GMAC_RX_DESC_COUNT * GMAC_DESC_SIZE));
}

static void gmac_adjust_link(struct rk3576_gmac *gmac, rt_uint32_t speed,
                             rt_bool_t full_duplex)
{
    rt_uint32_t value;

    gmac_set_rgmii_speed(speed);

    value = gmac_read(gmac, GMAC_CONFIG);
    value &= ~(GMAC_CONFIG_PS | GMAC_CONFIG_FES | GMAC_CONFIG_DM);
    if (speed != 1000U) {
        value |= GMAC_CONFIG_PS;
        if (speed == 100U) {
            value |= GMAC_CONFIG_FES;
        }
    }
    if (full_duplex) {
        value |= GMAC_CONFIG_DM;
    }
    gmac_write(gmac, GMAC_CONFIG, value);
}

static rt_err_t gmac_hw_init(struct rk3576_gmac *gmac)
{
    rt_err_t ret;

    if (gmac->initialized) {
        return RT_EOK;
    }

    ret = rk3576_cru_gmac0_enable();
    if (ret != RT_EOK) {
        return ret;
    }
    ret = gmac_power_on();
    if (ret != RT_EOK) {
        rt_kprintf("gmac0: failed to power on SDGMAC domain: %d\n", ret);
        return ret;
    }
    ret = rk3576_gmac0_iomux_init();
    if (ret != RT_EOK) {
        return ret;
    }

    gmac_rgmii_init();
    rk3576_cru_gmac0_reset();
    gmac_phy_hardware_reset();

    ret = gmac_dma_software_reset(gmac);
    if (ret != RT_EOK) {
        rt_kprintf("gmac0: DMA reset timed out\n");
        return ret;
    }
    ret = gmac_phy_init(gmac);
    if (ret != RT_EOK) {
        rt_kprintf("gmac0: PHY not found at MDIO address %u\n", PHY_ADDR);
        return ret;
    }

    gmac->initialized = RT_TRUE;
    return RT_EOK;
}

static rt_err_t gmac_device_init(rt_device_t device);

static void gmac_isr(int vector, void *parameter)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)parameter;
    rt_uint32_t status;

    (void)vector;
    status = gmac_read(gmac, DMA_CH0_STATUS);
    gmac_write(gmac, DMA_CH0_STATUS, status);

    if ((status & DMA_INT_FBE) != 0U) {
        gmac->fatal_bus_errors++;
    }
    if ((status & (DMA_INT_RIE | DMA_INT_RBU)) != 0U) {
        eth_device_ready(&gmac->parent);
    }
}

static rt_err_t gmac_device_init(rt_device_t device)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)device;
    rt_err_t ret;

    ret = gmac_hw_init(gmac);
    if (ret != RT_EOK) {
        return ret;
    }

    rt_hw_interrupt_install(GMAC0_SBD_IRQ, gmac_isr, gmac, "gmac0");
    rt_hw_interrupt_umask(GMAC0_SBD_IRQ);
    gmac_dma_start(gmac);

    rt_kprintf("gmac0: registered as e0, MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
               gmac->mac[0], gmac->mac[1], gmac->mac[2],
               gmac->mac[3], gmac->mac[4], gmac->mac[5]);
    return RT_EOK;
}

static rt_err_t gmac_device_open(rt_device_t device, rt_uint16_t oflag)
{
    (void)device;
    (void)oflag;
    return RT_EOK;
}

static rt_err_t gmac_device_close(rt_device_t device)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)device;

    if (gmac->initialized) {
        gmac_write(gmac, DMA_CH0_INT_ENABLE, 0);
        rt_hw_interrupt_mask(GMAC0_SBD_IRQ);
        gmac_write(gmac, DMA_CH0_TX_CONTROL,
                   gmac_read(gmac, DMA_CH0_TX_CONTROL) & ~DMA_TX_START);
        gmac_write(gmac, DMA_CH0_RX_CONTROL,
                   gmac_read(gmac, DMA_CH0_RX_CONTROL) & ~DMA_RX_START);
    }
    return RT_EOK;
}

static rt_size_t gmac_device_read(rt_device_t device, rt_off_t pos,
                                  void *buffer, rt_size_t size)
{
    (void)device;
    (void)pos;
    (void)buffer;
    (void)size;
    return 0;
}

static rt_size_t gmac_device_write(rt_device_t device, rt_off_t pos,
                                   const void *buffer, rt_size_t size)
{
    (void)device;
    (void)pos;
    (void)buffer;
    (void)size;
    return 0;
}

static rt_err_t gmac_device_control(rt_device_t device, int cmd, void *args)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)device;

    if (((cmd == NIOCTL_GADDR) || (cmd == RT_DEVICE_CTRL_NETIF_GETMAC)) &&
        (args != RT_NULL)) {
        rt_memcpy(args, gmac->mac, sizeof(gmac->mac));
        return RT_EOK;
    }

    return -RT_ENOSYS;
}

static const struct rt_device_ops gmac_device_ops = {
    gmac_device_init,
    gmac_device_open,
    gmac_device_close,
    gmac_device_read,
    gmac_device_write,
    gmac_device_control,
};

static rt_err_t gmac_eth_tx(rt_device_t device, struct pbuf *p)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)device;
    volatile struct gmac_desc *desc;
    struct pbuf *q;
    rt_uint8_t *buffer;
    rt_uint32_t offset = 0;
    rt_uint32_t next;
    rt_tick_t timeout;

    if ((p == RT_NULL) || (p->tot_len > GMAC_DMA_BUFFER_SIZE) || !gmac->link_up) {
        return -RT_ERROR;
    }

    if (rt_mutex_take(&gmac->tx_lock, rt_tick_from_millisecond(1000)) != RT_EOK) {
        return -RT_ETIMEOUT;
    }

    desc = &gmac->tx_desc[gmac->tx_index];
    timeout = rt_tick_get() + rt_tick_from_millisecond(1000);
    while ((desc->des3 & DESC3_OWN) != 0U) {
        if ((rt_int32_t)(rt_tick_get() - timeout) >= 0) {
            rt_mutex_release(&gmac->tx_lock);
            return -RT_ETIMEOUT;
        }
        rt_thread_yield();
    }

    buffer = gmac->tx_buffer + gmac->tx_index * GMAC_DMA_BUFFER_SIZE;
    for (q = p; q != RT_NULL; q = q->next) {
        rt_memcpy(buffer + offset, q->payload, q->len);
        offset += q->len;
    }

    desc->des0 = gmac_dma_phys(GMAC_TX_BUFFER_OFFSET +
                               gmac->tx_index * GMAC_DMA_BUFFER_SIZE);
    desc->des1 = 0;
    desc->des2 = offset;
    gmac_barrier();
    desc->des3 = DESC3_OWN | DESC3_FD | DESC3_LD | offset;
    gmac_barrier();

    next = (gmac->tx_index + 1U) % GMAC_TX_DESC_COUNT;
    gmac_write(gmac, DMA_CH0_TXDESC_TAIL,
               gmac_dma_phys(GMAC_TX_DESC_OFFSET + next * GMAC_DESC_SIZE));

    timeout = rt_tick_get() + rt_tick_from_millisecond(1000);
    while ((desc->des3 & DESC3_OWN) != 0U) {
        if ((rt_int32_t)(rt_tick_get() - timeout) >= 0) {
            rt_mutex_release(&gmac->tx_lock);
            return -RT_ETIMEOUT;
        }
        rt_thread_yield();
    }

    gmac->tx_index = next;
    rt_mutex_release(&gmac->tx_lock);
    return RT_EOK;
}

static struct pbuf *gmac_eth_rx(rt_device_t device)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)device;
    volatile struct gmac_desc *desc;
    struct pbuf *p = RT_NULL;
    struct pbuf *q;
    rt_uint8_t *buffer;
    rt_uint32_t status;
    rt_uint32_t length;
    rt_uint32_t copied;

    desc = &gmac->rx_desc[gmac->rx_index];
    gmac_barrier();
    status = desc->des3;
    if ((status & DESC3_OWN) != 0U) {
        return RT_NULL;
    }

    length = status & DESC3_PACKET_LEN_MASK;
    buffer = gmac->rx_buffer + gmac->rx_index * GMAC_DMA_BUFFER_SIZE;
    if (((status & (DESC3_FD | DESC3_LD)) == (DESC3_FD | DESC3_LD)) &&
        ((status & DESC3_RX_ERROR) == 0U) &&
        (length > 0U) && (length <= GMAC_DMA_BUFFER_SIZE)) {
        p = pbuf_alloc(PBUF_RAW, (u16_t)length, PBUF_POOL);
        if (p != RT_NULL) {
            copied = 0;
            for (q = p; q != RT_NULL; q = q->next) {
                rt_memcpy(q->payload, buffer + copied, q->len);
                copied += q->len;
            }
        }
    }

    desc->des0 = gmac_dma_phys(GMAC_RX_BUFFER_OFFSET +
                               gmac->rx_index * GMAC_DMA_BUFFER_SIZE);
    desc->des1 = 0;
    desc->des2 = 0;
    gmac_barrier();
    desc->des3 = DESC3_OWN | DESC3_RX_IOC | DESC3_RX_BUF1_VALID;
    gmac_barrier();

    gmac->rx_index = (gmac->rx_index + 1U) % GMAC_RX_DESC_COUNT;
    gmac_write(gmac, DMA_CH0_RXDESC_TAIL,
               gmac_dma_phys(GMAC_RX_DESC_OFFSET +
                             gmac->rx_index * GMAC_DESC_SIZE));
    return p;
}

static int gmac_hex_value(char c)
{
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }
    if ((c >= 'a') && (c <= 'f')) {
        return c - 'a' + 10;
    }
    if ((c >= 'A') && (c <= 'F')) {
        return c - 'A' + 10;
    }
    return -1;
}

static rt_err_t gmac_parse_mac(const char *text, rt_uint8_t mac[6])
{
    rt_size_t i;
    int high;
    int low;

    for (i = 0; i < 6U; i++) {
        high = gmac_hex_value(text[0]);
        low = gmac_hex_value(text[1]);
        if ((high < 0) || (low < 0)) {
            return -RT_EINVAL;
        }
        mac[i] = (rt_uint8_t)((high << 4) | low);
        text += 2;
        if (i != 5U) {
            if ((*text != ':') && (*text != '-')) {
                return -RT_EINVAL;
            }
            text++;
        }
    }

    return RT_EOK;
}

static rt_err_t gmac_phy_get_link(struct rk3576_gmac *gmac, rt_bool_t *up,
                                  rt_uint32_t *speed, rt_bool_t *full_duplex)
{
    int value;
    rt_uint32_t speed_mode;

    value = gmac_mdio_read(gmac, PHY_ADDR, PHY_SPEC_STATUS);
    if (value < 0) {
        return -RT_EIO;
    }

    *up = ((rt_uint32_t)value & PHY_STATUS_LINK) != 0U;
    *full_duplex = ((rt_uint32_t)value & PHY_STATUS_DUPLEX) != 0U;
    speed_mode = ((rt_uint32_t)value & PHY_STATUS_SPEED_MASK) >> 14;
    if (speed_mode == 2U) {
        *speed = 1000U;
    } else if (speed_mode == 1U) {
        *speed = 100U;
    } else {
        *speed = 10U;
    }

    return RT_EOK;
}

static void gmac_phy_thread(void *parameter)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)parameter;
    rt_bool_t up;
    rt_bool_t full_duplex;
    rt_uint32_t speed;

    while (1) {
        if (gmac->initialized &&
            (gmac_phy_get_link(gmac, &up, &speed, &full_duplex) == RT_EOK)) {
            if (up && ((!gmac->link_up) || (speed != gmac->speed) ||
                       (full_duplex != gmac->full_duplex))) {
                gmac_adjust_link(gmac, speed, full_duplex);
                gmac->speed = speed;
                gmac->full_duplex = full_duplex;
                rt_kprintf("gmac0: link up, %u Mbps, %s duplex\n",
                           speed, full_duplex ? "full" : "half");
            }

            if (up != gmac->link_up) {
                gmac->link_up = up;
                eth_device_linkchange(&gmac->parent, up);
                if (!up) {
                    rt_kprintf("gmac0: link down\n");
                }
            }
        }

        rt_thread_mdelay(1000);
    }
}



/* todo */
static int rk3576_gmac0_register(void)
{
    rt_thread_t thread;
    rt_uint16_t flags;
    rt_err_t ret;

    rt_memset(&gmac0, 0, sizeof(gmac0));

    gmac0.base = GMAC0_MMIO_BASE;
    ret = gmac_parse_mac(BSP_GMAC0_MAC_ADDRESS, gmac0.mac);
    if (ret != RT_EOK) {
        rt_kprintf("gmac0: invalid BSP_GMAC0_MAC_ADDRESS\n");
        return ret;
    }

    rt_mutex_init(&gmac0.tx_lock, "e0tx", RT_IPC_FLAG_PRIO);
    gmac0.parent.parent.ops         = &gmac_device_ops;
    gmac0.parent.parent.user_data   = &gmac0;
    gmac0.parent.eth_tx             = gmac_eth_tx;
    gmac0.parent.eth_rx             = gmac_eth_rx;

    flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
#if LWIP_IGMP
    flags |= NETIF_FLAG_IGMP;
#endif
    ret = eth_device_init_with_flag(&gmac0.parent, "e0", flags);
    if (ret != RT_EOK) {
        return ret;
    }

    thread = rt_thread_create("e0phy", gmac_phy_thread, &gmac0, 2048, 18, 20);
    if (thread == RT_NULL) {
        return -RT_ENOMEM;
    }
    rt_thread_startup(thread);

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rk3576_gmac0_register);

static int gmac0_info(int argc, char **argv)
{
    int phy_status = -1;

    (void)argc;
    (void)argv;
    if (gmac0.initialized) {
        phy_status = gmac_mdio_read(&gmac0, PHY_ADDR, PHY_SPEC_STATUS);
    }

    rt_kprintf("gmac0: init=%u link=%u speed=%u duplex=%s phy11=0x%04x\n",
               gmac0.initialized, gmac0.link_up, gmac0.speed,
               gmac0.full_duplex ? "full" : "half",
               phy_status < 0 ? 0xffffU : (rt_uint32_t)phy_status);
    rt_kprintf("gmac0: dma_status=0x%08x fatal_bus_errors=%u tx=%u rx=%u\n",
               gmac0.initialized ? gmac_read(&gmac0, DMA_CH0_STATUS) : 0U,
               gmac0.fatal_bus_errors, gmac0.tx_index, gmac0.rx_index);
    return 0;
}
MSH_CMD_EXPORT(gmac0_info, show GMAC0 link PHY and DMA status);

static int gmac0_dhcp(int argc, char **argv)
{
    struct netdev *netdev;
    rt_bool_t enable;
    rt_bool_t renew = RT_FALSE;
    int ret;

    if (argc != 3) {
        rt_kprintf("usage: dhcp <interface> on|off|renew\n");
        return -RT_EINVAL;
    }

    netdev = netdev_get_by_name(argv[1]);
    if (netdev == RT_NULL) {
        rt_kprintf("dhcp: interface %s not found\n", argv[1]);
        return -RT_ENOSYS;
    }

    if (rt_strcmp(argv[2], "on") == 0) {
        enable = RT_TRUE;
    } else if (rt_strcmp(argv[2], "off") == 0) {
        enable = RT_FALSE;
    } else if (rt_strcmp(argv[2], "renew") == 0) {
        enable = RT_TRUE;
        renew = RT_TRUE;
    } else {
        rt_kprintf("usage: dhcp <interface> on|off|renew\n");
        return -RT_EINVAL;
    }

    if (renew) {
        ret = netdev_dhcp_enabled(netdev, RT_FALSE);
        if (ret != RT_EOK) {
            return ret;
        }
    }

    ret = netdev_dhcp_enabled(netdev, enable);
    if (ret == RT_EOK) {
        rt_kprintf("dhcp: %s %s\n", argv[1], enable ? "enabled" : "disabled");
    }
    return ret;
}
MSH_CMD_EXPORT_ALIAS(gmac0_dhcp, dhcp, enable disable or renew DHCP on an interface);

#endif /* BSP_USING_GMAC0 */
