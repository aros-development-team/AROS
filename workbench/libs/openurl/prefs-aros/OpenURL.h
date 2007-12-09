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
**
**  Main includes and definitions
*/

#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG
#endif

#ifndef __AROS__
#define __NOLIBBASE__
#define __USE_SYSBASE
#endif

#include <proto/exec.h>
#include <proto/dos.h>

#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/locale.h>
#include <proto/openurl.h>

#include <workbench/startup.h>

#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/mui.h>

#include <clib/alib_protos.h>

#ifdef __AROS__
#include <mui/NList_mcc.h>
#else /* __AROS__ */
#include <clib/debug_protos.h>

#include <mui/Textinput_mcc.h>
#include <mui/Urltext_mcc.h>
#include <mui/MUIundoc.h>
#endif /* __AROS__ */

#include <string.h>
#include <ctype.h>

#include <macros.h>

/**************************************************************************/
/*
** Globals
*/

extern struct ExecBase        *SysBase;
extern struct DosLibrary      *DOSBase;
extern struct IntuitionBase   *IntuitionBase;
extern struct GfxBase         *GfxBase;
extern struct Library         *MUIMasterBase;
#ifdef __AROS__
extern struct UtilityBase     *UtilityBase;
#else
extern struct Library         *UtilityBase;
#endif
extern struct Library         *IconBase;
extern struct LocaleBase      *LocaleBase;
extern struct Library         *OpenURLBase;

extern struct MUI_CustomClass *g_appClass;
extern struct MUI_CustomClass *g_pensClass;
extern struct MUI_CustomClass *g_aboutClass;
extern struct MUI_CustomClass *g_winClass;
extern struct MUI_CustomClass *g_appListClass;
extern struct MUI_CustomClass *g_browserEditWinClass;
extern struct MUI_CustomClass *g_mailerEditWinClass;
extern struct MUI_CustomClass *g_FTPEditWinClass;
extern struct MUI_CustomClass *g_popportClass;
extern struct MUI_CustomClass *g_popphClass;

extern struct Catalog         *g_cat;
extern APTR                   g_pool;
//extern ULONG                  g_MUI4;

/**************************************************************************/
/*
** Definitions
*/
#define THIS_PREFS_VERSION ((UBYTE)4)

#define APPBASENAME  "OPENURL"
#define APPAUTHOR    "Troels Walsted Hansen, Alfonso Ranieri, Stefan Kost"
#define APPHELP      PRG".guide"

/**************************************************************************/
/*
** Identify a generic URL_#? node
*/

struct URL_Node
{
    struct MinNode Node;
    ULONG          Flags;
};

/**************************************************************************/
/*
** MUI tags bases
*/

#define TAG_MUI_TWH            ((int)0x81480000+100)

#define TAGBASE_APP            (TAG_MUI_TWH+0)
#define TAGBASE_ABOUT          (TAG_MUI_TWH+20)
#define TAGBASE_PENS           (TAG_MUI_TWH+40)
#define TAGBASE_WIN            (TAG_MUI_TWH+60)
#define TAGBASE_APPLIST        (TAG_MUI_TWH+80)
#define TAGBASE_BROWSEREDITWIN (TAG_MUI_TWH+100)
#define TAGBASE_MAILEREDITWIN  (TAG_MUI_TWH+120)
#define TAGBASE_FTPEDITWIN     (TAG_MUI_TWH+140)
#define TAGBASE_POPPORT        (TAG_MUI_TWH+160)
#define TAGBASE_LAMP           (TAG_MUI_TWH+180)
#define TAGBASE_POPPH          (TAG_MUI_TWH+200)

/**************************************************************************/
/*
** App class
*/

#ifdef __AROS__
#define appObject BOOPSIOBJMACRO_START(g_appClass->mcc_Class)
#else
#define appObject NewObject(g_appClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_App_OpenWin    (TAGBASE_APP+0)
#define MUIM_App_GetPrefs   (TAGBASE_APP+1)
#define MUIM_App_About      (TAGBASE_APP+2)
#define MUIM_App_DisposeWin (TAGBASE_APP+4)
#define MUIM_App_CloseWin   (TAGBASE_APP+3)
#define MUIM_App_CheckSave  (TAGBASE_APP+4)

/* Structures */

struct MUIP_App_OpenWin
{
    ULONG         MethodID;
    struct IClass *Class;
    ULONG         IDAttr;
    ULONG         IDVal;
    ULONG         InitAttrs;
};

struct MUIP_App_GetPrefs
{
    ULONG MethodID;
    ULONG mode;
};

/* mode */
enum
{
    MUIV_App_GetPrefs_InUse,
    MUIV_App_GetPrefs_LastSaveds,
    MUIV_App_GetPrefs_Restore,
    MUIV_App_GetPrefs_Defaults,
};

struct MUIP_App_DisposeWin
{
    ULONG  MethodID;
    Object *win;
};

struct MUIP_App_CloseWin
{
    ULONG MethodID;
    ULONG IDAttr;
    ULONG IDVal;
};

/* Attributes */
#define MUIA_App_IsSubWin (TAGBASE_APP+0)
#define MUIA_App_Pens     (TAGBASE_APP+1)

/**************************************************************************/
/*
** About class
*/

#ifdef __AROS__
#define aboutObject BOOPSIOBJMACRO_START(g_aboutClass->mcc_Class)
#else
#define aboutObject NewObject(g_aboutClass->mcc_Class,NULL
#endif

/**************************************************************************/
/*
** Win class
*/

#ifdef __AROS__
#define winObject BOOPSIOBJMACRO_START(g_winClass->mcc_Class)
#else
#define winObject NewObject(g_winClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_Win_GetPrefs   (TAGBASE_WIN+0)
#define MUIM_Win_StorePrefs (TAGBASE_WIN+1)
#define MUIM_Win_Delete     (TAGBASE_WIN+2)

/* Structures */

struct MUIP_Win_GetPrefs
{
    ULONG MethodID;
    ULONG mode;
};

/* mode */
enum
{
    MUIV_Win_GetPrefs_InUse,
    MUIV_Win_GetPrefs_LastSaveds,
    MUIV_Win_GetPrefs_Restore,
    MUIV_Win_GetPrefs_Defaults,
};

struct MUIP_Win_StorePrefs
{
    ULONG MethodID;
    ULONG How;
};

/* How values*/
enum
{
    MUIV_Win_StorePrefs_Save,
    MUIV_Win_StorePrefs_Use,
    MUIV_Win_StorePrefs_Apply,
};

struct MUIP_Win_Delete
{
    ULONG MethodID;
    APTR  entry;
};

/**************************************************************************/
/*
** AppList class
*/

#ifdef __AROS__
#define appListObject BOOPSIOBJMACRO_START(g_appListClass->mcc_Class)
#else
#define appListObject NewObject(g_appListClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_AppList_Add           (TAGBASE_APPLIST+0)
#define MUIM_AppList_Edit          (TAGBASE_APPLIST+1)
#define MUIM_AppList_Clone         (TAGBASE_APPLIST+2)
#define MUIM_AppList_Delete        (TAGBASE_APPLIST+3)
#define MUIM_AppList_ActiveChanged (TAGBASE_APPLIST+4)
#define MUIM_AppList_Disable       (TAGBASE_APPLIST+5)
#define MUIM_AppList_Move          (TAGBASE_APPLIST+6)

struct MUIP_AppList_Edit
{
    ULONG MethodID;
    ULONG check;
};

struct MUIP_AppList_Disable
{
    ULONG MethodID;
    ULONG disable;
};

struct MUIP_AppList_Move
{
    ULONG MethodID;
    ULONG up;
};

/* Attributes */
#define MUIA_AppList_Type               (TAGBASE_APPLIST+0)
#define MUIA_AppList_ListObj            (TAGBASE_APPLIST+1)
#define MUIA_AppList_NodeNameOffset     (TAGBASE_APPLIST+2)
#define MUIA_AppList_NodePathOffset     (TAGBASE_APPLIST+3)
#define MUIA_AppList_NodeSize           (TAGBASE_APPLIST+4)

enum
{
    MUIV_AppList_Type_Browser,
    MUIV_AppList_Type_Mailer,
    MUIV_AppList_Type_FTP,
};

/**************************************************************************/
/*
** BrowserEditWin class
*/

#ifdef __AROS__
#define browserEditWinObject BOOPSIOBJMACRO_START(g_browserEditWinClass->mcc_Class)
#else
#define browserEditWinObject NewObject(g_browserEditWinClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_BrowserEditWin_Use (TAGBASE_BROWSEREDITWIN+0)

/* Attributes */
#define MUIA_BrowserEditWin_Browser (TAGBASE_BROWSEREDITWIN+0)
#define MUIA_BrowserEditWin_ListObj (TAGBASE_BROWSEREDITWIN+1)

/**************************************************************************/
/*
** MailerEditWin class
*/

#ifdef __AROS__
#define mailerEditWinObject BOOPSIOBJMACRO_START(g_mailerEditWinClass->mcc_Class)
#else
#define mailerEditWinObject NewObject(g_mailerEditWinClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_MailerEditWin_Use (TAGBASE_MAILEREDITWIN+0)

/* Attributes */
#define MUIA_MailerEditWin_Mailer  (TAGBASE_MAILEREDITWIN+0)
#define MUIA_MailerEditWin_ListObj (TAGBASE_MAILEREDITWIN+1)

/**************************************************************************/
/*
** FTPEditWin class
*/

#ifdef __AROS__
#define FTPEditWinObject BOOPSIOBJMACRO_START(g_FTPEditWinClass->mcc_Class)
#else
#define FTPEditWinObject NewObject(g_FTPEditWinClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_FTPEditWin_Use (TAGBASE_FTPEDITWIN+0)

/* Attributes */
#define MUIA_FTPEditWin_FTP     (TAGBASE_FTPEDITWIN+0)
#define MUIA_FTPEditWin_ListObj (TAGBASE_FTPEDITWIN+1)

/**************************************************************************/
/*
** Popport class
*/

#ifdef __AROS__
#define popportObject BOOPSIOBJMACRO_START(g_popportClass->mcc_Class)
#else
#define popportObject NewObject(g_popportClass->mcc_Class,NULL
#endif

/* Attributes */
#define MUIA_Popport_Key     (TAGBASE_POPPORT+0)
#define MUIA_Popport_Len     (TAGBASE_POPPORT+1)

/**************************************************************************/
/*
** Lamp class
*/

/* Attributes */
#define MUIA_Lamp_Disabled (TAGBASE_LAMP)

/**************************************************************************/
/*
** Popph class
*/

#ifdef __AROS__
#define popphObject BOOPSIOBJMACRO_START(g_popphClass->mcc_Class)
#else
#define popphObject NewObject(g_popphClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_Popph_RequestFile  (TAGBASE_POPPH)

/* Attributes */
#define MUIA_Popph_Syms         (TAGBASE_POPPH)
#define MUIA_Popph_Names        (TAGBASE_POPPH+1)
#define MUIA_Popph_StrObj       (TAGBASE_POPPH+2)
#define MUIA_Popph_MaxLen       (TAGBASE_POPPH+3)
#define MUIA_Popph_Key          (TAGBASE_POPPH+4)
#define MUIA_Popph_Asl          (TAGBASE_POPPH+5)

/**************************************************************************/
/*
** Pens class
*/

#ifdef __AROS__
#define pensObject BOOPSIOBJMACRO_START(g_pensClass->mcc_Class)
#else
#define pensObject NewObject(g_pensClass->mcc_Class,NULL
#endif

/* Methods */
#define MUIM_Pens_Change        (TAGBASE_PENS)

/**************************************************************************/
/*
** Various
*/

enum
{
    IBT_Up,
    IBT_Down,
};

/* Long story */
#ifndef MUIA_Window_AllowTopMenus
#define MUIA_Window_AllowTopMenus 0x8042fe69
#endif

#define DEF_EnabledPen  "m6"
#define DEF_DisabledPen "m1"
#define DEF_DetailPen   "m4"

/**************************************************************************/
/*
** Macros
**/

#define MTITLE(t) {NM_TITLE,(STRPTR)(t),0,0,0,(APTR)(t)}
#define MTITEM(t) {NM_ITEM,(STRPTR)(t),0,CHECKIT|MENUTOGGLE,0,(APTR)(t)}
#define MITEM(t)  {NM_ITEM,(STRPTR)(t),0,0,0,(APTR)(t)}
#define MBAR      {NM_ITEM,(STRPTR)NM_BARLABEL,0,0,0,NULL}
#define MEND      {NM_END,NULL,0,0,0,NULL}

#undef set
#undef get
#undef nnset
#undef nfset
#define get(obj,attr,store)            GetAttr((ULONG)(attr),(Object *)obj,(ULONG *)(store))
#define set(obj,attr,value)            SetAttrs((Object *)(obj),(ULONG)(attr),(ULONG)(value),TAG_DONE)
#define nnset(obj,attr,value)          SetAttrs((Object *)(obj),MUIA_NoNotify,TRUE,(ULONG)(attr),(ULONG)(value),TAG_DONE)
#define superset(cl,obj,attr,value)    SetSuperAttrs((APTR)(cl),(Object *)(obj),(ULONG)(attr),(ULONG)(value),TAG_DONE)
#define supernnset(cl,obj,attr,value)  SetSuperAttrs((APTR)(cl),(Object *)(obj),(ULONG)(attr),(ULONG)(value),MUIA_NoNotify,TRUE,TAG_DONE)
#define superget(cl,obj,attr,valPtr)   DoSuperMethod((APTR)(cl),(Object *)(obj),OM_GET,(ULONG)(attr),(ULONG)(valPtr))
#define nfset(obj,attr,value)          SetAttrs((Object *)(obj),(ULONG)(attr),(ULONG)(value),MUIA_Group_Forward,FALSE,TAG_DONE)

#define wspace(w) RectangleObject, MUIA_Weight, w, End

/***********************************************************************/
/*
** Protos
*/

#include "OpenURL_protos.h"

/**************************************************************************/
