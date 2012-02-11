/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#include <aros/macros.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include <string.h>    
#include <stdio.h>
#include <ctype.h>

#include "ntfs_fs.h"
#include "ntfs_protos.h"

#include "debug.h"

/* set the name of an entry. */
LONG SetDirEntryName(struct DirEntry *short_de, STRPTR name, ULONG len)
{
    LONG err = 0;

    D(bug("[NTFS] %s: setting name for entry no #%ld to '", __PRETTY_FUNCTION__, short_de->no);
      RawPutChars(name, len); bug("'\n"));

    return err;
}
