/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef VC_FB_H
#define VC_FB_H

#include <inttypes.h>

extern int vcfb_init(void);

#if !defined(KRN_Dummy)
#define KRN_Dummy               (TAG_USER + 0x03d00000)
#endif
#if !defined(KRN_FuncPutC)
#define KRN_FuncPutC      (KRN_Dummy + 99) /* RAW FrameBuffer descriptor */
#endif

/* temp location to use for our vc messages */
#define MESSAGE_BUFFER	0x1000

#endif	/* VC_FB_H */
