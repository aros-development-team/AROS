/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#ifndef PARALLELPC_IO_H
#define PARALLELPC_IO_H

#include <asm/io.h>

static inline void parallel_out(struct HIDDParallelUnitData * data,
                                int offset,
                                int value)
{
        outb(value, data->baseaddr+offset);
}

static inline unsigned int parallel_in(struct HIDDParallelUnitData * data,
                                       int offset)
{
        return inb(data->baseaddr+offset);
}

#define parallel_usleep(x) __asm__ __volatile__(__SLOW_DOWN_IO::)

#endif /* PARALLELPC_IO_H */