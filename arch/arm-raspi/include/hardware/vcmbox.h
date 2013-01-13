/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef VCMBOX_H
#define VCMBOX_H

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

#endif	/* VCMBOX_H */
