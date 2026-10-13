/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: EFS client - the big-endian images of the protocol: header,
          BSTR-style names, FileInfoBlock and ExAllData records
          (re/spec/efs-protocol.md §2.1, §2.6).
*/

#include <proto/exec.h>
#include <string.h>

#include "efs_intern.h"

void PutL(UBYTE *p, ULONG off, ULONG v)
{
    p[off] = v >> 24; p[off + 1] = v >> 16; p[off + 2] = v >> 8; p[off + 3] = v;
}

ULONG GetL(const UBYTE *p, ULONG off)
{
    return ((ULONG)p[off] << 24) | ((ULONG)p[off + 1] << 16) | ((ULONG)p[off + 2] << 8) | p[off + 3];
}

void PutW(UBYTE *p, ULONG off, UWORD v)
{
    p[off] = v >> 8; p[off + 1] = v;
}

UWORD GetW(const UBYTE *p, ULONG off)
{
    return ((UWORD)p[off] << 8) | p[off + 1];
}

ULONG Align4(ULONG off)
{
    return (off + 3) & ~3UL;
}

void HdrSet(struct Globals *glob, UBYTE *buf, ULONG action, ULONG cookie, ULONG a1, ULONG a2, ULONG a3, ULONG a4, ULONG a5, ULONG a6)
{
    PutL(buf, WH_MOUNTID, glob->MountID);
    PutL(buf, WH_COOKIE, cookie);
    PutL(buf, WH_ACTION, action);
    PutL(buf, WH_RES1, 0);
    PutL(buf, WH_RES2, 0);
    PutL(buf, WH_ARG1, a1);
    PutL(buf, WH_ARG2, a2);
    PutL(buf, WH_ARG3, a3);
    PutL(buf, WH_ARG4, a4);
    PutL(buf, WH_ARG5, a5);
    PutL(buf, WH_ARG6, a6);
    PutL(buf, 44, 0);
    PutL(buf, 48, 0);
}

/* BSTR-style name at off; returns the offset after it */
ULONG PutBName(UBYTE *buf, ULONG off, CONST_STRPTR s)
{
    ULONG len = strlen(s);
    if (len > 255)
        len = 255;
    if (off + 1 + len > EFS_BUFSIZE)
        len = EFS_BUFSIZE - off - 1;
    buf[off] = len;
    memcpy(buf + off + 1, s, len);
    return off + 1 + len;
}

ULONG PutCName(UBYTE *buf, ULONG off, CONST_STRPTR s)
{
    ULONG len = strlen(s);
    if (off + len + 1 > EFS_BUFSIZE)
        len = EFS_BUFSIZE - off - 1;
    memcpy(buf + off, s, len);
    buf[off + len] = '\0';
    return off + len + 1;
}

void CopyStr(char *dst, ULONG size, CONST_STRPTR src)
{
    ULONG len = src ? strlen(src) : 0;
    if (len > size - 1)
        len = size - 1;
    if (len)
        memcpy(dst, src, len);
    dst[len] = '\0';
}

/* a BSTR packet argument as a C string */
void GetBstrArg(struct Globals *glob, BSTR b, char *dst, ULONG size)
{
    ULONG len, i;

    if (!b)
    {
        dst[0] = '\0';
        return;
    }
    len = AROS_BSTR_strlen(b);
    if (len > size - 1)
        len = size - 1;
    for (i = 0; i < len; i++)
        dst[i] = AROS_BSTR_getchar(b, i);
    dst[len] = '\0';
}

/* names travel relative to the lock: everything up to the first ':' goes */
CONST_STRPTR StripDevice(CONST_STRPTR name)
{
    CONST_STRPTR colon = strchr(name, ':');
    return colon ? colon + 1 : name;
}

/* fill a BSTR/CSTR field of a FileInfoBlock as this AROS build expects it */
void SetBString(UBYTE *dst, ULONG max, const UBYTE *src, ULONG len)
{
    /* handlers return FIB names and comments as BCPL strings; dos.library
     * converts them to C strings for the caller */
    if (len > max - 2)
        len = max - 2;
    dst[0] = len;
    memcpy(dst + 1, src, len);
    dst[len + 1] = '\0';
}

/* the 260-byte m68k FileInfoBlock image -> this system's FileInfoBlock */
void WireToFib(struct Globals *glob, const UBYTE *w, struct FileInfoBlock *fib)
{
    ULONG nlen = w[8], clen = w[144];

    if (nlen > 107) nlen = 107;
    if (clen > 79)  clen = 79;
    memset(fib, 0, sizeof(*fib));
    fib->fib_DiskKey = GetL(w, 0);
    fib->fib_DirEntryType = (LONG)GetL(w, 4);
    SetBString(fib->fib_FileName, sizeof(fib->fib_FileName), w + 9, nlen);
    fib->fib_Protection = (LONG)GetL(w, 116);
    fib->fib_EntryType = (LONG)GetL(w, 120);
    fib->fib_Size = GetL(w, 124);
    fib->fib_NumBlocks = GetL(w, 128);
    fib->fib_Date.ds_Days = GetL(w, 132);
    fib->fib_Date.ds_Minute = GetL(w, 136);
    fib->fib_Date.ds_Tick = GetL(w, 140);
    SetBString(fib->fib_Comment, sizeof(fib->fib_Comment), w + 145, clen);
    fib->fib_OwnerUID = GetW(w, 224);
    fib->fib_OwnerGID = GetW(w, 226);
}

/* an ExAllData record of a directory block -> FileInfoBlock (§2.6, client conversion) */
void ExRecToFib(struct Globals *glob, const UBYTE *block, ULONG off, ULONG type, struct FileInfoBlock *fib)
{
    ULONG nameoff = GetL(block, off + 4), comoff;
    CONST_STRPTR name = (nameoff && nameoff < EFS_SCANBLOCK) ? (CONST_STRPTR)block + nameoff : (CONST_STRPTR)"";
    ULONG size;

    memset(fib, 0, sizeof(*fib));
    SetBString(fib->fib_FileName, sizeof(fib->fib_FileName), (const UBYTE *)name, strnlen(name, EFS_SCANBLOCK - nameoff));
    if (type >= 2)
    {
        fib->fib_DirEntryType = (LONG)GetL(block, off + 8);
        fib->fib_EntryType = fib->fib_DirEntryType;
    }
    if (type >= 3)
    {
        size = GetL(block, off + 12);
        fib->fib_Size = size;
        fib->fib_NumBlocks = ((size + 511) >> 9) + 1;
    }
    if (type >= 4)
        fib->fib_Protection = (LONG)GetL(block, off + 16);
    if (type >= 5)
    {
        fib->fib_Date.ds_Days = GetL(block, off + 20);
        fib->fib_Date.ds_Minute = GetL(block, off + 24);
        fib->fib_Date.ds_Tick = GetL(block, off + 28);
    }
    if (type >= 6)
    {
        comoff = GetL(block, off + 32);
        if (comoff && comoff < EFS_SCANBLOCK)
            SetBString(fib->fib_Comment, sizeof(fib->fib_Comment), block + comoff, strnlen((CONST_STRPTR)block + comoff, EFS_SCANBLOCK - comoff));
    }
    if (type >= 7)
    {
        fib->fib_OwnerUID = GetW(block, off + 36);
        fib->fib_OwnerGID = GetW(block, off + 38);
    }
}
