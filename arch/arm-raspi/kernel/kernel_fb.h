/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#if !defined(KRN_Dummy)
#define KRN_Dummy               (TAG_USER + 0x03d00000)
#endif
#if !defined(KRN_FuncPutC)
#define KRN_FuncPutC      (KRN_Dummy + 99) /* RAW FrameBuffer descriptor */
#endif

extern void (*_KrnPutC)(char *);