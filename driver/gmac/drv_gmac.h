/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_GMAC_H__
#define __DRV_GMAC_H__

#include <rtthread.h>

#define BSP_GMAC0_MAC_ADDRESS      { 0x02U, 0x35U, 0x76U, 0x00U, 0x00U, 0x01U }
#define BSP_GMAC0_IP_ADDRESS       "192.168.10.222"
#define BSP_GMAC0_NETMASK          "255.255.255.0"
#define BSP_GMAC0_GATEWAY          "192.168.10.1"

#define RK3576_GMAC_DMA_POOL_SIZE  0x00020000UL

#ifdef BSP_USING_GMAC0
extern rt_uint8_t rk3576_gmac_dma_pool[RK3576_GMAC_DMA_POOL_SIZE];
#endif

#endif /* __DRV_GMAC_H__ */
