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
**  Popplaceholder replacement
*/


#include "OpenURL.h"
#include <exec/execbase.h>
#include <libraries/asl.h>

/**************************************************************************/
/*
** Place holders list
*/

static struct MUI_CustomClass *listClass = NULL;
#ifdef __AROS__
#define listObject BOOPSIOBJMACRO_START(listClass->mcc_Class)
#else
#define listObject NewObject(listClass->mcc_Class,NULL
#endif

struct listData
{
    UBYTE       **phs;
    UBYTE       **names;
    struct Hook dispHook;
};

/**************************************************************************/

#ifdef __MORPHOS__
static ULONG
conFun(void)
{
    ULONG num = REG_A1;
#elif defined(__AROS__)
AROS_UFH3S(ULONG, conFun,
AROS_UFHA(struct Hook *, hook, A0),
AROS_UFHA(APTR         , pool, A2),
AROS_UFHA(ULONG        , num , A1))
{
    AROS_USERFUNC_INIT
#else
static ULONG SAVEDS ASM
conFun(REG(a0,struct Hook *hook),REG(a2,APTR pool),REG(a1,ULONG num))
{
#endif
    return num+1;
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry conTrap = {TRAP_LIB,0,(void (*)(void))conFun};
static struct Hook conHook = {0,0,(HOOKFUNC)&conTrap};
#else
static struct Hook conHook = {0,0,(HOOKFUNC)conFun};
#endif

/**************************************************************************/

#ifdef __MORPHOS__
static void
dispFun(void)
{
    struct Hook  *hook = (struct Hook *)REG_a0;
    UBYTE        **array = (UBYTE **)REG_A2;
    ULONG        num = REG_A1;
#elif defined(__AROS__)
AROS_UFH3S(void, dispFun,
AROS_UFHA(struct Hook *, hook , A0),
AROS_UFHA(UBYTE **     , array, A2),
AROS_UFHA(ULONG        , num  , A1))
{
    AROS_USERFUNC_INIT
#else
static void SAVEDS ASM
dispFun(REG(a0,struct Hook *hook),REG(a2,UBYTE **array),REG(a1,ULONG num))
{
#endif
    struct listData *data = hook->h_Data;

    if (num)
    {
        num--;

        *array++ = data->phs[num];
        *array   = data->names[num];
    }
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry dispTrap = {TRAP_LIBNR,0,(void (*)(void))dispFun};
#endif

/**************************************************************************/

static ULONG
mListNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    struct TagItem *attrs = msg->ops_AttrList;
    UBYTE          **phs, **names;

    phs   = (UBYTE **)GetTagData(MUIA_Popph_Syms,(ULONG)NULL,attrs);
    names = (UBYTE **)GetTagData(MUIA_Popph_Names,(ULONG)NULL,attrs);
    if (!phs || !names) return 0;

    if (obj = (Object *)DoSuperNew(cl,obj,
            InputListFrame,
            MUIA_List_Format,        ",",
            MUIA_List_Pool,          g_pool,
            MUIA_List_ConstructHook, &conHook,
            TAG_MORE, attrs))
    {
        struct listData *data = INST_DATA(cl,obj);
        int             i;

        data->phs   = phs;
        data->names = names;

        #ifdef __MORPHOS__
        data->dispHook.h_Entry = (HOOKFUNC)&dispTrap;
        #else
        data->dispHook.h_Entry = (HOOKFUNC)dispFun;
        #endif
        data->dispHook.h_Data  = data;

        superset(cl,obj,MUIA_List_DisplayHook,&data->dispHook);

        conHook.h_Data  = data;

        for (i = 0; phs[i]; i++)
            DoSuperMethod(cl,obj,MUIM_List_InsertSingle,i,MUIV_List_Insert_Bottom);
    }

    return (ULONG)obj;
}

/**************************************************************************/

M_DISP(listDispatcher)
{
    M_DISPSTART

    switch (msg->MethodID)
    {
        case OM_NEW: return mListNew(cl,obj,(APTR)msg);

        default:     return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(listDispatcher)

/**************************************************************************/

static ULONG
initListClass(void)
{
    return (ULONG)(listClass = MUI_CreateCustomClass(NULL,MUIC_List,NULL,sizeof(struct listData),DISP(listDispatcher)));
}

/**************************************************************************/

static void
disposeListClass(void)
{
    if (listClass) MUI_DeleteCustomClass(listClass);
}

/**************************************************************************/

struct data
{
    Object               *str;

    struct Hook          closeHook;
    struct FileRequester *req;

    STRPTR               *phs;
    STRPTR               *names;
};

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
static void closeFun(void)
{
    struct Hook *hook = (struct Hook *)REG_A0;
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
    struct data *data = hook->h_Data;
    ULONG       a;

    get(list,MUIA_List_Active,&a);
    if (a>=0)
    {
        STRPTR buf, x;
        ULONG pos, lx, l;

        get(str,MUIA_String_BufferPos,&pos);
        get(str,MUIA_String_Contents,&x);

        lx = strlen(x);
        l  = strlen(data->phs[a]);

        if (buf = AllocPooled(g_pool,lx+l+1))
        {
            if (pos>0) CopyMem(x,buf,pos);
            CopyMem(data->phs[a],buf+pos,l);
            if (lx) CopyMem(x+pos,buf+pos+l,lx-pos+1);
            set(str,MUIA_String_Contents,buf);
            FreePooled(g_pool,buf,lx+l+1);
        }
    }
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry closeTrap = {TRAP_LIB,0,(void (*)(void))closeFun};
#endif

static ULONG
mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object         *str, *lv;
    struct TagItem *attrs = msg->ops_AttrList;
    STRPTR         *phs, *names;

    phs   = (STRPTR*)GetTagData(MUIA_Popph_Syms,(ULONG)NULL,attrs);
    if (!phs) return 0;

    names = (STRPTR*)GetTagData(MUIA_Popph_Names,FALSE,attrs);
    if (!names) return 0;

    if (obj = (Object *)DoSuperNew(cl,obj,
            MUIA_Group_Horiz,        TRUE,
            MUIA_Group_HorizSpacing, 1,

            Child, PopobjectObject,
                MUIA_Popstring_String, str = ostring(GetTagData(MUIA_Popph_MaxLen,0,attrs),GetTagData(MUIA_Popph_Key,(ULONG)NULL,attrs),0),
                MUIA_Popstring_Button, opopbutton(MUII_PopUp,0),
                MUIA_Popobject_Object, lv = ListviewObject,
                    MUIA_Listview_List, listObject,
                        MUIA_Popph_Syms,  phs,
                        MUIA_Popph_Names, names,
                    End,
                End,
                MUIA_Popobject_WindowHook, &windowHook,
            End,

            TAG_MORE, attrs))
    {

        struct data *data = INST_DATA(cl,obj);

        data->str = str;

        data->phs   = phs;
        data->names = names;

        #ifdef __MORPHOS__
        data->closeHook.h_Entry = (HOOKFUNC)&closeTrap;
        #else
        data->closeHook.h_Entry = (HOOKFUNC)closeFun;
        #endif
        data->closeHook.h_Data  = data;
        set(obj,MUIA_Popobject_ObjStrHook,&data->closeHook);

        if (GetTagData(MUIA_Popph_Asl,FALSE,attrs))
        {
            APTR req;

            if (req = MUI_AllocAslRequest(ASL_FileRequest,NULL))
            {
                Object *bt;

                if (bt = opopbutton(MUII_PopFile,0))
                {
                    DoSuperMethod(cl,obj,OM_ADDMEMBER,(ULONG)bt);

                    data->req = req;

                    DoMethod(bt,MUIM_Notify,MUIA_Pressed,FALSE,(ULONG)obj,1,MUIM_Popph_RequestFile);
                }
                else MUI_FreeAslRequest(req);
            }
        }

        DoMethod(lv,MUIM_Notify,MUIA_Listview_DoubleClick,TRUE,(ULONG)obj,2,MUIM_Popstring_Close,TRUE);
    }

    return (ULONG)obj;
}

/***********************************************************************/

static ULONG
mDispose(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    if (data->req) MUI_FreeAslRequest(data->req);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

#ifdef __MORPHOS__
static void
reqIntuiFun(void)
{
    struct Hook     *hook = (struct Hook *)REG_A0;
    struct IntuiMessage *imsg = (struct IntuiMessage *)REG_A1;
#elif defined(__AROS__)
AROS_UFH3S(void, reqIntuiFun,
AROS_UFHA(struct Hook *        , hook, A0),
AROS_UFHA(Object *             , dummy, A2),
AROS_UFHA(struct IntuiMessage *, imsg, A1))
{
    AROS_USERFUNC_INIT
#else
static void SAVEDS ASM
reqIntuiFun(REG(a0,struct Hook *hook),REG(a1,struct IntuiMessage *imsg))
{
#endif
    if (imsg->Class==IDCMP_REFRESHWINDOW)
    DoMethod(hook->h_Data,MUIM_Application_CheckRefresh);
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

#ifdef __MORPHOS__
static struct EmulLibEntry reqIntuiTrap = {TRAP_LIBNR,0,(void (*)(void))reqIntuiFun};
#endif

static ULONG
mRequestFile(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);
    struct Hook reqIntuiHook = {0};
    TEXT       path[256], *x, *file, *p;

    set(_app(obj),MUIA_Application_Sleep,TRUE);

    #ifdef __MORPHOS__
    reqIntuiHook.h_Entry = (HOOKFUNC)&reqIntuiTrap;
    #else
    reqIntuiHook.h_Entry = (HOOKFUNC)reqIntuiFun;
    #endif
    reqIntuiHook.h_Data  = _app(obj);


    get(data->str,MUIA_String_Contents,&x);
    file = FilePart(x);
    if (p = PathPart(x))
    {
        stccpy(path,x,p-x+1);
        p = path;
    }

    if (MUI_AslRequestTags(data->req,
                           ASLFR_InitialFile,       (ULONG)file,
                           p ? ASLFR_InitialDrawer : TAG_IGNORE, (ULONG)p,
                           ASLFR_IntuiMsgFunc,      (ULONG)&reqIntuiHook,
                           ASLFR_Window,            (ULONG)_window(obj),
                           ASLFR_PrivateIDCMP,      TRUE,
                           ASLFR_Flags1,            FRF_INTUIFUNC,
                           TAG_DONE))
    {
        TEXT buf[256];

        strcpy(buf,data->req->fr_Drawer);
        AddPart(buf,data->req->fr_File,sizeof(buf));

        if (*buf) set(data->str,MUIA_String_Contents,buf);
    }
    else if (IoErr()) DisplayBeep(0);

    set(_app(obj),MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/

M_DISP(dispatcher)
{
    M_DISPSTART

    switch (msg->MethodID)
    {
        case OM_NEW:                 return mNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:             return mDispose(cl,obj,(APTR)msg);

        case MUIM_Popph_RequestFile: return mRequestFile(cl,obj,(APTR)msg);

        default:                     return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(dispatcher)

/***********************************************************************/

ULONG
initPopphClass(void)
{
    if (initListClass())
    {
        if (g_popphClass = MUI_CreateCustomClass(NULL,MUIC_Group,NULL,sizeof(struct data),DISP(dispatcher)))
        {
            return TRUE;
        }

        disposeListClass();
    }

    return FALSE;
}

/**************************************************************************/

void
disposePopphClass(void)
{
    disposeListClass();
    if (g_popphClass) MUI_DeleteCustomClass(g_popphClass);
}

/**************************************************************************/

