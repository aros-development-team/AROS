/* SysLog main file */

#if !defined(__AROS__)
#define __NOLIBBASE__
#endif

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/locale.h>
#include <clib/alib_protos.h>

#include <libraries/mui.h>

#include <proto/miami.h>
#include <proto/socket.h>

#define ioctl IoctlSocket

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <signal.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netdb.h>
#include <syslog.h>

#include "hooks.h"
#include "main.h"
#include "colorlist.h"


extern struct Library 	*UtilityBase;

#if !defined(__AROS__)
extern struct Library 	*SysBase,
									*DOSBase;
#endif

struct Library             	*MUIMasterBase,
									*GfxBase;

#if !defined(__AROS__)
struct Library             	*IntuitionBase,
									*LocaleBase;
#endif
									
Object *App, *Win, *SWin, *List, *DbgLevel;

struct MsgPort *Port;

struct Locale *Locale;

STRPTR DebugLevels[] = {"emergency", "alerts", "critical", "errors", "warnings", "notice", "info", "debug", "off", NULL};

BOOL Setup(void)
{
  if (!(GfxBase = OpenLibrary("graphics.library", 0))) return FALSE;
  if (!(IntuitionBase = OpenLibrary("intuition.library", 0))) return FALSE;
  if (!(MUIMasterBase = OpenLibrary("muimaster.library", 0))) return FALSE;
  if (!(LocaleBase = OpenLibrary("locale.library", 0))) return FALSE;
  if (!(ColorList = CreateColorListClass())) return FALSE;
  Locale = OpenLocale(NULL);
  if (FindPort("SysLog")) return FALSE;
  return TRUE;
}

void Cleanup(void)
{
  CloseLocale(Locale);
  if (ColorList) MUI_DeleteCustomClass(ColorList);
  if (LocaleBase) CloseLibrary(LocaleBase);
  if (MUIMasterBase) CloseLibrary(MUIMasterBase);
  if (IntuitionBase) CloseLibrary(IntuitionBase);
  if (GfxBase) CloseLibrary(GfxBase);
}

#define MUIA_InnerSpacing(a) MUIA_InnerLeft, a,\
 MUIA_InnerRight, a,\
 MUIA_InnerTop, a,\
 MUIA_InnerBottom, a

Object *BuildSettingsWindow(void)
{
  Object *win;

  win = MUI_NewObject(MUIC_Window,
	 MUIA_Window_Title, (ULONG)"SysLog settings",
   MUIA_Window_ID, 0x4D444C50,
   MUIA_Window_RootObject, (ULONG)MUI_NewObject(MUIC_Group,
    MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Group,
     MUIA_Group_Columns, 2,
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Text,
      MUIA_Text_Contents, (ULONG)"\33rColor of error messages",
      MUIA_Weight, 0,
     TAG_END),
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Poppen,
      MUIA_FixWidthTxt, (ULONG)"6EDA",
      MUIA_InnerSpacing(1),
      MUIA_ObjectID, 1,
      MUIA_UserData, PREFS_POPPEN_ERRORS,
      MUIA_Window_Title, (ULONG)"Error messages",
     TAG_END),
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Text,
      MUIA_Text_Contents, (ULONG)"\33rColor of important information",
      MUIA_Weight, 0,
     TAG_END),
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Poppen,
      MUIA_FixWidthTxt, (ULONG)"6EDA",
      MUIA_InnerSpacing(1),
      MUIA_ObjectID, 2,
      MUIA_UserData, PREFS_POPPEN_IMPORTANT,
      MUIA_Window_Title, (ULONG)"Important information",
     TAG_END),
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Text,
      MUIA_Text_Contents, (ULONG)"\33rColor of debug information",
      MUIA_Weight, 0,
     TAG_END),
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Poppen,
      MUIA_FixWidthTxt, (ULONG)"6EDA",
      MUIA_InnerSpacing(1),
      MUIA_ObjectID, 3,
      MUIA_UserData, PREFS_POPPEN_OTHERS,
      MUIA_Window_Title, (ULONG)"Debug information",
     TAG_END),
    TAG_END),
    MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Group,
     MUIA_Group_Horiz, TRUE,
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Text,
      MUIA_Text_Contents, (ULONG)"Save",
      MUIA_Text_PreParse, (ULONG)"\33c",
      MUIA_Text_HiChar, 's',
      MUIA_ControlChar, 's',
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_Background, MUII_ButtonBack,
      MUIA_Frame, MUIV_Frame_Button,
      MUIA_Font, MUIV_Font_Button,
      MUIA_UserData, PREFS_BUTTON_SAVE,
     TAG_END),
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Text,
      MUIA_Text_Contents, (ULONG)"Use",
      MUIA_Text_PreParse, (ULONG)"\33c",
      MUIA_Text_HiChar, 'u',
      MUIA_ControlChar, 'u',
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_Background, MUII_ButtonBack,
      MUIA_Frame, MUIV_Frame_Button,
      MUIA_Font, MUIV_Font_Button,
      MUIA_UserData, PREFS_BUTTON_USE,
     TAG_END),
     MUIA_Group_Child, (ULONG)MUI_NewObject(MUIC_Text,
      MUIA_Text_Contents, (ULONG)"Cancel",
      MUIA_Text_PreParse, (ULONG)"\33c",
      MUIA_Text_HiChar, 'c',
      MUIA_ControlChar, 'c',
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_Background, MUII_ButtonBack,
      MUIA_Frame, MUIV_Frame_Button,
      MUIA_Font, MUIV_Font_Button,
      MUIA_UserData, PREFS_BUTTON_CANCEL,
     TAG_END),
    TAG_END),
   TAG_END),
  TAG_END);

  return win;
}


void MainLoop(Object *win)
{
  ULONG signals, portmask;

  SetAttrs(win, MUIA_Window_Open, TRUE, TAG_END);
  portmask = 1 << Port->mp_SigBit;

  while (DoMethod(App, MUIM_Application_NewInput, (ULONG)&signals) != (ULONG)MUIV_Application_ReturnID_Quit)
  {
    if (signals)
    {
      signals = Wait(signals | portmask | SIGBREAKF_CTRL_C);
      if (signals & portmask)
      {
	struct SysLogPacket *slp;

	while (slp = (struct SysLogPacket*)GetMsg(Port))
        {
		if (slp->Level != LOG_CLOSE)
		{
			DoMethod(List, MUIM_List_InsertSingle, (ULONG)slp, MUIV_List_Insert_Bottom);
			DoMethod(List, MUIM_List_Jump, MUIV_List_Jump_Bottom, TAG_END);
		}
		ReplyMsg(&slp->Msg);
        }
      }
      if (signals & SIGBREAKF_CTRL_C) break;
    }
  }

  SetAttrs(win, MUIA_Window_Open, FALSE, TAG_END);
  return;
}


/* Settings hook */
#if !defined(__AROS__)
LONG SettingsHook(void)
{
#else
AROS_UFH3(
    void, SettingsHook,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
    AROS_USERFUNC_INIT
#endif

    // Get data
  SetAttrs(Win, MUIA_Window_Sleep, TRUE, TAG_END);
  MainLoop(SWin);
  SetAttrs(Win, MUIA_Window_Sleep, FALSE, TAG_END);
#if !defined(__AROS__)
  return 0;
#else
    AROS_USERFUNC_EXIT
#endif
}


#if !defined(__AROS__)
struct EmulLibEntry g_SettingsHook = {TRAP_LIB, 0, (void (*)(void))SettingsHook};
struct Hook h_SettingsHook = {{NULL, NULL}, (HOOKFUNC)&g_SettingsHook, NULL, NULL};
#else
struct Hook               h_SettingsHook;
#endif

Object* CreateMenu(void)
{
  Object *o;

  o = MUI_NewObject(MUIC_Menustrip,
   MUIA_Family_Child, (ULONG)MUI_NewObject(MUIC_Menu,
    MUIA_Menu_Title, (ULONG)"Project",
    MUIA_Family_Child, (ULONG)MUI_NewObject(MUIC_Menuitem,
     MUIA_Menuitem_Title, (ULONG)"Clear",
     MUIA_Menuitem_Shortcut, (ULONG)"C",
     MUIA_UserData, MENU_CLEAR,
    TAG_END),
/*  MUIA_Family_Child, (ULONG)MUI_NewObject(MUIC_Menuitem,
     MUIA_Menuitem_Title, (ULONG)"Save as...",
     MUIA_Menuitem_Shortcut, (ULONG)"S",
     MUIA_UserData, MENU_SAVEAS,
    TAG_END),*/
    MUIA_Family_Child, (ULONG)MUI_NewObject(MUIC_Menuitem,
     MUIA_Menuitem_Title, (ULONG)"Settings...",
     MUIA_UserData, MENU_SETTINGS,
    TAG_END),
    MUIA_Family_Child, (ULONG)MUI_NewObject(MUIC_Menuitem,
     MUIA_Menuitem_Title, MENUBAR,
    TAG_END),
    MUIA_Family_Child, (ULONG)MUI_NewObject(MUIC_Menuitem,
     MUIA_Menuitem_Title, (ULONG)"Quit",
     MUIA_Menuitem_Shortcut, (ULONG)"Q",
     MUIA_UserData, MENU_QUIT,
    TAG_END),
   TAG_END),
  TAG_END);

  return o;
}


BOOL BuildGui(void)
{
  Object *lv;

  App = MUI_NewObject(MUIC_Application,
   MUIA_Application_Author, (ULONG)"Grzegorz \"Krashan\" Kraszewski, Pavel \"Sonic\" Fedin, Nick \"Kalamatee\" Andrews",
   MUIA_Application_Base, (ULONG)"SysLog",
	 MUIA_Application_Copyright, (ULONG)"(c) 2005 by Grzegorz Kraszewski, Pavel Fedin, Nick Andrews",
	 MUIA_Application_Description, (ULONG)"AROSTCP syslog viewer",
   MUIA_Application_SingleTask, TRUE,
	 MUIA_Application_Title, (ULONG)"SysLog",
	 MUIA_Application_Version, (ULONG)"$VER: SysLog 1.2 (15.11.2005)\n",
   MUIA_Application_Menustrip, (ULONG)CreateMenu(),
   MUIA_Application_Window, (ULONG)(Win = MUI_NewObject(MUIC_Window,
		MUIA_Window_Title, (ULONG)"AROSTCP syslog viewer",
		MUIA_Window_ScreenTitle, (ULONG)"SysLog 1.2 (c) 2005-2007 Grzegorz Kraszewski, Pavel Fedin, Nick Andrews",
    MUIA_Window_ID, 0x4D444C47,
    MUIA_Window_RootObject, (ULONG)MUI_NewObject(MUIC_Group,
     MUIA_Group_Child, (ULONG)(lv = MUI_NewObject(MUIC_Listview,
      MUIA_Listview_List, (ULONG)(List = NewObject(ColorList->mcc_Class, NULL,
       MUIA_Frame, MUIV_Frame_ReadList,
			 MUIA_List_Format, (ULONG)"BAR,BAR,BAR,",
       MUIA_List_Title, TRUE,
      TAG_END)),
      MUIA_Listview_Input, FALSE,
     TAG_END)),
     MUIA_Group_Child, MUI_NewObject(MUIC_Group,
      MUIA_Group_Horiz, TRUE,
      MUIA_Group_Child, MUI_NewObject(MUIC_Text,
       MUIA_Text_SetMax, TRUE,
       MUIA_Text_Contents, (ULONG)"Debug level",
      TAG_END),
      MUIA_Group_Child, DbgLevel = MUI_NewObject(MUIC_Cycle,
       MUIA_Cycle_Entries, (ULONG)DebugLevels,
       MUIA_Weight, 1,
      TAG_END),
      MUIA_Group_Child, MUI_NewObject(MUIC_Rectangle,
      TAG_END),
     TAG_END),
    TAG_END),
    MUIA_Window_DefaultObject, (ULONG)lv,
   TAG_END)),
   MUIA_Application_Window, SWin = BuildSettingsWindow(),
  TAG_END);

  if (App) return TRUE;
  else return FALSE;
}


void SetNotifications (void)
{
#if defined(__AROS__)
	h_SettingsHook.h_Entry = ( HOOKFUNC )SettingsHook;
#endif
  DoMethod(Win, MUIM_Notify, MUIA_Window_CloseRequest, MUIV_EveryTime, (ULONG)App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
  DoMethod(App, MUIM_Notify, MUIA_Application_MenuAction, MENU_QUIT, (ULONG)App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
/*DoMethod(App, MUIM_Notify, MUIA_Application_MenuAction, MENU_SAVEAS, (ULONG)List, 2, MUIM_CallHook, (ULONG)&h_SaveAsHook);*/
  DoMethod(App, MUIM_Notify, MUIA_Application_MenuAction, MENU_SETTINGS, (ULONG)App, 2, MUIM_CallHook, (ULONG)&h_SettingsHook);
  DoMethod(App, MUIM_Notify, MUIA_Application_MenuAction, MENU_CLEAR,
   (ULONG)List, 1, MUIM_List_Clear);

  /* close prefs window */

  DoMethod(SWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, App, 2,
   MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
  DoMethod(findobj(SWin, PREFS_BUTTON_CANCEL), MUIM_Notify, MUIA_Pressed,
   FALSE, App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
  DoMethod(findobj(SWin, PREFS_BUTTON_USE), MUIM_Notify, MUIA_Pressed,
   FALSE, App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
  DoMethod(findobj(SWin, PREFS_BUTTON_SAVE), MUIM_Notify, MUIA_Pressed,
   FALSE, App, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

  /* saving prefs */

  DoMethod(findobj(SWin, PREFS_BUTTON_USE), MUIM_Notify, MUIA_Pressed,
   FALSE, App, 2, MUIM_Application_Save, MUIV_Application_Save_ENV);
  DoMethod(findobj(SWin, PREFS_BUTTON_SAVE), MUIM_Notify, MUIA_Pressed,
   FALSE, App, 2, MUIM_Application_Save, MUIV_Application_Save_ENV);
  DoMethod(findobj(SWin, PREFS_BUTTON_SAVE), MUIM_Notify, MUIA_Pressed,
   FALSE, App, 2, MUIM_Application_Save, MUIV_Application_Save_ENVARC);

  /* list redisplay */

  DoMethod(findobj(SWin, PREFS_BUTTON_USE), MUIM_Notify, MUIA_Pressed,
   FALSE, List, 1, CLL_ChangePens);
  DoMethod(findobj(SWin, PREFS_BUTTON_SAVE), MUIM_Notify, MUIA_Pressed,
   FALSE, List, 1, CLL_ChangePens);

}


int main(void)
{
  struct Library *MiamiBase;
  if (Setup())
  {
    if (Port = CreateMsgPort())
    {
      struct Message *m;

      Port->mp_Node.ln_Name = "SysLog";
      Port->mp_Node.ln_Pri = 12;
      AddPort(Port);
      MiamiBase = OpenLibrary("miami.library",13);
      if (MiamiBase) {
	SetSysLogPort();
	CloseLibrary(MiamiBase);
      }
      if (BuildGui())
      {
        DoMethod(App, MUIM_Application_Load, MUIV_Application_Load_ENV);
        SetNotifications();
        MainLoop(Win);
        MUI_DisposeObject(App);
      }
      RemPort(Port);
      MiamiBase = OpenLibrary("miami.library",13);
      if (MiamiBase) {
	SetSysLogPort();
	CloseLibrary(MiamiBase);
      }
      while (m = GetMsg(Port)) ReplyMsg(m);
      DeleteMsgPort(Port);
    }
  }
  else Printf("Setup() failed.\n");
  Cleanup();
  return 0;
}
