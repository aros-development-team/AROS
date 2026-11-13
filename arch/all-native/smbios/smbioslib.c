/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Static helper for locating and walking the SMBIOS structure table.
*/

#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/efi.h>

#include <hardware/efi/config.h>

#include "smbioslib.h"

static BOOL SMBIOS_ChecksumOK(const UBYTE *p, ULONG len)
{
    UBYTE sum = 0;

    while (len--)
        sum += *p++;

    return sum == 0;
}

/*
 * Fill in 'st' from an entry point candidate at 'eps'. 'anchor' says
 * which anchor was matched. Returns FALSE if the entry point is invalid.
 */
static BOOL SMBIOS_ParseEntryPoint(struct SMBIOSTable *st, const UBYTE *eps, UBYTE version)
{
    if (version == 3)
    {
        const struct SMBIOSEntryPoint3 *ep3 = (const struct SMBIOSEntryPoint3 *)eps;

        if (ep3->length < sizeof(*ep3) || !SMBIOS_ChecksumOK(eps, ep3->length))
            return FALSE;
        if (ep3->table_address == 0 || ep3->table_length < sizeof(struct SMBIOSHeader))
            return FALSE;
#if __WORDSIZE < 64
        if (ep3->table_address >> 32)
            return FALSE;
#endif
        st->st_EntryPointVersion = 3;
        st->st_MajorVersion = ep3->major;
        st->st_MinorVersion = ep3->minor;
        st->st_TableLength = ep3->table_length;
        st->st_Table = (APTR)(IPTR)ep3->table_address;
    }
    else
    {
        const struct SMBIOSEntryPoint2 *ep2 = (const struct SMBIOSEntryPoint2 *)eps;

        if (ep2->length < sizeof(*ep2) || !SMBIOS_ChecksumOK(eps, ep2->length))
            return FALSE;
        if (ep2->table_address == 0 || ep2->table_length < sizeof(struct SMBIOSHeader))
            return FALSE;

        st->st_EntryPointVersion = 2;
        st->st_MajorVersion = ep2->major;
        st->st_MinorVersion = ep2->minor;
        st->st_TableLength = ep2->table_length;
        st->st_Table = (APTR)(IPTR)ep2->table_address;
    }

    st->st_Reserved = 0;
    st->st_EntryPoint = (APTR)eps;

    D(bug("[SMBIOS] %d.%d entry point @ 0x%p, table @ 0x%p (%u bytes)\n",
        st->st_MajorVersion, st->st_MinorVersion, st->st_EntryPoint,
        st->st_Table, st->st_TableLength));

    return TRUE;
}

BOOL SMBIOS_Locate(struct SMBIOSTable *st)
{
    struct Library *EFIBase = OpenResource("efi.resource");

    /* On UEFI firmware the entry point is published as a configuration table */
    if (EFIBase)
    {
        const uuid_t smbios3_guid = SMBIOS3_TABLE_GUID;
        const uuid_t smbios_guid = SMBIOS_TABLE_GUID;
        const UBYTE *eps;

        eps = EFI_FindConfigTable(&smbios3_guid);
        if (eps && SMBIOS_ParseEntryPoint(st, eps, 3))
            return TRUE;

        eps = EFI_FindConfigTable(&smbios_guid);
        if (eps && SMBIOS_ParseEntryPoint(st, eps, 2))
            return TRUE;
    }

    /*
     * Legacy BIOS places it in the 0xF0000-0xFFFFF window, paragraph aligned.
     * Only the PC has that window; on other platforms reading it faults.
     */
#if defined(__i386__) || defined(__x86_64__)
    {
        const UBYTE *ptr;

        for (ptr = (const UBYTE *)0x000F0000; ptr < (const UBYTE *)0x00100000; ptr += 16)
        {
            if (ptr[0] != '_' || ptr[1] != 'S' || ptr[2] != 'M')
                continue;
            if (ptr[3] == '3' && ptr[4] == '_' && SMBIOS_ParseEntryPoint(st, ptr, 3))
                return TRUE;
            if (ptr[3] == '_' && SMBIOS_ParseEntryPoint(st, ptr, 2))
                return TRUE;
        }
    }
#endif

    return FALSE;
}

static inline const UBYTE *SMBIOS_TableEnd(const struct SMBIOSTable *st)
{
    return (const UBYTE *)st->st_Table + st->st_TableLength;
}

const struct SMBIOSHeader *SMBIOS_FirstStructure(const struct SMBIOSTable *st)
{
    const struct SMBIOSHeader *s = st->st_Table;

    if (!s || (const UBYTE *)s + sizeof(*s) > SMBIOS_TableEnd(st))
        return NULL;
    if (s->sm_Type == SMBIOS_TYPE_END)
        return NULL;

    return s;
}

const struct SMBIOSHeader *SMBIOS_NextStructure(const struct SMBIOSTable *st,
    const struct SMBIOSHeader *s)
{
    const UBYTE *end = SMBIOS_TableEnd(st);
    const UBYTE *p;

    if (!s || s->sm_Type == SMBIOS_TYPE_END || s->sm_Length < sizeof(*s))
        return NULL;

    /* Skip the formatted area, then the string set (terminated by a double NUL) */
    p = (const UBYTE *)s + s->sm_Length;
    while (p + 1 < end)
    {
        if (p[0] == 0 && p[1] == 0)
        {
            p += 2;
            if (p + sizeof(*s) > end)
                return NULL;
            s = (const struct SMBIOSHeader *)p;
            if (s->sm_Type == SMBIOS_TYPE_END)
                return NULL;
            return s;
        }
        p++;
    }

    return NULL;
}

const struct SMBIOSHeader *SMBIOS_FindStructure(const struct SMBIOSTable *st,
    UBYTE type, const struct SMBIOSHeader *after)
{
    const struct SMBIOSHeader *s;

    s = after ? SMBIOS_NextStructure(st, after) : SMBIOS_FirstStructure(st);
    while (s)
    {
        if (s->sm_Type == type)
            return s;
        s = SMBIOS_NextStructure(st, s);
    }

    return NULL;
}

const char *SMBIOS_GetString(const struct SMBIOSTable *st,
    const struct SMBIOSHeader *s, UBYTE index)
{
    const UBYTE *end = SMBIOS_TableEnd(st);
    const UBYTE *p;
    UBYTE n = 1;

    if (!s || index == 0 || s->sm_Length < sizeof(*s))
        return NULL;

    p = (const UBYTE *)s + s->sm_Length;
    while (p < end && *p != 0)
    {
        const UBYTE *str = p;

        while (p < end && *p != 0)
            p++;
        if (p >= end)
            return NULL;
        if (n == index)
            return (const char *)str;
        n++;
        p++;
    }

    return NULL;
}
