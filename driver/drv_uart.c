#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <board.h>
#include <drv_cru.h>
#include <drv_iomux.h>


/*******************************************************************************/
/* uart reg */

#define UART_RX             0       /* In: Receive buffer */
#define UART_TX             0       /* Out: Transmit buffer */

#define UART_DLL            0       /* Out: Divisor Latch Low */
#define UART_DLM            1       /* Out: Divisor Latch High */

#define UART_IER            1       /* Out: Interrupt Enable Register */
#define UART_IER_RDI        0x01    /* Enable receiver data interrupt */

#define UART_SSR            0x22    /* In: Software Reset Register */
#define UART_USR            0x1f    /* UART Status Register */

#define UART_LCR            3       /* Out: Line Control Register */
#define UART_LCR_DLAB       0x80    /* Divisor latch access bit */
#define UART_LCR_SPAR       0x20    /* Stick parity (?) */
#define UART_LCR_PARITY     0x8     /* Parity Enable */
#define UART_LCR_STOP       0x4     /* Stop bits: 0=1 bit, 1=2 bits */
#define UART_LCR_WLEN8      0x3     /* Wordlength: 8 bits */

#define UART_MCR            4       /* Out: Modem Control Register */
#define UART_MCR_RTS        0x02    /* RTS complement */

#define UART_LSR            5       /* In: Line Status Register */
#define UART_LSR_BI         0x10    /* Break interrupt indicator */
#define UART_LSR_DR         0x01    /* Receiver data ready */
#define UART_LSR_THRE       0x20    /* Transmit holding register empty */

#define UART_IIR            2       /* In: Interrupt ID Register */
#define UART_IIR_NO_INT     0x01    /* No interrupts pending */
#define UART_IIR_BUSY       0x07    /* DesignWare APB Busy Detect */
#define UART_IIR_RX_TIMEOUT 0x0c    /* OMAP RX Timeout interrupt */

#define UART_FCR            2       /* Out: FIFO Control Register */
#define UART_FCR_EN_FIFO    0x01    /* Enable the FIFO */
#define UART_FCR_CLEAR_RCVR 0x02    /* Clear the RCVR FIFO */
#define UART_FCR_CLEAR_XMIT 0x04    /* Clear the XMIT FIFO */

#define UART_REG_SHIFT      0x2     /* Register Shift*/

/* uart reg */
/*******************************************************************************/

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))



struct hw_uart_device {
    rt_ubase_t   hw_base;
    rt_uint32_t  irqno;
    rt_uint32_t  id;
    rt_uint32_t  mux_group;
    const char  *name;
};



/**
 * @brief 串口配置(基地址，中断号，复用类型)
 */
static struct hw_uart_device   uart_devs[] = {
#ifdef RT_USING_UART0
    {
        .hw_base    = UART0_MMIO_BASE,
        .irqno      = UART0_IRQ,
        .id         = 0,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart0",
    },
#endif
#ifdef RT_USING_UART1
    {
        .hw_base    = UART1_MMIO_BASE,
        .irqno      = UART1_IRQ,
        .id         = 1,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart1",
    },
#endif
#ifdef RT_USING_UART2
    {
        .hw_base    = UART2_MMIO_BASE,
        .irqno      = UART2_IRQ,
        .id         = 2,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart2",
    },
#endif
#ifdef RT_USING_UART3
    {
        .hw_base    = UART3_MMIO_BASE,
        .irqno      = UART3_IRQ,
        .id         = 3,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart3",
    },
#endif
#ifdef RT_USING_UART4
    {
        .hw_base    = UART4_MMIO_BASE,
        .irqno      = UART4_IRQ,
        .id         = 4,
        .mux_group  = RK3576_UART_M0,
        .name        "uart4",
    },
#endif
#ifdef RT_USING_UART5
    {
        .hw_base    = UART5_MMIO_BASE,
        .irqno      = UART5_IRQ,
        .id         = 5,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart5",
    },
#endif
#ifdef RT_USING_UART6
    {
        .hw_base    = UART6_MMIO_BASE,
        .irqno      = UART6_IRQ,
        .id         = 6,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart6",
    },
#endif
#ifdef RT_USING_UART7
    {
        .hw_base    = UART7_MMIO_BASE,
        .irqno      = UART7_IRQ,
        .id         = 7,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart7",
    },
#endif
#ifdef RT_USING_UART8
    {
        .hw_base    = UART8_MMIO_BASE,
        .irqno      = UART8_IRQ,
        .id         = 8,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart8",
    },
#endif
#ifdef RT_USING_UART9
    {
        .hw_base    = UART9_MMIO_BASE,
        .irqno      = UART9_IRQ,
        .id         = 9,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart9",
    },
#endif
#ifdef RT_USING_UART10
    {
        .hw_base    = UART10_MMIO_BASE,
        .irqno      = UART10_IRQ,
        .id         = 10,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart10",
    },
#endif
#ifdef RT_USING_UART11
    {
        .hw_base    = UART11_MMIO_BASE,
        .irqno      = UART11_IRQ,
        .id         = 11,
        .mux_group  = RK3576_UART_M0,
        .name       = "uart11",
    },
#endif
};

static struct rt_serial_device serial_devs[ARRAY_SIZE(uart_devs)];


/*******************************************************************************/
/* reg rw */

rt_inline rt_uint32_t dw8250_read32(rt_ubase_t addr, rt_ubase_t offset)
{
    return *((volatile rt_uint32_t *)(addr + (offset << UART_REG_SHIFT)));
}

rt_inline void        dw8250_write32(rt_ubase_t addr, rt_ubase_t offset, rt_uint32_t value)
{
    *((volatile rt_uint32_t *)(addr + (offset << UART_REG_SHIFT))) = value;

    if (offset == UART_LCR) {
        int tries = 1000;

        /* Make sure LCR write wasn't ignored */
        while (tries--) {
            unsigned int lcr = dw8250_read32(addr, UART_LCR);

            if ((value & ~UART_LCR_SPAR) == (lcr & ~UART_LCR_SPAR)) {
                return;
            }

            dw8250_write32(addr, UART_FCR, UART_FCR_EN_FIFO | UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);
            dw8250_read32(addr, UART_RX);

            *((volatile rt_uint32_t *)(addr + (offset << UART_REG_SHIFT))) = value;
        }
    }
}

/* reg rw */
/*******************************************************************************/


/*******************************************************************************/
/* ops fun */

static rt_err_t dw8250_uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    RT_UNUSED(cfg);

    rt_base_t               base;
    rt_uint32_t             baud_rate, target_clk, uart_clk, divisor;
    rt_err_t                ret;
    struct hw_uart_device  *uart;

    RT_ASSERT(serial != RT_NULL);
    uart        = (struct hw_uart_device *)serial->parent.user_data;
    base        = uart->hw_base;
    baud_rate   = serial->config.baud_rate;

    ret = rk3576_uart_iomux_init(uart->id, uart->mux_group);
    if (ret != RT_EOK) {
        return ret;
    }

    ret = rk3576_cru_uart_enable(uart->id);
    if (ret != RT_EOK) {
        return ret;
    }

    if (baud_rate <= 115200) {
        target_clk = RK3576_OSC_HZ;
    } else if ((baud_rate == 230400) || (baud_rate == 1152000)) {
        target_clk = baud_rate * 16U * 2U;
    } else {
        target_clk = baud_rate * 16U;
    }

    uart_clk = rk3576_cru_set_uart_clk(uart->id, target_clk);
    if (uart_clk == 0) {
        uart_clk = RK3576_OSC_HZ;
    }

    divisor = (uart_clk + (baud_rate * 8U)) / (baud_rate * 16U);
    if (divisor == 0) {
        divisor = 1;
    }

    /* Resset UART */
    dw8250_write32(base, UART_SSR, 1);
    dw8250_write32(base, UART_SSR, 0);

    dw8250_write32(base, UART_IER, !UART_IER_RDI);
    dw8250_write32(base, UART_FCR, UART_FCR_EN_FIFO | UART_FCR_CLEAR_RCVR | UART_FCR_CLEAR_XMIT);

    /* Disable flow ctrl */
    dw8250_write32(base, UART_MCR, 0);
    /* Clear RTS */
    dw8250_write32(base, UART_MCR, dw8250_read32(base, UART_MCR) | UART_MCR_RTS);

    /* Enable access DLL & DLH */
    dw8250_write32(base, UART_LCR, dw8250_read32(base, UART_LCR) | UART_LCR_DLAB);
    dw8250_write32(base, UART_DLL, (divisor & 0xff));
    dw8250_write32(base, UART_DLM, (divisor & 0xff00) >> 8);
    /* Clear DLAB bit */
    dw8250_write32(base, UART_LCR, dw8250_read32(base, UART_LCR) & (~UART_LCR_DLAB));

    dw8250_write32(base, UART_LCR, (dw8250_read32(base, UART_LCR) & (~UART_LCR_WLEN8)) | UART_LCR_WLEN8);
    dw8250_write32(base, UART_LCR, dw8250_read32(base, UART_LCR) & (~UART_LCR_STOP));
    dw8250_write32(base, UART_LCR, dw8250_read32(base, UART_LCR) & (~UART_LCR_PARITY));

    dw8250_write32(base, UART_IER, UART_IER_RDI);

    return RT_EOK;
}

static rt_err_t dw8250_uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;

    switch (cmd) {
    case RT_DEVICE_CTRL_CLR_INT:
        /* Disable rx irq */
        dw8250_write32(uart->hw_base, UART_IER, !UART_IER_RDI);
        rt_hw_interrupt_mask(uart->irqno);
        break;

    case RT_DEVICE_CTRL_SET_INT:
        /* Enable rx irq */
        dw8250_write32(uart->hw_base, UART_IER, UART_IER_RDI);
        rt_hw_interrupt_umask(uart->irqno);
        break;
    }

    return RT_EOK;
}

static int dw8250_uart_putc(struct rt_serial_device *serial, char c)
{
    rt_base_t base;
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;
    base = uart->hw_base;

    while ((dw8250_read32(base, UART_LSR) & UART_LSR_THRE) == 0) {
    }

    dw8250_write32(base, UART_TX, c);

    return 1;
}

static int dw8250_uart_getc(struct rt_serial_device *serial)
{
    int ch = -1;
    rt_base_t base;
    struct hw_uart_device *uart;

    RT_ASSERT(serial != RT_NULL);
    uart = (struct hw_uart_device *)serial->parent.user_data;
    base = uart->hw_base;

    if ((dw8250_read32(base, UART_LSR) & 0x1)) {
        ch = dw8250_read32(base, UART_RX) & 0xff;
    }

    return ch;
}

static const struct rt_uart_ops _uart_ops = {
    dw8250_uart_configure,
    dw8250_uart_control,
    dw8250_uart_putc,
    dw8250_uart_getc,
};

/* ops fun */
/*******************************************************************************/


static void rt_hw_uart_isr(int irqno, void *param)
{
    unsigned int iir, status;
    struct rt_serial_device *serial = (struct rt_serial_device *)param;
    struct hw_uart_device   *uart   = (struct hw_uart_device *)serial->parent.user_data;

    iir = dw8250_read32(uart->hw_base, UART_IIR);

    /* If don't do this in non-DMA mode then the "RX TIMEOUT" interrupt will fire forever. */
    if ((iir & 0x3f) == UART_IIR_RX_TIMEOUT) {

        status = dw8250_read32(uart->hw_base, UART_LSR);

        if (!(status & (UART_LSR_DR | UART_LSR_BI))) {
            dw8250_read32(uart->hw_base, UART_RX);
        }
    }

    if (!(iir & UART_IIR_NO_INT)) {
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
    }

    if ((iir & UART_IIR_BUSY) == UART_IIR_BUSY) {
        /* Clear the USR */
        dw8250_read32(uart->hw_base, UART_USR);

        return;
    }
}



int rt_hw_uart_init(void)
{
    rt_size_t i;
    struct hw_uart_device   *uart;
    struct rt_serial_device *serial;
    struct serial_configure  config = RT_SERIAL_CONFIG_DEFAULT;

    config.baud_rate = 1500000;

    for (i = 0; i < ARRAY_SIZE(uart_devs); i++) {
        uart   = &uart_devs[i];
        serial = &serial_devs[i];

        serial->ops    = &_uart_ops;
        serial->config = config;

        rt_hw_serial_register(serial, uart->name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX, uart);
        rt_hw_interrupt_install(uart->irqno, rt_hw_uart_isr, serial, uart->name);
    }

    return 0;
}
