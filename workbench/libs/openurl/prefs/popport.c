/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "openurl.h"

#include <libraries/openurl.h>
#include <exec/execbase.h>

#include "SDI_hook.h"
#include "macros.h"

#include "debug.h"

/**************************************************************************/
/*
** Public ports list
*/

static struct MUI_CustomClass *listClass = NULL;
#define listObject NewObject(listClass->mcc_Class,NULL

static IPTR mListNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    return (IPTR)DoSuperNew(cl,obj,
      MUIA_Frame,              MUIV_Frame_InputList,
      MUIA_Background,         MUII_ListBack,
      MUIA_List_AutoVisible,   TRUE,
      MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
      MUIA_List_DestructHook,  MUIV_List_DestructHook_String,
      TAG_MORE,(IPTR)msg->ops_AttrList);
}

/**************************************************************************/

static IPTR mListSetup(struct IClass *cl, Object *obj, Msg msg)
{
  IPTR success = FALSE;

  ENTER();

  if(DoSuperMethodA(cl, obj, msg))
  {
    struct List *portList;
    struct PortNode
    {
    	struct Node node;
    	STRPTR name;
    };

    DoSuperMethod(cl, obj, MUIM_List_Clear);

    #if defined(__amigaos4__)
    portList = AllocSysObjectTags(ASOT_LIST, TAG_DONE);
    #else
    portList = AllocVec(sizeof(*portList), MEMF_ANY);
    #endif

    if(portList != NULL)
    {
      struct Node *mstate;
      struct PortNode *portNode;

      #if !defined(__amigaos4__)
      NewList(portList);
      #endif

      Forbid();

      for(mstate = SysBase->PortList.lh_Head; mstate->ln_Succ; mstate = mstate->ln_Succ)
      {
        // don't distinguish between OS4 and other systems here, because AllocSysObject()
        // might do things which break the surrounding Forbid(), which AllocVec() is
        // guaranteed *NOT* to do.
        if((portNode = AllocVec(sizeof(*portNode), MEMF_CLEAR)) != NULL)
        {
          if((portNode->name = AllocVec(strlen(mstate->ln_Name)+1, MEMF_ANY)) != NULL)
          {
            strcpy(portNode->name, mstate->ln_Name);
            AddTail(portList, &portNode->node);
          }
          else
            FreeVec(portNode);
        }
      }

      Permit();

      // now that the port names have been copied we can insert them into the list
      while((portNode = (struct PortNode *)RemHead(portList)) != NULL)
      {
        DoSuperMethod(cl, obj, MUIM_List_InsertSingle, portNode->name, MUIV_List_Insert_Sorted);

        // free the complete node, the name was already copied during MUIM_List_InsertSingle
        // due to the given construct hook
        FreeVec(portNode->name);
        FreeVec(portNode);
      }

      #if defined(__amigaos4__)
      FreeSysObject(ASOT_LIST, portList);
      #else
      FreeVec(portList);
      #endif
    }

    // signal success, even if copying the list failed for some reason
    // but the MUIM_Setup invocation of the super class succeeded.
    success = TRUE;
  }

  RETURN(success);
  return success;
}

/**************************************************************************/

SDISPATCHER(listDispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:     return mListNew(cl,obj,(APTR)msg);

        case MUIM_Setup: return mListSetup(cl,obj,(APTR)msg);

        default:         return DoSuperMethodA(cl,obj,msg);
    }
}

/**************************************************************************/

static BOOL initListClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if((listClass = MUI_CreateCustomClass(NULL, MUIC_List, NULL, 0, ENTRY(listDispatcher))) != NULL)
        success = TRUE;

    RETURN(success);
    return success;
}

/**************************************************************************/

static void disposeListClass(void)
{
    ENTER();

    if(listClass != NULL)
       MUI_DeleteCustomClass(listClass);

    LEAVE();
}

/**************************************************************************/

HOOKPROTONH(windowFun, void, Object *pop, Object *win)
{
  set(win,MUIA_Window_DefaultObject,pop);
}
MakeStaticHook(windowHook, windowFun);

/***********************************************************************/

HOOKPROTONH(openFun, ULONG, Object *list, Object *str)
{
    STRPTR s, x;
    int   i;

    s = (STRPTR)xget(str,MUIA_String_Contents);

    for (i = 0; ;i++)
    {
        DoMethod(list,MUIM_List_GetEntry,i,(IPTR)&x);
        if (!x)
        {
            set(list,MUIA_List_Active,MUIV_List_Active_Off);
            break;
        }
        else
            if (!stricmp(x,s))
            {
                set(list,MUIA_List_Active,i);
                break;
            }
    }

    return TRUE;
}
MakeStaticHook(openHook, openFun);

/***********************************************************************/

HOOKPROTONH(closeFun, void, Object *list, Object *str)
{
    STRPTR port;

    DoMethod(list,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,(IPTR)&port);
    if (port)
    {
        TEXT buf[PORT_LEN], *dot, *digit;

        dot = strrchr(port,'.');

        if (dot)
        {
            dot++;

            for (digit = dot; *digit; digit++)
                if (!isdigit(*digit))
                {
                    dot = NULL;
                    break;
                }

                if (dot)
                {
                    strlcpy(buf, port, dot-port);
                    port = buf;
                }
        }
    }

    set(str,MUIA_String_Contents,port);
}
MakeStaticHook(closeHook, closeFun);

/***********************************************************************/

static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object *lv;

    if((obj = (Object *)DoSuperNew(cl,obj,

            MUIA_Popstring_String, ostring(GetTagData(MUIA_Popport_Len,64,msg->ops_AttrList),GetTagData(MUIA_Popport_Key,0,msg->ops_AttrList),0),
            MUIA_Popstring_Button, opopbutton(MUII_PopUp,0),

            MUIA_Popobject_Object, lv = ListviewObject,
                MUIA_Listview_List, listObject, End,
            End,
            MUIA_Popobject_WindowHook, &windowHook,
            MUIA_Popobject_StrObjHook, &openHook,
            MUIA_Popobject_ObjStrHook, &closeHook,

            TAG_MORE,(IPTR)msg->ops_AttrList)) != NULL)
    {
        DoMethod(lv,MUIM_Notify,MUIA_Listview_DoubleClick,TRUE,(IPTR)obj,2,MUIM_Popstring_Close,TRUE);
    }

    return (IPTR)obj;
}

/***********************************************************************/

SDISPATCHER(dispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return mNew(cl,obj,(APTR)msg);
        default:     return DoSuperMethodA(cl,obj,msg);
    }
}

/***********************************************************************/

BOOL initPopportClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if(initListClass() == TRUE)
    {
        if((g_popportClass = MUI_CreateCustomClass(NULL, MUIC_Popobject, NULL, 0, ENTRY(dispatcher))) != NULL)
            success = TRUE;
        else
            disposeListClass();
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void disposePopportClass(void)
{
    ENTER();

    disposeListClass();
    if(g_popportClass != NULL)
        MUI_DeleteCustomClass(g_popportClass);

    LEAVE();
}

/**************************************************************************/

