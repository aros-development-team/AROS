/*
 * bootstrap.h
 *
 *  Created on: Dec 1, 2009
 *      Author: misc
 */

#ifndef BOOTSTRAP_H_
#define BOOTSTRAP_H_

#include <aros/kernel.h>
#include <sys/types.h>

#if DEBUG
#define D(x) x
#else
#define D(x) /* eps */
#endif

#define BOOT_STACK_SIZE 65536
#define BOOT_TMP_SIZE	 524288

#define rdcr(reg) \
    ({ long val; asm volatile("mov %%" #reg ",%0":"=r"(val)); val; })

#define wrcr(reg, val) \
    do { asm volatile("mov %0,%%" #reg::"r"(val)); } while(0)

#define _STR(x) # x
#define STR(x) _STR(x)

#define MAX_BSS_SECTIONS	256

void mem_free();
size_t mem_avail();
size_t mem_used();

#endif /* BOOTSTRAP_H_ */
