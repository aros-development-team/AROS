/*
    Copyright (C) 2013-2026, The AROS Development Team. All rights reserved.
*/

#ifndef VC_MB_H
#define VC_MB_H

#include <stdint.h>

#define VCMB_PROPCHAN           8

extern volatile unsigned int *vcmb_read(intptr_t, unsigned int chan);
extern void vcmb_write(intptr_t mb, unsigned int chan, void *msg);

/* TRUE when real VideoCore firmware answers the mailbox, FALSE under emulation. */
extern int vcmb_firmware_present(uintptr_t mb, volatile unsigned int *msg);

#endif	/* VC_MB_H */
