/*
  Copyright  2004-2008, The AROS Development Team. All rights reserved.
  $Id$
*/

#include "portable_macros.h"
#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

#define ICONWINDOW_OPTION_NOSEARCHBUTTON
//#define ICONWINDOW_BUFFERLIST

#ifdef __AROS__
#define DEBUG 0
#include <aros/debug.h>
#endif

#include <exec/types.h>
#include <libraries/mui.h>

#ifdef __AROS__
#include <zune/customclasses.h>
#else
#include <zune_AROS/customclasses.h>
#endif

#include <proto/utility.h>

#include <proto/graphics.h>

#include <proto/exec.h>
#include <proto/datatypes.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include <stdio.h>
#include <string.h>

#include <intuition/screens.h>
#include <datatypes/pictureclass.h>
#include <clib/macros.h>

#ifdef __AROS__
#include <clib/alib_protos.h>

#include <prefs/wanderer.h>
#else
#include <prefs_AROS/wanderer.h>
#endif

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#ifndef _PROTO_INTUITION_H
#include <proto/intuition.h>
#endif
#include <proto/muimaster.h>

#include "Classes/iconlist.h"
#include "Classes/iconlistview.h"
#include "Classes/iconlist_attributes.h"
#include "wanderer.h"
#include "wandererprefs.h"

#include "iconwindow.h"
#include "iconwindow_attributes.h"
#include "iconwindowcontents.h"
#include "iconwindowbackfill.h"

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

static char __intern_wintitle_wanderer[] = "Wanderer";

/*** Private Global Data *********************************************************/

static struct List                     iconwindow_BackFillNodes;
struct IconWindow_BackFill_Descriptor  *iconwindow_BackFill_Active = NULL;

/*** Hook functions *********************************************************/

///IconWindow__HookFunc_ToolbarLocationStringFunc()
#ifdef __AROS__
AROS_UFH3(
  void, IconWindow__HookFunc_ToolbarLocationStringFunc,
  AROS_UFHA(struct Hook *,    hook,   A0),
  AROS_UFHA(APTR *,           obj,    A2),
  AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(IconWindow__HookFunc_ToolbarLocationStringFunc, void, APTR *obj, APTR param)
{
#endif
  AROS_USERFUNC_INIT

  /* Get data */
  Object                *self = ( Object *)obj;
  Class                 *CLASS = *( Class **)param;
  STRPTR                str = NULL;
  BPTR                  fp = (BPTR) NULL;
  struct FileInfoBlock  *fib;

#warning "stegerg: doesn't allocate fib with AllocDOSObject"

  SETUP_ICONWINDOW_INST_DATA;

  /* Only change dir if it is a valid directory/volume */
  GET(data->iwd_Toolbar_LocationStringObj, MUIA_String_Contents, &str);

#warning "TODO: Signal that it is a wrong path"
  /* so that the user understands (here where we abort with return) */

  fib = AllocDosObject(DOS_FIB, NULL);
  if (!fib)
    return;
  
  if (!(fp = Lock(str, ACCESS_READ)))
  {
    FreeDosObject(DOS_FIB, fib);
    return;
  }

  if (!(Examine(fp, fib)))
  {
    UnLock (fp );
  FreeDosObject(DOS_FIB, fib);
    return;
  }

  /* Change directory! */
  if (fib->fib_DirEntryType >= 0)
    SET(self, MUIA_IconWindow_Location, (IPTR)str);

  UnLock(fp);
  
  FreeDosObject(DOS_FIB, fib);
  
  AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(iwd_pathStrHook,IconWindow__HookFunc_ToolbarLocationStringFunc);
#endif

///

///IconWindow__HookFunc_PrefsUpdatedFunc()
#ifdef __AROS__
AROS_UFH3(
  void, IconWindow__HookFunc_PrefsUpdatedFunc,
  AROS_UFHA(struct Hook *,    hook,   A0),
  AROS_UFHA(APTR *,           obj,    A2),
  AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(IconWindow__HookFunc_PrefsUpdatedFunc, void, APTR *obj, APTR param)
{
#endif
  AROS_USERFUNC_INIT
  
  /* Get our private data */
  Object *self = ( Object *)obj;
  Class *CLASS = *( Class **)param;
  IPTR changed_state;

  SETUP_ICONWINDOW_INST_DATA;

  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  changed_state = 0;
  GET(self, MUIA_IconWindow_Changed, &changed_state);

  if ((changed_state) && (data->iwd_IconListObj))
  {
    D(bug("[IconWindow] %s: Window contents have changed .. updating display ..\n", __PRETTY_FUNCTION__));
    DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);
    SET(self, MUIA_IconWindow_Changed, FALSE);
  }

  AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(iwd_PrefsUpdated_hook,IconWindow__HookFunc_PrefsUpdatedFunc);
#endif
///

///IconWindow__HookFunc_ProcessBackgroundFunc()
#ifdef __AROS__
AROS_UFH3(
  void, IconWindow__HookFunc_ProcessBackgroundFunc,
  AROS_UFHA(struct Hook *,    hook,   A0),
  AROS_UFHA(APTR *,           obj,    A2),
  AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(IconWindow__HookFunc_ProcessBackgroundFunc, void, APTR *obj, APTR param)
{
#endif
  AROS_USERFUNC_INIT
  
  /* Get our private data */
  Object *self = ( Object *)obj,
       *prefs = NULL;
  Class *CLASS = *( Class **)param;

  SETUP_ICONWINDOW_INST_DATA;

  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  DoMethod(self, MUIM_IconWindow_BackFill_ProcessBackground, data->iwd_BackFillInfo, data->iwd_RootViewObj);

  GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

  if (prefs)
  {
//    BOOL    options_changed = FALSE;
    IPTR  prefs_Processing = 0;

    GET(prefs, MUIA_WandererPrefs_Processing, &prefs_Processing);
    if (!prefs_Processing)
    {
#warning "TODO: We arent in prefs-processing so cause an update!"
    }
  }

  AROS_USERFUNC_EXIT
}
MakeStaticHook(iwd_ProcessBackground_hook,IconWindow__HookFunc_ProcessBackgroundFunc);
///

///IconWindow__HookFunc_WandererBackFillFunc()
#ifdef __AROS__
AROS_UFH3(
    void, IconWindow__HookFunc_WandererBackFillFunc,
    AROS_UFHA(struct Hook *,        hook,   A0),
    AROS_UFHA(struct RastPort *,    RP,    A2),
    AROS_UFHA(struct BackFillMsg *, BFM,  A1)
)
{
#else
HOOKPROTO(IconWindow__HookFunc_WandererBackFillFunc, void, struct RastPort *RP, struct BackFillMsg *BFM)
{
#endif
    AROS_USERFUNC_INIT
  
    struct IconWindow_BackFillHookData *HookData = NULL;

D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
    
    if ((HookData = hook->h_Data) && (iconwindow_BackFill_Active != NULL))
    {
        Class                              *CLASS = HookData->bfhd_IWClass;
        Object                 *self = HookData->bfhd_IWObject;

        SETUP_ICONWINDOW_INST_DATA;

        struct RastPort           *DrawBackGround_RastPort = NULL;
        struct IconWindowBackFillMsg  DrawBackGround_BackFillMsg;

        GET(data->iwd_IconListObj, MUIA_IconList_BufferRastport, &DrawBackGround_RastPort);

        if (DrawBackGround_RastPort != NULL)
        {
            if ((DrawBackGround_BackFillMsg.Layer = DrawBackGround_RastPort->Layer) == NULL)
            {
  D(bug("[IconWindow] %s: Rastport Layer = NULL!\n", __PRETTY_FUNCTION__));
            }

            GET(data->iwd_IconListObj,MUIA_IconList_BufferLeft, &DrawBackGround_BackFillMsg.AreaBounds.MinX);
            GET(data->iwd_IconListObj,MUIA_IconList_BufferTop, &DrawBackGround_BackFillMsg.AreaBounds.MinY);
            GET(data->iwd_IconListObj,MUIA_IconList_BufferWidth, &DrawBackGround_BackFillMsg.AreaBounds.MaxX);
            GET(data->iwd_IconListObj,MUIA_IconList_BufferHeight, &DrawBackGround_BackFillMsg.AreaBounds.MaxY);

            DrawBackGround_BackFillMsg.AreaBounds.MaxX += DrawBackGround_BackFillMsg.AreaBounds.MinX - 1;
            DrawBackGround_BackFillMsg.AreaBounds.MaxY += DrawBackGround_BackFillMsg.AreaBounds.MinY - 1;

            DrawBackGround_BackFillMsg.DrawBounds.MinX = BFM->Bounds.MinX;
            DrawBackGround_BackFillMsg.DrawBounds.MinY = BFM->Bounds.MinY;
            DrawBackGround_BackFillMsg.DrawBounds.MaxX = BFM->Bounds.MaxX;
            DrawBackGround_BackFillMsg.DrawBounds.MaxY = BFM->Bounds.MaxY;

            /* Offset into source image (ala scroll bar position) */
            DrawBackGround_BackFillMsg.OffsetX = BFM->OffsetX;
            DrawBackGround_BackFillMsg.OffsetY = BFM->OffsetY;
            DoMethod(self, MUIM_IconWindow_BackFill_DrawBackground, data->iwd_BackFillInfo, &DrawBackGround_BackFillMsg, DrawBackGround_RastPort);
        }

    }

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_WandererBackFillFunc,IconWindow__HookFunc_WandererBackFillFunc);
#endif
///

/*** Methods ****************************************************************/
///IconWindow__SetupToolbar()
void IconWindow__SetupToolbar(Class *CLASS, Object *self, Object *prefs)
{
  SETUP_ICONWINDOW_INST_DATA;

  Object          *strObj = NULL,
          *bt_dirup = NULL;
  Object *toolbarPanel;
  Object          *bt_search = NULL;
#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
  Object          *bt_search = ImageButton("", "THEME:Images/Gadgets/Prefs/Test");
#endif


  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  D(bug("[IconWindow] %s: App PrefsObj @ 0x%p\n", __PRETTY_FUNCTION__, prefs));

  if (prefs != NULL)
  {
    data->iwd_Toolbar_PrefsNotificationObject =(Object *) DoMethod(prefs,
                              MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                              (STRPTR) "Toolbar");
    //Set up our prefs notification handlers ..

  }

  /* Create the "ToolBar" panel object .. */
  toolbarPanel = MUI_NewObject(MUIC_Group,
    MUIA_InnerLeft,(0),
    MUIA_InnerRight,(0),
    MUIA_InnerTop,(0),
    MUIA_InnerBottom,(0),
    MUIA_Frame, MUIV_Frame_None,
    Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
      MUIA_InnerLeft,(4),
      MUIA_InnerRight,(4),
      MUIA_InnerTop,(4),
      MUIA_InnerBottom,(4),
      MUIA_Frame, MUIV_Frame_None,
      MUIA_Weight, 100,
      Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
        MUIA_InnerLeft,(0),
        MUIA_InnerRight,(0),
        MUIA_InnerTop,(0),
        MUIA_InnerBottom,(0),
        MUIA_Weight, 100,
        Child, (IPTR)( strObj = MUI_NewObject(MUIC_String,
          MUIA_String_Contents, (IPTR)"",
          MUIA_CycleChain, 1,
          MUIA_Frame, MUIV_Frame_String,
        TAG_DONE) ),
      TAG_DONE),
      Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
        MUIA_InnerLeft,(0),
        MUIA_InnerRight,(0),
        MUIA_InnerTop,(0),
        MUIA_InnerBottom,(0),
        MUIA_HorizWeight,   0,
        MUIA_VertWeight,    100,
        Child, (IPTR) (bt_dirup = ImageButton("", "THEME:Images/Gadgets/Prefs/Revert")),
        (bt_search ? Child : TAG_IGNORE), (IPTR) (bt_search),
      TAG_DONE),
    TAG_DONE),
    Child, (IPTR)MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
      MUIA_InnerLeft,(0),
      MUIA_InnerRight,(0),
      MUIA_InnerTop,(0),
      MUIA_InnerBottom,(0),
      MUIA_Group_Spacing, 0,
      MUIA_FixHeight, 1,
      MUIA_Frame, MUIV_Frame_None,
      MUIA_Background, MUII_SHADOW,
      Child, (IPTR)MUI_NewObject(MUIC_Rectangle,
        MUIA_Frame, MUIV_Frame_None,
      TAG_DONE),
    TAG_DONE),
  TAG_DONE);
  
  /* Got a toolbarpanel? setup notifies and other values are 
     copied to the data of the object */
  if ( toolbarPanel != NULL )
  {
    SET(bt_dirup, MUIA_Background, XGET( toolbarPanel, MUIA_Background ) );
    SET(bt_dirup, MUIA_CycleChain, 1);
    SET(bt_dirup, MUIA_Frame, MUIV_Frame_None );
#if !defined(ICONWINDOW_OPTION_NOSEARCHBUTTON)
    SET(bt_search, MUIA_Background, XGET( toolbarPanel, MUIA_Background ) );
    SET(bt_search, MUIA_CycleChain, 1);
    SET(bt_search, MUIA_Frame, MUIV_Frame_None );
#endif

    if (DoMethod( data->iwd_ExtensionGroupObj, MUIM_Group_InitChange ))
    {
      DoMethod(data->iwd_ExtensionGroupObj, OM_ADDMEMBER, (IPTR)toolbarPanel);
      if (data->iwd_ExtensionGroupSpacerObj)
      {
        DoMethod(data->iwd_ExtensionGroupObj, OM_REMMEMBER, (IPTR)data->iwd_ExtensionGroupSpacerObj);
        data->iwd_ExtensionGroupSpacerObj = NULL;
      }

      DoMethod(data->iwd_ExtensionGroupObj, MUIM_Group_ExitChange);
      data->iwd_Toolbar_PanelObj = toolbarPanel;
    }

    if (data->iwd_Toolbar_PanelObj)
    {
      DoMethod( 
        bt_dirup, MUIM_Notify, MUIA_Pressed, FALSE, 
        (IPTR)self, 1, MUIM_IconWindow_DirectoryUp
      );

      data->iwd_Toolbar_LocationStringObj = strObj;
      #ifdef __AROS__
      data->iwd_pathStrHook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_ToolbarLocationStringFunc;
      #else
      data->iwd_pathStrHook= &iwd_pathStrHook;
      #endif

      SET(
        data->iwd_Toolbar_LocationStringObj, MUIA_String_Contents, 
        XGET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer)
      );

      /* Make changes to string contents change dir on enter */
      DoMethod ( 
        strObj, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime, 
        (IPTR)self, 3, MUIM_CallHook, &data->iwd_pathStrHook, (IPTR)CLASS
      );
    }
  }
  else
  {
    data->iwd_Toolbar_PanelObj = NULL;
  }
}
///

///OM_NEW()
Object *IconWindow__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
  struct Screen                   *_newIconWin__Screen = NULL;
  Object                          *_newIconWin__IconListObj = NULL,
                                  *_newIconWin__RootViewObj = NULL,
                                  *_newIconWin__ExtensionContainerObj = NULL, // around extension group
                                  *_newIconWin__ExtensionGroupObj = NULL,     // extension group top
                                  *_newIconWin__ExtensionGroupSpacerObj = NULL,     // extension group top
                                  *prefs = NULL;

  char                            *_newIconWin__Title = NULL;
  
  BOOL                            isRoot = FALSE,
                                  isBackdrop = FALSE,
                                  hasToolbar = FALSE;

  struct Hook                     *actionHook = NULL;
  struct TextFont                 *_newIconWin__WindowFont = NULL;

  struct Hook                     *_newIconWin__BackFillHook = NULL;

  APTR                            WindowBF_TAG =(APTR) TAG_IGNORE;

  IPTR                            _newIconWin__WindowWidth = 0;
  IPTR                            _newIconWin__WindowHeight = 0;
  IPTR                            _newIconWin__WindowLeft = 0;
  IPTR                            _newIconWin__WindowTop = 0;

  IPTR                            _newIconWin__FSNotifyPort =(IPTR) NULL;

  D(bug("[iconwindow]: %s()\n", __PRETTY_FUNCTION__));

  /* More than one GetTagData is not very efficient, however since this isn't called very often... */

  isBackdrop = (BOOL)GetTagData(MUIA_IconWindow_IsBackdrop, (IPTR)FALSE, message->ops_AttrList);

  if (!(isRoot = (BOOL)GetTagData(MUIA_IconWindow_IsRoot, (IPTR)FALSE, message->ops_AttrList)))
    hasToolbar = (BOOL)GetTagData(MUIA_IconWindowExt_Toolbar_Enabled, (IPTR)FALSE, message->ops_AttrList);

  actionHook = (struct Hook *)GetTagData(MUIA_IconWindow_ActionHook, (IPTR) NULL, message->ops_AttrList);
  _newIconWin__WindowFont = (struct TextFont *)GetTagData(MUIA_IconWindow_Font, (IPTR) NULL, message->ops_AttrList);
  prefs = (Object *)GetTagData(MUIA_Wanderer_Prefs, (IPTR) NULL, message->ops_AttrList);

  _newIconWin__FSNotifyPort = (IPTR)GetTagData(MUIA_Wanderer_FileSysNotifyPort, (IPTR) NULL, message->ops_AttrList);

  /* Request the screen we should use .. */

  if (!(_newIconWin__Screen = (struct Screen *)GetTagData(MUIA_Wanderer_Screen, (IPTR) NULL, message->ops_AttrList)))
  {
    D(bug("[IconWindow] %s: NO SCREEN SET!\n", __PRETTY_FUNCTION__));
    return NULL;
  }
  D(bug("[iconwindow] %s: Screen @ 0x%x\n", __PRETTY_FUNCTION__, _newIconWin__Screen));

  if ((_newIconWin__BackFillHook = AllocVec(sizeof(struct Hook), MEMF_CLEAR|MEMF_PUBLIC))!=NULL)
  {
    D(bug("[IconWindow] %s: Allocated WindowBackFillHook @ 0x%p\n", __PRETTY_FUNCTION__, _newIconWin__BackFillHook));

    #ifdef __AROS__
    _newIconWin__BackFillHook->h_Entry = ( HOOKFUNC )IconWindow__HookFunc_WandererBackFillFunc;
    #else
    _newIconWin__BackFillHook = &Hook_WandererBackFillFunc;
    #endif

//#if defined(__MORPHOS__)
//    WindowBF_TAG = MUIA_Window_BackFillHook;
//#else
    WindowBF_TAG =(APTR) WA_BackFill;
//#endif
  }

  if (isRoot)
  {
    #ifdef __AROS__
        _newIconWin__IconListObj = (Object *)IconWindowIconVolumeListObject,
                                       MUIA_Font, (IPTR)_newIconWin__WindowFont,
                                    End;
    #else
    _newIconWin__IconListObj = (Object *)NewObject(IconWindowIconVolumeList_CLASS->mcc_Class, NULL,
                                           MUIA_Font, (IPTR)_newIconWin__WindowFont,
                                TAG_DONE);
    #endif

                         

    _newIconWin__WindowWidth = GetBitMapAttr(_newIconWin__Screen->RastPort.BitMap, BMA_WIDTH);
    _newIconWin__WindowHeight = GetBitMapAttr(_newIconWin__Screen->RastPort.BitMap, BMA_HEIGHT);
    D(bug("[iconwindow] %s: Screen dimensions ..  %d x %d\n", __PRETTY_FUNCTION__, _newIconWin__WindowWidth, _newIconWin__WindowHeight));

    if (isBackdrop)
    {
      D(bug("[iconwindow] %s: BACKDROP ROOT Window\n", __PRETTY_FUNCTION__));
      _newIconWin__Title = NULL;
    }
    else
    {
      D(bug("[iconwindow] %s: Plain ROOT Window\n", __PRETTY_FUNCTION__));
      _newIconWin__Title = __intern_wintitle_wanderer;
    }
    
    _newIconWin__WindowTop = _newIconWin__Screen->BarHeight + 1;
    _newIconWin__WindowLeft = 0;
    
    _newIconWin__WindowHeight -= _newIconWin__WindowTop;
  }
  else
  {
    IPTR _newIconWin__TitleLen;
    BPTR                      dir_info_lock;
    char                      *dir_info_name;
    D(bug("[iconwindow] %s: Directory Window\n", __PRETTY_FUNCTION__));
    _newIconWin__Title = (STRPTR) GetTagData(MUIA_IconWindow_Location, (IPTR)NULL, message->ops_AttrList);
    _newIconWin__TitleLen = strlen(_newIconWin__Title);

    D(bug("[iconwindow] %s: Dir: '%s'\n", __PRETTY_FUNCTION__, _newIconWin__Title));

    #ifdef __AROS__
      _newIconWin__IconListObj = (Object *) IconWindowIconDrawerListObject ,
                                 MUIA_Font, (IPTR)_newIconWin__WindowFont,
                                 MUIA_IconDrawerList_Drawer, (IPTR) _newIconWin__Title,
                                 MUIA_Wanderer_FileSysNotifyPort, _newIconWin__FSNotifyPort,
                                End;
    #else
    _newIconWin__IconListObj = (Object *) NewObject(IconWindowIconDrawerList_CLASS->mcc_Class, NULL,
                                 MUIA_Font, (IPTR)_newIconWin__WindowFont,
                                 MUIA_IconDrawerList_Drawer, (IPTR) _newIconWin__Title,
                                 MUIA_Wanderer_FileSysNotifyPort, _newIconWin__FSNotifyPort,
                                TAG_DONE);
    #endif
                                

    if (_newIconWin__Title[_newIconWin__TitleLen - 1] == ':')
    {
      BPTR                      volume_info_lock = (BPTR) NULL;
      char                      *volume_info_name = NULL;

D(bug("[iconwindow] %s: Opening Volume Root Window\n", __PRETTY_FUNCTION__));

      if ((volume_info_name = AllocVec(_newIconWin__TitleLen + 10, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
      {
        sprintf(volume_info_name, "%sdisk.info\0", _newIconWin__Title);
        if ((volume_info_lock = Lock(volume_info_name, SHARED_LOCK)) !=(BPTR) NULL)
        {
          UnLock(volume_info_lock);
        }
        else
        {
          IPTR current_DispFlags = 0;
D(bug("[iconwindow] %s: No disk.info found - setting show all files\n", __PRETTY_FUNCTION__));

          GET(_newIconWin__IconListObj, MUIA_IconList_DisplayFlags, &current_DispFlags);
          current_DispFlags &= ~ICONLIST_DISP_SHOWINFO;
          SET(_newIconWin__IconListObj, MUIA_IconList_DisplayFlags, current_DispFlags);
        }
        FreeVec(volume_info_name);
      }
    }

    dir_info_lock = (BPTR) NULL;
    dir_info_name = NULL;

    if ((dir_info_name = AllocVec(_newIconWin__TitleLen + 7, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
    {
      sprintf(dir_info_name, "%s", _newIconWin__Title);
      AddPart(dir_info_name, ".info", _newIconWin__TitleLen + 7);

      if ((dir_info_lock = Lock(dir_info_name, SHARED_LOCK)) != (BPTR) NULL)
      {
        UnLock(dir_info_lock);
#warning "TODO: Read in the .info files coords and dimensions"
D(bug("[iconwindow] %s: Gettin window coords/dimensions from drawer '.info' file\n", __PRETTY_FUNCTION__));
      }
      else
      {
D(bug("[iconwindow] %s: No Drawer .info found - Using default dimensions/coords\n", __PRETTY_FUNCTION__));
        _newIconWin__WindowTop = MUIV_Window_TopEdge_Centered;
        _newIconWin__WindowLeft = MUIV_Window_LeftEdge_Centered;
        _newIconWin__WindowWidth = 300;
        _newIconWin__WindowHeight = 300;
      }
      FreeVec(dir_info_name);
    }

    _newIconWin__ExtensionGroupObj = MUI_NewObject(MUIC_Group,
        MUIA_InnerLeft,(0),
        MUIA_InnerRight,(0),
        MUIA_InnerTop,(0),
        MUIA_InnerBottom,(0),
        MUIA_Frame, MUIV_Frame_None,
        MUIA_Group_Spacing, 0,
        Child, (_newIconWin__ExtensionGroupSpacerObj = HSpace(0)),
    TAG_DONE);

    if (_newIconWin__ExtensionGroupObj)
    {
      _newIconWin__ExtensionContainerObj = MUI_NewObject(MUIC_Group, MUIA_Group_Horiz, TRUE                      ,
        InnerSpacing(0,0),
        MUIA_HorizWeight,   100,
        MUIA_VertWeight,    0,
        MUIA_Frame,         MUIV_Frame_None,
        MUIA_Group_Spacing, 3,
        /* extension on top of the list */
        Child, (IPTR)_newIconWin__ExtensionGroupObj,
      TAG_DONE);
    }
  }
  D(bug("[iconwindow] %s: Using dimensions ..  %d x %d\n", __PRETTY_FUNCTION__, _newIconWin__WindowWidth, _newIconWin__WindowHeight));

  #ifdef __AROS__
  _newIconWin__RootViewObj = (Object *) IconListviewObject,
                                    MUIA_Weight,                           100,
                                MUIA_IconListview_UseWinBorder,        TRUE,
                                MUIA_IconListview_IconList,     (IPTR) _newIconWin__IconListObj,
                            End;
  #else
  _newIconWin__RootViewObj = (Object *) NewObject(IconListview_Class->mcc_Class, NULL    ,
                                        MUIA_Weight,                           100,
                                MUIA_IconListview_UseWinBorder,        TRUE,
                                MUIA_IconListview_IconList,     (IPTR) _newIconWin__IconListObj,
                            TAG_DONE);
  #endif
                            

  D(bug("[iconwindow] %s: Font @ 0x%p\n", __PRETTY_FUNCTION__, _newIconWin__WindowFont));

  self = (Object *) DoSuperNewTags
  (
    CLASS, self, NULL,

    MUIA_Window_Screen,                                    _newIconWin__Screen,
    MUIA_Window_Backdrop,                                  isBackdrop ? TRUE : FALSE,
    MUIA_Window_Borderless,                                isBackdrop ? TRUE : FALSE,
    MUIA_Window_Width,                                     _newIconWin__WindowWidth,
    MUIA_Window_Height,                                    _newIconWin__WindowHeight,
    MUIA_Window_LeftEdge,                                  _newIconWin__WindowLeft,
    MUIA_Window_TopEdge,                                   _newIconWin__WindowTop,
    (!isBackdrop) ? MUIA_Window_AltWidth : TAG_IGNORE,     100,
    (!isBackdrop) ? MUIA_Window_AltHeight : TAG_IGNORE,    80,
    MUIA_Window_Title,                                   (IPTR)_newIconWin__Title,
    
    MUIA_Window_DragBar,                                   (!isBackdrop) ? TRUE : FALSE,
    MUIA_Window_CloseGadget,                               (!isBackdrop) ? TRUE : FALSE,
    MUIA_Window_SizeGadget,                                (!isBackdrop) ? TRUE : FALSE,
    MUIA_Window_DepthGadget,                               (!isBackdrop) ? TRUE : FALSE,
#if defined(MUIA_Window_ZoomGadget)
    MUIA_Window_ZoomGadget,                                (!isBackdrop) ? TRUE : FALSE,
#endif
    MUIA_Window_UseBottomBorderScroller,                   (!isBackdrop) ? TRUE : FALSE,
    MUIA_Window_UseRightBorderScroller,                    (!isBackdrop) ? TRUE : FALSE,
    MUIA_Window_IsSubWindow,                             TRUE,
    #ifdef __AROS__
    WindowBF_TAG,                                        _newIconWin__BackFillHook,
    #else
    WindowBF_TAG,                                        *_newIconWin__BackFillHook,
    #endif
    MUIA_Window_ScreenTitle,                             (IPTR) "",
    MUIA_Font,                                           (IPTR) _newIconWin__WindowFont,

    WindowContents, (IPTR) MUI_NewObject(MUIC_Group,
      MUIA_Group_Spacing,  0,
      MUIA_Group_SameSize, FALSE,
      InnerSpacing(0,0),

      /* "Extension" group */
      _newIconWin__ExtensionContainerObj ? Child : TAG_IGNORE, (IPTR)_newIconWin__ExtensionContainerObj,

      /* icon list */
      Child, (IPTR) _newIconWin__RootViewObj,
      
    TAG_DONE),
    
    TAG_MORE, (IPTR) message->ops_AttrList
  );

  if (self != NULL)
  {
    SETUP_ICONWINDOW_INST_DATA;

    D(bug("[iconwindow] %s: SELF = 0x%p\n", __PRETTY_FUNCTION__, self));

        data->iwd_Screen                  = _newIconWin__Screen;
        
    data->iwd_Title                   = _newIconWin__Title;

    data->iwd_RootViewObj             = _newIconWin__RootViewObj;

    data->iwd_IconListObj             = _newIconWin__IconListObj;

    SET(data->iwd_RootViewObj, MUIA_IconWindow_Window,      self);

    data->iwd_ActionHook              = actionHook;

    data->iwd_ExtensionGroupObj       = _newIconWin__ExtensionGroupObj;
    data->iwd_ExtensionContainerObj   = _newIconWin__ExtensionContainerObj;
    data->iwd_ExtensionGroupSpacerObj = _newIconWin__ExtensionGroupSpacerObj;

    data->iwd_Toolbar_PanelObj         = NULL;

    data->iwd_Flag_ISROOT             = isRoot;
    data->iwd_Flag_ISBACKDROP         = isBackdrop;

    data->iwd_WindowFont              = _newIconWin__WindowFont;        

    data->iwd_ViewSettings_Attrib = data->iwd_Flag_ISROOT 
          ? "Workbench"
          : "Drawer";

    if (prefs)
    {
      #ifdef __AROS__
      data->iwd_PrefsUpdated_hook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_PrefsUpdatedFunc;
      #else
      data->iwd_PrefsUpdated_hook = &iwd_PrefsUpdated_hook;
      #endif

      DoMethod
      (
        prefs, MUIM_Notify, MUIA_WandererPrefs_Processing, FALSE,
        (IPTR) self, 3, 
        MUIM_CallHook, &data->iwd_PrefsUpdated_hook, (IPTR)CLASS
      );

      data->iwd_ViewSettings_PrefsNotificationObject = (Object *) DoMethod(prefs,
                                MUIM_WandererPrefs_ViewSettings_GetNotifyObject,
                                data->iwd_ViewSettings_Attrib);

      if (data->iwd_ExtensionContainerObj)
      {
        DoMethod
        (
          prefs, MUIM_Notify, MUIA_IconWindowExt_Toolbar_Enabled, MUIV_EveryTime, 
          (IPTR)self, 3, MUIM_Set, MUIA_IconWindowExt_Toolbar_Enabled, MUIV_TriggerValue
        );
      }
    }

    #ifdef __AROS__
    data->iwd_ProcessBackground_hook.h_Entry = ( HOOKFUNC )IconWindow__HookFunc_ProcessBackgroundFunc;
    #else
    data->iwd_ProcessBackground_hook = &iwd_ProcessBackground_hook;
    #endif

    if ((data->iwd_BackFill_hook = _newIconWin__BackFillHook))
    {
      data->iwd_BackFillHookData.bfhd_IWClass = CLASS;
      data->iwd_BackFillHookData.bfhd_IWObject = self;
      data->iwd_BackFill_hook->h_Data = &data->iwd_BackFillHookData;
    }

    /* no tool bar when root */
    if (!isRoot && hasToolbar && data->iwd_ExtensionContainerObj)
      IconWindow__SetupToolbar(CLASS, self, prefs);

    /* If double clicked then we call our own private methods, that's 
       easier then using Hooks */
    DoMethod
    (
      _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_DoubleClick, TRUE, 
      (IPTR) self, 1, MUIM_IconWindow_DoubleClicked
    );

    /* notify when icons dropped on another (wanderer) window */
    DoMethod
    (
      _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_IconsDropped, MUIV_EveryTime,
      (IPTR) self, 1, MUIM_IconWindow_IconsDropped
    );

    /* notify when icons dropped on custom application */
    DoMethod
    (
      _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_AppWindowDrop, MUIV_EveryTime,
      (IPTR) self, 1, MUIM_IconWindow_AppWindowDrop
    );

    DoMethod
    (
      _newIconWin__IconListObj, MUIM_Notify, MUIA_IconList_Clicked, MUIV_EveryTime,
      (IPTR) self, 1, MUIM_IconWindow_Clicked
    );
  }
D(bug("[IconWindow] obj = %ld\n", self));
  return self;
}
///

///OM_DISPOSE()
IPTR IconWindow__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
  SETUP_ICONWINDOW_INST_DATA;

  Object *prefs = NULL;

  GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

  if (prefs)
  {
    DoMethod
    (
      prefs,
      MUIM_KillNotifyObj, MUIA_WandererPrefs_Processing, (IPTR) self
    );

    DoMethod
    (
      prefs,
      MUIM_KillNotifyObj, MUIA_IconWindowExt_Toolbar_Enabled, (IPTR) self
    );
  }
  
  if (data->iwd_BackFill_hook)
    FreeVec(data->iwd_BackFill_hook);
  
  return DoSuperMethodA(CLASS, self, message);
}
///

///OM_SET()
IPTR IconWindow__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
  SETUP_ICONWINDOW_INST_DATA;

  struct TagItem  *tstate = message->ops_AttrList, *tag;
  BOOL      UpdateIconlist = FALSE;
  IPTR      focusicon = (IPTR) NULL;
  IPTR        rv = TRUE;

  while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
  {
    switch (tag->ti_Tag)
    {
        case MUIA_Window_Screen:
            D(bug("[iconwindow] %s: MUIA_Window_Screen [screen @ %p]\n", __PRETTY_FUNCTION__, tag->ti_Data));
            data->iwd_Screen = (struct Screen *) tag->ti_Tag;
            break;

        case MUIA_ShowMe:
      D(bug("[iconwindow] %s: MUIA_ShowMe [%x]\n", __PRETTY_FUNCTION__, tag->ti_Data));
            if ((BOOL)tag->ti_Data == TRUE)
            {
                struct Screen *__Wanderer__Screen = NULL;
                GET(_app(self), MUIA_Wanderer_Screen, &__Wanderer__Screen);
                if (__Wanderer__Screen != data->iwd_Screen)
                {
                    D(bug("[iconwindow] %s: Screen Changed [old = %p, new = %p]\n", __PRETTY_FUNCTION__, data->iwd_Screen, __Wanderer__Screen));
                    SET(self, MUIA_Window_Screen, __Wanderer__Screen);
                    if ((data->iwd_Flag_ISROOT) && (data->iwd_Flag_ISBACKDROP))
                    {
                        IPTR                            _IconWin__NewWindowWidth = 0;
                        IPTR                            _IconWin__NewWindowHeight = 0;
                        IPTR                            _IconWin__NewWindowLeft = 0;
                        IPTR                            _IconWin__NewWindowTop = 0;

                        D(bug("[iconwindow] %s: Updating Backdrop Window Dimensions\n", __PRETTY_FUNCTION__));

                        _IconWin__NewWindowWidth = GetBitMapAttr(__Wanderer__Screen->RastPort.BitMap, BMA_WIDTH);
                        _IconWin__NewWindowHeight = GetBitMapAttr(__Wanderer__Screen->RastPort.BitMap, BMA_HEIGHT);
                        D(bug("[iconwindow] %s: Screen dimensions ..  %d x %d\n", __PRETTY_FUNCTION__, _IconWin__NewWindowWidth, _IconWin__NewWindowHeight));

                        _IconWin__NewWindowTop = __Wanderer__Screen->BarHeight + 1;
                        _IconWin__NewWindowLeft = 0;

                        _IconWin__NewWindowHeight -= _IconWin__NewWindowTop;

                        D(bug("[iconwindow] %s: New Window dimensions ..  %d x %d @ %d, %d\n", __PRETTY_FUNCTION__, _IconWin__NewWindowWidth, _IconWin__NewWindowHeight, _IconWin__NewWindowLeft, _IconWin__NewWindowTop));

                        SET(self, MUIA_Window_Width, _IconWin__NewWindowWidth);
                        SET(self, MUIA_Window_Height, _IconWin__NewWindowHeight);
                    }
                }
            }
            break;

    case MUIA_IconWindow_Changed:
      data->iwd_Flag_NEEDSUPDATE = (BOOL)tag->ti_Data;
      break;

    case MUIA_Window_Open:
      D(bug("[iconwindow] %s: MUIA_Window_Open [%x]\n", __PRETTY_FUNCTION__, tag->ti_Data));
            if ((BOOL)tag->ti_Data == TRUE)
      {
                DoMethod(data->iwd_IconListObj, MUIM_IconList_Clear);

                rv = DoSuperMethodA(CLASS, self, (Msg) message);
#if defined(ICONWINDOW_BUFFERLIST)
                if (!(data->iwd_Flag_ISROOT))
                {
                    IPTR        CURDISPFLAGS = NULL;
                    GET(data->iwd_IconListObj, MUIA_IconList_DisplayFlags, &CURDISPFLAGS);
                    CURDISPFLAGS |= ICONLIST_DISP_BUFFERED;
                    SET(data->iwd_IconListObj, MUIA_IconList_DisplayFlags, CURDISPFLAGS);
                }
#endif
                D(bug("[IconWindow] %s: Process the background ..\n", __PRETTY_FUNCTION__));
                DoMethod(self, MUIM_IconWindow_BackFill_ProcessBackground, data->iwd_BackFillInfo, data->iwd_RootViewObj);

                D(bug("[IconWindow] %s: Force an update of the list ..\n", __PRETTY_FUNCTION__));
                DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);
                return rv;
      }
            break;

    case MUIA_Window_Activate:
      if (data->iwd_IconListObj)
        GET(data->iwd_IconListObj, MUIA_IconList_FocusIcon, &focusicon);

      break;

    case MUIA_IconWindow_Font:
      data->iwd_WindowFont = (struct TextFont  *)tag->ti_Data;
      D(bug("[iconwindow] %s: MUIA_IconWindow_Font [font @ 0x%p]\n", __PRETTY_FUNCTION__, data->iwd_WindowFont));

      if (!data->iwd_WindowFont)
        SetFont(_rp(self), data->iwd_WindowFont);

      break;

    case MUIA_IconWindow_Location:
      D(bug("[iconwindow] %s: MUIA_IconWindow_Location [drawer '%s']\n", __PRETTY_FUNCTION__, data->iwd_DirectoryPath));

      if (!data->iwd_Flag_ISROOT)
      {
        strcpy(data->iwd_DirectoryPath, (STRPTR)tag->ti_Data);
        SET(self, MUIA_Window_Title, (IPTR)data->iwd_DirectoryPath);
        SET(data->iwd_Toolbar_LocationStringObj, MUIA_String_Contents, (IPTR)data->iwd_DirectoryPath);
        SET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer, (IPTR)data->iwd_DirectoryPath);
      }
      break;

    case MUIA_IconWindow_BackgroundAttrib:
      D(bug("[iconwindow] %s: MUIA_IconWindow_BackgroundAttrib (not implemented)\n", __PRETTY_FUNCTION__));
      break;

    case MUIA_IconWindowExt_Toolbar_Enabled:   
      if ((!(data->iwd_Flag_ISROOT)) && (data->iwd_ExtensionContainerObj))
      {
        // remove toolbar
        if (!(( BOOL )tag->ti_Data))
        {
          //Force classic navigation when the toolbar is disabled ..
          Object *prefs = NULL;

          if (data->iwd_Toolbar_PanelObj != NULL)
          {
            data->iwd_ExtensionGroupSpacerObj = HSpace(0);
            
            SET(data->iwd_ExtensionContainerObj, MUIA_Frame, MUIV_Frame_None);
            SET(data->iwd_ExtensionContainerObj, MUIA_Group_Spacing, 0);

            if ((data->iwd_ExtensionGroupSpacerObj) && (DoMethod(data->iwd_ExtensionGroupObj, MUIM_Group_InitChange)))
            {
              DoMethod(data->iwd_ExtensionGroupObj, OM_REMMEMBER, (IPTR)data->iwd_Toolbar_PanelObj);
              DoMethod(data->iwd_ExtensionGroupObj, OM_ADDMEMBER, (IPTR)data->iwd_ExtensionGroupSpacerObj);
              DoMethod(data->iwd_ExtensionGroupObj, MUIM_Group_ExitChange);
              data->iwd_Toolbar_PanelObj = NULL;
            }
          }
        

          GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
          if (prefs)
            SET(prefs, MUIA_IconWindowExt_Toolbar_NavigationMethod, WPD_NAVIGATION_CLASSIC);
        }
        else
        {
          // setup toolbar
          if (data->iwd_Toolbar_PanelObj == NULL)
          {
            Object *prefs = NULL;
            GET(_app(self), MUIA_Wanderer_Prefs, &prefs);
            IconWindow__SetupToolbar(CLASS, self, prefs);
          }
         }
         data->iwd_Flag_EXT_TOOLBARENABLED = (IPTR)tag->ti_Data;
       }
       break;     
    }
  }

  if (UpdateIconlist)
    DoMethod(data->iwd_IconListObj, MUIM_IconList_Update);

  rv = DoSuperMethodA(CLASS, self, (Msg) message);

  if (focusicon)
  {
    D(bug("[iconwindow] %s: Updating focused icon (@ 0x%p)\n", __PRETTY_FUNCTION__, focusicon));
  //  MUI_Redraw(data->iwd_IconListObj, MADF_DRAWOBJECT);
  }

  return rv;
}
///

///OM_GET()
IPTR IconWindow__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
  SETUP_ICONWINDOW_INST_DATA;

  IPTR *store = message->opg_Storage;
  IPTR  rv    = TRUE;
  
  switch (message->opg_AttrID)
  {
    case MUIA_IconWindow_Changed:
      *store = (IPTR)data->iwd_Flag_NEEDSUPDATE;
      break;

    case MUIA_IconWindow_Window:
      *store = (IPTR)self;
      break;

    case MUIA_IconWindow_Location:
      *store = !data->iwd_Flag_ISROOT
        ? XGET(data->iwd_IconListObj, MUIA_IconDrawerList_Drawer)
        : (IPTR)NULL;
      break;

    case MUIA_IconWindow_IconList:
      *store = (IPTR)data->iwd_IconListObj;
      break;

    case MUIA_IconWindowExt_Toolbar_Enabled:
      *store = (IPTR)data->iwd_Flag_EXT_TOOLBARENABLED;
      break;

    case MUIA_IconWindow_IsRoot:
      *store = (IPTR)data->iwd_Flag_ISROOT;
      break;

    case MUIA_IconWindow_IsBackdrop:
      *store = (IPTR)data->iwd_Flag_ISBACKDROP;
      break;

    case MUIA_IconWindow_BackFillData:
      *store = (IPTR)data->iwd_BackFillInfo;
      break;

    case MUIA_IconWindow_BackgroundAttrib:
      *store = (IPTR)data->iwd_ViewSettings_Attrib;
      break;

    default:
      rv = DoSuperMethodA(CLASS, self, (Msg) message);
  }
  
  return rv;
}
///

///IconWindow__MUIM_Window_Setup()
IPTR IconWindow__MUIM_Window_Setup
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  Object *prefs = NULL;

  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (!DoSuperMethodA(CLASS, self, message)) return FALSE;

    if (iconwindow_BackFill_Active)
    {
        data->iwd_BackFillInfo =(struct BackFillInfo *) DoMethod(self, MUIM_IconWindow_BackFill_Setup);
        D(bug("[iconwindow] %s: Window BackFill_Data @ 0x%p for '%s'\n", __PRETTY_FUNCTION__, data->iwd_BackFillInfo, iconwindow_BackFill_Active->bfd_BackFillID));
    }

  GET(_app(self), MUIA_Wanderer_Prefs, &prefs);

  D(bug("[IconWindow] %s: App PrefsObj @ 0x%p\n", __PRETTY_FUNCTION__, prefs));
  
  if ((prefs) && (data->iwd_ViewSettings_PrefsNotificationObject))
  {
    D(bug("[IconWindow] %s: Setting up window background change hook\n", __PRETTY_FUNCTION__));

    /* Set-up a hook to call ProcessBackground on prefs notification */
    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_Background, MUIV_EveryTime,
      (IPTR) self, 3, 
      MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
    );
    
    if (data->iwd_Flag_ISROOT)
    {
      DoMethod
      (
        data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGRenderMode, MUIV_EveryTime,
        (IPTR) self, 3, 
        MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
      );
    }

    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGTileMode, MUIV_EveryTime,
      (IPTR) self, 3, 
      MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
    );

    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGXOffset, MUIV_EveryTime,
      (IPTR) self, 3, 
      MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
    );

    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject, MUIM_Notify, MUIA_IconWindowExt_ImageBackFill_BGYOffset, MUIV_EveryTime,
      (IPTR) self, 3, 
      MUIM_CallHook, &data->iwd_ProcessBackground_hook, (IPTR)CLASS
    );
  }
  
  D(bug("[iconwindow] %s: Setup complete!\n", __PRETTY_FUNCTION__));
  
  return TRUE;
}
///

///IconWindow__MUIM_Window_Cleanup()
IPTR IconWindow__MUIM_Window_Cleanup
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (data->iwd_BackFillInfo)
  {
    DoMethod(self, MUIM_IconWindow_BackFill_Cleanup, data->iwd_BackFillInfo);
    data->iwd_BackFillInfo = NULL;
  }

  if (data->iwd_ViewSettings_PrefsNotificationObject)
  {
    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject,
      MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGYOffset, (IPTR) self
    );
  
    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject,
      MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGXOffset, (IPTR) self
    );
  
    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject,
      MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGTileMode, (IPTR) self
    );

    if (data->iwd_Flag_ISROOT)
    {
      DoMethod
      (
        data->iwd_ViewSettings_PrefsNotificationObject,
        MUIM_KillNotifyObj, MUIA_IconWindowExt_ImageBackFill_BGRenderMode, (IPTR) self
      );
    }

    DoMethod
    (
      data->iwd_ViewSettings_PrefsNotificationObject,
      MUIM_KillNotifyObj, MUIA_Background, (IPTR) self
    );
  }
  return DoSuperMethodA(CLASS, self, message);
}
///

///IconWindow__MUIM_IconWindow_DoubleClicked()
IPTR IconWindow__MUIM_IconWindow_DoubleClicked
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (data->iwd_ActionHook)
  {
    struct IconWindow_ActionMsg msg;
    msg.type     = ICONWINDOW_ACTION_OPEN;
    msg.iconlist = data->iwd_IconListObj;
    msg.isroot   = data->iwd_Flag_ISROOT;
    msg.click    = NULL;
    CallHookPkt(data->iwd_ActionHook, self, &msg);
  }
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_Clicked()
IPTR IconWindow__MUIM_IconWindow_Clicked
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (data->iwd_ActionHook)
  {
    struct IconWindow_ActionMsg msg;
    msg.type     = ICONWINDOW_ACTION_CLICK;
    msg.iconlist = data->iwd_IconListObj;
    msg.isroot   = data->iwd_Flag_ISROOT;
    GET(data->iwd_IconListObj, MUIA_IconList_Clicked, &msg.click);
    CallHookPkt(data->iwd_ActionHook, self, &msg);
  }
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_IconsDropped()
IPTR IconWindow__MUIM_IconWindow_IconsDropped
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (data->iwd_ActionHook)
  {
    struct IconWindow_ActionMsg msg;
    msg.type     = ICONWINDOW_ACTION_ICONDROP;
    msg.iconlist = data->iwd_IconListObj;
    msg.isroot   = data->iwd_Flag_ISROOT;
    GET(data->iwd_IconListObj, MUIA_IconList_IconsDropped, &msg.drop);
    CallHookPkt(data->iwd_ActionHook, self, &msg);
  }
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_AppWindowDrop()
IPTR IconWindow__MUIM_IconWindow_AppWindowDrop
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (data->iwd_ActionHook)
  {
    struct IconWindow_ActionMsg msg;
    msg.type     = ICONWINDOW_ACTION_APPWINDOWDROP;
    msg.iconlist = data->iwd_IconListObj;
    msg.isroot   = data->iwd_Flag_ISROOT;
    GET(data->iwd_IconListObj, MUIA_IconList_IconsDropped, &msg.drop);
    CallHookPkt(data->iwd_ActionHook, self, &msg);
  }
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_Open()
IPTR IconWindow__MUIM_IconWindow_Open
(
  Class *CLASS, Object *self, Msg message
)
{
//    SETUP_ICONWINDOW_INST_DATA;

    #ifdef __AROS__
    IPTR CURDISPFLAGS;
    #endif
  
D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

    if (!XGET(self, MUIA_Window_Open))
    {
        SET(self, MUIA_Window_Open, TRUE);
    }

D(bug("[IconWindow] %s: Setting window as active ..\n", __PRETTY_FUNCTION__));
    SET(self, MUIA_Window_Activate, TRUE);

D(bug("[IconWindow] %s: All done\n", __PRETTY_FUNCTION__));
    
    return TRUE;
}
///

///IconWindow__MUIM_IconWindow_DirectoryUp()
IPTR IconWindow__MUIM_IconWindow_DirectoryUp
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  if (data->iwd_ActionHook)
  {
    struct IconWindow_ActionMsg msg;
    msg.type     = ICONWINDOW_ACTION_DIRUP;
    msg.iconlist = data->iwd_IconListObj;
    msg.isroot   = data->iwd_Flag_ISROOT;
    msg.click    = NULL;
    CallHookPkt(data->iwd_ActionHook, self, &msg);
    
  }
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_UnselectAll()
IPTR IconWindow__MUIM_IconWindow_UnselectAll
(
  Class *CLASS, Object *self, Msg message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  DoMethod(data->iwd_IconListObj, MUIM_IconList_UnselectAll);
  
  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_Remove()
IPTR IconWindow__MUIM_IconWindow_Remove
(
  Class *CLASS, Object *self, Msg message
)
{
//  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));
  
  // Remove window
  SET( self, MUIA_Window_Open, FALSE );
  DoMethod ( _app(self), OM_REMMEMBER, self );
  DisposeObject(self);
  
  return TRUE;
}
///

/*** Stubs for Backfill Hooks ******************************************************************/
///IconWindow__MUIM_IconWindow_BackFill_Register()
IPTR IconWindow__MUIM_IconWindow_BackFill_Register
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Register *message
)
{
//    SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s('%s')\n", __PRETTY_FUNCTION__, message->register_Node->bfd_BackFillID));

  AddTail(&iconwindow_BackFillNodes, (struct Node *)message->register_Node);
  if (iconwindow_BackFill_Active == NULL) iconwindow_BackFill_Active = message->register_Node;

  return TRUE;
}
///

///IconWindow__MUIM_IconWindow_BackFill_Setup()
IPTR IconWindow__MUIM_IconWindow_BackFill_Setup
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Setup *message
)
{
//  SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active == NULL) return FALSE;

  return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_Setup)(CLASS, self, message);
}
///

///IconWindow__MUIM_IconWindow_BackFill_Cleanup()
IPTR IconWindow__MUIM_IconWindow_BackFill_Cleanup
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_Cleanup *message
)
{
  //SETUP_ICONWINDOW_INST_DATA;
  
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active == NULL) return FALSE;

  return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_Cleanup)(CLASS, self, message);
}
///

///IconWindow__MUIM_IconWindow_BackFill_ProcessBackground()
IPTR IconWindow__MUIM_IconWindow_BackFill_ProcessBackground
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_ProcessBackground *message
)
{
  SETUP_ICONWINDOW_INST_DATA;
  
  IPTR retVal = (IPTR)FALSE;

  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active != NULL)
  {
    D(bug("[IconWindow] %s: Asking module @ 0x%p to process ..\n", __PRETTY_FUNCTION__, iconwindow_BackFill_Active));
    retVal = (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_ProcessBackground)(CLASS, self, message);
  }
  
  if ((retVal == (IPTR)FALSE) && (data->iwd_RootViewObj != NULL))
  {
    Object                *IconWindowPB_PrefsObj = NULL;

    D(bug("[IconWindow] %s: No BackFill module/ module cant render mode\n", __PRETTY_FUNCTION__));
    D(bug("[IconWindow] %s: Using default MUI functions ..\n", __PRETTY_FUNCTION__));
    
    GET(_app(self), MUIA_Wanderer_Prefs, &IconWindowPB_PrefsObj);
    if (IconWindowPB_PrefsObj)
    {
      IPTR IconWindowPB_Background = 0;
      IPTR IconWindowPB_BGMode     = 0;
      IPTR IconWindowPB_BGTileMode = 0;

      if ((IconWindowPB_Background = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute, data->iwd_ViewSettings_Attrib, MUIA_Background)) != -1)
      {
        char *bgmode_string;
        BYTE this_mode;
  
        if ((IconWindowPB_BGMode = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                        data->iwd_ViewSettings_Attrib, MUIA_IconWindowExt_ImageBackFill_BGRenderMode)) == -1)
          IconWindowPB_BGMode = IconWindowExt_ImageBackFill_RenderMode_Tiled;

        if ((IconWindowPB_BGTileMode = DoMethod(IconWindowPB_PrefsObj, MUIM_WandererPrefs_ViewSettings_GetAttribute,
                                  data->iwd_ViewSettings_Attrib, MUIA_IconWindowExt_ImageBackFill_BGTileMode)) == -1)
          IconWindowPB_BGTileMode = IconWindowExt_ImageBackFill_TileMode_Float;
        
        SET(data->iwd_RootViewObj, MUIA_Background, IconWindowPB_Background);

        bgmode_string =(STRPTR) IconWindowPB_Background;
        this_mode = bgmode_string[0] - 48;

        D(bug("[IconWindow] %s: MUI BG Mode = %d\n", __PRETTY_FUNCTION__, this_mode));

        switch (this_mode)
        {
          case 5:
            //Image =D
            if (IconWindowPB_BGMode == IconWindowExt_ImageBackFill_RenderMode_Scale)
            {
              SET(data->iwd_RootViewObj, MUIA_IconListview_ScaledBackground, TRUE);
              break;
            }
            else
              SET(data->iwd_RootViewObj, MUIA_IconListview_ScaledBackground, FALSE);
  
          case 0:
            //MUI Pattern
            if (IconWindowPB_BGTileMode == IconWindowExt_ImageBackFill_TileMode_Fixed)
              SET(data->iwd_RootViewObj, MUIA_IconListview_FixedBackground, TRUE);
            else
              SET(data->iwd_RootViewObj, MUIA_IconListview_FixedBackground, FALSE);
            break;
        }
      }
    }
    retVal = FALSE;
  }

  return retVal;
}
///

///IconWindow__MUIM_IconWindow_BackFill_DrawBackground()
IPTR IconWindow__MUIM_IconWindow_BackFill_DrawBackground
(
  Class *CLASS, Object *self, struct MUIP_IconWindow_BackFill_DrawBackground *message
)
{
//  SETUP_ICONWINDOW_INST_DATA;

  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  if (iconwindow_BackFill_Active == NULL) return FALSE;

  return (iconwindow_BackFill_Active->bfd_MUIM_IconWindow_BackFill_DrawBackground)(CLASS, self, message);
}
///

///
IPTR IconWindow__SetupClass()
{
  D(bug("[IconWindow]: %s()\n", __PRETTY_FUNCTION__));

  NewList(&iconwindow_BackFillNodes);
  iconwindow_BackFill_Active = NULL;

  return TRUE;
}
///
/*** Setup ******************************************************************/
ICONWINDOW_CUSTOMCLASS
(
  IconWindow, NULL, MUIC_Window, NULL,
  OM_NEW,                                     struct opSet *,
  OM_DISPOSE,                         Msg,
  OM_SET,                                     struct opSet *,
  OM_GET,                                     struct opGet *,
  MUIM_Window_Setup,                          Msg,
  MUIM_Window_Cleanup,                        Msg,
  MUIM_IconWindow_Open,                       Msg,
  MUIM_IconWindow_UnselectAll,                Msg,
  MUIM_IconWindow_DoubleClicked,              Msg,
  MUIM_IconWindow_IconsDropped,               Msg,
  MUIM_IconWindow_Clicked,                    Msg,
  MUIM_IconWindow_DirectoryUp,                Msg,
  MUIM_IconWindow_AppWindowDrop,              Msg,
  MUIM_IconWindow_Remove,                     Msg,
  MUIM_IconWindow_BackFill_Register,          struct MUIP_IconWindow_BackFill_Register *,
  MUIM_IconWindow_BackFill_Setup,             struct MUIP_IconWindow_BackFill_Setup *,
  MUIM_IconWindow_BackFill_Cleanup,           struct MUIP_IconWindow_BackFill_Cleanup *,
  MUIM_IconWindow_BackFill_ProcessBackground, struct MUIP_IconWindow_BackFill_ProcessBackground *,
  MUIM_IconWindow_BackFill_DrawBackground,    struct MUIP_IconWindow_BackFill_DrawBackground *
);

ADD2INIT(IconWindow__SetupClass, 0);

#ifndef __AROS__
int initIconWindowClass(void)
{
  IPTR ret1 = IconWindow_Initialize();

  IPTR ret2 = IconWindow__SetupClass();

  if (ret1 && ret2)
    return TRUE;
  else
    return FALSE;

}
#endif
