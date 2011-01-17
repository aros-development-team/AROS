/*
     AHI - The AHI preferences program
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.
     
     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.
     
     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


//#define NO_INLINE_STDARG

#include <config.h>

#include <libraries/asl.h>
#include <libraries/bgui.h>
#include <libraries/bgui_macros.h>
#include <proto/bgui.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <math.h>
#include <string.h>

#include "ahi.h"
#include "ahiprefs_Cat.h"
#include "support.h"
#include "gui.h"

static void GUINewSettings(void);
static void GUINewUnit(void);
static void GUINewMode(void);


enum windowIDs {
  WINID_MAIN=1,
  WINID_COUNT
};

enum actionIDs {
  ACTID_OPEN=1, ACTID_SAVEAS, ACTID_ABOUT, ACTID_QUIT,
  ACTID_DEFAULT, ACTID_LASTSAVED, ACTID_RESTORE,
  ACTID_ICONS,
  ACTID_HELP, ACTID_GUIDE, ACTID_HELPINDEX,
  ACTID_SAVE, ACTID_USE,

  ACTID_TABS, ACTID_PAGE,

  ACTID_UNIT, ACTID_MODE, 
  SHOWID_MODE,

  ACTID_FREQ, ACTID_CHANNELS, ACTID_OUTVOL, ACTID_MONVOL, ACTID_GAIN,
  ACTID_INPUT, ACTID_OUTPUT,
  SHOWID_FREQ, SHOWID_CHANNELS, SHOWID_OUTVOL, SHOWID_MONVOL, SHOWID_GAIN,
  SHOWID_INPUT, SHOWID_OUTPUT,

  ACTID_DEBUG, ACTID_SURROUND, ACTID_ECHO, ACTID_CLIPMV,
  ACTID_CPULIMIT, SHOWID_CPULIMIT,
  

  ACTID_COUNT
};

#define ItCk(t,s,i,f) { NM_ITEM, t, s, f, 0, (APTR)i }
#define HelpNode(n) BT_HelpFile, HELPFILE, BT_HelpNode, n

struct Library       *BGUIBase  = NULL;
static struct Window *window    = NULL;
static Object        *WO_Window = NULL;
static Object        *Window_Objs[ACTID_COUNT];
static Object        *openreq   = NULL;
static Object        *savereq   = NULL;
static Object        *vgroup    = NULL;  // To replace the unit cycle gadget
struct TR_Project *Project = NULL;

static BOOL PopUpMenus = TRUE;         // Turn cycle gadgets into popup menus?

static const struct TagItem  pagemap[] = {
  { MX_Active, PAGE_Active },
  { TAG_END, NULL }
};

static ULONG cpumap[] = { SLIDER_Level,  INDIC_Level, TAG_END };

static struct NewMenu Menus[] = {
  Title( NULL /* Project */ ),
    Item( NULL /* Open... */,             NULL, ACTID_OPEN      ),
    Item( NULL /* Save As... */,          NULL, ACTID_SAVEAS    ),
    ItemBar,
    Item( NULL /* About... */,            NULL, ACTID_ABOUT     ),
    ItemBar,
    Item( NULL /* Quit */,                NULL, ACTID_QUIT      ),
  Title( NULL /* Edit */ ),
    Item( NULL /* Reset To Defaults */,   NULL, ACTID_DEFAULT   ),
    Item( NULL /* Last Saved */,          NULL, ACTID_LASTSAVED ),
    Item( NULL /* Restore */,             NULL, ACTID_RESTORE   ),
  Title( NULL /* Settings */ ),
    ItCk( NULL /* Create Icons? */,       NULL, ACTID_ICONS, CHECKIT|MENUTOGGLE ),
  Title( NULL /* Help */ ),
    ItCk( NULL /* Help... */,             NULL, ACTID_HELP, COMMSEQ),
    ItemBar,
    Item( NULL /* AHI User's guide... */, NULL, ACTID_GUIDE),
    Item( NULL /* Concept Index... */,    NULL, ACTID_HELPINDEX ),
  End
};

static char *PageNames[] =
{
	NULL,  /* Mode settings */
	NULL,  /* Advanced settings */
	NULL
};

static char * DebugLabels[] = {
  NULL,  /* None */
  NULL,  /* Low */
  NULL,  /* High */
  NULL,  /* Full */
  NULL
};

static char * EchoLabels[] = {
  NULL,  /* Enabled */
  NULL,  /* Fast */
  NULL,  /* Disabled */
  NULL
};

static char * SurroundLabels[] = {
  NULL,  /* Enabled */
  NULL,  /* Disabled */
  NULL
};

static char * ClipMVLabels[] = {
  NULL,  /* Without clipping */
  NULL,  /* With clipping */
  NULL
};

/***** Local function to update the strings above ****************************/

static void UpdateStrings(void) {
  char ** strings[] =
  {
    (char**) &msgMenuProject,
    (char**) &msgItemOpen,
    (char**) &msgItemSaveAs,
    (char**) &msgItemAbout,
    (char**) &msgItemQuit,
    (char**) &msgMenuEdit,
    (char**) &msgItemDefaults,
    (char**) &msgItemLastSaved,
    (char**) &msgItemRestore,
    (char**) &msgMenuSettings,
    (char**) &msgItemCreateIcons,
    (char**) &msgMenuHelp,
    (char**) &msgItemHelp,
    (char**) &msgItemUsersGuide,
    (char**) &msgItemConceptIndex
  };

  struct NewMenu   *menuptr;
  char           ***stringptr;
  
  menuptr   = (struct NewMenu *) &Menus;
  stringptr = (char ***) &strings;

  while(menuptr->nm_Type != NM_END)
  {
    if(menuptr->nm_Label == NULL)
    {
      if(strlen(**stringptr) != 0) {
        menuptr->nm_CommKey = **stringptr;
      }
      menuptr->nm_Label = **stringptr + strlen(**stringptr) + 1;
      stringptr++;
    }
    menuptr++;
  }


  PageNames[0] = (char *) msgPageMode;
  PageNames[1] = (char *) msgPageAdvanced;
  DebugLabels[0] = (char *) msgDebugNone;
  DebugLabels[1] = (char *) msgDebugLow;
  DebugLabels[2] = (char *) msgDebugHigh;
  DebugLabels[3] = (char *) msgDebugFull;
  EchoLabels[0] = (char *) msgEchoEnabled;
  EchoLabels[1] = (char *) msgEchoFast;
  EchoLabels[2] = (char *) msgEchoDisabled;
  SurroundLabels[0] = (char *) msgSurroundEnabled;
  SurroundLabels[1] = (char *) msgSurroundDisabled;
  ClipMVLabels[0] = (char *) msgMVNoClip;
  ClipMVLabels[1] = (char *) msgMVClip;

}

/***** Local function to update a slider indicator ***************************/

static void UpdateSliderLevel(int src, int dst, LONG *index, char * (*func)(void)) {
  if(GetAttr( SLIDER_Level, Window_Objs[src], (ULONG *) index)) {
    SetGadgetAttrs((struct Gadget *) Window_Objs[dst],
        window, NULL,
        INFO_TextFormat, (ULONG) (*(func))(), 
        TAG_DONE );
  }
}

/***** Local function to find a menu item ************************************/

static struct MenuItem *FindMenuItem(ULONG id) {
  struct Menu     *menu = NULL;
  struct MenuItem *item;

  GetAttr( WINDOW_MenuStrip, WO_Window, (ULONG *) &menu);
  while(menu) {
    for(item = menu->FirstItem; item; item = item->NextItem) {
      if(GTMENUITEM_USERDATA(item) == (APTR) id) {
        return item;
      }
    }
    menu = menu->NextMenu;
  }

  return NULL;
}

/***** Local function to update the unit gadget (and everything below) *******/

static void GUINewSettings(void) {
  Object *oldcycle;
  Object *cycle = CycleObject,
      CYC_Labels,    Units,
      CYC_Active,    state.UnitSelected,
      CYC_PopActive, PopUpMenus,
      CYC_Popup,     PopUpMenus,
      GA_ID,         ACTID_UNIT,
  EndObject;

  if(cycle) {
    oldcycle = (Object *) DoMethod( vgroup, GRM_REPLACEMEMBER,
      Window_Objs[ACTID_UNIT], cycle, FixMinHeight, TAG_DONE);
    if(oldcycle) {
      DisposeObject(oldcycle);
      Window_Objs[ACTID_UNIT] = cycle;
    }
    else {
      DisposeObject(cycle);
    }
  }

  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_DEBUG], window, NULL,
      CYC_Active, globalprefs.ahigp_DebugLevel, TAG_DONE);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_SURROUND], window, NULL,
      CYC_Active, globalprefs.ahigp_DisableSurround, TAG_DONE);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_ECHO], window, NULL,
      CYC_Active, (globalprefs.ahigp_DisableEcho ? 2 : 0) | 
                  (globalprefs.ahigp_FastEcho    ? 1 : 0),     TAG_DONE);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_CLIPMV], window, NULL,
      CYC_Active, globalprefs.ahigp_ClipMasterVolume, TAG_DONE);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_CPULIMIT], window, NULL,
      SLIDER_Level, (globalprefs.ahigp_MaxCPU * 100 + 32768) >> 16, TAG_DONE);

  GUINewUnit();
}

/***** Local function to update all gadgets that depend on the unit **********/

static void GUINewUnit(void) {
  DoMethod( Window_Objs[ACTID_MODE], LVM_CLEAR, NULL );
  DoMethod( Window_Objs[ACTID_MODE], LVM_ADDENTRIES, NULL, Modes, LVAP_HEAD);
  BGUI_DoGadgetMethod( Window_Objs[ACTID_MODE], window, NULL,
    LVM_REFRESH );
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MODE], window, NULL,
    LISTV_Select,      state.ModeSelected,
    TAG_DONE );
  GUINewMode();
}

/***** Local function to update all gadgets that depend on the mode **********/

static char *infoargs[6];

static void GUINewMode(void) {
  int Max, Sel;

  infoargs[0] = (char *) getAudioMode();
  infoargs[1] = getRecord();
  infoargs[2] = getAuthor();
  infoargs[3] = getCopyright();
  infoargs[4] = getDriver();
  infoargs[5] = getVersion();

  SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_MODE], window, NULL,
      INFO_TextFormat, (ULONG) "0x%08lx\n"
                       "%s\n"
                       "%s\n"
                       "%s\n"
                       "Devs:AHI/%s.audio\n"
                       "%s",
      INFO_Args,       (ULONG) &infoargs,
      TAG_DONE );

  Max = max(state.Frequencies -1, 0);
  Sel = min(Max, state.FreqSelected);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_FREQ], window, NULL,
      SLIDER_Min,   0,
      SLIDER_Max,   Max,
      SLIDER_Level, Sel,
      GA_Disabled,  (Max == 0),
      TAG_DONE );
  Max = max(state.Channels, 0);
  Sel = min(Max, state.ChannelsSelected);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_CHANNELS], window, NULL,
      SLIDER_Min,   1,
      SLIDER_Max,   Max,
      SLIDER_Level, Sel,
      GA_Disabled,  (Max == 1) || state.ChannelsDisabled,
      TAG_DONE );
  Max = max(state.OutVols -1, 0);
  Sel = min(Max, state.OutVolSelected);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_OUTVOL], window, NULL,
      SLIDER_Min,   0,
      SLIDER_Max,   Max,
      SLIDER_Level, Sel,
      GA_Disabled,  (Max == 0),
      TAG_DONE );
  Max = max(state.MonVols -1, 0);
  Sel = min(Max, state.MonVolSelected);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MONVOL], window, NULL,
      SLIDER_Min,   0,
      SLIDER_Max,   Max,
      SLIDER_Level, Sel,
      GA_Disabled,  (Max == 0),
      TAG_DONE );
  Max = max(state.Gains -1, 0);
  Sel = min(Max, state.GainSelected);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_GAIN], window, NULL,
      SLIDER_Min,   0,
      SLIDER_Max,   Max,
      SLIDER_Level, Sel,
      GA_Disabled,  (Max == 0),
      TAG_DONE );
  Max = max(state.Inputs -1, 0);
  Sel = min(Max, state.InputSelected);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_INPUT], window, NULL,
      SLIDER_Min,   0,
      SLIDER_Max,   Max,
      SLIDER_Level, Sel,
      GA_Disabled,  (Max == 0),
      TAG_DONE );
  Max = max(state.Outputs -1, 0);
  Sel = min(Max, state.OutputSelected);
  SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_OUTPUT], window, NULL,
      SLIDER_Min,   0,
      SLIDER_Max,   Max,
      SLIDER_Level, Sel,
      GA_Disabled,  (Max == 0),
      TAG_DONE );

    // Update indicators..

    SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_FREQ], window, NULL,
        INFO_TextFormat, (ULONG) getFreq(), TAG_DONE );

    SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_CHANNELS], window, NULL,
        INFO_TextFormat, (ULONG) getChannels(), TAG_DONE );

    SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_OUTVOL], window, NULL,
        INFO_TextFormat, (ULONG) getOutVol(), TAG_DONE );

    SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_MONVOL], window, NULL,
        INFO_TextFormat, (ULONG) getMonVol(), TAG_DONE );

    SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_GAIN], window, NULL,
        INFO_TextFormat, (ULONG) getGain(), TAG_DONE );

    SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_INPUT], window, NULL,
        INFO_TextFormat, (ULONG) getInput(), TAG_DONE );

    SetGadgetAttrs((struct Gadget *) Window_Objs[SHOWID_OUTPUT], window, NULL,
        INFO_TextFormat, (ULONG) getOutput(), TAG_DONE );

}

/***** Gadget hook ***********************************************************/

static ULONG HOOKCALL
GadgetHookFunc( REG( a0, struct Hook *hook ),
                REG( a2, Object *obj ),
                REG( a1, struct opUpdate *opu ) ) {

  if(obj == Window_Objs[ ACTID_FREQ] ) {
    UpdateSliderLevel(ACTID_FREQ, SHOWID_FREQ,
        &state.FreqSelected, getFreq);
  }

  else if(obj == Window_Objs[ ACTID_CHANNELS] ) {
    UpdateSliderLevel(ACTID_CHANNELS, SHOWID_CHANNELS,
        &state.ChannelsSelected, getChannels);
  }

  else if(obj == Window_Objs[ ACTID_OUTVOL] ) {
    UpdateSliderLevel(ACTID_OUTVOL, SHOWID_OUTVOL,
        &state.OutVolSelected, getOutVol);
  }

  else if(obj == Window_Objs[ ACTID_MONVOL] ) {
    UpdateSliderLevel(ACTID_MONVOL, SHOWID_MONVOL,
        &state.MonVolSelected, getMonVol);
  }

  else if(obj == Window_Objs[ ACTID_GAIN] ) {
    UpdateSliderLevel(ACTID_GAIN, SHOWID_GAIN,
        &state.GainSelected, getGain);
  }

  else if(obj == Window_Objs[ ACTID_INPUT] ) {
    UpdateSliderLevel(ACTID_INPUT, SHOWID_INPUT,
        &state.InputSelected, getInput);
  }

  else if(obj == Window_Objs[ ACTID_OUTPUT] ) {
    UpdateSliderLevel(ACTID_OUTPUT, SHOWID_OUTPUT,
        &state.OutputSelected, getOutput);
  }
  else return 0;

	return 1;
}

static struct Hook GadgetHook =
{ 
  { NULL,NULL },
  (HOOKFUNC) GadgetHookFunc,
  NULL,
  NULL
};

static void HOOKCALL
IDCMPhookFunc( REG( a0, struct Hook *hook ),
               REG( a2, Object *obj ),
               REG( a1, struct IntuiMessage *msg) )
{
  switch(msg->Code) {

    case 0x42:
    {
      /*
      **      TAB             - Next page
      **      SHIFT + TAB     - Prev page
      */
      ULONG          pos = 0;

      GetAttr( MX_Active,  Window_Objs[ACTID_TABS], &pos );

      if ( msg->Qualifier & ( IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT ))
        pos--;
      else 
        pos++;
      SetGadgetAttrs(( struct Gadget * ) Window_Objs[ACTID_TABS], window, NULL,
          MX_Active, pos, TAG_END );
      break;
    }

    case 0x4C:
    {
      /*
      **      UP              - Move entry up.
      **      SHIFT + UP      - Move page up.
      **      CTRL + UP       - Move to the top.
      **/
      ULONG mode = 0;

      if(msg->Qualifier & ( IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT ))
        SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MODE], window, NULL,
            LISTV_Select, LISTV_Select_Page_Up, TAG_END );
      else if(msg->Qualifier & IEQUALIFIER_CONTROL )
        SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MODE], window, NULL,
            LISTV_Select, LISTV_Select_First, TAG_END );
      else
        SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MODE], window, NULL,
            LISTV_Select, LISTV_Select_Previous, TAG_END );

      GetAttr( LISTV_LastClickedNum, Window_Objs[ACTID_MODE], &mode);
      FillUnit();
      NewMode(mode);
      GUINewMode();
      break;
    }

    case 0x4D:
    {
      /*
      **      DOWN            - Move entry down.
      **      SHIFT + DOWN    - Move page down.
      **      CTRL + DOWN     - Move to the end.
      **/
      ULONG mode = 0;

      if(msg->Qualifier & ( IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT ))
        SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MODE], window, NULL,
            LISTV_Select, LISTV_Select_Page_Down, TAG_END );
      else if(msg->Qualifier & IEQUALIFIER_CONTROL )
        SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MODE], window, NULL,
            LISTV_Select, LISTV_Select_Last, TAG_END );
      else
        SetGadgetAttrs((struct Gadget *) Window_Objs[ACTID_MODE], window, NULL,
            LISTV_Select, LISTV_Select_Next, TAG_END );

      GetAttr( LISTV_LastClickedNum, Window_Objs[ACTID_MODE], &mode);
      FillUnit();
      NewMode(mode);
      GUINewMode();
      break;
    }

    case 0x4f:
    {
      /*
      **      LEFT            - Prev unit
      */
      LONG pos = 0;
      int i;
      char **u = Units;

      for(i=0; *u; i++, u++);

      GetAttr( CYC_Active,  Window_Objs[ACTID_UNIT], (ULONG *) &pos );

      pos = (pos - 1) % i;
      if(pos < 0) pos += i;

      SetGadgetAttrs(( struct Gadget * ) Window_Objs[ACTID_UNIT], window, NULL,
          CYC_Active, pos, TAG_END );

      GetAttr( CYC_Active, Window_Objs[ACTID_UNIT], (ULONG *) &pos);
      FillUnit();
      NewUnit(pos);
      GUINewUnit();
      break;
    }

    case 0x4e:
    {
      /*
      **      RIGHT           - Prev unit
      */
      LONG pos = 0;
      int i;
      char **u = Units;

      for(i=0; *u; i++, u++);

      GetAttr( CYC_Active,  Window_Objs[ACTID_UNIT], (ULONG *) &pos );

      pos = (pos + 1) % i;;

      SetGadgetAttrs(( struct Gadget * ) Window_Objs[ACTID_UNIT], window, NULL,
          CYC_Active, pos, TAG_END );

      GetAttr( CYC_Active, Window_Objs[ACTID_UNIT], (ULONG *) &pos);

      FillUnit();
      NewUnit(pos);
      GUINewUnit();
      break;
    }

  }
}


static struct Hook IDCMPhook =
{
  { NULL, NULL },
  (HOOKFUNC) IDCMPhookFunc,
  NULL,
  NULL
};


/******************************************************************************
**** Call to open a requester *************************************************
******************************************************************************/

static ULONG Req( UBYTE *gadgets, UBYTE *body, ... ) {
  struct bguiRequest   req = { NULL };

  req.br_Title        = (char *) msgTextProgramName;
  req.br_ReqPos       = POS_TOPLEFT;
  req.br_GadgetFormat = gadgets;
  req.br_TextFormat   = body;
  req.br_Flags        = BREQF_LOCKWINDOW | BREQF_AUTO_ASPECT;

  return( BGUI_RequestA( window, &req, ( ULONG * )( &body + 1 )));
}


/******************************************************************************
**** Call to open the window **************************************************
******************************************************************************/


BOOL BuildGUI(char *screenname) {
	Object **ar = Window_Objs;
  struct Screen *screen;
  BOOL OptionFrame = FALSE;
  LONG indicatorwidth = 100;

  UpdateStrings();

  // Kludge for Piccaso 96/BGUI.
  if(FindTask("Picasso96"))
  {
    PopUpMenus = FALSE;
  }

  BGUIBase = (void *)OpenLibrary("bgui.library", 41);
  if(BGUIBase == NULL) {
    Printf((char *) msgTextNoOpen, (ULONG) "bgui.library", 41);
    Printf("\n");
    return FALSE;
  }

  screen = LockPubScreen(screenname);

  indicatorwidth = max(indicatorwidth, 16 * screen->RastPort.Font->tf_XSize);
  if(screen->Height > 240) {
    OptionFrame = TRUE;
  }

  WO_Window = WindowObject,
    WINDOW_PubScreen,       screen,
    WINDOW_Title,           (char *) msgTextProgramName,
    WINDOW_CloseGadget,     FALSE,
    WINDOW_MenuStrip,       Menus,
    WINDOW_AutoAspect,      TRUE,
    WINDOW_AutoKeyLabel,    TRUE,
    WINDOW_ScaleWidth,      60,
    WINDOW_ScaleHeight,     20,
    WINDOW_HelpFile,        HELPFILE,
    WINDOW_HelpNode,        "AHI",
    WINDOW_IDCMPHook,      &IDCMPhook,
    WINDOW_IDCMPHookBits,   IDCMP_RAWKEY,

    WINDOW_MasterGroup, VGroupObject, NormalOffset,

/* TABS */

      StartMember, ar[ACTID_TABS] = MxObject,
        MX_Labels, PageNames,
        MX_TabsObject, TRUE,
        GA_ID, ACTID_TABS,
      EndObject, FixMinHeight, EndMember,

      StartMember, ar[ACTID_PAGE] = PageObject, NormalSpacing, 
        PAGE_NoBufferRP, TRUE,

/* PAGE 1 */

        PageMember, HGroupObject, NormalOffset, NormalSpacing, TabAboveFrame,

          StartMember, vgroup = VGroupObject, NormalSpacing,

            StartMember, ar[ACTID_UNIT] = CycleObject,
              CYC_Labels,    Units,
              CYC_Active,    state.UnitSelected,
              CYC_PopActive, PopUpMenus,
              CYC_Popup,     PopUpMenus,
              GA_ID,         ACTID_UNIT,
            EndObject, FixMinHeight, EndMember,

            StartMember, ar[ACTID_MODE] = ListviewObject,
//              LISTV_EntryArray,  Modes,
//              LISTV_Select,      state.ModeSelected,
              PGA_NewLook, TRUE,
              GA_ID, ACTID_MODE,
            EndObject, EndMember,

            StartMember, HGroupObject,
              ButtonFrame, FRM_Flags, FRF_RECESSED,
          
              StartMember, InfoObject,
                INFO_TextFormat,  (char *) msgProperties,
                INFO_FixTextWidth,TRUE,
                INFO_MinLines,    6,
              EndObject, FixMinWidth, EndMember,

              StartMember, ar[SHOWID_MODE] = InfoObject,
                INFO_MinLines,   6,
              EndObject,  EndMember,
            EndObject, FixMinHeight, EndMember,

          EndObject /* vgroup */, EndMember,

          StartMember, VGroupObject, NormalSpacing, 

            VarSpace(1),

            StartMember, VGroupObject,
              (OptionFrame ? GROUP_HorizOffset : TAG_IGNORE), GRSPACE_NORMAL,
              (OptionFrame ? GROUP_VertOffset  : TAG_IGNORE), GRSPACE_NORMAL,
              (OptionFrame ? GROUP_Spacing     : TAG_IGNORE), GRSPACE_NORMAL,
              (OptionFrame ? FRM_Type          : TAG_IGNORE), FRTYPE_RIDGE,
              (OptionFrame ? FRM_Flags         : TAG_IGNORE), FRF_RECESSED,
              (OptionFrame ? FRM_Title         : TAG_IGNORE), (char *) msgOptions,

              StartMember, HGroupObject,
                StartMember, ar[ACTID_FREQ] = SliderObject,
                  LAB_Label,    (char *) msgOptFrequency,
                  SLIDER_Min,   0,
                  SLIDER_Max,   state.Frequencies -1 ,
                  SLIDER_Level, state.FreqSelected,
                  PGA_NewLook, TRUE,
                  GA_ID, ACTID_FREQ,
                EndObject, EndMember,

                StartMember, ar[SHOWID_FREQ] = InfoObject,
                  INFO_TextFormat,    getFreq(),
                  INFO_VertOffset,    0,
                EndObject, FixWidth(indicatorwidth), EndMember,
              EndObject, FixMinHeight, EndMember,

              StartMember, HGroupObject,
                StartMember, ar[ACTID_CHANNELS] = SliderObject,
                  LAB_Label,    (char *) msgOptChannels,
                  SLIDER_Min,   1,
                  SLIDER_Max,   state.Channels,
                  SLIDER_Level, state.ChannelsSelected,
                  PGA_NewLook, TRUE,
                  GA_ID, ACTID_CHANNELS,
                EndObject, EndMember,

                StartMember, ar[SHOWID_CHANNELS] = InfoObject,
                  INFO_TextFormat,    getChannels(),
                  INFO_VertOffset,    0,
                EndObject, FixWidth(indicatorwidth), EndMember,
              EndObject, FixMinHeight, EndMember,

              StartMember, HGroupObject,
                StartMember, ar[ACTID_OUTVOL] = SliderObject,
                  LAB_Label,    (char *) msgOptVolume,
                  SLIDER_Min,   0,
                  SLIDER_Max,   state.OutVols-1 ,
                  SLIDER_Level, state.OutVolSelected,
                  PGA_NewLook, TRUE,
                  GA_ID, ACTID_OUTVOL,
                EndObject, EndMember,

                StartMember, ar[SHOWID_OUTVOL] = InfoObject,
                  INFO_TextFormat,    getOutVol(),
                  INFO_VertOffset,    0,
                EndObject, FixWidth(indicatorwidth), EndMember,
              EndObject, FixMinHeight, EndMember,

              StartMember, HGroupObject,
                StartMember, ar[ACTID_MONVOL] = SliderObject,
                  LAB_Label,    (char *) msgOptMonitor,
                  SLIDER_Min,   0,
                  SLIDER_Max,   state.MonVols-1 ,
                  SLIDER_Level, state.MonVolSelected,
                  PGA_NewLook, TRUE,
                  GA_ID, ACTID_MONVOL,
                EndObject, EndMember,

                StartMember, ar[SHOWID_MONVOL] = InfoObject,
                  INFO_TextFormat,    getMonVol(),
                  INFO_VertOffset,    0,
                EndObject, FixWidth(indicatorwidth), EndMember,
              EndObject, FixMinHeight, EndMember,

              StartMember, HGroupObject,
                StartMember, ar[ACTID_GAIN] = SliderObject,
                  LAB_Label,    (char *) msgOptGain,
                  SLIDER_Min,   0,
                  SLIDER_Max,   state.Gains-1 ,
                  SLIDER_Level, state.GainSelected,
                  PGA_NewLook, TRUE,
                  GA_ID, ACTID_GAIN,
                EndObject, EndMember,

                StartMember, ar[SHOWID_GAIN] = InfoObject,
                  INFO_TextFormat,    getGain(),
                  INFO_VertOffset,    0,
                EndObject, FixWidth(indicatorwidth), EndMember,
              EndObject, FixMinHeight, EndMember,

              StartMember, HGroupObject,
                StartMember, ar[ACTID_INPUT] = SliderObject,
                  LAB_Label,    (char *) msgOptInput,
                  SLIDER_Min,   0,
                  SLIDER_Max,   state.Inputs-1 ,
                  SLIDER_Level, state.InputSelected,
                  PGA_NewLook, TRUE,
                  GA_ID, ACTID_INPUT,
                EndObject, EndMember,

                StartMember, ar[SHOWID_INPUT] = InfoObject,
                  INFO_TextFormat,    getInput(),
                  INFO_VertOffset,    0,
                EndObject, FixWidth(indicatorwidth), EndMember,
              EndObject, FixMinHeight, EndMember,

              StartMember, HGroupObject,
                StartMember, ar[ACTID_OUTPUT] = SliderObject,
                  LAB_Label,    (char *) msgOptOutput,
                  SLIDER_Min,   0,
                  SLIDER_Max,   state.Outputs-1 ,
                  SLIDER_Level, state.OutputSelected,
                  PGA_NewLook, TRUE,
                  GA_ID, ACTID_OUTPUT,
                EndObject, EndMember,

                StartMember, ar[SHOWID_OUTPUT] = InfoObject,
                  INFO_TextFormat,    getOutput(),
                  INFO_VertOffset,    0,
                EndObject, FixWidth(indicatorwidth), EndMember,
              EndObject, FixMinHeight, EndMember,

            EndObject /* vgroup "Options" */, FixMinHeight, EndMember,

          VarSpace(1),

          EndObject /* vgroup */, EndMember,

        EndObject, /* (EndMember) page */

/* PAGE 2 */

        PageMember, HGroupObject, NormalSpacing, TabAboveFrame,

          VarSpace(1),

          StartMember, VGroupObject, NormalSpacing,

            VarSpace(1), 

            StartMember, VGroupObject, NormalOffset, NormalSpacing,
              RidgeFrame, FRM_Flags, FRF_RECESSED, FrameTitle((char *) msgGlobalOptions),

              StartMember, HGroupObject, NormalSpacing,
              StartMember, VGroupObject, NormalSpacing,

              StartMember, ar[ACTID_DEBUG] = CycleObject,
                LAB_Label,      (char *) msgGlobOptDebugLevel,
                LAB_Place,      PLACE_LEFT,
                CYC_Labels,     DebugLabels,
                CYC_Active,     globalprefs.ahigp_DebugLevel,
                CYC_PopActive,  PopUpMenus,
                CYC_Popup,      PopUpMenus,
                GA_ID,          ACTID_DEBUG,
              EndObject, FixMinHeight, EndMember,

              StartMember, ar[ACTID_ECHO] = CycleObject,
                LAB_Label,      (char *) msgGlobOptEcho,
                LAB_Place,      PLACE_LEFT,
                CYC_Labels,     EchoLabels,
                CYC_Active,     (globalprefs.ahigp_DisableEcho ? 2 : 0) |
                                (globalprefs.ahigp_FastEcho    ? 1 : 0),
                CYC_PopActive,  PopUpMenus,
                CYC_Popup,      PopUpMenus,
                GA_ID,          ACTID_ECHO,
              EndObject, FixMinHeight, EndMember,

              StartMember, ar[ACTID_SURROUND] = CycleObject,
                LAB_Label,      (char *) msgGlobOptSurround,
                LAB_Place,      PLACE_LEFT,
                CYC_Labels,     SurroundLabels,
                CYC_Active,     globalprefs.ahigp_DisableSurround,
                GA_ID,          ACTID_SURROUND,
              EndObject, FixMinHeight, EndMember,

              StartMember, ar[ACTID_CLIPMV] = CycleObject,
                LAB_Label,      (char *) msgGlobOptMasterVol,
                LAB_Place,      PLACE_LEFT,
                CYC_Labels,     ClipMVLabels,
                CYC_Active,     globalprefs.ahigp_ClipMasterVolume,
                GA_ID,          ACTID_CLIPMV,
              EndObject, FixMinHeight, EndMember,

              StartMember, ar[ACTID_CPULIMIT] = SliderObject,
                LAB_Label,    (char *) msgGlobOptCPULimit,
                SLIDER_Min,   0,
                SLIDER_Max,   100,
                SLIDER_Level, (globalprefs.ahigp_MaxCPU * 100 + 32768) >> 16,
                PGA_NewLook, TRUE,
                GA_ID, ACTID_CPULIMIT,
              EndObject, EndMember,

              EndObject /* vgroup */, EndMember,

              StartMember, VGroupObject, NormalSpacing,

                VarSpace(1),

                StartMember, ar[SHOWID_CPULIMIT] = IndicatorObject,\
                  INDIC_Min,              0,
                  INDIC_Max,              100,
                  INDIC_Level,            (globalprefs.ahigp_MaxCPU * 100 + 32768) / 65536,
                  INDIC_Justification,    IDJ_LEFT,
                  INDIC_FormatString,     "%ld%%",
                EndObject, FixMinWidth, FixMinHeight, EndMember,

              EndObject /* vgroup */, EndMember,
              EndObject /* hgroup */, EndMember,

            EndObject, FixMinWidth, FixMinHeight, EndMember,

            VarSpace(1),

          EndObject /* vgroup */ , FixMinWidth, EndMember,

          VarSpace(1),

        EndObject, /* (EndMember) page */

      EndObject /* page */, EndMember,


/* BUTTONS */
      StartMember, HGroupObject, NormalSpacing, NormalVOffset,

        StartMember, ar[ACTID_SAVE] = ButtonObject,
          ButtonFrame,
          LAB_Label, (char *) msgButtonSave,
          GA_ID,     ACTID_SAVE,
        EndObject, EndMember,

        StartMember, ar[ACTID_USE] = ButtonObject,
          ButtonFrame,
          LAB_Label, (char *) msgButtonUse,
          GA_ID,     ACTID_USE,
        EndObject, EndMember,

        StartMember, ar[ACTID_QUIT] = ButtonObject,
          ButtonFrame,
          LAB_Label, (char *) msgButtonCancel,
          GA_ID,     ACTID_QUIT,
        EndObject, EndMember,
      EndObject, FixMinHeight, EndMember,
    EndObject,
  EndObject;

  if(WO_Window) {
    AddMap( ar[ACTID_TABS], ar[ACTID_PAGE], pagemap );

    DoMethod( ar[ACTID_FREQ],     BASE_ADDHOOK, &GadgetHook );
    DoMethod( ar[ACTID_CHANNELS], BASE_ADDHOOK, &GadgetHook );
    DoMethod( ar[ACTID_OUTVOL],   BASE_ADDHOOK, &GadgetHook );
    DoMethod( ar[ACTID_MONVOL],   BASE_ADDHOOK, &GadgetHook );
    DoMethod( ar[ACTID_GAIN],     BASE_ADDHOOK, &GadgetHook );
    DoMethod( ar[ACTID_INPUT],    BASE_ADDHOOK, &GadgetHook );
    DoMethod( ar[ACTID_OUTPUT],   BASE_ADDHOOK, &GadgetHook );

    AddMap( ar[ACTID_CPULIMIT], ar[SHOWID_CPULIMIT], cpumap);

    window = WindowOpen(WO_Window);
  }

  if(screen) {
    UnlockPubScreen(NULL, screen);
  }

  if(window == NULL) {
    Printf((char *) msgTextNoWindow);
    Printf("\n");
    return FALSE;
  }

  openreq = FileReqObject,
    ASLFR_Window,        window,
    ASLFR_SleepWindow,   TRUE,
    ASLFR_RejectIcons,   TRUE,
    ASLFR_InitialDrawer, "SYS:Prefs/Presets",
  EndObject;

  savereq = FileReqObject,
    ASLFR_Window,        window,
    ASLFR_SleepWindow,   TRUE,
    ASLFR_RejectIcons,   TRUE,
    ASLFR_InitialDrawer, "SYS:Prefs/Presets",
    ASLFR_DoSaveMode,    TRUE,
  EndObject;

  if((openreq == NULL) || (savereq == NULL)) {
    Printf((char *) msgTextNoFileRequester);
    Printf("\n");
    return FALSE;
  }

  // Update the checkmark for "Create Icons?"
  {
    struct Menu     *menu = NULL;
    struct MenuItem *item;

    GetAttr( WINDOW_MenuStrip, WO_Window, (ULONG *) &menu);
    ClearMenuStrip(window);

    item = FindMenuItem(ACTID_ICONS);

    if(item) {
      if(SaveIcons)
        item->Flags |= CHECKED;
      else
        item->Flags &= ~CHECKED;
    }
    ResetMenuStrip(window, menu);
  }

  GUINewUnit();
  return TRUE;
}


/******************************************************************************
**** Call to close the window *************************************************
******************************************************************************/

void CloseGUI(void) {

  if(savereq)
    DisposeObject(savereq);
  if(openreq)
    DisposeObject(openreq);

  savereq   = NULL;
  openreq   = NULL;

  if(WO_Window)
    DisposeObject(WO_Window);

  WO_Window = NULL;
  window    = NULL;

  if (BGUIBase)
    CloseLibrary(BGUIBase);

  BGUIBase = NULL;
}



/******************************************************************************
**** Handles the input events *************************************************
******************************************************************************/

void EventLoop(void) {
  ULONG signal  = NULL, rc;
  BOOL  running = TRUE;

  GetAttr( WINDOW_SigMask, WO_Window, &signal );
  if(signal) {
    do {
      if(Wait(signal | SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C ) {
        running=FALSE;
      }

      while (( rc = HandleEvent( WO_Window )) != WMHI_NOMORE ) {
        switch(rc) {

          case WMHI_CLOSEWINDOW:
            running=FALSE;
            break;

          case ACTID_OPEN:
          {
            if(DoMethod( openreq, FRM_DOREQUEST ) == FRQ_OK) {
              char *file = NULL;

              GetAttr( FRQ_Path, openreq, (ULONG *) &file);
              NewSettings(file);
              GUINewSettings();
            }
            break;
          }

          case ACTID_SAVEAS:
          {
            FillUnit();
            if(DoMethod( savereq, FRM_DOREQUEST ) == FRQ_OK) {
              char *file = NULL;

              GetAttr( FRQ_Path, savereq, (ULONG *) &file);
              SaveSettings(file, UnitList);
              if(SaveIcons) {
                WriteIcon(file);
              }
            }
            break;
          
          }
          
          case ACTID_ABOUT:
            Req( (char *) msgButtonOK,
                (char *) msgTextCopyright,
                ISEQ_C,  msgTextProgramName,
                "1996-2005 Martin Blom" );
            break;

          case ACTID_SAVE:
            FillUnit();
            SaveSettings(ENVFILE, UnitList);
            SaveSettings(ENVARCFILE, UnitList);
            running=FALSE;
            break;

          case ACTID_USE:
            FillUnit();
            SaveSettings(ENVFILE, UnitList);
            running=FALSE;
            break;

          case ACTID_QUIT:
            running=FALSE;
            break;

          case ACTID_DEFAULT:
            NewSettings(NULL);
            GUINewSettings();
            break;

          case ACTID_LASTSAVED:
            NewSettings(ENVARCFILE);
            GUINewSettings();
            break;

          case ACTID_RESTORE:
            NewSettings(args.from);
            GUINewSettings();
            break;

          case ACTID_ICONS:
          {
            struct MenuItem *item;

            item = FindMenuItem(ACTID_ICONS);

            if(item) {
              if(item->Flags & CHECKED)
                SaveIcons = TRUE;
              else
                SaveIcons = FALSE;
            }
            break;
          }

          case ACTID_HELP: {
            APTR lock = BGUI_LockWindow(window);
            if(!BGUI_Help( window, HELPFILE, "AHI", NULL)) {
              Req( (char *) msgButtonOK, (char *) msgTextNoFind, HELPFILE);
            }
            BGUI_UnlockWindow(lock);
            break;
          }

          case ACTID_GUIDE: {
            APTR lock = BGUI_LockWindow(window);
            if(!BGUI_Help( window, HELPFILE, NULL, NULL)) {
              Req( (char *) msgButtonOK, (char *) msgTextNoFind, HELPFILE);
            }
            BGUI_UnlockWindow(lock);
            break;
          }

          case ACTID_HELPINDEX: {
            APTR lock = BGUI_LockWindow(window);
            if(!BGUI_Help( window, HELPFILE, "Concept Index", 0)) {
              Req( (char *) msgButtonOK, (char *) msgTextNoFind, HELPFILE);
            }
            BGUI_UnlockWindow(lock);
            break;
          }

          case ACTID_UNIT: {
            ULONG unit;

            FillUnit();
            GetAttr( CYC_Active, Window_Objs[rc], &unit);
            NewUnit(unit);
            GUINewUnit();
            break;
          }

          case ACTID_MODE: {
            ULONG mode;

            FillUnit();
            GetAttr( LISTV_LastClickedNum, Window_Objs[rc], &mode);
            NewMode(mode);
            GUINewMode();
            break;
          }

          case ACTID_DEBUG:
          case ACTID_SURROUND:
          case ACTID_ECHO:
          case ACTID_CPULIMIT:
          case ACTID_CLIPMV:
          {
            ULONG debug = AHI_DEBUG_NONE, surround = FALSE, echo = 0, cpu = 90;
            ULONG clip = FALSE;

            GetAttr( CYC_Active,   Window_Objs[ACTID_DEBUG],    &debug);
            GetAttr( CYC_Active,   Window_Objs[ACTID_SURROUND], &surround);
            GetAttr( CYC_Active,   Window_Objs[ACTID_ECHO],     &echo);
            GetAttr( CYC_Active,   Window_Objs[ACTID_CLIPMV],   &clip);
            GetAttr( SLIDER_Level, Window_Objs[ACTID_CPULIMIT], &cpu);

            globalprefs.ahigp_DebugLevel      = debug;
            globalprefs.ahigp_DisableSurround = surround;
            globalprefs.ahigp_DisableEcho     = (echo == 2);
            globalprefs.ahigp_FastEcho        = (echo == 1);
            globalprefs.ahigp_MaxCPU = (cpu << 16) / 100;
            globalprefs.ahigp_ClipMasterVolume= clip;

            break;
          }
        }
      }
    } while(running);
  }
}
