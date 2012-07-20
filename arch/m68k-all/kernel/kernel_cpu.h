/*
 * M68K CPU-specific definitions.
 *
 */

#ifndef KERNEL_CPU_H_
#define KERNEL_CPU_H_

#include <proto/exec.h>

#include "cpu_m68k.h"
 
/* User/supervisor mode switching */
#define cpumode_t __unused int

/* Only used on protected memory systems. On the
 * m68k-amiga, these are unneeded.
 */
#define goSuper() (0)
#define goUser()  do { } while (0)
#define goBack(mode)  do { } while (0)

#endif /* _KERNEL_CPU_H */
