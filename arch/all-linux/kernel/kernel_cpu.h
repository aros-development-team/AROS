#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

#include <sigcore.h>

/* Number of exceptions supported by the CPU. Needed by kernel_base.h */
#define EXCEPTIONS_COUNT 1

/* Some common definitions */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)

#endif