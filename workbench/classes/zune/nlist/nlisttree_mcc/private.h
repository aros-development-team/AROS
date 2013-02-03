#ifndef NLISTTREE_MCC_PRIVATE_H
#define NLISTTREE_MCC_PRIVATE_H

/***************************************************************************

 NListtree.mcc - New Listtree MUI Custom Class
 Copyright (C) 1999-2001 by Carsten Scholling
 Copyright (C) 2001-2011 by NList Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 NList classes Support Site:  http://www.sf.net/projects/nlist-classes

 $Id$

***************************************************************************/

/*** Include stuff ***/

#ifndef LIBRARIES_MUI_H
#include <libraries/mui.h>
#endif

#include <mcc_common.h>
#include "mui/NList_mcc.h"

//#include "NListtree.h"
#include "Debug.h"

#include "amiga-align.h"

/*** MUI Defines ***/

#define MUIC_NListtree  "NListtree.mcc"
#define NListtreeObject MUI_NewObject(MUIC_NListtree

#define MUIC_NListtreeP "NListtree.mcp" /*i*/
#define NListtreePObject MUI_NewObject(MUIC_NListtreeP  /*i*/

#define SERNR_CASI      0x7ec8                                      // ***  My serial number. *ic*
#define CLASS_TAGBASE   ( TAG_USER | (SERNR_CASI<<16) | 0x1000 )    // ***  Start of class tags. *ic*
#define MUIM_TB         ( CLASS_TAGBASE + 0x0100 )                  // ***  Start of method tags. *ic*
#define MUIA_TB         ( CLASS_TAGBASE + 0x0200 )                  // ***  Start of attribute tags. *ic*
#define MUICFG_TB       ( CLASS_TAGBASE + 0x0000 )                  // ***  Start of config value tags. *ic*

/*** Attributes ***/

#define MUIA_NListtree_Active                               ( MUIA_TB | 0x0001 )    // *** [.SGN]
#define MUIA_NListtree_ActiveList                           ( MUIA_TB | 0x0002 )    // *** [..GN]
#define MUIA_NListtree_CloseHook                            ( MUIA_TB | 0x0003 )    // *** [IS..]
#define MUIA_NListtree_ConstructHook                        ( MUIA_TB | 0x0004 )    // *** [IS..]
#define MUIA_NListtree_DestructHook                         ( MUIA_TB | 0x0005 )    // *** [IS..]
#define MUIA_NListtree_DisplayHook                          ( MUIA_TB | 0x0006 )    // *** [IS..]
#define MUIA_NListtree_DoubleClick                          ( MUIA_TB | 0x0007 )    // *** [ISGN]
#define MUIA_NListtree_DragDropSort                         ( MUIA_TB | 0x0008 )    // *** [IS..]
#define MUIA_NListtree_DupNodeName                          ( MUIA_TB | 0x0009 )    // *** [IS..]
#define MUIA_NListtree_EmptyNodes                           ( MUIA_TB | 0x000a )    // *** [IS..]
#define MUIA_NListtree_Format                               ( MUIA_TB | 0x000b )    // *** [IS..]
#define MUIA_NListtree_OpenHook                             ( MUIA_TB | 0x000c )    // *** [IS..]
#define MUIA_NListtree_Quiet                                ( MUIA_TB | 0x000d )    // *** [.S..]
#define MUIA_NListtree_CompareHook                          ( MUIA_TB | 0x000e )    // *** [IS..]
#define MUIA_NListtree_Title                                ( MUIA_TB | 0x000f )    // *** [IS..]
#define MUIA_NListtree_TreeColumn                           ( MUIA_TB | 0x0010 )    // *** [ISG.]
#define MUIA_NListtree_AutoVisible                          ( MUIA_TB | 0x0011 )    // *** [ISG.]
#define MUIA_NListtree_FindNameHook                         ( MUIA_TB | 0x0012 )    // *** [IS..]
#define MUIA_NListtree_MultiSelect                          ( MUIA_TB | 0x0013 )    // *** [I...]
#define MUIA_NListtree_MultiTestHook                        ( MUIA_TB | 0x0014 )    // *** [IS..]
#define MUIA_NListtree_CopyToClipHook                       ( MUIA_TB | 0x0017 )    // *** [IS..]
#define MUIA_NListtree_DropType                             ( MUIA_TB | 0x0018 )    // *** [..G.]
#define MUIA_NListtree_DropTarget                           ( MUIA_TB | 0x0019 )    // *** [..G.]
#define MUIA_NListtree_DropTargetPos                        ( MUIA_TB | 0x001a )    // *** [..G.]
#define MUIA_NListtree_FindUserDataHook                     ( MUIA_TB | 0x001b )    // *** [IS..]
#define MUIA_NListtree_ShowTree                             ( MUIA_TB | 0x001c )    // *** [ISG.]
#define MUIA_NListtree_SelectChange                         ( MUIA_TB | 0x001d )    // *** [ISGN]
#define MUIA_NListtree_NoRootTree                           ( MUIA_TB | 0x001e )    // *** [I...]

/*i* Private attributes & configs ***/

#define MUIA_NListtree_OnlyTrigger                          ( MUIA_TB | 0x0015 )    // *** $$$ Was ListtreeCompatibility before!
#define MUIA_NListtree_IsMCP                                ( MUIA_TB | 0x0016 )

#define MUICFG_NListtree_ImageSpecClosed                    ( MUICFG_TB | 0x0001 )
#define MUICFG_NListtree_ImageSpecOpen                      ( MUICFG_TB | 0x0002 )
#define MUICFG_NListtree_ImageSpecFolder                    ( MUICFG_TB | 0x0003 )

#define MUICFG_NListtree_PenSpecLines                       ( MUICFG_TB | 0x0004 )
#define MUICFG_NListtree_PenSpecShadow                      ( MUICFG_TB | 0x0005 )
#define MUICFG_NListtree_PenSpecGlow                        ( MUICFG_TB | 0x0006 )

#define MUICFG_NListtree_RememberStatus                     ( MUICFG_TB | 0x0007 )
#define MUICFG_NListtree_IndentWidth                        ( MUICFG_TB | 0x0008 )
#define MUICFG_NListtree_OpenAutoScroll                     ( MUICFG_TB | 0x000a )
#define MUICFG_NListtree_LineType                           ( MUICFG_TB | 0x000b )
#define MUICFG_NListtree_UseFolderImage                     ( MUICFG_TB | 0x000c )

/*i* Private attribute & config values ***/

#define MUICFGV_NListtree_LineType_Disabled                 0
#define MUICFGV_NListtree_LineType_Normal                   1
#define MUICFGV_NListtree_LineType_Dotted                   2
#define MUICFGV_NListtree_LineType_Shadow                   3
#define MUICFGV_NListtree_LineType_Glow                     4

#define MUICFGV_NListtree_ImageSpecClosed_Default           "6:30" // MUII_TapePlay
#define MUICFGV_NListtree_ImageSpecOpen_Default             "6:39" // MUII_TapeDown
#define MUICFGV_NListtree_ImageSpecFolder_Default           "6:22" // MUII_Drawer
#define MUICFGV_NListtree_PenSpecLines_Default              MPEN_SHINE
#define MUICFGV_NListtree_PenSpecShadow_Default             MPEN_SHADOW
#define MUICFGV_NListtree_PenSpecGlow_Default               MPEN_HALFSHADOW
#define MUICFGV_NListtree_RememberStatus_Default            TRUE
#define MUICFGV_NListtree_IndentWidth_Default               0
#define MUICFGV_NListtree_OpenAutoScroll_Default            TRUE
#define MUICFGV_NListtree_LineType_Default                  MUICFGV_NListtree_LineType_Disabled
#define MUICFGV_NListtree_UseFolderImage_Default            FALSE

/*** Special attribute values ***/

#define MUIV_NListtree_Active_Off                           0
#define MUIV_NListtree_Active_Parent                        -2
#define MUIV_NListtree_Active_First                         -3
#define MUIV_NListtree_Active_FirstVisible                  -4
#define MUIV_NListtree_Active_LastVisible                   -5

#define MUIV_NListtree_ActiveList_Off                       0

#define MUIV_NListtree_AutoVisible_Off                      0
#define MUIV_NListtree_AutoVisible_Normal                   1
#define MUIV_NListtree_AutoVisible_FirstOpen                2
#define MUIV_NListtree_AutoVisible_Expand                   3

#define MUIV_NListtree_CompareHook_Head                     0
#define MUIV_NListtree_CompareHook_Tail                     -1
#define MUIV_NListtree_CompareHook_LeavesTop                -2
#define MUIV_NListtree_CompareHook_LeavesMixed              -3
#define MUIV_NListtree_CompareHook_LeavesBottom             -4

#define MUIV_NListtree_ConstructHook_String                 -1
#define MUIV_NListtree_ConstructHook_Flag_AutoCreate        (1<<15)

#define MUIV_NListtree_CopyToClipHook_Default               0

#define MUIV_NListtree_DestructHook_String                  -1

#define MUIV_NListtree_DisplayHook_Default                  -1

#define MUIV_NListtree_DoubleClick_Off                      -1
#define MUIV_NListtree_DoubleClick_All                      -2
#define MUIV_NListtree_DoubleClick_Tree                     -3
#define MUIV_NListtree_DoubleClick_NoTrigger                -4

#define MUIV_NListtree_DropType_None                        0
#define MUIV_NListtree_DropType_Above                       1
#define MUIV_NListtree_DropType_Below                       2
#define MUIV_NListtree_DropType_Onto                        3
#define MUIV_NListtree_DropType_Sorted                      4

#define MUIV_NListtree_FindNameHook_CaseSensitive           0
#define MUIV_NListtree_FindNameHook_CaseInsensitive         -1
#define MUIV_NListtree_FindNameHook_PartCaseSensitive       -2
#define MUIV_NListtree_FindNameHook_PartCaseInsensitive     -3
#define MUIV_NListtree_FindNameHook_PointerCompare          -4
#define MUIV_NListtree_FindNameHook_Part                    MUIV_NListtree_FindNameHook_PartCaseSensitive /* obsolete */

#define MUIV_NListtree_FindUserDataHook_CaseSensitive       0
#define MUIV_NListtree_FindUserDataHook_CaseInsensitive     -1
#define MUIV_NListtree_FindUserDataHook_Part                -2
#define MUIV_NListtree_FindUserDataHook_PartCaseInsensitive -3
#define MUIV_NListtree_FindUserDataHook_PointerCompare      -4

#define MUIV_NListtree_MultiSelect_None                     0
#define MUIV_NListtree_MultiSelect_Default                  1
#define MUIV_NListtree_MultiSelect_Shifted                  2
#define MUIV_NListtree_MultiSelect_Always                   3

#define MUIV_NListtree_MultiSelect_Flag_AutoSelectChilds    (1<<31) /*i*/

#define MUIV_NListtree_ShowTree_Toggle                      -1




/*** Structures & Flags ***/

struct MUI_NListtree_TreeNode {
    struct  MinNode tn_Node;    // ***  To make it a node.
    STRPTR  tn_Name;            // ***  Simple name field.
    UWORD   tn_Flags;           // ***  Used for the flags below.
    APTR    tn_User;            // ***  Free for user data.
    UWORD   tn_Space;           // ***  For counting pixel and saving string space in InsertTreeImages(). *i*
    UWORD   tn_IFlags;          // ***  Internal flags *i*
    UWORD   tn_ImagePos;        // ***  Internal: Open/closed image position *i*
    struct  MUI_NListtree_TreeNode  *tn_Parent;     // ***  Parent node *i*
    struct  NListEntry *tn_NListEntry;              // ***  NList TypeEntry *i*
};


#define TNF_OPEN                    (1<<0)
#define TNF_LIST                    (1<<1)
#define TNF_FROZEN                  (1<<2)
#define TNF_NOSIGN                  (1<<3)
#define TNF_SELECTED                (1<<4)



struct MUI_NListtree_TestPos_Result {
    struct  MUI_NListtree_TreeNode *tpr_TreeNode;
    UWORD   tpr_Type;
    LONG    tpr_ListEntry;
    UWORD   tpr_ListFlags;
    WORD    tpr_Column;

};

#define tpr_Flags tpr_Type      /* OBSOLETE */


/*i* Private Structures & Flags ***/

/*
**  NList TypeEntry for entry position
*/
struct NListEntry {
  APTR  Entry;
  BYTE  Select;
  UBYTE Wrap;
  WORD  PixLen;
  WORD  pos;
  WORD  len;
  WORD  style;
  UWORD dnum;
  ULONG entpos;
};


struct Table {
    struct  MUI_NListtree_TreeNode  **tb_Table;     // ***  Table of entries.
    LONG    tb_Entries,                             // ***  Number of entries in this list.
            tb_Size,                                // ***  Size of the list.
            tb_Current;                             // ***  Current entry (not used everytime)
};

struct MUI_NListtree_ListNode {
    struct  MinNode ln_Node;    // ***  To make it a node.
    STRPTR  ln_Name;            // ***  Simple name field.
    UWORD   ln_Flags;           // ***  Used for some flags.
    APTR    ln_User;            // ***  Free for user data.
    UWORD   tn_Space;           // ***  For saving string space in InertTreeImages().
    UWORD   ln_IFlags;          // ***  Internal flags.
    UWORD   ln_ImagePos;        // ***  Internal: Open/closed image position.
    struct  MUI_NListtree_TreeNode  *ln_Parent;     // ***  Parent node.
    struct  NListEntry *tn_NListEntry;              // ***  NList TypeEntry
    struct  MinList                 ln_List;        // ***  List which holds all the child entries.
    struct  Table                   ln_Table;       // ***  Table of entries.
};


#define TNIF_COLLAPSED              (1<<0)  /* Entry is collapsed. */
#define TNIF_VISIBLE                (1<<1)  /* Entry is currently visible. */
#define TNIF_TEMPFLAG               (1<<2)  /* Multi purpose flag for signalling we have to do something with this node */
#define TNIF_ALLOCATED              (1<<3)  /* Indicates, that memory was previsously allocated for tn_Name field. */
#define TNIF_xxx                    (1<<4)  /* */
#define TNIF_FREE3                  (1<<5)
#define TNIF_ROOT                   (1<<6)  /* This entry is the root list. */



/*** Methods ***/

#define MUIM_NListtree_Open                                 ( MUIM_TB | 0x0001 )
#define MUIM_NListtree_Close                                ( MUIM_TB | 0x0002 )
#define MUIM_NListtree_Insert                               ( MUIM_TB | 0x0003 )
#define MUIM_NListtree_Remove                               ( MUIM_TB | 0x0004 )
#define MUIM_NListtree_Exchange                             ( MUIM_TB | 0x0005 )
#define MUIM_NListtree_Move                                 ( MUIM_TB | 0x0006 )
#define MUIM_NListtree_Rename                               ( MUIM_TB | 0x0007 )
#define MUIM_NListtree_FindName                             ( MUIM_TB | 0x0008 )
#define MUIM_NListtree_GetEntry                             ( MUIM_TB | 0x0009 )
#define MUIM_NListtree_GetNr                                ( MUIM_TB | 0x000a )
#define MUIM_NListtree_Sort                                 ( MUIM_TB | 0x000b )
#define MUIM_NListtree_TestPos                              ( MUIM_TB | 0x000c )
#define MUIM_NListtree_Redraw                               ( MUIM_TB | 0x000d )
#define MUIM_NListtree_NextSelected                         ( MUIM_TB | 0x0010 )
#define MUIM_NListtree_MultiTest                            ( MUIM_TB | 0x0011 )
#define MUIM_NListtree_Select                               ( MUIM_TB | 0x0012 )
#define MUIM_NListtree_Copy                                 ( MUIM_TB | 0x0013 )
#define MUIM_NListtree_InsertStruct                         ( MUIM_TB | 0x0014 )    // *** Insert a struct (like a path) into the list.
#define MUIM_NListtree_Active                               ( MUIM_TB | 0x0015 )    // *** Method which gives the active node/number.
#define MUIM_NListtree_DoubleClick                          ( MUIM_TB | 0x0016 )    // *** Occurs on every double click.
#define MUIM_NListtree_PrevSelected                         ( MUIM_TB | 0x0018 )    // *** Like reverse NextSelected.
#define MUIM_NListtree_CopyToClip                           ( MUIM_TB | 0x0019 )    // *** Copy an entry or part to the clipboard.
#define MUIM_NListtree_FindUserData                         ( MUIM_TB | 0x001a )    // *** Find a node upon user data.
#define MUIM_NListtree_Clear                                ( MUIM_TB | 0x001b )    // *** Clear complete tree.
#define MUIM_NListtree_DropType                             ( MUIM_TB | 0x001e )    // ***
#define MUIM_NListtree_DropDraw                             ( MUIM_TB | 0x001f )    // ***
#define MUIM_NListtree_Construct                            ( MUIM_TB | 0x0020 )    // *** Construct a treenode
#define MUIM_NListtree_Destruct                             ( MUIM_TB | 0x0021 )    // *** Destruct a treenode
#define MUIM_NListtree_Display                              ( MUIM_TB | 0x0022 )    // *** Display a treenode
#define MUIM_NListtree_Compare                              ( MUIM_TB | 0x0023 )    // *** Compare two treenodes


/*i* Private Methods ***/

#define MUIM_NListtree_GetListActive                        ( MUIM_TB | 0x000e )    // *** NEW: Internal! Get MUIA_NList_Active
#define MUIM_NListtree_Data                                 ( MUIM_TB | 0x000f )    // *** Get or set some internal data.
#define MUIM_NListtree_GetDoubleClick                       ( MUIM_TB | 0x0017 )    // *** NEW: Internal! Get MUIA_NList_DoubleClick


/*** Method structs ***/

struct MUIP_NListtree_Open {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *ListNode;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_Close {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *ListNode;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_Insert {
    STACKED ULONG   MethodID;
    STACKED STRPTR  Name;
    STACKED APTR    User;
    STACKED struct  MUI_NListtree_TreeNode *ListNode;
    STACKED struct  MUI_NListtree_TreeNode *PrevNode;
    STACKED ULONG   Flags;
};


struct MUIP_NListtree_Remove {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *ListNode;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_Clear {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *ListNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_FindName {
    STACKED ULONG   MethodID;
    STACKED struct  MUI_NListtree_TreeNode *ListNode;
    STACKED STRPTR  Name;
    STACKED ULONG   Flags;
};


struct MUIP_NListtree_FindUserData {
    STACKED ULONG   MethodID;
    STACKED struct  MUI_NListtree_TreeNode *ListNode;
    STACKED APTR    User;
    STACKED ULONG   Flags;
};


struct MUIP_NListtree_GetEntry {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *Node;
    STACKED LONG  Position;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_GetNr {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_Move {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *OldListNode;
    STACKED struct MUI_NListtree_TreeNode *OldTreeNode;
    STACKED struct MUI_NListtree_TreeNode *NewListNode;
    STACKED struct MUI_NListtree_TreeNode *NewTreeNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_Exchange {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *ListNode1;
    STACKED struct MUI_NListtree_TreeNode *TreeNode1;
    STACKED struct MUI_NListtree_TreeNode *ListNode2;
    STACKED struct MUI_NListtree_TreeNode *TreeNode2;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_Rename {
    STACKED ULONG   MethodID;
    STACKED struct  MUI_NListtree_TreeNode *TreeNode;
    STACKED STRPTR  NewName;
    STACKED ULONG   Flags;
};


struct MUIP_NListtree_Sort {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *ListNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_TestPos {
    STACKED ULONG MethodID;
    STACKED LONG  X;
    STACKED LONG  Y;
    STACKED APTR  Result;
};


struct MUIP_NListtree_Redraw {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_Select {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED LONG    SelType;
    STACKED LONG    SelFlags;
    STACKED LONG    *State;
};


struct MUIP_NListtree_NextSelected {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode **TreeNode;
};


struct MUIP_NListtree_MultiTest {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED LONG    SelType;
    STACKED LONG    SelFlags;
    STACKED LONG    CurrType;
};


struct MUIP_NListtree_Copy {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *SourceListNode;
    STACKED struct MUI_NListtree_TreeNode *SourceTreeNode;
    STACKED struct MUI_NListtree_TreeNode *DestListNode;
    STACKED struct MUI_NListtree_TreeNode *DestTreeNode;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_InsertStruct {
    STACKED ULONG   MethodID;
    STACKED STRPTR  Name;
    STACKED APTR    User;
    STACKED STRPTR  Delimiter;
    STACKED ULONG   Flags;
};


struct MUIP_NListtree_Active {
    STACKED ULONG MethodID;
    STACKED LONG Pos;
    STACKED struct MUI_NListtree_TreeNode *ActiveNode;
};


struct MUIP_NListtree_DoubleClick {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED LONG Entry;
    STACKED LONG Column;
};


struct MUIP_NListtree_PrevSelected {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode **TreeNode;
};


struct MUIP_NListtree_CopyToClip {
    STACKED ULONG MethodID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED LONG Pos;
    STACKED LONG Unit;
};


struct  MUIP_NListtree_DropType {
    STACKED ULONG MethodID;
    STACKED LONG *Pos;
    STACKED LONG *Type;
    STACKED LONG MinX, MaxX, MinY, MaxY;
    STACKED LONG MouseX, MouseY;
};


struct  MUIP_NListtree_DropDraw {
    STACKED ULONG MethodID;
    STACKED LONG Pos;
    STACKED LONG Type;
    STACKED LONG MinX, MaxX, MinY, MaxY;
};


struct MUIP_NListtree_Construct
{
  STACKED ULONG MethodID;
  STACKED STRPTR Name;
  STACKED APTR UserData;
  STACKED APTR MemPool;
  STACKED ULONG Flags;
};


struct MUIP_NListtree_Destruct
{
  STACKED ULONG MethodID;
  STACKED STRPTR Name;
  STACKED APTR UserData;
  STACKED APTR MemPool;
  STACKED ULONG Flags;
};


struct MUIP_NListtree_Display
{
  STACKED ULONG   MethodID;
  STACKED struct  MUI_NListtree_TreeNode *TreeNode;
  STACKED LONG    EntryPos;
  STACKED STRPTR  *Array;
  STACKED STRPTR  *Preparse;
};


struct MUIP_NListtree_Compare
{
  STACKED ULONG MethodID;
  STACKED struct MUI_NListtree_TreeNode *TreeNode1;
  STACKED struct MUI_NListtree_TreeNode *TreeNode2;
  STACKED LONG SortType;
};


/*i* Private method structs ***/

struct MUIP_NListtree_GetListActive {
    STACKED ULONG MethodID;
};


struct MUIP_NListtree_Data {
    STACKED ULONG MethodID;
    STACKED LONG ID;
    STACKED LONG Set;
};



/*** Special method values ***/

#define MUIV_NListtree_Close_ListNode_Root              ((IPTR)0)
#define MUIV_NListtree_Close_ListNode_Parent                ((IPTR)-1)
#define MUIV_NListtree_Close_ListNode_Active                ((IPTR)-2)

#define MUIV_NListtree_Close_TreeNode_Head              ((IPTR)0)
#define MUIV_NListtree_Close_TreeNode_Tail              ((IPTR)-1)
#define MUIV_NListtree_Close_TreeNode_Active                ((IPTR)-2)
#define MUIV_NListtree_Close_TreeNode_All               ((IPTR)-3)

#define MUIV_NListtree_Close_Flag_Nr                        (1<<15) /*i*/
#define MUIV_NListtree_Close_Flag_Visible                   (1<<14) /*i*/


#define MUIV_NListtree_Exchange_ListNode1_Root              ((IPTR)0)
#define MUIV_NListtree_Exchange_ListNode1_Active            ((IPTR)-2)

#define MUIV_NListtree_Exchange_TreeNode1_Head              ((IPTR)0)
#define MUIV_NListtree_Exchange_TreeNode1_Tail              ((IPTR)-1)
#define MUIV_NListtree_Exchange_TreeNode1_Active            ((IPTR)-2)

#define MUIV_NListtree_Exchange_ListNode2_Root              ((IPTR)0)
#define MUIV_NListtree_Exchange_ListNode2_Active            ((IPTR)-2)

#define MUIV_NListtree_Exchange_TreeNode2_Head              ((IPTR)0)
#define MUIV_NListtree_Exchange_TreeNode2_Tail              ((IPTR)-1)
#define MUIV_NListtree_Exchange_TreeNode2_Active            ((IPTR)-2)
#define MUIV_NListtree_Exchange_TreeNode2_Up                ((IPTR)-5)
#define MUIV_NListtree_Exchange_TreeNode2_Down              ((IPTR)-6)


#define MUIV_NListtree_FindName_ListNode_Root               ((IPTR)0)
#define MUIV_NListtree_FindName_ListNode_Active             ((IPTR)-2)

#define MUIV_NListtree_FindName_Flag_SameLevel              (1<<15)
#define MUIV_NListtree_FindName_Flag_Visible                (1<<14)
#define MUIV_NListtree_FindName_Flag_Activate               (1<<13)
#define MUIV_NListtree_FindName_Flag_FindPart               (1<<12) /*i*/
#define MUIV_NListtree_FindName_Flag_Selected               (1<<11)
#define MUIV_NListtree_FindName_Flag_StartNode              (1<<10)
#define MUIV_NListtree_FindName_Flag_Reverse                (1<<9)


#define MUIV_NListtree_FindUserData_ListNode_Root           ((IPTR)0)
#define MUIV_NListtree_FindUserData_ListNode_Active         ((IPTR)-2)

#define MUIV_NListtree_FindUserData_Flag_SameLevel          (1<<15)
#define MUIV_NListtree_FindUserData_Flag_Visible            (1<<14)
#define MUIV_NListtree_FindUserData_Flag_Activate           (1<<13)
#define MUIV_NListtree_FindUserData_Flag_FindPart           (1<<12) /*i*/
#define MUIV_NListtree_FindUserData_Flag_Selected           (1<<11)
#define MUIV_NListtree_FindUserData_Flag_StartNode          (1<<10)
#define MUIV_NListtree_FindUserData_Flag_Reverse            (1<<9)


#define MUIV_NListtree_GetEntry_ListNode_Root               ((IPTR)0)
#define MUIV_NListtree_GetEntry_ListNode_Active             ((IPTR)-2)
#define MUIV_NListtree_GetEntry_TreeNode_Active             ((IPTR)-3)

#define MUIV_NListtree_GetEntry_Position_Head               0
#define MUIV_NListtree_GetEntry_Position_Tail               -1
#define MUIV_NListtree_GetEntry_Position_Active             -2
#define MUIV_NListtree_GetEntry_Position_Next               -3
#define MUIV_NListtree_GetEntry_Position_Previous           -4
#define MUIV_NListtree_GetEntry_Position_Parent             -5
#define MUIV_NListtree_GetEntry_Position_Root               -15 /*i*/

#define MUIV_NListtree_GetEntry_Flag_SameLevel              (1<<15)
#define MUIV_NListtree_GetEntry_Flag_Visible                (1<<14)


#define MUIV_NListtree_GetNr_TreeNode_Root              ((IPTR)0)
#define MUIV_NListtree_GetNr_TreeNode_Active                ((IPTR)-2)

#define MUIV_NListtree_GetNr_Flag_CountAll                  (1<<15)
#define MUIV_NListtree_GetNr_Flag_CountLevel                (1<<14)
#define MUIV_NListtree_GetNr_Flag_CountList                 (1<<13)
#define MUIV_NListtree_GetNr_Flag_ListEmpty                 (1<<12)
#define MUIV_NListtree_GetNr_Flag_Visible                   (1<<11)


#define MUIV_NListtree_Insert_ListNode_Root                 ((IPTR)0)
#define MUIV_NListtree_Insert_ListNode_Active               ((IPTR)-2)
#define MUIV_NListtree_Insert_ListNode_LastInserted         ((IPTR)-3)
#define MUIV_NListtree_Insert_ListNode_ActiveFallback       ((IPTR)-4)

#define MUIV_NListtree_Insert_PrevNode_Head                 ((IPTR)0)
#define MUIV_NListtree_Insert_PrevNode_Tail                 ((IPTR)-1)
#define MUIV_NListtree_Insert_PrevNode_Active               ((IPTR)-2)
#define MUIV_NListtree_Insert_PrevNode_Sorted               ((IPTR)-4)

#define MUIV_NListtree_Insert_Flag_Nr                       (1<<15) /*i*/
#define MUIV_NListtree_Insert_Flag_Visible                  (1<<14) /*i*/
#define MUIV_NListtree_Insert_Flag_Active                   (1<<13)
#define MUIV_NListtree_Insert_Flag_NextNode                 (1<<12)


#define MUIV_NListtree_Move_OldListNode_Root                ((IPTR)0)
#define MUIV_NListtree_Move_OldListNode_Active              ((IPTR)-2)

#define MUIV_NListtree_Move_OldTreeNode_Head                ((IPTR)0)
#define MUIV_NListtree_Move_OldTreeNode_Tail                ((IPTR)-1)
#define MUIV_NListtree_Move_OldTreeNode_Active              ((IPTR)-2)

#define MUIV_NListtree_Move_NewListNode_Root                ((IPTR)0)
#define MUIV_NListtree_Move_NewListNode_Active              ((IPTR)-2)

#define MUIV_NListtree_Move_NewTreeNode_Head                ((IPTR)0)
#define MUIV_NListtree_Move_NewTreeNode_Tail                ((IPTR)-1)
#define MUIV_NListtree_Move_NewTreeNode_Active              ((IPTR)-2)
#define MUIV_NListtree_Move_NewTreeNode_Sorted              ((IPTR)-4)

#define MUIV_NListtree_Move_Flag_Nr                         (1<<15) /*i*/
#define MUIV_NListtree_Move_Flag_Visible                    (1<<14) /*i*/
#define MUIV_NListtree_Move_Flag_KeepStructure              (1<<13)


#define MUIV_NListtree_Open_ListNode_Root                   ((IPTR)0)
#define MUIV_NListtree_Open_ListNode_Parent                 ((IPTR)-1)
#define MUIV_NListtree_Open_ListNode_Active                 ((IPTR)-2)
#define MUIV_NListtree_Open_TreeNode_Head                   ((IPTR)0)
#define MUIV_NListtree_Open_TreeNode_Tail                   ((IPTR)-1)
#define MUIV_NListtree_Open_TreeNode_Active                 ((IPTR)-2)
#define MUIV_NListtree_Open_TreeNode_All                    ((IPTR)-3)

#define MUIV_NListtree_Open_Flag_Nr                         (1<<15) /*i*/
#define MUIV_NListtree_Open_Flag_Visible                    (1<<14) /*i*/


#define MUIV_NListtree_Remove_ListNode_Root                 ((IPTR)0)
#define MUIV_NListtree_Remove_ListNode_Active               ((IPTR)-2)
#define MUIV_NListtree_Remove_TreeNode_Head                 ((IPTR)0)
#define MUIV_NListtree_Remove_TreeNode_Tail                 ((IPTR)-1)
#define MUIV_NListtree_Remove_TreeNode_Active               ((IPTR)-2)
#define MUIV_NListtree_Remove_TreeNode_All                  ((IPTR)-3)
#define MUIV_NListtree_Remove_TreeNode_Selected             ((IPTR)-4)

#define MUIV_NListtree_Remove_Flag_Nr                       (1<<15) /*i*/
#define MUIV_NListtree_Remove_Flag_Visible                  (1<<14) /*i*/
#define MUIV_NListtree_Remove_Flag_NoActive                 (1<<13)


#define MUIV_NListtree_Clear_TreeNode_Root                  0 /*i*/
#define MUIV_NListtree_Clear_TreeNode_Active                -2 /*i*/


#define MUIV_NListtree_Rename_TreeNode_Active               ((IPTR)-2)

#define MUIV_NListtree_Rename_Flag_User                     (1<<8)
#define MUIV_NListtree_Rename_Flag_NoRefresh                (1<<9)


#define MUIV_NListtree_Sort_ListNode_Root                   ((IPTR)0)
#define MUIV_NListtree_Sort_ListNode_Active                 ((IPTR)-2)
#define MUIV_NListtree_Sort_TreeNode_Active                 ((IPTR)-3)

#define MUIV_NListtree_Sort_Flag_Nr                         (1<<15) /*i*/
#define MUIV_NListtree_Sort_Flag_Visible                    (1<<14) /*i*/
#define MUIV_NListtree_Sort_Flag_RecursiveOpen              (1<<13)
#define MUIV_NListtree_Sort_Flag_RecursiveAll               (1<<12)


#define MUIV_NListtree_TestPos_Result_None                  0
#define MUIV_NListtree_TestPos_Result_Above                 1
#define MUIV_NListtree_TestPos_Result_Below                 2
#define MUIV_NListtree_TestPos_Result_Onto                  3
#define MUIV_NListtree_TestPos_Result_Sorted                4

#define MUIV_NListtree_Redraw_Active                        ((IPTR)-1)
#define MUIV_NListtree_Redraw_All                           ((IPTR)-2)

#define MUIV_NListtree_Redraw_Flag_Nr                       (1<<15)

#define MUIV_NListtree_Select_Active                        -1
#define MUIV_NListtree_Select_All                           -2
#define MUIV_NListtree_Select_Visible                       -3

#define MUIV_NListtree_Select_Off                           0
#define MUIV_NListtree_Select_On                            1
#define MUIV_NListtree_Select_Toggle                        2
#define MUIV_NListtree_Select_Ask                           3

#define MUIV_NListtree_Select_Flag_Force                    (1<<15)


#define MUIV_NListtree_NextSelected_Start                   ((IPTR)-1)
#define MUIV_NListtree_NextSelected_End                     ((IPTR)-1)


#define MUIV_NListtree_Copy_SourceListNode_Root             ((IPTR)0)
#define MUIV_NListtree_Copy_SourceListNode_Active           ((IPTR)-2)

#define MUIV_NListtree_Copy_SourceTreeNode_Head             ((IPTR)0)
#define MUIV_NListtree_Copy_SourceTreeNode_Tail             ((IPTR)-1)
#define MUIV_NListtree_Copy_SourceTreeNode_Active           ((IPTR)-2)

#define MUIV_NListtree_Copy_DestListNode_Root               ((IPTR)0)
#define MUIV_NListtree_Copy_DestListNode_Active             ((IPTR)-2)

#define MUIV_NListtree_Copy_DestTreeNode_Head               ((IPTR)0)
#define MUIV_NListtree_Copy_DestTreeNode_Tail               ((IPTR)-1)
#define MUIV_NListtree_Copy_DestTreeNode_Active             ((IPTR)-2)
#define MUIV_NListtree_Copy_DestTreeNode_Sorted             ((IPTR)-4)

#define MUIV_NListtree_Copy_Flag_Nr                         (1<<15) /*i*/
#define MUIV_NListtree_Copy_Flag_Visible                    (1<<14) /*i*/
#define MUIV_NListtree_Copy_Flag_KeepStructure              (1<<13)


#define MUIV_NListtree_InsertStruct_Flag_AllowDuplicates    (1<<11)


#define MUIV_NListtree_PrevSelected_Start                   ((IPTR)-1)
#define MUIV_NListtree_PrevSelected_End                     ((IPTR)-1)


#define MUIV_NListtree_CopyToClip_Active                    ((IPTR)-1)


/*i* Private method values ***/

#define MUIV_NListtree_Data_MemPool                         1
#define MUIV_NListtree_Data_VersInfo                        2
#define MUIV_NListtree_Data_Sample                          3



/*** Hook message structs ***/

struct MUIP_NListtree_CloseMessage
{
    STACKED ULONG HookID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
};


struct MUIP_NListtree_CompareMessage
{
    STACKED ULONG HookID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode1;
    STACKED struct MUI_NListtree_TreeNode *TreeNode2;
    STACKED LONG SortType;
};


struct MUIP_NListtree_ConstructMessage
{
    STACKED ULONG HookID;
    STACKED STRPTR Name;
    STACKED APTR UserData;
    STACKED APTR MemPool;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_DestructMessage
{
    STACKED ULONG HookID;
    STACKED STRPTR Name;
    STACKED APTR UserData;
    STACKED APTR MemPool;
    STACKED ULONG Flags;
};


struct MUIP_NListtree_DisplayMessage
{
    STACKED ULONG   HookID;
    STACKED struct  MUI_NListtree_TreeNode *TreeNode;
    STACKED LONG    EntryPos;
    STACKED STRPTR  *Array;
    STACKED STRPTR  *Preparse;
};


struct MUIP_NListtree_CopyToClipMessage
{
    STACKED ULONG   HookID;
    STACKED struct  MUI_NListtree_TreeNode *TreeNode;
    STACKED LONG    Pos;
    STACKED LONG    Unit;
};


struct MUIP_NListtree_FindNameMessage
{
    STACKED ULONG   HookID;
    STACKED STRPTR  Name;
    STACKED STRPTR  NodeName;
    STACKED APTR    UserData;
    STACKED ULONG   Flags;
};


struct MUIP_NListtree_FindUserDataMessage
{
    STACKED ULONG   HookID;
    STACKED APTR    User;
    STACKED APTR    UserData;
    STACKED STRPTR  NodeName;
    STACKED ULONG   Flags;
};


struct MUIP_NListtree_OpenMessage
{
    STACKED ULONG HookID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
};


struct MUIP_NListtree_MultiTestMessage
{
    STACKED ULONG HookID;
    STACKED struct MUI_NListtree_TreeNode *TreeNode;
    STACKED LONG    SelType;
    STACKED LONG    SelFlag;
    STACKED LONG    CurrType;
};

// ClipboardServer.c
BOOL StartClipboardServer(void);
void ShutdownClipboardServer(void);
void StringToClipboard(ULONG unit, STRPTR str);

#include "default-align.h"

// special flagging macros
#define isFlagSet(v,f)      (((v) & (f)) == (f))  // return TRUE if the flag is set
#define hasFlag(v,f)        (((v) & (f)) != 0)    // return TRUE if one of the flags in f is set in v
#define isFlagClear(v,f)    (((v) & (f)) == 0)    // return TRUE if flag f is not set in v
#define SET_FLAG(v,f)       ((v) |= (f))          // set the flag f in v
#define CLEAR_FLAG(v,f)     ((v) &= ~(f))         // clear the flag f in v
#define MASK_FLAG(v,f)      ((v) &= (f))          // mask the variable v with flag f bitwise

#endif /* NLISTTREE_MCC_PRIVATE_H */
