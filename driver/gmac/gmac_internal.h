/*
 * RK3576 GMAC0 internal interfaces.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __GMAC_INTERNAL_H__
#define __GMAC_INTERNAL_H__

#include <rthw.h>
#include <rtdevice.h>
#include <rtthread.h>

#include <lwip/pbuf.h>
#include <netif/ethernetif.h>

#include <board.h>

#include "drv_gmac.h"

#define GMAC_BIT(n)                       (1U << (n))
#define GMAC_HIWORD_UPDATE(mask, value)   ((((mask) & 0xffffU) << 16) | ((value) & 0xffffU))

#define GMAC_DMA_STATUS_FATAL_BUS_ERROR   GMAC_BIT(12)
#define GMAC_DMA_STATUS_RX_BUFFER_UNAVAIL GMAC_BIT(7)
#define GMAC_DMA_STATUS_RX_INTERRUPT      GMAC_BIT(6)

struct gmac_desc;

struct rk3576_gmac {
    struct eth_device       parent;
    rt_ubase_t              base;
    rt_uint8_t              mac[6];

    volatile struct gmac_desc   *tx_desc;
    volatile struct gmac_desc   *rx_desc;
    rt_uint8_t                  *tx_buffer;
    rt_uint8_t                  *rx_buffer;
    rt_uint32_t                  tx_index;
    rt_uint32_t                  rx_index;
    struct rt_mutex              tx_lock;

    rt_bool_t                   initialized;
    rt_bool_t                   link_up;
    rt_bool_t                   full_duplex;
    rt_uint32_t                 speed;
    rt_uint32_t                 fatal_bus_errors;
};

static inline rt_uint32_t gmac_read(struct rk3576_gmac *gmac, rt_uint32_t offset)
{
    return HWREG32(gmac->base + offset);
}

static inline void gmac_write(struct rk3576_gmac *gmac, rt_uint32_t offset, rt_uint32_t value)
{
    HWREG32(gmac->base + offset) = value;
}

static inline void gmac_barrier(void)
{
    __asm__ volatile("dsb sy" ::: "memory");
}

rt_err_t rk3576_gmac_clock_enable(void);
rt_err_t rk3576_gmac_power_on(void);
void rk3576_gmac_clock_reset(void);
void rk3576_gmac_clock_set_speed(rt_uint32_t speed);

rt_err_t rk3576_gmac_pins_init(void);
void rk3576_gmac_phy_reset(void);

rt_err_t rk3576_gmac_dma_reset(struct rk3576_gmac *gmac);
void rk3576_gmac_dma_start(struct rk3576_gmac *gmac);
void rk3576_gmac_dma_interrupt_disable(struct rk3576_gmac *gmac);
void rk3576_gmac_dma_stop(struct rk3576_gmac *gmac);
rt_uint32_t rk3576_gmac_dma_irq_ack(struct rk3576_gmac *gmac);
rt_uint32_t rk3576_gmac_dma_status(struct rk3576_gmac *gmac);
rt_err_t rk3576_gmac_dma_transmit(struct rk3576_gmac *gmac, struct pbuf *p);
struct pbuf *rk3576_gmac_dma_receive(struct rk3576_gmac *gmac);

#endif /* __GMAC_INTERNAL_H__ */
