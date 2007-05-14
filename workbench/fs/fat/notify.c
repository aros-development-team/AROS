/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "fat_fs.h"
#include "fat_protos.h"

#define DEBUG DEBUG_NOTIFY
#include <aros/debug.h>

/* send a notification for the file referenced by the passed global lock */
void SendNotifyByLock (struct GlobalLock *gl) {
    D(bug("[fat] notifying for lock (%ld/%ld)\n", gl->dir_cluster, gl->dir_entry));
}

/* send a notification for the file referenced by the passed dir entry */
void SendNotifyByDirEntry (struct DirEntry *de) {
    D(bug("[fat] notifying for dir entry (%ld/%ld)\n", de->cluster, de->index));
}
