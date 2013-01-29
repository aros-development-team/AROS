/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef VCMBOX_H
#define VCMBOX_H

#define VCMB_BASE               (BCM_PHYSBASE + 0x00B880)

#define VCMB_CHAN_MAX           8
#define VCMB_CHAN_MASK          0xF

/* Mailbox register offsets */
#define VCMB_READ               0
#define VCMB_POLL               16
#define VCMB_SENDER             20
#define VCMB_STATUS             24
#define VCMB_CONFIG             28
#define VCMB_WRITE              32

/* Flags for VCMB_STATUS */
#define VCMB_STATUS_READREADY   (1 << 30)
#define VCMB_STATUS_WRITEREADY  (1 << 31)

/* Tags used to commmunicate with VideoCore */
#define VCTAG_REQ	        0x00000000
#define VCTAG_RESP	        0x80000000

#define VCTAG_GETFWREV          0x00000001
#define VCTAG_GETBOARD          0x00010001
#define VCTAG_GETBOARDREV       0x00010002
#define VCTAG_GETBOARDMAC       0x00010003
#define VCTAG_GETBOARDSERIAL    0x00010004
#define VCTAG_GETARMRAM         0x00010005
#define VCTAG_GETVCRAM          0x00010006
#define VCTAG_GETCLOCKS         0x00010007

#define VCTAG_GETPOWER          0x00020001
#define VCTAG_GETTIMING         0x00020002
#define VCTAG_SETPOWER	        0x00028001

#define VCTAG_GETCLKSTATE       0x00030001
#define VCTAG_GETCLKRATE        0x00030002
#define VCTAG_GETVOLTAGE        0x00030003
#define VCTAG_GETCLKMAX         0x00030004
#define VCTAG_GETVOLTMAX        0x00030005
#define VCTAG_GETTEMP           0x00030006
#define VCTAG_GETCLKMIN         0x00030007
#define VCTAG_GETVOLTMIN        0x00030008
#define VCTAG_GETTURBO          0x00030009
#define VCTAG_GETTEMPMAX        0x0003000A

#define VCTAG_ALLOCMEM          0x0003000C
#define VCTAG_LOCKMEM           0x0003000D
#define VCTAG_UNLOCKMEM         0x0003000E
#define VCTAG_FREEMEM           0x0003000F

#define VCTAG_EXECUTE           0x00030010

#define VCTAG_GETEDID           0x00030020

#define VCTAG_SETCLKSTATE       0x00038001
#define VCTAG_SETCLKRATE        0x00038002
#define VCTAG_SETVOLTAGE        0x00038003
#define VCTAG_SETTURBO          0x00038009

#define VCTAG_FBALLOC	        0x00040001
#define VCTAG_SCRBLANK          0x00040002
#define VCTAG_GETRES	        0x00040003
#define VCTAG_GETVRES           0x00040004
#define VCTAG_GETDEPTH          0x00040005
#define VCTAG_GETPIXFMT         0x00040006
#define VCTAG_GETALPHAMODE      0x00040007
#define VCTAG_GETPITCH          0x00040008
#define VCTAG_GETVOFFSET        0x00040009
#define VCTAG_GETOVERSCAN       0x0004000A
#define VCTAG_GETPALETTE        0x0004000B

#define VCTAG_TESTRES	        0x00044003
#define VCTAG_TESTVRES          0x00044004
#define VCTAG_TESTDEPTH         0x00044005
#define VCTAG_TESTPIXFMT        0x00044006
#define VCTAG_TESTALPHAMODE     0x00044007
#define VCTAG_TESTVOFFSET       0x00044009
#define VCTAG_TESTOVERSCAN      0x0004400A
#define VCTAG_TESTPALETTE       0x0004400B

#define VCTAG_FBFREE            0x00048001
#define VCTAG_SETRES            0x00048003
#define VCTAG_SETVRES           0x00048004
#define VCTAG_SETDEPTH          0x00048005
#define VCTAG_SETPIXFMT         0x00048006
#define VCTAG_SETALPHAMODE      0x00048007
#define VCTAG_SETVOFFSET        0x00048009
#define VCTAG_SETOVERSCAN       0x0004800A
#define VCTAG_SETPALETTE        0x0004800B

#define VCTAG_GETCMDLINE        0x00050001

#define VCTAG_GETDMACHAN        0x00060001

/* GPU Memory allocation flags */
#define VCMEM_DISCARDABLE       (1 << 0)        // can be released at any time
#define VCMEM_NORMAL            (0 << 2)        // normal allocating alias. Don't use from ARM
#define VCMEM_DIRECT            (1 << 2)        // 0xC alias. Uncached
#define VCMEM_COHERENT          (2 << 2)        // 0x8 alias. Non-allocating in L2 but coherent
#define VCMEM_NONALLOCATING     (VCMEM_DIRECT | VCMEM_COHERENT) // Allocating in L2
#define VCMEM_ZERO              (1 << 4)        // zero buffer
#define VCMEM_NOINIT            (1 << 5)        // don't initialise (default initialises to all ones)
#define VCMEM_LAZYLOCK          (1 << 6)        // can be locked for extended periods


#endif	/* VCMBOX_H */
