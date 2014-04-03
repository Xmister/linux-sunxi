/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        newbie Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : standby.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-30 18:34
* Descript: platform standby fucntion.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "standby_i.h"

#define __here__ standby_puts(__FILE__); standby_puts("====");standby_put_dec(__LINE__);standby_puts("\n");
extern unsigned int save_sp(void);
extern void restore_sp(unsigned int sp);
extern void standby_flush_tlb(void);
extern void standby_preload_tlb(void);
extern char *__bss_start;
extern char *__bss_end;
extern char *__standby_start;
extern char *__standby_end;

static __u32 sp_backup;
static void standby(void);
static __u32 dcdc2, dcdc3;
static struct pll_factor_t orig_pll;
static struct pll_factor_t local_pll;

static struct sun4i_clk_div_t  clk_div;
static struct sun4i_clk_div_t  tmp_clk_div;

__u32 standby_ctrl_flag = 0;

/* parameter for standby, it will be transfered from sys_pwm module */
struct aw_pm_info  pm_info;

#define DRAM_BASE_ADDR      0xc0000000
#define DRAM_TRANING_SIZE   (16)
static __u32 dram_traning_area_back[DRAM_TRANING_SIZE];
struct standby_ir_buffer ir_buffer;



/*
*********************************************************************************************************
*                                   STANDBY MAIN PROCESS ENTRY
*
* Description: standby main process entry.
*
* Arguments  : arg  pointer to the parameter that transfered from sys_pwm module.
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/
int main(struct aw_pm_info *arg)
{
    char    *tmpPtr = (char *)&__bss_start;
    unsigned int wake_event = 0;
    
    if(!arg){
        /* standby parameter is invalid */
        return -1;
    }

    standby_ctrl_flag = 0;
    ir_buffer.dcnt = 0;
    /* flush data and instruction tlb, there is 32 items of data tlb and 32 items of instruction tlb,
       The TLB is normally allocated on a rotating basis. The oldest entry is always the next allocated */
    standby_flush_tlb();
    /* preload tlb for standby */
    standby_preload_tlb();

    /* clear bss segment */
    do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);

#if(ALLOW_DISABLE_HOSC)
    standby_ctrl_flag &= ~HOSC_ENABLE_MASK;
#else
    standby_ctrl_flag |= HOSC_ENABLE_MASK;
#endif

    /* copy standby parameter from dram */
    standby_memcpy(&pm_info, arg, sizeof(pm_info));
    /* copy standby code & data to load tlb */
    //standby_memcpy((char *)&__standby_end, (char *)&__standby_start, (char *)&__bss_end - (char *)&__bss_start);
    /* backup dram traning area */
    standby_memcpy((char *)dram_traning_area_back, (char *)DRAM_BASE_ADDR, sizeof(__u32)*DRAM_TRANING_SIZE);

    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    /* init module before dram enter selfrefresh */
    /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    /* initialise standby modules */
    standby_clk_init();
    standby_clk_apbinit();
    standby_int_init();
    standby_tmr_init();
    standby_power_init();
    /* init some system wake source */
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_EXINT){
        standby_enable_int(INT_SOURCE_EXTNMI);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_KEY){
        standby_key_init();
        standby_enable_int(INT_SOURCE_LRADC);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_IR){
        standby_ctrl_flag |= HOSC_ENABLE_MASK;
        standby_ir_init();
        standby_enable_int(INT_SOURCE_IR0);
        standby_enable_int(INT_SOURCE_IR1);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_ALARM){
        //standby_alarm_init();???
        standby_enable_int(INT_SOURCE_ALARM);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_USB){
        standby_ctrl_flag |= HOSC_ENABLE_MASK;
        standby_ctrl_flag |= DRAM_ENABLE_MASK;
        standby_ctrl_flag |= PLL6_ENABLE_MASK;
        standby_ctrl_flag |= PLL1_ENABLE_MASK;
        standby_ctrl_flag = (standby_ctrl_flag&~CPU_SRC_MASK)|CPU_SRC_PLL1;
        standby_usb_init();
        standby_enable_int(INT_SOURCE_USB0);
        standby_enable_int(INT_SOURCE_USB1);
        standby_enable_int(INT_SOURCE_USB2);
        standby_enable_int(INT_SOURCE_USB3);
        standby_enable_int(INT_SOURCE_USB4);
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_TIMEOFF){
        /* set timer for power off */
        if(pm_info.standby_para.time_off) {
            standby_tmr_set(pm_info.standby_para.time_off);
            standby_enable_int(INT_SOURCE_TIMER0);
        }
    }
    if ((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_PLL1) 
    {
        standby_ctrl_flag |= PLL1_ENABLE_MASK;
    }
    if ((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_PLL6) 
    {
        standby_ctrl_flag |= PLL6_ENABLE_MASK;
    }
    if ((standby_ctrl_flag & DRAM_ENABLE_MASK) != 0)
    {
        standby_ctrl_flag |= PLL5_ENABLE_MASK;
    }

    if ((standby_ctrl_flag & DRAM_ENABLE_MASK) == 0)
    {
        /* save stack pointer registger, switch stack to sram */
        sp_backup = save_sp();
        
        /* enable dram enter into self-refresh */
        dram_power_save_process();
    }
    /* process standby */
    standby();

    /* check system wakeup event */
    wake_event |= standby_query_int(INT_SOURCE_EXTNMI)? 0:SUSPEND_WAKEUP_SRC_EXINT;
    wake_event |= standby_query_int(INT_SOURCE_USB0)? 0:SUSPEND_WAKEUP_SRC_USB;
    wake_event |= standby_query_int(INT_SOURCE_USB1)? 0:SUSPEND_WAKEUP_SRC_USB;
    wake_event |= standby_query_int(INT_SOURCE_USB2)? 0:SUSPEND_WAKEUP_SRC_USB;
    wake_event |= standby_query_int(INT_SOURCE_USB3)? 0:SUSPEND_WAKEUP_SRC_USB;
    wake_event |= standby_query_int(INT_SOURCE_USB4)? 0:SUSPEND_WAKEUP_SRC_USB;
    wake_event |= standby_query_int(INT_SOURCE_LRADC)? 0:SUSPEND_WAKEUP_SRC_KEY;
    wake_event |= standby_query_int(INT_SOURCE_IR0)? 0:SUSPEND_WAKEUP_SRC_IR;
    wake_event |= standby_query_int(INT_SOURCE_ALARM)? 0:SUSPEND_WAKEUP_SRC_ALARM;
    wake_event |= standby_query_int(INT_SOURCE_TIMER0)? 0:SUSPEND_WAKEUP_SRC_TIMEOFF;
    standby_put_hex(wake_event);
#if 0 
	change_runtime_env(1);
	io_init(); 
	io_init_high(); 
	delay_ms(10); 
	io_init_low(); 
	delay_ms(20); 
	io_init_high(); 
#endif

    if ((standby_ctrl_flag & DRAM_ENABLE_MASK) == 0)
    {
        /* enable watch-dog to preserve dram training failed */
        standby_tmr_enable_watchdog();
        /* restore dram */
        dram_power_up_process();
        /* disable watch-dog    */
        standby_tmr_disable_watchdog();
        /* restore stack pointer register, switch stack back to dram */
        restore_sp(sp_backup);
    }


    /* exit standby module */
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_USB){
        standby_usb_exit();
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_IR){
        standby_ir_exit();
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_ALARM){
        //standby_alarm_exit();
    }
    if(pm_info.standby_para.event & SUSPEND_WAKEUP_SRC_KEY){
        standby_key_exit();
    }
    standby_power_exit();
    standby_tmr_exit();
    standby_int_exit();
    standby_clk_apbexit();
    standby_clk_exit();

    /* restore dram traning area */
    if ((standby_ctrl_flag & DRAM_ENABLE_MASK) == 0)
    {
        standby_memcpy((char *)DRAM_BASE_ADDR, (char *)dram_traning_area_back, sizeof(__u32)*DRAM_TRANING_SIZE);
    }

    /* report which wake source wakeup system */
    arg->standby_para.event = wake_event;
    arg->standby_para.ir_data_cnt = ir_buffer.dcnt;
    standby_memcpy(arg->standby_para.ir_buffer, ir_buffer.buf, STANDBY_IR_BUF_SIZE);
    return 0;
}


/*
*********************************************************************************************************
*                                     SYSTEM PWM ENTER STANDBY MODE
*
* Description: enter standby mode.
*
* Arguments  : none
*
* Returns    : none;
*********************************************************************************************************
*/
static void standby(void)
{
    if ((standby_ctrl_flag&DRAM_ENABLE_MASK) == 0)
    {
        /* gating off dram clock */
        standby_clk_dramgating(0);
    }

	/* backup cpu freq */
	standby_clk_get_pll_factor(&orig_pll);

	/*lower freq from 1008M to 384M*/
	local_pll.FactorN = 16;
	local_pll.FactorK = 0;
	local_pll.FactorM = 0;
	local_pll.FactorP = 0;
	standby_clk_set_pll_factor(&local_pll);
	change_runtime_env(1);
	delay_ms(10);

    //standby_ctrl_flag = (standby_ctrl_flag&~CPU_SRC_MASK)|CPU_SRC_PLL1;
//    standby_ctrl_flag = (standby_ctrl_flag&~PLL1_ENABLE_MASK);


    standby_put_hex(standby_ctrl_flag);
    if (((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_LOSC)
        || ((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_HOSC))
    {
        /* switch cpu clock to HOSC*/
        standby_clk_core2hosc();
        change_runtime_env(1);
        delay_us(1);
    }
    else
    {
        //lower freq from 384M to 204M
        local_pll.FactorN = 17;
        local_pll.FactorK = 0;
        local_pll.FactorM = 0;
        local_pll.FactorP = 1;
        standby_clk_set_pll_factor(&local_pll);
        change_runtime_env(1);
        delay_ms(10);
        
        //lower freq from 204M to 36M
        local_pll.FactorN = 12;
        local_pll.FactorK = 0;
        local_pll.FactorM = 0;
        local_pll.FactorP = 3;
        standby_clk_set_pll_factor(&local_pll);
        change_runtime_env(1);
        delay_ms(10);
    }
    /*disable pll. cpu src clock may be dll1, so pll disable should later than switch cpu to hosc*/
    standby_clk_plldisable();
    
    /* backup voltages */
    dcdc2 = standby_get_voltage(POWER_VOL_DCDC2);
    dcdc3 = standby_get_voltage(POWER_VOL_DCDC3);
    
    /* adjust voltage */
    standby_set_voltage(POWER_VOL_DCDC3, STANDBY_DCDC3_VOL);
    standby_set_voltage(POWER_VOL_DCDC2, STANDBY_DCDC2_VOL);
    
    /* set clock division cpu:axi:ahb:apb = 2:2:2:1 */
    standby_clk_getdiv(&clk_div);
    tmp_clk_div.axi_div = 0;
    tmp_clk_div.ahb_div = 0;
    tmp_clk_div.apb_div = 0;
    standby_clk_setdiv(&tmp_clk_div);

    /* swtich apb1 to losc */
    if ((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_LOSC)
    {
        standby_clk_apb2losc();
        change_runtime_env(1);
        //delay_ms(1);
        
        /* switch cpu to 32k */
        standby_clk_core2losc();
        if ((standby_ctrl_flag&HOSC_ENABLE_MASK) == 0)
        {
            // disable HOSC, and disable LDO
            standby_clk_hoscdisable();
            standby_clk_ldodisable();
        }
    }

    /* cpu enter sleep, wait wakeup by interrupt */
    asm("WFI");

    if ((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_LOSC)
    {
        if ((standby_ctrl_flag&HOSC_ENABLE_MASK) == 0)
        {
            /* enable LDO, enable HOSC */
            standby_clk_ldoenable();
            /* delay 1ms for power be stable */
            //3ms
            standby_delay_cycle(1);
            standby_clk_hoscenable();
            //3ms
            standby_delay_cycle(1);
        }
        
        /* switch clock to hosc */
        standby_clk_core2hosc();
        
        /* swtich apb1 to hosc */
        standby_clk_apb2hosc();
    }
    
    /* restore clock division */
    standby_clk_setdiv(&clk_div);
    
    /* restore voltage for exit standby */
    standby_set_voltage(POWER_VOL_DCDC2, dcdc2);
    standby_set_voltage(POWER_VOL_DCDC3, dcdc3);
    
    change_runtime_env(1);
    delay_ms(10);
    /* enable pll */
    standby_clk_pllenable();

    if (((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_LOSC)
        || ((standby_ctrl_flag&CPU_SRC_MASK)==CPU_SRC_HOSC))
    {
        __here__
        /* switch cpu clock to core pll */
        standby_clk_core2pll();
        change_runtime_env(1);
        delay_ms(10);
    }
    else
    {
        /*lower freq from 36M to 204M*/
        local_pll.FactorN = 17;
        local_pll.FactorK = 0;
        local_pll.FactorM = 0;
        local_pll.FactorP = 1;
        standby_clk_set_pll_factor(&local_pll);
        change_runtime_env(1);
        delay_ms(10);

    	/*lower freq from 204M to 384M*/
    	local_pll.FactorN = 16;
    	local_pll.FactorK = 0;
    	local_pll.FactorM = 0;
    	local_pll.FactorP = 0;
    	standby_clk_set_pll_factor(&local_pll);
    	change_runtime_env(1);
    	delay_ms(10);
    }

    /*restore freq from 384 to 1008M*/
	standby_clk_set_pll_factor(&orig_pll);
	change_runtime_env(1);
	delay_ms(5);
	
    /* gating on dram clock */
    if ((standby_ctrl_flag&DRAM_ENABLE_MASK) == 0)
    {
        standby_clk_dramgating(1);
    }

    return;
}

