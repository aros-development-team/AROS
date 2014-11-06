/*
    Copyright © 2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <hardware/sun4i/uart.h>

#include <stdio.h>

static inline void kprintf_out(char chr) {
    while ((UART0_LSR & ((1<<6)|(1<<5))) == 0);
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

static void kprintf_int32_d(int32_t value) {

    if(value<0) {
        kprintf_out('-');
        kprintf_uint32_d(-value);
    } else {
        kprintf_uint32_d(value);
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
                    t = va_arg(vp, uint32_t);
                    kprintf_uint32_d(t);
                    format++;
                break;
                case 'd':
                    t = va_arg(vp, int32_t);
                    kprintf_int32_d(t);
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

