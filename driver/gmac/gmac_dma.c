/*
 * RK3576 GMAC0 MTL and DMA data path.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gmac_internal.h"

#define GMAC_CONFIG                 0x0000U
#define GMAC_PACKET_FILTER          0x0008U
#define GMAC_Q0_TX_FLOW_CTRL        0x0070U
#define GMAC_RX_FLOW_CTRL           0x0090U
#define GMAC_RXQ_CTRL0              0x00a0U
#define GMAC_HW_FEATURE1            0x0120U
#define GMAC_ADDR0_HIGH             0x0300U
#define GMAC_ADDR0_LOW              0x0304U

#define GMAC_CONFIG_CST             GMAC_BIT(21)
#define GMAC_CONFIG_ACS             GMAC_BIT(20)
#define GMAC_CONFIG_PS              GMAC_BIT(15)
#define GMAC_CONFIG_FES             GMAC_BIT(14)
#define GMAC_CONFIG_DM              GMAC_BIT(13)
#define GMAC_CONFIG_TE              GMAC_BIT(1)
#define GMAC_CONFIG_RE              GMAC_BIT(0)

#define GMAC_PACKET_FILTER_PM       GMAC_BIT(4)
#define GMAC_RXQ0_ENABLE_DCB        2U

#define MTL_TXQ0_OPERATION_MODE     0x0d00U
#define MTL_TXQ0_QUANTUM_WEIGHT     0x0d18U
#define MTL_RXQ0_OPERATION_MODE     0x0d30U
#define MTL_TXQ_TQS_SHIFT           16U
#define MTL_TXQ_TQS_MASK            (0x1ffU << MTL_TXQ_TQS_SHIFT)
#define MTL_TXQ_TXQEN_SHIFT         2U
#define MTL_TXQ_TXQEN_MASK          (3U << MTL_TXQ_TXQEN_SHIFT)
#define MTL_TXQ_TXQEN_ENABLED       (2U << MTL_TXQ_TXQEN_SHIFT)
#define MTL_TXQ_TSF                 GMAC_BIT(1)
#define MTL_RXQ_RQS_SHIFT           20U
#define MTL_RXQ_RQS_MASK            (0x3ffU << MTL_RXQ_RQS_SHIFT)
#define MTL_RXQ_RSF                 GMAC_BIT(5)

#define DMA_MODE                    0x1000U
#define DMA_SYSBUS_MODE             0x1004U
#define DMA_CH0_CONTROL             0x1100U
#define DMA_CH0_TX_CONTROL          0x1104U
#define DMA_CH0_RX_CONTROL          0x1108U
#define DMA_CH0_TXDESC_HI           0x1110U
#define DMA_CH0_TXDESC_LO           0x1114U
#define DMA_CH0_RXDESC_HI           0x1118U
#define DMA_CH0_RXDESC_LO           0x111cU
#define DMA_CH0_TXDESC_TAIL         0x1120U
#define DMA_CH0_RXDESC_TAIL         0x1128U
#define DMA_CH0_TX_RING_LEN         0x112cU
#define DMA_CH0_RX_RING_LEN         0x1130U
#define DMA_CH0_INT_ENABLE          0x1134U
#define DMA_CH0_STATUS              0x1160U

#define DMA_MODE_SWR                GMAC_BIT(0)
#define DMA_SYSBUS_WR_OSR_LMT_SHIFT 24U
#define DMA_SYSBUS_RD_OSR_LMT_SHIFT 16U
#define DMA_SYSBUS_MB               GMAC_BIT(14)
#define DMA_SYSBUS_EAME             GMAC_BIT(11)
#define DMA_SYSBUS_BLEN16           GMAC_BIT(3)
#define DMA_SYSBUS_BLEN8            GMAC_BIT(2)
#define DMA_SYSBUS_BLEN4            GMAC_BIT(1)
#define DMA_CH_CONTROL_PBLX8        GMAC_BIT(16)
#define DMA_TX_TXPBL_SHIFT          16U
#define DMA_TX_OSP                  GMAC_BIT(4)
#define DMA_TX_START                GMAC_BIT(0)
#define DMA_RX_RXPBL_SHIFT          16U
#define DMA_RX_RBSZ_SHIFT           1U
#define DMA_RX_START                GMAC_BIT(0)

/* DWMAC 4.10 and later channel interrupt layout. */
#define DMA_INT_NIE           GMAC_BIT(15)
#define DMA_INT_AIE           GMAC_BIT(14)
#define DMA_INT_FBE           GMAC_DMA_STATUS_FATAL_BUS_ERROR
#define DMA_INT_RBU           GMAC_DMA_STATUS_RX_BUFFER_UNAVAIL
#define DMA_INT_RIE           GMAC_DMA_STATUS_RX_INTERRUPT

#define DESC3_OWN             GMAC_BIT(31)
#define DESC3_RX_IOC          GMAC_BIT(30)
#define DESC3_FD              GMAC_BIT(29)
#define DESC3_LD              GMAC_BIT(28)
#define DESC3_RX_BUF1_VALID   GMAC_BIT(24)
#define DESC3_RX_ERROR        GMAC_BIT(15)
#define DESC3_PACKET_LEN_MASK 0x7fffU

#define GMAC_TX_DESC_COUNT    4U
#define GMAC_RX_DESC_COUNT    8U
#define GMAC_DMA_BUFFER_SIZE  1600U
#define GMAC_DESC_SIZE        16U

#define GMAC_TX_DESC_OFFSET   0U                                                                    /* 发送描述符区：4 * 16 = 64字节 */
#define GMAC_RX_DESC_OFFSET   (GMAC_TX_DESC_OFFSET + GMAC_TX_DESC_COUNT * GMAC_DESC_SIZE)           /* 接收描述符区：起始偏移64，共8 * 16 = 128字节 */
#define GMAC_RX_BUFFER_OFFSET (GMAC_RX_DESC_OFFSET + GMAC_RX_DESC_COUNT * GMAC_DESC_SIZE)           /* 接收缓冲区：起始偏移192，共8 * 1600 = 12800字节 */
#define GMAC_TX_BUFFER_OFFSET (GMAC_RX_BUFFER_OFFSET + GMAC_RX_DESC_COUNT * GMAC_DMA_BUFFER_SIZE)   /* 发送缓冲区：起始偏移12992，共4 * 1600 = 6400字节 */
#define GMAC_DMA_USED_SIZE    (GMAC_TX_BUFFER_OFFSET + GMAC_TX_DESC_COUNT * GMAC_DMA_BUFFER_SIZE)   /* DMA内存实际使用总大小：19392字节 */

typedef char gmac_dma_layout_must_fit[(GMAC_DMA_USED_SIZE <= RK3576_GMAC_DMA_POOL_SIZE) ? 1 : -1];

struct gmac_desc {
    rt_uint32_t des0;
    rt_uint32_t des1;
    rt_uint32_t des2;
    rt_uint32_t des3;
};

rt_uint8_t rk3576_gmac_dma_pool[RK3576_GMAC_DMA_POOL_SIZE] __attribute__((aligned(GMAC_DMA_ALIAS_SIZE)));

static rt_uint32_t gmac_dma_phys(rt_uint32_t offset)
{
    return (rt_uint32_t)((rt_ubase_t)rk3576_gmac_dma_pool + offset);
}

static void *gmac_dma_virt(rt_uint32_t offset)
{
    return (void *)(GMAC_DMA_ALIAS_BASE + offset);
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

static void gmac_set_mac_address(struct rk3576_gmac *gmac)
{
    rt_uint32_t high;
    rt_uint32_t low;

    high = GMAC_BIT(31) | ((rt_uint32_t)gmac->mac[5] << 8) | gmac->mac[4];
    low = ((rt_uint32_t)gmac->mac[3] << 24) | ((rt_uint32_t)gmac->mac[2] << 16) | ((rt_uint32_t)gmac->mac[1] << 8) |
          gmac->mac[0];
    gmac_write(gmac, GMAC_ADDR0_HIGH, high);
    gmac_write(gmac, GMAC_ADDR0_LOW, low);
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
        gmac->rx_desc[i].des3 = DESC3_OWN | DESC3_RX_IOC | DESC3_RX_BUF1_VALID;
    }
    gmac_barrier();
}




rt_err_t rk3576_gmac_dma_reset(struct rk3576_gmac *gmac)
{
    rt_uint32_t count;

    gmac_write(gmac, DMA_MODE, gmac_read(gmac, DMA_MODE) | DMA_MODE_SWR);
    for (count = 0; count < 100U; count++) {
        if ((gmac_read(gmac, DMA_MODE) & DMA_MODE_SWR) == 0U) {
            return RT_EOK;
        }
        rt_thread_mdelay(1);
    }

    return -RT_ETIMEOUT;
}

void rk3576_gmac_dma_start(struct rk3576_gmac *gmac)
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
    value |= (tx_queue_size << MTL_TXQ_TQS_SHIFT) | MTL_TXQ_TXQEN_ENABLED | MTL_TXQ_TSF;
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
    gmac_write(gmac, GMAC_Q0_TX_FLOW_CTRL, (0xffffU << 16) | GMAC_BIT(1));
    gmac_write(gmac, GMAC_RX_FLOW_CTRL, GMAC_BIT(0));
    gmac_set_mac_address(gmac);

    gmac_init_dma_memory(gmac);
    gmac_write(gmac, DMA_CH0_TXDESC_HI, 0);
    gmac_write(gmac, DMA_CH0_TXDESC_LO, gmac_dma_phys(GMAC_TX_DESC_OFFSET));
    gmac_write(gmac, DMA_CH0_RXDESC_HI, 0);
    gmac_write(gmac, DMA_CH0_RXDESC_LO, gmac_dma_phys(GMAC_RX_DESC_OFFSET));
    gmac_write(gmac, DMA_CH0_TX_RING_LEN, GMAC_TX_DESC_COUNT - 1U);
    gmac_write(gmac, DMA_CH0_RX_RING_LEN, GMAC_RX_DESC_COUNT - 1U);

    gmac_write(gmac,
               DMA_SYSBUS_MODE,
               (4U << DMA_SYSBUS_WR_OSR_LMT_SHIFT) | (8U << DMA_SYSBUS_RD_OSR_LMT_SHIFT) | DMA_SYSBUS_MB |
                   DMA_SYSBUS_EAME | DMA_SYSBUS_BLEN16 | DMA_SYSBUS_BLEN8 | DMA_SYSBUS_BLEN4);
    gmac_write(gmac, DMA_CH0_CONTROL, DMA_CH_CONTROL_PBLX8);
    gmac_write(gmac, DMA_CH0_TX_CONTROL, (8U << DMA_TX_TXPBL_SHIFT) | DMA_TX_OSP);
    gmac_write(gmac, DMA_CH0_RX_CONTROL, (8U << DMA_RX_RXPBL_SHIFT) | (GMAC_DMA_BUFFER_SIZE << DMA_RX_RBSZ_SHIFT));

    gmac_write(gmac, DMA_CH0_STATUS, 0xffffffffU);
    gmac_write(gmac, DMA_CH0_INT_ENABLE, DMA_INT_NIE | DMA_INT_AIE | DMA_INT_FBE | DMA_INT_RBU | DMA_INT_RIE);

    gmac_write(gmac, DMA_CH0_TX_CONTROL, gmac_read(gmac, DMA_CH0_TX_CONTROL) | DMA_TX_START);
    gmac_write(gmac, DMA_CH0_RX_CONTROL, gmac_read(gmac, DMA_CH0_RX_CONTROL) | DMA_RX_START);

    value = gmac_read(gmac, GMAC_CONFIG);
    value &= ~(GMAC_CONFIG_PS | GMAC_CONFIG_FES);
    value |= GMAC_CONFIG_CST | GMAC_CONFIG_ACS | GMAC_CONFIG_DM | GMAC_CONFIG_TE | GMAC_CONFIG_RE;
    gmac_write(gmac, GMAC_CONFIG, value);

    gmac_barrier();
    gmac_write(gmac, DMA_CH0_RXDESC_TAIL, gmac_dma_phys(GMAC_RX_DESC_OFFSET + GMAC_RX_DESC_COUNT * GMAC_DESC_SIZE));
}

void rk3576_gmac_dma_interrupt_disable(struct rk3576_gmac *gmac)
{
    gmac_write(gmac, DMA_CH0_INT_ENABLE, 0);
}

void rk3576_gmac_dma_stop(struct rk3576_gmac *gmac)
{
    gmac_write(gmac, DMA_CH0_TX_CONTROL, gmac_read(gmac, DMA_CH0_TX_CONTROL) & ~DMA_TX_START);
    gmac_write(gmac, DMA_CH0_RX_CONTROL, gmac_read(gmac, DMA_CH0_RX_CONTROL) & ~DMA_RX_START);
}

rt_uint32_t rk3576_gmac_dma_irq_ack(struct rk3576_gmac *gmac)
{
    rt_uint32_t status = gmac_read(gmac, DMA_CH0_STATUS);

    gmac_write(gmac, DMA_CH0_STATUS, status);
    return status;
}

rt_uint32_t rk3576_gmac_dma_status(struct rk3576_gmac *gmac)
{
    return gmac_read(gmac, DMA_CH0_STATUS);
}

rt_err_t rk3576_gmac_dma_transmit(struct rk3576_gmac *gmac, struct pbuf *p)
{
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

    desc->des0 = gmac_dma_phys(GMAC_TX_BUFFER_OFFSET + gmac->tx_index * GMAC_DMA_BUFFER_SIZE);
    desc->des1 = 0;
    desc->des2 = offset;
    gmac_barrier();
    desc->des3 = DESC3_OWN | DESC3_FD | DESC3_LD | offset;
    gmac_barrier();

    next = (gmac->tx_index + 1U) % GMAC_TX_DESC_COUNT;
    gmac_write(gmac, DMA_CH0_TXDESC_TAIL, gmac_dma_phys(GMAC_TX_DESC_OFFSET + next * GMAC_DESC_SIZE));

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

struct pbuf *rk3576_gmac_dma_receive(struct rk3576_gmac *gmac)
{
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
    if (((status & (DESC3_FD | DESC3_LD)) == (DESC3_FD | DESC3_LD)) && ((status & DESC3_RX_ERROR) == 0U) &&
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

    desc->des0 = gmac_dma_phys(GMAC_RX_BUFFER_OFFSET + gmac->rx_index * GMAC_DMA_BUFFER_SIZE);
    desc->des1 = 0;
    desc->des2 = 0;
    gmac_barrier();
    desc->des3 = DESC3_OWN | DESC3_RX_IOC | DESC3_RX_BUF1_VALID;
    gmac_barrier();

    gmac->rx_index = (gmac->rx_index + 1U) % GMAC_RX_DESC_COUNT;
    gmac_write(gmac, DMA_CH0_RXDESC_TAIL, gmac_dma_phys(GMAC_RX_DESC_OFFSET + gmac->rx_index * GMAC_DESC_SIZE));
    return p;
}
