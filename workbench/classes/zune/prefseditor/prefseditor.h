#ifndef ZUNE_PREFSEDITOR_H
#define ZUNE_PREFSEDITOR_H

/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    This file is part of the PrefsEditor class, which is distributed under
    the terms of version 2.1 of the GNU Lesser General Public License.
    
    $Id$
*/

#include <dos/dos.h>
#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_PrefsEditor           "PrefsEditor.mcc"

/*** Identifier base ********************************************************/
#define MUIB_PrefsEditor           (MUIB_AROS | 0x00000500)

/*** Public Methods *********************************************************/
#define MUIM_PrefsEditor_Test      (MUIB_PrefsEditor | 0x00000000)
#define MUIM_PrefsEditor_Revert    (MUIB_PrefsEditor | 0x00000001)
#define MUIM_PrefsEditor_Save      (MUIB_PrefsEditor | 0x00000002)
#define MUIM_PrefsEditor_Use       (MUIB_PrefsEditor | 0x00000003)
#define MUIM_PrefsEditor_Cancel    (MUIB_PrefsEditor | 0x00000004)
#define MUIM_PrefsEditor_Import    (MUIB_PrefsEditor | 0x00000005)
struct  MUIP_PrefsEditor_Import    {ULONG MethodID; CONST_STRPTR filename;};
#define MUIM_PrefsEditor_Export    (MUIB_PrefsEditor | 0x00000006)
struct  MUIP_PrefsEditor_Export    {ULONG MethodID; CONST_STRPTR filename;};

/*** Public (Abstract) Methods **********************************************/
#define MUIM_PrefsEditor_ImportFH  (MUIB_PrefsEditor | 0x00000007)
struct  MUIP_PrefsEditor_ImportFH  {ULONG MethodID; BPTR fh;};
#define MUIM_PrefsEditor_ExportFH  (MUIB_PrefsEditor | 0x00000008)
struct  MUIP_PrefsEditor_ExportFH  {ULONG MethodID; BPTR fh;};

/*** Public Attributes ******************************************************/
#define MUIA_PrefsEditor_Name      (MUIB_PrefsEditor | 0x00000000) /* --G  CONST_STRPTR */
#define MUIA_PrefsEditor_Changed   (MUIB_PrefsEditor | 0x00000001) /* -SG  BOOL */
#define MUIA_PrefsEditor_Testing   (MUIB_PrefsEditor | 0x00000002) /* -SG  BOOL */
#define MUIA_PrefsEditor_CanSave   (MUIB_PrefsEditor | 0x00000004) /* --G  BOOL */
#define MUIA_PrefsEditor_CanTest   (MUIB_PrefsEditor | 0x00000005) /* --G  BOOL */

/*** Protected Attributes ***************************************************/
#define MUIA_PrefsEditor_Path      (MUIB_PrefsEditor | 0x00000003) /* I-G  CONST_STRPTR */

/*** Macros *****************************************************************/
#define PrefsEditorObject MUIOBJMACRO_START(MUIC_PrefsEditor)

#endif /* ZUNE_PREFSEDITOR_H */
