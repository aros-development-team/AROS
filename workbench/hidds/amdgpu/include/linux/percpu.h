/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _LINUX_PERCPU_H_
#define _LINUX_PERCPU_H_
#define DEFINE_PER_CPU(type, name)      __typeof__(type) name
#define DECLARE_PER_CPU(type, name)     extern __typeof__(type) name
#define __this_cpu_read(pcp)            (pcp)
#define __this_cpu_write(pcp, val)      ((pcp) = (val))
#define __this_cpu_inc_return(pcp)      (++(pcp))
#define __this_cpu_dec_return(pcp)      (--(pcp))
#define __this_cpu_inc(pcp)             ((pcp)++)
#define __this_cpu_dec(pcp)             ((pcp)--)

#endif
