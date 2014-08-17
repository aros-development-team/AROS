#ifndef ZUNE_LISTTREE_MCC_H
#define ZUNE_LISTTREE_MCC_H

#include <libraries/mui.h>

/*** Name *******************************************************************/
#define MUIC_Listtree "Listtree.mcc"

/*** Identifier base ********************************************************/

/*** Attributes *************************************************************/
#define MUIA_Listtree_Active               (MUIB_MUI|0x00020020) /* [.SG]  APTR          */
#define MUIA_Listtree_Quiet                (MUIB_MUI|0x0002000a) /* [.SG]  BOOL          */
#define MUIA_Listtree_DoubleClick          (MUIB_MUI|0x0002000d) /* [.SG]  BOOL          */
#define MUIA_Listtree_ConstructHook        (MUIB_MUI|0x00020016) /* [I..]  struct Hook * */
#define MUIA_Listtree_DestructHook         (MUIB_MUI|0x00020017) /* [I..]  struct Hook * */
#define MUIA_Listtree_DisplayHook          (MUIB_MUI|0x00020018) /* [I..]  struct Hook * */
#define MUIA_Listtree_Title                (MUIB_MUI|0x00020015) /* [I..]  BOOL          */
#define MUIA_Listtree_Format               (MUIB_MUI|0x00020014) /* [I..]  STRPTR        */
#define MUIA_Listtree_DragDropSort         (MUIB_MUI|0x00020031) /* [I..]  BOOL          */
#define MUIA_Listtree_SortHook             (MUIB_MUI|0x00020010) /* [I..]  struct Hook * */

#define MUIV_Listtree_FindName_ListNode_Root         0

#define MUIV_Listtree_FindName_Flags_SameLevel      (1<<15)


#define MUIV_Listtree_GetEntry_Position_Tail        -1
#define MUIV_Listtree_GetEntry_Position_Active      -2
#define MUIV_Listtree_GetEntry_Position_Next        -3
#define MUIV_Listtree_GetEntry_Position_Previous    -4
#define MUIV_Listtree_GetEntry_Position_Parent      -5

#define MUIV_Listtree_GetEntry_Flags_SameLevel      (1<<15)
#define MUIV_Listtree_GetEntry_Flags_Visible        (1<<14)

#define MUIV_Listtree_GetEntry_ListNode_Root         0
#define MUIV_Listtree_GetEntry_ListNode_Active      -2

#define MUIV_Listtree_Remove_ListNode_Root           0

#define MUIV_Listtree_Remove_TreeNode_Active        -2
#define MUIV_Listtree_Remove_TreeNode_All           -3

#define MUIV_Listtree_Insert_ListNode_Root           0

#define MUIV_Listtree_Insert_PrevNode_Tail          -1
#define MUIV_Listtree_Insert_PrevNode_Sorted        -4

#define MUIV_Listtree_Open_ListNode_Root             0
#define MUIV_Listtree_Open_ListNode_Parent          -1

#define MUIV_Listtree_Open_TreeNode_All             -3

#define MUIV_Listtree_Close_ListNode_Root            0

#define MUIV_Listtree_Close_TreeNode_All            -3

#define MUIV_Listtree_SetDropMark_Values_None        0

#define MUIV_Listtree_TestPos_Result_Flags_Onto      3


#define TNF_OPEN   (1<<00)
#define TNF_LIST   (1<<01)

/*** Methods ****************************************************************/
#define MUIM_Listtree_FindName             (MUIB_MUI|0x0002003c)
#define MUIM_Listtree_GetEntry             (MUIB_MUI|0x0002002b)
#define MUIM_Listtree_GetNr                (MUIB_MUI|0x0002000e)
#define MUIM_Listtree_Remove               (MUIB_MUI|0x00020012)
#define MUIM_Listtree_Insert               (MUIB_MUI|0x00020011)
#define MUIM_Listtree_Rename               (MUIB_MUI|0x0002000c)
#define MUIM_Listtree_Open                 (MUIB_MUI|0x0002001e)
#define MUIM_Listtree_Close                (MUIB_MUI|0x0002001f)
#define MUIM_Listtree_TestPos              (MUIB_MUI|0x0002004b)
#define MUIM_Listtree_SetDropMark          (MUIB_MUI|0x0002004c)

struct MUIS_Listtree_TreeNode
{
    SIPTR  tn_Private1;
    SIPTR  tn_Private2;
    STRPTR tn_Name;
    UWORD  tn_Flags;
    APTR   tn_User;
};

struct MUIS_Listtree_TestPos_Result
{
    APTR  tpr_TreeNode;
    UWORD tpr_Flags;
    LONG  tpr_ListEntry;
    UWORD tpr_ListFlags;
};

struct MUIP_Listtree_Insert {STACKED ULONG MethodID;STACKED STRPTR Name;STACKED APTR User;STACKED APTR ListNode;STACKED APTR PrevNode;STACKED ULONG Flags;};
struct MUIP_Listtree_GetEntry {STACKED ULONG MethodID; STACKED APTR Node;STACKED LONG Position;STACKED ULONG Flags;};
struct MUIP_Listtree_GetNr {STACKED ULONG MethodID;STACKED APTR TreeNode;STACKED ULONG Flags;};
struct MUIP_Listtree_Remove {STACKED ULONG MethodID;STACKED APTR ListNode;STACKED APTR TreeNode;STACKED ULONG Flags;};
struct MUIP_Listtree_Rename {STACKED ULONG MethodID;STACKED APTR TreeNode;STACKED STRPTR NewName;STACKED ULONG Flags;};
struct MUIP_Listtree_Open {STACKED ULONG MethodID;STACKED APTR ListNode;STACKED APTR TreeNode;STACKED ULONG Flags;};
struct MUIP_Listtree_Close {STACKED ULONG MethodID;STACKED APTR ListNode;STACKED APTR TreeNode;STACKED ULONG Flags;};
struct MUIP_Listtree_TestPos {STACKED ULONG MethodID;STACKED LONG X;STACKED LONG Y;STACKED APTR Result;};
struct MUIP_Listtree_SetDropMark {STACKED ULONG MethodID;STACKED LONG Entry;STACKED ULONG Values;};
struct MUIP_Listtree_FindName {STACKED ULONG MethodID;STACKED APTR  ListNode;STACKED STRPTR Name;STACKED ULONG Flags;};

#endif /* ZUNE_LISTTREE_MCC_H */
