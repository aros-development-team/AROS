/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
**
**  Pop public ports object
*/


#include "OpenURL.h"
#include "libraries/openurl.h"
#include <exec/execbase.h>

/**************************************************************************/
/*
** Public ports list
*/

static struct MUI_CustomClass *listClass = NULL;
#ifdef __AROS__
#define listObject BOOPSIOBJMACRO_START(listClass->mcc_Class)
#else
#define listObject NewObject(listClass->mcc_Class,NULL
#endif

static ULONG
mListNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    return (ULONG)DoSuperNew(cl,obj,
      MUIA_Frame,              MUIV_Frame_InputList,
      MUIA_Background,         MUII_ListBack,
      MUIA_List_AutoVisible,   TRUE,
      MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
      MUIA_List_DestructHook,  MUIV_List_DestructHook_String,
      TAG_MORE,(ULONG)msg->ops_AttrList);
}

/**************************************************************************/

static ULONG
mListSetup(struct IClass *cl,Object *obj,Msg msg)
{
    struct Node *mstate;

    if (!DoSuperMethodA(cl,obj,msg)) return FALSE;

    DoSuperMethod(cl,obj,MUIM_List_Clear);

    Forbid();

    for (mstate = SysBase->PortList.lh_Head; mstate->ln_Succ; mstate = mstate->ln_Succ)
        DoSuperMethod(cl,obj,MUIM_List_InsertSingle,(ULONG)mstate->ln_Name,MUIV_List_Insert_Sorted);

    Permit();

    return TRUE;
}

/**************************************************************************/

M_DISP(listDispatcher)
{
    M_DISPSTART

    switch (msg->MethodID)
    {
        case OM_NEW:     return mListNew(cl,obj,(APTR)msg);

        case MUIM_Setup: return mListSetup(cl,obj,(APTR)msg);

        default:         return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(listDispatcher)

/**************************************************************************/

static ULONG
initListClass(void)
{
    return (ULONG)(listClass = MUI_CreateCustomClass(NULL,
#ifdef __AROS__
        // Zune Listclass is too buggy
        MUIC_NList,
#else
        MUIC_List,
#endif
        NULL,0,DISP(listDispatcher)));
}

/**************************************************************************/

static void
disposeListClass(void)
{
    if (listClass) MUI_DeleteCustomClass(listClass);
}

/**************************************************************************/

#ifdef __MORPHOS__
static void
windowFun(void)
{
    //struct Hook *hook = (struct Hook *)REG_A0;
    Object      *pop = (Object *)REG_A2;
    Object      *win = (Object *)REG_A1;
#elif defined(__AROS__)
AROS_UFH3S(void, windowFun,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(Object *     , pop , A2),
AROS_UFHA(Object *     , win , A1))
{
    AROS_USERFUNC_INIT
#else
static void SAVEDS ASM
windowFun(REG(a0,struct Hook *hook),REG(a2,Object *pop),REG(a1,Object *win))
{
#endif
    set(win,MUIA_Window_DefaultObject,pop);
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry windowTrap = {TRAP_LIB,0,(void (*)(void))windowFun};
static struct Hook windowHook = {0,0,(HOOKFUNC)&windowTrap};
#else
static struct Hook windowHook = {0,0,(HOOKFUNC)&windowFun};
#endif

/***********************************************************************/

#ifdef __MORPHOS__
static ULONG
openFun(void)
{
    //struct Hook *hook = (struct Hook *)REG_A0;
    Object      *list = (Object *)REG_A2;
    Object      *str = (Object *)REG_A1;
#elif defined(__AROS__)
AROS_UFH3S(ULONG, openFun,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(Object *     , list, A2),
AROS_UFHA(Object *     , str , A1))
{
    AROS_USERFUNC_INIT
#else
static ULONG SAVEDS ASM
openFun(REG(a0,struct Hook *hook),REG(a2,Object *list),REG(a1,Object *str))
{
#endif
    STRPTR s, x;
    int   i;

    s = (STRPTR)xget(str,MUIA_String_Contents);

    for (i = 0; ;i++)
    {
        DoMethod(list,MUIM_List_GetEntry,i,(ULONG)&x);
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
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry openTrap = {TRAP_LIB,0,(void (*)(void))openFun};
static struct Hook openHook = {0,0,(HOOKFUNC)&openTrap};
#else
static struct Hook openHook = {0,0,(HOOKFUNC)&openFun};
#endif

/***********************************************************************/

#ifdef __MORPHOS__
static void closeFun(void)
{
    //struct Hook *hook = (struct Hook *)REG_A0;
    Object      *list = (Object *)REG_A2;
    Object      *str = (Object *)REG_A1;
#elif defined(__AROS__)
AROS_UFH3S(void, closeFun,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(Object *     , list, A2),
AROS_UFHA(Object *     , str , A1))
{
    AROS_USERFUNC_INIT
#else
static void SAVEDS ASM
closeFun(REG(a0,struct Hook *hook),REG(a2,Object *list),REG(a1,Object *str))
{
#endif
    STRPTR port;

    DoMethod(list,MUIM_List_GetEntry,MUIV_List_GetEntry_Active,(ULONG)&port);
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
                    stccpy(buf,port,dot-port);
                    port = buf;
                }
        }
    }

    set(str,MUIA_String_Contents,port);
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry closeTrap = {TRAP_LIB,0,(void (*)(void))closeFun};
static struct Hook closeHook = {0,0,(HOOKFUNC)&closeTrap};
#else
static struct Hook closeHook = {0,0,(HOOKFUNC)&closeFun};
#endif

/***********************************************************************/

static ULONG
mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object *lv;

    if (obj = (Object *)DoSuperNew(cl,obj,

            MUIA_Popstring_String, ostring(GetTagData(MUIA_Popport_Len,64,msg->ops_AttrList),GetTagData(MUIA_Popport_Key,0,msg->ops_AttrList),0),
            MUIA_Popstring_Button, opopbutton(MUII_PopUp,0),

            MUIA_Popobject_Object, lv = ListviewObject,
                MUIA_Listview_List, listObject, End,
            End,
            MUIA_Popobject_WindowHook, &windowHook,
            MUIA_Popobject_StrObjHook, &openHook,
            MUIA_Popobject_ObjStrHook, &closeHook,

            TAG_MORE,(ULONG)msg->ops_AttrList))
    {
        DoMethod(lv,MUIM_Notify,MUIA_Listview_DoubleClick,TRUE,(ULONG)obj,2,MUIM_Popstring_Close,TRUE);
    }

    return (ULONG)obj;
}

/***********************************************************************/

M_DISP(dispatcher)
{
    M_DISPSTART

    switch (msg->MethodID)
    {
        case OM_NEW: return mNew(cl,obj,(APTR)msg);
        default:     return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(dispatcher)

/***********************************************************************/

ULONG
initPopportClass(void)
{
    if (initListClass())
    {
        if (g_popportClass = MUI_CreateCustomClass(NULL,MUIC_Popobject,NULL,0,DISP(dispatcher)))
            return TRUE;

        disposeListClass();
    }

    return FALSE;
}

/**************************************************************************/

void
disposePopportClass(void)
{
    disposeListClass();
    if (g_popportClass) MUI_DeleteCustomClass(g_popportClass);
}

/**************************************************************************/

