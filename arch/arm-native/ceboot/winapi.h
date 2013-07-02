/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Windows CE API functions missing in mingw32ce headers
    Lang: english
*/

/*
 * These definitions are taken from HaRET project
 * (http://htc-linux.org/wiki/index.php?title=HaRET)
 */
#define GETGXINFO         0x00020000
#define GETRAWFRAMEBUFFER 0x00020001

typedef struct _RawFrameBufferInfo
{
    WORD  wFormat;
    WORD  wBPP;
    VOID *pFramePointer;
    int   cxStride;
    int   cyStride;
    int   cxPixels;
    int   cyPixels;
} RawFrameBufferInfo;

typedef struct
{
    long          Version;
    void         *pvFrameBuffer;
    unsigned long cbStride;
    unsigned long cxWidth;
    unsigned long cyHeight;
    unsigned long cBPP;
    unsigned long ffFormat;
    char          Unused[0x84-7*4];
} GXDeviceInfo;

/* Known screen formats */
#define FORMAT_565   1
#define FORMAT_555   2
#define FORMAT_OTHER 3

void *AllocPhysMem(DWORD, DWORD, DWORD, DWORD, PULONG);
BOOL SetKMode(BOOL);
