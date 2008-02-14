#include <asm/amcc440.h>
#include <asm/io.h>
#include <aros/libcall.h>
#include <stdarg.h>

#include "kernel_intern.h"

struct PrivData {
    struct KernelBase *kbase;
};

void __putc(char c)
{
    if (c == '\n')
    {
        while(!(inb(UART0_LSR) & UART_LSR_TEMT));
        outb('\r', UART0_THR);
    }
    while(!(inb(UART0_LSR) & UART_LSR_TEMT));
    outb(c, UART0_THR);
}

static int krnPutC(int chr, struct PrivData *data)
{
    __putc(chr);

    return 1;
}

AROS_LH2(void, KrnBug,
         AROS_LHA(const char *, format, A0),
         AROS_LHA(va_list, args, A1),
         struct KernelBase *, KernelBase, 11, Kernel)
{
    AROS_LIBFUNC_INIT

    struct PrivData data;
    
    data.kbase = KernelBase;
    __vcformat(&data, krnPutC, format, args);

    AROS_LIBFUNC_EXIT
}
