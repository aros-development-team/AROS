#ifndef _VBCCINLINE_CAPSIMAGE_H
#define _VBCCINLINE_CAPSIMAGE_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

CapsLong __CAPSInit(__reg("a6") struct Device *)="\tjsr\t-42(a6)";
#define CAPSInit() __CAPSInit(CapsImageBase)

CapsLong __CAPSExit(__reg("a6") struct Device *)="\tjsr\t-48(a6)";
#define CAPSExit() __CAPSExit(CapsImageBase)

CapsLong __CAPSAddImage(__reg("a6") struct Device *)="\tjsr\t-54(a6)";
#define CAPSAddImage() __CAPSAddImage(CapsImageBase)

CapsLong __CAPSRemImage(__reg("a6") struct Device *, __reg("d0") CapsLong id)="\tjsr\t-60(a6)";
#define CAPSRemImage(id) __CAPSRemImage(CapsImageBase, (id))

CapsLong __CAPSLockImage(__reg("a6") struct Device *, __reg("d0") CapsLong id, __reg("a0") CONST_STRPTR name)="\tjsr\t-66(a6)";
#define CAPSLockImage(id, name) __CAPSLockImage(CapsImageBase, (id), (name))

CapsLong __CAPSLockImageMemory(__reg("a6") struct Device *, __reg("d0") CapsLong id, __reg("a0") CapsUByte * buffer, __reg("d1") CapsULong length, __reg("d2") CapsULong flag)="\tjsr\t-72(a6)";
#define CAPSLockImageMemory(id, buffer, length, flag) __CAPSLockImageMemory(CapsImageBase, (id), (buffer), (length), (flag))

CapsLong __CAPSUnlockImage(__reg("a6") struct Device *, __reg("d0") CapsLong id)="\tjsr\t-78(a6)";
#define CAPSUnlockImage(id) __CAPSUnlockImage(CapsImageBase, (id))

CapsLong __CAPSLoadImage(__reg("a6") struct Device *, __reg("d0") CapsLong id, __reg("d1") CapsULong flag)="\tjsr\t-84(a6)";
#define CAPSLoadImage(id, flag) __CAPSLoadImage(CapsImageBase, (id), (flag))

CapsLong __CAPSGetImageInfo(__reg("a6") struct Device *, __reg("a0") struct CapsImageInfo * pi, __reg("d0") CapsLong id)="\tjsr\t-90(a6)";
#define CAPSGetImageInfo(pi, id) __CAPSGetImageInfo(CapsImageBase, (pi), (id))

CapsLong __CAPSLockTrack(__reg("a6") struct Device *, __reg("a0") struct CapsTrackInfo * pi, __reg("d0") CapsLong id, __reg("d1") CapsULong cylinder, __reg("d2") CapsULong head, __reg("d3") CapsULong flag)="\tjsr\t-96(a6)";
#define CAPSLockTrack(pi, id, cylinder, head, flag) __CAPSLockTrack(CapsImageBase, (pi), (id), (cylinder), (head), (flag))

CapsLong __CAPSUnlockTrack(__reg("a6") struct Device *, __reg("d0") CapsLong id, __reg("d1") CapsULong cylinder, __reg("d2") CapsULong head)="\tjsr\t-102(a6)";
#define CAPSUnlockTrack(id, cylinder, head) __CAPSUnlockTrack(CapsImageBase, (id), (cylinder), (head))

CapsLong __CAPSUnlockAllTracks(__reg("a6") struct Device *, __reg("d0") CapsLong id)="\tjsr\t-108(a6)";
#define CAPSUnlockAllTracks(id) __CAPSUnlockAllTracks(CapsImageBase, (id))

CONST_STRPTR __CAPSGetPlatformName(__reg("a6") struct Device *, __reg("d0") CapsULong pid)="\tjsr\t-114(a6)";
#define CAPSGetPlatformName(pid) __CAPSGetPlatformName(CapsImageBase, (pid))

CapsLong __CAPSGetVersionInfo(__reg("a6") struct Device *, __reg("a0") struct CapsVersionInfo * pi, __reg("d0") CapsULong flag)="\tjsr\t-120(a6)";
#define CAPSGetVersionInfo(pi, flag) __CAPSGetVersionInfo(CapsImageBase, (pi), (flag))

#endif /*  _VBCCINLINE_CAPSIMAGE_H  */
