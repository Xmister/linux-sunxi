/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : pm.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-27 14:08
* Descript: power manager for allwinners chips platform.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <linux/power/aw_pm.h>
#include <asm/mach/map.h>
#include <asm/cacheflush.h>
#include <mach/sys_config.h>
#include <mach/system.h>
#include "pm_debug.h"

#define AW_PM_DBG   1
#undef PM_DBG
#if(AW_PM_DBG)
    #define PM_DBG(format,args...)   printk("[pm]"format,##args)
#else
    #define PM_DBG(format,args...)   do{}while(0)
#endif

/* define major number for power manager */
#define AW_PMU_MAJOR    267


extern char *standby_bin_start;
extern char *standby_bin_end;

static struct aw_pm_info standby_info = {
    .standby_para = {
        .event = SUSPEND_WAKEUP_SRC_EXINT,
    },
    .pmu_arg = {
        .twi_port = 0,
        .dev_addr = 10,
    },
};
struct standby_output_t standby_output;
EXPORT_SYMBOL(standby_output);
unsigned int wakeup_src_ctrl = 0;
EXPORT_SYMBOL(wakeup_src_ctrl);


/*
*********************************************************************************************************
*                           aw_pm_valid
*
*Description: determine if given system sleep state is supported by the platform;
*
*Arguments  : state     suspend state;
*
*Return     : if the state is valid, return 1, else return 0;
*
*Notes      : this is a call-back function, registered into PM core;
*
*********************************************************************************************************
*/
extern bool console_suspend_enabled;

static int aw_pm_valid(suspend_state_t state)
{
    PM_DBG("valid\n");

    console_suspend_enabled = 0;
    if(!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))){
        PM_DBG("state (%d) invalid!\n", state);
        return 0;
    }

    return 1;
}


/*
*********************************************************************************************************
*                           aw_pm_begin
*
*Description: Initialise a transition to given system sleep state;
*
*Arguments  : state     suspend state;
*
*Return     : return 0 for process successed;
*
*Notes      : this is a call-back function, registered into PM core, and this function
*             will be called before devices suspened;
*********************************************************************************************************
*/
int aw_pm_begin(suspend_state_t state)
{
    PM_DBG("%d state begin\n", state);
/*must init perfcounter, because delay_us and delay_ms is depandant perf counter*/
#ifndef GET_CYCLE_CNT
		backup_perfcounter();
		init_perfcounters (1, 0);
#endif

	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare
*
*Description: Prepare the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core, this function
*             will be called after devices suspended, and before device late suspend
*             call-back functions;
*********************************************************************************************************
*/
int aw_pm_prepare(void)
{
    PM_DBG("prepare\n");

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare_late
*
*Description: Finish preparing the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core.
*             prepare_late is called before disabling nonboot CPUs and after
*              device drivers' late suspend callbacks have been executed;
*********************************************************************************************************
*/
int aw_pm_prepare_late(void)
{
    PM_DBG("prepare_late\n");

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_enter
*
*Description: Enter the system sleep state;
*
*Arguments  : state     system sleep state;
*
*Return     : return 0 is process successed;
*
*Notes      : this function is the core function for platform sleep.
*********************************************************************************************************
*/
static int aw_pm_enter(suspend_state_t state)
{
    int (*standby)(struct aw_pm_info *arg) = (int (*)(struct aw_pm_info *arg))SRAM_FUNC_START;

    PM_DBG("enter state %d\n", state);

    //move standby code to sram
    memcpy((void *)SRAM_FUNC_START, (void *)&standby_bin_start, (int)&standby_bin_end - (int)&standby_bin_start);

    /* config system wakeup evetn type */
    standby_info.standby_para.event = SUSPEND_WAKEUP_SRC_EXINT | SUSPEND_WAKEUP_SRC_ALARM;
    standby_info.standby_para.event |= wakeup_src_ctrl;
    wakeup_src_ctrl = 0;

    /*FIXME: cannot wakeup */
    /* goto sram and run */
    standby(&standby_info);
    standby_output.event = standby_info.standby_para.event;
    standby_output.ir_data_cnt = standby_info.standby_para.ir_data_cnt;
    if (standby_output.ir_data_cnt != 0)
    {
        memcpy(standby_output.ir_buffer, standby_info.standby_para.ir_buffer, STANDBY_IR_BUF_SIZE);
    }
    standby_info.standby_para.ir_data_cnt = 0;
    PM_DBG("wakeup event %x\n", standby_output.event);
    PM_DBG("standby_output.ir_data_cnt %x\n", standby_output.ir_data_cnt);

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_wake
*
*Description: platform wakeup;
*
*Arguments  : none;
*
*Return     : none;
*
*Notes      : This function called when the system has just left a sleep state, right after
*             the nonboot CPUs have been enabled and before device drivers' early resume
*             callbacks are executed. This function is opposited to the aw_pm_prepare_late;
*********************************************************************************************************
*/
static void aw_pm_wake(void)
{
    PM_DBG("platform wakeup, wakesource is:0x%x\n", standby_info.standby_para.event);
}


/*
*********************************************************************************************************
*                           aw_pm_finish
*
*Description: Finish wake-up of the platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called right prior to calling device drivers' regular suspend
*              callbacks. This function is opposited to the aw_pm_prepare function.
*********************************************************************************************************
*/
void aw_pm_finish(void)
{
    PM_DBG("platform wakeup finish\n");
}


/*
*********************************************************************************************************
*                           aw_pm_end
*
*Description: Notify the platform that system is in work mode now.
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called by the PM core right after resuming devices, to indicate to
*             the platform that the system has returned to the working state or
*             the transition to the sleep state has been aborted. This function is opposited to
*             aw_pm_begin function.
*********************************************************************************************************
*/
void aw_pm_end(void)
{
#ifndef GET_CYCLE_CNT
	#ifndef IO_MEASURE
			restore_perfcounter();
	#endif
#endif
    PM_DBG("aw_pm_end!\n");
}


/*
*********************************************************************************************************
*                           aw_pm_recover
*
*Description: Recover platform from a suspend failure;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function alled by the PM core if the suspending of devices fails.
*             This callback is optional and should only be implemented by platforms
*             which require special recovery actions in that situation.
*********************************************************************************************************
*/
void aw_pm_recover(void)
{
    PM_DBG("aw_pm_recover\n");
}


/*
    define platform_suspend_ops which is registered into PM core.
*/
static struct platform_suspend_ops aw_pm_ops = {
    .valid = aw_pm_valid,
    .begin = aw_pm_begin,
    .prepare = aw_pm_prepare,
    .prepare_late = aw_pm_prepare_late,
    .enter = aw_pm_enter,
    .wake = aw_pm_wake,
    .finish = aw_pm_finish,
    .end = aw_pm_end,
    .recover = aw_pm_recover,
};


/*
*********************************************************************************************************
*                           aw_pm_init
*
*Description: initial pm sub-system for platform;
*
*Arguments  : none;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init aw_pm_init(void)
{
    PM_DBG("aw_pm_init!\n");
    suspend_set_ops(&aw_pm_ops);

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_exit
*
*Description: exit pm sub-system on platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      :
*
*********************************************************************************************************
*/
static void __exit aw_pm_exit(void)
{
    PM_DBG("aw_pm_exit!\n");
    suspend_set_ops(NULL);
}

module_init(aw_pm_init);
module_exit(aw_pm_exit);

