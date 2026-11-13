#include <exec/interrupts.h>

void cpu_Trap(struct ExceptionContext *regs, unsigned long error_code, unsigned long irq_number);
