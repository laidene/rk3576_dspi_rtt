/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-3-08      GuEe-GUI     the first version
 */

#ifndef __RK3568_H__
#define __RK3568_H__



/*******************************************************************************/
/* uart */

/* key word: 33.4.1 internal address mapping */
#define UART0_MMIO_BASE     0x2ad40000
#define UART1_MMIO_BASE     0x27310000
#define UART2_MMIO_BASE     0x2ad50000
#define UART3_MMIO_BASE     0x2ad60000
#define UART4_MMIO_BASE     0x2ad70000
#define UART5_MMIO_BASE     0x2ad80000
#define UART6_MMIO_BASE     0x2ad90000
#define UART7_MMIO_BASE     0x2ada0000
#define UART8_MMIO_BASE     0x2adb0000
#define UART9_MMIO_BASE     0x2adc0000
#define UART10_MMIO_BASE    0x2afc0000
#define UART11_MMIO_BASE    0x2afd0000

/* key word: 1.3 system interrupt connection */
#define UART0_IRQ           108
#define UART1_IRQ           109
#define UART2_IRQ           110
#define UART3_IRQ           111
#define UART4_IRQ           112
#define UART5_IRQ           113
#define UART6_IRQ           114
#define UART7_IRQ           115
#define UART8_IRQ           116
#define UART9_IRQ           117
#define UART10_IRQ          118
#define UART11_IRQ          119

/* uart */
/*******************************************************************************/

/*******************************************************************************/
/* cru */

/* key word: 2.1 address mapping */
#define CRU_MMIO_BASE       0x27200000UL
#define CRU_MMIO_SIZE       0x50000UL

/* cru */
/*******************************************************************************/


/*******************************************************************************/
/* iomux */

/* key word: ioc_grf syscon, 0x26040000 size 0xc000 */
#define IOC_GRF_MMIO_BASE  0x26040000UL
#define IOC_GRF_MMIO_SIZE  0x0000c000UL

/* iomux */
/*******************************************************************************/


/*******************************************************************************/
/* gpio */

/* key word: 21.4.1 internal address mapping */
#define GPIO0_MMIO_BASE     0x27320000
#define GPIO1_MMIO_BASE     0x2ae10000
#define GPIO2_MMIO_BASE     0x2ae20000
#define GPIO3_MMIO_BASE     0x2ae30000
#define GPIO4_MMIO_BASE     0x2ae40000

#define GPIO0_0_IRQ         185
#define GPIO0_1_IRQ         186
#define GPIO0_2_IRQ         187
#define GPIO0_3_IRQ         188
#define GPIO1_0_IRQ         189
#define GPIO1_1_IRQ         190
#define GPIO1_2_IRQ         191
#define GPIO1_3_IRQ         192
#define GPIO2_0_IRQ         193
#define GPIO2_1_IRQ         194
#define GPIO2_2_IRQ         195
#define GPIO2_3_IRQ         196
#define GPIO3_0_IRQ         197
#define GPIO3_1_IRQ         198
#define GPIO3_2_IRQ         199
#define GPIO3_3_IRQ         200
#define GPIO4_0_IRQ         201
#define GPIO4_1_IRQ         202
#define GPIO4_2_IRQ         203
#define GPIO4_3_IRQ         204

/* gpio */
/*******************************************************************************/


/*******************************************************************************/
/* mmc */

/* key word: 39.4.1 internal address mapping */
/* key word: 38.4.3 detail register description */
#define SDMMC_MMIO_BASE     0x2a310000
#define SDIO_MMIO_BASE      0x2a320000
#define EMMC_MMIO_BASE      0x2a330000

#define SDMMC_DETECT_IRQ    282
#define SDMMC_IRQ           283
#define SDIO_IRQ            284
#define EMMC_IRQ            285

/* mmc */
/*******************************************************************************/


/*******************************************************************************/
/* gmac */

/* key word: 12.4.1 internal address mapping */
#define GMAC0_MMIO_BASE         0x2a220000
#define GMAC1_MMIO_BASE         0x2a230000

#define GMAC0_LPI_IRQ           324
#define GMAC0_SBD_IRQ           325
#define GMAC0_TX0_IRQ           326
#define GMAC0_TX1_IRQ           327
#define GMAC0_RX0_IRQ           328
#define GMAC0_RX1_IRQ           329
#define GMAC0_PMT_IRQ           330
#define GMAC0_MCGR_DMA_REQ_IRQ  331

#define GMAC1_LPI_IRQ           332
#define GMAC1_SBD_IRQ           333
#define GMAC1_TX0_IRQ           334
#define GMAC1_TX1_IRQ           335
#define GMAC1_RX0_IRQ           336
#define GMAC1_RX1_IRQ           337
#define GMAC1_PMT_IRQ           338
#define GMAC1_MCGR_DMA_REQ_IRQ  339

/* gmac */
/*******************************************************************************/



/*******************************************************************************/
/* gic */

/* key word: 1.3 system interrupt connection */
#define MAX_HANDLERS        512
#define GIC_IRQ_START       0
#define ARM_GIC_NR_IRQS     512
#define ARM_GIC_MAX_NR      1

#define IRQ_ARM_IPI_KICK    0
#define IRQ_ARM_IPI_CALL    1

/* key word 1.1 address mapping -> gic400 */
#define GIC_PL400_BASE      0x2a700000
#define GIC_PL400_GICD      (GIC_PL400_BASE + 0x1000)
#define GIC_PL400_GICC      (GIC_PL400_BASE + 0x2000)

rt_inline rt_uint32_t platform_get_gic_dist_base(void)
{
    return GIC_PL400_GICD;
}

rt_inline rt_uint32_t platform_get_gic_cpu_base(void)
{
    return GIC_PL400_GICC;
}


/* gic */
/*******************************************************************************/

#endif /* __RK3568_H__ */
