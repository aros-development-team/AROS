/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef _MUI_CLASSES_APPLICATION_H
#define _MUI_CLASSES_APPLICATION_H

#ifndef EXEC_SEMAPHORES_H
#include <exec/semaphores.h>
#endif

/* Classname */
#define MUIC_Application "Application.mui"

#ifndef METHOD_USER
#define METHOD_USER TAG_USER
#endif

/* Method Ids */
#define MUIM_Application_AboutMUI		(METHOD_USER|0x0042d21d) /* MUI: V14 */
#define MUIM_Application_AddInputHandler	(METHOD_USER|0x0042f099) /* MUI: V11 */
#define MUIM_Application_CheckRefresh		(METHOD_USER|0x00424d68) /* MUI: V11 */
#define MUIM_Application_GetMenuCheck		(METHOD_USER|0x0042c0a7) /* MUI: V4  */
#define MUIM_Application_GetMenuState		(METHOD_USER|0x0042a58f) /* MUI: V4  */
#define MUIM_Application_Input			(METHOD_USER|0x0042d0f5) /* MUI: V4  */
#define MUIM_Application_InputBuffered		(METHOD_USER|0x00427e59) /* MUI: V4  */
#define MUIM_Application_Load			(METHOD_USER|0x0042f90d) /* MUI: V4  */
#define MUIM_Application_NewInput		(METHOD_USER|0x00423ba6) /* MUI: V11 */
#define MUIM_Application_OpenConfigWindow	(METHOD_USER|0x004299ba) /* MUI: V11 */
#define MUIM_Application_PushMethod		(METHOD_USER|0x00429ef8) /* MUI: V4  */
#define MUIM_Application_RemInputHandler	(METHOD_USER|0x0042e7af) /* MUI: V11 */
#define MUIM_Application_ReturnID		(METHOD_USER|0x004276ef) /* MUI: V4  */
#define MUIM_Application_Save			(METHOD_USER|0x004227ef) /* MUI: V4  */
#define MUIM_Application_SetConfigItem		(METHOD_USER|0x00424a80) /* MUI: V11 */
#define MUIM_Application_SetMenuCheck		(METHOD_USER|0x0042a707) /* MUI: V4  */
#define MUIM_Application_SetMenuState		(METHOD_USER|0x00428bef) /* MUI: V4  */
#define MUIM_Application_ShowHelp		(METHOD_USER|0x00426479) /* MUI: V4  */

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

/* Attributes */
#define MUIA_Application_Active             	(TAG_USER|0x004260ab) /* MUI: V4  isg BOOL              */
#define MUIA_Application_Author             	(TAG_USER|0x00424842) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_Base               	(TAG_USER|0x0042e07a) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_Broker             	(TAG_USER|0x0042dbce) /* MUI: V4  ..g Broker *          */
#define MUIA_Application_BrokerHook         	(TAG_USER|0x00428f4b) /* MUI: V4  isg struct Hook *     */
#define MUIA_Application_BrokerPort         	(TAG_USER|0x0042e0ad) /* MUI: V6  ..g struct MsgPort *  */
#define MUIA_Application_BrokerPri          	(TAG_USER|0x0042c8d0) /* MUI: V6  i.g LONG              */
#define MUIA_Application_Commands           	(TAG_USER|0x00428648) /* MUI: V4  isg struct MUI_Command * */
#define MUIA_Application_Copyright          	(TAG_USER|0x0042ef4d) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_Description        	(TAG_USER|0x00421fc6) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_DiskObject         	(TAG_USER|0x004235cb) /* MUI: V4  isg struct DiskObject * */
#define MUIA_Application_DoubleStart        	(TAG_USER|0x00423bc6) /* MUI: V4  ..g BOOL              */
#define MUIA_Application_DropObject         	(TAG_USER|0x00421266) /* MUI: V5  is. Object *          */
#define MUIA_Application_ForceQuit          	(TAG_USER|0x004257df) /* MUI: V8  ..g BOOL              */
#define MUIA_Application_HelpFile           	(TAG_USER|0x004293f4) /* MUI: V8  isg STRPTR            */
#define MUIA_Application_Iconified          	(TAG_USER|0x0042a07f) /* MUI: V4  .sg BOOL              */
#define MUIA_Application_MenuAction         	(TAG_USER|0x00428961) /* MUI: V4  ..g ULONG             */
#define MUIA_Application_MenuHelp           	(TAG_USER|0x0042540b) /* MUI: V4  ..g ULONG             */
#define MUIA_Application_Menustrip          	(TAG_USER|0x004252d9) /* MUI: V8  i.. Object *          */
#define MUIA_Application_RexxHook           	(TAG_USER|0x00427c42) /* MUI: V7  isg struct Hook *     */
#define MUIA_Application_RexxMsg            	(TAG_USER|0x0042fd88) /* MUI: V4  ..g struct RxMsg *    */
#define MUIA_Application_RexxString         	(TAG_USER|0x0042d711) /* MUI: V4  .s. STRPTR            */
#define MUIA_Application_SingleTask         	(TAG_USER|0x0042a2c8) /* MUI: V4  i.. BOOL              */
#define MUIA_Application_Sleep              	(TAG_USER|0x00425711) /* MUI: V4  .s. BOOL              */
#define MUIA_Application_Title              	(TAG_USER|0x004281b8) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_UseCommodities     	(TAG_USER|0x00425ee5) /* MUI: V10 i.. BOOL              */
#define MUIA_Application_UsedClasses            (TAG_USER|0x0042e9a7) /* MUI undoc: V20 i.. STRPTR [] */
#define MUIA_Application_UseRexx            	(TAG_USER|0x00422387) /* MUI: V10 i.. BOOL              */
#define MUIA_Application_Version            	(TAG_USER|0x0042b33f) /* MUI: V4  i.g STRPTR            */
#define MUIA_Application_Window             	(TAG_USER|0x0042bfe0) /* MUI: V4  i.. Object *          */
#define MUIA_Application_WindowList         	(TAG_USER|0x00429abe) /* MUI: V13 ..g struct List *     */

/* MUI Obsolette tags */
#ifdef MUI_OBSOLETE
#define MUIA_Application_Menu							  (TAG_USER|0x00420e1f) /* MUI: V4  i.g struct NewMenu *  */
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


/**************************************************************************
 Zune extensions. Note that the tag values of the Zune extensions
 might be changed in the future
**************************************************************************/
#define MUIM_Application_Iconify       0x80429ab8 /* Zune: V1  */

extern const struct __MUIBuiltinClass _MUI_Application_desc; /* PRIV */



struct MUI_GlobalInfo
{
    ULONG priv0;
    Object *mgi_ApplicationObject;

    /* The following data is private only, might be extented! */
    struct MsgPort *mgi_UserPort; /* application-wide IDCMP port */ /* PRIV */
    Object *mgi_Configdata; /* The config data */
    struct ZunePrefsNew *mgi_Prefs; /* For faster access */

/* should be (-MUIV_Font_NegCount) */
    struct TextFont *mgi_Fonts[9]; /* Opened text fonts, done by zune_get_font() */
};


#endif
