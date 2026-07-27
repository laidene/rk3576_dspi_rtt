/*
 * RK3576 GMAC0 driver for the Synopsys DWMAC 4.20a controller.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include "gmac_internal.h"
#include "yt8521s.h"

#ifdef BSP_USING_GMAC0

#include <lwip/opt.h>
#include <lwip/pbuf.h>
#include <netdev.h>
#include <netif/ethernetif.h>

static struct rk3576_gmac gmac0;

/* 更新gmac协商参数 */
static void gmac_adjust_link(struct rk3576_gmac *gmac, rt_uint32_t speed, rt_bool_t full_duplex)
{
    enum {
        GMAC_CONFIG     = 0x0000U,
        GMAC_CONFIG_PS  = GMAC_BIT(15),
        GMAC_CONFIG_FES = GMAC_BIT(14),
        GMAC_CONFIG_DM  = GMAC_BIT(13),
    };
    rt_uint32_t value;

    rk3576_gmac_clock_set_speed(speed);

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

    ret = rk3576_gmac_clock_enable();
    if (ret != RT_EOK) {
        return ret;
    }
    ret = rk3576_gmac_power_on();
    if (ret != RT_EOK) {
        rt_kprintf("gmac0: failed to power on SDGMAC domain: %d\n", ret);
        return ret;
    }
    ret = rk3576_gmac_pins_init();
    if (ret != RT_EOK) {
        return ret;
    }

    rk3576_gmac_clock_reset();
    rk3576_gmac_phy_reset();

    ret = rk3576_gmac_dma_reset(gmac);
    if (ret != RT_EOK) {
        rt_kprintf("gmac0: DMA reset timed out\n");
        return ret;
    }
    ret = yt8521s_init(gmac);
    if (ret != RT_EOK) {
        rt_kprintf("gmac0: PHY not found at MDIO address %u\n", YT8521S_PHY_ADDR);
        return ret;
    }

    gmac->initialized = RT_TRUE;
    return RT_EOK;
}

static void gmac_isr(int vector, void *parameter)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)parameter;
    rt_uint32_t status;

    (void)vector;
    status = rk3576_gmac_dma_irq_ack(gmac);

    if ((status & GMAC_DMA_STATUS_FATAL_BUS_ERROR) != 0U) {
        gmac->fatal_bus_errors++;
    }
    if ((status & (GMAC_DMA_STATUS_RX_INTERRUPT | GMAC_DMA_STATUS_RX_BUFFER_UNAVAIL)) != 0U) {
        eth_device_ready(&gmac->parent);
    }
}


/***********************************************************************************************/
/* ops */


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
    rk3576_gmac_dma_start(gmac);

    rt_kprintf("gmac0: registered as e0, MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
               gmac->mac[0],
               gmac->mac[1],
               gmac->mac[2],
               gmac->mac[3],
               gmac->mac[4],
               gmac->mac[5]);
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
        rk3576_gmac_dma_interrupt_disable(gmac);
        rt_hw_interrupt_mask(GMAC0_SBD_IRQ);
        rk3576_gmac_dma_stop(gmac);
    }
    return RT_EOK;
}

static rt_size_t gmac_device_read(rt_device_t device, rt_off_t pos, void *buffer, rt_size_t size)
{
    (void)device;
    (void)pos;
    (void)buffer;
    (void)size;
    return 0;
}

static rt_size_t gmac_device_write(rt_device_t device, rt_off_t pos, const void *buffer, rt_size_t size)
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

    if (((cmd == NIOCTL_GADDR) || (cmd == RT_DEVICE_CTRL_NETIF_GETMAC)) && (args != RT_NULL)) {
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

/* ops */
/***********************************************************************************************/

static rt_err_t gmac_eth_tx(rt_device_t device, struct pbuf *p)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)device;

    return rk3576_gmac_dma_transmit(gmac, p);
}

static struct pbuf *gmac_eth_rx(rt_device_t device)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)device;

    return rk3576_gmac_dma_receive(gmac);
}

static void gmac_phy_thread(void *parameter)
{
    struct rk3576_gmac *gmac = (struct rk3576_gmac *)parameter;
    rt_bool_t up;
    rt_bool_t full_duplex;
    rt_uint32_t speed;

    while (1) {
        if (gmac->initialized && (yt8521s_get_link(gmac, &up, &speed, &full_duplex) == RT_EOK)) {
            if (up && ((!gmac->link_up) || (speed != gmac->speed) || (full_duplex != gmac->full_duplex))) {
                gmac_adjust_link(gmac, speed, full_duplex);
                gmac->speed = speed;
                gmac->full_duplex = full_duplex;
                rt_kprintf("gmac0: link up, %u Mbps, %s duplex\n", speed, full_duplex ? "full" : "half");
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



static int rk3576_gmac0_register(void)
{
    static const rt_uint8_t hwaddr[6] = BSP_GMAC0_MAC_ADDRESS;
    struct netdev *netdev;
    ip_addr_t addr;
    rt_thread_t thread;
    rt_uint16_t flags;
    rt_err_t ret;

    rt_memset(&gmac0, 0, sizeof(gmac0));

    gmac0.base = GMAC0_MMIO_BASE;
    rt_memcpy(gmac0.mac, hwaddr, sizeof(gmac0.mac));

    rt_mutex_init(&gmac0.tx_lock, "e0tx", RT_IPC_FLAG_PRIO);
    gmac0.parent.parent.ops = &gmac_device_ops;
    gmac0.parent.parent.user_data = &gmac0;
    gmac0.parent.eth_tx = gmac_eth_tx;
    gmac0.parent.eth_rx = gmac_eth_rx;

    flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;
    #if LWIP_IGMP
    flags |= NETIF_FLAG_IGMP;
    #endif
    ret = eth_device_init_with_flag(&gmac0.parent, "e0", flags);
    if (ret != RT_EOK) {
        return ret;
    }

    netdev = netdev_get_by_name("e0");
    if (netdev == RT_NULL) {
        return -RT_ERROR;
    }

    ret = netdev_dhcp_enabled(netdev, RT_FALSE);
    if (ret != RT_EOK) {
        return ret;
    }

    if (!ipaddr_aton(BSP_GMAC0_IP_ADDRESS, &addr)) {
        return -RT_EINVAL;
    }
    ret = netdev_set_ipaddr(netdev, &addr);
    if (ret != RT_EOK) {
        return ret;
    }

    if (!ipaddr_aton(BSP_GMAC0_NETMASK, &addr)) {
        return -RT_EINVAL;
    }
    ret = netdev_set_netmask(netdev, &addr);
    if (ret != RT_EOK) {
        return ret;
    }

    if (!ipaddr_aton(BSP_GMAC0_GATEWAY, &addr)) {
        return -RT_EINVAL;
    }
    ret = netdev_set_gw(netdev, &addr);
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
        phy_status = yt8521s_read_status(&gmac0);
    }

    rt_kprintf("gmac0: init=%u link=%u speed=%u duplex=%s phy11=0x%04x\n",
               gmac0.initialized,
               gmac0.link_up,
               gmac0.speed,
               gmac0.full_duplex ? "full" : "half",
               phy_status < 0 ? 0xffffU : (rt_uint32_t)phy_status);

    rt_kprintf("gmac0: dma_status=0x%08x fatal_bus_errors=%u tx=%u rx=%u\n",
               gmac0.initialized ? rk3576_gmac_dma_status(&gmac0) : 0U,
               gmac0.fatal_bus_errors,
               gmac0.tx_index,
               gmac0.rx_index);
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
