#ifndef IDENDIFY_INTERN_H
#define IDENDIFY_INTERN_H

/*
 * Copyright (c) 2010-2011 Matthias Rustler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *   
 * $Id$
 */

#include <exec/types.h>
#include <exec/libraries.h>

#include <proto/exec.h>

#define STRBUFSIZE (30)

struct FuncNode
{
    struct Node nd;
    ULONG offset;
};

struct LibNode
{
    struct Node nd;
    struct List funcList;
};

struct HardwareBuffer
{
    TEXT buf_OsVer[STRBUFSIZE];
    TEXT buf_ExecVer[STRBUFSIZE];
    TEXT buf_WbVer[STRBUFSIZE];
    TEXT buf_RomSize[STRBUFSIZE];
    TEXT buf_ChipRAM[STRBUFSIZE];
    TEXT buf_FastRAM[STRBUFSIZE];
    TEXT buf_RAM[STRBUFSIZE];
    TEXT buf_SetPatchVer[STRBUFSIZE];
    TEXT buf_VMChipRAM[STRBUFSIZE];
    TEXT buf_VMFastRAM[STRBUFSIZE];
    TEXT buf_VMRAM[STRBUFSIZE];
    TEXT buf_PlainChipRAM[STRBUFSIZE];
    TEXT buf_PlainFastRAM[STRBUFSIZE];
    TEXT buf_PlainRAM[STRBUFSIZE];
    TEXT buf_VBR[STRBUFSIZE]; // not cached
    TEXT buf_LastAlert[STRBUFSIZE]; // not cached
    TEXT buf_VBlankFreq[STRBUFSIZE];
    TEXT buf_PowerFreq[STRBUFSIZE];
    TEXT buf_EClock[STRBUFSIZE];
    TEXT buf_SlowRAM[STRBUFSIZE];
    TEXT buf_PPCClock[STRBUFSIZE];
    TEXT buf_CPU[STRBUFSIZE];
    TEXT buf_CPURev[STRBUFSIZE];
    TEXT buf_CPUClock[STRBUFSIZE];
    TEXT buf_FPUClock[STRBUFSIZE];
    TEXT buf_RAMAccess[STRBUFSIZE];
    TEXT buf_RAMWidth[STRBUFSIZE];
    TEXT buf_RAMBandwidth[STRBUFSIZE];
    TEXT buf_DeniseRev[STRBUFSIZE];
    TEXT buf_BoingBag[STRBUFSIZE];
    TEXT buf_XLVersion[STRBUFSIZE];
    TEXT buf_HostOS[STRBUFSIZE];
    TEXT buf_HostVers[STRBUFSIZE];
    TEXT buf_HostMachine[STRBUFSIZE];
    TEXT buf_HostCPU[STRBUFSIZE];
    TEXT buf_HostSpeed[STRBUFSIZE];
};

struct IdentifyBaseIntern
{
    struct Library base;

    struct SignalSemaphore sem;
    APTR poolMem;
    BOOL dirtyflag;
    struct HardwareBuffer hwb;
    struct List libList;
};

#endif
