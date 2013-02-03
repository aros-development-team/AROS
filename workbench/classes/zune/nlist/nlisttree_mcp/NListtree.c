/***************************************************************************

 NListtree.mcc - New Listtree MUI Custom Class
 Copyright (C) 1999-2001 by Carsten Scholling
 Copyright (C) 2001-2007 by NList Open Source Team

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

#if defined(__AROS__)
#define MUI_OBSOLETE 1
#endif

/*
**  Includes
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libraries/mui.h>
#include <utility/hooks.h>
#include <clib/alib_protos.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/locale.h>
#include <proto/exec.h>

#include <mui/NFloattext_mcc.h>
#include <mui/NListview_mcc.h>
#include <mui/NList_mcc.h>

#include "private.h"
#include "version.h"

#include "locale.h"

#include "muiextra.h"

// some undocumented MUI tags we are going to use
#ifndef MUIA_Imagedisplay_UseDefSize
#define MUIA_Imagedisplay_UseDefSize  0x8042186dUL /* V11 i.. BOOL */
#endif
#ifndef MUIA_Slider_Level
#define MUIA_Slider_Level                   0x8042ae3aUL /* V4  isg LONG              */
#endif
#ifndef MUIA_Slider_Min
#define MUIA_Slider_Min                     0x8042e404UL /* V4  isg LONG              */
#endif
#ifndef MUIA_Slider_Max
#define MUIA_Slider_Max                     0x8042d78aUL /* V4  isg LONG              */
#endif

struct SampleArray
{
  const char *name;
  UWORD flags;
};

/*
**  Sample
*/
static const struct SampleArray sa[] =
{
  { "root1", 0                            },

  { "comp", TNF_LIST | TNF_OPEN           },
  {    "sys", TNF_LIST | TNF_OPEN         },
  {       "amiga", TNF_LIST | TNF_OPEN    },
  {         "misc", 0x8000                },
  {       "mac", TNF_LIST                 },
  {         "system", 0x8000              },

  { "root2", 0                            },

  { "de", TNF_LIST | TNF_OPEN             },
  {    "comm", TNF_LIST                   },
  {      "software", TNF_LIST             },
  {         "ums", 0x8000                 },
  {    "comp", TNF_LIST | TNF_OPEN        },
  {       "sys", TNF_LIST | TNF_OPEN      },
  {         "amiga", TNF_LIST             },
  {           "misc", 0x8000              },
  {           "tech", 0x8000              },
  {         "amiga", 0x8000               },

  { "m", TNF_LIST                         },
  {   "i", TNF_LIST                       },
  {     "c", TNF_LIST                     },
  {       "h", TNF_LIST                   },
  {         "e", TNF_LIST                 },
  {           "l", TNF_LIST               },
  {             "a", TNF_LIST             },
  {               "n", TNF_LIST           },
  {                 "g", TNF_LIST         },
  {                   "e", TNF_LIST       },
  {                     "l", TNF_LIST     },
  {                       "o", 0          },

  { "end", TNF_LIST                       },
  {   "of", TNF_LIST                      },
  {     "tree", 0                         },

  { NULL, 0 }
};

/*****************************************************************************\
*******************************************************************************
**
**  Helpful object related functions.
**
*******************************************************************************
\*****************************************************************************/

static VOID DrawSampleTree( Object *ltobj )
{
  struct MUI_NListtree_TreeNode *tn1, *tn2, *tn3;
  WORD i = 0;

  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn3, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn2 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn2, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn3 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;

  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], MUIV_NListtree_Insert_ListNode_Root, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
  tn1 = (struct MUI_NListtree_TreeNode *)DoMethod( ltobj, MUIM_NListtree_Insert, sa[i].name, &sa[i], tn1, MUIV_NListtree_Insert_PrevNode_Tail, sa[i].flags ); i++;
}


STATIC VOID TransferValues(struct NListtreeP_Data *data)
{
  nnset(data->NLT_Sample, MUIA_NListtree_Quiet, TRUE);

  SetAttrs(data->NLT_Sample,  MUICFG_NListtree_ImageSpecClosed,  xget(data->PI_ImageClosed, MUIA_Imagedisplay_Spec),
                              MUICFG_NListtree_ImageSpecOpen,    xget(data->PI_ImageOpen, MUIA_Imagedisplay_Spec),
                              MUICFG_NListtree_ImageSpecFolder,  xget(data->PI_ImageFolder, MUIA_Imagedisplay_Spec),
                              MUICFG_NListtree_PenSpecLines,     xget(data->PP_LinePen, MUIA_Pendisplay_Spec),
                              MUICFG_NListtree_PenSpecShadow,    xget(data->PP_ShadowPen, MUIA_Pendisplay_Spec),
                              MUICFG_NListtree_PenSpecGlow,      xget(data->PP_GlowPen, MUIA_Pendisplay_Spec),
                              MUICFG_NListtree_RememberStatus,   xget(data->CH_RememberStatus, MUIA_Selected),
                              MUICFG_NListtree_IndentWidth,      xget(data->SL_IndentWidth, MUIA_Slider_Level),
                              MUICFG_NListtree_OpenAutoScroll,   xget(data->CH_OpenAutoScroll, MUIA_Selected),
                              MUICFG_NListtree_LineType,         xget(data->CY_LineType, MUIA_Cycle_Active),
                              MUICFG_NListtree_UseFolderImage,   xget(data->CH_UseFolderImage, MUIA_Selected),
                              TAG_DONE);

  nnset(data->NLT_Sample, MUIA_NListtree_Quiet, FALSE);

  return;
}



/*****************************************************************************\
*******************************************************************************
**
**  Standard class related functions.
**
*******************************************************************************
\*****************************************************************************/

HOOKPROTONHNO(dspfunc, LONG, struct MUIP_NListtree_DisplayMessage *msg)
{
  static const char *t1 = "\033bNewsgroups";
  static const char *t2 = "\033bFlags";
  static const char *t3 = "subscribed";
  static const char *t4 = "\0";
  static const char *t5 = "\033bCnt";
  static char buf[10];

  if ( msg->TreeNode != NULL )
  {
    /*
    **  The user data is a pointer to a SampleArray struct.
    */
    struct SampleArray *a = (struct SampleArray *)msg->TreeNode->tn_User;

    snprintf(buf, sizeof(buf), "%3ld", (unsigned long)msg->Array[-1]);

    *msg->Array++  = (STRPTR)a->name;
    *msg->Array++  = (STRPTR)((a->flags & 0x8000) ? t3 : t4);
    *msg->Array++  = buf;
  }
  else
  {
    *msg->Array++  = (STRPTR)t1;
    *msg->Array++  = (STRPTR)t2;
    *msg->Array++  = (STRPTR)t5;
  }

  return( 0 );
}
MakeStaticHook(dsphook, dspfunc);

static VOID ValuesChanged(struct NListtreeP_Data *data)
{
  nnset(data->PP_LinePen, MUIA_Disabled, xget(data->CY_LineType, MUIA_Cycle_Active) == MUICFGV_NListtree_LineType_Disabled);
  nnset(data->PP_ShadowPen, MUIA_Disabled, xget(data->CY_LineType, MUIA_Cycle_Active) != MUICFGV_NListtree_LineType_Shadow && xget(data->CY_LineType, MUIA_Cycle_Active) != MUICFGV_NListtree_LineType_Glow);
  nnset(data->PP_GlowPen, MUIA_Disabled, xget(data->CY_LineType, MUIA_Cycle_Active) != MUICFGV_NListtree_LineType_Glow);

  nnset(data->PI_ImageFolder, MUIA_Disabled, xget(data->CH_UseFolderImage, MUIA_Selected) == FALSE);

  return;
}

HOOKPROTONHNO(ValuesChangedFunc, VOID, IPTR *para)
{
  ValuesChanged((struct NListtreeP_Data *)para[0]);
}
MakeStaticHook(ValuesChangedHook, ValuesChangedFunc);

static IPTR _NewP( struct IClass *cl, Object *obj, Msg msg )
{
  struct NListtreeP_Data *data;
  static const char *CY_LineTypes_Entries[6];
  static const char infotext[] = "\033bNListtree.mcp " LIB_REV_STRING "\033n (" LIB_DATE ")\n"
                                 "Copyright (c) 1999-2001 Carsten Scholling\n"
                                 LIB_COPYRIGHT "\n\n"
                                 "Distributed under the terms of the LGPL2.\n\n"
                                 "For the latest version, check out:\n"
                                 "http://www.sf.net/projects/nlist-classes/\n\n";

  static unsigned char msg_closed_key;
  static unsigned char msg_open_key;
  static unsigned char msg_lines_key;
  static unsigned char msg_shadow_key;
  static unsigned char msg_glow_key;
  static unsigned char msg_linetypes_key;
  static unsigned char msg_indentwidth_key;
  static unsigned char msg_remember_status_key;
  static unsigned char msg_open_autoscroll_key;
  static unsigned char msg_bt_expand_key;
  static unsigned char msg_bt_collapse_key;
  static unsigned char msg_folder_key;
  static unsigned char msg_folderimage_key;

  ENTER();

  if( !( obj = (Object *)DoSuperMethodA( cl, obj, msg ) ) )
  {
    RETURN(0);
    return( 0 );
  }

  /*
  **  Init data.
  */
  data = INST_DATA( cl, obj );

  CY_LineTypes_Entries[0] = tr(MSG_LINETYPE_DISABLED);
  CY_LineTypes_Entries[1] = tr(MSG_LINETYPE_NORMAL);
  CY_LineTypes_Entries[2] = tr(MSG_LINETYPE_DOTTED);
  CY_LineTypes_Entries[3] = tr(MSG_LINETYPE_SHADOW);
  CY_LineTypes_Entries[4] = tr(MSG_LINETYPE_GLOW);
  CY_LineTypes_Entries[5] = NULL;

  msg_closed_key          = tr(MSG_BUTTON_SHORTCUT_CLOSED)[0];
  msg_open_key            = tr(MSG_BUTTON_SHORTCUT_OPEN)[0];
  msg_lines_key           = tr(MSG_BUTTON_SHORTCUT_LINES)[0];
  msg_shadow_key          = tr(MSG_BUTTON_SHORTCUT_SHADOW)[0];
  msg_glow_key            = tr(MSG_BUTTON_SHORTCUT_GLOW)[0];
  msg_linetypes_key       = tr(MSG_BUTTON_SHORTCUT_LINETYPES)[0];
  msg_indentwidth_key     = tr(MSG_BUTTON_SHORTCUT_INDENTWIDTH)[0];
  msg_remember_status_key = tr(MSG_BUTTON_SHORTCUT_REMEMBER)[0];
  msg_open_autoscroll_key = tr(MSG_BUTTON_SHORTCUT_AUTOSCROLL)[0];
  msg_bt_expand_key       = tr(MSG_BUTTON_SHORTCUT_EXPAND)[0];
  msg_bt_collapse_key     = tr(MSG_BUTTON_SHORTCUT_COLLAPSE)[0];
  msg_folder_key          = tr(MSG_BUTTON_SHORTCUT_FOLDER)[0];
  msg_folderimage_key     = tr(MSG_BUTTON_SHORTCUT_FOLDERIMAGE)[0];

  /*
  **  Preferences group.
  */
  data->GR_Prefs = VGroup,
    MUIA_Group_VertSpacing, 5,

    Child, HGroup,

      /*
      **  Images and Lines
      */
      Child, VGroup,

        Child, HGroup, GroupFrameT(tr(MSG_GROUP_IMAGES_COLORS)),
          MUIA_HorizWeight, 1,

          Child, VGroup,

            Child, FreeKeyCLabel(tr(MSG_BUTTON_LABEL_CLOSED), msg_closed_key),
            Child, data->PI_ImageClosed     = PopimageObject,
              MUIA_Imagedisplay_UseDefSize,   TRUE,
              MUIA_Imageadjust_Type,          MUIV_Imageadjust_Type_Image,
              MUIA_ControlChar,               msg_closed_key,
              MUIA_CycleChain,                TRUE,
              MUIA_Draggable,                 TRUE,
              MUIA_Window_Title,              tr(MSG_WIN_TITLE_CLOSED_IMAGE),
              MUIA_ShortHelp,                 tr(MSG_HELP_CLOSED_IMAGE),
            End,


            Child, FreeKeyCLabel(tr(MSG_BUTTON_LABEL_OPEN), msg_open_key),
            Child, data->PI_ImageOpen       = PopimageObject,
              MUIA_Imagedisplay_UseDefSize,   TRUE,
              MUIA_Imageadjust_Type,          MUIV_Imageadjust_Type_Image,
              MUIA_ControlChar,               msg_open_key,
              MUIA_CycleChain,                TRUE,
              MUIA_Draggable,                 TRUE,
              MUIA_Window_Title,              tr(MSG_WIN_TITLE_OPEN_IMAGE),
              MUIA_ShortHelp,                 tr(MSG_HELP_OPEN_IMAGE),
            End,

            Child, FreeKeyCLabel(tr(MSG_BUTTON_LABEL_FOLDER), msg_folder_key),
            Child, data->PI_ImageFolder     = PopimageObject,
              MUIA_Imagedisplay_UseDefSize,   TRUE,
              MUIA_Imageadjust_Type,          MUIV_Imageadjust_Type_Image,
              MUIA_ControlChar,               msg_folder_key,
              MUIA_CycleChain,                TRUE,
              MUIA_Draggable,                 TRUE,
              MUIA_Disabled,                  MUICFGV_NListtree_UseFolderImage_Default == FALSE,
              MUIA_Window_Title,              tr(MSG_WIN_TITLE_FOLDER_IMAGE),
              MUIA_ShortHelp,                 tr(MSG_HELP_FOLDER_IMAGE),
            End,

          End,


          Child, VGroup,

            Child, FreeKeyCLabel(tr(MSG_BUTTON_LABEL_LINES), msg_lines_key),
            Child, data->PP_LinePen = PoppenObject,
              MUIA_Disabled,      MUICFGV_NListtree_LineType_Default == MUICFGV_NListtree_LineType_Disabled,
              MUIA_ControlChar,   msg_lines_key,
              MUIA_CycleChain,    TRUE,
              MUIA_Window_Title,  tr(MSG_WIN_TITLE_LINES_COLOR),
              MUIA_ShortHelp,     tr(MSG_HELP_LINES_COLOR),
            End,

            Child, FreeKeyCLabel(tr(MSG_BUTTON_LABEL_SHADOW), msg_shadow_key),
            Child, data->PP_ShadowPen = PoppenObject,
              MUIA_Disabled,      MUICFGV_NListtree_LineType_Default != MUICFGV_NListtree_LineType_Shadow,
              MUIA_ControlChar,   msg_shadow_key,
              MUIA_CycleChain,    TRUE,
              MUIA_Window_Title,  tr(MSG_WIN_TITLE_SHADOW_COLOR),
              MUIA_ShortHelp,     tr(MSG_HELP_SHADOW_COLOR),
            End,

            Child, FreeKeyCLabel(tr(MSG_BUTTON_LABEL_GLOW), msg_glow_key),
            Child, data->PP_GlowPen = MUI_NewObject( MUIC_Poppen,
              MUIA_Disabled,          MUICFGV_NListtree_LineType_Default != MUICFGV_NListtree_LineType_Glow,
              MUIA_ControlChar,       msg_glow_key,
              MUIA_CycleChain,        TRUE,
              MUIA_Window_Title,      tr(MSG_WIN_TITLE_GLOW_COLOR),
              MUIA_ShortHelp,         tr(MSG_HELP_GLOW_COLOR),
            End,

          End,

        End,

        Child, HGroup, GroupFrameT(tr(MSG_GROUP_STYLE)),

          Child, ColGroup( 2 ),

            Child, KeyLabel(tr(MSG_BUTTON_LABEL_LINETYPES), msg_linetypes_key),
            Child, data->CY_LineType = CycleObject,
              MUIA_Cycle_Entries,  CY_LineTypes_Entries,
              MUIA_Cycle_Active,   MUICFGV_NListtree_LineType_Default,
              MUIA_CycleChain,     TRUE,
              MUIA_ControlChar,    msg_linetypes_key,
              MUIA_ShortHelp,      tr(MSG_HELP_LINETYPES),
            End,

            Child, KeyLabel(tr(MSG_BUTTON_LABEL_FOLDERIMAGE), msg_folderimage_key),
            Child, HGroup,
              Child, data->CH_UseFolderImage = ImageObject,
                ImageButtonFrame,
                MUIA_Image_Spec,    MUII_CheckMark,
                MUIA_InputMode,     MUIV_InputMode_Toggle,
                MUIA_Selected,      MUICFGV_NListtree_UseFolderImage_Default,
                MUIA_ShowSelState,  FALSE,
                MUIA_ControlChar,   msg_folderimage_key,
                MUIA_Background,    MUII_ButtonBack,
                MUIA_CycleChain,    TRUE,
                MUIA_ShortHelp,     tr(MSG_HELP_FOLDERIMAGE),
              End,
              Child, HVSpace,
            End,

            Child, KeyLabel(tr(MSG_BUTTON_LABEL_INDENTWIDTH), msg_indentwidth_key),
            Child, data->SL_IndentWidth = SliderObject,
              MUIA_Group_Horiz,     TRUE,
              MUIA_Slider_Min,      0,
              MUIA_Slider_Max,      16,
              MUIA_Slider_Level,    MUICFGV_NListtree_IndentWidth_Default,
              MUIA_Numeric_Format,  "%ldpx",
              MUIA_CycleChain,      TRUE,
              MUIA_ControlChar,     msg_indentwidth_key,
              MUIA_ShortHelp,       tr(MSG_HELP_INDENTWIDTH),
            End,

            Child, KeyLabel(tr(MSG_BUTTON_LABEL_REMEMBER), msg_remember_status_key),
            Child, HGroup,
              Child, data->CH_RememberStatus = ImageObject,
                ImageButtonFrame,
                MUIA_Image_Spec,    MUII_CheckMark,
                MUIA_InputMode,     MUIV_InputMode_Toggle,
                MUIA_Selected,      MUICFGV_NListtree_RememberStatus_Default,
                MUIA_ShowSelState,  FALSE,
                MUIA_ControlChar,   msg_remember_status_key,
                MUIA_Background,    MUII_ButtonBack,
                MUIA_CycleChain,    TRUE,
                MUIA_ShortHelp,     tr(MSG_HELP_REMEMBER),
              End,
              Child, HVSpace,
            End,

            Child, KeyLabel(tr(MSG_BUTTON_LABEL_AUTOSCROLL), msg_open_autoscroll_key),
            Child, HGroup,
              Child, data->CH_OpenAutoScroll = ImageObject,
                ImageButtonFrame,
                MUIA_Image_Spec,    MUII_CheckMark,
                MUIA_InputMode,     MUIV_InputMode_Toggle,
                MUIA_Selected,      MUICFGV_NListtree_OpenAutoScroll_Default,
                MUIA_ShowSelState,  FALSE,
                MUIA_ControlChar,   msg_open_autoscroll_key,
                MUIA_Background,    MUII_ButtonBack,
                MUIA_CycleChain,    TRUE,
                MUIA_ShortHelp,     tr(MSG_HELP_AUTOSCROLL),
              End,
              Child, HVSpace,
            End,

          End,

        End,

      End,


      /*
      **  Example
      */
      Child, VGroup, GroupFrameT(tr(MSG_TAB_EXAMPLE)),
        MUIA_HorizWeight, 99,

        Child, NListviewObject,
          MUIA_ShortHelp,                tr(MSG_HELP_LISTVIEW_EXAMPLE),
          MUIA_NListview_NList,          data->NLT_Sample = NListtreeObject,
            MUIA_Frame,                  MUIV_Frame_InputList,
            MUIA_CycleChain,             TRUE,
            MUIA_NListtree_IsMCP,        TRUE,
            MUIA_NListtree_DisplayHook,  &dsphook,
            MUIA_NListtree_CompareHook,  MUIV_NListtree_CompareHook_LeavesTop,
            MUIA_NListtree_DoubleClick,  MUIV_NListtree_DoubleClick_All,
            MUIA_NListtree_EmptyNodes,   FALSE,
            MUIA_NListtree_TreeColumn,   0,
            MUIA_NListtree_Title,        TRUE,
            MUIA_NListtree_Format,       ",",
          End,
        End,

        Child, HGroup,
          Child, data->BT_Sample_Expand    = KeyButton(tr(MSG_BUTTON_LABEL_EXPAND), msg_bt_expand_key),
          Child, data->BT_Sample_Collapse  = KeyButton(tr(MSG_BUTTON_LABEL_COLLAPSE), msg_bt_collapse_key),
        End,

      End,

    End,

    Child, CrawlingObject,
      TextFrame,
      MUIA_FixHeightTxt, "\n\n",
      MUIA_Background,   "m1",

      Child, TextObject,
        MUIA_Text_PreParse, "\033c",
        MUIA_Text_Contents, infotext,
      End,

      Child, TextObject,
        MUIA_Text_PreParse, "\033c",
        MUIA_Text_Contents, infotext,
      End,
    End,

  End;

  if ( data->GR_Prefs == NULL )
  {
    CoerceMethod( cl, obj, OM_DISPOSE );
    return( 0 );
  }

  DrawSampleTree( data->NLT_Sample );

  DoMethod( obj, OM_ADDMEMBER, data->GR_Prefs );


  /*
  **  Open/close all nodes
  */

  DoMethod( data->BT_Sample_Expand, MUIM_Notify, MUIA_Pressed, FALSE,
    data->NLT_Sample, 4, MUIM_NListtree_Open, MUIV_NListtree_Open_ListNode_Root, MUIV_NListtree_Open_TreeNode_All, 0 );

  DoMethod( data->BT_Sample_Collapse, MUIM_Notify, MUIA_Pressed, FALSE,
    data->NLT_Sample, 4, MUIM_NListtree_Close, MUIV_NListtree_Close_ListNode_Root, MUIV_NListtree_Close_TreeNode_All, 0 );

  /*
  **  Open/closed node and special images
  */
  DoMethod( data->PI_ImageClosed, MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_ImageSpecClosed, MUIV_TriggerValue );

  DoMethod( data->PI_ImageOpen, MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_ImageSpecOpen, MUIV_TriggerValue );

  DoMethod( data->PI_ImageFolder, MUIM_Notify, MUIA_Imagedisplay_Spec, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_ImageSpecFolder, MUIV_TriggerValue );

  /*
  **  Colors
  */

  DoMethod( data->PP_LinePen, MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_PenSpecLines, MUIV_TriggerValue );

  DoMethod( data->PP_ShadowPen, MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_PenSpecShadow, MUIV_TriggerValue );

  DoMethod( data->PP_GlowPen, MUIM_Notify, MUIA_Pendisplay_Spec, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_PenSpecGlow, MUIV_TriggerValue );

  /*
  **  Values
  */
  DoMethod( data->CH_RememberStatus, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_RememberStatus, MUIV_TriggerValue );

  DoMethod( data->SL_IndentWidth, MUIM_Notify, MUIA_Slider_Level, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_IndentWidth, MUIV_TriggerValue );

  DoMethod( data->CH_OpenAutoScroll, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_OpenAutoScroll, MUIV_TriggerValue );

  DoMethod( data->CY_LineType, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_LineType, MUIV_TriggerValue );

  DoMethod( data->CH_UseFolderImage, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
    data->NLT_Sample, 3, MUIM_Set, MUICFG_NListtree_UseFolderImage, MUIV_TriggerValue );

  // connect some values to the changedhook so that disabled status things
  // get updated
  DoMethod(data->CH_UseFolderImage, MUIM_Notify, MUIA_Selected, MUIV_EveryTime,
    data->NLT_Sample, 4, MUIM_CallHook, &ValuesChangedHook, data);
  DoMethod( data->CY_LineType, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
    data->NLT_Sample, 4, MUIM_CallHook, &ValuesChangedHook, data);

  // in case we are running for a newer MUI version we can register
  // our mcc gadgets accordingly
  if(MUIMasterBase->lib_Version >= 20)
  {
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->PI_ImageClosed,   MUICFG_NListtree_ImageSpecClosed, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->PI_ImageOpen,     MUICFG_NListtree_ImageSpecOpen, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->PI_ImageFolder,   MUICFG_NListtree_ImageSpecFolder, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->PP_LinePen,       MUICFG_NListtree_PenSpecLines, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->PP_ShadowPen,     MUICFG_NListtree_PenSpecShadow, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->PP_GlowPen,       MUICFG_NListtree_PenSpecGlow, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->CH_RememberStatus,MUICFG_NListtree_RememberStatus, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->SL_IndentWidth,   MUICFG_NListtree_IndentWidth, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->CH_OpenAutoScroll,MUICFG_NListtree_OpenAutoScroll, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->CY_LineType,      MUICFG_NListtree_LineType, 1, "");
    DoMethod(obj, MUIM_Mccprefs_RegisterGadget, data->CH_UseFolderImage,MUICFG_NListtree_UseFolderImage, 1, "");
  }

  return((IPTR)obj);
}


static IPTR _DisposeP( struct IClass *cl, Object *obj, Msg msg )
{
  struct NListtreeP_Data *data = INST_DATA( cl, obj );
    ULONG result;

  ENTER();

  DoMethod( obj, OM_REMMEMBER, data->GR_Prefs );

  if ( data->GR_Prefs )
    MUI_DisposeObject( data->GR_Prefs );

  result = DoSuperMethodA( cl, obj, msg );

    RETURN(result);
    return result;
}


static IPTR _SetupP( struct IClass *cl, Object *obj, struct MUIP_Setup *msg )
{
  struct NListtreeP_Data *data = INST_DATA( cl, obj );

  ENTER();

  if(!DoSuperMethodA(cl, obj, (Msg)msg))
  {
    RETURN(FALSE);
    return( FALSE );
  }

  /*
  **  Values
  **  We set the values here directly to the NL, because
  **  notification is only done, if values != old values.
  */
  TransferValues( data );

  RETURN(TRUE);
  return( TRUE );
}

static IPTR _ConfigToGadgets( struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_ConfigToGadgets *msg )
{
  struct NListtreeP_Data *data = INST_DATA(cl, obj);
  Object *pdobj, *idobj;
  struct MUI_ImageSpec *is;
  struct MUI_PenSpec *pen = NULL;
  IPTR d;

  ENTER();

  /*
  **  Create objects
  */
  pdobj = MUI_NewObject( MUIC_Pendisplay,    TAG_DONE );
  idobj = MUI_NewObject( MUIC_Imagedisplay,  TAG_DONE );

  D(DBF_ALWAYS, "pdobj: 0x%08lx, idobj: 0x%08lx", pdobj, idobj);


  /*
  **  Set images
  */
  if((is = (struct MUI_ImageSpec *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_ImageSpecClosed)))
  {
    set( data->PI_ImageClosed, MUIA_Imagedisplay_Spec, is );

    D(DBF_ALWAYS, "Closed node image: '%s'", (STRPTR)is);
  }
  else
  {
    if ( idobj )
    {
      DoMethod( idobj, MUIM_Set, MUIA_Imagedisplay_Spec, MUICFGV_NListtree_ImageSpecClosed_Default );

      get( idobj, MUIA_Imagedisplay_Spec, &is );
      set( data->PI_ImageClosed, MUIA_Imagedisplay_Spec, is );

      D(DBF_ALWAYS, "Closed node image: '%s'", (STRPTR)is);
    }
  }


  if((is = (struct MUI_ImageSpec *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_ImageSpecOpen)))
  {
    set( data->PI_ImageOpen, MUIA_Imagedisplay_Spec, is );

    D(DBF_ALWAYS, "Open node image: '%s'", (STRPTR)is);
  }
  else
  {
    if ( idobj )
    {
      DoMethod( idobj, MUIM_Set, MUIA_Imagedisplay_Spec, MUICFGV_NListtree_ImageSpecOpen_Default );

      get( idobj, MUIA_Imagedisplay_Spec, &is );
      set( data->PI_ImageOpen, MUIA_Imagedisplay_Spec, is );

      D(DBF_ALWAYS, "Open node image: '%s'", (STRPTR)is);
    }
  }


  if((is = (struct MUI_ImageSpec *)DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_ImageSpecFolder)))
  {
    set( data->PI_ImageFolder, MUIA_Imagedisplay_Spec, is );

    D(DBF_ALWAYS, "folder node image: '%s'", (STRPTR)is);
  }
  else
  {
    if ( idobj )
    {
      DoMethod( idobj, MUIM_Set, MUIA_Imagedisplay_Spec, MUICFGV_NListtree_ImageSpecFolder_Default );

      get( idobj, MUIA_Imagedisplay_Spec, &is );
      set( data->PI_ImageFolder, MUIA_Imagedisplay_Spec, is );

      D(DBF_ALWAYS, "folder node image: '%s'", (STRPTR)is);
    }
  }


  /*
  **  Set colors
  */
  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_PenSpecLines)))
  {
    set( data->PP_LinePen, MUIA_Pendisplay_Spec, d );

    D(DBF_ALWAYS, "Line color: '%s'", (STRPTR)d);
  }
  else
  {
    if( pdobj )
    {
      DoMethod( pdobj, MUIM_Pendisplay_SetMUIPen, MUICFGV_NListtree_PenSpecLines_Default );

      get( pdobj, MUIA_Pendisplay_Spec, &pen );
      set( data->PP_LinePen, MUIA_Pendisplay_Spec, pen );

      D(DBF_ALWAYS, "Line color: '%s'", pen);
    }
  }

  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_PenSpecShadow)))
  {
    set( data->PP_ShadowPen, MUIA_Pendisplay_Spec, d );

    D(DBF_ALWAYS, "Shadow color: '%s'", (STRPTR)d);
  }
  else
  {
    if( pdobj )
    {
      DoMethod( pdobj, MUIM_Pendisplay_SetMUIPen, MUICFGV_NListtree_PenSpecShadow_Default );

      get( pdobj, MUIA_Pendisplay_Spec, &pen );
      set( data->PP_ShadowPen, MUIA_Pendisplay_Spec, pen );

      D(DBF_ALWAYS, "Shadow color: '%s'", pen);
    }
  }

  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_PenSpecGlow)))
  {
    set( data->PP_GlowPen, MUIA_Pendisplay_Spec, d );

    D(DBF_ALWAYS, "Glow color: '%s'", (STRPTR)d);
  }
  else
  {
    if( pdobj )
    {
      DoMethod( pdobj, MUIM_Pendisplay_SetMUIPen, MUICFGV_NListtree_PenSpecGlow_Default );

      get( pdobj, MUIA_Pendisplay_Spec, &pen );
      set( data->PP_GlowPen, MUIA_Pendisplay_Spec, pen );

      D(DBF_ALWAYS, "Glow color: '%s'", pen);
    }
  }

  /*
  **  Set values
  */
  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_LineType)))
  {
    set( data->CY_LineType, MUIA_Cycle_Active, atoi( (STRPTR)d ) );
    ValuesChanged(data);

    D(DBF_ALWAYS, "LineType: %ld", atoi( (STRPTR)d ));
  }
  else
  {
    set( data->CY_LineType, MUIA_Cycle_Active, MUICFGV_NListtree_LineType_Default );
    ValuesChanged(data);

    D(DBF_ALWAYS, "Style: %ld", 0);
  }

  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_IndentWidth)))
  {
    set( data->SL_IndentWidth, MUIA_Slider_Level, atoi( (STRPTR)d ) );

    D(DBF_ALWAYS, "IndentWidth: %ld", atoi( (STRPTR)d ));
  }
  else
  {
    set( data->SL_IndentWidth, MUIA_Slider_Level, MUICFGV_NListtree_IndentWidth_Default );

    D(DBF_ALWAYS, "IndentWidth: %ld", MUICFGV_NListtree_IndentWidth_Default);
  }

  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_RememberStatus)))
  {
    set( data->CH_RememberStatus, MUIA_Selected, atoi( (STRPTR)d ) );

    D(DBF_ALWAYS, "RememberStatus: %ld", atoi( (STRPTR)d ));
  }
  else
  {
    set( data->CH_RememberStatus, MUIA_Selected, MUICFGV_NListtree_RememberStatus_Default );

    D(DBF_ALWAYS, "RememberStatus: %ld", MUICFGV_NListtree_RememberStatus_Default);
  }

  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_OpenAutoScroll)))
  {
    set( data->CH_OpenAutoScroll, MUIA_Selected, atoi( (STRPTR)d ) );

    D(DBF_ALWAYS, "OpenAutoScroll: %ld", atoi( (STRPTR)d ));
  }
  else
  {
    set( data->CH_OpenAutoScroll, MUIA_Selected, MUICFGV_NListtree_OpenAutoScroll_Default );

    D(DBF_ALWAYS, "OpenAutoScroll: %ld", MUICFGV_NListtree_OpenAutoScroll_Default);
  }

  if((d = DoMethod(msg->configdata, MUIM_Dataspace_Find, MUICFG_NListtree_UseFolderImage)))
  {
    set( data->CH_UseFolderImage, MUIA_Selected, atoi( (STRPTR)d ) );

    D(DBF_ALWAYS, "UseFolderImage: %ld", atoi( (STRPTR)d ));
  }
  else
  {
    set( data->CH_UseFolderImage, MUIA_Selected, MUICFGV_NListtree_UseFolderImage_Default );

    D(DBF_ALWAYS, "UseFolderImage: %ld", MUICFGV_NListtree_UseFolderImage_Default);
  }

  /*
  **  Dispose created objects
  */
  if( pdobj )
    MUI_DisposeObject( pdobj );
  if( idobj )
    MUI_DisposeObject( idobj );

  RETURN(0);
  return( 0 );
}


static IPTR _GadgetsToConfig( struct IClass *cl, Object *obj, struct MUIP_Settingsgroup_GadgetsToConfig *msg )
{
  struct NListtreeP_Data *data = INST_DATA( cl, obj );
  char buf[8];
  ULONG d=0;

  ENTER();

  /*
  **  Images
  */
  get( data->PI_ImageClosed,  MUIA_Imagedisplay_Spec,  &d );
  if( d != 0)
    DoMethod( msg->configdata, MUIM_Dataspace_Add, d, sizeof( struct MUI_ImageSpec ), MUICFG_NListtree_ImageSpecClosed );

  D(DBF_ALWAYS, "Image closed: '%s'", (STRPTR)d);


  get( data->PI_ImageOpen,  MUIA_Imagedisplay_Spec,  &d );
  if(d != 0)
    DoMethod( msg->configdata, MUIM_Dataspace_Add, d, sizeof( struct MUI_ImageSpec ), MUICFG_NListtree_ImageSpecOpen );

  D(DBF_ALWAYS, "Image open: '%s'", (STRPTR)d);


  get( data->PI_ImageFolder,  MUIA_Imagedisplay_Spec,  &d );
  if(d != 0)
    DoMethod( msg->configdata, MUIM_Dataspace_Add, d, sizeof( struct MUI_ImageSpec ), MUICFG_NListtree_ImageSpecFolder );

  D(DBF_ALWAYS, "Image folder: '%s'", (STRPTR)d);


  /*
  **  Colors
  */
  get( data->PP_LinePen, MUIA_Pendisplay_Spec, &d );
  if(d != 0)
    DoMethod( msg->configdata, MUIM_Dataspace_Add, d, sizeof( struct MUI_PenSpec ), MUICFG_NListtree_PenSpecLines );

  D(DBF_ALWAYS, "Line color: '%s'", (STRPTR)d);


  get( data->PP_ShadowPen, MUIA_Pendisplay_Spec, &d );
  if(d != 0)
    DoMethod( msg->configdata, MUIM_Dataspace_Add, d, sizeof( struct MUI_PenSpec ), MUICFG_NListtree_PenSpecShadow );

  D(DBF_ALWAYS, "Shadow color: '%s'", (STRPTR)d);


  get( data->PP_GlowPen, MUIA_Pendisplay_Spec, &d );
  if(d != 0)
    DoMethod( msg->configdata, MUIM_Dataspace_Add, d, sizeof( struct MUI_PenSpec ), MUICFG_NListtree_PenSpecGlow );

  D(DBF_ALWAYS, "Glow color: '%s'", (STRPTR)d);

  /*
  **  Values
  */


  get( data->CH_RememberStatus, MUIA_Selected, &d );
  snprintf(buf, sizeof(buf), "%d", (int)d);
  DoMethod( msg->configdata, MUIM_Dataspace_Add, buf, 5, MUICFG_NListtree_RememberStatus );

  D(DBF_ALWAYS, "RememberStatus: %ld", d);


  get( data->SL_IndentWidth, MUIA_Slider_Level, &d );
  snprintf(buf, sizeof(buf), "%d", (int)d);
  DoMethod( msg->configdata, MUIM_Dataspace_Add, buf, 5, MUICFG_NListtree_IndentWidth );

  D(DBF_ALWAYS, "IndentWidth: %ld", d);

  get( data->CH_OpenAutoScroll, MUIA_Selected, &d );
  snprintf(buf, sizeof(buf), "%d", (int)d);
  DoMethod( msg->configdata, MUIM_Dataspace_Add, buf, 5, MUICFG_NListtree_OpenAutoScroll );

  D(DBF_ALWAYS, "OpenAutoScroll: %ld", d);

  get( data->CY_LineType, MUIA_Cycle_Active, &d );    
  snprintf(buf, sizeof(buf), "%d", (int)d);   
  DoMethod( msg->configdata, MUIM_Dataspace_Add, buf, 5, MUICFG_NListtree_LineType );    
               
  D(DBF_ALWAYS, "LineType: %ld", d);

  get( data->CH_UseFolderImage, MUIA_Selected, &d );
  snprintf(buf, sizeof(buf), "%d", (int)d);
  DoMethod( msg->configdata, MUIM_Dataspace_Add, buf, 5, MUICFG_NListtree_UseFolderImage );

  D(DBF_ALWAYS, "UseFolderImage: %ld", d);

  RETURN(0);
  return( 0 );
}


static IPTR _HandleInputP( struct IClass *cl, Object *obj, struct MUIP_HandleInput *msg )
{
  if( msg->imsg )
  {
    switch( msg->imsg->Class )
    {
      case IDCMP_INTUITICKS:
        break;
    }
  }

  return( DoSuperMethodA( cl, obj, (Msg)msg ) );
}

DISPATCHER(_DispatcherP)
{
  switch(msg->MethodID)
  {
    case OM_NEW:              return(_NewP(cl, obj, (APTR)msg));
    case OM_DISPOSE:          return(_DisposeP(cl, obj, (APTR)msg));

    case MUIM_Setup:          return(_SetupP(cl, obj, (APTR)msg));
    case MUIM_HandleInput:    return(_HandleInputP(cl, obj, (APTR)msg));

    case MUIM_Settingsgroup_ConfigToGadgets:  return(_ConfigToGadgets(cl, obj, (APTR)msg));
    case MUIM_Settingsgroup_GadgetsToConfig:  return(_GadgetsToConfig(cl, obj, (APTR)msg));
  }

  return( DoSuperMethodA( cl, obj, msg ) );
}
