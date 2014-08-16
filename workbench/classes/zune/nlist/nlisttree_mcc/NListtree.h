#ifndef NLISTTREE_H
#define NLISTTREE_H

/***************************************************************************

 NListtree.mcc - New Listtree MUI Custom Class
 Copyright (C) 1999-2001 by Carsten Scholling
 Copyright (C) 2001-2014 NList Open Source Team

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

#include "private.h"

#define MUIA_TI_Spec                      0xfedcL
#define MUIA_TI_MaxWidth                  0xfedbL
#define MUIA_TI_MaxHeight                 0xfedaL
#define MUIA_TI_Style                     0xfed9L
#define MUIA_TI_Space                     0xfed8L
#define MUIA_TI_Pen                       0xfed7L

struct NListtree_HookMessage
{
  ULONG HookID;
};

struct MyImage
{
  Object *Image;      // ***  Image objects.
  Object *ListImage;  // ***  List image objects (created by ...CreateImage).

  struct NListtree_Data *nltdata;
};


struct MyContextMenuChoice
{
  LONG unit;
  LONG pos;
  LONG col;
  LONG ontop;
};


struct NListtree_Data
{
  /*
  **  General object related vars.
  */
  APTR   MemoryPool;                  // ***  Our global memory pool
  APTR   TreePool;                    // ***  Our tree memory pool
  Object *Obj;                        // ***  The listtree object itself
  STRPTR buf;                         // ***  Multi purpose buffer (most image include)
  struct MUI_EventHandlerNode EHNode; // ***  Event handler node.


  /*
  **  Image custom class stuff.
  */
  struct MUI_CustomClass *CL_TreeImage;
  struct MUI_CustomClass *CL_NodeImage;


  /*
  **  List handling.
  */
  struct MUI_NListtree_ListNode RootList;           // ***  The head root list.
  struct MUI_NListtree_ListNode *ActiveList;        // ***  The active list node
  struct MUI_NListtree_ListNode *LastInsertedList;  // ***  The list node, the last entry was inserted

  struct MUI_NListtree_TreeNode *ActiveNode;        // ***  Current active tree node
  struct MUI_NListtree_TreeNode *OpenCloseEntry;    // ***  Entry to open/close in handler.
  struct MUI_NListtree_TreeNode *TempActiveNode;    // ***  New active node after Remove for example.

  struct Table SelectedTable;                       // ***  Table of selected entries.
  struct MUI_NListtree_TreeNode *TempSelected;      // ***  Temporary selected entry.

  ULONG ActiveNodeNum;                              // ***  Number of the active node.


  /*
  **  Rendering information, images etc.
  */
  struct  MUI_RenderInfo      *MRI;         // ***  MUI render info structure (got in _Setup())

  struct  MyImage         Image[4];       // ***  My special image structure.

  UWORD             MaxImageWidth,      // ***  Maximum image width.
                    MaxImageHeight;     // ***  Maximum image height.

  /*
  **  Pen specific stuff.
  */
  LONG              Pen[3];         // ***  Obtained pen for lines/shadow/glow drawing.


  /*
  **  Other configuration stuff.
  */
  ULONG             LineType;       // *** The line type used (see private.h)
  ULONG             IndentWidth;    // *** The width of the indent
  BOOL              UseFolderImage; // *** boolean for selecting if the folder image should be shown or not

  UBYTE             MultiSelect;      // ***  Multi selection kind and flags.
  UWORD             MultiSelectFlags;

  struct MyContextMenuChoice    MenuChoice;

  /*
  **  All hooks used.
  */
  struct  Hook          *OpenHook,        // ***  Safe place for all hooks the user wants to be called
                        *CloseHook,

                        *ConstructHook,
                        *DestructHook,

                        *MultiTestHook,

                        *DisplayHook,
                        *CopyToClipHook,

                        *CompareHook,
                        *FindNameHook,
                        *FindUserDataHook;

  struct Hook IntCompareHook; /* The internal compare hook, is no pointer to avoid unnecessary allocations */


  /*
  **  List format save.
  */
  STRPTR              Format;         // ***  Here the list format is saved


  /*
  **  Input handling stuff (double click etc.)
  */
  BYTE              DoubleClick;      // ***  Holds specified double click value
  UWORD             TreeColumn;       // ***  Holds specified tree column

  ULONG             LDClickTimeSecs;    // ***  Left mouse button click handling
  ULONG             LDClickTimeMicros;  // ***  Left mouse button click handling
  LONG              LDClickEntry;
  WORD              LDClickColumn;

#if 0
  ULONG             MDClickTimeSecs;    // ***  Middle mouse button click handling
  ULONG             MDClickTimeMicros;  // ***  Middle mouse button click handling
  LONG              MDClickEntry;
  WORD              MDClickColumn;
#endif

  /*
  **  Auto visualization stuff
  */
  UBYTE             AutoVisible;      // ***  Holds information about autovisiblization


  /*
  **  Drag'n Drop stuff.
  */
  struct MUI_NListtree_TreeNode *DropTarget;      // ***  The entry which is currently the drop target.
  struct MUI_NListtree_TreeNode *OldDropTarget;   // ***  The entry which is currently the drop target.
  LONG compositingActive;

  ULONG             DropTargetPos;      // ***  Position of target drop entry.
  ULONG             DropType;       // ***  The dropping type (above, below, onto...)
  ULONG             OpenDropListSecs;
  ULONG             OpenDropListMicros;

  /*
  **  Number of entries total.
  */
  ULONG             NumEntries;       // ***  Global number of entries in the whole list tree


  /*
  **  Some flags and error var.
  */
  ULONG             Flags;          // ***  Some flags (see below)
  ULONG             QuietCounter;     // ***  Quiet nesting count.
  ULONG             ForceActiveNotify; // *** next MUIA_NListtree_Notify notify will be enforced
  ULONG             IgnoreSelectionChange; /* *** Ignores the next selection change */
  UWORD             Error;
};


/*
**  Flag values for Flags field
*/
#define NLTF_EMPTYNODES                   (1UL<<0)    // ***  Display empty nodes as leafs.
#define NLTF_DUPNODENAMES                 (1UL<<1)    // ***  Do not copy "name"-field. Use supplied pointer.
#define NLTF_QUIET                        (1UL<<2)    // ***  NListtree is in quiet state.
#define NLTF_TITLE                        (1UL<<3)    // ***  Show title.
#define NLTF_REFRESH                      (1UL<<4)    // ***  Do an general object refresh.
#define NLTF_REMEMBER_STATUS              (1UL<<5)    // ***  Config: Remember open/close status of nodes.
#define NLTF_DRAGDROPSORT                 (1UL<<6)    // ***  Enabled drag'n drop sort.
#define NLTF_REDRAW                       (1UL<<7)    // ***  Redraw specified line.
#define NLTF_AUTOVISIBLE                  (1UL<<8)    // ***  Activated entries will always be shown.
#define NLTF_SELECT_METHOD                (1UL<<9)    // ***  Selection through Select method.
#define NLTF_OVER_ARROW                   (1UL<<10)   // ***  Mouse clicked over arrow. No drag!!
#define NLTF_DRAGDROP                     (1UL<<11)   // ***  Drag&Drop in in progress.
#define NLTF_OPENAUTOSCROLL               (1UL<<12)   // ***  Config: Auto scroll entries when opening a node with a large list.
#define NLTF_NO_TREE                      (1UL<<13)   // ***  Do not show a tree.
#define NLTF_NLIST_NO_SCM_SUPPORT         (1UL<<14)   // ***  NList.mcc has no SelectChange method support implemented!
#define NLTF_INT_COMPAREHOOK              (1UL<<15)   // ***  Internal compare hook used!
#define NLTF_INT_CONSTRDESTRHOOK          (1UL<<16)   // ***  Internal construct/destruct hooks!
#define NLTF_ACTIVENOTIFY                 (1UL<<17)   // ***  Active notify activated?
#define NLTF_ISMCP                        (1UL<<18)   // ***  MCP is here ;-)
#define NLTF_NLIST_DIRECT_ENTRY_SUPPORT   (1UL<<19)   // ***  NList version supports direct entry request via backpointer
#define NLTF_NO_ROOT_TREE                 (1UL<<20)   // ***  Do not display root tree gfx.
#define NLTF_SETACTIVE                    (1UL<<21)   // ***  Set the active entry.
#define NLTF_AUTOSELECT_CHILDS            (1UL<<22)   // ***  Automatically select childs if their parents selected.
#define NLTF_SAFE_NOTIFIES                (1UL<<23)   // ***  Is removing MUI notifies safe?

#define NLTF_QUALIFIER_LCOMMAND           (1UL<<28)   // ***  Rawkey qualifier information.
#define NLTF_QUALIFIER_LALT               (1UL<<29)
#define NLTF_QUALIFIER_CONTROL            (1UL<<30)
#define NLTF_QUALIFIER_LSHIFT             (1UL<<31)


#define INSERT_POS_HEAD                   -1L
#define INSERT_POS_TAIL                   -2L


#define IMAGE_Closed                      0L
#define IMAGE_Open                        1L
#define IMAGE_Folder                      2L
#define IMAGE_Tree                        3L

#define SPEC_Vert                         1L
#define SPEC_VertT                        2L
#define SPEC_VertEnd                      3L
#define SPEC_Space                        4L
#define SPEC_Hor                          5L

#define PEN_Shadow                        0L
#define PEN_Line                          1L
#define PEN_Glow                          2L

/// xget()
//  Gets an attribute value from a MUI object
IPTR xget(Object *obj, const IPTR attr);
#if defined(__GNUC__)
  // please note that we do not evaluate the return value of GetAttr()
  // as some attributes (e.g. MUIA_Selected) always return FALSE, even
  // when they are supported by the object. But setting b=0 right before
  // the GetAttr() should catch the case when attr doesn't exist at all
  #define xget(OBJ, ATTR) ({IPTR b=0; GetAttr(ATTR, OBJ, &b); b;})
#endif
///

#endif /* NLISTTREE_H */
