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

/* UART */
/* key word: 33.4.1 internal address mapping */
#define UART0_MMIO_BASE 0x2ad40000
#define UART1_MMIO_BASE 0x27310000
#define UART2_MMIO_BASE 0x2ad50000
#define UART3_MMIO_BASE 0x2ad60000
#define UART4_MMIO_BASE 0x2ad70000
#define UART5_MMIO_BASE 0x2ad80000
#define UART6_MMIO_BASE 0x2ad90000
#define UART7_MMIO_BASE 0x2ada0000
#define UART8_MMIO_BASE 0x2adb0000
#define UART9_MMIO_BASE 0x2adc0000

#define UART_MMIO_SIZE  0x100

/* key word: 1.3 system interrupt connection */
#define UART0_IRQ       (32 + 76)
#define UART1_IRQ       (32 + 77)
#define UART2_IRQ       (32 + 78)
#define UART3_IRQ       (32 + 79)
#define UART4_IRQ       (32 + 80)
#define UART5_IRQ       (32 + 81)
#define UART6_IRQ       (32 + 82)
#define UART7_IRQ       (32 + 83)
#define UART8_IRQ       (32 + 84)
#define UART9_IRQ       (32 + 85)

/* GPIO */
#define GPIO0_MMIO_BASE 0xfdd60000
#define GPIO1_MMIO_BASE 0xfe740000
#define GPIO2_MMIO_BASE 0xfe750000
#define GPIO3_MMIO_BASE 0xfe760000
#define GPIO4_MMIO_BASE 0xfe770000

#define GPIO_MMIO_SIZE  0x100

#define GPIO_IRQ_BASE   (32 + 33)
#define GPIO0_IRQ       (GPIO_IRQ_BASE + 0)
#define GPIO1_IRQ       (GPIO_IRQ_BASE + 1)
#define GPIO2_IRQ       (GPIO_IRQ_BASE + 2)
#define GPIO3_IRQ       (GPIO_IRQ_BASE + 3)
#define GPIO4_IRQ       (GPIO_IRQ_BASE + 4)

/* MMC */
#define MMC0_MMIO_BASE  0xfe310000  /* sdhci */
#define MMC1_MMIO_BASE  0xfe2b0000  /* sdmmc0 */
#define MMC2_MMIO_BASE  0xfe2c0000  /* sdmmc1 */
#define MMC3_MMIO_BASE  0xfe000000  /* sdmmc2 */

#define MMC0_MMIO_SIZE  0x10000
#define MMC_MMIO_SIZE   0x4000

#define MMC0_IRQ        (32 + 19)
#define MMC1_IRQ        (32 + 98)
#define MMC2_IRQ        (32 + 99)
#define MMC3_IRQ        (32 + 100)

/* Ethernet */
#define GMAC0_MMIO_BASE 0xfe2a0000
#define GMAC1_MMIO_BASE 0xfe010000

#define GMAC_MMIO_SIZE  0x10000

#define GMAC0_MAC_IRQ   (32 + 27)
#define GMAC0_WAKE_IRQ  (32 + 24)
#define GMAC1_MAC_IRQ   (32 + 32)
#define GMAC1_WAKE_IRQ  (32 + 29)

/* GIC */
#define MAX_HANDLERS        256
#define GIC_IRQ_START       0
#define ARM_GIC_NR_IRQS     256
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

#endif /* __RK3568_H__ */
