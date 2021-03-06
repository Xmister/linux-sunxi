/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2007-2012 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __MALI_KERNEL_GP2_H__
#define __MALI_KERNEL_GP2_H__

extern struct mali_kernel_subsystem mali_subsystem_gp2;

#if USING_MALI_PMM
_mali_osk_errcode_t maligp_signal_power_up( mali_bool queue_only );
_mali_osk_errcode_t maligp_signal_power_down( mali_bool immediate_only );
#endif

#endif /* __MALI_KERNEL_GP2_H__ */
