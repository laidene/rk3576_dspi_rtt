#include <rthw.h>
#include <rtthread.h>

#include <board.h>
#include <drv_iomux.h>

#define RK3576_BIT(n)                   (1U << (n))
#define RK3576_HIWORD(mask, value)      ((((mask) & 0xffffU) << 16) | ((value) & 0xffffU))

#define RK3576_IOMUX_BITS_PER_PIN       4U
#define RK3576_IOMUX_PINS_PER_REG       4U
#define RK3576_IOMUX_PINS_PER_GROUP     8U
#define RK3576_IOMUX_MASK               0xfU

#define RK3576_DRV_BITS_PER_PIN         4U
#define RK3576_DRV_PINS_PER_REG         4U
#define RK3576_DRV_GPIO0_AL_OFFSET      0x10U
#define RK3576_DRV_GPIO0_BH_OFFSET      0x2014U
#define RK3576_DRV_GPIO1_OFFSET         0x6020U
#define RK3576_DRV_GPIO2_OFFSET         0x6040U
#define RK3576_DRV_GPIO3_OFFSET         0x6060U
#define RK3576_DRV_GPIO4_AL_OFFSET      0x6080U
#define RK3576_DRV_GPIO4_CL_OFFSET      0xa090U
#define RK3576_DRV_GPIO4_DL_OFFSET      0xb098U

#define RK3576_PULL_BITS_PER_PIN        2U
#define RK3576_PULL_PINS_PER_REG        8U
#define RK3576_PULL_GPIO0_AL_OFFSET     0x20U
#define RK3576_PULL_GPIO0_BH_OFFSET     0x2028U
#define RK3576_PULL_GPIO1_OFFSET        0x6110U
#define RK3576_PULL_GPIO2_OFFSET        0x6120U
#define RK3576_PULL_GPIO3_OFFSET        0x6130U
#define RK3576_PULL_GPIO4_AL_OFFSET     0x6140U
#define RK3576_PULL_GPIO4_CL_OFFSET     0xa148U
#define RK3576_PULL_GPIO4_DL_OFFSET     0xb14cU
#define RK3576_PULL_HW_NONE             0U
#define RK3576_PULL_HW_DOWN             1U
#define RK3576_PULL_HW_UP               3U

#define RK3576_SMT_BITS_PER_PIN         1U
#define RK3576_SMT_PINS_PER_REG         8U
#define RK3576_SMT_GPIO0_AL_OFFSET      0x30U
#define RK3576_SMT_GPIO0_BH_OFFSET      0x2040U
#define RK3576_SMT_GPIO1_OFFSET         0x6210U
#define RK3576_SMT_GPIO2_OFFSET         0x6220U
#define RK3576_SMT_GPIO3_OFFSET         0x6230U
#define RK3576_SMT_GPIO4_AL_OFFSET      0x6240U
#define RK3576_SMT_GPIO4_CL_OFFSET      0xa248U
#define RK3576_SMT_GPIO4_DL_OFFSET      0xb24cU


/* gpio0_d4 ---> bank=0 pin=pd4 */
struct rk3576_uart_pin {
    rt_uint8_t bank;
    rt_uint8_t pin;
    rt_uint8_t mux;
};

struct rk3576_uart_iomux_group {
    rt_uint8_t             valid;
    struct rk3576_uart_pin rx;
    struct rk3576_uart_pin tx;
};

static const rt_uint32_t rk3576_iomux_offsets[RK3576_GPIO_BANKS][4] =
{
    {0x0000U, 0x0008U, 0x2004U, 0x200cU},
    {0x4020U, 0x4028U, 0x4030U, 0x4038U},
    {0x4040U, 0x4048U, 0x4050U, 0x4058U},
    {0x4060U, 0x4068U, 0x4070U, 0x4078U},
    {0x4080U, 0x4088U, 0xa390U, 0xb398U},
};


/**
 * @brief 串口复用引脚配置表
 * 4种模式 每个模式对应2个引脚
 */
static const struct rk3576_uart_iomux_group rk3576_uart_iomux[12][4] =
{
    [0] =
    {
        [RK3576_UART_M0] = {1, {0, RK_PD5, 9},  {0, RK_PD4, 9}},
        [RK3576_UART_M1] = {1, {2, RK_PA0, 9},  {2, RK_PA1, 9}},
    },
    [1] =
    {
        [RK3576_UART_M0] = {1, {0, RK_PC0, 10}, {0, RK_PB7, 10}},
        [RK3576_UART_M1] = {1, {2, RK_PB1, 9},  {2, RK_PB0, 9}},
        [RK3576_UART_M2] = {1, {3, RK_PA6, 9},  {3, RK_PA7, 9}},
    },
    [2] =
    {
        [RK3576_UART_M0] = {1, {1, RK_PC7, 9},  {1, RK_PC6, 9}},
        [RK3576_UART_M1] = {1, {4, RK_PB4, 10}, {4, RK_PB5, 10}},
        [RK3576_UART_M2] = {1, {3, RK_PB7, 9},  {3, RK_PC0, 9}},
    },
    [3] =
    {
        [RK3576_UART_M0] = {1, {3, RK_PA1, 9},  {3, RK_PA0, 9}},
        [RK3576_UART_M1] = {1, {4, RK_PA1, 9},  {4, RK_PA0, 9}},
        [RK3576_UART_M2] = {1, {1, RK_PC1, 9},  {1, RK_PC0, 9}},
    },
    [4] =
    {
        [RK3576_UART_M0] = {1, {2, RK_PD1, 9},  {2, RK_PD0, 9}},
        [RK3576_UART_M1] = {1, {1, RK_PC5, 9},  {1, RK_PC4, 9}},
        [RK3576_UART_M2] = {1, {0, RK_PB5, 10}, {0, RK_PB4, 10}},
    },
    [5] =
    {
        [RK3576_UART_M0] = {1, {3, RK_PD4, 9},  {3, RK_PD5, 9}},
        [RK3576_UART_M1] = {1, {4, RK_PB1, 10}, {4, RK_PB0, 10}},
        [RK3576_UART_M2] = {1, {2, RK_PA4, 9},  {2, RK_PA5, 9}},
    },
    [6] =
    {
        [RK3576_UART_M0] = {1, {4, RK_PA6, 10}, {4, RK_PA4, 10}},
        [RK3576_UART_M1] = {1, {2, RK_PD3, 9},  {2, RK_PD2, 9}},
        [RK3576_UART_M2] = {1, {1, RK_PB3, 9},  {1, RK_PB0, 9}},
        [RK3576_UART_M3] = {1, {4, RK_PC5, 13}, {4, RK_PC4, 13}},
    },
    [7] =
    {
        [RK3576_UART_M0] = {1, {2, RK_PB7, 9},  {2, RK_PB6, 9}},
        [RK3576_UART_M1] = {1, {1, RK_PA3, 9},  {1, RK_PA2, 9}},
        [RK3576_UART_M2] = {1, {2, RK_PA0, 10}, {2, RK_PA1, 10}},
    },
    [8] =
    {
        [RK3576_UART_M0] = {1, {3, RK_PC5, 9},  {3, RK_PC6, 9}},
        [RK3576_UART_M1] = {1, {2, RK_PA7, 9},  {2, RK_PA6, 9}},
        [RK3576_UART_M2] = {1, {0, RK_PC2, 10}, {0, RK_PC1, 10}},
    },
    [9] =
    {
        [RK3576_UART_M0] = {1, {2, RK_PC0, 9},  {2, RK_PC1, 9}},
        [RK3576_UART_M1] = {1, {3, RK_PB2, 9},  {3, RK_PB3, 9}},
        [RK3576_UART_M2] = {1, {4, RK_PC3, 13}, {4, RK_PC2, 13}},
    },
    [10] =
    {
        [RK3576_UART_M0] = {1, {3, RK_PB0, 9},  {3, RK_PB1, 9}},
        [RK3576_UART_M1] = {1, {1, RK_PD1, 9},  {1, RK_PD0, 9}},
        [RK3576_UART_M2] = {1, {0, RK_PC5, 10}, {0, RK_PC4, 10}},
    },
    [11] =
    {
        [RK3576_UART_M0] = {1, {3, RK_PC1, 9},  {3, RK_PC4, 9}},
        [RK3576_UART_M1] = {1, {2, RK_PC5, 9},  {2, RK_PC4, 9}},
        [RK3576_UART_M2] = {1, {4, RK_PC1, 13}, {4, RK_PC0, 13}},
    },
};




static void rk3576_ioc_write(rt_uint32_t offset, rt_uint32_t value)
{
    HWREG32(IOC_GRF_MMIO_BASE + offset) = value;
}

static rt_err_t rk3576_check_pin(rt_uint32_t bank, rt_uint32_t pin)
{
    if ((bank >= RK3576_GPIO_BANKS) || (pin >= RK3576_GPIO_PINS)) {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static rt_err_t rk3576_drive_reg_bit(rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t *reg, rt_uint32_t *bit)
{
    if (rk3576_check_pin(bank, pin) != RT_EOK) {
        return -RT_EINVAL;
    }

    if ((bank == 0U) && (pin < RK_PB4)) {
        *reg = RK3576_DRV_GPIO0_AL_OFFSET;
    } else if (bank == 0U) {
        *reg = RK3576_DRV_GPIO0_BH_OFFSET - 0xcU;
    } else if (bank == 1U) {
        *reg = RK3576_DRV_GPIO1_OFFSET;
    } else if (bank == 2U) {
        *reg = RK3576_DRV_GPIO2_OFFSET;
    } else if (bank == 3U) {
        *reg = RK3576_DRV_GPIO3_OFFSET;
    } else if ((bank == 4U) && (pin < RK_PC0)) {
        *reg = RK3576_DRV_GPIO4_AL_OFFSET;
    } else if ((bank == 4U) && (pin < RK_PD0)) {
        *reg = RK3576_DRV_GPIO4_CL_OFFSET - 0x10U;
    } else {
        *reg = RK3576_DRV_GPIO4_DL_OFFSET - 0x18U;
    }

    *reg += (pin / RK3576_DRV_PINS_PER_REG) * sizeof(rt_uint32_t);
    *bit = (pin % RK3576_DRV_PINS_PER_REG) * RK3576_DRV_BITS_PER_PIN;

    return RT_EOK;
}

static rt_err_t rk3576_pull_reg_bit(rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t *reg, rt_uint32_t *bit)
{
    if (rk3576_check_pin(bank, pin) != RT_EOK) {
        return -RT_EINVAL;
    }

    if ((bank == 0U) && (pin < RK_PB4)) {
        *reg = RK3576_PULL_GPIO0_AL_OFFSET;
    } else if (bank == 0U) {
        *reg = RK3576_PULL_GPIO0_BH_OFFSET - 0x4U;
    } else if (bank == 1U) {
        *reg = RK3576_PULL_GPIO1_OFFSET;
    } else if (bank == 2U) {
        *reg = RK3576_PULL_GPIO2_OFFSET;
    } else if (bank == 3U) {
        *reg = RK3576_PULL_GPIO3_OFFSET;
    } else if ((bank == 4U) && (pin < RK_PC0)) {
        *reg = RK3576_PULL_GPIO4_AL_OFFSET;
    } else if ((bank == 4U) && (pin < RK_PD0)) {
        *reg = RK3576_PULL_GPIO4_CL_OFFSET - 0x8U;
    } else {
        *reg = RK3576_PULL_GPIO4_DL_OFFSET - 0xcU;
    }

    *reg += (pin / RK3576_PULL_PINS_PER_REG) * sizeof(rt_uint32_t);
    *bit = (pin % RK3576_PULL_PINS_PER_REG) * RK3576_PULL_BITS_PER_PIN;

    return RT_EOK;
}

static rt_err_t rk3576_schmitt_reg_bit(rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t *reg, rt_uint32_t *bit)
{
    if (rk3576_check_pin(bank, pin) != RT_EOK) {
        return -RT_EINVAL;
    }

    if ((bank == 0U) && (pin < RK_PB4)) {
        *reg = RK3576_SMT_GPIO0_AL_OFFSET;
    } else if (bank == 0U) {
        *reg = RK3576_SMT_GPIO0_BH_OFFSET - 0x4U;
    } else if (bank == 1U) {
        *reg = RK3576_SMT_GPIO1_OFFSET;
    } else if (bank == 2U) {
        *reg = RK3576_SMT_GPIO2_OFFSET;
    } else if (bank == 3U) {
        *reg = RK3576_SMT_GPIO3_OFFSET;
    } else if ((bank == 4U) && (pin < RK_PC0)) {
        *reg = RK3576_SMT_GPIO4_AL_OFFSET;
    } else if ((bank == 4U) && (pin < RK_PD0)) {
        *reg = RK3576_SMT_GPIO4_CL_OFFSET - 0x8U;
    } else {
        *reg = RK3576_SMT_GPIO4_DL_OFFSET - 0xcU;
    }

    *reg += (pin / RK3576_SMT_PINS_PER_REG) * sizeof(rt_uint32_t);
    *bit = (pin % RK3576_SMT_PINS_PER_REG) * RK3576_SMT_BITS_PER_PIN;

    return RT_EOK;
}




rt_err_t rk3576_iomux_set(rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t mux)
{
    rt_uint32_t group;
    rt_uint32_t reg;
    rt_uint32_t bit;
    rt_uint32_t mask;
    rt_uint32_t value;

    if ((rk3576_check_pin(bank, pin) != RT_EOK) || (mux > RK3576_IOMUX_MASK)) {
        return -RT_EINVAL;
    }

    group = pin / RK3576_IOMUX_PINS_PER_GROUP;
    reg = rk3576_iomux_offsets[bank][group];
    if ((pin % RK3576_IOMUX_PINS_PER_GROUP) >= RK3576_IOMUX_PINS_PER_REG) {
        reg += sizeof(rt_uint32_t);
    }

    if ((bank == 0U) && (pin >= RK_PB4) && (pin <= RK_PB7)) {
        reg += 0x1ff4U;
    }

    bit   = (pin % RK3576_IOMUX_PINS_PER_REG) * RK3576_IOMUX_BITS_PER_PIN;
    mask  = RK3576_IOMUX_MASK << bit;
    value = (mux & RK3576_IOMUX_MASK) << bit;

    rk3576_ioc_write(reg, RK3576_HIWORD(mask, value));

    return RT_EOK;
}

rt_err_t rk3576_pull_set(rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t pull)
{
    rt_uint32_t reg;
    rt_uint32_t bit;
    rt_uint32_t mask;
    rt_uint32_t value;

    if (rk3576_pull_reg_bit(bank, pin, &reg, &bit) != RT_EOK) {
        return -RT_EINVAL;
    }

    switch (pull) {
    case RK3576_PULL_NONE:
        value = RK3576_PULL_HW_NONE;
        break;
    case RK3576_PULL_UP:
        value = RK3576_PULL_HW_UP;
        break;
    case RK3576_PULL_DOWN:
        value = RK3576_PULL_HW_DOWN;
        break;
    default:
        return -RT_EINVAL;
    }

    mask = ((1U << RK3576_PULL_BITS_PER_PIN) - 1U) << bit;
    rk3576_ioc_write(reg, RK3576_HIWORD(mask, value << bit));

    return RT_EOK;
}

rt_err_t rk3576_drive_set(rt_uint32_t bank, rt_uint32_t pin, rt_uint32_t level)
{
    rt_uint32_t reg;
    rt_uint32_t bit;
    rt_uint32_t mask;
    rt_uint32_t value;

    if ((level > RK3576_DRIVE_LEVEL_MAX) || (rk3576_drive_reg_bit(bank, pin, &reg, &bit) != RT_EOK)) {
        return -RT_EINVAL;
    }

    value = ((level & RK3576_BIT(2)) >> 2) | ((level & RK3576_BIT(0)) << 2) | (level & RK3576_BIT(1));
    mask  = ((1U << RK3576_DRV_BITS_PER_PIN) - 1U) << bit;

    rk3576_ioc_write(reg, RK3576_HIWORD(mask, value << bit));

    return RT_EOK;
}

rt_err_t rk3576_schmitt_set(rt_uint32_t bank, rt_uint32_t pin, rt_bool_t enable)
{
    rt_uint32_t reg;
    rt_uint32_t bit;
    rt_uint32_t mask;
    rt_uint32_t value;

    if (rk3576_schmitt_reg_bit(bank, pin, &reg, &bit) != RT_EOK) {
        return -RT_EINVAL;
    }

    mask  = ((1U << RK3576_SMT_BITS_PER_PIN) - 1U) << bit;
    value = enable ? mask : 0U;

    rk3576_ioc_write(reg, RK3576_HIWORD(mask, value));

    return RT_EOK;
}



/**
 * @brief               串口复用初始化
 * @param id            串口id
 * @param mux_group     串口复用模式
 * @return 
 */
rt_err_t rk3576_uart_iomux_init(rt_uint32_t id, rt_uint32_t mux_group)
{
    const struct rk3576_uart_iomux_group *group;
    rt_err_t ret;

    if ((id >= 12U) || (mux_group >= 4U)) {
        return -RT_EINVAL;
    }

    group = &rk3576_uart_iomux[id][mux_group];
    if (group->valid == 0U) {
        return -RT_EINVAL;
    }

    ret = rk3576_iomux_set(group->rx.bank, group->rx.pin, group->rx.mux);
    if (ret != RT_EOK) {
        return ret;
    }

    ret = rk3576_iomux_set(group->tx.bank, group->tx.pin, group->tx.mux);
    if (ret != RT_EOK) {
        return ret;
    }

    ret = rk3576_pull_set(group->rx.bank, group->rx.pin, RK3576_PULL_UP);
    if (ret != RT_EOK) {
        return ret;
    }

    return rk3576_pull_set(group->tx.bank, group->tx.pin, RK3576_PULL_UP);
}


/**
 * @brief YT8521S 的引脚复用
 * @param  
 * @return 
 */
rt_err_t rk3576_gmac0_iomux_init(void)
{
    static const rt_uint8_t pins[] = {
        RK_PA6, RK_PA5, RK_PA7, RK_PB2, RK_PB1, RK_PB3, RK_PB5,
        RK_PB4, RK_PD1, RK_PB6, RK_PD3, RK_PD2, RK_PC3, RK_PC2,
    };
    rt_size_t i;
    rt_err_t ret;

    for (i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        ret = rk3576_iomux_set(3, pins[i], 3);
        if (ret != RT_EOK) {
            return ret;
        }

        ret = rk3576_pull_set(3, pins[i], RK3576_PULL_NONE);
        if (ret != RT_EOK) {
            return ret;
        }
    }

    /* GPIO0_C2 drives the active-low PHY reset input. */
    return rk3576_iomux_set(0, RK_PC2, 0);
}

