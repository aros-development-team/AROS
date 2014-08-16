#ifndef MUI_NList_priv_MCC_H
#define MUI_NList_priv_MCC_H

/***************************************************************************

 NList.mcc - New List MUI Custom Class
 Registered MUI class, Serial Number: 1d51 0x9d510030 to 0x9d5100A0
                                           0x9d5100C0 to 0x9d5100FF

 Copyright (C) 1996-2001 by Gilles Masson
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

#include <libraries/mui.h>

#include <mui/NList_mcc.h>

#include "Debug.h"
#include "Pointer.h"
#include "mcc_common.h"
#include "muiextra.h"

#ifndef LONG_MAX
#define LONG_MAX    0x7fffffff    /* max value for a long */
#endif
#ifndef LONG_MIN
#define LONG_MIN    0x80000000    /* min value for a long */
#endif
#ifndef ULONG_MAX
#define ULONG_MAX   0xffffffffU    /* max value for an unsigned long */
#endif

#define MUIM_NList_Trigger            0x9d510090 /* GM */

#ifndef MUIA_Prop_DoSmooth
#define MUIA_Prop_DoSmooth            0x804236ce /* V4 i.. LONG */
#endif

#define MUIV_NList_CursorType_None 0
#define MUIV_NList_CursorType_Bar 1
#define MUIV_NList_CursorType_Rect 2

/*
#define MUIV_NList_TypeSelect_Line        0
#define MUIV_NList_TypeSelect_Char        1
*/
#define MUIV_NList_TypeSelect_CWord       2
#define MUIV_NList_TypeSelect_CLine       3
#define MUIV_NList_TypeSelect_None        4

#define MUIV_NList_Active_UntilPageUp    -8
#define MUIV_NList_Active_UntilPageDown  -9
#define MUIV_NList_Active_UntilTop      -10
#define MUIV_NList_Active_UntilBottom   -11



#define IEQUALIFIER_SHIFT       (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)
#define IEQUALIFIER_ALT         (IEQUALIFIER_LALT|IEQUALIFIER_RALT)
#define IEQUALIFIER_COMMAND     (IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND)
#define IEQUALIFIER_CTRL_SHIFT  (IEQUALIFIER_CONTROL|IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)
#define IEQUALIFIER_CTRL_LSHIFT (IEQUALIFIER_CONTROL|IEQUALIFIER_LSHIFT)
#define IEQUALIFIER_CTRL_RSHIFT (IEQUALIFIER_CONTROL|IEQUALIFIER_RSHIFT)
#define IEQUALIFIER_CTRL_ALT    (IEQUALIFIER_CONTROL|IEQUALIFIER_LALT|IEQUALIFIER_RALT)
#define IEQUALIFIER_CTRL_LALT   (IEQUALIFIER_CONTROL|IEQUALIFIER_LALT)
#define IEQUALIFIER_CTRL_RALT   (IEQUALIFIER_CONTROL|IEQUALIFIER_RALT)

#define IEQUALIFIER_MASK       (IEQUALIFIER_CONTROL|IEQUALIFIER_SHIFT|IEQUALIFIER_ALT|IEQUALIFIER_COMMAND)

#ifndef RAWKEY_PAGEUP
#define RAWKEY_PAGEUP    0x48
#endif
#ifndef RAWKEY_PAGEDOWN
#define RAWKEY_PAGEDOWN  0x49
#endif
#ifndef RAWKEY_HOME
#define RAWKEY_HOME      0x70
#endif
#ifndef RAWKEY_END
#define RAWKEY_END       0x71
#endif

#define SCROLLBARSTIME 10

#define DATA_STRING_MAX 120

#define CI_PERCENT 0
#define CI_COL     1
#define CI_PIX     2

// defines for the different particalcol substitution types
#define PCS_DISABLED 0
#define PCS_RIGHT    1
#define PCS_LEFT     2
#define PCS_CENTER   3

struct colinfo
{
  struct colinfo *c;

  char *preparse;
  SIPTR colwidthbiggestptr;
  SIPTR colwidthbiggestptr2;
  WORD  colwidthbiggest;
  WORD  colwidthbiggest2;
  WORD  minx;
  WORD  maxx;
  WORD  dx;

  WORD  style;
  WORD  xoffset;
  WORD  colwidth;
  WORD  ninfo;

  WORD  colwidthmax;

  WORD  userwidth;
  WORD  titlebutton;

  WORD  delta;
  WORD  col;
  WORD  width;
  WORD  minwidth;
  WORD  maxwidth;
  WORD  mincolwidth;
  WORD  maxcolwidth;
  WORD  minpixwidth;
  WORD  maxpixwidth;
  BYTE  bar;
  BYTE  width_type;
  WORD  partcolsubst;
};


#define AFFINFOS_START_MAX 40

struct affinfo
{
  char *strptr;
  ULONG tag,tagval,button,imgnum; // RHP: Changed for Special ShortHelp
  IPTR  pen;
  WORD  pos;
  WORD  len;
  WORD  style;
  UBYTE addchar;
  UBYTE addinfo;
};


/* Select values */
#define TE_Select_None        -1
#define TE_Select_Line        -2

/* Wrap values */
#define TE_Wrap_None          0
#define TE_Wrap_TmpLine       0x80
#define TE_Wrap_TmpMask       0x7F

#include "amiga-align.h"

/* The Type entry is exported to NListtree (sigh), so this have to be amiga
 * aligned */

struct TypeEntry {
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

#include "default-align.h"

struct UseImage {
  Object *imgobj;
  struct BitMapImage *bmimg;
  ULONG flags;
};



#define MUIV_NList_Select_None -10
#define MUIV_NList_Select_List -11

struct SelPoint {
  LONG ent;
  WORD xoffset;
  WORD column;
  WORD colxoffset;
  WORD colpos;
};

#define PREPARSE_OFFSET_COL   2000
#define PREPARSE_OFFSET_ENTRY 1000


struct NImgList
{ struct NImgList *next;
  APTR NImgObj;
  WORD width,height;
  WORD dx,dy;
  char *ImgName;
};

#define MAXRAWBUF 40

struct NLData
{
  Object *this;         // pointer to the own object
  Object *nlistviewobj; // pointer to the parent/listview
  Object *listviewobj;  // pointer to the parent/listview if listview is Listview.mui
  Object *scrollersobj; // pointer to the scrollers object

  LONG SETUP;
  LONG SHOW;

  LONG pad1;
  LONG pad2;
  struct MUI_InputHandlerNode ihnode;
  LONG pad3;
  LONG pad4;

  LONG pad5;
  LONG pad6;
  struct MUI_EventHandlerNode ehnode;
  LONG Seconds;
  LONG Micros;

  LONG DRAW;

  char *NList_Format;
  char *NList_SkipChars;
  char *NList_WordSelectChars;
  char *NList_Title;
  const char *NList_IgnoreSpecialChars;
  LONG  NList_TitleSeparator;
  LONG  NList_TitleMark;
  LONG  NList_TitleMark2;
  LONG  NList_LastInserted;
  ULONG NList_Quiet;
  LONG  NList_AffActive;
  LONG  NList_Active;
  LONG  NList_Smooth;
  LONG  NList_AffFirst;
  LONG  NList_AffFirst_Incr;
  LONG  NList_First;
  LONG  NList_First_Incr;
  LONG  NList_Visible;
  LONG  NList_Entries;
  LONG  NList_Prop_First;
  LONG  NList_Prop_First_Real;
  LONG  NList_Prop_First_Prec;
  LONG  NList_Prop_Add;
  LONG  NList_Prop_Wait;
  LONG  NList_Prop_Visible;
  LONG  NList_Prop_Entries;
  LONG  NList_Horiz_AffFirst;
  LONG  NList_Horiz_First;
  LONG  NList_Horiz_Visible;
  LONG  NList_Horiz_Entries;
  LONG  NList_MultiSelect;
  BOOL  NList_DefaultObjectOnClick;
  BOOL  NList_ActiveObjectOnClick;
  LONG  NList_MinLineHeight;
  LONG  NList_Input;
  LONG  NList_TypeSelect;
  LONG  NList_SelectChange;
  IPTR NList_TitlePen;
  IPTR NList_ListPen;
  IPTR NList_SelectPen;
  IPTR NList_CursorPen;
  IPTR NList_UnselCurPen;
  IPTR NList_InactivePen;
  IPTR NList_TitleBackGround;
  IPTR NList_ListBackGround;
  IPTR NList_SelectBackground;
  IPTR NList_CursorBackground;
  IPTR NList_UnselCurBackground;
  IPTR NList_InactiveBackground;
  LONG  NList_DragType;
  LONG  NList_Dropable;
  LONG  NList_DragColOnly;
  LONG  NList_DragSortable;
  LONG  NList_DropMark;
  LONG  NList_ShowDropMarks;
  LONG  NList_ColWidthDrag;
  LONG  NList_AdjustHeight;
  LONG  NList_AdjustWidth;
  LONG  NList_SourceArray;
  IPTR  NList_KeepActive;
  IPTR  NList_MakeActive;
  LONG  NList_DefClickColumn;
  LONG  NList_AutoCopyToClip;
  LONG  NList_AutoVisible;
  LONG  NList_TabSize;
  LONG  NList_EntryValueDependent;
  APTR  NList_PrivateData;
  LONG  NList_ForcePen;
  LONG  NList_PartialCol;
  LONG  NList_List_Select;
  LONG  NList_PartialChar;
  LONG  NList_ContextMenu;
  LONG  NList_SortType;
  LONG  NList_SortType2;
  LONG  NList_ButtonClick;
  LONG  NList_MinColSortable;
  LONG  NList_Imports;
  LONG  NList_Exports;
  LONG  NList_Disabled;
  LONG  NList_SerMouseFix;
  LONG  NList_DragLines;
  LONG  NList_WheelStep;
  LONG  NList_WheelFast;
  LONG  NList_WheelMMB;
  LONG  NList_VerticalCenteredText;
  BOOL  NList_AutoClip;
  BOOL  NList_SelectPointer;

  // object pointers for forwarding the
  // key focus to other objects.
  Object *NList_KeyUpFocus;
  Object *NList_KeyDownFocus;
  Object *NList_KeyLeftFocus;
  Object *NList_KeyRightFocus;

  STRPTR NList_ShortHelp;

  LONG  ListCompatibility;
  struct KeyBinding *NList_Keys;
  struct KeyBinding *Wheel_Keys;
  UBYTE *NList_Columns;
  ULONG HLINE_thick_pen;
  LONG  ContextMenu;
  LONG  ContextMenuOn;
  struct TypeEntry **EntriesArray;
  struct Hook *NList_CompareHook;
  struct Hook *NList_ConstructHook;
  struct Hook *NList_DestructHook;
  struct Hook *NList_DisplayHook;
  struct Hook *NList_MultiTestHook;
  struct Hook *NList_CopyEntryToClipHook;
  struct Hook *NList_CopyColumnToClipHook;
  LONG         NList_CompareHook2;
  LONG         NList_ConstructHook2;
  LONG         NList_DestructHook2;
  LONG         NList_DisplayHook2;
  LONG         NList_CopyEntryToClipHook2;
  LONG         NList_CopyColumnToClipHook2;
  APTR	Pool;				/* Custom or internal pool pointer. */
  APTR	PoolInternal;	/* Internal pool pointer. */
  APTR  EntryPool;
  IPTR  NList_Font;
  LONG  MOUSE_MOVE;

  struct IClass *ncl;
  struct IClass *ocl;

  APTR  VertPropObject;
  Object *NL_Group;
  Object *VirtGroup;
  Object *VirtGroup2;
  struct IClass *VirtClass;
  Object *MenuObj;

  // for our own mouse pointer management
  Object *SizePointerObj;
  Object *MovePointerObj;
  Object *SelectPointerObj;
  enum PointerType activeCustomPointer;

  struct TextFont *InUseFont;

  struct NImgList *NImage2;
  struct NImgList NImage;

  struct UseImage *NList_UseImages;
  LONG   LastImage;

  APTR  dispentry; char *DisplayArray[DISPLAY_ARRAY_MAX*2 +2];
  LONG  LastEntry;

  LONG ForcePen;

  LONG lvisible;
  LONG hvisible;
  LONG ScrollBarsPos;
  LONG ScrollBars;
  LONG ScrollBarsOld;
  LONG ScrollBarsTime;

  LONG Notify;
  LONG DoNotify;
  LONG Notifying;

  LONG TitleClick;
  LONG TitleClick2;

  BOOL isActiveObject; // TRUE in case object is active object of window

  LONG MinImageHeight;

  LONG pushtrigger;
  LONG parse_column;
  LONG parse_ent;
  char *display_ptr;
  LONG drag_type;
  LONG drag_border;
  LONG drag_qualifier;
  LONG moves;
  LONG multiselect;
  LONG multisel_qualifier;
  LONG multiclick;
  LONG multiclickalone;
  LONG sorted;
  LONG selectmode;
  LONG lastselected;
  LONG lastactived;
  LONG selectskiped;
  LONG first_change;
  LONG last_change;
  LONG minx_change_offset;
  LONG minx_change_entry;
  LONG maxx_change_offset;
  LONG maxx_change_entry;
  LONG adding_member;
  LONG markdraw;
  LONG markerase;
  LONG marktype;
  LONG markdrawnum;
  LONG markerasenum;
  LONG actbackground;
  LONG NumIntuiTick;
  struct RastPort *DragRPort;
  char *DragText;
  LONG DragEntry;
  LONG DragWidth;
  LONG DragHeight;

  IPTR Pen_Title_init;
  IPTR Pen_List_init;
  IPTR Pen_Select_init;
  IPTR Pen_Cursor_init;
  IPTR Pen_UnselCur_init;
  IPTR Pen_Inactive_init;
  IPTR BG_Title_init;
  IPTR BG_List_init;
  IPTR BG_Select_init;
  IPTR BG_Cursor_init;
  IPTR BG_UnselCur_init;
  IPTR BG_Inactive_init;

  LONG old_prop_first;
  LONG old_prop_visible;
  LONG old_prop_entries;
  LONG old_prop_delta;
  LONG old_horiz_first;
  LONG old_horiz_visible;
  LONG old_horiz_entries;
  LONG old_horiz_delta;
  APTR drawsuper;

  struct RastPort *rp;
  struct TextFont *font;
  UWORD  *pens;

  ULONG secs, micros;
  LONG  mouse_x;
  LONG  mouse_y;
  LONG  click_line;
  LONG  click_x;
  LONG  click_y;

  WORD   min_sel;
  WORD   max_sel;
  struct SelPoint sel_pt[4];
  LONG   last_sel_click_x;
  LONG   last_sel_click_y;

  LONG   affover;  // RHP: Added for Special Shorthelp
  LONG   affimage; // RHP: Added for Special Shorthelp
  LONG   storebutton;
  LONG   affbutton;
  LONG   affbuttonline;
  LONG   affbuttoncol;
  LONG   affbuttonstate;
  struct IBox affbuttonpos;

  struct affinfo *aff_infos;
  struct colinfo *cols;

  WORD numaff_infos;
  WORD numcols;
  WORD numcols2;

  WORD format_chge;
  WORD do_draw_all;
  WORD do_draw_title;
  WORD do_draw_active;
  WORD do_draw;
  WORD do_parse;
  WORD do_images;
  WORD do_setcols;
  WORD do_updatesb;
  WORD do_wwrap;
  WORD force_wwrap;
  WORD nodraw;
  WORD dropping;
  WORD refreshing;
  WORD UpdateScrollersRedrawn;
  WORD UpdatingScrollbars;

  WORD adjustcolumn;
  WORD adjustbar;
  WORD adjustbar2;
  WORD adjustbar_old;
  WORD adjustbar_last;
  WORD adjustbar_last2;

  WORD tabsize;
  WORD spacesize;
  WORD Title_PixLen;
  WORD badrport;

  WORD mleft;
  WORD mright;
  WORD mtop;
  WORD mbottom;
  WORD mwidth;
  WORD mheight;

  WORD vdx;
  WORD vdy;
  WORD vleft;
  WORD vright;
  WORD vtop;
  WORD vbottom;
  WORD vwidth;
  WORD vheight;

  WORD drawall_bits;
  WORD drawall_dobit;

  WORD vpos;
  WORD voff;
  WORD vinc;
  WORD addvinc;
  WORD hpos;
  WORD hinc;
  WORD vdtitlepos;
  WORD vdtitleheight;
  WORD vdtpos;
  WORD vdt;
  WORD vdbpos;
  WORD vdb;

  WORD firstselect;

  WORD left;
  WORD top;
  WORD width;
  WORD height;

  BYTE column[DISPLAY_ARRAY_MAX+2];

  char imagebuf[64+4];

  struct InputEvent ievent;
  char rawtext[MAXRAWBUF];

  char NList_TitlePenBuffer[128];
  char NList_ListPenBuffer[128];
  char NList_SelectPenBuffer[128];
  char NList_CursorPenBuffer[128];
  char NList_UnselCurPenBuffer[128];
  char NList_InactivePenBuffer[128];
  char NList_TitleBackGroundBuffer[128];
  char NList_ListBackGroundBuffer[128];
  char NList_SelectBackgroundBuffer[128];
  char NList_CursorBackgroundBuffer[128];
  char NList_UnselCurBackgroundBuffer[128];
  char NList_InactiveBackgroundBuffer[128];
};


#define MUII_myListInactive  (data->NList_InactiveBackground)
#define MUII_myListUnselCur  (data->NList_UnselCurBackground)
#define MUII_myListCursor    (data->NList_CursorBackground)
#define MUII_myListSelCur    (data->NList_CursorBackground)
#define MUII_myListSelect    (data->NList_SelectBackground)


/*
extern struct TextFont *Topaz_8;
*/

#define LIBVER(lib)          ((struct Library *)lib)->lib_Version


#ifndef NO_PROTOS
#include "protos.h"
#endif

#define	MUIV_NList_PoolPuddleSize_Default	2048
#define	MUIV_NList_PoolThreshSize_Default	1024

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

#ifndef MIN
  #define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

// special flagging macros
#define isFlagSet(v,f)      (((v) & (f)) == (f))  // return TRUE if the flag is set
#define hasFlag(v,f)        (((v) & (f)) != 0)    // return TRUE if one of the flags in f is set in v
#define isFlagClear(v,f)    (((v) & (f)) == 0)    // return TRUE if flag f is not set in v
#define SET_FLAG(v,f)       ((v) |= (f))          // set the flag f in v
#define CLEAR_FLAG(v,f)     ((v) &= ~(f))         // clear the flag f in v
#define MASK_FLAG(v,f)      ((v) &= (f))          // mask the variable v with flag f bitwise

#define VERSION_IS_AT_LEAST(ver, rev, minver, minrev) (((ver) > (minver)) || ((ver) == (minver) && (rev) == (minrev)) || ((ver) == (minver) && (rev) > (minrev)))
#define LIB_VERSION_IS_AT_LEAST(lib, minver, minrev)  VERSION_IS_AT_LEAST(((struct Library *)(lib))->lib_Version, ((struct Library *)(lib))->lib_Revision, minver, minrev)

#if defined(__amigaos4__)
#define AllocVecShared(size, flags)  AllocVecTags((size), AVT_Type, MEMF_SHARED, AVT_Lock, FALSE, ((flags)&MEMF_CLEAR) ? AVT_ClearWithValue : TAG_IGNORE, 0, TAG_DONE)
#else
#define AllocVecShared(size, flags)  AllocVec((size), (flags))
#endif

#endif /* MUI_NList_priv_MCC_H */
