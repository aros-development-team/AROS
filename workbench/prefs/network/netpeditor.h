#ifndef _FPEDITOR_H_
#define _FPEDITOR_H_

/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
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
#define MUIM_NetPEditor_ActiveEntry     (MUIB_NetPEditor | 0x00000002)
#define MUIM_NetPEditor_AddEntry        (MUIB_NetPEditor | 0x00000003)
#define MUIM_NetPEditor_ChangeEntry     (MUIB_NetPEditor | 0x00000004)

#endif /* _FWPEDITOR_H_ */
