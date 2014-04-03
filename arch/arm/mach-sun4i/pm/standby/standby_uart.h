/*
*********************************************************************************************************
*                                                    MELIS
*                                    the Easy Portable/Player Develop Kits
*                                             	Standby system
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : standby_uart.h
* By      : Sunny
* Version : v1.0
* Date    : 2012-2-22
* Descript: uart operations for standby.
* Update  : date                auther      ver     notes
*           2012-2-22 15:24:27	Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#ifndef __STANDBY_UART_H__
#define __STANDBY_UART_H__
//#define STANDBY_PRINT 1
#ifdef STANDBY_PRINT
void standby_putc(char c);
void standby_puts(const char *str);
void standby_put_uint(__u32 input);
void standby_put_hex(__u32 input);
void standby_puts(const char *str);
void standby_put_dec(__u32 input);
void standby_put_hex_ext(__u32 *input, __u32 size);
#else
#define standby_putc(c) do{}while(0)
#define standby_puts(str) do{}while(0)
#define standby_put_uint(input) do{}while(0)
#define standby_put_hex(input) do{}while(0)
#define standby_puts(str) do{}while(0)
#define standby_put_dec(str) do{}while(0)
#define standby_put_hex_ext(input, size) do{}while(0)
#endif
#endif   //__STANDBY_UART_H__

