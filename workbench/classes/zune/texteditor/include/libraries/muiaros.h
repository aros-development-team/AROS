/*
    Copyright  2002-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef LIBRARIES_MUIAROS_H
#define LIBRARIES_MUIAROS_H

#ifndef INTUITION_CLASSES_H
#   include <intuition/classes.h>
#endif
#ifndef GRAPHICS_GRAPHICS_H
#   include <graphics/gfx.h>
#endif
#ifndef GRAPHICS_RASTPORT_H
#   include <graphics/rastport.h>
#endif
#ifndef LIBRARIES_IFFPARSE_H
#   include <libraries/iffparse.h>
#endif

#ifdef __AROS__
#   ifndef AROS_ASMCALL_H
#       include <aros/asmcall.h>
#   endif
#   define SAVEDS
#else
/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUIMASTER_SUPPORT_AMIGAOS_H_
#define _MUIMASTER_SUPPORT_AMIGAOS_H_

#ifdef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef AMIGA_COMILER_H
#include <amiga_compiler.h>
#endif

#ifndef PROTO_UTILITY_H
#include <proto/utility.h>
#endif

/* These are the identity function under AmigaOS */
#define AROS_LONG2BE(x) (x)
#define AROS_BE2LONG(x) (x)

#define IMSPEC_EXTERNAL_PREFIX "MUI:Images/"

/* Define all classes as built in...should be moved out to config.h like file */
#define ZUNE_BUILTIN_ABOUTMUI 1
#define ZUNE_BUILTIN_BALANCE 1
#define ZUNE_BUILTIN_BOOPSI 1
#define ZUNE_BUILTIN_COLORADJUST 1
#define ZUNE_BUILTIN_COLORFIELD 1
#define ZUNE_BUILTIN_FRAMEADJUST 1
#define ZUNE_BUILTIN_FRAMEDISPLAY 1
#define ZUNE_BUILTIN_GAUGE 1
#define ZUNE_BUILTIN_ICONLISTVIEW 0
#define ZUNE_BUILTIN_IMAGEADJUST 1
#define ZUNE_BUILTIN_IMAGEDISPLAY 1
#define ZUNE_BUILTIN_PENADJUST 1
#define ZUNE_BUILTIN_PENDISPLAY 1
#define ZUNE_BUILTIN_POPASL 1
#define ZUNE_BUILTIN_POPFRAME 1
#define ZUNE_BUILTIN_POPIMAGE 1
#define ZUNE_BUILTIN_POPPEN 1
#define ZUNE_BUILTIN_RADIO 1
#define ZUNE_BUILTIN_SCALE 1
#define ZUNE_BUILTIN_SCROLLGROUP 1
#define ZUNE_BUILTIN_SETTINGSGROUP 1
#define ZUNE_BUILTIN_VIRTGROUP 1

#ifdef __SASC
#include <dos.h>
#endif

#ifndef PI
#define PI 3.1415
#endif

#ifndef M_PI
#define M_PI PI
#endif

#define AROS_STACKSIZE 65536

char *StrDup(const char *x);
Object *DoSuperNewTagList(struct IClass *cl, Object *obj,void *dummy, struct TagItem *tags);
Object *VARARGS68K DoSuperNewTags(struct IClass *cl, Object *obj, void *dummy, ...);
int VARARGS68K SPrintf(char *buf, const char *fmt, ...);




/*** HookEntry for OS4 (is only a dummy) ************************************/
#ifdef __amigaos4__
ASM ULONG HookEntry(REG(a0, struct Hook *hook),REG(a2, APTR obj), REG(a1, APTR msg));
#endif

/*** OS4 Exec Interface support *********************************************/
#ifdef __amigaos4__
#define EXEC_INTERFACE_DECLARE(x) x
#define EXEC_INTERFACE_GET_MAIN(interface,libbase) (interface = (void*)GetInterface(libbase,"main",1,NULL))
#define EXEC_INTERFACE_DROP(interface) DropInterface((struct Interface*)interface)
#define EXEC_INTERFACE_ASSIGN(a,b) (a = b)
#else
#define EXEC_INTERFACE_DECLARE(x)
#define EXEC_INTERFACE_GET_MAIN(interface,libbase) 1
#define EXEC_INTERFACE_DROP(interface)
#define EXEC_INTERFACE_ASSIGN(a,b)
#endif

/*** AROS Exec extensions ***************************************************/
#ifndef __amigaos4__
APTR AllocVecPooled(APTR pool, ULONG size);
VOID FreeVecPooled(APTR pool, APTR memory);
#endif

/*** AROS Intuition extensions **********************************************/
#define DeinitRastPort(rp)      
#define CloneRastPort(rp) (rp)  
#define FreeRastPort(rp)        

/*** Miscellanous compiler supprot ******************************************/
#ifndef SAVEDS
#   ifdef __MAXON__
#       define __asm
#       define __inline
#       define SAVEDS
#       define const
#   else
#       define SAVEDS __saveds
#   endif
#endif 

#define __stackparm

/*** Miscellanous AROS macros ***********************************************/
#define AROS_LIBFUNC_INIT
#define AROS_LIBBASE_EXT_DECL(a, b) extern a b;
#define AROS_LIBFUNC_EXIT
#define AROS_ASMSYMNAME(a) a

#define LC_BUILDNAME(x) x
#define LIBBASETYPEPTR struct Library *

/*** AROS types *************************************************************/
#ifndef __AROS_TYPES_DEFINED__
#   define __AROS_TYPES_DEFINED__
    typedef unsigned long IPTR;
    typedef long          STACKLONG;
    typedef unsigned long STACKULONG;
    typedef void (*VOID_FUNC)();
#define STACKED 
#endif /* __AROS_TYPES_DEFINED__ */

/*** AROS list macros *******************************************************/
#define ForeachNode(l,n)                       \
for                                            \
(                                              \
    n=(void *)(((struct List *)(l))->lh_Head); \
    ((struct Node *)(n))->ln_Succ;             \
    n=(void *)(((struct Node *)(n))->ln_Succ)  \
)

/*** AROS register definitions **********************************************/
#define __REG_D0 __d0
#define __REG_D1 __d1
#define __REG_D2 __d2
#define __REG_D3 __d3
#define __REG_D4 __d4
#define __REG_D5 __d5
#define __REG_D6 __d6
#define __REG_D7 __d7
#define __REG_A0 __a0
#define __REG_A1 __a1
#define __REG_A2 __a2
#define __REG_A3 __a3
#define __REG_A4 __a4
#define __REG_A5 __a5
#define __REG_A6 __a6
#define __REG_A7 __a7

/*** AROS library function macros *******************************************/
#define AROS_LH0(rt, fn, bt, bn, lvo, p) \
    ASM rt LIB_##fn (void)
#define AROS_LH1(rt, fn, a1, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1)
#define AROS_LH2(rt, fn, a1, a2, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1, a2)
#define AROS_LH3(rt, fn, a1, a2, a3, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1, a2, a3)
#define AROS_LH4(rt, fn, a1, a2, a3, a4, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1, a2, a3, a4)
#define AROS_LH5(rt, fn, a1, a2, a3, a4, a5, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1, a2, a3, a4, a5)
#define AROS_LH6(rt, fn, a1, a2, a3, a4, a5, a6, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1, a2, a3, a4, a5, a6)
#define AROS_LH7(rt, fn, a1, a2, a3, a4, a5, a6, a7, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1, a2, a3, a4, a5, a6, a7)
#define AROS_LH8(rt, fn, a1, a2, a3, a4, a5, a6, a7, a8, bt, bn, lvo, p) \
    ASM rt LIB_##fn (a1, a2, a3, a4, a5, a6, a7, a8)

#ifdef __SASC
#   define AROS_LHA(type, name, reg) register __REG_##reg type name
#else
#   define AROS_LHA(type, name, reg) type name
#endif

/*** AROS user function macros **********************************************/
#define AROS_USERFUNC_INIT
#define AROS_USERFUNC_EXIT

#define AROS_UFH0(rt, fn) \
    ASM rt fn (void)
#define AROS_UFH1(rt, fn, a1) \
    ASM rt fn (a1)
#define AROS_UFH2(rt, fn, a1, a2) \
    ASM rt fn (a1, a2)
#define AROS_UFH3(rt, fn, a1, a2, a3) \
    ASM rt fn (a1, a2, a3)
#define AROS_UFH4(rt, fn, a1, a2, a3, a4) \
    ASM rt fn (a1, a2, a3, a4)
#define AROS_UFH5(rt, fn, a1, a2, a3, a4, a5) \
    ASM rt fn (a1, a2, a3, a4, a5)
#define AROS_UFH6(rt, fn, a1, a2, a3, a4, a5, a6) \
    ASM rt fn (a1, a2, a3, a4, a5, a6)
#define AROS_UFH7(rt, fn, a1, a2, a3, a4, a5, a6, a7) \
    ASM rt fn (a1, a2, a3, a4, a5, a6, a7)
#define AROS_UFH8(rt, fn, a1, a2, a3, a4, a5, a6, a7, a8) \
    ASM rt fn (a1, a2, a3, a4, a5, a6, a7, a8)

#define AROS_UFH0S(rt, fn) \
    ASM static rt fn (void)
#define AROS_UFH1S(rt, fn, a1) \
    ASM static rt fn (a1)
#define AROS_UFH2S(rt, fn, a1, a2) \
    ASM static rt fn (a1, a2)
#define AROS_UFH3S(rt, fn, a1, a2, a3) \
    ASM static rt fn (a1, a2, a3)
#define AROS_UFH4S(rt, fn, a1, a2, a3, a4) \
    ASM static rt fn (a1, a2, a3, a4)
#define AROS_UFH5S(rt, fn, a1, a2, a3, a4, a5) \
    ASM static rt fn (a1, a2, a3, a4, a5)
#define AROS_UFH6S(rt, fn, a1, a2, a3, a4, a5, a6) \
    ASM static rt fn (a1, a2, a3, a4, a5, a6)
#define AROS_UFH7S(rt, fn, a1, a2, a3, a4, a5, a6, a7) \
    ASM static rt fn (a1, a2, a3, a4, a5, a6, a7)
#define AROS_UFH8S(rt, fn, a1, a2, a3, a4, a5, a6, a7, a8) \
    ASM static rt fn (a1, a2, a3, a4, a5, a6, a7, a8)

#ifdef __SASC
#   define AROS_UFHA(type, name, reg) register __REG_##reg type name
#else
#   define AROS_UFHA(type, name, reg) type name
#endif

#define AROS_UFP0 AROS_UFH0
#define AROS_UFP1 AROS_UFH1
#define AROS_UFP2 AROS_UFH2
#define AROS_UFP3 AROS_UFH3
#define AROS_UFP4 AROS_UFH4
#define AROS_UFP5 AROS_UFH5
#define AROS_UFP6 AROS_UFH6
#define AROS_UFP7 AROS_UFH7
#define AROS_UFP8 AROS_UFH8

#define AROS_UFPA AROS_UFHA

/* 
    With the following define a typical dispatcher will looks like this:
    BOOPSI_DISPATCHER(IPTR,IconWindow_Dispatcher,cl,obj,msg)
*/
#define BOOPSI_DISPATCHER(rettype,name,cl,obj,msg) \
    AROS_UFH3(SAVEDS rettype, name,\
        AROS_UFHA(Class  *, cl,  A0),\
        AROS_UFHA(Object *, obj, A2),\
        AROS_UFHA(Msg     , msg, A1)) {AROS_USERFUNC_INIT
#define BOOPSI_DISPATCHER_END AROS_USERFUNC_EXIT}
#define BOOPSI_DISPATCHER_PROTO(rettype,name,cl,obj,msg) \
    AROS_UFP3(SAVEDS rettype, name,\
        AROS_UFPA(Class  *, cl,  A0),\
        AROS_UFPA(Object *, obj, A2),\
        AROS_UFPA(Msg     , msg, A1))


#endif /* _MUIMASTER_SUPPORT_AMIGAOS_H_ */
#endif


#ifndef _MUI_IDENTIFIERS_H
#define _MUI_IDENTIFIERS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>

#define MUIB_MUI  (TAG_USER)                /* Base for legacy MUI identifiers   */
#define MUIB_RSVD (MUIB_MUI  | 0x10400000)  /* Base for AROS reserved range      */
#define MUIB_ZUNE (MUIB_RSVD | 0x00020000)  /* Base for Zune core reserved range */
#define MUIB_AROS (MUIB_RSVD | 0x00070000)  /* Base for AROS core reserved range */

#endif /* _MUI_IDENTIFIERS_H */

#ifndef __AROS__
#define MUIMASTER_NAME "zunemaster.library"
#define MUIMASTER_VMIN    0
#define MUIMASTER_VLATEST 0
#else
#define MUIMASTER_NAME "muimaster.library"
#define MUIMASTER_VMIN    11
#define MUIMASTER_VLATEST 19
#endif


/* This structure is used for the internal classes */

struct __MUIBuiltinClass {
    CONST_STRPTR name;
    CONST_STRPTR supername;
    ULONG        datasize;

#ifndef __AROS__
    ULONG	   (*dispatcher)();
#else
    AROS_UFP3(IPTR, (*dispatcher),
        AROS_UFPA(Class  *,  cl, A0),
        AROS_UFPA(Object *, obj, A2),
        AROS_UFPA(Msg     , msg, A1));
#endif
};


#if defined(MUIMASTER_YES_INLINE_STDARG) && \
    !defined(NO_INLINE_STDARG)           && \
    !defined(__SASC)

#define MUIOBJMACRO_START(class) (APTR) \
({                                      \
     ClassID __class = (ClassID) class; \
     enum { __ismuiobjmacro = 1 };      \
     IPTR __tags[] = {0

#define BOOPSIOBJMACRO_START(class) (APTR) \
({                                         \
     Class *__class = (Class *) class;     \
     enum { __ismuiobjmacro = 0 };         \
     IPTR __tags[] = {0

#define OBJMACRO_END                                                            \
     TAG_DONE};                                                                 \
     (                                                                          \
         __ismuiobjmacro                                                        \
         ? MUI_NewObjectA((ClassID)__class, (struct TagItem *)(__tags + 1)) \
         : NewObjectA((Class *)__class, NULL, (struct TagItem *)(__tags + 1))   \
     );                                                                         \
})

#else

#ifdef __amigaos4__
#   define MUIOBJMACRO_START(class) (IZuneMaster->MUI_NewObject)(class
#   define BOOPSIOBJMACRO_START(class) (IIntuition->NewObject)(class, NULL
#else
#   define MUIOBJMACRO_START(class) MUI_NewObject(class
#   define BOOPSIOBJMACRO_START(class) NewObject(class, NULL
#endif

#define OBJMACRO_END TAG_DONE)

#endif

#ifndef _MUI_CLASSES_NOTIFY_H
#ifndef _MUI_CLASSES_NOTIFY_H
#define _MUI_CLASSES_NOTIFY_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

struct MUI_NotifyData
{
    struct MUI_GlobalInfo *mnd_GlobalInfo;
    IPTR                  mnd_UserData;
    ULONG                  mnd_ObjectID;

    /* private starts here */
    struct MinList        *mnd_NotifyList; /* priv1 */
    Object                *mnd_ParentObject;/* priv2 */
    STRPTR                 mnd_HelpNode;/* priv3 */
    LONG                   mnd_HelpLine;/* priv4 */
};

/*** Name *******************************************************************/
#define MUIC_Notify              "Notify.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Notify              (MUIB_ZUNE | 0x00001d00)  

/*** Methods ****************************************************************/
#define MUIM_CallHook            (MUIB_MUI|0x0042b96b) /* MUI: V4  */
#define MUIM_Export              (MUIB_MUI|0x00420f1c) /* MUI: V12 */
#define MUIM_FindUData           (MUIB_MUI|0x0042c196) /* MUI: V8  */
#define MUIM_GetConfigItem       (MUIB_MUI|0x00423edb) /* MUI: V11 */
#define MUIM_GetUData            (MUIB_MUI|0x0042ed0c) /* MUI: V8  */
#define MUIM_Import              (MUIB_MUI|0x0042d012) /* MUI: V12 */
#define MUIM_KillNotify          (MUIB_MUI|0x0042d240) /* MUI: V4  */
#define MUIM_KillNotifyObj       (MUIB_MUI|0x0042b145) /* MUI: V16 */
#define MUIM_MultiSet            (MUIB_MUI|0x0042d356) /* MUI: V7  */
#define MUIM_NoNotifySet         (MUIB_MUI|0x0042216f) /* MUI: V9  */
#define MUIM_Notify              (MUIB_MUI|0x0042c9cb) /* MUI: V4  */
#define MUIM_Set                 (MUIB_MUI|0x0042549a) /* MUI: V4  */
#define MUIM_SetAsString         (MUIB_MUI|0x00422590) /* MUI: V4  */
#define MUIM_SetUData            (MUIB_MUI|0x0042c920) /* MUI: V8  */
#define MUIM_SetUDataOnce        (MUIB_MUI|0x0042ca19) /* MUI: V11 */
#define MUIM_WriteLong           (MUIB_MUI|0x00428d86) /* MUI: V6  */
#define MUIM_WriteString         (MUIB_MUI|0x00424bf4) /* MUI: V6  */
struct MUIP_CallHook             {STACKED ULONG MethodID; STACKED struct Hook *Hook; STACKED IPTR param1; /* more might follow */};
struct MUIP_Export               {STACKED ULONG MethodID; STACKED Object *dataspace;};
struct MUIP_FindUData            {STACKED ULONG MethodID; STACKED IPTR udata;};
struct MUIP_GetConfigItem        {STACKED ULONG MethodID; STACKED ULONG id; STACKED IPTR *storage;};
struct MUIP_GetUData             {STACKED ULONG MethodID; STACKED ULONG udata; STACKED ULONG attr; STACKED IPTR *storage;};
struct MUIP_Import               {STACKED ULONG MethodID; STACKED Object *dataspace;};
struct MUIP_KillNotify           {STACKED ULONG MethodID; STACKED ULONG TrigAttr;};
struct MUIP_KillNotifyObj        {STACKED ULONG MethodID; STACKED ULONG TrigAttr; STACKED Object *dest;};
struct MUIP_MultiSet             {STACKED ULONG MethodID; STACKED ULONG attr; STACKED IPTR val; STACKED APTR obj; /* more might follow */};
struct MUIP_NoNotifySet          {STACKED ULONG MethodID; STACKED ULONG attr; STACKED IPTR val; /* more might follow */};
struct MUIP_Notify               {STACKED ULONG MethodID; STACKED ULONG TrigAttr; STACKED IPTR TrigVal; STACKED APTR DestObj; STACKED ULONG FollowParams; /* more might follow */};
struct MUIP_Set                  {STACKED ULONG MethodID; STACKED ULONG attr; STACKED IPTR val;};
struct MUIP_SetAsString          {STACKED ULONG MethodID; STACKED ULONG attr; STACKED char *format; STACKED IPTR val; /* more might follow */};
struct MUIP_SetUData             {STACKED ULONG MethodID; STACKED IPTR udata; STACKED ULONG attr; STACKED IPTR val;};
struct MUIP_SetUDataOnce         {STACKED ULONG MethodID; STACKED IPTR udata; STACKED ULONG attr; STACKED IPTR val;};
struct MUIP_WriteLong            {STACKED ULONG MethodID; STACKED ULONG val; STACKED ULONG *memory;};
struct MUIP_WriteString          {STACKED ULONG MethodID; STACKED char *str; STACKED char *memory;};

#define MUIM_ConnectParent       (MUIB_Notify | 0x00000000) /* Zune: V1 */
#define MUIM_DisconnectParent    (MUIB_Notify | 0x00000001) /* Zune: V1 */
struct MUIP_ConnectParent        {STACKED ULONG MethodID; STACKED Object *parent;};
struct MUIP_DisconnectParent     {STACKED ULONG MethodID;};

/*** Attributes *************************************************************/
#define MUIA_ApplicationObject   (MUIB_MUI|0x0042d3ee) /* MUI: V4  ..g Object *            */
#define MUIA_AppMessage          (MUIB_MUI|0x00421955) /* MUI: V5  ..g struct AppMessage * */
#define MUIA_HelpLine            (MUIB_MUI|0x0042a825) /* MUI: V4  isg LONG                */
#define MUIA_HelpNode            (MUIB_MUI|0x00420b85) /* MUI: V4  isg STRPTR              */
#define MUIA_NoNotify            (MUIB_MUI|0x004237f9) /* MUI: V7  .s. BOOL                */
#define MUIA_ObjectID            (MUIB_MUI|0x0042d76e) /* MUI: V11 isg ULONG               */
#define MUIA_Parent              (MUIB_MUI|0x0042e35f) /* MUI: V11 ..g Object *            */
#define MUIA_Revision            (MUIB_MUI|0x00427eaa) /* MUI: V4  ..g LONG                */
#define MUIA_UserData            (MUIB_MUI|0x00420313) /* MUI: V4  isg ULONG               */
#define MUIA_Version             (MUIB_MUI|0x00422301) /* MUI: V4  ..g LONG                */

/* Special values for MUIM_Notify */
#define MUIV_TriggerValue    0x49893131UL
#define MUIV_NotTriggerValue 0x49893133UL
#define MUIV_EveryTime       0x49893131UL /* as TrigVal */

enum
{
    MUIV_Notify_Self = 1,
    MUIV_Notify_Window,
    MUIV_Notify_Application,
    MUIV_Notify_Parent,
};


/* Private stuff */

#define _parent(obj)    (muiNotifyData(obj)->mnd_ParentObject) /* Shortcut */


#endif /* _MUI_CLASSES_NOTIFY_H */
#endif

#ifndef _MUI_CLASSES_FAMILY_H
#ifndef _CLASSES_FAMILY_H
#define _CLASSES_FAMILY_H

/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Family            "Family.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Family            (MUIB_ZUNE | 0x00000c00)

/*** Methods ****************************************************************/
#define MUIM_Family_AddHead    (MUIB_MUI|0x0042e200) /* MUI: V8  */
#define MUIM_Family_AddTail    (MUIB_MUI|0x0042d752) /* MUI: V8  */
#define MUIM_Family_Insert     (MUIB_MUI|0x00424d34) /* MUI: V8  */
#define MUIM_Family_Remove     (MUIB_MUI|0x0042f8a9) /* MUI: V8  */
#define MUIM_Family_Sort       (MUIB_MUI|0x00421c49) /* MUI: V8  */
#define MUIM_Family_Transfer   (MUIB_MUI|0x0042c14a) /* MUI: V8  */
struct MUIP_Family_AddHead     {STACKED ULONG MethodID; STACKED Object *obj;};
struct MUIP_Family_AddTail     {STACKED ULONG MethodID; STACKED Object *obj;};
struct MUIP_Family_Insert      {STACKED ULONG MethodID; STACKED Object *obj; STACKED Object *pred;};
struct MUIP_Family_Remove      {STACKED ULONG MethodID; STACKED Object *obj;};
struct MUIP_Family_Sort        {STACKED ULONG MethodID; STACKED Object *obj[1];};
struct MUIP_Family_Transfer    {STACKED ULONG MethodID; STACKED Object *family;};

/*** Attributes *************************************************************/
#define MUIA_Family_Child      (MUIB_MUI|0x0042c696) /* MUI: V8  i.. Object *          */
#define MUIA_Family_List       (MUIB_MUI|0x00424b9e) /* MUI: V8  ..g struct MinList *  */



#endif /* _CLASSES_FAMILY_H */
#endif

#ifndef _MUI_CLASSES_APPLICATION_H
/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_APPLICATION_H
#define _MUI_CLASSES_APPLICATION_H

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

/*** Name *******************************************************************/
#define MUIC_Application                        "Application.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Application                        (MUIB_ZUNE | 0x00000100)

/*** Methods ****************************************************************/
#define MUIM_Application_AboutMUI		(MUIB_MUI|0x0042d21d) /* MUI: V14 */
#define MUIM_Application_AddInputHandler	(MUIB_MUI|0x0042f099) /* MUI: V11 */
#define MUIM_Application_CheckRefresh		(MUIB_MUI|0x00424d68) /* MUI: V11 */
#define MUIM_Application_GetMenuCheck		(MUIB_MUI|0x0042c0a7) /* MUI: V4  */
#define MUIM_Application_GetMenuState		(MUIB_MUI|0x0042a58f) /* MUI: V4  */
#define MUIM_Application_Input			(MUIB_MUI|0x0042d0f5) /* MUI: V4  */
#define MUIM_Application_InputBuffered		(MUIB_MUI|0x00427e59) /* MUI: V4  */
#define MUIM_Application_Load			(MUIB_MUI|0x0042f90d) /* MUI: V4  */
#define MUIM_Application_NewInput		(MUIB_MUI|0x00423ba6) /* MUI: V11 */
#define MUIM_Application_OpenConfigWindow	(MUIB_MUI|0x004299ba) /* MUI: V11 */
#define MUIM_Application_PushMethod		(MUIB_MUI|0x00429ef8) /* MUI: V4  */
#define MUIM_Application_RemInputHandler	(MUIB_MUI|0x0042e7af) /* MUI: V11 */
#define MUIM_Application_ReturnID		(MUIB_MUI|0x004276ef) /* MUI: V4  */
#define MUIM_Application_Save			(MUIB_MUI|0x004227ef) /* MUI: V4  */
#define MUIM_Application_SetConfigItem		(MUIB_MUI|0x00424a80) /* MUI: V11 */
#define MUIM_Application_SetMenuCheck		(MUIB_MUI|0x0042a707) /* MUI: V4  */
#define MUIM_Application_SetMenuState		(MUIB_MUI|0x00428bef) /* MUI: V4  */
#define MUIM_Application_ShowHelp		(MUIB_MUI|0x00426479) /* MUI: V4  */

#define MUIM_Application_SetConfigdata		(MUIB_Application | 0x00000000) /* Zune 20030407 */
#define MUIM_Application_OpenWindows		(MUIB_Application | 0x00000001) /* Zune 20030407 */
#define MUIM_Application_Iconify                (MUIB_Application | 0x00000002) /* Zune: V1  */
#define MUIM_Application_Execute                (MUIB_Application | 0x00000003)
#define MUIM_Application_UpdateMenus            (MUIB_Application | 0x00000004) /* Zune 20070712 */
/* Method Structures */
struct MUIP_Application_AboutMUI		{ STACKED ULONG MethodID; STACKED Object *refwindow; };
struct MUIP_Application_AddInputHandler	{ STACKED ULONG MethodID; STACKED struct MUI_InputHandlerNode *ihnode; };
struct MUIP_Application_CheckRefresh		{ STACKED ULONG MethodID; };
struct MUIP_Application_GetMenuCheck		{ STACKED ULONG MethodID; STACKED ULONG MenuID; };
struct MUIP_Application_GetMenuState		{ STACKED ULONG MethodID; STACKED ULONG MenuID; };
struct MUIP_Application_Input			{ STACKED ULONG MethodID; STACKED ULONG *signal; };
struct MUIP_Application_InputBuffered		{ STACKED ULONG MethodID; };
struct MUIP_Application_Load			{ STACKED ULONG MethodID; STACKED STRPTR name; };
struct MUIP_Application_NewInput		{ STACKED ULONG MethodID; STACKED ULONG *signal; };
struct MUIP_Application_OpenConfigWindow	{ STACKED ULONG MethodID; STACKED ULONG flags; };
struct MUIP_Application_PushMethod		{ STACKED ULONG MethodID; STACKED Object *dest; STACKED LONG count; /* more elements may follow */ };
struct MUIP_Application_RemInputHandler	{ STACKED ULONG MethodID; STACKED struct MUI_InputHandlerNode *ihnode; };
struct MUIP_Application_ReturnID		{ STACKED ULONG MethodID; STACKED ULONG retid; };
struct MUIP_Application_Save			{ STACKED ULONG MethodID; STACKED STRPTR name; };
struct MUIP_Application_SetConfigItem		{ STACKED ULONG MethodID; STACKED ULONG item; STACKED APTR data; };
struct MUIP_Application_SetMenuCheck		{ STACKED ULONG MethodID; STACKED ULONG MenuID; STACKED LONG stat; };
struct MUIP_Application_SetMenuState		{ STACKED ULONG MethodID; STACKED ULONG MenuID; STACKED LONG stat; };
struct MUIP_Application_ShowHelp		{ STACKED ULONG MethodID; STACKED Object *window; STACKED char *name; STACKED char *node; STACKED LONG line; };
struct MUIP_Application_SetConfigdata		{ STACKED ULONG MethodID; STACKED APTR configdata; };
struct MUIP_Application_OpenWindows		{ STACKED ULONG MethodID; };
struct MUIP_Application_UpdateMenus             { STACKED ULONG MethodID; };

/*** Attributes *************************************************************/
#define MUIA_Application_Active             	(MUIB_MUI|0x004260ab) /* MUI: V4  isg BOOL              */
#define MUIA_Application_Author             	(MUIB_MUI|0x00424842) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_Base               	(MUIB_MUI|0x0042e07a) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_Broker             	(MUIB_MUI|0x0042dbce) /* MUI: V4  ..g Broker *          */
#define MUIA_Application_BrokerHook         	(MUIB_MUI|0x00428f4b) /* MUI: V4  isg struct Hook *     */
#define MUIA_Application_BrokerPort         	(MUIB_MUI|0x0042e0ad) /* MUI: V6  ..g struct MsgPort *  */
#define MUIA_Application_BrokerPri          	(MUIB_MUI|0x0042c8d0) /* MUI: V6  i.g LONG              */
#define MUIA_Application_Commands           	(MUIB_MUI|0x00428648) /* MUI: V4  isg struct MUI_Command * */
#define MUIA_Application_Copyright          	(MUIB_MUI|0x0042ef4d) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_Description        	(MUIB_MUI|0x00421fc6) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_DiskObject         	(MUIB_MUI|0x004235cb) /* MUI: V4  isg struct DiskObject * */
#define MUIA_Application_DoubleStart        	(MUIB_MUI|0x00423bc6) /* MUI: V4  ..g BOOL              */
#define MUIA_Application_DropObject         	(MUIB_MUI|0x00421266) /* MUI: V5  is. Object *          */
#define MUIA_Application_ForceQuit          	(MUIB_MUI|0x004257df) /* MUI: V8  ..g BOOL              */
#define MUIA_Application_HelpFile           	(MUIB_MUI|0x004293f4) /* MUI: V8  isg STRPTR            */
#define MUIA_Application_Iconified          	(MUIB_MUI|0x0042a07f) /* MUI: V4  .sg BOOL              */
#define MUIA_Application_MenuAction         	(MUIB_MUI|0x00428961) /* MUI: V4  ..g ULONG             */
#define MUIA_Application_MenuHelp           	(MUIB_MUI|0x0042540b) /* MUI: V4  ..g ULONG             */
#define MUIA_Application_Menustrip          	(MUIB_MUI|0x004252d9) /* MUI: V8  i.. Object *          */
#define MUIA_Application_RexxHook           	(MUIB_MUI|0x00427c42) /* MUI: V7  isg struct Hook *     */
#define MUIA_Application_RexxMsg            	(MUIB_MUI|0x0042fd88) /* MUI: V4  ..g struct RxMsg *    */
#define MUIA_Application_RexxString         	(MUIB_MUI|0x0042d711) /* MUI: V4  .s. STRPTR            */
#define MUIA_Application_SingleTask         	(MUIB_MUI|0x0042a2c8) /* MUI: V4  i.. BOOL              */
#define MUIA_Application_Sleep              	(MUIB_MUI|0x00425711) /* MUI: V4  .s. BOOL              */
#define MUIA_Application_Title              	(MUIB_MUI|0x004281b8) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_UseCommodities     	(MUIB_MUI|0x00425ee5) /* MUI: V10 i.. BOOL              */
#define MUIA_Application_UsedClasses            (MUIB_MUI|0x0042e9a7) /* MUI undoc: V20 i.. STRPTR [] */
#define MUIA_Application_UseRexx            	(MUIB_MUI|0x00422387) /* MUI: V10 i.. BOOL              */
#define MUIA_Application_SetWinPos              (MUIB_MUI|0x00432387)
#define MUIA_Application_GetWinPos              (MUIB_MUI|0x00432388)
#define MUIA_Application_SearchWinId            (MUIB_MUI|0x00432389)
#define MUIA_Application_GetWinPosAddr          (MUIB_MUI|0x00432390)
#define MUIA_Application_GetWinPosSize          (MUIB_MUI|0x00432391)
#define MUIA_Application_CopyWinPosToApp        (MUIB_MUI|0x00432392)
#define MAXWINS 300

struct windowpos
{ 
ULONG id;
WORD x1,y1,w1,h1;
WORD x2,y2,w2,h2;
};

/*+
    [I-G] CONST_STRPTR
    Standard DOS version string. Example: "$VER: Program 1.3 (14.11.03)".
    Zune extension: If unspecified or NULL, it will be automatically 
    constructed from MUIA_Application_Title, MUIA_Application_Version_Number,
    MUIA_Application_Version_Date and MUIA_Application_Version_Extra as 
    follows: "$VER: <title> <version> (<date>) [<extra>]".
+*/
#define MUIA_Application_Version            	(MUIB_MUI|0x0042b33f)

#define MUIA_Application_Window             	(MUIB_MUI|0x0042bfe0) /* MUI: V4  i.. Object *          */
#define MUIA_Application_WindowList         	(MUIB_MUI|0x00429abe) /* MUI: V13 ..g struct List *     */

#define MUIA_Application_Configdata         	(MUIB_Application | 0x00000000) /* Zune 20030407 .s. Object *     */

/*+
    [I-G] CONST_STRPTR
    Version number. Examples: "1.5", "2.37.4b".
+*/
#define MUIA_Application_Version_Number         (MUIB_Application | 0x00000001)

/*+
    [I-G] CONST_STRPTR
    Date information on the standard international YYYY-MM-DD format. 
+*/
#define MUIA_Application_Version_Date           (MUIB_Application | 0x00000002)

/*+
    [I-G] CONST_STRPTR
    Arbitrary extra version information. Example: "nightly build".
+*/
#define MUIA_Application_Version_Extra          (MUIB_Application | 0x00000003)


/* MUI Obsolette tags */
#ifdef MUI_OBSOLETE
#define MUIA_Application_Menu							  (MUIB_MUI|0x00420e1f) /* MUI: V4  i.g struct NewMenu *  */
#endif /* MUI_OBSOLETE */

/**************************************************************************
 Structure used ba MUIM_Application_AddInputHandler/RemInputHandler
**************************************************************************/
struct MUI_InputHandlerNode
{
    struct MinNode ihn_Node;
    Object *ihn_Object;
    union
    {
	ULONG ihn_sigs;
	struct
	{
	    UWORD ihn_millis;
	    UWORD ihn_current;
	} ihn_timer;
    }
    ihn_stuff;
    ULONG ihn_Flags;
    ULONG ihn_Method;
};

/* Easier access to the members */
#define ihn_Millis   ihn_stuff.ihn_timer.ihn_millis
#define ihn_Current  ihn_stuff.ihn_timer.ihn_current
#define ihn_Signals  ihn_stuff.ihn_sigs

/* Flags for ihn_Flags */
#define MUIIHNF_TIMER (1<<0) /* you want to be called every ihn_Millis msecs */

/**************************************************************************
 Special values for the name field of MUIM_Application_Load/Save 
**************************************************************************/
#define MUIV_Application_Save_ENV    ((STRPTR) 0)
#define MUIV_Application_Save_ENVARC ((STRPTR)~0)
#define MUIV_Application_Load_ENV    ((STRPTR) 0)
#define MUIV_Application_Load_ENVARC ((STRPTR)~0)


/**************************************************************************
 Special Values MUIM_Application_ReturnID. Usally programm should leave
 the event loop if this is set
**************************************************************************/
#define MUIV_Application_ReturnID_Quit ((ULONG)-1)






struct MUI_GlobalInfo
{
    ULONG priv0;
    Object *mgi_ApplicationObject;

    /* The following data is private only, might be extented! */
};


#endif /* _MUI_CLASSES_APPLICATION_H */
#endif


/**************************************************************************
 Here are the possible Objecttypes for MUI_MakeObject()
**************************************************************************/
enum
{
    MUIO_Label = 1,    /* STRPTR label, ULONG flags */
    MUIO_Button,       /* STRPTR label */
    MUIO_Checkmark,    /* STRPTR label */
    MUIO_Cycle,        /* STRPTR label, STRPTR *entries */
    MUIO_Radio,        /* STRPTR label, STRPTR *entries */
    MUIO_Slider,       /* STRPTR label, LONG min, LONG max */
    MUIO_String,       /* STRPTR label, LONG maxlen */
    MUIO_PopButton,    /* STRPTR imagespec */
    MUIO_HSpace,       /* LONG space */
    MUIO_VSpace,       /* LONG space */
    MUIO_HBar,         /* LONG space */
    MUIO_VBar,         /* LONG space */
    MUIO_MenustripNM,  /* struct NewMenu *nm, ULONG flags */
    MUIO_Menuitem,     /* STRPTR label, STRPTR shortcut, ULONG flags, ULONG data  */
    MUIO_BarTitle,     /* STRPTR label */
    MUIO_NumericButton,/* STRPTR label, LONG min, LONG max, STRPTR format */
    
    MUIO_CoolButton = 111, /* STRPTR label, APTR CoolImage, ULONG flags */
    MUIO_ImageButton,      /* CONST_STRPTR label, CONST_STRPTR imagePath */
};

/* flag for MUIO_Menuitem */
#define MUIO_Menuitem_CopyStrings (1<<30)

/* flags for MUIO_Label type */
#define MUIO_Label_SingleFrame   (1<< 8)
#define MUIO_Label_DoubleFrame   (1<< 9)
#define MUIO_Label_LeftAligned   (1<<10)
#define MUIO_Label_Centered      (1<<11)
#define MUIO_Label_FreeVert      (1<<12)

/* flag for MUIO_MenustripNM */
#define MUIO_MenustripNM_CommandKeyCheck (1<<0) /* check for "localized" menu items such as "O\0Open" */

/* flag for MUI_CoolButton  */
#define MUIO_CoolButton_CoolImageID (1<<0)

struct MUI_MinMax
{
    WORD MinWidth;
    WORD MinHeight;
    WORD MaxWidth;
    WORD MaxHeight;
    WORD DefWidth;
    WORD DefHeight;
};

/* special maximum dimension in case it is unlimited */
#define MUI_MAXMAX 10000 

/* Number of pens, the single definintion is below */
#define MPEN_COUNT 8

/* The mask for pens from MUI_ObtainPen() and a macro */
#define MUIPEN_MASK 0x0000ffff
#define MUIPEN(pen) ((pen) & MUIPEN_MASK)

/* These cannot be enums, since they will
 * not be passed properly in varadic
 * functions by some compilers.
 */
#define MUIV_Font_Inherit  ((IPTR)0)
#define MUIV_Font_Normal   ((IPTR)-1)
#define MUIV_Font_List     ((IPTR)-2)
#define MUIV_Font_Tiny     ((IPTR)-3)
#define MUIV_Font_Fixed    ((IPTR)-4)
#define MUIV_Font_Title    ((IPTR)-5)
#define MUIV_Font_Big      ((IPTR)-6)
#define MUIV_Font_Button   ((IPTR)-7)
#define MUIV_Font_Knob     ((IPTR)-8)
#define MUIV_Font_NegCount ((IPTR)-9)

/* Possible keyevents (user configurable) */
enum
{
    MUIKEY_RELEASE      = -2, /* this one is faked only, and thereforce not configurable */
    MUIKEY_NONE         = -1,
    MUIKEY_PRESS        = 0,
    MUIKEY_TOGGLE       = 1,
    MUIKEY_UP           = 2,
    MUIKEY_DOWN         = 3,
    MUIKEY_PAGEUP       = 4,
    MUIKEY_PAGEDOWN     = 5,
    MUIKEY_TOP          = 6,
    MUIKEY_BOTTOM       = 7,
    MUIKEY_LEFT         = 8,
    MUIKEY_RIGHT        = 9,
    MUIKEY_WORDLEFT     = 10,
    MUIKEY_WORDRIGHT    = 11,
    MUIKEY_LINESTART    = 12,
    MUIKEY_LINEEND      = 13,
    MUIKEY_GADGET_NEXT  = 14,
    MUIKEY_GADGET_PREV  = 15,
    MUIKEY_GADGET_OFF   = 16,
    MUIKEY_WINDOW_CLOSE = 17,
    MUIKEY_WINDOW_NEXT  = 18,
    MUIKEY_WINDOW_PREV  = 19,
    MUIKEY_HELP         = 20,
    MUIKEY_POPUP        = 21,
    MUIKEY_COUNT        = 22
};

/* The mask definitions of the above keys */
#define MUIKEYF_PRESS        (1<<MUIKEY_PRESS)
#define MUIKEYF_TOGGLE       (1<<MUIKEY_TOGGLE)
#define MUIKEYF_UP           (1<<MUIKEY_UP)
#define MUIKEYF_DOWN         (1<<MUIKEY_DOWN)
#define MUIKEYF_PAGEUP       (1<<MUIKEY_PAGEUP)
#define MUIKEYF_PAGEDOWN     (1<<MUIKEY_PAGEDOWN)
#define MUIKEYF_TOP          (1<<MUIKEY_TOP)
#define MUIKEYF_BOTTOM       (1<<MUIKEY_BOTTOM)
#define MUIKEYF_LEFT         (1<<MUIKEY_LEFT)
#define MUIKEYF_RIGHT        (1<<MUIKEY_RIGHT)
#define MUIKEYF_WORDLEFT     (1<<MUIKEY_WORDLEFT)
#define MUIKEYF_WORDRIGHT    (1<<MUIKEY_WORDRIGHT)
#define MUIKEYF_LINESTART    (1<<MUIKEY_LINESTART)
#define MUIKEYF_LINEEND      (1<<MUIKEY_LINEEND)
#define MUIKEYF_GADGET_NEXT  (1<<MUIKEY_GADGET_NEXT)
#define MUIKEYF_GADGET_PREV  (1<<MUIKEY_GADGET_PREV)
#define MUIKEYF_GADGET_OFF   (1<<MUIKEY_GADGET_OFF)
#define MUIKEYF_WINDOW_CLOSE (1<<MUIKEY_WINDOW_CLOSE)
#define MUIKEYF_WINDOW_NEXT  (1<<MUIKEY_WINDOW_NEXT)
#define MUIKEYF_WINDOW_PREV  (1<<MUIKEY_WINDOW_PREV)
#define MUIKEYF_HELP         (1<<MUIKEY_HELP)
#define MUIKEYF_POPUP        (1<<MUIKEY_POPUP)

struct MUI_CustomClass
{
    APTR mcc_UserData;                  /* freely usable */

    /* Zune/MUI had the following libraries opened for you */
    struct Library *mcc_UtilityBase;
    struct Library *mcc_DOSBase;
    struct Library *mcc_GfxBase;
    struct Library *mcc_IntuitionBase;

    struct IClass *mcc_Super;           /* the boopsi class' superclass */
    struct IClass *mcc_Class;           /* the boopsi class */

    /* the following stuff is private */

    struct Library *mcc_Module;         /* non-null if external class */
};

#undef MPEN_COUNT

typedef enum {
    MPEN_SHINE      = 0,
    MPEN_HALFSHINE  = 1,
    MPEN_BACKGROUND = 2,
    MPEN_HALFSHADOW = 3,
    MPEN_SHADOW     = 4,
    MPEN_TEXT       = 5,
    MPEN_FILL       = 6,
    MPEN_MARK       = 7,
    MPEN_COUNT      = 8,
} MPen;

typedef enum {
    PST_MUI = 'm',
    PST_CMAP = 'p',
    PST_RGB = 'r',
    PST_SYS = 's',
} PenSpecType;

/* MUI_PenSpec is a an ascii spec like this:

   "m5"     	    	    	(mui pen #5)
   "p123"   	    	    	(cmap entry #123)
   "rFFFFFFFF,00000000,00000000 (rgb #FF0000)
   "s3"                         (system pen #3)
   
   It needs to be like this, because for example nlist has
   default penspecs in it's source encoded like above which
   it directly passes to MUI_ObtainBestPen */
   
struct MUI_PenSpec
{
    UBYTE ps_buf[32];
};


struct MUI_FrameSpec
{
    char buf[32];
};

struct MUI_RGBcolor
{
    ULONG red;
    ULONG green;
    ULONG blue;
};

#ifndef _MUI_CLASSES_NOTIFY_H
#endif

#ifndef _MUI_CLASSES_WINDOW_H
#ifndef _MUI_CLASSES_WINDOW_H
#define _MUI_CLASSES_WINDOW_H

/*
    Copyright  1999, David Le Corfec.
    Copyright  2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Window                 "Window.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Window                 (MUIB_ZUNE | 0x00003600)

/*** Methods ****************************************************************/
#define MUIM_Window_ActionIconify   (MUIB_MUI|0x00422cc0) /* MUI: V18 undoc*/
#define MUIM_Window_AddEventHandler (MUIB_MUI|0x004203b7) /* MUI: V16 */
#define MUIM_Window_Cleanup         (MUIB_MUI|0x0042ab26) /* MUI: V18 undoc */ /* For custom classes only */
#define MUIM_Window_RemEventHandler (MUIB_MUI|0x0042679e) /* MUI: V16 */
#define MUIM_Window_ScreenToBack    (MUIB_MUI|0x0042913d) /* MUI: V4  */
#define MUIM_Window_ScreenToFront   (MUIB_MUI|0x004227a4) /* MUI: V4  */
#define MUIM_Window_Setup           (MUIB_MUI|0x0042c34c) /* MUI: V18 undoc */ /* For custom Classes only */
#define MUIM_Window_Snapshot        (MUIB_MUI|0x0042945e) /* MUI: V11 */
#define MUIM_Window_ToBack          (MUIB_MUI|0x0042152e) /* MUI: V4  */
#define MUIM_Window_ToFront         (MUIB_MUI|0x0042554f) /* MUI: V4  */
struct  MUIP_Window_ActionIconify   {STACKED ULONG MethodID;};
struct  MUIP_Window_AddEventHandler {STACKED ULONG MethodID; STACKED struct MUI_EventHandlerNode *ehnode;};
struct  MUIP_Window_Cleanup         {STACKED ULONG MethodID;};
struct  MUIP_Window_RemEventHandler {STACKED ULONG MethodID; STACKED struct MUI_EventHandlerNode *ehnode;};
struct  MUIP_Window_ScreenToBack    {STACKED ULONG MethodID;};
struct  MUIP_Window_ScreenToFront   {STACKED ULONG MethodID;};
struct  MUIP_Window_Setup           {STACKED ULONG MethodID;};
struct  MUIP_Window_Snapshot        {STACKED ULONG MethodID; STACKED LONG flags;};
struct  MUIP_Window_ToBack          {STACKED ULONG MethodID;};
struct  MUIP_Window_ToFront         {STACKED ULONG MethodID;};

#define MUIM_Window_AllocGadgetID          (MUIB_Window | 0x00000001) /* Zune: V1 - allocate a GadgetID for BOOPSI gadgets */
#define MUIM_Window_FreeGadgetID           (MUIB_Window | 0x00000004) /* Zune: V1 - free the GadgetID for BOOPSI gadgets */
struct  MUIP_Window_AddControlCharHandler  { STACKED ULONG MethodID; STACKED struct MUI_EventHandlerNode *ccnode; };
struct  MUIP_Window_AllocGadgetID          { STACKED ULONG MethodID; }; /* Custom Class - returns the Gadget ID */
struct  MUIP_Window_DrawBackground         { STACKED ULONG MethodID; STACKED LONG left; STACKED LONG top; STACKED LONG width; STACKED LONG height; STACKED LONG xoffset; STACKED LONG yoffset; STACKED LONG flags;};
struct  MUIP_Window_DragObject             { STACKED ULONG MethodID; STACKED Object *obj; STACKED LONG touchx; STACKED LONG touchy; STACKED ULONG flags; };
struct  MUIP_Window_FreeGadgetID           { STACKED ULONG MethodID; STACKED LONG gadgetid; }; /* Custom Class */
struct  MUIP_Window_RecalcDisplay          { STACKED ULONG MethodID; STACKED Object *originator; };
struct  MUIP_Window_RemControlCharHandler  { STACKED ULONG MethodID; STACKED struct MUI_EventHandlerNode *ccnode; };
struct  MUIP_Window_UpdateMenu             { STACKED ULONG MethodID; };

#ifdef MUI_OBSOLETE
#define MUIM_Window_GetMenuCheck    (MUIB_MUI|0x00420414) /* MUI: V4  */
#define MUIM_Window_GetMenuState    (MUIB_MUI|0x00420d2f) /* MUI: V4  */
#define MUIM_Window_SetCycleChain   (MUIB_MUI|0x00426510) /* MUI: V4  */
#define MUIM_Window_SetMenuCheck    (MUIB_MUI|0x00422243) /* MUI: V4  */
#define MUIM_Window_SetMenuState    (MUIB_MUI|0x00422b5e) /* MUI: V4  */
struct  MUIP_Window_GetMenuCheck    {STACKULONG MethodID; STACKED ULONG MenuID;};
struct  MUIP_Window_GetMenuState    {STACKULONG MethodID; STACKED ULONG MenuID;};
struct  MUIP_Window_SetCycleChain   {STACKULONG MethodID; STACKED Object *obj[1];};
struct  MUIP_Window_SetMenuCheck    {STACKULONG MethodID; STACKED ULONG MenuID; STACKED LONG stat;};
struct  MUIP_Window_SetMenuState    {STACKULONG MethodID; STACKED ULONG MenuID; STACKED LONG stat;};
#endif /* MUI_OBSOLETE */

/*** Attributes *************************************************************/
#define MUIA_Window_Activate                (MUIB_MUI|0x00428d2f) /* MUI: V4  isg BOOL                */
#define MUIA_Window_ActiveObject            (MUIB_MUI|0x00427925) /* MUI: V4  .sg Object *            */
#define MUIA_Window_AltHeight               (MUIB_MUI|0x0042cce3) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AltLeftEdge             (MUIB_MUI|0x00422d65) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AltTopEdge              (MUIB_MUI|0x0042e99b) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AltWidth                (MUIB_MUI|0x004260f4) /* MUI: V4  i.g LONG                */
#define MUIA_Window_AppWindow               (MUIB_MUI|0x004280cf) /* MUI: V5  i.. BOOL                */
#define MUIA_Window_Backdrop                (MUIB_MUI|0x0042c0bb) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_Borderless              (MUIB_MUI|0x00429b79) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_CloseGadget             (MUIB_MUI|0x0042a110) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_CloseRequest            (MUIB_MUI|0x0042e86e) /* MUI: V4  ..g BOOL                */
#define MUIA_Window_DefaultObject           (MUIB_MUI|0x004294d7) /* MUI: V4  isg Object *            */
#define MUIA_Window_DepthGadget             (MUIB_MUI|0x00421923) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_DisableKeys             (MUIB_MUI|0x00424c36) /* MUI: V15 isg ULONG               */ /* undoc */
#define MUIA_Window_DragBar                 (MUIB_MUI|0x0042045d) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_FancyDrawing            (MUIB_MUI|0x0042bd0e) /* MUI: V8  isg BOOL                */
#define MUIA_Window_Height                  (MUIB_MUI|0x00425846) /* MUI: V4  i.g LONG                */
#define MUIA_Window_ID                      (MUIB_MUI|0x004201bd) /* MUI: V4  isg ULONG               */
#define MUIA_Window_InputEvent              (MUIB_MUI|0x004247d8) /* MUI: V4  ..g struct InputEvent * */
#define MUIA_Window_IsSubWindow             (MUIB_MUI|0x0042b5aa) /* MUI: V4  isg BOOL                */
#define MUIA_Window_LeftEdge                (MUIB_MUI|0x00426c65) /* MUI: V4  i.g LONG                */
#define MUIA_Window_MenuAction              (MUIB_MUI|0x00427521) /* MUI: V8  isg ULONG               */
#define MUIA_Window_Menustrip               (MUIB_MUI|0x0042855e) /* MUI: V8  i.g Object *            */
#define MUIA_Window_MouseObject             (MUIB_MUI|0x0042bf9b) /* MUI: V10 ..g Object *            */
#define MUIA_Window_NeedsMouseObject        (MUIB_MUI|0x0042372a) /* MUI: V10 i.. BOOL                */
#define MUIA_Window_NoMenus                 (MUIB_MUI|0x00429df5) /* MUI: V4  is. BOOL                */
#define MUIA_Window_Open                    (MUIB_MUI|0x00428aa0) /* MUI: V4  .sg BOOL                */
#define MUIA_Window_PublicScreen            (MUIB_MUI|0x004278e4) /* MUI: V6  isg STRPTR              */
#define MUIA_Window_RefWindow               (MUIB_MUI|0x004201f4) /* MUI: V4  is. Object *            */
#define MUIA_Window_RootObject              (MUIB_MUI|0x0042cba5) /* MUI: V4  isg Object *            */
#define MUIA_Window_Screen                  (MUIB_MUI|0x0042df4f) /* MUI: V4  isg struct Screen *     */
#define MUIA_Window_ScreenTitle             (MUIB_MUI|0x004234b0) /* MUI: V5  isg STRPTR              */
#define MUIA_Window_SizeGadget              (MUIB_MUI|0x0042e33d) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_SizeRight               (MUIB_MUI|0x00424780) /* MUI: V4  i.. BOOL                */
#define MUIA_Window_Sleep                   (MUIB_MUI|0x0042e7db) /* MUI: V4  .sg BOOL                */
#define MUIA_Window_Title                   (MUIB_MUI|0x0042ad3d) /* MUI: V4  isg STRPTR              */
#define MUIA_Window_TopEdge                 (MUIB_MUI|0x00427c66) /* MUI: V4  i.g LONG                */
#define MUIA_Window_UseBottomBorderScroller (MUIB_MUI|0x00424e79) /* MUI: V13 isg BOOL                */
#define MUIA_Window_UseLeftBorderScroller   (MUIB_MUI|0x0042433e) /* MUI: V13 isg BOOL                */
#define MUIA_Window_UseRightBorderScroller  (MUIB_MUI|0x0042c05e) /* MUI: V13 isg BOOL                */
#define MUIA_Window_Width                   (MUIB_MUI|0x0042dcae) /* MUI: V4  i.g LONG                */
#define MUIA_Window_Window                  (MUIB_MUI|0x00426a42) /* MUI: V4  ..g struct Window *     */

#define MUIA_Window_EraseArea               (MUIB_Window | 0x00000000) /* Zune only i.. BOOL (default: TRUE) */
#define MUIA_Window_ZoomGadget              (MUIB_Window | 0x00000002)
#define MUIA_Window_ToolBox                 (MUIB_Window | 0x00000003)

#define MUIV_Window_ActiveObject_None       0
#define MUIV_Window_ActiveObject_Next       (-1)
#define MUIV_Window_ActiveObject_Prev       (-2)
#define MUIV_Window_AltHeight_MinMax(p)     (0-(p))
#define MUIV_Window_AltHeight_Visible(p)    (-100-(p))
#define MUIV_Window_AltHeight_Screen(p)     (-200-(p))
#define MUIV_Window_AltHeight_Scaled        (-1000)
#define MUIV_Window_AltLeftEdge_Centered    (-1)
#define MUIV_Window_AltLeftEdge_Moused      (-2)
#define MUIV_Window_AltLeftEdge_NoChange    (-1000)
#define MUIV_Window_AltTopEdge_Centered     (-1)
#define MUIV_Window_AltTopEdge_Moused       (-2)
#define MUIV_Window_AltTopEdge_Delta(p)     (-3-(p))
#define MUIV_Window_AltTopEdge_NoChange     (-1000)
#define MUIV_Window_AltWidth_MinMax(p)      (0-(p))
#define MUIV_Window_AltWidth_Visible(p)     (-100-(p))
#define MUIV_Window_AltWidth_Screen(p)      (-200-(p))
#define MUIV_Window_AltWidth_Scaled         (-1000)
#define MUIV_Window_Height_MinMax(p)        (0-(p))
#define MUIV_Window_Height_Visible(p)       (-100-(p))
#define MUIV_Window_Height_Screen(p)        (-200-(p))
#define MUIV_Window_Height_Scaled           (-1000)
#define MUIV_Window_Height_Default          (-1001)
#define MUIV_Window_LeftEdge_Centered       (-1)
#define MUIV_Window_LeftEdge_Moused         (-2)
#define MUIV_Window_TopEdge_Centered        (-1)
#define MUIV_Window_TopEdge_Moused          (-2)
#define MUIV_Window_TopEdge_Delta(p)        (-3-(p))
#define MUIV_Window_Width_MinMax(p)         (0-(p))
#define MUIV_Window_Width_Visible(p)        (-100-(p))
#define MUIV_Window_Width_Screen(p)         (-200-(p))
#define MUIV_Window_Width_Scaled            (-1000)
#define MUIV_Window_Width_Default           (-1001)

#define MUIV_Window_Button_MUI              1
#define MUIV_Window_Button_Snapshot         2
#define MUIV_Window_Button_Iconify          4
#define MUIV_Window_Button_Popup            8


#ifdef MUI_OBSOLETE
#define MUIA_Window_Menu            (MUIB_MUI|0x0042db94) /* MUI: V4  i.. struct NewMenu * */
#define MUIV_Window_Menu_NoMenu     (-1)
#endif /* MUI_OBSOLETE */

/* Forward declaration for application opaque custom frame specification */
struct dt_frame_image;

/**************************************************************************
 Info about the display environment on which all Area Objects have a
 reference to it.
**************************************************************************/

#define MRI_RARRAY_SIZE 20

struct MUI_RenderInfo
{
    Object          *mri_WindowObject;  /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    struct Screen   *mri_Screen;        /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    struct DrawInfo *mri_DrawInfo;      /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    UWORD           *mri_Pens;          /* accessable inbetween MUIM_Setup/MUIM_Cleanup */
    struct Window   *mri_Window;        /* accessable inbetween MUIM_Show/MUIM_Hide */
    struct RastPort *mri_RastPort;      /* accessable inbetween MUIM_Show/MUIM_Hide */
    ULONG            mri_Flags;         /* accessable inbetween MUIM_Setup/MUIM_Cleanup */

    /* the following stuff is private */
    struct ColorMap *mri_Colormap;
    UWORD            mri_ScreenWidth;
    UWORD            mri_ScreenHeight;
    UWORD            mri_PensStorage[MPEN_COUNT]; /* storage for pens, mri_Pens point to here */

    struct TextFont *mri_Fonts[-MUIV_Font_NegCount]; /* Opened text fonts, done by zune_get_font() */

    /* this is for AddClipping/AddClipRegion */
    struct Region   *mri_rArray[MRI_RARRAY_SIZE];
    int              mri_rCount;

    struct Rectangle mri_ClipRect;

    UWORD            mri_BorderTop;     /* The height of the windows top border (the title) */
    UWORD            mri_BorderBottom;  /* The height of the windows bottom bodder */
    UWORD            mri_BorderLeft;    /* The width of the windows left border */
    UWORD            mri_BorderRight;   /* The width of the windows right border */

    /* Stuff for Borderscrollers */
    Object *mri_LeftImage; /* Valid between MUIM_Setup/MUIM_Cleanup */
    Object *mri_RightImage;
    Object *mri_UpImage;
    Object *mri_DownImage;
    Object *mri_SizeImage;

    Object *mri_VertProp; /* Valid between MUIM_Show/MUIM_Hide */
    Object *mri_HorizProp;

    /* buffering */
    struct RastPort mri_BufferRP;
    struct BitMap  *mri_BufferBM;

    struct dt_frame_image *mri_FrameImage[16];
};

#define MUIMRI_RECTFILL (1<<0)
#define MUIMRI_TRUECOLOR (1<<1)
#define MUIMRI_THINFRAMES (1<<2)
#define MUIMRI_REFRESHMODE (1<<3)

/**************************************************************************
 MUI_EventHandlerNode as used by
 MUIM_Window_AddEventHandler/RemoveEventHandler
**************************************************************************/

struct MUI_EventHandlerNode
{
    struct MinNode ehn_Node;     /* embedded node structure, private! */
    BYTE           ehn_Reserved; /* private! */
    BYTE           ehn_Priority; /* sorted by priority. */
    UWORD          ehn_Flags;    /* some flags, see below */
    Object        *ehn_Object;   /* object which should receive MUIM_HandleEvent. */
    struct IClass *ehn_Class;    /* Class for CoerceMethod(). If NULL DoMethod() is used */
    ULONG          ehn_Events;   /* the IDCMP flags the handler should be invoked. */
};

/* here are the flags for ehn_Flags */
#define MUI_EHF_ALWAYSKEYS  (1<<0)
#define MUI_EHF_GUIMODE     (1<<1)  /* handler will not be called if object is not visible or disabled */

/* MUIM_HandleEvent must return a bitmask where following bit's can be set (all other must be 0) */
#define MUI_EventHandlerRC_Eat (1<<0) /* do not invoke more handlers ers */



#endif /* _MUI_CLASSES_WINDOW_H */
#endif

#ifndef _MUI_CLASSES_AREA_H
/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002 - 2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_AREA_H
#define _MUI_CLASSES_AREA_H

#ifndef INTUITION_INTUITION_H
#include <intuition/intuition.h>
#endif

#ifndef GRAPHICS_GRAPHICS_H
#include <graphics/gfx.h>
#endif

#ifndef _MUI_CLASSES_WINDOW_H
#endif

/*** Name *******************************************************************/
#define MUIC_Area                   "Area.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Area                   (MUIB_ZUNE | 0x00000200)

/*** Methods ****************************************************************/
#define MUIM_AskMinMax              (MUIB_MUI|0x00423874) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Cleanup                (MUIB_MUI|0x0042d985) /* MUI: V4  */ /* For Custom Classes only */
#define MUIM_ContextMenuBuild       (MUIB_MUI|0x00429d2e) /* MUI: V11 */
#define MUIM_ContextMenuChoice      (MUIB_MUI|0x00420f0e) /* MUI: V11 */
#define MUIM_CreateBubble	    (MUIB_MUI|0x00421c41) /* MUI: V18 */
#define MUIM_CreateDragImage	    (MUIB_MUI|0x0042eb6f) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_CreateShortHelp        (MUIB_MUI|0x00428e93) /* MUI: V11 */
#define MUIM_CustomBackfill	    (MUIB_MUI|0x00428d73) /* Undoc */
#define MUIM_DeleteBubble	    (MUIB_MUI|0x004211af) /* MUI: V18 */
#define MUIM_DeleteDragImage	    (MUIB_MUI|0x00423037) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_DeleteShortHelp        (MUIB_MUI|0x0042d35a) /* MUI: V11 */
#define MUIM_DoDrag		    (MUIB_MUI|0x004216bb) /* MUI: V18 */ /* For Custom Classes only */ /* Undoc */
#define MUIM_DragBegin	            (MUIB_MUI|0x0042c03a) /* MUI: V11 */
#define MUIM_DragDrop	            (MUIB_MUI|0x0042c555) /* MUI: V11 */
#define MUIM_UnknownDropDestination (MUIB_MUI|0x00425550) /* ZUNE */
#define MUIM_DragFinish	            (MUIB_MUI|0x004251f0) /* MUI: V11 */
#define MUIM_DragQuery	            (MUIB_MUI|0x00420261) /* MUI: V11 */
#define MUIM_DragReport	            (MUIB_MUI|0x0042edad) /* MUI: V11 */
#define MUIM_Draw		    (MUIB_MUI|0x00426f3f) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_DrawBackground	    (MUIB_MUI|0x004238ca) /* MUI: V11 */
#define MUIM_GoActive		    (MUIB_MUI|0x0042491a) /* Undoc */
#define MUIM_GoInactive	            (MUIB_MUI|0x00422c0c) /* Undoc */
#define MUIM_HandleEvent	    (MUIB_MUI|0x00426d66) /* MUI: V16 */ /* For Custom Classes only */ 
#define MUIM_HandleInput	    (MUIB_MUI|0x00422a1a) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Hide		    (MUIB_MUI|0x0042f20f) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Setup		    (MUIB_MUI|0x00428354) /* MUI: V4  */ /* For Custom Classes only */ 
#define MUIM_Show		    (MUIB_MUI|0x0042cc84) /* MUI: V4  */ /* For Custom Classes only */ 
struct MUIP_AskMinMax		    {STACKED ULONG MethodID; STACKED struct MUI_MinMax *MinMaxInfo;};
struct MUIP_Cleanup		    {STACKED ULONG MethodID;};
struct MUIP_ContextMenuBuild	    {STACKED ULONG MethodID; STACKED LONG mx; STACKED LONG my;};
struct MUIP_ContextMenuChoice	    {STACKED ULONG MethodID; STACKED Object *item;};
struct MUIP_CreateBubble	    {STACKED ULONG MethodID; STACKED LONG x; STACKED LONG y; STACKED char *txt; STACKED ULONG flags;};
struct MUIP_CreateDragImage	    {STACKED ULONG MethodID; STACKED LONG touchx; STACKED LONG touchy; STACKED ULONG flags;};
struct MUIP_CreateShortHelp	    {STACKED ULONG MethodID; STACKED LONG mx; STACKED LONG my;};
struct MUIP_CustomBackfill 	    {STACKED ULONG MethodID; STACKED LONG left; STACKED LONG top; STACKED LONG right; STACKED LONG bottom; STACKED LONG xoffset; STACKED LONG yoffset;};
struct MUIP_DeleteBubble	    {STACKED ULONG MethodID; STACKED APTR bubble;};
struct MUIP_DeleteDragImage	    {STACKED ULONG MethodID; STACKED struct MUI_DragImage *di;};
struct MUIP_DeleteShortHelp	    {STACKED ULONG MethodID; STACKED STRPTR help; };
struct MUIP_DoDrag          	    {STACKED ULONG MethodID; STACKED LONG touchx; STACKED LONG touchy; STACKED ULONG flags;};
struct MUIP_UnknownDropDestination  {STACKED ULONG MethodID; STACKED struct IntuiMessage *imsg; };
struct MUIP_DragBegin               {STACKED ULONG MethodID; STACKED Object *obj;};
struct MUIP_DragDrop                {STACKED ULONG MethodID; STACKED Object *obj; STACKED LONG x; STACKED LONG y;};
struct MUIP_DragFinish              {STACKED ULONG MethodID; STACKED Object *obj;};
struct MUIP_DragQuery               {STACKED ULONG MethodID; STACKED Object *obj;};
struct MUIP_DragReport              {STACKED ULONG MethodID; STACKED Object *obj; STACKED LONG x; STACKED LONG y; STACKED LONG update;};
struct MUIP_Draw                    {STACKED ULONG MethodID; STACKED ULONG flags;};
struct MUIP_DrawBackground          {STACKED ULONG MethodID; STACKED LONG left; STACKED LONG top; STACKED LONG width; STACKED LONG height; STACKED LONG xoffset; STACKED LONG yoffset; STACKED LONG flags;};
struct MUIP_DrawBackgroundBuffered  {STACKED ULONG MethodID; STACKED struct RastPort *rp; STACKED LONG left; STACKED LONG top; STACKED LONG width; STACKED LONG height; STACKED LONG xoffset; STACKED LONG yoffset; STACKED LONG flags;};
struct MUIP_GoActive                {STACKED ULONG MethodID;};
struct MUIP_GoInacrive              {STACKED ULONG MethodID;};
struct MUIP_HandleEvent             {STACKED ULONG MethodID; STACKED struct IntuiMessage *imsg; STACKED LONG muikey;};
struct MUIP_HandleInput             {STACKED ULONG MethodID; STACKED struct IntuiMessage *imsg; STACKED LONG muikey;};
struct MUIP_Hide                    {STACKED ULONG MethodID;};
struct MUIP_Setup                   {STACKED ULONG MethodID; STACKED struct MUI_RenderInfo *RenderInfo;};
struct MUIP_Show                    {STACKED ULONG MethodID;};

#define MUIM_Layout                 (MUIB_Area | 0x00000000)
#define MUIM_DrawParentBackground   (MUIB_Area | 0x00000001)
struct  MUIP_Layout                 {STACKED ULONG MethodID;};
struct  MUIP_DrawParentBackground   {STACKED ULONG MethodID; STACKED LONG left; STACKED LONG top; STACKED LONG width; STACKED LONG height; STACKED LONG xoffset; STACKED LONG yoffset; STACKED LONG flags;};

struct MUI_DragImage
{
    struct BitMap *bm;
    WORD width;  /* exact width and height of bitmap */
    WORD height;
    WORD touchx; /* position of pointer click relative to bitmap */
    WORD touchy;
    ULONG flags;
};

// #define MUIF_DRAGIMAGE_HASMASK       (1<<0) /* Use provided mask for drawing */ /* Not supported at the moment */
#define MUIF_DRAGIMAGE_SOURCEALPHA   (1<<1) /* Use drag image source alpha information for transparrent drawing */

/*** Attributes *************************************************************/
#define MUIA_Background		(MUIB_MUI|0x0042545b) /* MUI: V4  is. LONG              */
#define MUIA_BottomEdge		(MUIB_MUI|0x0042e552) /* MUI: V4  ..g LONG              */
#define MUIA_ContextMenu		(MUIB_MUI|0x0042b704) /* MUI: V11 isg Object *          */
#define MUIA_ContextMenuTrigger	(MUIB_MUI|0x0042a2c1) /* MUI: V11 ..g Object *          */
#define MUIA_ControlChar        	(MUIB_MUI|0x0042120b) /* MUI: V4  isg char              */
#define MUIA_CustomBackfill		(MUIB_MUI|0x00420a63) /* undoc    i..                   */
#define MUIA_CycleChain         	(MUIB_MUI|0x00421ce7) /* MUI: V11 isg LONG              */
#define MUIA_Disabled           	(MUIB_MUI|0x00423661) /* MUI: V4  isg BOOL              */
#define MUIA_Draggable          	(MUIB_MUI|0x00420b6e) /* MUI: V11 isg BOOL              */
#define MUIA_Dropable           	(MUIB_MUI|0x0042fbce) /* MUI: V11 isg BOOL              */
#define MUIA_FillArea           	(MUIB_MUI|0x004294a3) /* MUI: V4  is. BOOL              */
#define MUIA_FixHeight          	(MUIB_MUI|0x0042a92b) /* MUI: V4  i.. LONG              */
#define MUIA_FixHeightTxt       	(MUIB_MUI|0x004276f2) /* MUI: V4  i.. STRPTR            */
#define MUIA_FixWidth           	(MUIB_MUI|0x0042a3f1) /* MUI: V4  i.. LONG              */
#define MUIA_FixWidthTxt        	(MUIB_MUI|0x0042d044) /* MUI: V4  i.. STRPTR            */
#define MUIA_Font               	(MUIB_MUI|0x0042be50) /* MUI: V4  i.g struct TextFont * */
#define MUIA_Frame              	(MUIB_MUI|0x0042ac64) /* MUI: V4  i.. LONG              */
#define MUIA_FramePhantomHoriz  	(MUIB_MUI|0x0042ed76) /* MUI: V4  i.. BOOL              */
#define MUIA_FrameTitle         	(MUIB_MUI|0x0042d1c7) /* MUI: V4  i.. STRPTR            */
#define MUIA_Height             	(MUIB_MUI|0x00423237) /* MUI: V4  ..g LONG              */
#define MUIA_HorizDisappear     	(MUIB_MUI|0x00429615) /* MUI: V11 isg LONG              */
#define MUIA_HorizWeight        	(MUIB_MUI|0x00426db9) /* MUI: V4  isg WORD              */
#define MUIA_InnerBottom        	(MUIB_MUI|0x0042f2c0) /* MUI: V4  i.g LONG              */
#define MUIA_InnerLeft          	(MUIB_MUI|0x004228f8) /* MUI: V4  i.g LONG              */
#define MUIA_InnerRight         	(MUIB_MUI|0x004297ff) /* MUI: V4  i.g LONG              */
#define MUIA_InnerTop           	(MUIB_MUI|0x00421eb6) /* MUI: V4  i.g LONG              */
#define MUIA_InputMode          	(MUIB_MUI|0x0042fb04) /* MUI: V4  i.. LONG              */
#define MUIA_LeftEdge           	(MUIB_MUI|0x0042bec6) /* MUI: V4  ..g LONG              */
#define MUIA_MaxHeight          	(MUIB_MUI|0x004293e4) /* MUI: V11 i.. LONG              */
#define MUIA_MaxWidth           	(MUIB_MUI|0x0042f112) /* MUI: V11 i.. LONG              */
#define MUIA_Pressed            	(MUIB_MUI|0x00423535) /* MUI: V4  ..g BOOL              */
#define MUIA_RightEdge          	(MUIB_MUI|0x0042ba82) /* MUI: V4  ..g LONG              */
#define MUIA_Selected           	(MUIB_MUI|0x0042654b) /* MUI: V4  isg BOOL              */
#define MUIA_ShortHelp          	(MUIB_MUI|0x00428fe3) /* MUI: V11 isg STRPTR            */
#define MUIA_ShowMe             	(MUIB_MUI|0x00429ba8) /* MUI: V4  isg BOOL              */
#define MUIA_ShowSelState       	(MUIB_MUI|0x0042caac) /* MUI: V4  i.. BOOL              */
#define MUIA_Timer              	(MUIB_MUI|0x00426435) /* MUI: V4  ..g LONG              */
#define MUIA_TopEdge            	(MUIB_MUI|0x0042509b) /* MUI: V4  ..g LONG              */
#define MUIA_VertDisappear      	(MUIB_MUI|0x0042d12f) /* MUI: V11 isg LONG              */
#define MUIA_VertWeight         	(MUIB_MUI|0x004298d0) /* MUI: V4  isg WORD              */
#define MUIA_Weight             	(MUIB_MUI|0x00421d1f) /* MUI: V4  i.. WORD              */
#define MUIA_Width              	(MUIB_MUI|0x0042b59c) /* MUI: V4  ..g LONG              */
#define MUIA_Window             	(MUIB_MUI|0x00421591) /* MUI: V4  ..g struct Window *   */
#define MUIA_WindowObject       	(MUIB_MUI|0x0042669e) /* MUI: V4  ..g Object *          */

#define MUIA_NestedDisabled             (MUIB_Area | 0x00000000) /* Zune 20030530  isg BOOL        */

#ifdef MUI_OBSOLETE	 		
#define MUIA_ExportID (MUIB_MUI|0x0042d76e) /* V4  isg ULONG */
#endif /* MUI_OBSOLETE */		

struct MUI_ImageSpec_intern;

struct MUI_AreaData
{
    struct MUI_RenderInfo *mad_RenderInfo; /* RenderInfo for this object */
    struct MUI_ImageSpec_intern *mad_Background;  /* bg setting - *private* ! */
    struct TextFont   *mad_Font;           /* Font which is used to draw */
    struct MUI_MinMax  mad_MinMax;         /* min/max/default dimensions */
    struct IBox        mad_Box;            /* coordinates and dim of this object after layouted */
    BYTE               mad_addleft;        /* left offset (frame & innerspacing) */
    BYTE               mad_addtop;         /* top offset (frame & innerspacing) */
    BYTE               mad_subwidth;       /* additional width (frame & innerspacing) */
    BYTE               mad_subheight;      /* additional height (frame & innerspacing) */
    ULONG              mad_Flags;          /* some flags; see below */
    ULONG              mad_Flags2;
// 40 bytes up to here

    /* The following data is private */
// offset 40
    UWORD              mad_HorizWeight;    /* weight values for layout. default 100 */
    UWORD              mad_VertWeight;
// offset 44
// ?
// offset 48
    ULONG              mad_IDCMP;          /* IDCMP flags this listens to (for HandleInput) */
// offset 52
    CONST_STRPTR       mad_BackgroundSpec;
// offset 56
    IPTR               mad_FontPreset;     /* MUIV_Font_xxx or pointer to struct TextFont */
// offset 76
    CONST_STRPTR       mad_FrameTitle;     /* for groups. Req. mad_Frame > 0 */
// Inner values at offset 88 in MUI:
    BYTE               mad_InnerLeft;      /* frame or hardcoded */
    BYTE               mad_InnerTop;
    BYTE               mad_InnerRight;
    BYTE               mad_InnerBottom;
// offset 94
    BYTE               mad_FrameOBSOLETE;          /* frame setting -- private */
// offset 95
    BYTE               mad_InputMode;      /* how to react to events */
// offset 96
    TEXT               mad_ControlChar;   /* key shortcut */
    BYTE               mad_TitleHeightAdd;/* frame title height = mad_TitleBelow + mad_TitleBaseline */
    BYTE               mad_TitleHeightBelow; /* height below frame */
    BYTE               mad_TitleHeightAbove; /* height above frame */
// 100
// ?
    IPTR               mad_Frame;
    WORD               mad_HardHeight;     /* if harcoded dim (see flags)  */
    WORD               mad_HardWidth;      /* if harcoded dim (see flags)  */
    CONST_STRPTR       mad_HardWidthTxt;
    CONST_STRPTR       mad_HardHeightTxt;
// TODO: move SelBack in RenderInfo as it's common for all objects
    struct MUI_ImageSpec_intern *mad_SelBack;     /* selected state background */
    CONST_STRPTR       mad_ShortHelp;      /* bubble help */
// there's an event handler at 114
    struct MUI_EventHandlerNode mad_ehn;
    struct MUI_InputHandlerNode mad_Timer; /* MUIA_Timer */
    ULONG              mad_Timeval;       /* just to trigger notifications */
    struct MUI_EventHandlerNode mad_ccn;  /* gross hack for control char */
    Object            *mad_ContextMenu;   /* menu strip */
    LONG               mad_ClickX;        /* x position of the initial SELECTDOWN click */
    LONG               mad_ClickY;        /* y position of the intiial SELECTDOWN click */
    struct ZMenu      *mad_ContextZMenu;
    struct MUI_EventHandlerNode mad_hiehn; /* Eventhandler to simulate MUIM_HandleInput */

    LONG               mad_DisableCount; /* counts number of disables */
// only 148 bytes for the struct in MUI !
};

/* Flags during MUIM_Draw */
#define MADF_DRAWOBJECT        (1<< 0) /* draw object completely */
#define MADF_DRAWUPDATE        (1<< 1) /* update object */

#define MADF_DRAWALL           (1<< 31)


/* mad_Flags, private one */


#define MADF_DRAWFLAGS (MADF_DRAWOBJECT | MADF_DRAWUPDATE | MADF_DRAW_XXX \
    | MADF_DRAWFRAME | MADF_DRAW_XXX_2 | MADF_DRAWALL)


// offset 94 (byte) (frame << 1) (lsb is SETUP_DONE flag)
enum {
    MUIV_Frame_None = 0,
    MUIV_Frame_Button,
    MUIV_Frame_ImageButton,
    MUIV_Frame_Text,
    MUIV_Frame_String,
    MUIV_Frame_ReadList,
    MUIV_Frame_InputList,
    MUIV_Frame_Prop,
    MUIV_Frame_Gauge,
    MUIV_Frame_Group,
    MUIV_Frame_PopUp,
    MUIV_Frame_Virtual,
    MUIV_Frame_Slider,
    MUIV_Frame_Knob,
    MUIV_Frame_Drag,
    MUIV_Frame_Count,
};

// offset 95
enum {
    MUIV_InputMode_None = 0,  // 0x00
    MUIV_InputMode_RelVerify, // 0x40 (1<<6)
    MUIV_InputMode_Immediate, // 0x80 (1<<7)
    MUIV_InputMode_Toggle,    // 0xc0 (1<<7 | 1<<6)
};



enum {
    MUIV_DragQuery_Refuse = 0,
    MUIV_DragQuery_Accept,
};

enum {
    MUIV_DragReport_Abort =  0,
    MUIV_DragReport_Continue,
    MUIV_DragReport_Lock,
    MUIV_DragReport_Refresh,
};

#define MUIV_CreateBubble_DontHidePointer (1<<0)

/* A private functions and macros */
void __area_finish_minmax(Object *obj, struct MUI_MinMax *MinMaxInfo);

/*#define DRAW_BG_RECURSIVE (1<<1)*/


#endif /* _MUI_CLASSES_AREA_H */
#endif

#ifndef _MUI_CLASSES_GROUP_H
#ifndef _MUI_CLASSES_GROUP_H
#define _MUI_CLASSES_GROUP_H

/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Group                 "Group.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Group                 (MUIB_ZUNE | 0x00001000)

/*** Methods ****************************************************************/
#define MUIM_Group_ExitChange      (MUIB_MUI|0x0042d1cc) /* MUI: V11 */
#define MUIM_Group_InitChange      (MUIB_MUI|0x00420887) /* MUI: V11 */
#define MUIM_Group_Sort            (MUIB_MUI|0x00427417) /* MUI: V4  */
struct MUIP_Group_ExitChange       {STACKED ULONG MethodID;};
struct MUIP_Group_InitChange       {STACKED ULONG MethodID;};
struct MUIP_Group_Sort             {STACKED ULONG MethodID; STACKED Object *obj[1];};

#define MUIM_Group_DoMethodNoForward (MUIB_Group | 0x00000000)
struct MUIP_Group_DoMethodNoForward  {STACKED ULONG MethodID; STACKED ULONG DoMethodID; }; /* msg stuff follows */

/*** Attributes *************************************************************/
#define MUIA_Group_ActivePage      (MUIB_MUI|0x00424199) /* MUI: V5  isg LONG          */
#define MUIA_Group_Child           (MUIB_MUI|0x004226e6) /* MUI: V4  i.. Object *      */
#define MUIA_Group_ChildList       (MUIB_MUI|0x00424748) /* MUI: V4  ..g struct List * */
#define MUIA_Group_Columns         (MUIB_MUI|0x0042f416) /* MUI: V4  is. LONG          */
#define MUIA_Group_Forward         (MUIB_MUI|0x00421422) /* MUI: V11 .s. BOOL          */
#define MUIA_Group_Horiz           (MUIB_MUI|0x0042536b) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_HorizSpacing    (MUIB_MUI|0x0042c651) /* MUI: V4  isg LONG          */
#define MUIA_Group_LayoutHook      (MUIB_MUI|0x0042c3b2) /* MUI: V11 i.. struct Hook * */
#define MUIA_Group_PageMode        (MUIB_MUI|0x00421a5f) /* MUI: V5  i.. BOOL          */
#define MUIA_Group_Rows            (MUIB_MUI|0x0042b68f) /* MUI: V4  is. LONG          */
#define MUIA_Group_SameHeight      (MUIB_MUI|0x0042037e) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_SameSize        (MUIB_MUI|0x00420860) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_SameWidth       (MUIB_MUI|0x0042b3ec) /* MUI: V4  i.. BOOL          */
#define MUIA_Group_Spacing         (MUIB_MUI|0x0042866d) /* MUI: V4  is. LONG          */
#define MUIA_Group_VertSpacing     (MUIB_MUI|0x0042e1bf) /* MUI: V4  isg LONG          */

#define MUIA_Group_Virtual         (MUIB_Group | 0x00000000) /* Zune: V1 i.. BOOL  */

enum {
    MUIV_Group_ActivePage_First = 0,
    MUIV_Group_ActivePage_Last = -1,
    MUIV_Group_ActivePage_Prev = -2,
    MUIV_Group_ActivePage_Next = -3,
    MUIV_Group_ActivePage_Advance = -4,
};

/* This is the message you get if your custom layout hook is called */
struct MUI_LayoutMsg
{
    STACKED ULONG              lm_Type;     /* the message type */
    STACKED struct MinList    *lm_Children; /* exec list of the children of this group */
    STACKED struct MUI_MinMax  lm_MinMax;   /* here you have to place the MUILM_MINMAX results */
    struct
    {
	STACKED LONG Width;
	STACKED LONG Height;
	STACKED ULONG priv5;
	STACKED ULONG priv6;
    } lm_Layout;   /* size (and result) for MUILM_LAYOUT */
};

/* lm_Type */
enum
{
    MUILM_MINMAX = 1,  /* Please calc your min & max siizes */
    MUILM_LAYOUT = 2,  /* Please layout your children */
};

#define MUILM_UNKNOWN  -1  /* should be returned if the hook function doesn't understand lm_Type */



#endif /* _MUI_CLASSES_GROUP_H */
#endif

#ifndef _MUI_CLASSES_RECTANGLE_H
#ifndef _MUI_CLASSES_RECTANGLE_H
#define _MUI_CLASSES_RECTANGLE_H

/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Rectangle          "Rectangle.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Rectangle          (MUIB_ZUNE | 0x00002b00)  

/*** Attributes *************************************************************/
#define MUIA_Rectangle_BarTitle (MUIB_MUI|0x00426689) /* V11 i.g STRPTR */
#define MUIA_Rectangle_HBar     (MUIB_MUI|0x0042c943) /* V7  i.g BOOL   */
#define MUIA_Rectangle_VBar     (MUIB_MUI|0x00422204) /* V7  i.g BOOL   */


#endif /* _MUI_CLASSES_RECTANGLE_H */
#endif

#ifndef _MUI_CLASSES_TEXT_H
#ifndef _MUI_CLASSES_TEXT_H
#define _MUI_CLASSES_TEXT_H

/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Text           "Text.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Text           (MUIB_ZUNE | 0x00003500)  

/*** Attributes *************************************************************/
#define MUIA_Text_Contents  (MUIB_MUI|0x0042f8dc) /* MUI: V4  isg STRPTR */
#define MUIA_Text_HiChar    (MUIB_MUI|0x004218ff) /* MUI: V4  i.. char   */
#define MUIA_Text_HiCharIdx (MUIB_MUI|0x004214f5) /*          i.. char   */
#define MUIA_Text_PreParse  (MUIB_MUI|0x0042566d) /* MUI: V4  isg STRPTR */
#define MUIA_Text_SetMax    (MUIB_MUI|0x00424d0a) /* MUI: V4  i.. BOOL   */
#define MUIA_Text_SetMin    (MUIB_MUI|0x00424e10) /* MUI: V4  i.. BOOL   */
#define MUIA_Text_SetVMax   (MUIB_MUI|0x00420d8b) /* MUI: V11 i.. BOOL   */

#define MUIA_Text_Editable    (MUIB_Text | 0x00000000)  /* DEPRECATED */
#define MUIA_Text_Multiline   (MUIB_Text | 0x00000001)  /* DEPRECATED */

/* Codes which can be used in text strings */
#define MUIX_L "\033l"	    /* justify left */
#define MUIX_C "\033c"      /* justify centered */
#define MUIX_R "\033r"      /* justify right */

#define MUIX_N "\033n"      /* normal style */
#define MUIX_B "\033b"      /* bold style */
#define MUIX_I "\033i"      /* italic style */
#define MUIX_U "\033u"      /* underlined style */

#define MUIX_PT "\0332"     /* use text pen */
#define MUIX_PH "\0338"     /* use highlight text pen */



#endif /* _MUI_CLASSES_TEXT_H */
#endif

#ifndef _MUI_CLASSES_NUMERIC_H
#ifndef _MUI_CLASSES_NUMERIC_H
#define _MUI_CLASSES_NUMERIC_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Numeric               "Numeric.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Numeric               (MUIB_ZUNE | 0x00001e00)  

/*** Methods ****************************************************************/
#define MUIM_Numeric_Decrease      (MUIB_MUI|0x004243a7) /* MUI: V11 */
#define MUIM_Numeric_Increase      (MUIB_MUI|0x00426ecd) /* MUI: V11 */
#define MUIM_Numeric_ScaleToValue  (MUIB_MUI|0x0042032c) /* MUI: V11 */
#define MUIM_Numeric_SetDefault    (MUIB_MUI|0x0042ab0a) /* MUI: V11 */
#define MUIM_Numeric_Stringify     (MUIB_MUI|0x00424891) /* MUI: V11 */
#define MUIM_Numeric_ValueToScale  (MUIB_MUI|0x00423e4f) /* MUI: V11 */
struct MUIP_Numeric_Decrease       {STACKED ULONG MethodID; STACKED LONG amount;};
struct MUIP_Numeric_Increase       {STACKED ULONG MethodID; STACKED LONG amount;};
struct MUIP_Numeric_ScaleToValue   {STACKED ULONG MethodID; STACKED LONG scalemin; STACKED LONG scalemax; STACKED LONG scale;};
struct MUIP_Numeric_SetDefault     {STACKED ULONG MethodID;};
struct MUIP_Numeric_Stringify      {STACKED ULONG MethodID; STACKED LONG value;};
struct MUIP_Numeric_ValueToScale   {STACKED ULONG MethodID; STACKED LONG scalemin; STACKED LONG scalemax;};

/*** Attributes *************************************************************/
#define MUIA_Numeric_CheckAllSizes (MUIB_MUI|0x00421594) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_Default       (MUIB_MUI|0x004263e8) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Format        (MUIB_MUI|0x004263e9) /* MUI: V11 isg STRPTR */
#define MUIA_Numeric_Max           (MUIB_MUI|0x0042d78a) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Min           (MUIB_MUI|0x0042e404) /* MUI: V11 isg LONG   */
#define MUIA_Numeric_Reverse       (MUIB_MUI|0x0042f2a0) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_RevLeftRight  (MUIB_MUI|0x004294a7) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_RevUpDown     (MUIB_MUI|0x004252dd) /* MUI: V11 isg BOOL   */
#define MUIA_Numeric_Value         (MUIB_MUI|0x0042ae3a) /* MUI: V11 isg LONG   */

#define MUIM_Numeric_ValueToScaleExt (MUIB_Numeric | 0x00000000) /* ZUNE only */
struct MUIP_Numeric_ValueToScaleExt   {STACKED ULONG MethodID; STACKED LONG value; STACKED LONG scalemin; STACKED LONG scalemax;};


#endif /* _MUI_CLASSES_NUMERIC_H */
#endif

#ifndef _MUI_CLASSES_SLIDER_H
#ifndef _MUI_CLASSES_SLIDER_H
#define _MUI_CLASSES_SLIDER_H

/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002-2003, The AROS Development Team.
    All rights reserved.

    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Slider          "Slider.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Slider          (MUIB_ZUNE | 0x00003300)

/*** Attributes *************************************************************/
#define MUIA_Slider_Horiz    (MUIB_MUI|0x0042fad1) /* V11 isg BOOL */
#define MUIA_Slider_Quiet    (MUIB_MUI|0x00420b26) /* V6  i.. BOOL */

#ifdef MUI_OBSOLETE
#define MUIA_Slider_Level    (MUIB_MUI|0x0042ae3a) /* V4  isg LONG */
#define MUIA_Slider_Max      (MUIB_MUI|0x0042d78a) /* V4  isg LONG */
#define MUIA_Slider_Min      (MUIB_MUI|0x0042e404) /* V4  isg LONG */
#define MUIA_Slider_Reverse  (MUIB_MUI|0x0042f2a0) /* V4  isg BOOL */
#endif /* MUI_OBSOLETE */



#endif /* _MUI_CLASSES_SLIDER_H */
#endif

#ifndef _MUI_CLASSES_STRING_H
#ifndef _MUI_CLASSES_STRING_H
#define _MUI_CLASSES_STRING_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_String                  "String.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_String                  (MUIB_ZUNE | 0x00003400)

/*** Attributes *************************************************************/
#define MUIA_String_Accept           (MUIB_MUI|0x0042e3e1) /* V4  isg STRPTR        */
#define MUIA_String_Acknowledge      (MUIB_MUI|0x0042026c) /* V4  ..g STRPTR        */
#define MUIA_String_AdvanceOnCR      (MUIB_MUI|0x004226de) /* V11 isg BOOL          */
#define MUIA_String_AttachedList     (MUIB_MUI|0x00420fd2) /* V4  isg Object *      */
#define MUIA_String_BufferPos        (MUIB_MUI|0x00428b6c) /* V4  .sg LONG          */
#define MUIA_String_Contents         (MUIB_MUI|0x00428ffd) /* V4  isg STRPTR        */
#define MUIA_String_DisplayPos       (MUIB_MUI|0x0042ccbf) /* V4  .sg LONG          */
#define MUIA_String_EditHook         (MUIB_MUI|0x00424c33) /* V7  isg struct Hook * */
#define MUIA_String_Format           (MUIB_MUI|0x00427484) /* V4  i.g LONG          */
#define MUIA_String_Integer          (MUIB_MUI|0x00426e8a) /* V4  isg ULONG         */
#define MUIA_String_LonelyEditHook   (MUIB_MUI|0x00421569) /* V11 isg BOOL          */
#define MUIA_String_MaxLen           (MUIB_MUI|0x00424984) /* V4  i.g LONG          */
#define MUIA_String_Reject           (MUIB_MUI|0x0042179c) /* V4  isg STRPTR        */
#define MUIA_String_Secret           (MUIB_MUI|0x00428769) /* V4  i.g BOOL          */

enum {
    MUIV_String_Format_Left = 0,
    MUIV_String_Format_Center,
    MUIV_String_Format_Right,
};

/* Extended features taken over from Alan Odgaard's BetterString MCC.
   Attribute and method IDs match those of BetterString class. */

#define MUIA_String_Columns         	    0xad001005
#define MUIA_String_NoInput         	    0xad001007
#define MUIA_String_SelectSize      	    0xad001001
#define MUIA_String_StayActive      	    0xad001003
#define MUIA_String_KeyUpFocus      	    0xad001008
#define MUIA_String_KeyDownFocus    	    0xad001009

#define MUIM_String_ClearSelected   	    0xad001004
#define MUIM_String_FileNameStart   	    0xad001006
#define MUIM_String_Insert          	    0xad001002

#define MUIV_String_Insert_StartOfString    0x00000000
#define MUIV_String_Insert_EndOfString      0xfffffffe
#define MUIV_String_Insert_BufferPos        0xffffffff
#define MUIV_String_BufferPos_End           0xffffffff

#define MUIR_String_FileNameStart_Volume    0xffffffff

struct MUIP_String_ClearSelected {STACKED ULONG MethodID;};
struct MUIP_String_FileNameStart {STACKED ULONG MethodID; STACKED STRPTR buffer; STACKED LONG pos;};
struct MUIP_String_Insert        {STACKED ULONG MethodID; STACKED STRPTR text; STACKED LONG pos;};


#endif /* _MUI_CLASSES_STRING_H */
#endif

#ifndef _MUI_CLASSES_BOOPSI_H
/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_BOOPSI_H
#define _MUI_CLASSES_BOOPSI_H

/*** Name *******************************************************************/
#define MUIC_Boopsi             "Boopsi.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Boopsi             (MUIB_ZUNE | 0x00000600)

/*** Attributes *************************************************************/
#define MUIA_Boopsi_Class       (MUIB_MUI|0x00426999) /* V4  isg struct IClass * */
#define MUIA_Boopsi_ClassID     (MUIB_MUI|0x0042bfa3) /* V4  isg char *          */
#define MUIA_Boopsi_MaxHeight   (MUIB_MUI|0x0042757f) /* V4  isg ULONG           */
#define MUIA_Boopsi_MaxWidth    (MUIB_MUI|0x0042bcb1) /* V4  isg ULONG           */
#define MUIA_Boopsi_MinHeight   (MUIB_MUI|0x00422c93) /* V4  isg ULONG           */
#define MUIA_Boopsi_MinWidth    (MUIB_MUI|0x00428fb2) /* V4  isg ULONG           */
#define MUIA_Boopsi_Object      (MUIB_MUI|0x00420178) /* V4  ..g Object *        */
#define MUIA_Boopsi_Remember    (MUIB_MUI|0x0042f4bd) /* V4  i.. ULONG           */
#define MUIA_Boopsi_Smart       (MUIB_MUI|0x0042b8d7) /* V9  i.. BOOL            */
#define MUIA_Boopsi_TagDrawInfo (MUIB_MUI|0x0042bae7) /* V4  isg ULONG           */
#define MUIA_Boopsi_TagScreen   (MUIB_MUI|0x0042bc71) /* V4  isg ULONG           */
#define MUIA_Boopsi_TagWindow   (MUIB_MUI|0x0042e11d) /* V4  isg ULONG           */




#endif /* _MUI_CLASSES_BOOPSI_H */
#endif

#ifndef _MUI_CLASSES_PROP_H
#ifndef _MUI_CLASSES_PROP_H
#define _MUI_CLASSES_PROP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Prop           "Prop.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Prop           (MUIB_ZUNE | 0x00002900)  

/*** Methods ****************************************************************/
#define MUIM_Prop_Decrease  (MUIB_MUI|0x00420dd1) /* MUI: V16 */
#define MUIM_Prop_Increase  (MUIB_MUI|0x0042cac0) /* MUI: V16 */
struct  MUIP_Prop_Decrease  {STACKED ULONG MethodID; STACKED LONG amount;};
struct  MUIP_Prop_Increase  {STACKED ULONG MethodID; STACKED LONG amount;};

/*** Attributes *************************************************************/
#define MUIA_Prop_Entries        (MUIB_MUI|0x0042fbdb) /* MUI: V4  isg LONG */
#define MUIA_Prop_First          (MUIB_MUI|0x0042d4b2) /* MUI: V4  isg LONG */
#define MUIA_Prop_Horiz          (MUIB_MUI|0x0042f4f3) /* MUI: V4  i.g BOOL */
#define MUIA_Prop_Slider         (MUIB_MUI|0x00429c3a) /* MUI: V4  isg BOOL */
#define MUIA_Prop_UseWinBorder   (MUIB_MUI|0x0042deee) /* MUI: V13 i.. LONG */
#define MUIA_Prop_Visible        (MUIB_MUI|0x0042fea6) /* MUI: V4  isg LONG */


enum
{
    MUIV_Prop_UseWinBorder_None = 0,
    MUIV_Prop_UseWinBorder_Left,
    MUIV_Prop_UseWinBorder_Right,
    MUIV_Prop_UseWinBorder_Bottom,
};

#define MUIA_Prop_DeltaFactor    (MUIB_MUI|0x00427c5e) /* MUI:    is. LONG */
#define MUIA_Prop_DoSmooth       (MUIB_MUI|0x004236ce) /* MUI: V4 i.. LONG */



#endif /* _MUI_CLASSES_PROP_H */
#endif

#ifndef _MUI_CLASSES_SCROLLBAR_H
#ifndef _MUI_CLASSES_SCROLLBAR_H
#define _MUI_CLASSES_SCROLLBAR_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Scrollbar       "Scrollbar.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scrollbar       (MUIB_ZUNE | 0x00002e00)

/*** Attributes *************************************************************/
#define MUIA_Scrollbar_Type  (MUIB_MUI|0x0042fb6b) /* V11 i.. LONG */

enum
{
    MUIV_Scrollbar_Type_Default = 0,
    MUIV_Scrollbar_Type_Bottom,
    MUIV_Scrollbar_Type_Top,
    MUIV_Scrollbar_Type_Sym,
};



#endif /* _MUI_CLASSES_SCROLLBAR_H */
#endif

#ifndef _MUI_CLASSES_REGISTER_H
#ifndef _MUI_CLASSES_REGISTER_H
#define _MUI_CLASSES_REGISTER_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Register           "Register.mui"

/*** Identifier base  (for Zune extensions) *********************************/
#define MUIB_Register           (MUIB_ZUNE | 0x00002c00)

/*** Attributes *************************************************************/
#define MUIA_Register_Frame     (MUIB_MUI|0x0042349b) /* V7  i.g BOOL     */
#define MUIA_Register_Titles    (MUIB_MUI|0x004297ec) /* V7  i.g STRPTR * */

#define MUIA_Register_Columns   (MUIB_Register | 0x0000) /* Zune V1  i..  */



#endif /* _MUI_CLASSES_REGISTER_H */
#endif

#ifndef _MUI_CLASSES_MENUITEM_H
#ifndef _MUI_CLASSES_MENUITEM_H
#define _MUI_CLASSES_MENUITEM_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Menustrip         "Menustrip.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Menustrip         (MUIB_ZUNE | 0x00001a00)

/*** Attributes *************************************************************/
#define MUIA_Menustrip_Enabled (MUIB_MUI|0x0042815b) /* MUI: V8  isg BOOL */



/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Menu         "Menu.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Menu         (MUIB_ZUNE | 0x00001b00)  

/*** Attributes *************************************************************/
#define MUIA_Menu_Enabled (MUIB_MUI|0x0042ed48) /* MUI: V8  isg BOOL   */
#define MUIA_Menu_Title   (MUIB_MUI|0x0042a0e3) /* MUI: V8  isg STRPTR */



/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Menuitem               "Menuitem.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Menuitem               (MUIB_ZUNE | 0x00001c00)

/*** Attributes *************************************************************/
#define MUIA_Menuitem_Checked       (MUIB_MUI|0x0042562a) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Checkit       (MUIB_MUI|0x00425ace) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_CommandString (MUIB_MUI|0x0042b9cc) /* MUI: V16 isg BOOL              */
#define MUIA_Menuitem_Enabled       (MUIB_MUI|0x0042ae0f) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Exclude       (MUIB_MUI|0x00420bc6) /* MUI: V8  isg LONG              */
#define MUIA_Menuitem_Shortcut      (MUIB_MUI|0x00422030) /* MUI: V8  isg STRPTR            */
#define MUIA_Menuitem_Title         (MUIB_MUI|0x004218be) /* MUI: V8  isg STRPTR            */
#define MUIA_Menuitem_Toggle        (MUIB_MUI|0x00424d5c) /* MUI: V8  isg BOOL              */
#define MUIA_Menuitem_Trigger       (MUIB_MUI|0x00426f32) /* MUI: V8  ..g struct MenuItem * */

#define MUIA_Menuitem_NewMenu       (MUIB_Menuitem | 0x00000000) /* Zune: V1 ..g struct NewMenu *  */


#define MUIV_Menuitem_Shortcut_Check (-1)



#endif /* _MUI_CLASSES_MENUITEM_H */
#endif

#ifndef _MUI_CLASSES_DATASPACE_H
#ifndef _MUI_CLASSES_DATASPACE_H
#define _MUI_CLASSES_DATASPACE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Dataspace            "Dataspace.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Dataspace            (MUIB_ZUNE | 0x00000b00)


/*** Methods ****************************************************************/
#define MUIM_Dataspace_Add        (MUIB_MUI|0x00423366) /* MUI: V11 */
#define MUIM_Dataspace_Clear      (MUIB_MUI|0x0042b6c9) /* MUI: V11 */
#define MUIM_Dataspace_Find       (MUIB_MUI|0x0042832c) /* MUI: V11 */
#define MUIM_Dataspace_Merge      (MUIB_MUI|0x00423e2b) /* MUI: V11 */
#define MUIM_Dataspace_ReadIFF    (MUIB_MUI|0x00420dfb) /* MUI: V11 */
#define MUIM_Dataspace_Remove     (MUIB_MUI|0x0042dce1) /* MUI: V11 */
#define MUIM_Dataspace_WriteIFF   (MUIB_MUI|0x00425e8e) /* MUI: V11 */
struct MUIP_Dataspace_Add         {STACKED ULONG MethodID; STACKED APTR data; STACKED LONG len; STACKED ULONG id;};
struct MUIP_Dataspace_Clear       {STACKED ULONG MethodID;};
struct MUIP_Dataspace_Find        {STACKED ULONG MethodID; STACKED ULONG id;};
struct MUIP_Dataspace_Merge       {STACKED ULONG MethodID; STACKED Object *dataspace;};
struct MUIP_Dataspace_ReadIFF     {STACKED ULONG MethodID; STACKED struct IFFHandle *handle;};
struct MUIP_Dataspace_Remove      {STACKED ULONG MethodID; STACKED ULONG id;};
struct MUIP_Dataspace_WriteIFF    {STACKED ULONG MethodID; STACKED struct IFFHandle *handle; STACKED ULONG type; STACKED ULONG id;};

/*** Attributes *************************************************************/
#define MUIA_Dataspace_Pool       (MUIB_MUI|0x00424cf9) /* MUI: V11 i.. APTR */



#endif /* _MUI_CLASSES_DATASPACE_H */
#endif

#ifndef _MUI_CLASSES_VIRTGROUP_H
#ifndef _MUI_CLASSES_VIRTGROUP_H
#define _MUI_CLASSES_VIRTGROUP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Virtgroup          "Virtgroup.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Virtgroup          (MUIB_ZUNE | 0x00003700)  

/*** Attributes *************************************************************/
#define MUIA_Virtgroup_Height   (MUIB_MUI|0x00423038) /* V6  ..g LONG */
#define MUIA_Virtgroup_Input    (MUIB_MUI|0x00427f7e) /* V11 i.. BOOL */
#define MUIA_Virtgroup_Left     (MUIB_MUI|0x00429371) /* V6  isg LONG */
#define MUIA_Virtgroup_Top      (MUIB_MUI|0x00425200) /* V6  isg LONG */
#define MUIA_Virtgroup_Width    (MUIB_MUI|0x00427c49) /* V6  ..g LONG */




#endif /* _MUI_CLASSES_VIRTGROUP_H */
#endif

#ifndef _MUI_CLASSES_SCROLLGROUP_H
#ifndef _MUI_CLASSES_SCROLLGROUP_H
#define _MUI_CLASSES_SCROLLGROUP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Scrollgroup              "Scrollgroup.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scrollgroup              (MUIB_ZUNE | 0x00002f00)

/*** Attributes *************************************************************/
#define MUIA_Scrollgroup_Contents     (MUIB_MUI|0x00421261) /* V4  i.g Object * */
#define MUIA_Scrollgroup_FreeHoriz    (MUIB_MUI|0x004292f3) /* V9  i.. BOOL     */
#define MUIA_Scrollgroup_FreeVert     (MUIB_MUI|0x004224f2) /* V9  i.. BOOL     */
#define MUIA_Scrollgroup_HorizBar     (MUIB_MUI|0x0042b63d) /* V16 ..g Object * */
#define MUIA_Scrollgroup_UseWinBorder (MUIB_MUI|0x004284c1) /* V13 i.. BOOL     */
#define MUIA_Scrollgroup_VertBar      (MUIB_MUI|0x0042cdc0) /* V16 ..g Object * */



#endif /* _MUI_CLASSES_SCROLLGROUP_H */
#endif

#ifndef _MUI_CLASSES_SCROLLBUTTON_H
/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_SCROLLBUTTON_H
#define _MUI_CLASSES_SCROLLBUTTON_H

/*** Name *******************************************************************/
#define MUIC_Scrollbutton             "Scrollbutton.mui"

/*** Identifier base ********************************************************/
#define MUIB_Scrollbutton             (MUIB_ZUNE | 0x00004100)  

/*** Attributes *************************************************************/
#define MUIA_Scrollbutton_NewPosition (MUIB_Scrollbutton | 0x00000000) /* --G  ULONG (2 x WORD) */
#define MUIA_Scrollbutton_Horiz       (MUIB_Scrollbutton | 0x00000001) /* -SG  WORD             */
#define MUIA_Scrollbutton_Vert        (MUIB_Scrollbutton | 0x00000002) /* -SG  WORD             */
#define MUIA_Scrollbutton_HorizProp   (MUIB_Scrollbutton | 0x00000003) /* --G  Object *         */
#define MUIA_Scrollbutton_VertProp    (MUIB_Scrollbutton | 0x00000004) /* --G  Object *         */

/*** Macros *****************************************************************/
#define ScrollbuttonObject MUIOBJMACRO_START(MUIC_Scrollbutton)


#endif  /* _MUI_CLASSES_SCROLLBUTTON_H */
#endif

#ifndef _MUI_CLASSES_SEMAPHORE_H
#ifndef _MUI_CLASSES_SEMAPHORE_H
#define _MUI_CLASSES_SEMAPHORE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Semaphore "Semaphore.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Semaphore (MUIB_ZUNE | 0x00003000)

/*** Methods ****************************************************************/
#define MUIM_Semaphore_Attempt       (MUIB_MUI|0x00426ce2) /* MUI: V11 */
#define MUIM_Semaphore_AttemptShared (MUIB_MUI|0x00422551) /* MUI: V11 */
#define MUIM_Semaphore_Obtain        (MUIB_MUI|0x004276f0) /* MUI: V11 */
#define MUIM_Semaphore_ObtainShared  (MUIB_MUI|0x0042ea02) /* MUI: V11 */
#define MUIM_Semaphore_Release       (MUIB_MUI|0x00421f2d) /* MUI: V11 */
struct MUIP_Semaphore_Attempt        {STACKED ULONG MethodID;};
struct MUIP_Semaphore_AttemptShared  {STACKED ULONG MethodID;};
struct MUIP_Semaphore_Obtain         {STACKED ULONG MethodID;};
struct MUIP_Semaphore_ObtainShared   {STACKED ULONG MethodID;};
struct MUIP_Semaphore_Release        {STACKED ULONG MethodID;};



#endif /* _MUI_CLASSES_SEMAPHORE_H */
#endif

#ifndef _MUI_CLASSES_BITMAP_H
/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_BITMAP_H
#define _MUI_CLASSES_BITMAP_H

/*** Name *******************************************************************/
#define MUIC_Bitmap                 "Bitmap.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Bitmap                 (MUIB_ZUNE | 0x00000400)

/*** Attributes *************************************************************/
#define MUIA_Bitmap_Bitmap          (MUIB_MUI|0x004279bd) /* MUI: V8  isg struct BitMap *   */
#define MUIA_Bitmap_Height          (MUIB_MUI|0x00421560) /* MUI: V8  isg LONG              */
#define MUIA_Bitmap_MappingTable    (MUIB_MUI|0x0042e23d) /* MUI: V8  isg UBYTE *           */
#define MUIA_Bitmap_Precision       (MUIB_MUI|0x00420c74) /* MUI: V11 isg LONG              */
#define MUIA_Bitmap_RemappedBitmap  (MUIB_MUI|0x00423a47) /* MUI: V11 ..g struct BitMap *   */
#define MUIA_Bitmap_SourceColors    (MUIB_MUI|0x00425360) /* MUI: V8  isg ULONG *           */
#define MUIA_Bitmap_Transparent     (MUIB_MUI|0x00422805) /* MUI: V8  isg LONG              */
#define MUIA_Bitmap_UseFriend       (MUIB_MUI|0x004239d8) /* MUI: V11 i.. BOOL              */
#define MUIA_Bitmap_Width           (MUIB_MUI|0x0042eb3a) /* MUI: V8  isg LONG              */


#endif /* _MUI_CLASSES_BITMAP_H */
#endif

#ifndef _MUI_CLASSES_BODYCHUNK_H
/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_BODYCHUNK_H
#define _MUI_CLASSES_BODYCHUNK_H

/*** Name *******************************************************************/
#define MUIC_Bodychunk              "Bodychunk.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Bodychunk              (MUIB_ZUNE | 0x00000500)  

/*** Attributes *************************************************************/
#define MUIA_Bodychunk_Body         (MUIB_MUI|0x0042ca67) /* V8  isg UBYTE * */
#define MUIA_Bodychunk_Compression  (MUIB_MUI|0x0042de5f) /* V8  isg UBYTE   */
#define MUIA_Bodychunk_Depth        (MUIB_MUI|0x0042c392) /* V8  isg LONG    */
#define MUIA_Bodychunk_Masking      (MUIB_MUI|0x00423b0e) /* V8  isg UBYTE   */



#endif /* _MUI_CLASSES_BODYCHUNK_H */
#endif

#ifndef _MUI_CLASSES_CHUNKYIMAGE_H
#ifndef _MUI_CLASSES_CHUNKYIMAGE_H
#define _MUI_CLASSES_CHUNKYIMAGE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_ChunkyImage            "ChunkyImage.mui"

/*** Identifier base ********************************************************/
#define MUIB_ChunkyImage            (MUIB_ZUNE | 0x00004000)

/*** Attributes *************************************************************/
#define MUIA_ChunkyImage_Pixels     (MUIB_ChunkyImage | 0x00000000) /* V8  isg UBYTE * */
#define MUIA_ChunkyImage_Palette    (MUIB_ChunkyImage | 0x00000001) /* V8  isg UBYTE * */
#define MUIA_ChunkyImage_NumColors  (MUIB_ChunkyImage | 0x00000002) /* V8  isg LONG    */
#define MUIA_ChunkyImage_Modulo     (MUIB_ChunkyImage | 0x00000003) /* V8  isg LONG    */



#endif /* _MUI_CLASSES_CHUNKYIMAGE_H */
#endif

#ifndef _MUI_CLASSES_LISTVIEW_H
#ifndef _CLASSES_LISTVIEW_H
#define _CLASSES_LISTVIEW_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Listview "Listview.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Listview                (MUIB_ZUNE | 0x00001900)

/*** Attributes *************************************************************/
#define MUIA_Listview_ClickColumn    (MUIB_MUI|0x0042d1b3) /* V7  ..g LONG    */
#define MUIA_Listview_DefClickColumn (MUIB_MUI|0x0042b296) /* V7  isg LONG    */
#define MUIA_Listview_DoubleClick    (MUIB_MUI|0x00424635) /* V4  i.g BOOL    */
#define MUIA_Listview_DragType       (MUIB_MUI|0x00425cd3) /* V11 isg LONG    */
#define MUIA_Listview_Input          (MUIB_MUI|0x0042682d) /* V4  i.. BOOL    */
#define MUIA_Listview_List           (MUIB_MUI|0x0042bcce) /* V4  i.g Object  */
#define MUIA_Listview_MultiSelect    (MUIB_MUI|0x00427e08) /* V7  i.. LONG    */
#define MUIA_Listview_ScrollerPos    (MUIB_MUI|0x0042b1b4) /* V10 i.. BOOL    */
#define MUIA_Listview_SelectChange   (MUIB_MUI|0x0042178f) /* V4  ..g BOOL    */

enum
{
    MUIV_Listview_DragType_None = 0,
    MUIV_Listview_DragType_Immediate,
};

enum
{
    MUIV_Listview_MultiSelect_None = 0,
    MUIV_Listview_MultiSelect_Default,
    MUIV_Listview_MultiSelect_Shifted,
    MUIV_Listview_MultiSelect_Always,
};

enum
{
    MUIV_Listview_ScrollerPos_Default = 0,
    MUIV_Listview_ScrollerPos_Left,
    MUIV_Listview_ScrollerPos_Right,
    MUIV_Listview_ScrollerPos_None,
};



#endif /* _CLASSES_LISTVIEW_H */
#endif

#ifndef _MUI_CLASSES_LIST_H
#ifndef _MUI_CLASSES_LIST_H
#define _MUI_CLASSES_LIST_H

/*
    Copyright © 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_List                     "List.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_List                     (MUIB_ZUNE | 0x00001400)  

/*** Methods ****************************************************************/
#define MUIM_List_Clear               (MUIB_MUI|0x0042ad89) /* MUI: V4  */
#define MUIM_List_Compare             (MUIB_MUI|0x00421b68) /* MUI: V20 */
#define MUIM_List_Construct           (MUIB_MUI|0x0042d662) /* MUI: V20 */
#define MUIM_List_CreateImage         (MUIB_MUI|0x00429804) /* MUI: V11 */
#define MUIM_List_DeleteImage         (MUIB_MUI|0x00420f58) /* MUI: V11 */
#define MUIM_List_Destruct            (MUIB_MUI|0x00427d51) /* MUI: V20 */
#define MUIM_List_Display             (MUIB_MUI|0x00425377) /* MUI: V20 */
#define MUIM_List_Exchange            (MUIB_MUI|0x0042468c) /* MUI: V4  */
#define MUIM_List_GetEntry            (MUIB_MUI|0x004280ec) /* MUI: V4  */
#define MUIM_List_Insert              (MUIB_MUI|0x00426c87) /* MUI: V4  */
#define MUIM_List_InsertSingle        (MUIB_MUI|0x004254d5) /* MUI: V7  */
#define MUIM_List_Jump                (MUIB_MUI|0x0042baab) /* MUI: V4  */
#define MUIM_List_Move                (MUIB_MUI|0x004253c2) /* MUI: V9  */
#define MUIM_List_NextSelected        (MUIB_MUI|0x00425f17) /* MUI: V6  */
#define MUIM_List_Redraw              (MUIB_MUI|0x00427993) /* MUI: V4  */
#define MUIM_List_Remove              (MUIB_MUI|0x0042647e) /* MUI: V4  */
#define MUIM_List_Select              (MUIB_MUI|0x004252d8) /* MUI: V4  */
#define MUIM_List_Sort                (MUIB_MUI|0x00422275) /* MUI: V4  */
#define MUIM_List_TestPos             (MUIB_MUI|0x00425f48) /* MUI: V11 */
struct MUIP_List_Clear                {STACKED ULONG MethodID;};
struct MUIP_List_Compare              {STACKED ULONG MethodID; STACKED APTR entry1; STACKED APTR entry2; STACKED LONG sort_type1; STACKED LONG sort_type2;};
struct MUIP_List_Construct            {STACKED ULONG MethodID; STACKED APTR entry; STACKED APTR pool;};
struct MUIP_List_CreateImage          {STACKED ULONG MethodID; STACKED Object *obj; STACKED ULONG flags;};
struct MUIP_List_DeleteImage          {STACKED ULONG MethodID; STACKED APTR listimg;};
struct MUIP_List_Destruct             {STACKED ULONG MethodID; STACKED APTR entry; STACKED APTR pool;};
struct MUIP_List_Display              {STACKED ULONG MethodID; STACKED APTR entry; STACKED STRPTR *array; STACKED LONG entry_pos; STACKED STRPTR *preparses;};
struct MUIP_List_Exchange             {STACKED ULONG MethodID; STACKED LONG pos1; STACKED LONG pos2;};
struct MUIP_List_GetEntry             {STACKED ULONG MethodID; STACKED LONG pos; STACKED APTR *entry;};
struct MUIP_List_Insert               {STACKED ULONG MethodID; STACKED APTR *entries; STACKED LONG count; STACKED LONG pos;};
struct MUIP_List_InsertSingle         {STACKED ULONG MethodID; STACKED APTR entry; STACKED LONG pos;};
struct MUIP_List_Jump                 {STACKED ULONG MethodID; STACKED LONG pos;};
struct MUIP_List_Move                 {STACKED ULONG MethodID; STACKED LONG from; STACKED LONG to;};
struct MUIP_List_NextSelected         {STACKED ULONG MethodID; STACKED LONG *pos;};
struct MUIP_List_Redraw               {STACKED ULONG MethodID; STACKED LONG pos;};
struct MUIP_List_Remove               {STACKED ULONG MethodID; STACKED LONG pos;};
struct MUIP_List_Select               {STACKED ULONG MethodID; STACKED LONG pos; STACKED LONG seltype; STACKED LONG *state;};
struct MUIP_List_Sort                 {STACKED ULONG MethodID;};
struct MUIP_List_TestPos              {STACKED ULONG MethodID; STACKED LONG x; STACKED LONG y; STACKED struct MUI_List_TestPos_Result *res;};

struct MUIP_List_SelectChange         {STACKED ULONG MethodID; STACKED LONG pos; STACKED LONG state; STACKED ULONG flags;};

/*** Attributes *************************************************************/
#define MUIA_List_Active              (MUIB_MUI|0x0042391c) /* MUI: V4  isg LONG          */
#define MUIA_List_AdjustHeight        (MUIB_MUI|0x0042850d) /* MUI: V4  i.. BOOL          */
#define MUIA_List_AdjustWidth         (MUIB_MUI|0x0042354a) /* MUI: V4  i.. BOOL          */
#define MUIA_List_AutoVisible         (MUIB_MUI|0x0042a445) /* MUI: V11 isg BOOL          */
#define MUIA_List_CompareHook         (MUIB_MUI|0x00425c14) /* MUI: V4  is. struct Hook * */
#define MUIA_List_ConstructHook       (MUIB_MUI|0x0042894f) /* MUI: V4  is. struct Hook * */
#define MUIA_List_DestructHook        (MUIB_MUI|0x004297ce) /* MUI: V4  is. struct Hook * */
#define MUIA_List_DisplayHook         (MUIB_MUI|0x0042b4d5) /* MUI: V4  is. struct Hook * */
#define MUIA_List_DragSortable        (MUIB_MUI|0x00426099) /* MUI: V11 isg BOOL          */
#define MUIA_List_DropMark            (MUIB_MUI|0x0042aba6) /* MUI: V11 ..g LONG          */
#define MUIA_List_Entries             (MUIB_MUI|0x00421654) /* MUI: V4  ..g LONG          */
#define MUIA_List_First               (MUIB_MUI|0x004238d4) /* MUI: V4  ..g LONG          */
#define MUIA_List_Format              (MUIB_MUI|0x00423c0a) /* MUI: V4  isg STRPTR        */
#define MUIA_List_InsertPosition      (MUIB_MUI|0x0042d0cd) /* MUI: V9  ..g LONG          */
#define MUIA_List_MinLineHeight       (MUIB_MUI|0x0042d1c3) /* MUI: V4  i.. LONG          */
#define MUIA_List_MultiTestHook       (MUIB_MUI|0x0042c2c6) /* MUI: V4  is. struct Hook * */
#define MUIA_List_Pool                (MUIB_MUI|0x00423431) /* MUI: V13 i.. APTR          */
#define MUIA_List_PoolPuddleSize      (MUIB_MUI|0x0042a4eb) /* MUI: V13 i.. ULONG         */
#define MUIA_List_PoolThreshSize      (MUIB_MUI|0x0042c48c) /* MUI: V13 i.. ULONG         */
#define MUIA_List_Quiet               (MUIB_MUI|0x0042d8c7) /* MUI: V4  .s. BOOL          */
#define MUIA_List_ShowDropMarks       (MUIB_MUI|0x0042c6f3) /* MUI: V11 isg BOOL          */
#define MUIA_List_SourceArray         (MUIB_MUI|0x0042c0a0) /* MUI: V4  i.. APTR          */
#define MUIA_List_Title               (MUIB_MUI|0x00423e66) /* MUI: V6  isg char *        */
#define MUIA_List_Visible             (MUIB_MUI|0x0042191f) /* MUI: V4  ..g LONG          */


/* Structure of the List Position Text (MUIM_List_TestPos) */
struct MUI_List_TestPos_Result
{
    LONG  entry;   /* entry number, maybe -1 if testpos is not over valid entry */
    WORD  column;  /* the number of the column, maybe -1 (unvalid) */
    UWORD flags;   /* some flags, see below */
    WORD  xoffset; /* x offset (in pixels) of testpos relative to the start of the column */
    WORD  yoffset; /* y offset (in pixels) of testpos relative from center of line
	                    ( <0 => testpos was above, >0 => testpos was below center) */
};

#define MUI_LPR_ABOVE (1<<0)
#define MUI_LPR_BELOW (1<<1)
#define MUI_LPR_LEFT  (1<<2)
#define MUI_LPR_RIGHT (1<<3)

enum
{
    MUIV_List_Active_Off = -1,
    MUIV_List_Active_Top = -2,
    MUIV_List_Active_Bottom = -3,
    MUIV_List_Active_Up = -4,
    MUIV_List_Active_Down = -5,
    MUIV_List_Active_PageUp = -6,
    MUIV_List_Active_PageDown = -7,
};

#define MUIV_List_ConstructHook_String (IPTR)-1
#define MUIV_List_CopyHook_String      (IPTR)-1
#define MUIV_List_CursorType_None 0
#define MUIV_List_CursorType_Bar  1
#define MUIV_List_CursorType_Rect 2
#define MUIV_List_DestructHook_String  (IPTR)-1

enum
{
    MUIV_List_Insert_Top    =  0,
    MUIV_List_Insert_Active = -1,
    MUIV_List_Insert_Sorted = -2,
    MUIV_List_Insert_Bottom = -3
};

enum
{
    MUIV_List_Remove_First    =  0,
    MUIV_List_Remove_Active   = -1,
    MUIV_List_Remove_Last     = -2,
    MUIV_List_Remove_Selected = -3,
};

enum
{
    MUIV_List_Select_Active = -1,
    MUIV_List_Select_All    = -2,

    MUIV_List_Select_Off    = 0,
    MUIV_List_Select_On     = 1,
    MUIV_List_Select_Toggle = 2,
    MUIV_List_Select_Ask    = 3,
};

enum
{
    MUIV_List_GetEntry_Active = -1,
};

enum
{
    MUIV_List_Redraw_Active = -1,
    MUIV_List_Redraw_All    = -2,
};

enum
{
    MUIV_List_Move_Top      =  0,
    MUIV_List_Move_Active   = -1,
    MUIV_List_Move_Bottom   = -2,
    MUIV_List_Move_Next     = -3, /* for 2nd parameter only */
    MUIV_List_Move_Previous = -4, /* for 2nd parameter only */
};

enum
{
    MUIV_List_Exchange_Top      =  0,
    MUIV_List_Exchange_Active   = -1,
    MUIV_List_Exchange_Bottom   = -2,
    MUIV_List_Exchange_Next     = -3, /* for 2nd parameter only */
    MUIV_List_Exchange_Previous = -4, /* for 2nd parameter only */
};

enum
{
    MUIV_List_Jump_Top    =  0,
    MUIV_List_Jump_Active = -1,
    MUIV_List_Jump_Bottom = -2,
    MUIV_List_Jump_Down   = -3,
    MUIV_List_Jump_Up     = -4,
};

#define MUIV_List_NextSelected_Start  (-1)
#define MUIV_List_NextSelected_End    (-1)


#define MUIV_NList_SelectChange_Flag_Multi (1 << 0)






/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Scrmodelist "Scrmodelist.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scrmodelist (MUIB_ZUNE | 0x00001700)





#endif /* _MUI_CLASSES_LIST_H */
#endif

#ifndef _MUI_CLASSES_FLOATTEXT_H
#ifndef _MUI_CLASSES_FLOATTEXT_H
#define _MUI_CLASSES_FLOATTEXT_H

/*
    Copyright © 2002-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Floattext           "Floattext.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Floattext           (MUIB_ZUNE | 0x00001500)

/*** Attributes *************************************************************/
#define MUIA_Floattext_Justify   (MUIB_MUI|0x0042dc03) /* MUI: V4  isg BOOL   */
#define MUIA_Floattext_SkipChars (MUIB_MUI|0x00425c7d) /* MUI: V4  is. STRPTR */
#define MUIA_Floattext_TabSize   (MUIB_MUI|0x00427d17) /* MUI: V4  is. LONG   */
#define MUIA_Floattext_Text      (MUIB_MUI|0x0042d16a) /* MUI: V4  isg STRPTR */


#endif /* _MUI_CLASSES_VOLUMELIST_H */
#endif

#ifndef _MUI_CLASSES_POPSTRING_H
#ifndef _MUI_CLASSES_POPSTRING_H
#define _MUI_CLASSES_POPSTRING_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Popstring        "Popstring.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popstring        (MUIB_ZUNE | 0x00002800)  

/*** Methods ****************************************************************/
#define MUIM_Popstring_Close  (MUIB_MUI|0x0042dc52) /* MUI: V7  */
#define MUIM_Popstring_Open   (MUIB_MUI|0x004258ba) /* MUI: V7  */
struct  MUIP_Popstring_Close  {STACKED ULONG MethodID; STACKED LONG result;};
struct  MUIP_Popstring_Open   {STACKED ULONG MethodID; };

/*** Attributes *************************************************************/
#define MUIA_Popstring_Button    (MUIB_MUI|0x0042d0b9) /* MUI: V7  i.g Object *      */
#define MUIA_Popstring_CloseHook (MUIB_MUI|0x004256bf) /* MUI: V7  isg struct Hook * */
#define MUIA_Popstring_OpenHook  (MUIB_MUI|0x00429d00) /* MUI: V7  isg struct Hook * */
#define MUIA_Popstring_String    (MUIB_MUI|0x004239ea) /* MUI: V7  i.g Object *      */
#define MUIA_Popstring_Toggle    (MUIB_MUI|0x00422b7a) /* MUI: V7  isg BOOL          */



#endif /* _MUI_CLASSES_POPSTRING_H */
#endif

#ifndef _MUI_CLASSES_POPOBJECT_H
#ifndef _MUI_CLASSES_POPOBJECT_H
#define _MUI_CLASSES_POPOBJECT_H

/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Popobject              "Popobject.mui"

/*** Identifier base ********************************************************/
#define MUIB_Popobject              (MUIB_ZUNE | 0x00002400)

/*** Attributes *************************************************************/
#define MUIA_Popobject_Follow       (MUIB_MUI|0x00424cb5) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_Light        (MUIB_MUI|0x0042a5a3) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_Object       (MUIB_MUI|0x004293e3) /* MUI: V7  i.g Object *      */
#define MUIA_Popobject_ObjStrHook   (MUIB_MUI|0x0042db44) /* MUI: V7  isg struct Hook * */
#define MUIA_Popobject_StrObjHook   (MUIB_MUI|0x0042fbe1) /* MUI: V7  isg struct Hook * */
#define MUIA_Popobject_Volatile     (MUIB_MUI|0x004252ec) /* MUI: V7  isg BOOL          */
#define MUIA_Popobject_WindowHook   (MUIB_MUI|0x0042f194) /* MUI: V9  isg struct Hook * */



#endif /*_MUI_CLASSES_POPOBJECT_H */
#endif

#ifndef _MUI_CLASSES_CYCLE_H
#ifndef _MUI_CLASSES_CYCLE_H
#define _MUI_CLASSES_CYCLE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Cycle "Cycle.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Cycle         (MUIB_ZUNE | 0x00000a00)

/*** Attributes *************************************************************/
#define MUIA_Cycle_Active  (MUIB_MUI|0x00421788) /* MUI:V4  isg LONG      */
#define MUIA_Cycle_Entries (MUIB_MUI|0x00420629) /* MUI:V4  i.. STRPTR    */

enum
{
    MUIV_Cycle_Active_Next = -1,
    MUIV_Cycle_Active_Prev = -2,
};


#endif /* _MUI_CLASSES_CYCLE_H */
#endif

#ifndef _MUI_CLASSES_GAUGE_H
#ifndef _MUI_CLASSES_GAUGE_H
#define _MUI_CLASSES_GAUGE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Gauge          "Gauge.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Gauge          (MUIB_ZUNE | 0x00000f00)

/*** Attributes *************************************************************/
#define MUIA_Gauge_Current  (MUIB_MUI|0x0042f0dd) /* MUI: V4  isg LONG    */
#define MUIA_Gauge_Divide   (MUIB_MUI|0x0042d8df) /* MUI: V4  isg LONG    */
#define MUIA_Gauge_Horiz    (MUIB_MUI|0x004232dd) /* MUI: V4  i.. BOOL    */
#define MUIA_Gauge_InfoText (MUIB_MUI|0x0042bf15) /* MUI: V7  isg STRPTR  */
#define MUIA_Gauge_Max      (MUIB_MUI|0x0042bcdb) /* MUI: V4  isg LONG    */

#define MUIA_Gauge_DupInfoText (MUIB_Gauge | 0x00000000) /* ZUNE: V1  i.. BOOL - defaults to FALSE */



#endif /* _MUI_CLASSES_GAUGE_H */
#endif

#ifndef _MUI_CLASSES_IMAGE_H
#ifndef _MUI_CLASSES_IMAGE_H
#define _MUI_CLASSES_IMAGE_H

/*
    Copyright  2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Image                  "Image.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Image                  (MUIB_ZUNE | 0x00001300)

/*** Attributes *************************************************************/
#define MUIA_Image_FontMatch        (MUIB_MUI|0x0042815d) /* MUI: V4  i.. BOOL                */
#define MUIA_Image_FontMatchHeight  (MUIB_MUI|0x00429f26) /* MUI: V4  i.. BOOL                */
#define MUIA_Image_FontMatchWidth   (MUIB_MUI|0x004239bf) /* MUI: V4  i.. BOOL                */
#define MUIA_Image_FreeHoriz        (MUIB_MUI|0x0042da84) /* MUI: V4  i.. BOOL                */
#define MUIA_Image_FreeVert         (MUIB_MUI|0x0042ea28) /* MUI: V4  i.. BOOL                */
#define MUIA_Image_OldImage         (MUIB_MUI|0x00424f3d) /* MUI: V4  i.. struct Image *      */
#define MUIA_Image_Spec             (MUIB_MUI|0x004233d5) /* MUI: V4  i.. char *              */
#define MUIA_Image_State            (MUIB_MUI|0x0042a3ad) /* MUI: V4  is. LONG                */



#endif /* _MUI_CLASSES_IMAGE_H */
#endif

#ifndef _MUI_CLASSES_IMAGEDISPLAY_H
#ifndef _MUI_CLASSES_IMAGEDISPLAY_H
#define _MUI_CLASSES_IMAGEDISPLAY_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Imagedisplay           "Imagedisplay.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Imagedisplay           (MUIB_ZUNE | 0x00001200)

/*** Attributes *************************************************************/
#define MUIA_Imagedisplay_Spec       (MUIB_MUI|0x0042a547) /* MUI: V11 isg struct MUI_ImageSpec * */
#define MUIA_Imagedisplay_UseDefSize (MUIB_MUI|0x0042186d) /* MUI: V11 i.. BOOL (undoc) */

#define MUIA_Imagedisplay_FreeHoriz  (MUIB_Imagedisplay | 0x00000000) /* Zune 20030323 i.. BOOL [TRUE] */
#define MUIA_Imagedisplay_FreeVert   (MUIB_Imagedisplay | 0x00000001) /* Zune 20030323 i.. BOOL [TRUE] */



#endif /* _MUI_CLASSES_IMAGEDISPLAY_H */
#endif

#ifndef _MUI_CLASSES_POPASL_H
#ifndef _MUI_CLASSES_POPASL_H
#define _MUI_CLASSES_POPASL_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Popasl             "Popasl.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popasl            (MUIB_ZUNE | 0x00002100)


/*** Attributes *************************************************************/
#define MUIA_Popasl_Active     (MUIB_MUI|0x00421b37) /* MUI: V7  ..g BOOL          */
#define MUIA_Popasl_StartHook  (MUIB_MUI|0x0042b703) /* MUI: V7  isg struct Hook * */
#define MUIA_Popasl_StopHook   (MUIB_MUI|0x0042d8d2) /* MUI: V7  isg struct Hook * */
#define MUIA_Popasl_Type       (MUIB_MUI|0x0042df3d) /* MUI: V7  i.g ULONG         */



#endif /* _MUI_CLASSES_POPASL_H */
#endif

#ifndef _MUI_CLASSES_SETTINGSGROUP_H
#ifndef _MUI_CLASSES_SETTINGSGROUP_H
#define _MUI_CLASSES_SETTINGSGROUP_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Settingsgroup                 "Settingsgroup.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Settingsgroup                 (MUIB_ZUNE | 0x00003100)  

/*** Methods ****************************************************************/
#define MUIM_Settingsgroup_ConfigToGadgets (MUIB_MUI|0x00427043) /* MUI: V11 */
#define MUIM_Settingsgroup_GadgetsToConfig (MUIB_MUI|0x00425242) /* MUI: V11 */
struct MUIP_Settingsgroup_ConfigToGadgets  {STACKED ULONG MethodID; STACKED Object *configdata; };
struct MUIP_Settingsgroup_GadgetsToConfig  {STACKED ULONG MethodID; STACKED Object *configdata; };



#endif /* _MUI_CLASSES_SETTINGSGROUP_H */
#endif

#ifndef _MUI_CLASSES_SETTINGS_H
#ifndef _MUI_CLASSES_SETTINGS_H
#define _MUI_CLASSES_SETTINGS_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Settings  "Settings.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Settings  (MUIB_ZUNE | 0x00003200)



#endif /* _MUI_CLASSES_SETTINGS_H */
#endif

#ifndef _MUI_CLASSES_ABOUTMUI_H
/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_ABOUTMUI_H
#define _MUI_CLASSES_ABOUTMUI_H

/*** Name *******************************************************************/
#define MUIC_Aboutmui "Aboutmui.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Aboutmui (MUIB_ZUNE | 0x00000000)  

/*** Attributes *************************************************************/
#define MUIA_Aboutmui_Application (MUIB_MUI | 0x00422523) /* V11 i.. Object * */



#endif /* _MUI_CLASSES_ABOUTMUI_H */
#endif

#ifndef _MUI_CLASSES_CONFIGDATA_H
/*
    Copyright  2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_CONFIGDATA_H
#define _MUI_CLASSES_CONFIGDATA_H

/*** Name *******************************************************************/
#define MUIC_Configdata  "Configdata.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Configdata  (MUIB_ZUNE | 0x00000900)

/* The config items for MUIM_GetConfigItem */
#define MUICFG_Invalid                  (-1)
#define MUICFG_Window_Spacing_Left      0x01  /* ULONG, horiz pixels (def=4) */
#define MUICFG_Window_Spacing_Right     0x02  /* ULONG, horiz pixels (def=4) */
#define MUICFG_Window_Spacing_Top       0x03  /* ULONG, vert pixels (def=3) */
#define MUICFG_Window_Spacing_Bottom    0x04  /* ULONG, vert pixels (def=3) */
#define MUICFG_Radio_HSpacing           0x05  /* ULONG, horiz pixels (def=4) */
#define MUICFG_Radio_VSpacing           0x06  /* ULONG, vertical pixels (def=1) */
#define MUICFG_Group_HSpacing           0x07  /* ULONG, horiz pixels (def=6) */
#define MUICFG_Group_VSpacing           0x08  /* ULONG, vertical pixels (def=3) */
#define MUICFG_Scrollbar_Arrangement    0x09  /* ULONG, top = 0 (def), middle, bottom */
#define MUICFG_Listview_Refresh         0x0a  /* ULONG, linear, mixed = 1 (def) */
#define MUICFG_Listview_Font_Leading    0x0b  /* ULONG, vertical pixels (def=1) */
#define MUICFG_Listview_SmoothVal       0x0c  /* ULONG, ? (def=0) */
#define MUICFG_Listview_Multi           0x0d  /* ULONG, shifted = 0 (def), always */
#define MUICFG_GroupTitle_Position      0x0f  /* ULONG, 1=centered */
#define MUICFG_GroupTitle_Color         0x10  /* ULONG, 0=normal */
#define MUICFG_Cycle_MenuCtrl_Level     0x11  /* ULONG, num of entries (def=2) */
#define MUICFG_Cycle_MenuCtrl_Position  0x12  /* ULONG, below = 0 (def), on active */
#define MUICFG_Frame_Drag               0x18
#define MUICFG_Cycle_Menu_Recessed      0x19  /* ULONG, false = 0 (def), true */
#define MUICFG_Cycle_MenuCtrl_Speed     0x1a  /* ULONG, num of ticks (0..50) (def=0) */
#define MUICFG_Listview_Smoothed        0x1b  /* ULONG, false = 0 (def), true */
#define MUICFG_Window_Redraw            0x1d  /* ULONG, no clear, clear = 1 (def) */
#define MUICFG_Font_Normal              0x1e
#define MUICFG_Font_List                0x1f
#define MUICFG_Font_Tiny                0x20
#define MUICFG_Font_Fixed               0x21
#define MUICFG_Font_Title               0x22
#define MUICFG_Font_Big	                0x23
#define MUICFG_PublicScreen             0x24
#define MUICFG_Frame_Button             0x2b
#define MUICFG_Frame_ImageButton        0x2c
#define MUICFG_Frame_Text               0x2d
#define MUICFG_Frame_String             0x2e
#define MUICFG_Frame_ReadList           0x2f
#define MUICFG_Frame_InputList          0x30
#define MUICFG_Frame_Prop               0x31
#define MUICFG_Frame_Gauge              0x32
#define MUICFG_Frame_Group              0x33
#define MUICFG_Frame_PopUp              0x34
#define MUICFG_Frame_Virtual            0x35
#define MUICFG_Frame_Slider             0x36
#define MUICFG_Background_Window        0x37
#define MUICFG_Background_Requester     0x38
#define MUICFG_Background_Button        0x39
#define MUICFG_Background_List          0x3a
#define MUICFG_Background_Text          0x3b
#define MUICFG_Background_Prop          0x3c
#define MUICFG_Background_PopUp         0x3d
#define MUICFG_Background_Selected      0x3e
#define MUICFG_Background_ListCursor    0x3f
#define MUICFG_Background_ListSelect    0x40
#define MUICFG_Background_ListSelCur    0x41
#define MUICFG_Image_ArrowUp            0x42
#define MUICFG_Image_ArrowDown          0x43
#define MUICFG_Image_ArrowLeft          0x44
#define MUICFG_Image_ArrowRight         0x45
#define MUICFG_Image_CheckMark          0x46
#define MUICFG_Image_RadioButton        0x47
#define MUICFG_Image_Cycle              0x48
#define MUICFG_Image_PopUp              0x49
#define MUICFG_Image_PopFile            0x4a
#define MUICFG_Image_PopDrawer          0x4b
#define MUICFG_Image_PropKnob           0x4c
#define MUICFG_Image_Drawer             0x4d
#define MUICFG_Image_HardDisk           0x4e
#define MUICFG_Image_Disk               0x4f
#define MUICFG_Image_Chip               0x50
#define MUICFG_Image_Volume             0x51
#define MUICFG_Image_Network            0x52
#define MUICFG_Image_Assign             0x53
#define MUICFG_Background_Register      0x54
#define MUICFG_Image_TapePlay           0x55
#define MUICFG_Image_TapePlayBack       0x56
#define MUICFG_Image_TapePause          0x57
#define MUICFG_Image_TapeStop           0x58
#define MUICFG_Image_TapeRecord         0x59
#define MUICFG_Background_Framed        0x5a
#define MUICFG_Background_Slider        0x5b
#define MUICFG_Background_SliderKnob    0x5c
#define MUICFG_Image_TapeUp             0x5d
#define MUICFG_Image_TapeDown           0x5e
#define MUICFG_Keyboard_Press           0x5f
#define MUICFG_Keyboard_Toggle          0x60
#define MUICFG_Keyboard_Up              0x61
#define MUICFG_Keyboard_Down            0x62
#define MUICFG_Keyboard_PageUp          0x63
#define MUICFG_Keyboard_PageDown        0x64
#define MUICFG_Keyboard_Top             0x65
#define MUICFG_Keyboard_Bottom          0x66
#define MUICFG_Keyboard_Left            0x67
#define MUICFG_Keyboard_Right           0x68
#define MUICFG_Keyboard_WordLeft        0x69
#define MUICFG_Keyboard_WordRight       0x6a
#define MUICFG_Keyboard_LineStart       0x6b
#define MUICFG_Keyboard_LineEnd         0x6c
#define MUICFG_Keyboard_NextGadget      0x6d
#define MUICFG_Keyboard_PrevGadget      0x6e
#define MUICFG_Keyboard_GadgetOff       0x6f
#define MUICFG_Keyboard_CloseWindow     0x70
#define MUICFG_Keyboard_NextWindow      0x71
#define MUICFG_Keyboard_PrevWindow      0x72
#define MUICFG_Keyboard_Help            0x73
#define MUICFG_Keyboard_Popup           0x74
#define MUICFG_Window_Positions         0x7a
#define MUICFG_Balance_Look             0x7b /* ULONG, frame = 0 (def), object */
#define MUICFG_Font_Button              0x80
#define MUICFG_Scrollbar_Type           0x83 /* ULONG, standard = 0 (def), newlook, custom */
#define MUICFG_String_Background        0x84
#define MUICFG_String_Text              0x85
#define MUICFG_String_ActiveBackground  0x86
#define MUICFG_String_ActiveText        0x87
#define MUICFG_Font_Knob                0x88
#define MUICFG_Drag_LeftButton          0x89 /* ULONG, false, true (def) */
#define MUICFG_Drag_MiddleButton        0x8a /* ULONG, false (def), true */
#define MUICFG_Drag_LMBModifier         0x8b /* key desc (def = control) */
#define MUICFG_Drag_MMBModifier         0x8c /* key desc */
#define MUICFG_Drag_Autostart           0x8d /* ULONG, false = 0, true (def) */
#define MUICFG_Drag_Autostart_Length    0x8e /* ULONG, pixels (def = 3) */
#define MUICFG_ActiveObject_Color       0x8f /* penspec */
#define MUICFG_Frame_Knob               0x90
#define MUICFG_Dragndrop_Look           0x94 /* ULONG, solid, ghosted on obj (def), ... */
#define MUICFG_Background_Page          0x95
#define MUICFG_Background_ReadList      0x96
#define MUICFG_String_Cursor            0x400
#define MUICFG_String_MarkedBackground  0x401
#define MUICFG_String_MarkedText        0x402
#define MUICFG_Register_TruncateTitles  0x403
#define MUICFG_Window_Refresh	    	0x404
#define MUICFG_Screen_Mode              0x505
#define MUICFG_Screen_Mode_ID           0x506
#define MUICFG_Screen_Width             0x507
#define MUICFG_Screen_Height            0x508
#define MUICFG_WindowPos                0x509
#define MUICFG_Window_Buttons           0x50a

#define MUICFG_CustomFrame_1            0x600
#define MUICFG_CustomFrame_2            0x601
#define MUICFG_CustomFrame_3            0x602
#define MUICFG_CustomFrame_4            0x603
#define MUICFG_CustomFrame_5            0x604
#define MUICFG_CustomFrame_6            0x605
#define MUICFG_CustomFrame_7            0x606
#define MUICFG_CustomFrame_8            0x607
#define MUICFG_CustomFrame_9            0x608
#define MUICFG_CustomFrame_10           0x609
#define MUICFG_CustomFrame_11           0x60a
#define MUICFG_CustomFrame_12           0x60b
#define MUICFG_CustomFrame_13           0x60c
#define MUICFG_CustomFrame_14           0x60d
#define MUICFG_CustomFrame_15           0x60e
#define MUICFG_CustomFrame_16           0x60f

#define MUICFG_PublicScreen_PopToFront  0x700
#define MUICFG_Iconification_Hotkey     0x701
#define MUICFG_Iconification_ShowIcon   0x702
#define MUICFG_Iconification_ShowMenu   0x703
#define MUICFG_Iconification_OnStartup  0x704
#define MUICFG_Interfaces_EnableARexx   0x705
#define MUICFG_BubbleHelp_FirstDelay    0x706
#define MUICFG_BubbleHelp_NextDelay     0x707

#define MUIM_Configdata_GetWindowPos    (MUIB_Configdata | 0x0000002A)
#define MUIM_Configdata_SetWindowPos    (MUIB_Configdata | 0x0000002B)


/*** Methods ****************************************************************/
#define MUIM_Configdata_GetString      (MUIB_Configdata | 0x00000000) /* Zune 20030319 */
#define MUIM_Configdata_GetULong       (MUIB_Configdata | 0x00000001) /* Zune 20030319 */
#define MUIM_Configdata_SetULong       (MUIB_Configdata | 0x00000002) /* Zune 20030320 */
#define MUIM_Configdata_SetImspec      (MUIB_Configdata | 0x00000003) /* Zune 20030323 */
#define MUIM_Configdata_SetFramespec   (MUIB_Configdata | 0x00000004) /* Zune 20030331 */
#define MUIM_Configdata_SetFont        (MUIB_Configdata | 0x00000005) /* Zune 20030323 */
#define MUIM_Configdata_Save           (MUIB_Configdata | 0x00000006) /* Zune 20030320 */
#define MUIM_Configdata_Load           (MUIB_Configdata | 0x00000007) /* Zune 20030320 */
#define MUIM_Configdata_SetPenspec     (MUIB_Configdata | 0x00000008) /* Zune 20030714 */
#define MUIM_Configdata_SetString      (MUIB_Configdata | 0x00000009) /* Zune 20030808 */
struct MUIP_Configdata_GetString       {STACKED ULONG MethodID; STACKED ULONG id; };
struct MUIP_Configdata_GetULong        {STACKED ULONG MethodID; STACKED ULONG id; };
struct MUIP_Configdata_SetULong        {STACKED ULONG MethodID; STACKED ULONG id; STACKED ULONG val; };
struct MUIP_Configdata_SetImspec       {STACKED ULONG MethodID; STACKED ULONG id; STACKED CONST_STRPTR imspec; };
struct MUIP_Configdata_SetFramespec    {STACKED ULONG MethodID; STACKED ULONG id; STACKED CONST_STRPTR framespec; };
struct MUIP_Configdata_SetFont         {STACKED ULONG MethodID; STACKED ULONG id; STACKED CONST_STRPTR font; };
struct MUIP_Configdata_Save            {STACKED ULONG MethodID; STACKED CONST_STRPTR filename; };
struct MUIP_Configdata_Load            {STACKED ULONG MethodID; STACKED CONST_STRPTR filename; };
struct MUIP_Configdata_SetPenspec      {STACKED ULONG MethodID; STACKED ULONG id; STACKED CONST_STRPTR penspec; };
struct MUIP_Configdata_SetString       {STACKED ULONG MethodID; STACKED ULONG id; STACKED CONST_STRPTR string; };

/*** Attributes *************************************************************/
#define MUIA_Configdata_Application     (MUIB_Configdata | 0x00000000) /* ZV1: i..  Object * */
#define MUIA_Configdata_ApplicationBase (MUIB_Configdata | 0x00000002) /* ZV1: i..  Object * */



#endif /* _MUI_CLASSES_CONFIGDATA_H */
#endif

#ifndef _MUI_CLASSES_IMAGEADJUST_H
#ifndef _MUI_CLASSES_IMAGEADJUST_H
#define _MUI_CLASSES_IMAGEADJUST_H

/* 
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Imageadjust      "Imageadjust.mui"

/*** Identifier base (for AROS extensions) **********************************/
#define MUIB_Imageadjust      (MUIB_ZUNE | 0x00001100)  

/*** Attributes *************************************************************/
#define MUIA_Imageadjust_Type (MUIB_MUI|0x00422f2b) /* MUI: V11 i.. LONG */
#define MUIA_Imageadjust_Spec (MUIB_MUI|0x004279e1) /* MUI: ??? .g. char * */
#define MUIA_Imageadjust_Originator (MUIB_Imageadjust|0x00000000) /* Zune: i.. Object * */

enum
{
    MUIV_Imageadjust_Type_All = 0,
    MUIV_Imageadjust_Type_Image,
    MUIV_Imageadjust_Type_Background,
    MUIV_Imageadjust_Type_Pen,
};

/*** Methods ****************************************************************/


#endif /* _MUI_CLASSES_IMAGEADJUST_H */
#endif

#ifndef _MUI_CLASSES_POPIMAGE_H
#ifndef _MUI_CLASSES_POPIMAGE_H
#define _MUI_CLASSES_POPIMAGE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Popimage              "Popimage.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popimage              (MUIB_ZUNE | 0x00002300)  

/*** Methods ****************************************************************/



#endif /* _MUI_CLASSES_POPIMAGE_H */
#endif

#ifndef _MUI_CLASSES_SCALE_H
#ifndef _MUI_CLASSES_SCALE_H
#define _MUI_CLASSES_SCALE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Scale          "Scale.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Scale          (MUIB_ZUNE | 0x00002d00)  

/*** Attributes *************************************************************/
#define MUIA_Scale_Horiz    (MUIB_MUI|0x0042919a) /* MUI: V4  isg BOOL    */



#endif /* _MUI_CLASSES_SCALE_H */
#endif

#ifndef _MUI_CLASSES_RADIO_H
#ifndef _MUI_CLASSES_RADIO_H
#define _MUI_CLASSES_RADIO_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Radio         "Radio.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Radio         (MUIB_ZUNE | 0x00002a00)  

/*** Attributes *************************************************************/
#define MUIA_Radio_Active  (MUIB_MUI|0x00429b41) /* MUI:V4  isg LONG      */
#define MUIA_Radio_Entries (MUIB_MUI|0x0042b6a1) /* MUI:V4  i.. STRPTR *  */



#endif /* _MUI_CLASSES_RADIO_H */
#endif

#ifndef _MUI_CLASSES_BALANCE_H
/* 
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_BALANCE_H
#define _MUI_CLASSES_BALANCE_H

/*** Name *******************************************************************/
#define MUIC_Balance        "Balance.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Balance        (MUIB_ZUNE | 0x00000300)

/*** Attributes *************************************************************/
#define MUIA_Balance_Quiet  (MUIB_Balance | 0x00000000) /* (zune) V20 i   LONG */



#endif /* _MUI_CLASSES_BALANCE_H */
#endif

#ifndef _MUI_CLASSES_PENDISPLAY_H
#ifndef _MUI_CLASSES_PENDISPLAY_H
#define _MUI_CLASSES_PENDISPLAY_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Pendisplay              "Pendisplay.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Pendisplay              (MUIB_ZUNE | 0x00002000)  

/*** Methods ****************************************************************/
#define MUIM_Pendisplay_SetColormap  (MUIB_MUI|0x004243a7) /* MUI: V13 */
#define MUIM_Pendisplay_SetMUIPen    (MUIB_MUI|0x00426ecd) /* MUI: V13 */
#define MUIM_Pendisplay_SetRGB       (MUIB_MUI|0x0042032c) /* MUI: V13 */
struct MUIP_Pendisplay_SetColormap   {STACKED ULONG MethodID; STACKED LONG colormap;};
struct MUIP_Pendisplay_SetMUIPen     {STACKED ULONG MethodID; STACKED LONG muipen;};
struct MUIP_Pendisplay_SetRGB        {STACKED ULONG MethodID; STACKED ULONG r; STACKED ULONG g; STACKED ULONG b;};

/*** Attributes *************************************************************/
#define MUIA_Pendisplay_Pen        (MUIB_MUI|0x0042a748) /* MUI: V13  ..g Object *       */
#define MUIA_Pendisplay_Reference  (MUIB_MUI|0x0042dc24) /* MUI: V13  isg Object *       */
#define MUIA_Pendisplay_RGBcolor   (MUIB_MUI|0x0042a1a9) /* MUI: V11  isg struct MUI_RGBcolor * */
#define MUIA_Pendisplay_Spec       (MUIB_MUI|0x0042a204) /* MUI: V11  isg struct MUI_PenSpec  * */



#endif /* _MUI_CLASSES_PENDISPLAY_H */
#endif

#ifndef _MUI_CLASSES_PENADJUST_H
#ifndef _MUI_CLASSES_PENADJUST_H
#define _MUI_CLASSES_PENADJUST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Penadjust "Penadjust.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Penadjust         (MUIB_ZUNE | 0x00001f00)  

/*** Attributes *************************************************************/
#define MUIA_Penadjust_PSIMode (MUIB_MUI|0x00421cbb) /* MUI: V11  i.. BOOL       */




#endif /* _MUI_CLASSES_PENADJUST_H */
#endif

#ifndef _MUI_CLASSES_POPPEN_H
#ifndef _MUI_CLASSES_POPPEN_H
#define _MUI_CLASSES_POPPEN_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Poppen              "Poppen.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Poppen              (MUIB_ZUNE | 0x00002700)

/*** Methods ****************************************************************/



#endif /* _MUI_CLASSES_POPPEN_H */
#endif

#ifndef _MUI_CLASSES_COLORFIELD_H
/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_COLORFIELD_H
#define _MUI_CLASSES_COLORFIELD_H

/*** Name *******************************************************************/
#define MUIC_Colorfield "Colorfield.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Colorfield         (MUIB_ZUNE | 0x00000800)

/*** Attributes *************************************************************/
#define MUIA_Colorfield_Pen 	(MUIB_MUI|0x0042713a) /* ..g  ULONG   */
#define MUIA_Colorfield_Red 	(MUIB_MUI|0x004279f6) /* isg  ULONG   */
#define MUIA_Colorfield_Green	(MUIB_MUI|0x00424466) /* isg  ULONG   */
#define MUIA_Colorfield_Blue 	(MUIB_MUI|0x0042d3b0) /* isg  ULONG   */
#define MUIA_Colorfield_RGB 	(MUIB_MUI|0x0042677a) /* isg  ULONG * */



#endif /* _MUI_CLASSES_COLORFIELD_H */
#endif

#ifndef _MUI_CLASSES_COLORADJUST_H
/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_COLORADJUST_H
#define _MUI_CLASSES_COLORADJUST_H

/*** Name *******************************************************************/
#define MUIC_Coloradjust        "Coloradjust.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Coloradjust        (MUIB_ZUNE | 0x00000700)

/*** Attributes *************************************************************/
#define MUIA_Coloradjust_Red 	(MUIB_MUI|0x00420eaa) /* isg ULONG   */
#define MUIA_Coloradjust_Green	(MUIB_MUI|0x004285ab) /* isg ULONG   */
#define MUIA_Coloradjust_Blue 	(MUIB_MUI|0x0042b8a3) /* isg ULONG   */
#define MUIA_Coloradjust_RGB 	(MUIB_MUI|0x0042f899) /* isg ULONG * */
#define MUIA_Coloradjust_ModeID (MUIB_MUI|0x0042ec59) /* isg ULONG   */



#endif /* _MUI_CLASSES_COLORADJUST_H */
#endif

#ifndef _MUI_CLASSES_MCCPREFS_H
/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef _MUI_CLASSES_MCCPREFS_H
#define _MUI_CLASSES_MCCPREFS_H

#define MUIC_Mccprefs "Mccprefs.mui"


#endif
#endif

#ifndef _MUI_CLASSES_FRAMEADJUST_H
#ifndef _MUI_CLASSES_FRAMEADJUST_H
#define _MUI_CLASSES_FRAMEADJUST_H

/* 
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Frameadjust "Frameadjust.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Frameadjust      (MUIB_ZUNE | 0x00000d00)

/*** Attributes *************************************************************/
#define MUIA_Frameadjust_Spec (MUIB_Frameadjust | 0x00000000) /* Zune 20030330 ig. CONST_STRPTR */



#endif /* _MUI_CLASSES_FRAMEADJUST_H */
#endif

#ifndef _MUI_CLASSES_FRAMEDISPLAY_H
#ifndef _MUI_CLASSES_FRAMEDISPLAY_H
#define _MUI_CLASSES_FRAMEDISPLAY_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Framedisplay      "Framedisplay.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Framedisplay      (MUIB_ZUNE | 0x00000e00)

/*** Attributes *************************************************************/
#define MUIA_Framedisplay_Spec (MUIB_MUI|0x00421794) /* MUI: V??  isg struct MUI_FrameSpec  * */


#endif /* _MUI_CLASSES_FRAMEDISPLAY_H */
#endif

#ifndef _MUI_CLASSES_POPFRAME_H
#ifndef _MUI_CLASSES_POPFRAME_H
#define _MUI_CLASSES_POPFRAME_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Popframe              "Popframe.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popframe              (MUIB_ZUNE | 0x00002200)

/*** Methods ****************************************************************/



#endif /* _MUI_CLASSES_POPFRAME_H */
#endif

#ifndef _MUI_CLASSES_VOLUMELIST_H
#ifndef _MUI_CLASSES_VOLUMELIST_H
#define _MUI_CLASSES_VOLUMELIST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Volumelist              "Volumelist.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Volumelist              (MUIB_ZUNE | 0x1600)  


#endif /* _MUI_CLASSES_VOLUMELIST_H */
#endif

#ifndef _MUI_CLASSES_DIRLIST_H
#ifndef _MUI_CLASSES_DIRLIST_H
#define _MUI_CLASSES_DIRLIST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Dirlist "Dirlist.mui"

/*** Identifer base (for Zune extensions) ***********************************/
#define MUIB_Dirlist (MUIB_ZUNE | 0x00001800)  

/*** Methods ****************************************************************/

#define MUIM_Dirlist_ReRead         (MUIB_MUI|0x00422d71) /* MUI: V4  */
struct  MUIP_Dirlist_ReRead         {STACKED ULONG MethodID;};

/*** Attributes *************************************************************/
#define MUIA_Dirlist_AcceptPattern  (MUIB_MUI|0x0042760a) /* MUI: V4  is. STRPTR        */
#define MUIA_Dirlist_Directory      (MUIB_MUI|0x0042ea41) /* MUI: V4  isg STRPTR        */
#define MUIA_Dirlist_DrawersOnly    (MUIB_MUI|0x0042b379) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_FilesOnly      (MUIB_MUI|0x0042896a) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_FilterDrawers  (MUIB_MUI|0x00424ad2) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_FilterHook     (MUIB_MUI|0x0042ae19) /* MUI: V4  is. struct Hook * */
#define MUIA_Dirlist_MultiSelDirs   (MUIB_MUI|0x00428653) /* MUI: V6  is. BOOL          */
#define MUIA_Dirlist_NumBytes       (MUIB_MUI|0x00429e26) /* MUI: V4  ..g LONG          */
#define MUIA_Dirlist_NumDrawers     (MUIB_MUI|0x00429cb8) /* MUI: V4  ..g LONG          */
#define MUIA_Dirlist_NumFiles       (MUIB_MUI|0x0042a6f0) /* MUI: V4  ..g LONG          */
#define MUIA_Dirlist_Path           (MUIB_MUI|0x00426176) /* MUI: V4  ..g STRPTR        */
#define MUIA_Dirlist_RejectIcons    (MUIB_MUI|0x00424808) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_RejectPattern  (MUIB_MUI|0x004259c7) /* MUI: V4  is. STRPTR        */
#define MUIA_Dirlist_SortDirs       (MUIB_MUI|0x0042bbb9) /* MUI: V4  is. LONG          */
#define MUIA_Dirlist_SortHighLow    (MUIB_MUI|0x00421896) /* MUI: V4  is. BOOL          */
#define MUIA_Dirlist_SortType       (MUIB_MUI|0x004228bc) /* MUI: V4  is. LONG          */
#define MUIA_Dirlist_Status         (MUIB_MUI|0x004240de) /* MUI: V4  ..g LONG          */

enum {
    MUIV_Dirlist_SortDirs_First = 0,
    MUIV_Dirlist_SortDirs_Last,
    MUIV_Dirlist_SortDirs_Mix,
};

enum {
    MUIV_Dirlist_SortType_Name = 0,
    MUIV_Dirlist_SortType_Date,
    MUIV_Dirlist_SortType_Size,
};

enum {
    MUIV_Dirlist_Status_Invalid = 0,
    MUIV_Dirlist_Status_Reading,
    MUIV_Dirlist_Status_Valid,
};


#endif /* _MUI_CLASSES_DIRLIST_H */
#endif

#ifndef _MUI_CLASSES_NUMERICBUTTON_H
#ifndef _MUI_CLASSES_NUMERICBUTTON_H
#define _MUI_CLASSES_NUMERICBUTTON_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Numericbutton              "Numericbutton.mui"

/*** Identifier base (for Zune extensions) **********************************/
//#define MUIB_Numericbutton              (MUIB_ZUNE | 0x????)  


#endif /* _MUI_CLASSES_NUMERICBUTTON_H */
#endif

#ifndef _MUI_CLASSES_POPLIST_H
#ifndef _MUI_CLASSES_POPLIST_H
#define _MUI_CLASSES_POPLIST_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Poplist                "Poplist.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Poplist                (MUIB_ZUNE | 0x00002500)

/*** Attributes *************************************************************/
#define MUIA_Poplist_Array          (MUIB_MUI|0x0042084c) /* MUI: V8  i.. char ** */


#endif /* _MUI_CLASSES_POPLIST_H */
#endif

#ifndef _MUI_CLASSES_POPSCREEN_H
#ifndef _MUI_CLASSES_POPSCREEN_H
#define _MUI_CLASSES_POPSCREEN_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Popscreen              "Popscreen.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Popscreen              (MUIB_ZUNE | 0x00002600)



#endif /* _MUI_CLASSES_POPSCREEN_H */
#endif

#ifndef _MUI_CLASSES_CRAWLING_H
#ifndef _MUI_CLASSES_CRAWLING_H
#define _MUI_CLASSES_CRAWLING_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/****************************************************************************/
/*** Name *******************************************************************/
#define MUIC_Crawling                "Crawling.mcc"

/*** Identifier base (for Zune extensions) **********************************/
//#define MUIB_Crawling                (MUIB_ZUNE | 0x0000????)


#endif /* _MUI_CLASSES_CRAWLING_H */
#endif

#ifndef _MUI_CLASSES_LEVELMETER_H
#ifndef _MUI_CLASSES_LEVELMETER_H
#define _MUI_CLASSES_LEVELMETER_H

/*
    Copyright © 2002-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Levelmeter  	"Levelmeter.mui"

/*** Identifier base (for Zune extensions) **********************************/
//#define MUIB_Levelmeter   	(MUIB_ZUNE | 0x????)  

/*** Attributes *************************************************************/
#define MUIA_Levelmeter_Label	(MUIB_MUI | 0x00420dd5) /* V11 isg STRPTR */



#endif /* _MUI_CLASSES_LEVELMETER_H */
#endif

#ifndef _MUI_CLASSES_KNOB_H
#ifndef _MUI_CLASSES_KNOB_H
#define _MUI_CLASSES_KNOB_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Knob  	"Knob.mui"

/*** Identifier base (for Zune extensions) **********************************/
//#define MUIB_Knob   	(MUIB_ZUNE | 0x????)  


#endif /* _MUI_CLASSES_KNOB_H */
#endif

#ifndef _MUI_CLASSES_DTPIC_H
#ifndef _MUI_CLASSES_DTPIC_H
#define _MUI_CLASSES_DTPIC_H

/*
    Copyright © 2002-2009, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Dtpic          "Dtpic.mui"

/*** Attributes *************************************************************/
#define MUIA_Dtpic_Name     (MUIB_MUI|0x00423d72) /* MUI: V18 isg STRPTR */


#endif /* _MUI_CLASSES_DTPIC_H */
#endif

#ifndef _MUI_CLASSES_PALETTE_H
#ifndef _MUI_CLASSES_PALETTE_H
#define _MUI_CLASSES_PALETTE_H

/*
    Copyright © 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*** Name *******************************************************************/
#define MUIC_Palette "Palette.mui"

/*** Identifier base (for Zune extensions) **********************************/
#define MUIB_Palette         (MUIB_ZUNE | 0x00008a00)

/*** Attributes *************************************************************/
#define MUIA_Palette_Entries                0x8042a3d8 /* V6  i.g struct MUI_Palette_Entry * */
#define MUIA_Palette_Groupable              0x80423e67 /* V6  isg BOOL              */
#define MUIA_Palette_Names                  0x8042c3a2 /* V6  isg char **           */

#define MUIV_Palette_Entry_End -1

struct MUI_Palette_Entry
{
    LONG  mpe_ID;
    ULONG mpe_Red;
    ULONG mpe_Green;
    ULONG mpe_Blue;
    LONG  mpe_Group;
};


#endif /* _MUI_PALETTE_H */
#endif

/**************************************************************************
 Zune/MUI Image and Background definition
**************************************************************************/
/* configured by the user within the prefs programm */
#define MUII_WindowBack        0UL
#define MUII_RequesterBack     1UL
#define MUII_ButtonBack        2UL
#define MUII_ListBack          3UL
#define MUII_TextBack          4UL
#define MUII_PropBack          5UL
#define MUII_PopupBack         6UL
#define MUII_SelectedBack      7UL
#define MUII_ListCursor        8UL
#define MUII_ListSelect        9UL
#define MUII_ListSelCur        10UL
#define MUII_ArrowUp           11UL
#define MUII_ArrowDown         12UL
#define MUII_ArrowLeft         13UL
#define MUII_ArrowRight        14UL
#define MUII_CheckMark         15UL
#define MUII_RadioButton       16UL
#define MUII_Cycle             17UL
#define MUII_PopUp             18UL
#define MUII_PopFile           19UL
#define MUII_PopDrawer         20UL
#define MUII_PropKnob          21UL
#define MUII_Drawer            22UL
#define MUII_HardDisk          23UL
#define MUII_Disk              24UL
#define MUII_Chip              25UL
#define MUII_Volume            26UL
#define MUII_RegisterBack      27UL
#define MUII_Network           28UL
#define MUII_Assign            29UL
#define MUII_TapePlay          30UL
#define MUII_TapePlayBack      31UL
#define MUII_TapePause         32UL
#define MUII_TapeStop          33UL
#define MUII_TapeRecord        34UL
#define MUII_GroupBack         35UL
#define MUII_SliderBack        36UL
#define MUII_SliderKnob        37UL
#define MUII_TapeUp            38UL
#define MUII_TapeDown          39UL
#define MUII_PageBack          40UL
#define MUII_ReadListBack      41UL
#define MUII_Count             42UL

/* direct color's and combinations */
#define MUII_BACKGROUND        128UL
#define MUII_SHADOW            129UL
#define MUII_SHINE             130UL
#define MUII_FILL              131UL
#define MUII_SHADOWBACK        132UL
#define MUII_SHADOWFILL        133UL
#define MUII_SHADOWSHINE       134UL
#define MUII_FILLBACK          135UL
#define MUII_FILLSHINE         136UL
#define MUII_SHINEBACK         137UL
#define MUII_FILLBACK2         138UL
#define MUII_HSHINEBACK        139UL
#define MUII_HSHADOWBACK       140UL
#define MUII_HSHINESHINE       141UL
#define MUII_HSHADOWSHADOW     142UL
#define MUII_MARKSHINE         143UL
#define MUII_MARKHALFSHINE     144UL
#define MUII_MARKBACKGROUND    145UL
#define MUII_LASTPAT           146UL


/**************************************************************************
 For ARexx
**************************************************************************/
struct MUI_Command
{
    char        *mc_Name;
    char        *mc_Template;
    LONG         mc_Parameters;
    struct Hook *mc_Hook;
    LONG         mc_Reserved[5];
};

#define MC_TEMPLATE_ID ((STRPTR)~0)

#define MUI_RXERR_BADDEFINITION  -1
#define MUI_RXERR_OUTOFMEMORY    -2
#define MUI_RXERR_UNKNOWNCOMMAND -3
#define MUI_RXERR_BADSYNTAX      -4

#ifndef _MUI_MACROS_H
#ifndef _MUI_MACROS_H
#define _MUI_MACROS_H

/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$

    Macros available in original MUI and also some additional ones.
*/

/* Some nice macrodefinitions for creating your object tree */
#define MenustripObject     MUIOBJMACRO_START(MUIC_Menustrip)
#define MenuObject          MUIOBJMACRO_START(MUIC_Menu)
#define MenuObjectT(name)   MUIOBJMACRO_START(MUIC_Menu), MUIA_Menu_Title, name
#define MenuitemObject      MUIOBJMACRO_START(MUIC_Menuitem)
#define WindowObject        MUIOBJMACRO_START(MUIC_Window)
#define ImageObject         MUIOBJMACRO_START(MUIC_Image)
#define ImagedisplayObject  MUIOBJMACRO_START(MUIC_Imagedisplay)
#define BitmapObject        MUIOBJMACRO_START(MUIC_Bitmap)
#define BodychunkObject     MUIOBJMACRO_START(MUIC_Bodychunk)
#define ChunkyImageObject   MUIOBJMACRO_START(MUIC_ChunkyImage)
#define NotifyObject        MUIOBJMACRO_START(MUIC_Notify)
#define ApplicationObject   MUIOBJMACRO_START(MUIC_Application)
#define TextObject          MUIOBJMACRO_START(MUIC_Text)
#define RectangleObject     MUIOBJMACRO_START(MUIC_Rectangle)
#define BalanceObject       MUIOBJMACRO_START(MUIC_Balance)
#define ListObject          MUIOBJMACRO_START(MUIC_List)
#define PropObject          MUIOBJMACRO_START(MUIC_Prop)
#define StringObject        MUIOBJMACRO_START(MUIC_String)
#define ScrollbarObject     MUIOBJMACRO_START(MUIC_Scrollbar)
#define ListviewObject      MUIOBJMACRO_START(MUIC_Listview)
#define RadioObject         MUIOBJMACRO_START(MUIC_Radio)
#define VolumelistObject    MUIOBJMACRO_START(MUIC_Volumelist)
#define FloattextObject     MUIOBJMACRO_START(MUIC_Floattext)
#define DirlistObject       MUIOBJMACRO_START(MUIC_Dirlist)
#define CycleObject         MUIOBJMACRO_START(MUIC_Cycle)
#define GaugeObject         MUIOBJMACRO_START(MUIC_Gauge)
#define ScaleObject         MUIOBJMACRO_START(MUIC_Scale)
#define NumericObject       MUIOBJMACRO_START(MUIC_Numeric)
#define SliderObject        MUIOBJMACRO_START(MUIC_Slider)
#define NumericbuttonObject MUIOBJMACRO_START(MUIC_Numericbutton)
#define KnobObject          MUIOBJMACRO_START(MUIC_Knob)
#define LevelmeterObject    MUIOBJMACRO_START(MUIC_Levelmeter)
#define BoopsiObject        MUIOBJMACRO_START(MUIC_Boopsi)
#define ColorfieldObject    MUIOBJMACRO_START(MUIC_Colorfield)
#define PenadjustObject     MUIOBJMACRO_START(MUIC_Penadjust)
#define ColoradjustObject   MUIOBJMACRO_START(MUIC_Coloradjust)
#define PaletteObject       MUIOBJMACRO_START(MUIC_Palette)
#define GroupObject         MUIOBJMACRO_START(MUIC_Group)
#define RegisterObject      MUIOBJMACRO_START(MUIC_Register)
#define VirtgroupObject     MUIOBJMACRO_START(MUIC_Virtgroup)
#define ScrollgroupObject   MUIOBJMACRO_START(MUIC_Scrollgroup)
#define PopstringObject     MUIOBJMACRO_START(MUIC_Popstring)
#define PopobjectObject     MUIOBJMACRO_START(MUIC_Popobject)
#define PoplistObject       MUIOBJMACRO_START(MUIC_Poplist)
#define PopscreenObject     MUIOBJMACRO_START(MUIC_Popscreen)
#define PopaslObject        MUIOBJMACRO_START(MUIC_Popasl)
#define PendisplayObject    MUIOBJMACRO_START(MUIC_Pendisplay)
#define PoppenObject        MUIOBJMACRO_START(MUIC_Poppen)
#define CrawlingObject      MUIOBJMACRO_START(MUIC_Crawling)
/* The following in zune only */
#define PopimageObject      MUIOBJMACRO_START(MUIC_Popimage)
#define PopframeObject      MUIOBJMACRO_START(MUIC_Popframe)
#define AboutmuiObject      MUIOBJMACRO_START(MUIC_Aboutmui)
#define ScrmodelistObject   MUIOBJMACRO_START(MUIC_Scrmodelist)
#define KeyentryObject      MUIOBJMACRO_START(MUIC_Keyentry)
#define VGroup              MUIOBJMACRO_START(MUIC_Group)
#define HGroup              MUIOBJMACRO_START(MUIC_Group), MUIA_Group_Horiz, TRUE
#define ColGroup(columns)   MUIOBJMACRO_START(MUIC_Group), MUIA_Group_Columns, (columns)
#define RowGroup(rows)      MUIOBJMACRO_START(MUIC_Group), MUIA_Group_Rows   , (rows)
#define PageGroup           MUIOBJMACRO_START(MUIC_Group), MUIA_Group_PageMode, TRUE
#define VGroupV             MUIOBJMACRO_START(MUIC_Virtgroup)
#define HGroupV             MUIOBJMACRO_START(MUIC_Virtgroup), MUIA_Group_Horiz, TRUE
#define ColGroupV(columns)  MUIOBJMACRO_START(MUIC_Virtgroup), MUIA_Group_Columns, (columns)
#define RowGroupV(rows)     MUIOBJMACRO_START(MUIC_Virtgroup), MUIA_Group_Rows   , (rows)
#define PageGroupV          MUIOBJMACRO_START(MUIC_Virtgroup), MUIA_Group_PageMode, TRUE
#define RegisterGroup(ts)   MUIOBJMACRO_START(MUIC_Register), MUIA_Register_Titles, ((IPTR) (ts))

#define End                 OBJMACRO_END

#define Child               MUIA_Group_Child
#define SubWindow           MUIA_Application_Window
#define WindowContents      MUIA_Window_RootObject


/**************************************************************************
 Zune/MUI's differnt frame types. Use one per object
**************************************************************************/
#define NoFrame          MUIA_Frame, MUIV_Frame_None
#define ButtonFrame      MUIA_Frame, MUIV_Frame_Button
#define ImageButtonFrame MUIA_Frame, MUIV_Frame_ImageButton
#define TextFrame        MUIA_Frame, MUIV_Frame_Text
#define StringFrame      MUIA_Frame, MUIV_Frame_String
#define ReadListFrame    MUIA_Frame, MUIV_Frame_ReadList
#define InputListFrame   MUIA_Frame, MUIV_Frame_InputList
#define PropFrame        MUIA_Frame, MUIV_Frame_Prop
#define SliderFrame      MUIA_Frame, MUIV_Frame_Slider
#define GaugeFrame       MUIA_Frame, MUIV_Frame_Gauge
#define VirtualFrame     MUIA_Frame, MUIV_Frame_Virtual
#define GroupFrame       MUIA_Frame, MUIV_Frame_Group
#define GroupFrameT(t)   MUIA_Frame, MUIV_Frame_Group, MUIA_FrameTitle, ((IPTR) (t)), MUIA_Background, MUII_GroupBack


/**************************************************************************
 Space objects
**************************************************************************/
#define HVSpace           MUI_NewObject(MUIC_Rectangle,TAG_DONE)
#define HSpace(x)         MUI_MakeObject(MUIO_HSpace,x)
#define VSpace(x)         MUI_MakeObject(MUIO_VSpace,x)
#define HBar(x)           MUI_MakeObject(MUIO_HBar,x)
#define VBar(x)           MUI_MakeObject(MUIO_VBar,x)
#define HCenter(obj)      (HGroup, GroupSpacing(0), Child, (IPTR)HSpace(0), Child, (IPTR)(obj), Child, (IPTR)HSpace(0), End)
#define VCenter(obj)      (VGroup, GroupSpacing(0), Child, (IPTR)VSpace(0), Child, (IPTR)(obj), Child, (IPTR)VSpace(0), End)
#define InnerSpacing(h,v) MUIA_InnerLeft,(h),MUIA_InnerRight,(h),MUIA_InnerTop,(v),MUIA_InnerBottom,(v)
#define GroupSpacing(x)   MUIA_Group_Spacing,x

#ifdef MUI_OBSOLETE
/**************************************************************************
 These macros will create a simple string gadget. Don't use this in
 new code. Use MUI_MakeObject() instead.
**************************************************************************/
#define String(contents,maxlen)\
    StringObject,\
	StringFrame,\
	MUIA_String_MaxLen  , maxlen,\
	MUIA_String_Contents, contents,\
	End

#define KeyString(contents,maxlen,controlchar)\
    StringObject,\
	StringFrame,\
	MUIA_ControlChar    , controlchar,\
	MUIA_String_MaxLen  , maxlen,\
	MUIA_String_Contents, contents,\
	End

#endif

#ifdef MUI_OBSOLETE
/**************************************************************************
 These macros will create a simple checkmark gadget. Don't use this in
 new code. Use MUI_MakeObject() instead.
**************************************************************************/
#define CheckMark(sel) ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected, sel, End
#define KeyCheckMark(sel,ctrl) ImageObject, ImageButtonFrame, MUIA_InputMode, MUIV_InputMode_Toggle, MUIA_Image_Spec, MUII_CheckMark, MUIA_Image_FreeVert, TRUE, MUIA_Background, MUII_ButtonBack, MUIA_ShowSelState, FALSE, MUIA_Selected, sel, MUIA_ControlChar, ctrl, End
#endif


/**************************************************************************
 These macros will create a simple button. It's simply calling
 MUI_MakeObject()
**************************************************************************/
#define SimpleButton(label) MUI_MakeObject(MUIO_Button,(IPTR)(label))
#define ImageButton(label, imagePath) MUI_MakeObject(MUIO_ImageButton, (IPTR) (label), (IPTR) (imagePath))

#define CoolImageButton(label,image) MUI_MakeObject(MUIO_CoolButton, (IPTR)(label), (IPTR)(image), 0)
#define CoolImageIDButton(label,imageid) MUI_MakeObject(MUIO_CoolButton, (IPTR)(label), imageid, MUIO_CoolButton_CoolImageID)

#ifdef MUI_OBSOLETE
/**************************************************************************
 A Keybutton macro. The key should be in lower case.
 Don't use this in new code. Use MUI_MakeObject() instead.
**************************************************************************/
#define KeyButton(name,key) TextObject, ButtonFrame, MUIA_Font, MUIV_Font_Button, MUIA_Text_Contents, (IPTR)(name), MUIA_Text_PreParse, "\33c", MUIA_Text_HiChar, (IPTR)(key), MUIA_ControlChar, key, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_Background, MUII_ButtonBack, End
#endif


#ifdef MUI_OBSOLETE
/**************************************************************************
 Obsolette Cycle macros
**************************************************************************/
#define Cycle(ent)        CycleObject, MUIA_Font, MUIV_Font_Button, MUIA_Cycle_Entries, ent, End
#define KeyCycle(ent,key) CycleObject, MUIA_Font, MUIV_Font_Button, MUIA_Cycle_Entries, ent, MUIA_ControlChar, key, End

/**************************************************************************
 Obsolette Radios macros
**************************************************************************/
#define Radio(name,array) RadioObject, GroupFrameT(name), MUIA_Radio_Entries, (IPTR)(array), End
#define KeyRadio(name,array,key) RadioObject, GroupFrameT(name), MUIA_Radio_Entries, (IPTR)(array), MUIA_ControlChar, (IPTR)(key), End

/**************************************************************************
 Obsolette Slider macros
**************************************************************************/
#define Slider(min,max,level) SliderObject, MUIA_Numeric_Min, min, MUIA_Numeric_Max, max, MUIA_Numeric_Value, level, End
#define KeySlider(min,max,level,key) SliderObject, MUIA_Numeric_Min, min, MUIA_Numeric_Max, max, MUIA_Numeric_Value, level, MUIA_ControlChar, key, End
#endif



/**************************************************************************
 Use this for getting a pop button
**************************************************************************/
#define PopButton(img) MUI_MakeObject(MUIO_PopButton, img)


/**************************************************************************
 Macros for Labelobjects
 Use them for example in a group containing 2 columns, in the first
 columns the label and in the second columns the object.

 These objects should be uses because the user might have set strange
 values.

 xxxLabel() is suited for Objects without frame
 xxxLabel1() is suited for objects with a single frame, like buttons
 xxxLabel2() is suited for objects with with double frames, like string gadgets
**************************************************************************/

/* Right aligned */
#define Label(label)   MUI_MakeObject(MUIO_Label, (IPTR)(label), 0)
#define Label1(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_SingleFrame)
#define Label2(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_DoubleFrame)

/* Left aligned */
#define LLabel(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_LeftAligned)
#define LLabel1(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_LeftAligned | MUIO_Label_SingleFrame)
#define LLabel2(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame)

/* Centered */
#define CLabel(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_Centered)
#define CLabel1(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_Centered | MUIO_Label_SingleFrame)
#define CLabel2(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_Centered | MUIO_Label_DoubleFrame)

/* Freevert - Right aligned */
#define FreeLabel(label)   MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert)
#define FreeLabel1(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_SingleFrame)
#define FreeLabel2(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_DoubleFrame)

/* Freevert - Left aligned */
#define FreeLLabel(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_LeftAligned)
#define FreeLLabel1(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_SingleFrame)
#define FreeLLabel2(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame)

/* Freevert - Centered */
#define FreeCLabel(label)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_Centered)
#define FreeCLabel1(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_SingleFrame)
#define FreeCLabel2(label) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_DoubleFrame)

/* The same as above + keys */
#define KeyLabel(label,key)   MUI_MakeObject(MUIO_Label, (IPTR)(label), key)
#define KeyLabel1(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_SingleFrame | (key))
#define KeyLabel2(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_DoubleFrame | (key))
#define KeyLLabel(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_LeftAligned | (key))
#define KeyLLabel1(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_LeftAligned | MUIO_Label_SingleFrame|(key))
#define KeyLLabel2(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame|(key))
#define KeyCLabel(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_Centered | (key))
#define KeyCLabel1(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_Centered | MUIO_Label_SingleFrame|(key))
#define KeyCLabel2(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_Centered | MUIO_Label_DoubleFrame|(key))

#define FreeKeyLabel(label,key)   MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | (key))
#define FreeKeyLabel1(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_SingleFrame | (key))
#define FreeKeyLabel2(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_DoubleFrame | (key))
#define FreeKeyLLabel(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_LeftAligned | (key))
#define FreeKeyLLabel1(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_SingleFrame | (key))
#define FreeKeyLLabel2(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_LeftAligned | MUIO_Label_DoubleFrame | (key))
#define FreeKeyCLabel(label,key)  MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_Centered | (key))
#define FreeKeyCLabel1(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_SingleFrame | (key))
#define FreeKeyCLabel2(label,key) MUI_MakeObject(MUIO_Label, (IPTR)(label), MUIO_Label_FreeVert | MUIO_Label_Centered | MUIO_Label_DoubleFrame | (key))


/* Some macros */
#ifndef __cplusplus

#ifdef __GNUC__
#define get(obj, attr, storage)                                         \
({                                                                      \
    union {                                                             \
       IPTR  __zune_get_storage;                                        \
       typeof(*storage) __zune_val_storage;                             \
    } __tmp;                                                              \
    __tmp.__zune_val_storage = *storage;                                  \
    ULONG __zune_get_ret = GetAttr((attr), (obj), &__tmp.__zune_get_storage); \
    *(storage) = __tmp.__zune_val_storage;                                \
    __zune_get_ret;                                                     \
})
#else  /* !__GNUC__ */
#define get(obj,attr,store) GetAttr(attr,obj,(IPTR *)store)
#endif /* !__GNUC__ */

#ifdef __GNUC__
#define XGET(object, attribute)                 \
({                                              \
    IPTR __storage = 0;                         \
    GetAttr((attribute), (object), &__storage); \
    __storage;                                  \
})
#endif /* __GNUC__ */

#define set(obj,attr,value) SetAttrs(obj,attr,(IPTR)(value),TAG_DONE)
#define nnset(obj,attr,value) SetAttrs(obj,MUIA_NoNotify,TRUE,attr,(IPTR)(value),TAG_DONE)

/* Zune */
#define nfset(obj,attr,value) SetAttrs(obj,MUIA_Group_Forward,FALSE,attr,(IPTR)(value),TAG_DONE)
#define nnfset(obj,attr,value) SetAttrs(obj,MUIA_Group_Forward,FALSE,MUIA_NoNotify,TRUE,attr,(IPTR)(value),TAG_DONE)

/* Some aliases... */
#define GET(obj,attr,store) get(obj,attr,store)
#define SET(obj,attr,value) set(obj,attr,value)
#define NNSET(obj,attr,value) nnset(obj,attr,value)
#define NFSET(obj,attr,value) nfset(obj,attr,value)
#define NNFSET(obj,attr,value) nnfset(obj,attr,value)

#define setmutex(obj,n)     set(obj,MUIA_Radio_Active,n)
#define setcycle(obj,n)     set(obj,MUIA_Cycle_Active,n)
#define setstring(obj,s)    set(obj,MUIA_String_Contents,(IPTR)(s))
#define setcheckmark(obj,b) set(obj,MUIA_Selected,b)
#define setslider(obj,l)    set(obj,MUIA_Numeric_Value,l)

#endif /* __cplusplus */


/* We need the notify and area Instace Data at least here, but this stuff should be placed at the button anywhy */
#ifndef _MUI_CLASSES_NOTIFY_H
#endif

#ifndef _MUI_CLASSES_AREA_H
#endif

struct __dummyAreaData__
{
    struct MUI_NotifyData mnd;
    struct MUI_AreaData   mad;
};

#define muiNotifyData(obj) (&(((struct __dummyAreaData__ *)(obj))->mnd))
#define muiAreaData(obj)   (&(((struct __dummyAreaData__ *)(obj))->mad))

#define muiGlobalInfo(obj) (((struct __dummyAreaData__ *)(obj))->mnd.mnd_GlobalInfo)
#define muiUserData(obj)   (((struct __dummyAreaData__ *)(obj))->mnd.mnd_UserData)
#define muiRenderInfo(obj) (((struct __dummyAreaData__ *)(obj))->mad.mad_RenderInfo)


/* the following macros are only valid inbetween MUIM_Setup and MUIM_Cleanup */
#define _app(obj)          (muiGlobalInfo(obj)->mgi_ApplicationObject)
#define _win(obj)          (muiRenderInfo(obj)->mri_WindowObject)
#define _dri(obj)          (muiRenderInfo(obj)->mri_DrawInfo)
#define _screen(obj)       (muiRenderInfo(obj)->mri_Screen)
#define _pens(obj)         (muiRenderInfo(obj)->mri_Pens)
#define _font(obj)         (muiAreaData(obj)->mad_Font)

/* the following macros are only valid during MUIM_Draw */
#define _left(obj)         (muiAreaData(obj)->mad_Box.Left)
#define _top(obj)          (muiAreaData(obj)->mad_Box.Top)
#define _width(obj)        (muiAreaData(obj)->mad_Box.Width)
#define _height(obj)       (muiAreaData(obj)->mad_Box.Height)
#define _right(obj)        (_left(obj) + _width(obj) - 1)
#define _bottom(obj)       (_top(obj) + _height(obj) - 1)
#define _addleft(obj)      (muiAreaData(obj)->mad_addleft  )
#define _addtop(obj)       (muiAreaData(obj)->mad_addtop   )
#define _subwidth(obj)     (muiAreaData(obj)->mad_subwidth )
#define _subheight(obj)    (muiAreaData(obj)->mad_subheight)
#define _mleft(obj)        (_left(obj) + _addleft(obj))
#define _mtop(obj)         (_top(obj) + _addtop(obj))
#define _mwidth(obj)       (_width(obj) - _subwidth(obj))
#define _mheight(obj)      (_height(obj) - _subheight(obj))
#define _mright(obj)       (_mleft(obj) + _mwidth(obj) - 1)
#define _mbottom(obj)      (_mtop(obj) + _mheight(obj) - 1)

/* the following macros are only valid inbetween MUIM_Show and MUIM_Hide */
#define _window(obj)       (muiRenderInfo(obj)->mri_Window)
#define _rp(obj)           (muiRenderInfo(obj)->mri_RastPort)
#define _minwidth(obj)     (muiAreaData(obj)->mad_MinMax.MinWidth)
#define _minheight(obj)    (muiAreaData(obj)->mad_MinMax.MinHeight)
#define _maxwidth(obj)     (muiAreaData(obj)->mad_MinMax.MaxWidth)
#define _maxheight(obj)    (muiAreaData(obj)->mad_MinMax.MaxHeight)
#define _defwidth(obj)     (muiAreaData(obj)->mad_MinMax.DefWidth)
#define _defheight(obj)    (muiAreaData(obj)->mad_MinMax.DefHeight)
#define _flags(obj)        (muiAreaData(obj)->mad_Flags)



#endif /* _MUI_MACROS_H */
#endif

#endif /* LIBRARIES_MUIAROS_H */
