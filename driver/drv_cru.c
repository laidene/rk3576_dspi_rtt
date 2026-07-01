/*
 * 目前还在开发
 *      1 当前PLL的时钟是直接利用uboot配置之后的
 *      1 当前只支持串口的时钟配置
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <rthw.h>
#include <rtthread.h>

#include <board.h>
#include <drv_cru.h>

#define RK3576_BIT(n)                    (1U << (n))
#define HIWORD_UPDATE(mask, value)       ((((mask) & 0xffffU) << 16) | ((value) & 0xffffU))

#define UART_FRAC_NUM_SHIFT              16
#define UART_FRAC_NUM_MASK               (0xffffU << UART_FRAC_NUM_SHIFT)
#define UART_FRAC_DEN_SHIFT              0
#define UART_FRAC_DEN_MASK               (0xffffU << UART_FRAC_DEN_SHIFT)

#define UART_FRAC_SRC_SEL_SHIFT          0
#define UART_FRAC_SRC_SEL_MASK           (0x3U << UART_FRAC_SRC_SEL_SHIFT)
#define UART_FRAC_SRC_SEL_GPLL           0U
#define UART_FRAC_SRC_SEL_CPLL           1U
#define UART_FRAC_SRC_SEL_AUPLL          2U
#define UART_FRAC_SRC_SEL_OSC            3U

#define UART_SEL_SHIFT                   8
#define UART_SEL_MASK                    (0x7U << UART_SEL_SHIFT)
#define UART_SEL_GPLL                    0U
#define UART_SEL_CPLL                    1U
#define UART_SEL_AUPLL                   2U
#define UART_SEL_OSC                     3U
#define UART_SEL_FRAC0                   4U
#define UART_SEL_FRAC1                   5U
#define UART_SEL_FRAC2                   6U

#define UART_DIV_SHIFT                   0
#define UART_DIV_MASK                    (0xffU << UART_DIV_SHIFT)

#define UART1_SRC_SEL_SHIFT              13
#define UART1_SRC_SEL_MASK               (0x7U << UART1_SRC_SEL_SHIFT)
#define UART1_SRC_DIV_SHIFT              5
#define UART1_SRC_DIV_MASK               (0xffU << UART1_SRC_DIV_SHIFT)

#define UART1_SEL_SHIFT                  0
#define UART1_SEL_MASK                   (0x1U << UART1_SEL_SHIFT)
#define UART1_SEL_TOP                    0U
#define UART1_SEL_OSC                    1U

struct rk3576_gate_desc {
    rt_uint32_t offset;
    rt_uint8_t bit;
};

struct rk3576_uart_clk_desc {
    rt_uint8_t              id;
    rt_uint32_t             clksel;
    struct rk3576_gate_desc pclk;
    struct rk3576_gate_desc sclk;
};

struct rk3576_uart_frac_desc {
    rt_uint32_t div;
    rt_uint32_t sel;
};

/**
 * @brief 12个串口的寄存器
 */
static const struct rk3576_uart_clk_desc uart_clks[] = {
    { 0,  RK3576_CLKSEL_CON(60), { RK3576_CLKGATE_CON(13), 10 }, { RK3576_CLKGATE_CON(14), 5  } },
    { 2,  RK3576_CLKSEL_CON(61), { RK3576_CLKGATE_CON(13), 11 }, { RK3576_CLKGATE_CON(14), 6  } },
    { 3,  RK3576_CLKSEL_CON(62), { RK3576_CLKGATE_CON(13), 12 }, { RK3576_CLKGATE_CON(14), 9  } },
    { 4,  RK3576_CLKSEL_CON(63), { RK3576_CLKGATE_CON(13), 13 }, { RK3576_CLKGATE_CON(14), 12 } },
    { 5,  RK3576_CLKSEL_CON(64), { RK3576_CLKGATE_CON(13), 14 }, { RK3576_CLKGATE_CON(14), 15 } },
    { 6,  RK3576_CLKSEL_CON(65), { RK3576_CLKGATE_CON(13), 15 }, { RK3576_CLKGATE_CON(15), 2  } },
    { 7,  RK3576_CLKSEL_CON(66), { RK3576_CLKGATE_CON(14), 0  }, { RK3576_CLKGATE_CON(15), 5  } },
    { 8,  RK3576_CLKSEL_CON(67), { RK3576_CLKGATE_CON(14), 1  }, { RK3576_CLKGATE_CON(15), 8  } },
    { 9,  RK3576_CLKSEL_CON(68), { RK3576_CLKGATE_CON(14), 2  }, { RK3576_CLKGATE_CON(15), 9  } },
    { 10, RK3576_CLKSEL_CON(69), { RK3576_CLKGATE_CON(14), 3  }, { RK3576_CLKGATE_CON(15), 10 } },
    { 11, RK3576_CLKSEL_CON(70), { RK3576_CLKGATE_CON(14), 4  }, { RK3576_CLKGATE_CON(15), 11 } },
};

/**
 * @brief 3个串口分数时钟寄存器
 */
static const struct rk3576_uart_frac_desc uart_fracs[] = {
    { RK3576_CLKSEL_CON(21), RK3576_CLKSEL_CON(22) },
    { RK3576_CLKSEL_CON(23), RK3576_CLKSEL_CON(24) },
    { RK3576_CLKSEL_CON(25), RK3576_CLKSEL_CON(26) },
};

static rt_uint32_t rk3576_cru_parent_rate(rt_uint32_t sel)
{
    switch (sel) {
    case UART_FRAC_SRC_SEL_GPLL:
        return RK3576_GPLL_HZ;
    case UART_FRAC_SRC_SEL_CPLL:
        return RK3576_CPLL_HZ;
    case UART_FRAC_SRC_SEL_AUPLL:
        return RK3576_AUPLL_HZ;
    case UART_FRAC_SRC_SEL_OSC:
    default:
        return RK3576_OSC_HZ;
    }
}

static const struct rk3576_uart_clk_desc *rk3576_uart_desc(rt_uint32_t id)
{
    rt_size_t i;

    for (i = 0; i < sizeof(uart_clks) / sizeof(uart_clks[0]); i++) {
        if (uart_clks[i].id == id) {
            return &uart_clks[i];
        }
    }

    return RT_NULL;
}

rt_uint32_t rk3576_cru_read(rt_uint32_t offset)
{
    return HWREG32(CRU_MMIO_BASE + offset);
}

void rk3576_cru_write(rt_uint32_t offset, rt_uint32_t value)
{
    HWREG32(CRU_MMIO_BASE + offset) = value;
}

void rk3576_cru_hiword_update(rt_uint32_t offset, rt_uint32_t mask, rt_uint32_t value)
{
    rk3576_cru_write(offset, HIWORD_UPDATE(mask, value));
}

void rk3576_cru_gate_enable(rt_uint32_t offset, rt_uint32_t bit)
{
    rk3576_cru_hiword_update(offset, RK3576_BIT(bit), 0);
}

void rk3576_cru_gate_disable(rt_uint32_t offset, rt_uint32_t bit)
{
    rk3576_cru_hiword_update(offset, RK3576_BIT(bit), RK3576_BIT(bit));
}

void rk3576_cru_reset_assert(rt_uint32_t offset, rt_uint32_t bit)
{
    rk3576_cru_hiword_update(offset, RK3576_BIT(bit), RK3576_BIT(bit));
}

void rk3576_cru_reset_deassert(rt_uint32_t offset, rt_uint32_t bit)
{
    rk3576_cru_hiword_update(offset, RK3576_BIT(bit), 0);
}

/**
 * @brief 给 uart_frac_x 设置父时钟和分子分母
 * @param frac_id 
 * @param parent_sel 
 * @param numerator 
 * @param denominator 
 */
static void rk3576_uart_frac_set(rt_uint32_t frac_id,
                                 rt_uint32_t parent_sel,
                                 rt_uint32_t numerator,
                                 rt_uint32_t denominator)
{
    rt_uint32_t val;

    if (frac_id >= sizeof(uart_fracs) / sizeof(uart_fracs[0])) {
        return;
    }

    val = ((numerator   << UART_FRAC_NUM_SHIFT) & UART_FRAC_NUM_MASK) |
          ((denominator << UART_FRAC_DEN_SHIFT) & UART_FRAC_DEN_MASK);

    rk3576_cru_write(uart_fracs[frac_id].div, val);
    rk3576_cru_hiword_update(uart_fracs[frac_id].sel, UART_FRAC_SRC_SEL_MASK, parent_sel << UART_FRAC_SRC_SEL_SHIFT);
    rk3576_cru_gate_enable(RK3576_CLKGATE_CON(2), 5 + frac_id);
}

static rt_uint32_t rk3576_uart_frac_get(rt_uint32_t frac_id)
{
    rt_uint32_t con;
    rt_uint32_t div;
    rt_uint32_t parent;
    rt_uint32_t numerator;
    rt_uint32_t denominator;
    rt_uint32_t sel;

    if (frac_id >= sizeof(uart_fracs) / sizeof(uart_fracs[0])) {
        return 0;
    }

    con    = rk3576_cru_read(uart_fracs[frac_id].sel);
    sel    = (con & UART_FRAC_SRC_SEL_MASK) >> UART_FRAC_SRC_SEL_SHIFT;
    parent = rk3576_cru_parent_rate(sel);

    div         = rk3576_cru_read(uart_fracs[frac_id].div);
    numerator   = (div & UART_FRAC_NUM_MASK) >> UART_FRAC_NUM_SHIFT;
    denominator = (div & UART_FRAC_DEN_MASK) >> UART_FRAC_DEN_SHIFT;

    if (denominator == 0) {
        return 0;
    }

    return (rt_uint32_t)(((rt_uint64_t)parent * numerator) / denominator);
}






void rk3576_cru_init(void)
{
    /* 3个串口分数时钟 */
    /* uart_frac_0 = GPLL * 1536 / 24750 = 73.728 MHz */
    /* uart_frac_1 = CPLL *   12 /   125 = 96     MHz */
    /* uart_frac_2 = CPLL *   16 /   125 = 128    MHz */
    rk3576_uart_frac_set(0, UART_FRAC_SRC_SEL_GPLL, 1536, 24750);
    rk3576_uart_frac_set(1, UART_FRAC_SRC_SEL_CPLL,   12,   125);
    rk3576_uart_frac_set(2, UART_FRAC_SRC_SEL_CPLL,   16,   125);
}

rt_err_t rk3576_cru_uart_enable(rt_uint32_t id)
{
    const struct rk3576_uart_clk_desc *desc;

    if (id == 1) {
        rk3576_cru_gate_enable(RK3576_CLKGATE_CON(2), 13);
        rk3576_cru_gate_enable(RK3576_PMU_CLKGATE_CON(5), 5);
        rk3576_cru_gate_enable(RK3576_PMU_CLKGATE_CON(5), 6);
        return RT_EOK;
    }

    desc = rk3576_uart_desc(id);
    if (desc == RT_NULL) {
        return -RT_EINVAL;
    }

    rk3576_cru_gate_enable(desc->pclk.offset, desc->pclk.bit);
    rk3576_cru_gate_enable(desc->sclk.offset, desc->sclk.bit);

    return RT_EOK;
}

rt_uint32_t rk3576_cru_get_uart_clk(rt_uint32_t id)
{
    const struct rk3576_uart_clk_desc *desc;
    rt_uint32_t con;
    rt_uint32_t src;
    rt_uint32_t div;
    rt_uint32_t parent;

    if (id == 1) {
        con = rk3576_cru_read(RK3576_PMU_CLKSEL_CON(8));
        if (((con & UART1_SEL_MASK) >> UART1_SEL_SHIFT) == UART1_SEL_OSC) {
            return RK3576_OSC_HZ;
        }

        con = rk3576_cru_read(RK3576_CLKSEL_CON(27));
        src = (con & UART1_SRC_SEL_MASK) >> UART1_SRC_SEL_SHIFT;
        div = (con & UART1_SRC_DIV_MASK) >> UART1_SRC_DIV_SHIFT;
    } else {
        desc = rk3576_uart_desc(id);
        if (desc == RT_NULL) {
            return 0;
        }

        con = rk3576_cru_read(desc->clksel);
        src = (con & UART_SEL_MASK) >> UART_SEL_SHIFT;
        div = (con & UART_DIV_MASK) >> UART_DIV_SHIFT;
    }

    switch (src) {
    case UART_SEL_GPLL:
        parent = RK3576_GPLL_HZ;
        break;
    case UART_SEL_CPLL:
        parent = RK3576_CPLL_HZ;
        break;
    case UART_SEL_AUPLL:
        parent = RK3576_AUPLL_HZ;
        break;
    case UART_SEL_FRAC0:
        parent = rk3576_uart_frac_get(0);
        break;
    case UART_SEL_FRAC1:
        parent = rk3576_uart_frac_get(1);
        break;
    case UART_SEL_FRAC2:
        parent = rk3576_uart_frac_get(2);
        break;
    case UART_SEL_OSC:
    default:
        parent = RK3576_OSC_HZ;
        break;
    }

    return parent / (div + 1);
}

rt_uint32_t rk3576_cru_set_uart_clk(rt_uint32_t id, rt_uint32_t rate)
{
    const struct rk3576_uart_clk_desc *desc;
    rt_uint32_t src = UART_SEL_OSC;
    rt_uint32_t parent = RK3576_OSC_HZ;
    rt_uint32_t div;
    rt_uint32_t value;

    if (rate == 0) {
        return 0;
    }

    if ((RK3576_GPLL_HZ % rate) == 0) {
        src = UART_SEL_GPLL;
        parent = RK3576_GPLL_HZ;
    } else if ((RK3576_CPLL_HZ % rate) == 0) {
        src = UART_SEL_CPLL;
        parent = RK3576_CPLL_HZ;
    } else if ((rk3576_uart_frac_get(0) != 0) && ((rk3576_uart_frac_get(0) % rate) == 0)) {
        src = UART_SEL_FRAC0;
        parent = rk3576_uart_frac_get(0);
    } else if ((rk3576_uart_frac_get(1) != 0) && ((rk3576_uart_frac_get(1) % rate) == 0)) {
        src = UART_SEL_FRAC1;
        parent = rk3576_uart_frac_get(1);
    } else if ((rk3576_uart_frac_get(2) != 0) && ((rk3576_uart_frac_get(2) % rate) == 0)) {
        src = UART_SEL_FRAC2;
        parent = rk3576_uart_frac_get(2);
    } else if ((RK3576_OSC_HZ % rate) == 0) {
        src = UART_SEL_OSC;
        parent = RK3576_OSC_HZ;
    } else {
        src = UART_SEL_OSC;
        parent = RK3576_OSC_HZ;
    }

    div = (parent + rate - 1) / rate;
    if (div == 0) {
        div = 1;
    } else if (div > 256) {
        div = 256;
    }

    if (id == 1) {
        if (src == UART_SEL_OSC && div == 1) {
            rk3576_cru_hiword_update(RK3576_PMU_CLKSEL_CON(8), UART1_SEL_MASK, UART1_SEL_OSC << UART1_SEL_SHIFT);
        } else {
            value = (src << UART1_SRC_SEL_SHIFT) | ((div - 1) << UART1_SRC_DIV_SHIFT);
            rk3576_cru_hiword_update(RK3576_CLKSEL_CON(27), UART1_SRC_SEL_MASK | UART1_SRC_DIV_MASK, value);
            rk3576_cru_hiword_update(RK3576_PMU_CLKSEL_CON(8), UART1_SEL_MASK, UART1_SEL_TOP << UART1_SEL_SHIFT);
        }

        return rk3576_cru_get_uart_clk(id);
    }

    desc = rk3576_uart_desc(id);
    if (desc == RT_NULL) {
        return 0;
    }

    value = (src << UART_SEL_SHIFT) | ((div - 1) << UART_DIV_SHIFT);
    rk3576_cru_hiword_update(desc->clksel, UART_SEL_MASK | UART_DIV_MASK, value);

    return rk3576_cru_get_uart_clk(id);
}
