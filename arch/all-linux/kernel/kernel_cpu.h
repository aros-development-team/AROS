#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

#include <sigcore.h>

/* Some common definitions */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

void krnSysCall(unsigned char n);

#endif
