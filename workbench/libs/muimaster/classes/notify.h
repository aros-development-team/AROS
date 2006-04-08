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

#define MUIM_ConnectParent       (MUIB_Notify | 0x00000000) /* Zune: V1 */
#define MUIM_DisconnectParent    (MUIB_Notify | 0x00000001) /* Zune: V1 */
struct MUIP_ConnectParent        {ULONG MethodID; Object *parent;};
struct MUIP_DisconnectParent     {ULONG MethodID;};

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


#endif /* _MUI_CLASSES_NOTIFY_H */
