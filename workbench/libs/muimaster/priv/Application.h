/****************************************************************************/
/** Application                                                            **/
/****************************************************************************/

#ifdef _DCC
extern char MUIC_Application[];
#else
#define MUIC_Application "Application.mui"
#endif

/* Methods */

#define MUIM_Application_AboutMUI           0x8042d21d /* V14 */
#define MUIM_Application_AddInputHandler    0x8042f099 /* V11 */
#define MUIM_Application_CheckRefresh       0x80424d68 /* V11 */
#define MUIM_Application_GetMenuCheck       0x8042c0a7 /* V4  */
#define MUIM_Application_GetMenuState       0x8042a58f /* V4  */
#define MUIM_Application_Input              0x8042d0f5 /* V4  */
#define MUIM_Application_InputBuffered      0x80427e59 /* V4  */
#define MUIM_Application_Load               0x8042f90d /* V4  */
#define MUIM_Application_NewInput           0x80423ba6 /* V11 */
#define MUIM_Application_OpenConfigWindow   0x804299ba /* V11 */
#define MUIM_Application_PushMethod         0x80429ef8 /* V4  */
#define MUIM_Application_RemInputHandler    0x8042e7af /* V11 */
#define MUIM_Application_ReturnID           0x804276ef /* V4  */
#define MUIM_Application_Save               0x804227ef /* V4  */
#define MUIM_Application_SetConfigItem      0x80424a80 /* V11 */
#define MUIM_Application_SetMenuCheck       0x8042a707 /* V4  */
#define MUIM_Application_SetMenuState       0x80428bef /* V4  */
#define MUIM_Application_ShowHelp           0x80426479 /* V4  */
struct  MUIP_Application_AboutMUI           { ULONG MethodID; Object *refwindow; };
struct  MUIP_Application_AddInputHandler    { ULONG MethodID; struct MUI_InputHandlerNode *ihnode; };
struct  MUIP_Application_CheckRefresh       { ULONG MethodID; };
struct  MUIP_Application_GetMenuCheck       { ULONG MethodID; ULONG MenuID; };
struct  MUIP_Application_GetMenuState       { ULONG MethodID; ULONG MenuID; };
struct  MUIP_Application_Input              { ULONG MethodID; ULONG *signal; };
struct  MUIP_Application_InputBuffered      { ULONG MethodID; };
struct  MUIP_Application_Load               { ULONG MethodID; STRPTR name; };
struct  MUIP_Application_NewInput           { ULONG MethodID; ULONG *signal; };
struct  MUIP_Application_OpenConfigWindow   { ULONG MethodID; ULONG flags; };
struct  MUIP_Application_PushMethod         { ULONG MethodID; Object *dest; LONG count; /* ... */ };
struct  MUIP_Application_RemInputHandler    { ULONG MethodID; struct MUI_InputHandlerNode *ihnode; };
struct  MUIP_Application_ReturnID           { ULONG MethodID; ULONG retid; };
struct  MUIP_Application_Save               { ULONG MethodID; STRPTR name; };
struct  MUIP_Application_SetConfigItem      { ULONG MethodID; ULONG item; APTR data; };
struct  MUIP_Application_SetMenuCheck       { ULONG MethodID; ULONG MenuID; LONG stat; };
struct  MUIP_Application_SetMenuState       { ULONG MethodID; ULONG MenuID; LONG stat; };
struct  MUIP_Application_ShowHelp           { ULONG MethodID; Object *window; char *name; char *node; LONG line; };

/* Attributes */

#define MUIA_Application_Active             0x804260ab /* V4  isg BOOL              */
#define MUIA_Application_Author             0x80424842 /* V4  i.g STRPTR            */
#define MUIA_Application_Base               0x8042e07a /* V4  i.g STRPTR            */
#define MUIA_Application_Broker             0x8042dbce /* V4  ..g Broker *          */
#define MUIA_Application_BrokerHook         0x80428f4b /* V4  isg struct Hook *     */
#define MUIA_Application_BrokerPort         0x8042e0ad /* V6  ..g struct MsgPort *  */
#define MUIA_Application_BrokerPri          0x8042c8d0 /* V6  i.g LONG              */
#define MUIA_Application_Commands           0x80428648 /* V4  isg struct MUI_Command * */
#define MUIA_Application_Copyright          0x8042ef4d /* V4  i.g STRPTR            */
#define MUIA_Application_Description        0x80421fc6 /* V4  i.g STRPTR            */
#define MUIA_Application_DiskObject         0x804235cb /* V4  isg struct DiskObject * */
#define MUIA_Application_DoubleStart        0x80423bc6 /* V4  ..g BOOL              */
#define MUIA_Application_DropObject         0x80421266 /* V5  is. Object *          */
#define MUIA_Application_ForceQuit          0x804257df /* V8  ..g BOOL              */
#define MUIA_Application_HelpFile           0x804293f4 /* V8  isg STRPTR            */
#define MUIA_Application_Iconified          0x8042a07f /* V4  .sg BOOL              */
#ifdef MUI_OBSOLETE
#define MUIA_Application_Menu               0x80420e1f /* V4  i.g struct NewMenu *  */
#endif /* MUI_OBSOLETE */
#define MUIA_Application_MenuAction         0x80428961 /* V4  ..g ULONG             */
#define MUIA_Application_MenuHelp           0x8042540b /* V4  ..g ULONG             */
#define MUIA_Application_Menustrip          0x804252d9 /* V8  i.. Object *          */
#define MUIA_Application_RexxHook           0x80427c42 /* V7  isg struct Hook *     */
#define MUIA_Application_RexxMsg            0x8042fd88 /* V4  ..g struct RxMsg *    */
#define MUIA_Application_RexxString         0x8042d711 /* V4  .s. STRPTR            */
#define MUIA_Application_SingleTask         0x8042a2c8 /* V4  i.. BOOL              */
#define MUIA_Application_Sleep              0x80425711 /* V4  .s. BOOL              */
#define MUIA_Application_Title              0x804281b8 /* V4  i.g STRPTR            */
#define MUIA_Application_UseCommodities     0x80425ee5 /* V10 i.. BOOL              */
#define MUIA_Application_UseRexx            0x80422387 /* V10 i.. BOOL              */
#define MUIA_Application_Version            0x8042b33f /* V4  i.g STRPTR            */
#define MUIA_Application_Window             0x8042bfe0 /* V4  i.. Object *          */
#define MUIA_Application_WindowList         0x80429abe /* V13 ..g struct List *     */

#define MUIV_Application_Package_NetConnect 0xa3ff7b49

