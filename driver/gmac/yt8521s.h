/*
 * Motorcomm YT8521S PHY interfaces.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __YT8521S_H__
#define __YT8521S_H__

#include <rtthread.h>

#define YT8521S_PHY_ADDR 0U

struct rk3576_gmac;

rt_err_t yt8521s_init(struct rk3576_gmac *gmac);
rt_err_t yt8521s_get_link(struct rk3576_gmac *gmac, rt_bool_t *up, rt_uint32_t *speed, rt_bool_t *full_duplex);
int yt8521s_read_status(struct rk3576_gmac *gmac);

#endif /* __YT8521S_H__ */
