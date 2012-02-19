#ifndef _MUI_CLASSES_PROCESS_H
#define _MUI_CLASSES_PROCESS_H

/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Process            "Process.mui"

/*** Identifier base (for Zune extensions) **********************************/

/*** Methods ****************************************************************/
#define MUIM_Process_Kill               (MUIB_MUI|0x004264cf) /* V20 */
#define MUIM_Process_Launch             (MUIB_MUI|0x00425df7) /* V20 */
#define MUIM_Process_Process            (MUIB_MUI|0x004230aa) /* V20 */
#define MUIM_Process_Signal             (MUIB_MUI|0x0042e791) /* V20 */
struct  MUIP_Process_Kill               {STACKED ULONG MethodID; STACKED LONG maxdelay;};
struct  MUIP_Process_Launch             {STACKED ULONG MethodID;};
struct  MUIP_Process_Process            {STACKED ULONG MethodID; STACKED ULONG *kill; STACKED Object *proc;};
struct  MUIP_Process_Signal             {STACKED ULONG MethodID; STACKED ULONG sigs;};

/*** Attributes *************************************************************/
#define MUIA_Process_AutoLaunch         (MUIB_MUI|0x00428855) /* V20 i.. ULONG             */
#define MUIA_Process_Name               (MUIB_MUI|0x0042732b) /* V20 i.. ULONG             */
#define MUIA_Process_Priority           (MUIB_MUI|0x00422a54) /* V20 i.. ULONG             */
#define MUIA_Process_SourceClass        (MUIB_MUI|0x0042cf8b) /* V20 i.. ULONG             */
#define MUIA_Process_SourceObject       (MUIB_MUI|0x004212a2) /* V20 i.. ULONG             */
#define MUIA_Process_StackSize          (MUIB_MUI|0x004230d0) /* V20 i.. ULONG             */
#define MUIA_Process_Task               (MUIB_MUI|0x0042b123) /* V20 ..g ULONG             */


extern const struct __MUIBuiltinClass _MUI_Process_desc; /* PRIV */

#endif /* _MUI_CLASSES_PROCESS_H */
