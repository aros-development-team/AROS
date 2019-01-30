/*
    Copyright � 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef VC_MB_H
#define VC_MB_H

#define VCMB_PROPCHAN           8

extern volatile unsigned int *vcmb_read(void *mb, unsigned int chan);
extern void vcmb_write(void *mb, unsigned int chan, void *msg);

#endif	/* VC_MB_H */
