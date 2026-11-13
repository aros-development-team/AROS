#ifndef _VBCCINLINE_DISKIMAGE_H
#define _VBCCINLINE_DISKIMAGE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

LONG __MountImage(__reg("a6") struct Library *, __reg("d0") ULONG unit_num, __reg("a0") CONST_STRPTR filename)="\tjsr\t-42(a6)";
#define MountImage(unit_num, filename) __MountImage(DiskImageBase, (unit_num), (filename))

LONG __UnitInfo(__reg("a6") struct Library *, __reg("d0") ULONG unit_num, __reg("a0") STRPTR * filename, __reg("a1") BOOL * writeprotect)="\tjsr\t-48(a6)";
#define UnitInfo(unit_num, filename, writeprotect) __UnitInfo(DiskImageBase, (unit_num), (filename), (writeprotect))

LONG __WriteProtect(__reg("a6") struct Library *, __reg("d0") ULONG unit_num, __reg("d1") LONG writeprotect)="\tjsr\t-54(a6)";
#define WriteProtect(unit_num, writeprotect) __WriteProtect(DiskImageBase, (unit_num), (writeprotect))

LONG __UnitControlA(__reg("a6") struct Library *, __reg("d0") ULONG unit_num, __reg("a0") struct TagItem * tags)="\tjsr\t-60(a6)";
#define UnitControlA(unit_num, tags) __UnitControlA(DiskImageBase, (unit_num), (tags))

#if !defined(NO_INLINE_STDARG) && (__STDC__ == 1L) && (__STDC_VERSION__ >= 199901L)
LONG __UnitControl(__reg("a6") struct Library *, __reg("d0") ULONG unit_num, Tag tags, ...)="\tmove.l\ta0,-(a7)\n\tlea\t4(a7),a0\n\tjsr\t-60(a6)\n\tmovea.l\t(a7)+,a0";
#define UnitControl(unit_num, ...) __UnitControl(DiskImageBase, (unit_num), __VA_ARGS__)
#endif

LONG __ReloadPlugins(__reg("a6") struct Library *)="\tjsr\t-66(a6)";
#define ReloadPlugins() __ReloadPlugins(DiskImageBase)

void __DoHookPlugins(__reg("a6") struct Library *, __reg("a0") struct Hook * hook)="\tjsr\t-72(a6)";
#define DoHookPlugins(hook) __DoHookPlugins(DiskImageBase, (hook))

void __AddDiskChangeHook(__reg("a6") struct Library *, __reg("a0") struct Hook * hook, __reg("d0") LONG add_or_remove)="\tjsr\t-78(a6)";
#define AddDiskChangeHook(hook, add_or_remove) __AddDiskChangeHook(DiskImageBase, (hook), (add_or_remove))

void __AddReloadPluginsHook(__reg("a6") struct Library *, __reg("a0") struct Hook * hook, __reg("d0") LONG add_or_remove)="\tjsr\t-84(a6)";
#define AddReloadPluginsHook(hook, add_or_remove) __AddReloadPluginsHook(DiskImageBase, (hook), (add_or_remove))

#endif /*  _VBCCINLINE_DISKIMAGE_H  */
