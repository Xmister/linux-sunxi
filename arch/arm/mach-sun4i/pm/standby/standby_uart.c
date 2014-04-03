/*
*********************************************************************************************************
*                                                    MELIS
*                                    the Easy Portable/Player Develop Kits
*                                             	Standby system
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : standby_uart.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-2-22
* Descript: uart operations for standby.
* Update  : date                auther      ver     notes
*           2012-2-22 15:24:27	Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include "standby_i.h"
#include "standby_uart_i.h"
#ifdef STANDBY_PRINT
//use uart0
static standby_uart_t *uart_port = (standby_uart_t *)STANDBY_UART0_REGS_BASE;

/*
*********************************************************************************************************
*                                       STANDBY PUT CHAR
*
* Description: standby put char.
*
* Arguments  : none.
*
* Returns    : none;
*********************************************************************************************************
*/
void standby_putc(char c)
{
	//check fifo not full
	while ((uart_port->usr & TXFIFO_FULL) == 0)
	{
		//fifo is full, wait always
		;
	}
	
	//put a charset
	uart_port->thr = c;
}

/*
*********************************************************************************************************
*                                       STANDBY PUT STRING
*
* Description: standby put string.
*
* Arguments  : none.
*
* Returns    : none;
*********************************************************************************************************
*/
void standby_puts(const char *str)
{
	while(*str != '\0')
	{
		if(*str == '\n')
		{
			// if current character is '\n', insert and output '\r'
			standby_putc('\r');
		}
		standby_putc(*str++);
	}
}
/*
void standby_put_uint(__u32 input)
{
    char stack[11] ;
    int i ;
    int j ;
    __u32 tmpIn = input;
    
    
    if( tmpIn == 0 )
    {
        standby_putc('0');
        return ;
    }
    
    for( i = 0; tmpIn > 0; ++i )
    {
        stack[i] = tmpIn%10 + '0';       // characters in reverse order are put in 'stack' .
        tmpIn /= 10;
    }                                    // at the end of 'for' loop, 'i' is the number of characters.

    for( --i, j = 0; i >= 0; --i, ++j )
    {
        standby_putc(stack[i]);
    }
}
*/
void __standby_put_hex(__u32 input)
{
    char tmpList[20];
    char stack[11] ;
    int i ;
    int j ;
    __u32 tmpIn = input;
    
    for( i = 0; i < 10; i++ )
    {
        tmpList[i] = '0' + i;
    }                                    
    for( i = 10; i < 16; i++ )
    {
        tmpList[i] = 'A' + i - 10;
    }                                   
    
    standby_puts("0x");
    if( tmpIn == 0 )
    {
        standby_puts("00");
        return ;
    }
    
    for( i = 0; tmpIn > 0; ++i )
    {
        stack[i] = tmpList[tmpIn-(tmpIn>>4)*16];       // characters in reverse order are put in 'stack' .
        tmpIn = tmpIn>>4;
    }                                    // at the end of 'for' loop, 'i' is the number of characters.

    for( --i, j = 0; i >= 0; --i, ++j )
    {
        standby_putc(stack[i]);
    }
}

void standby_put_hex(__u32 input)
{
    __standby_put_hex(input);
    standby_puts("\n");
}
__u32 tmptable[] = {1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};
void standby_put_dec(__u32 input)
{
    __u32 div;
    __u32 tmpc;
    __u32 start = 1;
    __u32 i = 0;
    div = tmptable[i];
    while (div > 1)
    {
        tmpc = 0;
        while (input >= div)
        {
            tmpc++;
            input -= div;
        }
        if (start != 1 || tmpc != 0)
        {
            standby_putc('0'+tmpc);
            start = 0;
        }
        div = tmptable[++i];
    }
    standby_putc('0'+input);
    standby_puts("\n");
}

void standby_put_hex_ext(__u32 *input, __u32 size)
{
    __u32 i;
    __u32 j = 0;
    __u8 *tmp;
    tmp = (__u8 *)input;
    for (i = 0; i < size; i++)
    {
        __standby_put_hex(*tmp);
        standby_puts("    ");
        j++;
        if (j == 16)
        {
            standby_puts("\n");
            j = 0;
        }
        tmp++;
    }
    standby_puts("\n");
}

#endif
