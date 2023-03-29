/*
    Copyright (C) 2023, The AROS Development Team. All rights reserved.
*/

#ifndef SERIALPC_IO_H
#define SERIALPC_IO_H

#include <asm/io.h>

static inline void serial_out(struct HIDDSerialUnitData * data,
                              int offset,
                              int value)
{
  outb(value, data->baseaddr+offset);
}

static inline void serial_outp(struct HIDDSerialUnitData * data,
                               int offset,
                               int value)
{
  outb_p(value, data->baseaddr+offset);
}

static inline unsigned int serial_in(struct HIDDSerialUnitData * data,
                                     int offset)
{
  return inb(data->baseaddr+offset);
}

static inline unsigned int serial_inp(struct HIDDSerialUnitData * data,
                                      int offset)
{
  return inb_p(data->baseaddr+offset);
}

#endif /* SERIALPC_IO_H */