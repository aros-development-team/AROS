/* First include CPU-dependant definitions */

#ifdef __i386__
#include "cpu_i386.h"
#endif

/* This macro serves as an indicator of included file */
#ifndef PRINT_CPUCONTEXT
#error Unsupported CPU
#endif

/* Some common definitions */
#define cpumode_t __unused char

#define goSuper() 0
#define goUser()
#define goBack(mode)
