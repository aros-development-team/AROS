#ifndef CPU_H
#define CPU_H

#include <exec/types.h>

#define VECTOR_RESET               0x00
#define VECTOR_UNDEFINED           0x04
#define VECTOR_SWI                 0x08
#define VECTOR_PREFETCH_ABORT      0x0c
#define VECTOR_DATA_ABORT          0x10
#define VECTOR_IRQ                 0x18
#define VECTOR_FIQ                 0x1c

#define INSTALL_IRQ_HANDLER(vectoraddress, routineaddress) \
	*(ULONG *)vectoraddress = (((ULONG)routineaddress - vectoraddress - 8) >> 2) | \
	                          0xea000000

#define MODE_USER                  0x10
#define MODE_FIQ                   0x11
#define MODE_IRQ                   0x12
#define MODE_SVC                   0x13
#define MODE_ABORT                 0x17
#define MODE_UNDEF                 0x1b
#define MODE_SYSTEM                0x1f

#endif