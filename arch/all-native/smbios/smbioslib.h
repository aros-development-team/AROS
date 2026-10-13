#ifndef SMBIOSLIB_H
#define SMBIOSLIB_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Static helper for locating and walking the SMBIOS structure table.
          Linked into modules as libsmbios.a (linklibs-smbios).
*/

#include <exec/types.h>
#include <hardware/smbios.h>

struct SMBIOSTable
{
    UBYTE  st_EntryPointVersion;    /* 2 ("_SM_") or 3 ("_SM3_") */
    UBYTE  st_MajorVersion;         /* SMBIOS specification version */
    UBYTE  st_MinorVersion;
    UBYTE  st_Reserved;
    ULONG  st_TableLength;          /* bytes available at st_Table */
    APTR   st_EntryPoint;
    APTR   st_Table;                /* first structure */
};

/* Locate the entry point: the EFI configuration table on UEFI firmware,
   otherwise (x86 only) a scan of the 0xF0000-0xFFFFF window. The entry
   point checksum is verified. Returns FALSE if none is found. */
BOOL SMBIOS_Locate(struct SMBIOSTable *st);

/* Walk the structure table. Both return NULL at the end of the table
   or at the end-of-table (type 127) structure. */
const struct SMBIOSHeader *SMBIOS_FirstStructure(const struct SMBIOSTable *st);
const struct SMBIOSHeader *SMBIOS_NextStructure(const struct SMBIOSTable *st,
    const struct SMBIOSHeader *s);

/* Find the next structure of the given type after 'after' (NULL = from
   the start). */
const struct SMBIOSHeader *SMBIOS_FindStructure(const struct SMBIOSTable *st,
    UBYTE type, const struct SMBIOSHeader *after);

/* Return string number 'index' (1-based) of a structure, or NULL. */
const char *SMBIOS_GetString(const struct SMBIOSTable *st,
    const struct SMBIOSHeader *s, UBYTE index);

#endif /* SMBIOSLIB_H */
