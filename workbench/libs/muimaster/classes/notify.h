/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#ifndef _MUI_CLASSES_NOTIFY_H
#define _MUI_CLASSES_NOTIFY_H

struct MUI_NotifyData
{
    struct MUI_GlobalInfo *mnd_GlobalInfo;
    ULONG                  mnd_UserData;
    STRPTR                 mnd_ObjectID;

    /* private starts here */
    struct MinList        *mnd_NotifyList; /* priv1 */
    Object                *mnd_ParentObject;/* priv2 */
    STRPTR                 mnd_HelpNode;/* priv3 */
    LONG                   mnd_HelpLine;/* priv4 */
};

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

#define MUIC_Notify "Notify.mui"

/* Notify methods */
#define MUIM_CallHook            (METHOD_USER|0x0042b96b) /* MUI: V4  */
#define MUIM_Export              (METHOD_USER|0x00420f1c) /* MUI: V12 */
#define MUIM_FindUData           (METHOD_USER|0x0042c196) /* MUI: V8  */
#define MUIM_GetConfigItem       (METHOD_USER|0x00423edb) /* MUI: V11 */
#define MUIM_GetUData            (METHOD_USER|0x0042ed0c) /* MUI: V8  */
#define MUIM_Import              (METHOD_USER|0x0042d012) /* MUI: V12 */
#define MUIM_KillNotify          (METHOD_USER|0x0042d240) /* MUI: V4  */
#define MUIM_KillNotifyObj       (METHOD_USER|0x0042b145) /* MUI: V16 */
#define MUIM_MultiSet            (METHOD_USER|0x0042d356) /* MUI: V7  */
#define MUIM_NoNotifySet         (METHOD_USER|0x0042216f) /* MUI: V9  */
#define MUIM_Notify              (METHOD_USER|0x0042c9cb) /* MUI: V4  */
#define MUIM_Set                 (METHOD_USER|0x0042549a) /* MUI: V4  */
#define MUIM_SetAsString         (METHOD_USER|0x00422590) /* MUI: V4  */
#define MUIM_SetUData            (METHOD_USER|0x0042c920) /* MUI: V8  */
#define MUIM_SetUDataOnce        (METHOD_USER|0x0042ca19) /* MUI: V11 */
#define MUIM_WriteLong           (METHOD_USER|0x00428d86) /* MUI: V6  */
#define MUIM_WriteString         (METHOD_USER|0x00424bf4) /* MUI: V6  */
struct MUIP_CallHook             {ULONG MethodID; struct Hook *Hook; ULONG param1; /* more might follow */};
struct MUIP_Export               {ULONG MethodID; Object *dataspace;};
struct MUIP_FindUData            {ULONG MethodID; ULONG udata;};
struct MUIP_GetConfigItem        {ULONG MethodID; ULONG id; ULONG *storage;};
struct MUIP_GetUData             {ULONG MethodID; ULONG udata; ULONG attr; ULONG *storage;};
struct MUIP_Import               {ULONG MethodID; Object *dataspace;};
struct MUIP_KillNotify           {ULONG MethodID; ULONG TrigAttr;};
struct MUIP_KillNotifyObj        {ULONG MethodID; ULONG TrigAttr; Object *dest;};
struct MUIP_MultiSet             {ULONG MethodID; ULONG attr; ULONG val; APTR obj; /* more might follow */};
struct MUIP_NoNotifySet          {ULONG MethodID; ULONG attr; ULONG val; /* more might follow */};
struct MUIP_Notify               {ULONG MethodID; ULONG TrigAttr; ULONG TrigVal; APTR DestObj; ULONG FollowParams; /* more might follow */};
struct MUIP_Set                  {ULONG MethodID; ULONG attr; ULONG val;};
struct MUIP_SetAsString          {ULONG MethodID; ULONG attr; char *format; ULONG val; /* more might follow */};
struct MUIP_SetUData             {ULONG MethodID; ULONG udata; ULONG attr; ULONG val;};
struct MUIP_SetUDataOnce         {ULONG MethodID; ULONG udata; ULONG attr; ULONG val;};
struct MUIP_WriteLong            {ULONG MethodID; ULONG val; ULONG *memory;};
struct MUIP_WriteString          {ULONG MethodID; char *str; char *memory;};

/* Notify attributes */
#define MUIA_ApplicationObject   (TAG_USER|0x0042d3ee) /* MUI: V4  ..g Object *            */
#define MUIA_AppMessage          (TAG_USER|0x00421955) /* MUI: V5  ..g struct AppMessage * */
#define MUIA_HelpLine            (TAG_USER|0x0042a825) /* MUI: V4  isg LONG                */
#define MUIA_HelpNode            (TAG_USER|0x00420b85) /* MUI: V4  isg STRPTR              */
#define MUIA_NoNotify            (TAG_USER|0x004237f9) /* MUI: V7  .s. BOOL                */
#define MUIA_ObjectID            (TAG_USER|0x0042d76e) /* MUI: V11 isg ULONG               */
#define MUIA_Parent              (TAG_USER|0x0042e35f) /* MUI: V11 ..g Object *            */
#define MUIA_Revision            (TAG_USER|0x00427eaa) /* MUI: V4  ..g LONG                */
#define MUIA_UserData            (TAG_USER|0x00420313) /* MUI: V4  isg ULONG               */
#define MUIA_Version             (TAG_USER|0x00422301) /* MUI: V4  ..g LONG                */

/* Special values for MUIM_Notify */
#define MUIV_TriggerValue    0x49893131
#define MUIV_NotTriggerValue 0x49893133
#define MUIV_EveryTime       0x49893131 /* as TrigVal */

enum
{
    MUIV_Notify_Self = 1,
    MUIV_Notify_Window,
    MUIV_Notify_Application,
    MUIV_Notify_Parent,
};

extern const struct __MUIBuiltinClass _MUI_Notify_desc; /* PRIV */

/* Private stuff */

#define _parent(obj)    (muiNotifyData(obj)->mnd_ParentObject) /* Shortcut */

#define MUIM_ConnectParent       (METHOD_USER|0x10429ab9) /* Zune: V1 */
#define MUIM_DisconnectParent    (METHOD_USER|0x10429aba) /* Zune: V1 */

struct MUIP_ConnectParent          {ULONG MethodID; Object *parent;};
struct MUIP_DisconnectParent       {ULONG MethodID;};

#endif
