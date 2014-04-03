/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: usb_hcd_servers.c
*
* Author 		: javen
*
* Description 	: USB 主机控制器驱动服务函数集
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-4-14            1.0          create this file
*
*************************************************************************************
*/
#include  "../include/sw_usb_config.h"
#include  "usb_hcd_servers.h"

/*
*******************************************************************************
*                     sw_usb_disable_hcd
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int sw_usb_disable_hcd(__u32 usbc_no)
{
	if(usbc_no == 0){
		sw_usb_disable_hcd0();
	}else if(usbc_no == 1){
		sw_usb_disable_hcd1();
	}else if(usbc_no == 2){
		sw_usb_disable_hcd2();
	}else{
		DMSG_PANIC("ERR: unkown usbc_no(%d)\n", usbc_no);
		return -1;
	}

    return 0;
}
EXPORT_SYMBOL(sw_usb_disable_hcd);

/*
*******************************************************************************
*                     sw_usb_enable_hcd
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
int sw_usb_enable_hcd(__u32 usbc_no)
{
	if(usbc_no == 0){
		sw_usb_enable_hcd0();
	}else if(usbc_no == 1){
		sw_usb_enable_hcd1();
	}else if(usbc_no == 2){
		sw_usb_enable_hcd2();
	}else{
		DMSG_PANIC("ERR: unkown usbc_no(%d)\n", usbc_no);
		return -1;
	}

    return 0;
}
EXPORT_SYMBOL(sw_usb_enable_hcd);





