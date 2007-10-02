#ifndef _IPEDITOR_H_
#define _IPEDITOR_H_

/*
    Copyright  2004, The AROS Development Team. All rights reserved.
    $Id: ipeditor.h 21130 2004-02-28 22:50:12Z chodorowski, dariusb $
*/

#include <exec/types.h>
#include <libraries/mui.h>

/*** Identifier base ********************************************************/
#define MUIB_IPEditor                  (TAG_USER | 0x10000000)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *IPEditor_CLASS;

/*** Macros *****************************************************************/
#define IPEditorObject BOOPSIOBJMACRO_START(IPEditor_CLASS->mcc_Class)

#define MUIA_MyStringifyType        0x8001

#define STRINGIFY_DoubleClickDelay  0x00
#define STRINGIFY_RepeatDelay       0x01
#define STRINGIFY_RepeatRate        0x02

#endif /* _IPEDITOR_H_ */
