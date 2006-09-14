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
/* Method Structures */
struct MUIP_Application_AboutMUI		{ ULONG MethodID; Object *refwindow; };
struct MUIP_Application_AddInputHandler	{ ULONG MethodID; struct MUI_InputHandlerNode *ihnode; };
struct MUIP_Application_CheckRefresh		{ ULONG MethodID; };
struct MUIP_Application_GetMenuCheck		{ ULONG MethodID; ULONG MenuID; };
struct MUIP_Application_GetMenuState		{ ULONG MethodID; ULONG MenuID; };
struct MUIP_Application_Input			{ ULONG MethodID; ULONG *signal; };
struct MUIP_Application_InputBuffered		{ ULONG MethodID; };
struct MUIP_Application_Load			{ ULONG MethodID; STRPTR name; };
struct MUIP_Application_NewInput		{ ULONG MethodID; ULONG *signal; };
struct MUIP_Application_OpenConfigWindow	{ ULONG MethodID; ULONG flags; };
struct MUIP_Application_PushMethod		{ ULONG MethodID; Object *dest; LONG count; /* more elements may follow */ };
struct MUIP_Application_RemInputHandler	{ ULONG MethodID; struct MUI_InputHandlerNode *ihnode; };
struct MUIP_Application_ReturnID		{ ULONG MethodID; ULONG retid; };
struct MUIP_Application_Save			{ ULONG MethodID; STRPTR name; };
struct MUIP_Application_SetConfigItem		{ ULONG MethodID; ULONG item; APTR data; };
struct MUIP_Application_SetMenuCheck		{ ULONG MethodID; ULONG MenuID; LONG stat; };
struct MUIP_Application_SetMenuState		{ ULONG MethodID; ULONG MenuID; LONG stat; };
struct MUIP_Application_ShowHelp		{ ULONG MethodID; Object *window; char *name; char *node; LONG line; };
struct MUIP_Application_SetConfigdata		{ ULONG MethodID; APTR configdata; };
struct MUIP_Application_OpenWindows		{ ULONG MethodID; };

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
#define MUIV_Application_ReturnID_Quit (-1)



extern const struct __MUIBuiltinClass _MUI_Application_desc; /* PRIV */



struct MUI_GlobalInfo
{
    ULONG priv0;
    Object *mgi_ApplicationObject;

    /* The following data is private only, might be extented! */
    struct MsgPort      *mgi_WindowsPort; /* application-wide IDCMP port */ /* PRIV */
    Object              *mgi_Configdata; /* The config data */ /* PRIV */
    struct ZunePrefsNew *mgi_Prefs; /* For faster access */ /* PRIV */
};


#endif /* _MUI_CLASSES_APPLICATION_H */
