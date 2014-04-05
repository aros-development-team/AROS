#ifndef _NETPEDITOR_H_
#define _NETPEDITOR_H_

/*
    Copyright © 2009-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_NetPEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *NetPEditor_CLASS;

/*** Macros *****************************************************************/
#define NetPEditorObject BOOPSIOBJMACRO_START(NetPEditor_CLASS->mcc_Class)

#define MUIM_NetPEditor_IPModeChanged   (MUIB_NetPEditor | 0x00000001)
#define MUIM_NetPEditor_ShowEntry       (MUIB_NetPEditor | 0x00000002)
#define MUIM_NetPEditor_EditEntry       (MUIB_NetPEditor | 0x00000003)
#define MUIM_NetPEditor_ApplyEntry      (MUIB_NetPEditor | 0x00000004)
#define MUIM_NetPEditor_ShowHostEntry   (MUIB_NetPEditor | 0x00000005)
#define MUIM_NetPEditor_EditHostEntry   (MUIB_NetPEditor | 0x00000006)
#define MUIM_NetPEditor_ApplyHostEntry  (MUIB_NetPEditor | 0x00000007)
#define MUIM_NetPEditor_ShowNetEntry    (MUIB_NetPEditor | 0x00000008)
#define MUIM_NetPEditor_EditNetEntry    (MUIB_NetPEditor | 0x00000009)
#define MUIM_NetPEditor_ApplyNetEntry   (MUIB_NetPEditor | 0x0000000A)
#define MUIM_NetPEditor_ShowServerEntry (MUIB_NetPEditor | 0x0000000B)
#define MUIM_NetPEditor_EditServerEntry (MUIB_NetPEditor | 0x0000000C)
#define MUIM_NetPEditor_ApplyServerEntry (MUIB_NetPEditor | 0x0000000D)

struct MUIP_NetPEditor_EditEntry {STACKED ULONG MethodID; STACKED ULONG addEntry;};
struct MUIP_NetPEditor_EditNetEntry {STACKED ULONG MethodID; STACKED ULONG addEntry;};
struct MUIP_NetPEditor_IPModeChanged {STACKED ULONG MethodID; STACKED ULONG interface;};

#endif /* _NETPEDITOR_H_ */
