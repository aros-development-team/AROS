#ifndef _DISKINFO_H_
#define _DISKINFO_H_

/*
    Copyright © 2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <libraries/mui.h>
#include <intuition/classusr.h>
/*** Attributes *************************************************************/

/*+
    [I..] CONST_STRPTR
    Initial command line.
+*/
#define MUIA_DiskInfo_Initial (TAG_USER | 0x20000001)

/*+
    [..G] STRPTR
+*/
#define MUIA_DiskInfo_Volname (TAG_USER | 0x20000002)

/*+
    [..G] ULONG
+*/
#define MUIA_DiskInfo_Percent (TAG_USER | 0x20000003)

/*+
    [ISG] ULONG
+*/
#define MUIA_DiskInfo_Aspect (TAG_USER | 0x20000004)

/*+
    [ISG] ULONG
+*/
#define MUIA_DiskInfo_UserLevel (TAG_USER | 0x20000005)

/*** Variables **************************************************************/
extern struct MUI_CustomClass *DiskInfo_CLASS;

/*** Macros *****************************************************************/
#define DiskInfoObject BOOPSIOBJMACRO_START(DiskInfo_CLASS->mcc_Class)

/*** Prototypes *************************************************************/
BOOL DiskInfo_Initialize();
void DiskInfo_Deinitialize();

#endif /* _DISKINFO_H_ */
