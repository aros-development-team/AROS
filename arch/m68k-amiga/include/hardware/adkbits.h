#ifndef	HARDWARE_ADKBITS_H
#define	HARDWARE_ADKBITS_H
/*
    Copyright © 2022, The AROS Development Team. All rights reserved.
*/

#define ADKB_USE0V1     0
#define ADKF_USE0V1     (1 << ADKB_USE0V1)
#define ADKB_USE1V2     1
#define ADKF_USE1V2     (1 << ADKB_USE1V2)
#define ADKB_USE2V3     2
#define ADKF_USE2V3     (1 << ADKB_USE2V3)
#define ADKB_USE3VN     3
#define ADKF_USE3VN     (1 << ADKB_USE3VN)
#define ADKB_USE0P1     4
#define ADKF_USE0P1     (1 << ADKB_USE0P1)
#define ADKB_USE1P2     5
#define ADKF_USE1P2     (1 << ADKB_USE1P2)
#define ADKB_USE2P3     6
#define ADKF_USE2P3     (1 << ADKB_USE2P3)
#define ADKB_USE3PN     7
#define ADKF_USE3PN     (1 << ADKB_USE3PN)
#define ADKB_FAST       8
#define ADKF_FAST       (1 << ADKB_FAST)
#define ADKB_MSBSYNC    9
#define ADKF_MSBSYNC    (1 << ADKB_MSBSYNC)
#define ADKB_WORDSYNC   10
#define ADKF_WORDSYNC   (1 << ADKB_WORDSYNC)
#define ADKB_UARTBRK    11
#define ADKF_UARTBRK    (1 << ADKB_UARTBRK)
#define ADKB_MFMPREC    12
#define ADKF_MFMPREC    (1 << ADKB_MFMPREC)
#define ADKB_PRECOMP0   13
#define ADKF_PRECOMP0   (1 << ADKB_PRECOMP0)
#define ADKB_PRECOMP1   14
#define ADKF_PRECOMP1   (1 << ADKB_PRECOMP1)
#define ADKB_SETCLR     15
#define ADKF_SETCLR     (1 << ADKB_SETCLR)

#define ADKF_PRE000NS   0
#define ADKF_PRE140NS   (ADKF_PRECOMP0)
#define ADKF_PRE280NS   (ADKF_PRECOMP1)
#define ADKF_PRE560NS   (ADKF_PRECOMP0 | ADKF_PRECOMP1)

#endif
