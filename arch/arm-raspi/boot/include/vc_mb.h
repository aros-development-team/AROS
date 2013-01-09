/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef VC_MB_H
#define VC_MB_H

#define VCMB_CHANS      8
#define VCMB_CHANMASK   0xF

#define VCMB_BASE       0x2000B880
#define VCMB_READ       0
#define VCMB_POLL       16
#define VCMB_SENDER     20
#define VCMB_STATUS     24
#define VCMB_CONFIG     28
#define VCMB_WRITE      32

#define VCMB_STATUS_READREADY   (1 << 30)
#define VCMB_STATUS_WRITEREADY  (1 << 31)

extern volatile unsigned int *vcmb_read(void *mb, unsigned int chan);
extern void vcmb_write(void *mb, unsigned int chan, void *msg);

#endif	/* VC_MB_H */
