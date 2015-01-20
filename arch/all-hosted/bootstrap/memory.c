/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>

/*
 * Allocate memory for kickstart's .code and .rodata.
 * At this point it needs to be writable because we will load
 * kickstart files into it afterwards.
 */
void *AllocateRO(size_t len)
{
    return malloc(len);
}

/*
 * Commit executable and read-only state for kickstart's .code
 */
int SetRO(void *addr, size_t len)
{
    /* 0 means success */
    return 0;
}

/*
 * Allocate simple read-write memory for kickstart's .data and .bss.
 */
void *AllocateRW(size_t len)
{
    return malloc(len);
}

/*
 * Allocate read-write-execute area to be used as AROS RAM
 */
void *AllocateRAM32(size_t len)
{
    return malloc(len);
}

/*
 * Allocate read-write-execute area to be used as AROS RAM
 */
void *AllocateRAM(size_t len)
{
    return malloc(len);
}
