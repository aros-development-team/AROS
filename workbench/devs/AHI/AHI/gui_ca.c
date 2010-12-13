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


#define NO_INLINE_STDARG
#define ALL_REACTION_CLASSES

#include <config.h>

#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <reaction/reaction.h>
#include <reaction/reaction_macros.h>

#include <clib/alib_protos.h>
#include <clib/reaction_lib_protos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/intuition.h>
#include <proto/slider.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ahi.h"
#include "ahiprefs_Cat.h"
#include "support.h"
#include "gui.h"

#ifdef __AMIGAOS4__
ULONG
HookEntry(struct Hook *hookPtr, Object *obj, APTR message) {
  return ((ULONG (*)(struct Hook*, Object*, APTR)) hookPtr->h_SubEntry)
    (hookPtr, obj, message);
}
#endif

static void GUINewSettings(void);
static void GUINewUnit(void);
static void GUINewMode(void);


enum windowIDs {
  WINID_MAIN=1,
  WINID_COUNT
};

enum actionIDs {
  LAYOUT_PAGE,
  ACTID_OPEN, ACTID_SAVEAS, ACTID_ABOUT, ACTID_QUIT,
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


/* Version of ClassAct gadgets to use */
long __classactversion = 41;

static struct Window *Window                   = NULL;
static struct Menu   *Menu                     = NULL;
static void          *vi                       = NULL;
static Object        *WO_Window                = NULL;
static Object        *Window_Objs[ACTID_COUNT] = {NULL};
static Object        *openreq                  = NULL;
static Object        *savereq                  = NULL;

/* Dynamic */
static struct List   *unitlist                 = NULL;
static struct List   *modelist                 = NULL;
static struct List   *infolist                 = NULL;

/* Static */
static struct List   *pagelist                 = NULL;
static struct List   *debuglist                = NULL;
static struct List   *echolist                 = NULL;
static struct List   *surroundlist             = NULL;
static struct List   *clipMVlist               = NULL;

static char *infotext     = NULL;    // Copy of msgProperties
static char *infotexts[7] = {NULL};  // Entries + NULL
static char *infoargs[7]  = {NULL};

struct ColumnInfo ci[] = {
  { 100, NULL, 0 },
  { 100, NULL, 0 },
  { -1, (STRPTR)~0, -1 }
};

#define Title(t)        { NM_TITLE, t, NULL, 0, 0, NULL }
#define Item(t,s,i)     { NM_ITEM, t, s, 0, 0, (APTR)i }
#define ItemBar         { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL }
#define SubItem(t,s,i)  { NM_SUB, t, s, 0, 0, (APTR)i }
#define SubBar          { NM_SUB, NM_BARLABEL, NULL, 0, 0, NULL }
#define EndMenu         { NM_END, NULL, NULL, 0, 0, NULL }
#define ItCk(t,s,i,f)   { NM_ITEM, t, s, f, 0, (APTR)i }

static struct NewMenu NewMenus[] = {
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
  EndMenu
};

struct TagItem MapTab2Page[] = {
  { CLICKTAB_Current, PAGE_Current },
  { TAG_DONE, 0 }
};


/***** Local function to set gadget attributes *******************************/

static void MySetGadgetAttrsA(Object *gadget, struct TagItem *tags) {

  if(SetPageGadgetAttrsA((struct Gadget *) gadget, Window_Objs[ACTID_PAGE],
                         Window, NULL, tags)) {
    RefreshGList((struct Gadget *) gadget, Window, NULL, 1);
  }
}

static void MySetGadgetAttrs(Object *gadget, ULONG tag1, ...) {

  MySetGadgetAttrsA(gadget, (struct TagItem *) &tag1);
}

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

  menuptr   = (struct NewMenu *) &NewMenus;
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

  /* Convert the \n-separated properties texts to 6 separate texts */

  {
    char  *s, *d;
    char **p;

    infotext = AllocVec(strlen((char *) msgProperties) + 1, MEMF_PUBLIC);
    if(infotext == NULL) return;

    s = (char *) msgProperties;
    d = infotext;
    p = infotexts;

    *p++ = d;

    while(*s) {
      if(*s == '\n') {
        *d = '\0';
        *p++ = d + 1;
      }
      else *d = *s;
      s++;
      d++;
    }
    *d = '\0';
  }
}

/***** Local function to update a slider indicator ***************************/

static void UpdateSliderLevel(int src, int dst, LONG *index, char * (*func)(void)) {
  if(GetAttr( SLIDER_Level, Window_Objs[src], (ULONG *) index)) {
    Printf("%ld\n", *index);
    MySetGadgetAttrs(Window_Objs[dst],
        GA_Text, (*(func))(), 
        TAG_DONE );
  }
}

/***** Local function to find a menu item ************************************/

static struct MenuItem *FindMenuItem(ULONG id) {
  struct Menu     *menu;
  struct MenuItem *item;

  menu = Menu;

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


/***** Local functions to handle the multicolumn list ************************/

static void FreeBrowserNodes2(struct List *list) {
  struct Node *node, *nextnode;

  if(list == NULL) return;

  node = list->lh_Head;

  while( ( nextnode = node->ln_Succ ) != NULL ) {
    FreeListBrowserNode(node);
    node = nextnode;
  }

  FreeVec(list);
}


static struct List *BrowserNodes2(char **labels1, char **labels2) {
  struct List *list;

  list = AllocVec(sizeof(struct List), MEMF_PUBLIC|MEMF_CLEAR);

  if(list != NULL) {
    struct Node *node;

    NewList(list);

    while (*labels1 && *labels2)
    {
      node = AllocListBrowserNode(2,
              LBNA_Column, 0,
                LBNCA_Text, *labels1,
              LBNA_Column, 1,
                LBNCA_Text, *labels2,
              TAG_DONE );
      if (node != NULL )
      {
        AddTail(list, node);
      }
      else {
        FreeBrowserNodes2(list);
        list = NULL;
        break;
      }

      labels1++;
      labels2++;
    }

  }

  return list;
}


/***** Local function to update the unit gadget (and everything below) *******/

static void GUINewSettings(void) {
  struct List *tmp;

  tmp = ChooserLabelsA( (STRPTR *) Units);

  MySetGadgetAttrs(Window_Objs[ACTID_UNIT],
    CHOOSER_Labels, tmp,
    CHOOSER_Active, state.UnitSelected,
    TAG_DONE);

  if(unitlist) FreeChooserLabels(unitlist);

  unitlist = tmp;

  MySetGadgetAttrs(Window_Objs[ACTID_DEBUG],
      CHOOSER_Active, globalprefs.ahigp_DebugLevel, TAG_DONE);
  MySetGadgetAttrs(Window_Objs[ACTID_SURROUND],
      CHOOSER_Active, globalprefs.ahigp_DisableSurround, TAG_DONE);
  MySetGadgetAttrs(Window_Objs[ACTID_ECHO],
      CHOOSER_Active, (globalprefs.ahigp_DisableEcho ? 2 : 0) | 
                  (globalprefs.ahigp_FastEcho    ? 1 : 0),     TAG_DONE);
  MySetGadgetAttrs(Window_Objs[ACTID_CLIPMV],
      CHOOSER_Active, globalprefs.ahigp_ClipMasterVolume, TAG_DONE);
  MySetGadgetAttrs(Window_Objs[ACTID_CPULIMIT],
      INTEGER_Number, (globalprefs.ahigp_MaxCPU * 100 + 32768) >> 16, TAG_DONE);

  GUINewUnit();
}


/***** Local function to update all gadgets that depend on the unit **********/

static void GUINewUnit(void) {

  MySetGadgetAttrs(Window_Objs[ACTID_MODE],
    LISTBROWSER_Labels, ~0,
    TAG_DONE);

  if(modelist) FreeBrowserNodes(modelist);

  modelist = BrowserNodesA( (STRPTR *) Modes);

  MySetGadgetAttrs(Window_Objs[ACTID_MODE],
    LISTBROWSER_Labels, modelist,
    TAG_DONE);

  MySetGadgetAttrs(Window_Objs[ACTID_MODE],
    LISTBROWSER_Selected,    state.ModeSelected,
    LISTBROWSER_MakeVisible, state.ModeSelected,
    TAG_DONE);

  GUINewMode();
}


/***** Local function to update all gadgets that depend on the mode **********/

static char modetext[11];
static char drivertext[128];

static void GUINewMode(void) {
  int Max, Sel;

  sprintf(modetext,"0x%08lx", getAudioMode());
  infoargs[0] = modetext;
  infoargs[1] = getRecord();
  infoargs[2] = getAuthor();
  infoargs[3] = getCopyright();
  sprintf(drivertext, "Devs:AHI/%s.audio", getDriver());
  infoargs[4] = drivertext;
  infoargs[5] = getVersion();

  MySetGadgetAttrs(Window_Objs[SHOWID_MODE],
    LISTBROWSER_Labels, ~0,
    TAG_DONE);

  if(infolist) FreeBrowserNodes2(infolist);

  infolist = BrowserNodes2(infotexts, infoargs);

  MySetGadgetAttrs(Window_Objs[SHOWID_MODE],
    LISTBROWSER_Labels, infolist,
    TAG_DONE);

  Max = max(state.Frequencies -1, 0);
  Sel = min(Max, state.FreqSelected);
  MySetGadgetAttrs(Window_Objs[ACTID_FREQ],
      SLIDER_Min,       0,
      SLIDER_Max,       Max,
      SLIDER_Level,     Sel,
      GA_Disabled,      (Max == 0),
      TAG_DONE );
  Max = max(state.Channels, 1);
  Sel = min(Max, state.ChannelsSelected);
  MySetGadgetAttrs(Window_Objs[ACTID_CHANNELS],
      SLIDER_Min,       1,
      SLIDER_Max,       Max,
      SLIDER_Level,     Sel,
      GA_Disabled,      (Max == 1) || state.ChannelsDisabled,
      TAG_DONE );
  Max = max(state.OutVols -1, 0);
  Sel = min(Max, state.OutVolSelected);
  MySetGadgetAttrs(Window_Objs[ACTID_OUTVOL],
      SLIDER_Min,       0,
      SLIDER_Max,       Max,
      SLIDER_Level,     Sel,
      GA_Disabled,      (Max == 0),
      TAG_DONE );
  Max = max(state.MonVols -1, 0);
  Sel = min(Max, state.MonVolSelected);
  MySetGadgetAttrs(Window_Objs[ACTID_MONVOL],
      SLIDER_Min,       0,
      SLIDER_Max,       Max,
      SLIDER_Level,     Sel,
      GA_Disabled,      (Max == 0),
      TAG_DONE );
  Max = max(state.Gains -1, 0);
  Sel = min(Max, state.GainSelected);
  MySetGadgetAttrs(Window_Objs[ACTID_GAIN],
      SLIDER_Min,       0,
      SLIDER_Max,       Max,
      SLIDER_Level,     Sel,
      GA_Disabled,      (Max == 0),
      TAG_DONE );
  Max = max(state.Inputs -1, 0);
  Sel = min(Max, state.InputSelected);
  MySetGadgetAttrs(Window_Objs[ACTID_INPUT],
      SLIDER_Min,       0,
      SLIDER_Max,       Max,
      SLIDER_Level,     Sel,
      GA_Disabled,      (Max == 0),
      TAG_DONE );
  Max = max(state.Outputs -1, 0);
  Sel = min(Max, state.OutputSelected);
  MySetGadgetAttrs(Window_Objs[ACTID_OUTPUT],
      SLIDER_Min,       0,
      SLIDER_Max,       Max,
      SLIDER_Level,     Sel,
      GA_Disabled,      (Max == 0),
      TAG_DONE );

    // Update indicators..

    MySetGadgetAttrs(Window_Objs[SHOWID_FREQ],
        GA_Text, getFreq(), TAG_DONE );

    MySetGadgetAttrs(Window_Objs[SHOWID_CHANNELS],
        GA_Text, getChannels(), TAG_DONE );

    MySetGadgetAttrs(Window_Objs[SHOWID_OUTVOL],
        GA_Text, getOutVol(), TAG_DONE );

    MySetGadgetAttrs(Window_Objs[SHOWID_MONVOL],
        GA_Text, getMonVol(), TAG_DONE );

    MySetGadgetAttrs(Window_Objs[SHOWID_GAIN],
        GA_Text, getGain(), TAG_DONE );

    MySetGadgetAttrs(Window_Objs[SHOWID_INPUT],
        GA_Text, getInput(), TAG_DONE );

    MySetGadgetAttrs(Window_Objs[SHOWID_OUTPUT],
        GA_Text, getOutput(), TAG_DONE );

}


/***** Gadget hook ***********************************************************/

static ULONG 
GadgetHookFunc( struct Hook *hook,
		Object *obj,
		struct opUpdate *opu) {

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
  { NULL, NULL },
  HookEntry,
  (HOOKFUNC) GadgetHookFunc,
  NULL
};

static void IDCMPhookFunc(struct Hook *hook,
			  Object *obj,
			  struct IntuiMessage *msg)
{
  switch(msg->Code) {
  
    case 0x42:
    {
      /*
      **      TAB             - Next page
      **      SHIFT + TAB     - Prev page
      */
      ULONG          pos = 0;
      struct TagItem tags[] = {
        {CLICKTAB_Current, 0}, 
        {TAG_DONE, 0}
      };

      GetAttr( CLICKTAB_Current,  Window_Objs[ACTID_TABS], &pos );

      if ( msg->Qualifier & ( IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT ))
        pos--;
      else 
        pos++;

      pos %= 2;

      tags[0].ti_Data = pos;

      MySetGadgetAttrs(Window_Objs[ACTID_TABS],
          CLICKTAB_Current, pos, TAG_DONE );
      MySetGadgetAttrs(Window_Objs[ACTID_PAGE],
          PAGE_Current, pos, TAG_DONE );

      RethinkLayout((struct Gadget *) Window_Objs[LAYOUT_PAGE], Window, NULL, TRUE);

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
        MySetGadgetAttrs(Window_Objs[ACTID_MODE],
            LISTBROWSER_Position, LBP_PAGEUP, TAG_DONE );
      else if(msg->Qualifier & IEQUALIFIER_CONTROL )
        MySetGadgetAttrs(Window_Objs[ACTID_MODE],
            LISTBROWSER_Position, LBP_TOP, TAG_DONE );
      else
        MySetGadgetAttrs(Window_Objs[ACTID_MODE],
            LISTBROWSER_Position, LBP_LINEUP, TAG_DONE );

      GetAttr( LISTBROWSER_Selected, Window_Objs[ACTID_MODE], &mode);
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
        MySetGadgetAttrs(Window_Objs[ACTID_MODE],
            LISTBROWSER_Position, LBP_PAGEDOWN, TAG_DONE );
      else if(msg->Qualifier & IEQUALIFIER_CONTROL )
        MySetGadgetAttrs(Window_Objs[ACTID_MODE],
            LISTBROWSER_Position, LBP_BOTTOM, TAG_DONE );
      else
        MySetGadgetAttrs(Window_Objs[ACTID_MODE],
            LISTBROWSER_Position, LBP_LINEDOWN, TAG_DONE );

      GetAttr( LISTBROWSER_Selected, Window_Objs[ACTID_MODE], &mode);
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

      GetAttr( CHOOSER_Active, Window_Objs[ACTID_UNIT], (ULONG *) &pos );

      pos = (pos - 1) % i;
      if(pos < 0) pos += i;

      MySetGadgetAttrs(Window_Objs[ACTID_UNIT],
          CHOOSER_Active, pos, TAG_DONE );

      GetAttr( CHOOSER_Active, Window_Objs[ACTID_UNIT], (ULONG *) &pos);
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

      GetAttr( CHOOSER_Active, Window_Objs[ACTID_UNIT], (ULONG *) &pos );

      pos = (pos + 1) % i;;

      MySetGadgetAttrs(Window_Objs[ACTID_UNIT],
          CHOOSER_Active, pos, TAG_DONE );

      GetAttr( CHOOSER_Active, Window_Objs[ACTID_UNIT], (ULONG *) &pos);

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
  ( HOOKFUNC ) IDCMPhookFunc,
  NULL,
  NULL
};


/******************************************************************************
**** Call to open a requester *************************************************
******************************************************************************/

static ULONG Req( UBYTE *gadgets, UBYTE *body, ... ) {
  struct EasyStruct req = {
    sizeof (struct EasyStruct), 0, NULL, NULL, NULL
#ifdef __AMIGAOS4__
    , NULL, NULL
#endif
  };
  ULONG rc;

  req.es_Title        = (char *) msgTextProgramName;
  req.es_TextFormat   = body; 
  req.es_GadgetFormat = gadgets;

  SetAttrs( WO_Window, WA_BusyPointer, TRUE, TAG_DONE);
 	rc = EasyRequestArgs( Window, &req, NULL, ( ULONG * )( &body + 1 ) );
  SetAttrs( WO_Window, WA_BusyPointer, FALSE, TAG_DONE);

  return rc;
}


/******************************************************************************
**** Call to open the window **************************************************
******************************************************************************/


BOOL BuildGUI(char *screenname) {

	Object **ar = Window_Objs;
  struct Screen *screen;
  BOOL OptionFrame = FALSE;
  LONG indicatorwidth = 100;
  Object *l1, *l2, *l3, *l4, *l5, *l6, *l7;

  UpdateStrings();

  if (ButtonBase == NULL) { /* force it open */
    Printf((char *) msgTextNoOpen, (ULONG) "button.gadget", __classactversion);
    Printf("\n");
    return FALSE;
  }

  /* Dynamic */
  unitlist      = ChooserLabelsA( (STRPTR *) Units);
  modelist      = BrowserNodesA( (STRPTR *) Modes);
  infolist      = BrowserNodes2(infotexts, infoargs);

  /* Static */
  pagelist      = ClickTabs( (char *) msgPageMode, (char *) msgPageAdvanced, NULL);
  debuglist     = ChooserLabels( (char *) msgDebugNone,
                                 (char *) msgDebugLow,
                                 (char *) msgDebugHigh,
                                 (char *) msgDebugFull,
                                 NULL);
  echolist      = ChooserLabels( (char *) msgEchoEnabled,
                                 (char *) msgEchoFast,
                                 (char *) msgEchoDisabled,
                                 NULL);
  surroundlist  = ChooserLabels( (char *) msgSurroundEnabled,
                                 (char *) msgSurroundDisabled,
                                 NULL);
  clipMVlist    = ChooserLabels( (char *) msgMVNoClip,
                                 (char *) msgMVClip,
                                 NULL);


  screen = LockPubScreen(screenname);

  vi = GetVisualInfoA(screen, NULL);

  Menu = CreateMenusA(NewMenus, NULL);

  if(vi == NULL || Menu == NULL) {
    Printf((char *) msgTextNoWindow);
    Printf("\n");
    return FALSE;
  }

  if(!LayoutMenus(Menu, vi, GTMN_NewLookMenus, TRUE, TAG_DONE)) {
    Printf((char *) msgTextNoWindow);
    Printf("\n");
    return FALSE;
  }

  indicatorwidth = max(indicatorwidth, 16 * screen->RastPort.Font->tf_XSize);
  if(screen->Height > 240) {
    OptionFrame = TRUE;
  }

  ar[ACTID_PAGE] = PageObject,
    PAGE_Current,         0,

/* PAGE 1 */
    PAGE_Add, HLayoutObject,
      LAYOUT_VertAlignment, LALIGN_CENTER,

      LAYOUT_AddChild, VLayoutObject,

        LAYOUT_AddChild, ar[ACTID_UNIT] = PopUpObject,
          CHOOSER_Labels,         unitlist,
          CHOOSER_Active,         state.UnitSelected,
        ChooserEnd,
        CHILD_WeightedHeight, 0,

        LAYOUT_AddChild, ar[ACTID_MODE] = ListBrowserObject,
          LISTBROWSER_Labels,     modelist,
          LISTBROWSER_MakeVisible,state.ModeSelected,
          LISTBROWSER_Selected,   state.ModeSelected,
          LISTBROWSER_ShowSelected,TRUE,
        ListBrowserEnd,

        LAYOUT_AddChild, ar[SHOWID_MODE] = ListBrowserObject,
          LISTBROWSER_Labels,     infolist,
          LISTBROWSER_ColumnInfo, (ULONG)&ci,
          LISTBROWSER_AutoFit,    TRUE,
          LISTBROWSER_VertSeparators, FALSE,
          GA_ReadOnly,            TRUE,
        ListBrowserEnd,
        CHILD_MaxHeight, screen->RastPort.Font->tf_YSize * 6 + 6, // UHH!!
      LayoutEnd,

      LAYOUT_AddChild, HLayoutObject,
        (OptionFrame ? LAYOUT_SpaceOuter   : TAG_IGNORE), TRUE,
        (OptionFrame ? LAYOUT_RightSpacing : TAG_IGNORE), 0,
        (OptionFrame ? LAYOUT_BevelStyle   : TAG_IGNORE), BVS_GROUP,
        (OptionFrame ? LAYOUT_Label        : TAG_IGNORE), (char *) msgOptions,

        LAYOUT_AddChild, VLayoutObject,

          LAYOUT_AddChild, l1 = HLayoutObject,
            LAYOUT_AddChild, ar[ACTID_FREQ] = SliderObject,
              SLIDER_Min,           0,
              SLIDER_Max,           state.Frequencies,
              SLIDER_Level,         state.FreqSelected,
              SLIDER_Orientation,   SORIENT_HORIZ,
              GA_UserData,            &GadgetHook,
            SliderEnd,
            CHILD_Label, LabelObject,
              LABEL_Text,             (char *) msgOptFrequency,
            LabelEnd,
            LAYOUT_AddChild, ar[SHOWID_FREQ] = ButtonObject,
              GA_Text,                getFreq(),
              GA_ReadOnly,            TRUE,
              BUTTON_BevelStyle,      BVS_NONE,
              BUTTON_Transparent,     TRUE,
              BUTTON_Justification,   BCJ_LEFT,
            ButtonEnd,
            CHILD_MinWidth,         indicatorwidth,
            CHILD_MaxWidth,         indicatorwidth,
          LayoutEnd,

          LAYOUT_AddChild, l2 = HLayoutObject,
            LAYOUT_AddChild, ar[ACTID_CHANNELS] = SliderObject,
              SLIDER_Min,           1,
              SLIDER_Max,           state.Channels,
              SLIDER_Level,         state.ChannelsSelected,
              SLIDER_Orientation,   SORIENT_HORIZ,
              GA_UserData,            &GadgetHook,
            SliderEnd,
            CHILD_Label, LabelObject,
              LABEL_Text,             (char *) msgOptChannels,
            LabelEnd,
            LAYOUT_AddChild, ar[SHOWID_CHANNELS] = ButtonObject,
              GA_Text,                getChannels(),
              GA_ReadOnly,            TRUE,
              BUTTON_BevelStyle,      BVS_NONE,
              BUTTON_Transparent,     TRUE,
              BUTTON_Justification,   BCJ_LEFT,
            ButtonEnd,
            CHILD_MinWidth,         indicatorwidth,
            CHILD_MaxWidth,         indicatorwidth,
          LayoutEnd,

          LAYOUT_AddChild, l3 = HLayoutObject,
            LAYOUT_AddChild, ar[ACTID_OUTVOL] = SliderObject,
              SLIDER_Min,           0,
              SLIDER_Max,           state.OutVols,
              SLIDER_Level,         state.OutVolSelected,
              SLIDER_Orientation,   SORIENT_HORIZ,
              GA_UserData,            &GadgetHook,
            SliderEnd,
            CHILD_Label, LabelObject,
              LABEL_Text,             (char *) msgOptVolume,
            LabelEnd,
            LAYOUT_AddChild, ar[SHOWID_OUTVOL] = ButtonObject,
              GA_Text,                getOutVol(),
              GA_ReadOnly,            TRUE,
              BUTTON_BevelStyle,      BVS_NONE,
              BUTTON_Transparent,     TRUE,
              BUTTON_Justification,   BCJ_LEFT,
            ButtonEnd,
            CHILD_MinWidth,         indicatorwidth,
            CHILD_MaxWidth,         indicatorwidth,
          LayoutEnd,

          LAYOUT_AddChild, l4 = HLayoutObject,
            LAYOUT_AddChild, ar[ACTID_MONVOL] = SliderObject,
              SLIDER_Min,           0,
              SLIDER_Max,           state.MonVols,
              SLIDER_Level,         state.MonVolSelected,
              SLIDER_Orientation,   SORIENT_HORIZ,
              GA_UserData,            &GadgetHook,
            SliderEnd,
            CHILD_Label, LabelObject,
              LABEL_Text,             (char *) msgOptMonitor,
            LabelEnd,
            LAYOUT_AddChild, ar[SHOWID_MONVOL] = ButtonObject,
              GA_Text,                getMonVol(),
              GA_ReadOnly,            TRUE,
              BUTTON_BevelStyle,      BVS_NONE,
              BUTTON_Transparent,     TRUE,
              BUTTON_Justification,   BCJ_LEFT,
            ButtonEnd,
            CHILD_MinWidth,         indicatorwidth,
            CHILD_MaxWidth,         indicatorwidth,
          LayoutEnd,

          LAYOUT_AddChild, l5 = HLayoutObject,
            LAYOUT_AddChild, ar[ACTID_GAIN] = SliderObject,
              SLIDER_Min,           0,
              SLIDER_Max,           state.Gains,
              SLIDER_Level,         state.GainSelected,
              SLIDER_Orientation,   SORIENT_HORIZ,
              GA_UserData,            &GadgetHook,
            SliderEnd,
            CHILD_Label, LabelObject,
              LABEL_Text,             (char *) msgOptGain,
            LabelEnd,
            LAYOUT_AddChild, ar[SHOWID_GAIN] = ButtonObject,
              GA_Text,                getGain(),
              GA_ReadOnly,            TRUE,
              BUTTON_BevelStyle,      BVS_NONE,
              BUTTON_Transparent,     TRUE,
              BUTTON_Justification,   BCJ_LEFT,
            ButtonEnd,
            CHILD_MinWidth,         indicatorwidth,
            CHILD_MaxWidth,         indicatorwidth,
          LayoutEnd,

          LAYOUT_AddChild, l6 = HLayoutObject,
            LAYOUT_AddChild, ar[ACTID_INPUT] = SliderObject,
              SLIDER_Min,           0,
              SLIDER_Max,           state.Inputs,
              SLIDER_Level,         state.InputSelected,
              SLIDER_Orientation,   SORIENT_HORIZ,
              GA_UserData,            &GadgetHook,
            SliderEnd,
            CHILD_Label, LabelObject,
              LABEL_Text,             (char *) msgOptInput,
            LabelEnd,
            LAYOUT_AddChild, ar[SHOWID_INPUT] = ButtonObject,
              GA_Text,                getInput(),
              GA_ReadOnly,            TRUE,
              BUTTON_BevelStyle,      BVS_NONE,
              BUTTON_Transparent,     TRUE,
              BUTTON_Justification,   BCJ_LEFT,
            ButtonEnd,
            CHILD_MinWidth,         indicatorwidth,
            CHILD_MaxWidth,         indicatorwidth,
          LayoutEnd,

          LAYOUT_AddChild, l7 = HLayoutObject,
            LAYOUT_AddChild, ar[ACTID_OUTPUT] = SliderObject,
              SLIDER_Min,           0,
              SLIDER_Max,           state.Outputs,
              SLIDER_Level,         state.OutputSelected,
              SLIDER_Orientation,   SORIENT_HORIZ,
              GA_UserData,            &GadgetHook,
            SliderEnd,
            CHILD_Label, LabelObject,
              LABEL_Text,             (char *) msgOptOutput,
            LabelEnd,
            LAYOUT_AddChild, ar[SHOWID_OUTPUT] = ButtonObject,
              GA_Text,                getOutput(),
              GA_ReadOnly,            TRUE,
              BUTTON_BevelStyle,      BVS_NONE,
              BUTTON_Transparent,     TRUE,
              BUTTON_Justification,   BCJ_LEFT,
            ButtonEnd,
            CHILD_MinWidth,         indicatorwidth,
            CHILD_MaxWidth,         indicatorwidth,
          LayoutEnd,

        LayoutEnd,

      LayoutEnd,
      CHILD_WeightedHeight, 0,

    LayoutEnd,

/* PAGE 2 */
    PAGE_Add, HLayoutObject,
      LAYOUT_VertAlignment, LALIGN_CENTER,
      LAYOUT_HorizAlignment,  LALIGN_CENTER,

      LAYOUT_AddChild, VLayoutObject,
        LAYOUT_SpaceOuter,      TRUE,
        LAYOUT_BevelStyle,      BVS_GROUP,
        LAYOUT_Label,           (char *) msgGlobalOptions,
        LAYOUT_VertAlignment, LALIGN_BOTTOM,

        LAYOUT_AddChild, ar[ACTID_DEBUG] = PopUpObject,
          CHOOSER_Labels,       debuglist,
          CHOOSER_Active,       globalprefs.ahigp_DebugLevel,
        ChooserEnd,
        CHILD_Label, LabelObject,
          LABEL_Text,             (char *) msgGlobOptDebugLevel,
        LabelEnd,

        LAYOUT_AddChild, ar[ACTID_ECHO] = PopUpObject,
          CHOOSER_Labels,       echolist,
          CHOOSER_Active,       (globalprefs.ahigp_DisableEcho ? 2 : 0) |
                                (globalprefs.ahigp_FastEcho    ? 1 : 0),
        ChooserEnd,
        CHILD_Label, LabelObject,
          LABEL_Text,             (char *) msgGlobOptEcho,
        LabelEnd,

        LAYOUT_AddChild, ar[ACTID_SURROUND] = PopUpObject,
          CHOOSER_Labels,       surroundlist,
          CHOOSER_Active,       globalprefs.ahigp_DisableSurround,
        ChooserEnd,
        CHILD_Label, LabelObject,
          LABEL_Text,             (char *) msgGlobOptSurround,
        LabelEnd,

        LAYOUT_AddChild, ar[ACTID_CLIPMV] = PopUpObject,
          CHOOSER_Labels,       clipMVlist,
          CHOOSER_Active,       globalprefs.ahigp_ClipMasterVolume,
        ChooserEnd,
        CHILD_Label, LabelObject,
          LABEL_Text,             (char *) msgGlobOptMasterVol,
        LabelEnd,

        LAYOUT_AddChild, ar[ACTID_CPULIMIT] = IntegerObject,
          INTEGER_MaxChars,       3,
          INTEGER_MinVisible,     3,
          INTEGER_Minimum,        0,
          INTEGER_Maximum,        100,
          INTEGER_Number,         (globalprefs.ahigp_MaxCPU * 100 + 32768) >> 16,
          INTEGER_Arrows,         TRUE,
        IntegerEnd,
        CHILD_Label, LabelObject,
          LABEL_Text,             (char *) msgGlobOptCPULimit,
         LabelEnd,

      LayoutEnd,
      CHILD_WeightedHeight, 0,
      CHILD_WeightedWidth, 0,
    LayoutEnd,

  PageEnd;

  if(ar[ACTID_PAGE] != NULL) {
    SetAttrs( l1, LAYOUT_AlignLabels, l2, TAG_DONE);
    SetAttrs( l2, LAYOUT_AlignLabels, l3, TAG_DONE);
    SetAttrs( l3, LAYOUT_AlignLabels, l4, TAG_DONE);
    SetAttrs( l4, LAYOUT_AlignLabels, l5, TAG_DONE);
    SetAttrs( l5, LAYOUT_AlignLabels, l6, TAG_DONE);
    SetAttrs( l6, LAYOUT_AlignLabels, l7, TAG_DONE);
    SetAttrs( l7, LAYOUT_AlignLabels, l1, TAG_DONE);
  }

  WO_Window = WindowObject,
    WA_PubScreen,           screen,
    WA_Title,               (char *) msgTextProgramName,
    WA_CloseGadget,         TRUE,
    WA_DepthGadget,         TRUE,
    WA_DragBar,             TRUE,
    WA_Activate,            TRUE,
    WA_SizeGadget,          TRUE,
    WA_SizeBBottom,         TRUE,
    WA_NewLookMenus,        TRUE,
    WA_InnerWidth,          800,
    WA_InnerHeight,         400,

    WINDOW_MenuStrip,       Menu,
    WINDOW_Position,        WPOS_CENTERSCREEN,
    WINDOW_GadgetUserData,  WGUD_HOOK,
    WINDOW_IDCMPHook,      &IDCMPhook,
    WINDOW_IDCMPHookBits,   IDCMP_RAWKEY,

    WINDOW_Layout, VLayoutObject,
      LAYOUT_SpaceOuter,    TRUE,
      LAYOUT_DeferLayout,   TRUE,
      

/* TABS */
      LAYOUT_AddChild, ar[LAYOUT_PAGE] = VLayoutObject,
        LAYOUT_AddChild, ar[ACTID_TABS] = ClickTabObject,
          CLICKTAB_Labels,      pagelist,
          CLICKTAB_Current,     0L,
          ICA_TARGET,           ar[ACTID_PAGE],
          ICA_MAP,              MapTab2Page,
        ClickTabEnd,

        LAYOUT_AddChild, ar[ACTID_PAGE],
      LayoutEnd,

/* BUTTONS */
      LAYOUT_AddChild, HLayoutObject,
        LAYOUT_EvenSize, TRUE,

        LAYOUT_AddChild, ar[ACTID_SAVE] = ButtonObject,
          GA_Text,              (char *) msgButtonSave,
        ButtonEnd,
        CHILD_WeightedHeight, 0,

        LAYOUT_AddChild, ar[ACTID_USE] = ButtonObject,
          GA_Text,              (char *) msgButtonUse,
        ButtonEnd,
        CHILD_WeightedHeight, 0,

        LAYOUT_AddChild, ar[ACTID_QUIT] = ButtonObject,
          GA_Text,              (char *) msgButtonCancel,
        ButtonEnd,
        CHILD_WeightedHeight, 0,

      LayoutEnd,
      CHILD_WeightedHeight, 0,

    LayoutEnd,
  WindowEnd;

  if(WO_Window) {
    int i;
    for(i = 0; i < ACTID_COUNT; i++) {
      if(Window_Objs[i] != NULL) {
        SetAttrs(Window_Objs[i], GA_ID,        i,
                                 GA_RelVerify, TRUE, 
                                 TAG_DONE);
      }
    }
    Window = (struct Window *) RA_OpenWindow(WO_Window);
  }

  if(screen) {
    UnlockPubScreen(NULL, screen);
  }

  if(Window == NULL) {
    Printf((char *) msgTextNoWindow);
    Printf("\n");
    return FALSE;
  }

  openreq = GetFileObject,
    GETFILE_RejectIcons,   TRUE,
    GETFILE_Drawer,        "SYS:Prefs/Presets",
  EndObject;

  savereq = GetFileObject,
    GETFILE_RejectIcons,   TRUE,
    GETFILE_Drawer,        "SYS:Prefs/Presets",
    GETFILE_DoSaveMode,    TRUE,
  EndObject;

  if((openreq == NULL) || (savereq == NULL)) {
    Printf((char *) msgTextNoFileRequester);
    Printf("\n");
    return FALSE;
  }

  // Update the checkmark for "Create Icons?"
  {
    struct Menu     *menu;
    struct MenuItem *item;

    menu = Menu;

    ClearMenuStrip(Window);

    item = FindMenuItem(ACTID_ICONS);

    if(item) {
      if(SaveIcons)
        item->Flags |= CHECKED;
      else
        item->Flags &= ~CHECKED;
    }
    ResetMenuStrip(Window, menu);
  }

  GUINewUnit();
  return TRUE;
}


/******************************************************************************
**** Call to close the window *************************************************
******************************************************************************/

void CloseGUI(void) {

  if(savereq) DisposeObject(savereq);
  if(openreq) DisposeObject(openreq);

  savereq   = NULL;
  openreq   = NULL;

  if(WO_Window) DisposeObject(WO_Window);

  WO_Window = NULL;
  Window    = NULL;

  /* Dynamic */
  if(unitlist)     FreeChooserLabels(unitlist);
  if(modelist)     FreeBrowserNodes(modelist);
  if(infolist)     FreeBrowserNodes2(infolist);

  /* Static */
  if(pagelist)     FreeClickTabs(pagelist);
  if(debuglist)    FreeChooserLabels(debuglist);
  if(echolist)     FreeChooserLabels(echolist);
  if(surroundlist) FreeChooserLabels(surroundlist);
  if(clipMVlist)   FreeChooserLabels(clipMVlist);

  unitlist = modelist = infolist =
  pagelist = debuglist = echolist = surroundlist = clipMVlist = NULL;

  if(Menu) FreeMenus(Menu);

  Menu = NULL;

  if(vi) FreeVisualInfo(vi);

  vi = NULL;

  if(infotext) FreeVec(infotext);

  infotext = NULL;

}



/******************************************************************************
**** Handles the input events *************************************************
******************************************************************************/

void EventLoop(void) {
  ULONG signal  = 0, rc;
  WORD  code;
  BOOL  running = TRUE;

  GetAttr( WINDOW_SigMask, WO_Window, &signal );
  if(signal) {
    do {
      if(Wait(signal | SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C ) {
        running=FALSE;
      }

      while (( rc = RA_HandleInput( WO_Window, &code)) != WMHI_LASTMSG) {

        switch(rc & WMHI_CLASSMASK) {

          case WMHI_CLOSEWINDOW:
            running=FALSE;
            break;

          case WMHI_GADGETUP:
          case WMHI_MENUPICK:
          {

            if((rc & WMHI_CLASSMASK) == WMHI_MENUPICK) {
              struct MenuItem *mi;
              
              mi = ItemAddress(Menu, rc & WMHI_MENUMASK);
              
              if(mi) {
                rc = (ULONG) GTMENUITEM_USERDATA(mi);
              }
            }


            switch( rc & WMHI_GADGETMASK) {

#if 0
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
              
#endif
              case ACTID_ABOUT:
                Req( (char *) msgButtonOK,
                    (char *) msgTextCopyright,
                    "",(char *) msgTextProgramName,
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
                SetAttrs( WO_Window, WA_BusyPointer, TRUE, TAG_DONE);
                NewSettings(NULL);    // This is VERY slow!!
                SetAttrs( WO_Window, WA_BusyPointer, FALSE, TAG_DONE);
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

#if 0
    
              case ACTID_HELP: {
                APTR lock = BGUI_LockWindow(Window);
                if(!BGUI_Help( Window, HELPFILE, "AHI", NULL)) {
                  Req( (char *) msgButtonOK, (char *) msgTextNoFind, HELPFILE);
                }
                BGUI_UnlockWindow(lock);
                break;
              }
    
              case ACTID_GUIDE: {
                APTR lock = BGUI_LockWindow(Window);
                if(!BGUI_Help( Window, HELPFILE, NULL, NULL)) {
                  Req( (char *) msgButtonOK, (char *) msgTextNoFind, HELPFILE);
                }
                BGUI_UnlockWindow(lock);
                break;
              }
    
              case ACTID_HELPINDEX: {
                APTR lock = BGUI_LockWindow(Window);
                if(!BGUI_Help( Window, HELPFILE, "Concept Index", 0)) {
                  Req( (char *) msgButtonOK, (char *) msgTextNoFind, HELPFILE);
                }
                BGUI_UnlockWindow(lock);
                break;
              }
#endif
    
              case ACTID_UNIT: {
                FillUnit();
                NewUnit(code);
                GUINewUnit();
                break;
              }
    
              case ACTID_MODE: {
                FillUnit();
                NewMode(code);
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
    
                GetAttr( CHOOSER_Active, Window_Objs[ACTID_DEBUG],    &debug);
                GetAttr( CHOOSER_Active, Window_Objs[ACTID_SURROUND], &surround);
                GetAttr( CHOOSER_Active, Window_Objs[ACTID_ECHO],     &echo);
                GetAttr( CHOOSER_Active, Window_Objs[ACTID_CLIPMV],   &clip);
                GetAttr( INTEGER_Number, Window_Objs[ACTID_CPULIMIT], &cpu);
    
                globalprefs.ahigp_DebugLevel      = debug;
                globalprefs.ahigp_DisableSurround = surround;
                globalprefs.ahigp_DisableEcho     = (echo == 2);
                globalprefs.ahigp_FastEcho        = (echo == 1);
                globalprefs.ahigp_MaxCPU = (cpu << 16) / 100;
                globalprefs.ahigp_ClipMasterVolume= clip;
    
                break;
              }
            } /* switch(GADGETMASK) */
          }
        } /* switch(CLASSMASK) */
      } /* HandleInput */
    } while(running);
  }
}
