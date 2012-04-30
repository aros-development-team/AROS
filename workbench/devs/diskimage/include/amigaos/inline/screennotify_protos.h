#ifndef _VBCCINLINE_SCREENNOTIFY_H
#define _VBCCINLINE_SCREENNOTIFY_H

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

APTR __AddCloseScreenClient(__reg("a6") struct Library *, __reg("a0") struct Screen * screen, __reg("a1") struct MsgPort * port, __reg("d0") LONG pri)="\tjsr\t-30(a6)";
#define AddCloseScreenClient(screen, port, pri) __AddCloseScreenClient(ScreenNotifyBase, (screen), (port), (pri))

BOOL __RemCloseScreenClient(__reg("a6") struct Library *, __reg("a0") APTR handle)="\tjsr\t-36(a6)";
#define RemCloseScreenClient(handle) __RemCloseScreenClient(ScreenNotifyBase, (handle))

APTR __AddPubScreenClient(__reg("a6") struct Library *, __reg("a0") struct MsgPort * port, __reg("d0") LONG pri)="\tjsr\t-42(a6)";
#define AddPubScreenClient(port, pri) __AddPubScreenClient(ScreenNotifyBase, (port), (pri))

BOOL __RemPubScreenClient(__reg("a6") struct Library *, __reg("a0") APTR handle)="\tjsr\t-48(a6)";
#define RemPubScreenClient(handle) __RemPubScreenClient(ScreenNotifyBase, (handle))

APTR __AddWorkbenchClient(__reg("a6") struct Library *, __reg("a0") struct MsgPort * port, __reg("d0") LONG pri)="\tjsr\t-54(a6)";
#define AddWorkbenchClient(port, pri) __AddWorkbenchClient(ScreenNotifyBase, (port), (pri))

BOOL __RemWorkbenchClient(__reg("a6") struct Library *, __reg("a0") APTR handle)="\tjsr\t-60(a6)";
#define RemWorkbenchClient(handle) __RemWorkbenchClient(ScreenNotifyBase, (handle))

#endif /*  _VBCCINLINE_SCREENNOTIFY_H  */
