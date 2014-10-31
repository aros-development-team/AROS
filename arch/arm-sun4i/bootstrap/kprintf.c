/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <inttypes.h>
#include <stdio.h>

#define UART0_RBR (*(volatile uint32_t *)0x01c28000)
#define UART0_THR (*(volatile uint32_t *)0x01c28000)
#define UART0_DLL (*(volatile uint32_t *)0x01c28000)
#define UART0_DLH (*(volatile uint32_t *)0x01c28004)
#define UART0_IER (*(volatile uint32_t *)0x01c28004)
#define UART0_IIR (*(volatile uint32_t *)0x01c28008)
#define UART0_FCR (*(volatile uint32_t *)0x01c28008)
#define UART0_LCR (*(volatile uint32_t *)0x01c2800c)
#define UART0_MCR (*(volatile uint32_t *)0x01c28010)
#define UART0_LSR (*(volatile uint32_t *)0x01c28014)
#define UART0_MSR (*(volatile uint32_t *)0x01c28018)
#define UART0_SCH (*(volatile uint32_t *)0x01c2801c)
#define UART0_USR (*(volatile uint32_t *)0x01c2807c)
#define UART0_TFL (*(volatile uint32_t *)0x01c28080)
#define UART0_RFL (*(volatile uint32_t *)0x01c28084)
#define UART0_HLT (*(volatile uint32_t *)0x01c280a4)

static inline void kprintf_out(char chr) {
    while ((UART0_LSR & (1<<6)) == 0);
    UART0_THR = chr;
}

static void kprintf_uint32_d(uint32_t value) {

	if(value == 0) {
        kprintf_out('0');
	} else {

        char d[10];
        uint32_t index = 0;

        do {	
            d[index++] = value%10 + '0';
            value /= 10;
        } while(value);

        do {
            kprintf_out((uint32_t)d[--index]);
        } while(index);
    }

}

static void kprintf_uint32_x(uint32_t value) {

    kprintf_out('0');
    kprintf_out('x');

    uint32_t index;
    static char base[] = "0123456789abcdef";

    for(index = 0; index < 8; index++ ) {
        kprintf_out(base[((value & 0xf0000000)>>28)]);
        value <<= 4;
    }

}

void kprintf(const char *format, ...) {

    va_list vp;
    va_start(vp, format);

    uint32_t t;

    while(*format) {
        if( *format == '%' ) {
            format++;
            switch(*format) {
                case 'u':
                case 'd':
                    t = va_arg(vp, uint32_t);
                    kprintf_uint32_d(t);
                    format++;
                break;
                case 'x':
                    t = va_arg(vp, uint32_t);
                    kprintf_uint32_x(t);
                    format++;
                break;
            }
        } else {
            kprintf_out(*format++);
        }
    }

    va_end(vp);
}

