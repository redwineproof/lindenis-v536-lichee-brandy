#ifndef __SUNXI_CFG_H__
#define __SUNXI_CFG_H__
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <config.h>

/*dtb addr*/
#define CONFIG_DTB_STORE_IN_DRAM_BASE		(CONFIG_SYS_TEXT_BASE + (2 * 1024 * 1024))
/*soccfg addr*/
#define CONFIG_SOCCFG_STORE_IN_DRAM_BASE	(CONFIG_DTB_STORE_IN_DRAM_BASE + (1 * 1024 * 1024))
/*doardcfg addr*/
#define CONFIG_BDCFG_STORE_IN_DRAM_BASE		(CONFIG_SOCCFG_STORE_IN_DRAM_BASE + (512 * 1024))

#ifdef USE_BOARD_CONFIG
#define BD_SCRIPT	0
#define SOC_SCRIPT	1
#else
#define BD_SCRIPT	1
#define SOC_SCRIPT	0
#endif

#endif /*__SUNXI_CFG_H__*/
