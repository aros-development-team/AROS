/****************************************************************************/
/** Notify                                                                 **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Notify[];
#else
#define MUIC_Notify "Notify.mui"
#endif

/* Methods */

#define MUIM_CallHook                       0x8042b96b /* V4  */
#define MUIM_Export                         0x80420f1c /* V12 */
#define MUIM_FindUData                      0x8042c196 /* V8  */
#define MUIM_GetConfigItem                  0x80423edb /* V11 */
#define MUIM_GetUData                       0x8042ed0c /* V8  */
#define MUIM_Import                         0x8042d012 /* V12 */
#define MUIM_KillNotify                     0x8042d240 /* V4  */
#define MUIM_KillNotifyObj                  0x8042b145 /* V16 */
#define MUIM_MultiSet                       0x8042d356 /* V7  */
#define MUIM_NoNotifySet                    0x8042216f /* V9  */
#define MUIM_Notify                         0x8042c9cb /* V4  */
#define MUIM_Set                            0x8042549a /* V4  */
#define MUIM_SetAsString                    0x80422590 /* V4  */
#define MUIM_SetUData                       0x8042c920 /* V8  */
#define MUIM_SetUDataOnce                   0x8042ca19 /* V11 */
#define MUIM_WriteLong                      0x80428d86 /* V6  */
#define MUIM_WriteString                    0x80424bf4 /* V6  */
struct  MUIP_CallHook                       { ULONG MethodID; struct Hook *Hook; ULONG param1; /* ... */ };
struct  MUIP_Export                         { ULONG MethodID; Object *dataspace; };
struct  MUIP_FindUData                      { ULONG MethodID; ULONG udata; };
struct  MUIP_GetConfigItem                  { ULONG MethodID; ULONG id; ULONG *storage; };
struct  MUIP_GetUData                       { ULONG MethodID; ULONG udata; ULONG attr; ULONG *storage; };
struct  MUIP_Import                         { ULONG MethodID; Object *dataspace; };
struct  MUIP_KillNotify                     { ULONG MethodID; ULONG TrigAttr; };
struct  MUIP_KillNotifyObj                  { ULONG MethodID; ULONG TrigAttr; Object *dest; };
struct  MUIP_MultiSet                       { ULONG MethodID; ULONG attr; ULONG val; APTR obj; /* ... */ };
struct  MUIP_NoNotifySet                    { ULONG MethodID; ULONG attr; ULONG val; /* ... */ };
struct  MUIP_Notify                         { ULONG MethodID; ULONG TrigAttr; ULONG TrigVal; APTR DestObj; ULONG FollowParams; /* ... */ };
struct  MUIP_Set                            { ULONG MethodID; ULONG attr; ULONG val; };
struct  MUIP_SetAsString                    { ULONG MethodID; ULONG attr; char *format; ULONG val; /* ... */ };
struct  MUIP_SetUData                       { ULONG MethodID; ULONG udata; ULONG attr; ULONG val; };
struct  MUIP_SetUDataOnce                   { ULONG MethodID; ULONG udata; ULONG attr; ULONG val; };
struct  MUIP_WriteLong                      { ULONG MethodID; ULONG val; ULONG *memory; };
struct  MUIP_WriteString                    { ULONG MethodID; char *str; char *memory; };

/* Attributes */

#define MUIA_ApplicationObject              0x8042d3ee /* V4  ..g Object *          */
#define MUIA_AppMessage                     0x80421955 /* V5  ..g struct AppMessage * */
#define MUIA_HelpLine                       0x8042a825 /* V4  isg LONG              */
#define MUIA_HelpNode                       0x80420b85 /* V4  isg STRPTR            */
#define MUIA_NoNotify                       0x804237f9 /* V7  .s. BOOL              */
#define MUIA_ObjectID                       0x8042d76e /* V11 isg ULONG             */
#define MUIA_Parent                         0x8042e35f /* V11 ..g Object *          */
#define MUIA_Revision                       0x80427eaa /* V4  ..g LONG              */
#define MUIA_UserData                       0x80420313 /* V4  isg ULONG             */
#define MUIA_Version                        0x80422301 /* V4  ..g LONG              */

