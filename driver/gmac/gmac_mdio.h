/*
 * DWC GMAC MDIO controller interfaces.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __GMAC_MDIO_H__
#define __GMAC_MDIO_H__

#include <rtthread.h>

struct rk3576_gmac;

int gmac_mdio_read(struct rk3576_gmac *gmac, rt_uint32_t phy, rt_uint32_t reg);
rt_err_t gmac_mdio_write(struct rk3576_gmac *gmac, rt_uint32_t phy, rt_uint32_t reg, rt_uint16_t data);

#endif /* __GMAC_MDIO_H__ */
