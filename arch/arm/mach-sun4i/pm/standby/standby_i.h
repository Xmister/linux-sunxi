/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby_i.h
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 17:21
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __STANDBY_I_H__
#define __STANDBY_I_H__

#include <linux/power/aw_pm.h>
#include <mach/platform.h>

#include "standby_cfg.h"
#include "common.h"
#include "standby_clock.h"
#include "standby_key.h"
#include "standby_power.h"
#include "standby_usb.h"
#include "standby_twi.h"
#include "standby_ir.h"
#include "standby_tmr.h"
#include "standby_int.h"
#include "standby_uart.h"
#include "../pm.h"
extern struct aw_pm_info  pm_info;

#define HOSC_ENABLE_MASK 0x1
#define DRAM_ENABLE_MASK 0x2
#define PLL1_ENABLE_MASK 0x4
#define PLL2_ENABLE_MASK 0x8
#define PLL3_ENABLE_MASK 0x10
#define PLL4_ENABLE_MASK 0x20
#define PLL5_ENABLE_MASK 0x40
#define PLL6_ENABLE_MASK 0x80
#define PLL7_ENABLE_MASK 0x100
#define CPU_SRC_MASK 0x30000



#define CPU_SRC_LOSC 0x00000
#define CPU_SRC_HOSC 0x10000
#define CPU_SRC_PLL1 0x20000
#define CPU_SRC_PLL6 0x30000


extern struct aw_pm_info  pm_info;
extern __u32 standby_ctrl_flag;


#endif  //__STANDBY_I_H__

