/*
   This include file is used for compiling host-side parts of hardware emulation modules
   on hosted AROS. These functions reside in host-side kernel module.
   
   This API is experimental and subject to change.
*/

#ifndef _AROS_KERNEL_HOST_H
#define _AROS_KERNEL_HOST_H

#ifdef _WIN32
#define IMPORT __declspec(dllimport)
#else
#define IMPORT
#endif

long IMPORT KrnAllocIRQ(void);
void IMPORT KrnFreeIRQ(unsigned char irq);
void *IMPORT KrnGetIRQObject(unsigned char irq);
unsigned long IMPORT KrnCauseIRQ(unsigned char irq);

#endif
