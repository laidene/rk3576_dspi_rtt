/*
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DRV_GMAC_H__
#define __DRV_GMAC_H__

#include <rtthread.h>

#define RK3576_GMAC_DMA_POOL_SIZE  0x00020000UL

#ifdef BSP_USING_GMAC0
extern rt_uint8_t rk3576_gmac_dma_pool[RK3576_GMAC_DMA_POOL_SIZE];
#endif

#endif /* __DRV_GMAC_H__ */
