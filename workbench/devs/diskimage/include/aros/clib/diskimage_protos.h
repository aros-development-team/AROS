/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef CLIB_DISKIMAGE_PROTOS_H
#define CLIB_DISKIMAGE_PROTOS_H

/*
**	$VER: diskimage_protos.h 52.1 (13.02.2008)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2001 Amiga, Inc.
**	    All Rights Reserved
*/

#include <dos/dos.h>
#include <devices/trackdisk.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

LONG MountImage(ULONG unit_num, CONST_STRPTR filename);
LONG UnitInfo(ULONG unit_num, STRPTR *filename, BOOL *writeprotect);
LONG WriteProtect(ULONG unit_num, BOOL writeprotect);
LONG UnitControlA(ULONG unit_num, struct TagItem *tags);
LONG UnitControl(ULONG unit_num, Tag tags, ...);
LONG ReloadPlugins(void);
void DoHookPlugins(struct Hook *hook);
void AddDiskChangeHook(struct Hook *hook, BOOL add_or_remove);
void AddReloadPluginsHook(struct Hook *hook, BOOL add_or_remove);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_DISKIMAGE_PROTOS_H */
