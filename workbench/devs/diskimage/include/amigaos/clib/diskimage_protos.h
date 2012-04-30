#ifndef CLIB_DISKIMAGE_PROTOS_H
#define CLIB_DISKIMAGE_PROTOS_H


/*
**	$VER: diskimage_protos.h 52.1 (23.11.2011)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright © 2011 
**	All Rights Reserved
*/

#ifndef  DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef  DEVICES_TRACKDISK_H
#include <devices/trackdisk.h>
#endif

LONG MountImage(ULONG unit_num, CONST_STRPTR filename);
LONG UnitInfo(ULONG unit_num, STRPTR * filename, BOOL * writeprotect);
LONG WriteProtect(ULONG unit_num, LONG writeprotect);
LONG UnitControlA(ULONG unit_num, struct TagItem * tags);
LONG UnitControl(ULONG unit_num, Tag tags, ...);
LONG ReloadPlugins(void);
void DoHookPlugins(struct Hook * hook);
void AddDiskChangeHook(struct Hook * hook, LONG add_or_remove);
void AddReloadPluginsHook(struct Hook * hook, LONG add_or_remove);

#endif	/*  CLIB_DISKIMAGE_PROTOS_H  */
