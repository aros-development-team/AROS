/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: filesystem.service - the wire images. Everything on the wire is
          the big-endian 32-bit m68k memory image of the DOS structures
          (re/spec/efs-protocol.md §0, §1.5, §2.6, §2.7, §2.10); on AROS
          the native structures differ in size, so every field is placed
          by hand.
*/

#include <proto/exec.h>
#include <string.h>

#include "fs_intern.h"

ULONG EfsGet32(const UBYTE *p)
{
    return ((ULONG)p[0] << 24) | ((ULONG)p[1] << 16) | ((ULONG)p[2] << 8) | p[3];
}

void EfsPut32(UBYTE *p, ULONG v)
{
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}

UWORD EfsGet16(const UBYTE *p)
{
    return ((UWORD)p[0] << 8) | p[1];
}

void EfsPut16(UBYTE *p, UWORD v)
{
    p[0] = v >> 8; p[1] = v;
}

/* C string into a fixed, zero-filled field */
void EfsPutCStr(UBYTE *dst, ULONG size, CONST_STRPTR s)
{
    ULONG n = s ? strlen(s) : 0;

    if (n > size - 1)
        n = size - 1;
    memset(dst, 0, size);
    if (n)
        CopyMem((APTR)s, dst, n);
}

/* BSTR-style string (length byte + characters) out of a request */
BOOL EfsGetBSTR(const UBYTE *buf, ULONG buflen, ULONG off, STRPTR dst, ULONG size)
{
    ULONG n;

    dst[0] = '\0';
    if (off >= buflen)
        return FALSE;
    n = buf[off];
    if (off + 1 + n > buflen)
        n = buflen - off - 1;
    if (n > size - 1)
        n = size - 1;
    CopyMem((APTR)(buf + off + 1), dst, n);
    dst[n] = '\0';
    return TRUE;
}

/* NUL-terminated string out of a request */
BOOL EfsGetCStr(const UBYTE *buf, ULONG buflen, ULONG off, STRPTR dst, ULONG size)
{
    ULONG n = 0;

    dst[0] = '\0';
    if (off >= buflen)
        return FALSE;
    while (off + n < buflen && n < size - 1 && buf[off + n])
    {
        dst[n] = buf[off + n];
        n++;
    }
    dst[n] = '\0';
    return TRUE;
}

static void PutBSTRField(UBYTE *dst, ULONG size, CONST_STRPTR s)
{
    ULONG n = s ? strlen(s) : 0;

    if (n > size - 1)
        n = size - 1;
    memset(dst, 0, size);
    dst[0] = n;
    if (n)
        CopyMem((APTR)s, dst + 1, n);
}

/* 260-byte FileInfoBlock (§2.6) */
void EfsPutFIB(UBYTE *dst, const struct FileInfoBlock *fib)
{
    memset(dst, 0, 260);
    EfsPut32(dst + 0, (ULONG)fib->fib_DiskKey);
    EfsPut32(dst + 4, (ULONG)fib->fib_DirEntryType);
    PutBSTRField(dst + 8, 108, fib->fib_FileName);
    EfsPut32(dst + 116, (ULONG)fib->fib_Protection);
    EfsPut32(dst + 120, (ULONG)fib->fib_EntryType);
    EfsPut32(dst + 124, fib->fib_Size > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (ULONG)fib->fib_Size);
    EfsPut32(dst + 128, fib->fib_NumBlocks > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (ULONG)fib->fib_NumBlocks);
    EfsPut32(dst + 132, fib->fib_Date.ds_Days);
    EfsPut32(dst + 136, fib->fib_Date.ds_Minute);
    EfsPut32(dst + 140, fib->fib_Date.ds_Tick);
    PutBSTRField(dst + 144, 80, fib->fib_Comment);
    EfsPut16(dst + 224, fib->fib_OwnerUID);
    EfsPut16(dst + 226, fib->fib_OwnerGID);
}

/* 36-byte InfoData (§2.7) */
void EfsPutInfoData(UBYTE *dst, const struct InfoData *id)
{
    EfsPut32(dst + 0, id->id_NumSoftErrors);
    EfsPut32(dst + 4, id->id_UnitNumber);
    EfsPut32(dst + 8, id->id_DiskState);
    EfsPut32(dst + 12, id->id_NumBlocks > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (ULONG)id->id_NumBlocks);
    EfsPut32(dst + 16, id->id_NumBlocksUsed > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (ULONG)id->id_NumBlocksUsed);
    EfsPut32(dst + 20, id->id_BytesPerBlock);
    EfsPut32(dst + 24, id->id_DiskType);
    EfsPut32(dst + 28, 0);                          /* a server-side BPTR means nothing remotely */
    EfsPut32(dst + 32, id->id_InUse ? 0xFFFFFFFFUL : 0);
}

static const ULONG exallfixed[8] = { 0, 8, 12, 16, 20, 32, 36, 40 };

/*
 * ExAllData records as the protocol wants them (§2.6): the fixed fields
 * for the type, name and comment strings after them, every pointer an
 * offset from the start of the block. Returns the bytes used.
 */
ULONG EfsPutExAll(UBYTE *dst, ULONG size, const struct ExAllData *ed, LONG type, ULONG *count)
{
    ULONG pos = 0, fixed, n = 0;
    UBYTE *prev = NULL;

    if (type < 1 || type > 7)
        type = 7;
    fixed = exallfixed[type];
    while (ed)
    {
        ULONG namelen = ed->ed_Name ? strlen(ed->ed_Name) + 1 : 1;
        ULONG commlen = (type >= ED_COMMENT && ed->ed_Comment) ? strlen(ed->ed_Comment) + 1 : 1;
        ULONG need = fixed + namelen + (type >= ED_COMMENT ? commlen : 0);
        UBYTE *rec = dst + pos;

        if (pos + need > size)
            break;
        memset(rec, 0, fixed);
        if (prev)
            EfsPut32(prev, pos);                    /* ed_Next of the previous record */
        EfsPut32(rec + 4, pos + fixed);             /* ed_Name */
        if (ed->ed_Name)
            CopyMem(ed->ed_Name, rec + fixed, namelen);
        else
            rec[fixed] = '\0';
        if (type >= ED_TYPE)
            EfsPut32(rec + 8, (ULONG)ed->ed_Type);
        if (type >= ED_SIZE)
            EfsPut32(rec + 12, ed->ed_Size > 0xFFFFFFFFULL ? 0xFFFFFFFFUL : (ULONG)ed->ed_Size);
        if (type >= ED_PROTECTION)
            EfsPut32(rec + 16, ed->ed_Prot);
        if (type >= ED_DATE)
        {
            EfsPut32(rec + 20, ed->ed_Days);
            EfsPut32(rec + 24, ed->ed_Mins);
            EfsPut32(rec + 28, ed->ed_Ticks);
        }
        if (type >= ED_COMMENT)
        {
            EfsPut32(rec + 32, pos + fixed + namelen);
            if (ed->ed_Comment)
                CopyMem(ed->ed_Comment, rec + fixed + namelen, commlen);
            else
                rec[fixed + namelen] = '\0';
        }
        if (type >= ED_OWNER)
        {
            EfsPut16(rec + 36, ed->ed_OwnerUID);
            EfsPut16(rec + 38, ed->ed_OwnerGID);
        }
        prev = rec;
        n++;
        pos = (pos + need + 3) & ~3;
        ed = ed->ed_Next;
    }
    if (prev)
        EfsPut32(prev, 0);                          /* last record */
    if (count)
        *count = n;
    return pos;
}

/* 40-byte UserInfo / GroupInfo (§2.10) */
void EfsPutUserInfo(UBYTE *dst, const struct UserInfo *ui)
{
    EfsPutCStr(dst, 32, ui->ui_UserName);
    EfsPut16(dst + 32, ui->ui_UserID);
    EfsPut16(dst + 34, ui->ui_PrimaryGroupID);
    EfsPut32(dst + 36, ui->ui_Flags);
}

void EfsPutGroupInfo(UBYTE *dst, const struct GroupInfo *gi)
{
    EfsPutCStr(dst, 32, gi->gi_GroupName);
    EfsPut16(dst + 32, gi->gi_GroupID);
    EfsPut16(dst + 34, gi->gi_AdminID);
    EfsPut32(dst + 36, gi->gi_Flags);
}
