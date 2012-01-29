/***************************************************************************

 codesets.library - Amiga shared library for handling different codesets
 Copyright (C) 2001-2005 by Alfonso [alfie] Ranieri <alforan@tin.it>.
 Copyright (C) 2005-2010 by codesets.library Open Source Team

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 codesets.library project: http://sourceforge.net/projects/codesetslib/

 Most of the code included in this file was relicensed from GPL to LGPL
 from the source code of SimpleMail (http://www.sf.net/projects/simplemail)
 with full permissions by its authors.

 $Id$

***************************************************************************/

#include <proto/codesets.h>

#if defined(__MORPHOS__)
#if defined(USE_INLINE_STDARG)
#undef USE_INLINE_STDARG
#endif
#endif
#define INTUITION_NO_INLINE_STDARG
#define MUIMASTER_NO_INLINE_STDARG

#include <clib/alib_protos.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <mui/TextEditor_mcc.h>

#include <stdio.h>
#include <string.h>

#include "SDI_compiler.h"
#include "SDI_hook.h"
#include "SDI_stdarg.h"

/***********************************************************************/
/*
** Some macro
*/

#ifndef MAKE_ID
#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

#if defined(__amigaos4__)
#define GETINTERFACE(iface, base)	(iface = (APTR)GetInterface((struct Library *)(base), "main", 1L, NULL))
#define DROPINTERFACE(iface)			(DropInterface((struct Interface *)iface), iface = NULL)
#else
#define GETINTERFACE(iface, base)	TRUE
#define DROPINTERFACE(iface)
#endif

/***********************************************************************/
/*
** Globals
*/

char __ver[] = "\0$VER: CodesetsDemo1 1.0 (10.11.2004)";
long __stack = 8192;

struct Library *MUIMasterBase = NULL;
struct Library *CodesetsBase = NULL;
#ifdef __AROS__
struct UtilityBase *UtilityBase = NULL;
#else
struct Library *UtilityBase = NULL;
#endif

#if defined(__amigaos4__)
struct MUIMasterIFace *IMUIMaster = NULL;
struct CodesetsIFace  *ICodesets = NULL;
struct IntuitionIFace *IIntuition = NULL;
struct UtilityIFace   *IUtility = NULL;
struct Library *IntuitionBase = NULL;
#else
struct IntuitionBase *IntuitionBase = NULL;
#endif


struct MUI_CustomClass *appClass, *popupCodesetsClass, *editorClass;

/***********************************************************************/
/*
** MUI stuff
*/

/* App attributes */
#define MUIA_App_Win            (TAG_USER+1)

/* App methods */
#define MUIM_App_DisposeWin     (TAG_USER+2)
#define MUIM_App_About          (TAG_USER+3)

struct MUIP_App_DisposeWin
{
    ULONG  MethodID;
    Object *win;
};

/* Editor attributes */
#define MUIA_Editor_CodesetsObj (TAG_USER+5)

/* Editor methods */
#define MUIM_Editor_Load        (TAG_USER+6)
#define MUIM_Editor_Save        (TAG_USER+7)

struct MUIP_Editor_Load
{
    ULONG MethodID;
    ULONG plain;
};

/* Classes object creation macros */
#define appObject          NewObject(appClass->mcc_Class,NULL
#define editorObject       NewObject(editorClass->mcc_Class,NULL
#define popupCodesetObject NewObject(popupCodesetsClass->mcc_Class,NULL

/***********************************************************************/
/*
** Usual DoSuperNew funct
*/

/// DoSuperNew
//  Calls parent NEW method within a subclass
#if !defined(__MORPHOS__)
#ifdef __AROS__
#define DoSuperNew(cl, obj, ...) DoSuperNewTags(cl, obj, NULL, __VA_ARGS__)
#else
Object * STDARGS VARARGS68K DoSuperNew(struct IClass *cl, Object *obj, ...)
{
  Object *rc;
  VA_LIST args;

  VA_START(args, obj);
  rc = (Object *)DoSuperMethod(cl, obj, OM_NEW, VA_ARG(args, ULONG), NULL);
  VA_END(args);

  return rc;
}
#endif
#endif
///

/***********************************************************************/
/*
** Codesets popup open window hook funct
*/

HOOKPROTONH(popupWindowFun, void, Object *pop, Object *win)
{
  set(win,MUIA_Window_DefaultObject,pop);
}
MakeStaticHook(popupWindowHook, popupWindowFun);

/***********************************************************************/
/*
** Codesets popup open hook funct
** Sets the active entry in the list
*/

HOOKPROTONH(popupOpenFun, ULONG, Object *list, Object *str)
{
    STRPTR s = NULL, x;
    int i;

    get(str, MUIA_Text_Contents, (IPTR *)&s);

    if(s != NULL)
    {
        for (i = 0; ;i++)
        {
            DoMethod(list,MUIM_List_GetEntry,i,&x);
            if (x == NULL)
            {
                set(list,MUIA_List_Active,MUIV_List_Active_Off);
                break;
            }
            else
                if (stricmp(x,s) == 0)
                {
                    set(list,MUIA_List_Active,i);
                    break;
                }
        }
    }

    return TRUE;
}
MakeStaticHook(popupOpenHook, popupOpenFun);

/***********************************************************************/
/*
** Codesets popup close hook funct
** Set the string contents
*/

HOOKPROTONH(popupCloseFun, void, Object *list, Object *str)
{
    STRPTR e;

    DoMethod(list,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,&e);
    set(str,MUIA_Text_Contents,e);
}
MakeStaticHook(popupCloseHook, popupCloseFun);

/***********************************************************************/
/*
** Codesets popup new method
*/

static Object *
mpopupNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object *str, *bt, *lv, *l;

    if((obj = (Object *)DoSuperNew(cl,obj,

            MUIA_Popstring_String, str = TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
            End,

            MUIA_Popstring_Button, bt = MUI_MakeObject(MUIO_PopButton,MUII_PopUp),

            MUIA_Popobject_Object, lv = ListviewObject,
                MUIA_Listview_List, l = ListObject,
                    MUIA_Frame,              MUIV_Frame_InputList,
                    MUIA_Background,         MUII_ListBack,
                    MUIA_List_AutoVisible,   TRUE,
                    MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
                    MUIA_List_DestructHook,  MUIV_List_DestructHook_String,
                End,
            End,
            MUIA_Popobject_WindowHook, &popupWindowHook,
            MUIA_Popobject_StrObjHook, &popupOpenHook,
            MUIA_Popobject_ObjStrHook, &popupCloseHook,

            TAG_MORE,msg->ops_AttrList)))
    {
        struct codeset *codeset;
        STRPTR         *array;

        set(bt,MUIA_CycleChain,TRUE);
        DoMethod(lv,MUIM_Notify,MUIA_Listview_DoubleClick,TRUE,obj,2,MUIM_Popstring_Close,TRUE);

        /* Build list of available codesets */
        if((array = CodesetsSupportedA(NULL)))
        {
            DoMethod(l,MUIM_List_Insert,array,-1,MUIV_List_Insert_Sorted);
            CodesetsFreeA(array,NULL);
        }
        else SetSuperAttrs(cl,obj,MUIA_Disabled,TRUE,TAG_DONE);

        /* Use the default codeset */
        codeset = CodesetsFindA(NULL,NULL);
        set(str,MUIA_Text_Contents,codeset->name);
    }

    return obj;
}

/***********************************************************************/
/*
** Codesets popup dispatcher
*/

DISPATCHER(popupDispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return (IPTR)mpopupNew(cl,obj,(APTR)msg);
        default:     return DoSuperMethodA(cl,obj,msg);
    }
}

/***********************************************************************/
/*
** Editor instance
*/

struct editorData
{
    Object               *codesetsObj;
    struct FileRequester *req;
};

/***********************************************************************/
/*
** Editor new method
*/

static Object *
meditorNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    struct FileRequester *req;

    if ((req = MUI_AllocAslRequest(ASL_FileRequest,NULL)) &&
        (obj = (Object *)DoSuperNew(cl,obj,
            TAG_MORE,msg->ops_AttrList)))
    {
        struct editorData *data = INST_DATA(cl,obj);

        data->codesetsObj = (Object *)GetTagData(MUIA_Editor_CodesetsObj, 0, msg->ops_AttrList);

        data->req = req;
    }
    else
    {
        if (req) MUI_FreeAslRequest(req);
    }

    return obj;
}

/***********************************************************************/
/*
** Editor dispose method
*/

static ULONG
meditorDispose(struct IClass *cl,Object *obj,Msg msg)
{
    struct editorData *data = INST_DATA(cl,obj);

    if (data->req) MUI_FreeAslRequest(data->req);

    return DoSuperMethodA(cl,obj,msg);
}

/***********************************************************************/
/*
** Editor load method
*/

static ULONG
meditorLoad(struct IClass *cl,Object *obj,struct MUIP_Editor_Load *msg)
{
    struct editorData *data = INST_DATA(cl,obj);

    set(_app(obj),MUIA_Application_Sleep,TRUE);
    SetSuperAttrs(cl,obj,MUIA_TextEditor_Quiet,FALSE,TAG_DONE);

    /* Request file name */
    if (MUI_AslRequestTags(data->req,ASLFR_TitleText,msg->plain ?
        "Select a file to load" : "Select a file to load as UTF8",TAG_DONE))
    {
        char fname[256];
        BPTR lock;

        strlcpy(fname,data->req->fr_Drawer,sizeof(fname));
        AddPart(fname,data->req->fr_File,sizeof(fname));

        /* Get size */
        if((lock = Lock(fname,SHARED_LOCK)))
        {
            struct FileInfoBlock *fib;
            ULONG                go = FALSE, size = 0;

            if((fib = AllocDosObject(DOS_FIB,NULL)))
            {
                if (Examine(lock,fib))
                {
                    size = fib->fib_Size;
                    go = TRUE;
                }

                FreeDosObject(DOS_FIB,fib);
            }

            UnLock(lock);

            if (go)
            {
                DoSuperMethod(cl,obj,MUIM_TextEditor_ClearText);

                if (size>0)
                {
                    STRPTR buf;

                    /* Alloc whole file buf */
                    if((buf = AllocMem(size+1,MEMF_ANY)))
                    {
                        BPTR file;

                        if((file = Open(fname,MODE_OLDFILE)))
                        {
                            LONG r;

                            r = Read(file,buf,size);
                            if(r >= 0)
                            {
                                buf[r] = 0;

                                if (msg->plain)
                                {
                                    /* If plain just set it */
                                    set(obj,MUIA_TextEditor_Contents,buf);
                                }
                                else
                                {
                                    struct codeset *codeset;
                                    STRPTR         str;
                                    STRPTR                  cname = NULL;

                                    /* Get used codeset */
                                    get(data->codesetsObj, MUIA_Text_Contents, (IPTR *)&cname);
                                    codeset = CodesetsFindA(cname,NULL);

                                    /* Convert */
                                    str = CodesetsUTF8ToStr(CSA_Source,        (Tag)buf,
                                                            CSA_SourceCodeset, (Tag)codeset,
                                                            TAG_DONE);
                                    if (str)
                                    {
                                        SetSuperAttrs(cl,obj,MUIA_TextEditor_Contents,str,TAG_DONE);
                                        CodesetsFreeA(str,NULL);
                                    }
                                }
                            }

                            Close(file);
                        }

                        FreeMem(buf,size+1);
                    }
                }

                SetSuperAttrs(cl,obj,MUIA_TextEditor_CursorX, 0,
                                     MUIA_TextEditor_CursorY, 0,
                                     TAG_DONE);
            }
        }
    }

    SetSuperAttrs(cl,obj,MUIA_TextEditor_Quiet,FALSE,TAG_DONE);
    set(_app(obj),MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/
/*
** Editor save method
*/

static ULONG
meditorSave(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct editorData *data = INST_DATA(cl,obj);
    STRPTR                     text;

    set(_app(obj),MUIA_Application_Sleep,TRUE);

    /* Get editor text */
    if((text = (STRPTR)DoSuperMethod(cl,obj,MUIM_TextEditor_ExportText)))
    {
        struct codeset *codeset;
        UTF8           *utf8;
        STRPTR                  cname = NULL;
        ULONG                   dlen;

        /* Get current user codeset */
        get(data->codesetsObj, MUIA_Text_Contents, (IPTR *)&cname);
        codeset = CodesetsFindA(cname,NULL);

        /* Convert text as utf8 */
        if((utf8 = CodesetsUTF8Create(CSA_Source,         (Tag)text,
                                      CSA_SourceCodeset,  (Tag)codeset,
                                      CSA_DestLenPtr,     (Tag)&dlen,
                                      TAG_DONE)))
        {
            /* Save converted text to a file */

            if (MUI_AslRequestTags(data->req,ASLFR_DoSaveMode,TRUE,ASLFR_TitleText,"Select a file to save as UTF8",TAG_DONE))
            {
                char fname[256];
                BPTR file;

                strlcpy(fname,data->req->fr_Drawer,sizeof(fname));
                AddPart(fname,data->req->fr_File,sizeof(fname));

                if((file = Open(fname,MODE_NEWFILE)))
                {
                    Write(file,utf8,dlen);
                    Close(file);
                }
            }

            /* Free converted string */
            CodesetsFreeA(utf8,NULL);
        }

        FreeVec(text);
    }

    set(_app(obj),MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/
/*
** Editor dispatcher
*/

DISPATCHER(editorDispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:           return (IPTR)meditorNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:       return meditorDispose(cl,obj,(APTR)msg);
        case MUIM_Editor_Save: return meditorSave(cl,obj,(APTR)msg);
        case MUIM_Editor_Load: return meditorLoad(cl,obj,(APTR)msg);
        default:               return DoSuperMethodA(cl,obj,msg);
    }
}

/***********************************************************************/
/*
** App instance
*/

struct appData
{
    Object *win;
    Object *about;
    Object *aboutMUI;
    Object *config;
};

/***********************************************************************/
/*
** App new method
*/

/* Menus */
#define MTITLE(t)  {NM_TITLE,(STRPTR)(t),0,0,0,0}
#define MITEM(t,d) {NM_ITEM,(STRPTR)(t),0,0,0,(APTR)(d)}
#define MBAR       {NM_ITEM,(STRPTR)NM_BARLABEL,0,0,0,NULL}
#define MEND       {NM_END,NULL,0,0,0,NULL}

enum
{
    MABOUT = 1,
    MABOUTMUI,
    MMUI,
    MQUIT,
};

static struct NewMenu appMenu[] =
{
    MTITLE("Project"),
        MITEM("?\0About...",MABOUT),
        MITEM("M\0About MUI...",MABOUTMUI),
        MBAR,
        MITEM("Q\0Quit",MQUIT),

    MTITLE("Editor"),
        MITEM("M\0MUI Settings...",MMUI),

    MEND
};

static Object *
mappNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object *strip, *win, *codesets = NULL, *editor, *sb, *loadPlain, *loadUTF8, *save, *cancel;

    codesets = popupCodesetObject, End;

    if((obj = (Object *)DoSuperNew(cl,obj,
                MUIA_Application_Title,        "Codesets Demo1",
                MUIA_Application_Version,      "$VER: CodesetsDemo1 1.0 (10.11.2004)",
                MUIA_Application_Copyright,    "Copyright 2004 by Alfonso Ranieri",
                MUIA_Application_Author,       "Alfonso Ranieri <alforan@tin.it>",
                MUIA_Application_Description,  "Codesets example",
                MUIA_Application_Base,         "CODESETSEXAMPLE",
                MUIA_Application_Menustrip,    strip = MUI_MakeObject(MUIO_MenustripNM,appMenu,MUIO_MenustripNM_CommandKeyCheck),

                SubWindow, win = WindowObject,
                    MUIA_Window_ID,             MAKE_ID('M','A','I','N'),
                    MUIA_Window_Title,          "Codesets Demo1",
                    WindowContents, VGroup,

                        Child, HGroup,
                            Child, Label2("Charset"),
                            Child, codesets,
                        End,

                        Child, HGroup,
                            MUIA_Group_Horiz,   TRUE,
                            MUIA_Group_Spacing, 0,
                            Child, editor = editorObject,
                                MUIA_Editor_CodesetsObj, codesets,
                            End,
                            Child, sb = ScrollbarObject, End,
                        End,

                        Child, HGroup,
                            Child, loadPlain = MUI_MakeObject(MUIO_Button,"Load _plain"),
                            Child, RectangleObject, MUIA_Weight, 50, End,
                            Child, loadUTF8 = MUI_MakeObject(MUIO_Button,"_Load utf8"),
                            Child, RectangleObject, MUIA_Weight, 50, End,
                            Child, save = MUI_MakeObject(MUIO_Button,"_Save utf8"),
                            Child, RectangleObject, MUIA_Weight, 50, End,
                            Child, cancel = MUI_MakeObject(MUIO_Button,"_Cancel"),
                        End,

                    End,
                End,
                TAG_MORE,msg->ops_AttrList)))
    {
        struct appData *data = INST_DATA(cl,obj);
        data->win = win;

        set(editor,MUIA_TextEditor_Slider,sb);

        set(loadPlain,MUIA_CycleChain,TRUE);
        set(loadUTF8,MUIA_CycleChain,TRUE);
        set(save,MUIA_CycleChain,TRUE);
        set(cancel,MUIA_CycleChain,TRUE);

        DoMethod(win,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,MUIV_Notify_Application,2,
            MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

        DoMethod(loadPlain,MUIM_Notify,MUIA_Pressed,FALSE,editor,2,MUIM_Editor_Load,TRUE);
        DoMethod(loadUTF8,MUIM_Notify,MUIA_Pressed,FALSE,editor,2,MUIM_Editor_Load,FALSE);
        DoMethod(save,MUIM_Notify,MUIA_Pressed,FALSE,editor,1,MUIM_Editor_Save);
        DoMethod(cancel,MUIM_Notify,MUIA_Pressed,FALSE,MUIV_Notify_Application,2,
            MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MABOUT),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,1,MUIM_App_About);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MABOUTMUI),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,1,MUIM_Application_AboutMUI);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MQUIT),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,2,MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MMUI),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,obj,2,MUIM_Application_OpenConfigWindow,0);

        set(win,MUIA_Window_Open,TRUE);
    }

    return obj;
}

/***********************************************************************/
/*
** App dispose win method
*/

static ULONG
mappDisposeWin(struct IClass *cl,Object *obj,struct MUIP_App_DisposeWin *msg)
{
    struct appData *data = INST_DATA(cl,obj);
    Object         *win = msg->win;

    set(win,MUIA_Window_Open,FALSE);
    DoSuperMethod(cl,obj,OM_REMMEMBER,win);
    MUI_DisposeObject(win);

    if (win==data->about) data->about = NULL;
    else if (win==data->aboutMUI) data->aboutMUI = NULL;

    return 0;
}

/***********************************************************************/
/*
** App about method
*/

static ULONG
mappAbout(struct IClass *cl,Object *obj,UNUSED Msg msg)
{
    struct appData *data = INST_DATA(cl,obj);

    SetSuperAttrs(cl,obj,MUIA_Application_Sleep,TRUE,TAG_DONE);

    if (!data->about)
    {
        Object *ok;

        if((data->about = WindowObject,
                MUIA_Window_RefWindow, data->win,
                MUIA_Window_Title,     "About Codesets Demo1",
                WindowContents, VGroup,
                    Child, TextObject,
                        MUIA_Text_Contents, "\n"
                                            "Codesets Demo1\n"
                                            "Copyright 2004 by Alfonso Ranieri <alforan@tin.it>\n",
                        MUIA_Text_PreParse, MUIX_C,
                    End,
                    Child, RectangleObject, MUIA_Weight, 0, MUIA_Rectangle_HBar, TRUE, End,
                    Child, HGroup,
                        Child, RectangleObject, MUIA_Weight, 200, End,
                        Child, ok = MUI_MakeObject(MUIO_Button,"_OK"),
                        Child, RectangleObject, MUIA_Weight, 200, End,
                    End,
                End,
            End))
        {
            DoSuperMethod(cl,obj,OM_ADDMEMBER,data->about);

            set(data->about,MUIA_Window_ActiveObject,ok);

            DoMethod(data->about,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,obj,5,
                MUIM_Application_PushMethod,obj,2,MUIM_App_DisposeWin,data->about);

            DoMethod(ok,MUIM_Notify,MUIA_Pressed,FALSE,obj,5,
                MUIM_Application_PushMethod,obj,2,MUIM_App_DisposeWin,data->about);
        }
    }

    set(data->about,MUIA_Window_Open,TRUE);

    SetSuperAttrs(cl,obj,MUIA_Application_Sleep,FALSE,TAG_DONE);

    return 0;
}

/***********************************************************************/
/*
** App MUI settings method
*/

static ULONG
mappOpenMUIConfigWindow(struct IClass *cl,Object *obj,Msg msg)
{
    ULONG res;

    SetSuperAttrs(cl,obj,MUIA_Application_Sleep,TRUE,TAG_DONE);
    res = DoSuperMethodA(cl,obj,msg);
    SetSuperAttrs(cl,obj,MUIA_Application_Sleep,FALSE,TAG_DONE);

    return res;
}

/***********************************************************************/
/*
** App dispatcher
*/

DISPATCHER(appDispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:                            return (IPTR)mappNew(cl,obj,(APTR)msg);
        case MUIM_App_DisposeWin:               return mappDisposeWin(cl,obj,(APTR)msg);
        case MUIM_App_About:                    return mappAbout(cl,obj,(APTR)msg);
        case MUIM_Application_OpenConfigWindow: return mappOpenMUIConfigWindow(cl,obj,(APTR)msg);
        default:                                return DoSuperMethodA(cl,obj,msg);
    }
}

/***********************************************************************/
/*
** Main
*/

int
main(UNUSED int argc,char **argv)
{
    int res = RETURN_FAIL;

    if((IntuitionBase = (APTR)OpenLibrary("intuition.library", 39)) &&  // open intuition.library
       GETINTERFACE(IIntuition, IntuitionBase))
    {
      if((UtilityBase = (APTR)OpenLibrary("utility.library", 39)) &&      // open utility.library
         GETINTERFACE(IUtility, UtilityBase))
      {
        if((CodesetsBase = OpenLibrary(CODESETSNAME, CODESETSVER)) && // open codesets.library
           GETINTERFACE(ICodesets, CodesetsBase))
        {
            /* Open muimaster.library */
            if((MUIMasterBase = OpenLibrary("muimaster.library",19)) &&
               GETINTERFACE(IMUIMaster, MUIMasterBase))
            {
                /* Create classes */
		if ((appClass = MUI_CreateCustomClass(NULL, 		MUIC_Application, NULL, sizeof(struct appData), ENTRY(appDispatcher))) &&
		    (popupCodesetsClass = MUI_CreateCustomClass(NULL, MUIC_Popobject, NULL, 0, ENTRY(popupDispatcher))) &&
		    (editorClass = MUI_CreateCustomClass(NULL, MUIC_TextEditor, NULL, sizeof(struct editorData), ENTRY(editorDispatcher))))
                {
                    Object *app;

                    /* Create application */
                    if((app = appObject, End))
                    {
                       /* Here we go */
                        ULONG sigs = 0;

                        while (DoMethod(app,MUIM_Application_NewInput,&sigs) != (ULONG)MUIV_Application_ReturnID_Quit)
                        {
                            if (sigs)
                            {
                                sigs = Wait(sigs | SIGBREAKF_CTRL_C);
                                if (sigs & SIGBREAKF_CTRL_C) break;
                            }
                        }

                        MUI_DisposeObject(app);

                        res = RETURN_OK;
                    }
                    else
                    {
                        printf("%s: can't create application\n",argv[0]);
                    }

                    MUI_DeleteCustomClass(popupCodesetsClass);
                    MUI_DeleteCustomClass(editorClass);
                    MUI_DeleteCustomClass(appClass);
                }
                else
                {
                    if (appClass)
                    {
                        if (popupCodesetsClass) MUI_DeleteCustomClass(popupCodesetsClass);
                        MUI_DeleteCustomClass(appClass);
                    }

                    printf("%s: can't create custom classes\n",argv[0]);
                }

                DROPINTERFACE(IMUIMaster);
                CloseLibrary(MUIMasterBase);
            }
            else
            {
                printf("%s: Can't open muimaster.library ver 19 or higher\n",argv[0]);
                res = RETURN_ERROR;
            }

            DROPINTERFACE(ICodesets);
            CloseLibrary(CodesetsBase);
        }
        else
        {
            printf("%s: Can't open codesets.library ver %d or higher.\n", argv[0], CODESETSVER);
            res = RETURN_ERROR;
        }

        DROPINTERFACE(IUtility);
        CloseLibrary((struct Library *)UtilityBase);
      }

      DROPINTERFACE(IIntuition);
      CloseLibrary((struct Library *)IntuitionBase);
    }

    return res;
}

/***********************************************************************/
