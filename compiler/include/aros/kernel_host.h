#ifndef _AROS_KERNEL_HOST_H
#define _AROS_KERNEL_HOST_H

#ifdef _WIN32
#define IMPORT __declspec(dllimport)
#else
#define IMPORT
#endif

unsigned long IMPORT CauseException(unsigned char irq);

#endif
