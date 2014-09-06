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

#include <config.h>

#include <exec/memory.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>

#include <clib/alib_protos.h>
#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#define NO_INLINE_STDARG
#include <proto/muimaster.h>
#undef NO_INLINE_STDARG

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "ahi.h"
#include "ahiprefs_Cat.h"
#include "support.h"
#include "gui.h"



#if defined( __morphos__ ) || defined( __MORPHOS__ )

#ifndef DoMethod
#define DoMethod(obj, ... )         \
({                                  \
  ULONG _args[] = { __VA_ARGS__ };  \
  DoMethodA(obj,(Msg) _args);       \
})
#endif

/* Why the #$*@ is this not in libamiga.a?! */

static ULONG
gw_HookEntry( void )
{
  struct Hook* h   = (struct Hook*) REG_A0;
  void*        o   = (void*)        REG_A2;
  void*        msg = (void*)        REG_A1;

  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
}

struct EmulLibEntry _HookEntry =
{
  TRAP_LIB, 0, (void (*)(void)) &gw_HookEntry
};

__asm( ".globl HookEntry;HookEntry=_HookEntry" );


#elif defined(__AMIGAOS4__)

ULONG HookEntry(struct Hook* h, void* o, void* msg) {
  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
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

  ACTID_PLAY,

  ACTID_DEBUG, ACTID_SURROUND, ACTID_ECHO, ACTID_CLIPMV,
  ACTID_CPULIMIT, SHOWID_CPULIMIT,
  ACTID_ACTIME, SHOWID_ACTIME,
  ACTID_SCALEMODE,


  ACTID_COUNT
};

#define Title(t)        { NM_TITLE, t, NULL, 0, 0, NULL }
#define Item(t,s,i)     { NM_ITEM, t, s, 0, 0, (APTR)i }
#define ItemBar         { NM_ITEM, NM_BARLABEL, NULL, 0, 0, NULL }
#define SubItem(t,s,i)  { NM_SUB, t, s, 0, 0, (APTR)i }
#define SubBar          { NM_SUB, NM_BARLABEL, NULL, 0, 0, NULL }
#define EndMenu         { NM_END, NULL, NULL, 0, 0, NULL }
#define ItCk(t,s,i,f)   { NM_ITEM, t, s, f, 0, (APTR)i }

struct Library       *MUIMasterBase  = NULL;

#ifdef __AMIGAOS4__
struct MUIMasterIFace *IMUIMaster = NULL;
#endif

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
  EndMenu
};

static char *PageNames[] =
{
  NULL,  /* Mode settings */
  NULL,  /* Advanced settings */
  NULL
};

static char * ScaleLabels[] = {
  NULL,  /* Safe */
  NULL,  /* Safe, dynamic */
  NULL,  /* Full volume */
  NULL,  /* -3 dB */
  NULL,  /* -6 dB */
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
  ScaleLabels[0] = (char *) msgSMFixedSafe;
  ScaleLabels[1] = (char *) msgSMDynSafe;
  ScaleLabels[2] = (char *) msgSM0dB;
  ScaleLabels[3] = (char *) msgSM3dB;
  ScaleLabels[4] = (char *) msgSM6dB;
}


static Object *MUIWindow,*MUIList,*MUIInfos,*MUIUnit;
static Object *MUIFreq,*MUIChannels,*MUIOutvol,*MUIMonvol,*MUIGain,*MUIInput,*MUIOutput;
static Object *MUILFreq,*MUILChannels,*MUILOutvol,*MUILMonvol,*MUILGain,*MUILInput,*MUILOutput,*MUIPlay;
static Object *MUIDebug,*MUIEcho,*MUISurround,*MUIClipvol,*MUICpu,*MUIACTime,*MUIScalemode;

LONG xget(Object * obj, ULONG attribute)
{
  LONG x = 0;

  get(obj, attribute, &x);
  return (x);
}

static void GUINewSettings(void)
{
  set(MUIUnit,MUIA_Cycle_Entries,Units);
  set(MUIUnit,MUIA_Cycle_Active,state.UnitSelected);
  set(MUIDebug, MUIA_Cycle_Active, globalprefs.ahigp_DebugLevel);
  set(MUIEcho, MUIA_Cycle_Active, (globalprefs.ahigp_DisableEcho ? 2 : 0)|(globalprefs.ahigp_FastEcho    ? 1 : 0));
  set(MUISurround, MUIA_Cycle_Active, globalprefs.ahigp_DisableSurround);
  set(MUIClipvol, MUIA_Cycle_Active, globalprefs.ahigp_ClipMasterVolume);
  set(MUICpu, MUIA_Cycle_Active, (globalprefs.ahigp_MaxCPU * 100 + 32768) >> 16);
  set(MUIACTime, MUIA_Cycle_Active, (globalprefs.ahigp_AntiClickTime * 1000 + 32768) >> 16);
  set(MUIScalemode, MUIA_Cycle_Active, globalprefs.ahigp_ScaleMode);

  GUINewUnit();
}

static void GUINewUnit(void)
{
  DoMethod(MUIList, MUIM_List_Clear);
  set(MUIList, MUIA_List_Quiet, TRUE);
  DoMethod(MUIList, MUIM_List_Insert, Modes, -1, MUIV_List_Insert_Bottom);
  set(MUIList, MUIA_List_Quiet, FALSE);
  set(MUIList, MUIA_List_Active, state.ModeSelected != ~0 ? state.ModeSelected : MUIV_List_Active_Off);
  GUINewMode();
}

static void GUINewMode(void)
{
  int Max, Sel, Dis;
  char* buffer;
  char* arg1 = getRecord();
  char* arg2 = getAuthor();
  char* arg3 = getCopyright();
  char* arg4 = getDriver();
  char* arg5 = getVersion();
  char* arg6 = getAnnotation();

  buffer = AllocVec( strlen( arg1 ) +
                     strlen( arg2 ) +
                     strlen( arg3 ) +
                     strlen( arg4 ) +
                     strlen( arg5 ) +
                     strlen( arg6 ) +
                     128,
                     MEMF_ANY);

  if( buffer != NULL )
  {
    sprintf( buffer,"0x%08lx\n%s\n%s\n%s\nDevs:AHI/%s.audio\n%s\n%s",
      getAudioMode(),
      arg1,
      arg2,
      arg3,
      arg4,
      arg5,
      arg6 );

    set(MUIInfos, MUIA_Text_Contents, (IPTR) buffer);

    FreeVec(buffer);
  }

  Max = max(state.Frequencies -1, 0);
  Sel = min(Max, state.FreqSelected);
  Dis = Max==0;
  set(MUIFreq, MUIA_Disabled, Dis);
  if( !Dis )
  {
    set(MUIFreq, MUIA_Numeric_Max, Max);
    set(MUIFreq, MUIA_Numeric_Value, Sel);
  }
  set(MUILFreq, MUIA_Text_Contents, (IPTR) getFreq());

  Max = max(state.Channels, 0);
  Sel = min(Max, state.ChannelsSelected);
  Dis = (Max == 0 || Max == 1) || state.ChannelsDisabled;
  set(MUIChannels, MUIA_Disabled, Dis);
  if( !Dis )
  {
    set(MUIChannels, MUIA_Numeric_Max, Max);
    set(MUIChannels, MUIA_Numeric_Value, Sel);
  }
  set(MUILChannels, MUIA_Text_Contents, (IPTR) getChannels());

  Max = max(state.OutVols -1, 0);
  Sel = min(Max, state.OutVolSelected);
  Dis = Max==0;
  set(MUIOutvol, MUIA_Disabled, Dis);
  if( !Dis )
  {
    set(MUIOutvol, MUIA_Numeric_Max, Max);
    set(MUIOutvol, MUIA_Numeric_Value, Sel);
  }
  set(MUILOutvol, MUIA_Text_Contents, (IPTR) getOutVol());

  Max = max(state.MonVols -1, 0);
  Sel = min(Max, state.MonVolSelected);
  Dis = Max==0;
  set(MUIMonvol, MUIA_Disabled, Dis);
  if( !Dis )
  {
    set(MUIMonvol, MUIA_Numeric_Max, Max);
    set(MUIMonvol, MUIA_Numeric_Value, Sel);
  }
  set(MUILMonvol, MUIA_Text_Contents, (IPTR) getMonVol());

  Max = max(state.Gains -1, 0);
  Sel = min(Max, state.GainSelected);
  Dis = Max==0;
  set(MUIGain, MUIA_Disabled, Dis);
  if( !Dis )
  {
    set(MUIGain, MUIA_Numeric_Max, Max);
    set(MUIGain, MUIA_Numeric_Value, Sel);
  }
  set(MUILGain, MUIA_Text_Contents, (IPTR) getGain());

  Max = max(state.Inputs -1, 0);
  Sel = min(Max, state.InputSelected);
  Dis = Max==0;
  set(MUIInput, MUIA_Disabled, Dis);
  if( !Dis )
  {
    set(MUIInput, MUIA_Numeric_Max, Max);
    set(MUIInput, MUIA_Numeric_Value, Sel);
  }
  set(MUILInput, MUIA_Text_Contents, (IPTR) getInput());

  Max = max(state.Outputs -1, 0);
  Sel = min(Max, state.OutputSelected);
  Dis = Max==0;
  set(MUIOutput, MUIA_Disabled, Dis);
  if( !Dis )
  {
    set(MUIOutput, MUIA_Numeric_Max, Max);
    set(MUIOutput, MUIA_Numeric_Value, Sel);
  }
  set(MUILOutput, MUIA_Text_Contents, (IPTR) getOutput());


  set(MUIPlay, MUIA_Disabled, getAudioMode() == AHI_INVALID_ID);
}

static VOID
SliderHookFunc( struct Hook *hook,
                Object *obj,
                ULONG** arg )
{
  if(obj == MUIFreq)
  {
    state.FreqSelected = (LONG) (*arg);
    set(MUILFreq,MUIA_Text_Contents, (IPTR) getFreq());
  }
  else if(obj == MUIChannels )
  {
    state.ChannelsSelected = (LONG) (*arg);
    set(MUILChannels,MUIA_Text_Contents, (IPTR) getChannels());
  }
  else if(obj == MUIOutvol )
  {
    state.OutVolSelected = (LONG) (*arg);
    set(MUILOutvol,MUIA_Text_Contents, (IPTR) getOutVol());
  }
  else if(obj == MUIMonvol )
  {
    state.MonVolSelected = (LONG) (*arg);
    set(MUILMonvol,MUIA_Text_Contents, (IPTR) getMonVol());
  }
  else if(obj == MUIGain )
  {
    state.GainSelected = (LONG) (*arg);
    set(MUILGain,MUIA_Text_Contents, (IPTR) getGain());
  }
  else if(obj == MUIInput )
  {
    state.InputSelected = (LONG) (*arg);
    set(MUILInput,MUIA_Text_Contents, (IPTR) getInput());
  }
  else if(obj == MUIOutput )
  {
  state.OutputSelected = (ULONG) (*arg);
  set(MUILOutput,MUIA_Text_Contents, (IPTR) getOutput());
  }
}

static struct Hook hookSlider =
{
  { NULL, NULL },
  HookEntry,
  (HOOKFUNC) SliderHookFunc,
  NULL
};

/******************************************************************************
**** Call to open the window **************************************************
******************************************************************************/

static Object *MUIApp,*MUIMenu;

static Object* SpecialLabel(STRPTR label)
{
  return(TextObject,
      MUIA_HorizWeight, 75,
      MUIA_Text_Contents, label,
      MUIA_Text_PreParse, "\33l",
    End);
}

static Object* SpecialButton(STRPTR label)
{
  Object *button = NULL;
  STRPTR lab;

  lab = AllocVec(strlen(label)+1,0);

  if( lab != NULL )
  {
    char ctrlchar = 0;
    STRPTR l = lab;

    while(*label)
    {
      *l = *label;
      if(*label++ == '_')
      {
        ctrlchar = ToLower(*label);
        *l = *label++;
      }
      l++;
    }
    *l = '\0';
    button = TextObject,
      MUIA_HorizWeight, 0,
      MUIA_Text_Contents, lab,
      MUIA_Text_PreParse, "\33r",
      MUIA_Text_HiChar, ctrlchar,
      MUIA_ControlChar, ctrlchar,
      MUIA_InputMode, MUIV_InputMode_RelVerify,
      MUIA_ShowSelState, FALSE,
    End;
    FreeVec(lab);
  }
  return button;
}

static Object* SpecialSlider(LONG min, LONG max, LONG value)
{
  return(SliderObject,
	 MUIA_CycleChain,     1,
	 MUIA_Slider_Quiet,   TRUE,
	 MUIA_Numeric_Min,    min,
	 MUIA_Numeric_Max,    max >= min ? max : min,
	 MUIA_Numeric_Value,  value >= min && value <=max ? value : min,
	 MUIA_Numeric_Format, "",
    End);
}

BOOL BuildGUI(char *screenname)
{
  Object *MUISave, *MUIUse, *MUICancel;
  Object *page1,*page2;
  Object *MUITFreq,*MUITChannels,*MUITOutvol,*MUITMonvol,*MUITGain,*MUITInput,*MUITOutput,*MUITDebug,*MUITEcho,*MUITSurround,*MUITClipvol,*MUITCpu,*MUITACTime,*MUITScalemode;

  UpdateStrings();

  MUIMasterBase = (void *)OpenLibrary("muimaster.library", MUIMASTER_VLATEST);
  if(MUIMasterBase == NULL)
  {
    Printf((char *) msgTextNoOpen, (ULONG) "muimaster.library", MUIMASTER_VLATEST);
    Printf("\n");
    return FALSE;
  }

#ifdef __AMIGAOS4__
  IMUIMaster = (struct MUIMasterIFace *) GetInterface(MUIMasterBase, "main", 1, NULL);
  if(IMUIMaster == NULL)
  {
    Printf((char *) msgTextNoOpen, (ULONG) "MUIMaster main interface", 1);
    Printf("\n");
    CloseLibrary((struct Library*) MUIMasterBase);
    return FALSE;
  }
#endif


  page1 = HGroup,
    Child, VGroup,
      Child, MUIUnit = CycleObject,
        MUIA_CycleChain, 1,
        MUIA_Cycle_Entries, Units,
        MUIA_Cycle_Active, state.UnitSelected,
      End,
      Child, ListviewObject,
        MUIA_CycleChain, 1,
        MUIA_Listview_List, MUIList = ListObject,
          InputListFrame,
          MUIA_List_AutoVisible, TRUE,
        End,
      End,
      Child, HGroup,
        ReadListFrame,
        MUIA_Background, MUII_TextBack,
        Child, TextObject,
          MUIA_Text_Contents, msgProperties,
          MUIA_Text_SetMax, TRUE,
        End,
        Child, MUIInfos = TextObject,
          MUIA_Text_Contents, "\n\n\n\n\n\n",
          MUIA_Text_SetMin, FALSE,
        End,
      End,
    End,
    Child, BalanceObject,
    End,
    Child, VGroup,
      Child, HVSpace,
      Child, ColGroup(3),
        GroupFrameT(msgOptions),
        Child, MUITFreq = SpecialButton((STRPTR)msgOptFrequency),
        Child, MUIFreq = SpecialSlider(0,max(state.Frequencies-1,0),state.FreqSelected),
        Child, MUILFreq = SpecialLabel(getFreq()),
        Child, MUITChannels = SpecialButton((STRPTR)msgOptChannels),
        Child, MUIChannels = SpecialSlider(1,state.Channels,state.ChannelsSelected),
        Child, MUILChannels = SpecialLabel(getChannels()),
        Child, MUITOutvol = SpecialButton((STRPTR)msgOptVolume),
        Child, MUIOutvol = SpecialSlider(0,max(state.OutVols-1,0),state.OutVolSelected),
        Child, MUILOutvol = SpecialLabel(getOutVol()),
        Child, MUITMonvol = SpecialButton((STRPTR)msgOptMonitor),
        Child, MUIMonvol = SpecialSlider(0,max(state.MonVols-1,1),state.MonVolSelected),
        Child, MUILMonvol = SpecialLabel(getMonVol()),
        Child, MUITGain = SpecialButton((STRPTR)msgOptGain),
        Child, MUIGain = SpecialSlider(0,max(state.Gains-1,0),state.GainSelected),
        Child, MUILGain = SpecialLabel(getGain()),
        Child, MUITInput = SpecialButton((STRPTR)msgOptInput),
        Child, MUIInput = SpecialSlider(0,max(state.Inputs-1,0),state.InputSelected),
        Child, MUILInput = SpecialLabel(getInput()),
        Child, MUITOutput = SpecialButton((STRPTR)msgOptOutput),
        Child, MUIOutput = SpecialSlider(0,max(state.Outputs-1,0),state.OutputSelected),
        Child, MUILOutput = SpecialLabel(getOutput()),
      End,
      Child, MUIPlay = SimpleButton(msgButtonPlay),
      Child, HVSpace,
    End,
  End;

  page2 = VGroup,
    Child, HVSpace,
    Child, HGroup,
      Child, HVSpace,
      Child, ColGroup(2),
        GroupFrameT(msgGlobalOptions),
        Child, MUITDebug = SpecialButton((STRPTR)msgGlobOptDebugLevel),
        Child, MUIDebug = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, DebugLabels,
          MUIA_Cycle_Active, globalprefs.ahigp_DebugLevel,
        End,
        Child, MUITEcho = SpecialButton((STRPTR)msgGlobOptEcho),
        Child, MUIEcho  = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, EchoLabels,
          MUIA_Cycle_Active, (globalprefs.ahigp_DisableEcho ? 2 : 0) | (globalprefs.ahigp_FastEcho ? 1 : 0),
          MUIA_Disabled, AHIBase->lib_Version >= 5,
        End,
        Child, MUITSurround = SpecialButton((STRPTR)msgGlobOptSurround),
        Child, MUISurround = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, SurroundLabels,
          MUIA_Cycle_Active, globalprefs.ahigp_DisableSurround,
          MUIA_Disabled, AHIBase->lib_Version >= 5,
        End,
        Child, MUITClipvol = SpecialButton((STRPTR)msgGlobOptMasterVol),
        Child, MUIClipvol = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, ClipMVLabels,
          MUIA_Cycle_Active, globalprefs.ahigp_ClipMasterVolume,
          MUIA_Disabled, AHIBase->lib_Version >= 5,
        End,
        Child, MUITCpu = SpecialButton((STRPTR)msgGlobOptCPULimit),
        Child, MUICpu = SliderObject,
          MUIA_CycleChain, 1,
          MUIA_Slider_Horiz, TRUE,
          MUIA_Numeric_Min, 0,
          MUIA_Numeric_Max, 100,
          MUIA_Numeric_Value,(globalprefs.ahigp_MaxCPU * 100 + 32768) / 65536,
          MUIA_Numeric_Format,msgPercentFmt,
        End,
        Child, MUITACTime = SpecialButton((STRPTR)msgGlobOptACTime),
        Child, MUIACTime = SliderObject,
          MUIA_CycleChain, 1,
          MUIA_Slider_Horiz, TRUE,
          MUIA_Numeric_Min, 0,
          MUIA_Numeric_Max, 100,
          MUIA_Numeric_Value,(globalprefs.ahigp_AntiClickTime * 1000 + 32768) >> 16,
          MUIA_Numeric_Format,msgACTimeFmt,
          MUIA_Disabled, AHIBase->lib_Version <= 4,
        End,
        Child, MUITScalemode = SpecialButton((STRPTR)msgOptScalemode),
        Child, MUIScalemode = CycleObject,
          MUIA_CycleChain, 1,
          MUIA_Cycle_Entries, ScaleLabels,
          MUIA_Cycle_Active, globalprefs.ahigp_ScaleMode,
          MUIA_Disabled, AHIBase->lib_Version <= 4,
        End,
      End,
      Child, HVSpace,
    End,
    Child, HVSpace,
  End;

  MUIApp = ApplicationObject,
    MUIA_Application_Title, (char *) msgTextProgramName,
    MUIA_Application_Version, Version,
    MUIA_Application_Copyright, "©1996-2005 Martin Blom",
    MUIA_Application_Author, "Stéphane Barbaray/Martin Blom",
    MUIA_Application_Description, (char *) msgTextProgramName,
    MUIA_Application_Base, "AHI",
    MUIA_Application_HelpFile, HELPFILE,
    MUIA_Application_Menustrip, MUIMenu = MUI_MakeObject(MUIO_MenustripNM,Menus,0),
    MUIA_Application_SingleTask, TRUE,
    SubWindow, MUIWindow = WindowObject,
      MUIA_Window_Title, (char *) msgTextProgramName,
      MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
      MUIA_HelpNode, "AHI",
      WindowContents, VGroup,
       Child, RegisterGroup(PageNames),
         MUIA_CycleChain, 1,
         Child, page1,
         Child, page2,
       End,
       Child, HGroup,
          Child, MUISave = ImageButton(msgButtonSave, "THEME:Images/Gadgets/Save"),
          Child, MUIUse =  ImageButton(msgButtonUse,"THEME:Images/Gadgets/Use"),
          Child, MUICancel =  ImageButton(msgButtonCancel,"THEME:Images/Gadgets/Cancel"),
        End,
      End,
    End,
  End;

  if( MUIApp != NULL )
  {
    APTR item = (APTR)DoMethod(MUIMenu,MUIM_FindUData,ACTID_ICONS);
    if(item)
    {
      set(item, MUIA_Menuitem_Checked, SaveIcons);
    }
    DoMethod(MUIWindow, MUIM_MultiSet, MUIA_Text_PreParse,"\033l",MUILFreq,MUILChannels,MUILOutvol,MUILMonvol,MUILGain,MUILInput,MUILOutput,NULL);
    DoMethod(MUIWindow, MUIM_MultiSet, MUIA_CycleChain, 1, MUISave,MUIUse,MUICancel,NULL);

    DoMethod(MUITFreq, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIFreq);
    DoMethod(MUITChannels, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIChannels);
    DoMethod(MUITOutvol, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIOutvol);
    DoMethod(MUITMonvol, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIMonvol);
    DoMethod(MUITGain, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIGain);
    DoMethod(MUITInput, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIInput);
    DoMethod(MUITOutput, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIOutput);
    DoMethod(MUIPlay, MUIM_Notify, MUIA_Pressed, FALSE, MUIApp, 2, MUIM_Application_ReturnID, ACTID_PLAY);
    DoMethod(MUITDebug, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIDebug);
    DoMethod(MUITEcho, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIEcho);
    DoMethod(MUITSurround, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUISurround);
    DoMethod(MUITClipvol, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIClipvol);
    DoMethod(MUITCpu, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUICpu);
    DoMethod(MUITACTime, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIACTime);
    DoMethod(MUITScalemode, MUIM_Notify, MUIA_Pressed, TRUE, MUIWindow, 3, MUIM_Set, MUIA_Window_ActiveObject, MUIScalemode);

    DoMethod(MUIWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIApp, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(MUISave, MUIM_Notify, MUIA_Pressed, FALSE, MUIApp, 2, MUIM_Application_ReturnID, ACTID_SAVE);
    DoMethod(MUIUse, MUIM_Notify, MUIA_Pressed, FALSE, MUIApp, 2, MUIM_Application_ReturnID, ACTID_USE);
    DoMethod(MUICancel, MUIM_Notify, MUIA_Pressed, FALSE, MUIApp, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(MUIUnit, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID, ACTID_UNIT);
    DoMethod(MUIList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID, ACTID_MODE);
    DoMethod(MUIDebug, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_DEBUG);
    DoMethod(MUISurround, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_SURROUND);
    DoMethod(MUIEcho, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_ECHO);
    DoMethod(MUICpu, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_CPULIMIT);
    DoMethod(MUIClipvol, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_CLIPMV);
    DoMethod(MUIACTime, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_ACTIME);
    DoMethod(MUIScalemode, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime, MUIApp, 2, MUIM_Application_ReturnID,  ACTID_SCALEMODE);
    DoMethod(MUIFreq, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIChannels, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIOutvol, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIMonvol, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIGain, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIInput, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    DoMethod(MUIOutput, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Self, 3, MUIM_CallHook, &hookSlider, MUIV_TriggerValue);
    set(MUIWindow, MUIA_Window_Open, TRUE);
    GUINewUnit();
    return TRUE;
  }
  return FALSE;
}

/******************************************************************************
**** Call to close the window *************************************************
******************************************************************************/

void CloseGUI(void)
{
  if (MUIApp)
    MUI_DisposeObject(MUIApp);
  if (MUIMasterBase)
    CloseLibrary(MUIMasterBase);
#ifdef __AMIGAOS4__
  DropInterface((struct Interface*) IMUIMaster);
#endif
}


/******************************************************************************
**** Handles the input events *************************************************
******************************************************************************/

void EventLoop(void)
{
  ULONG sigs = 0UL;

  while (1)
  {
    ULONG rc = DoMethod(MUIApp, MUIM_Application_NewInput, &sigs);

    switch(rc)
    {
      case MUIV_Application_ReturnID_Quit:
        return;

      case ACTID_OPEN:
      {
        struct FileRequester *request;

        request = MUI_AllocAslRequestTags(ASL_FileRequest,
            ASLFR_Window, xget(MUIWindow, MUIA_Window_Window),
            ASLFR_TitleText, msgTextProgramName,
            ASLFR_RejectIcons, TRUE,
            ASLFR_InitialDrawer, "SYS:Prefs/Presets",
            TAG_DONE );
        if( request != NULL )
        {
          DoMethod(MUIApp, MUIA_Application_Sleep, TRUE);
          if (MUI_AslRequest(request, NULL))
          {
            char *file;

            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);

            file = AllocVec(strlen(request->fr_Drawer)+128,0);

            if( file != NULL )
            {
              CopyMem(request->fr_Drawer, file, strlen(request->fr_Drawer)+1);
              AddPart(file, request->fr_File, 128);
              NewSettings(file);
              GUINewSettings();
              FreeVec(file);
            }
          }
          else
          {
            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);
          }
          MUI_FreeAslRequest(request);
        }
        break;
      }

      case ACTID_SAVEAS:
      {
        struct FileRequester *request;

        request = MUI_AllocAslRequestTags(ASL_FileRequest,
            ASLFR_Window, xget(MUIWindow, MUIA_Window_Window),
            ASLFR_TitleText, msgTextProgramName,
            ASLFR_RejectIcons, TRUE,
            ASLFR_DoSaveMode, TRUE,
            ASLFR_InitialDrawer, "SYS:Prefs/Presets",
            TAG_DONE);

        if(request != NULL )
        {
          DoMethod(MUIApp, MUIA_Application_Sleep, TRUE);
          FillUnit();
          if (MUI_AslRequest(request, NULL))
          {
            char *file;

            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);

            file = AllocVec(strlen(request->fr_Drawer)+128,0);

            if( file != NULL )
            {
              CopyMem(request->fr_Drawer, file, strlen(request->fr_Drawer)+1);
              AddPart(file, request->fr_File, 128);
              SaveSettings(file, UnitList);
              if(SaveIcons)
              {
                WriteIcon(file);
              }
              FreeVec(file);
            }
          }
          else
          {
            DoMethod(MUIApp, MUIA_Application_Sleep, FALSE);
          }
          MUI_FreeAslRequest(request);
        }
        break;
      }

      case ACTID_ABOUT:
      {
        char* args[] = { "\033c",
                         (char*)msgTextProgramName,
                         "1996-2005 Stéphane Barbaray/Martin Blom"
                       };

        MUI_RequestA(MUIApp, MUIWindow, 0, (char *) msgTextProgramName,
            (char*)msgButtonOK, (char*)msgTextCopyright, args );
        break;
      }

      case ACTID_SAVE:
        FillUnit();
        SaveSettings(ENVFILE, UnitList);
        SaveSettings(ENVARCFILE, UnitList);
        return;

      case ACTID_USE:
        FillUnit();
        SaveSettings(ENVFILE, UnitList);
        return;

      case ACTID_QUIT:
        return;

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
        APTR item = (APTR)DoMethod(MUIMenu,MUIM_FindUData,ACTID_ICONS);

        if(item)
        {
          if(xget(item, MUIA_Menuitem_Checked))
          {
            SaveIcons = TRUE;
          }
          else
          {
            SaveIcons = FALSE;
          }
        }
        break;
      }

      case ACTID_HELP:
        DoMethod(MUIApp,MUIM_Application_ShowHelp,NULL,NULL,"AHI",0);
        break;

      case ACTID_GUIDE:
        DoMethod(MUIApp,MUIM_Application_ShowHelp,NULL,NULL,"MAIN",0);
        break;

      case ACTID_HELPINDEX:
        DoMethod(MUIApp,MUIM_Application_ShowHelp,NULL,NULL,"Concept Index",0);
        break;

      case ACTID_UNIT:
        FillUnit();
        NewUnit(xget(MUIUnit,MUIA_Cycle_Active));
        GUINewUnit();
        break;

      case ACTID_MODE:
        FillUnit();
        NewMode(xget(MUIList, MUIA_List_Active));
        GUINewMode();
        break;

      case ACTID_PLAY:
      {
        int              unit_id;
        struct UnitNode* unit;

        FillUnit();
        unit_id = xget( MUIUnit, MUIA_Cycle_Active );
        unit    = (struct UnitNode *) GetNode( unit_id, UnitList );

        PlaySound( &unit->prefs );
        break;
      }

      case ACTID_DEBUG:
      case ACTID_SURROUND:
      case ACTID_ECHO:
      case ACTID_CPULIMIT:
      case ACTID_CLIPMV:
      case ACTID_ACTIME:
      case ACTID_SCALEMODE:
      {
        ULONG debug = AHI_DEBUG_NONE, surround = FALSE, echo = 0, cpu = 90;
        ULONG clip = FALSE, actime = 0, scalemode = AHI_SCALE_FIXED_0_DB;

        get(MUIDebug, MUIA_Cycle_Active, &debug);
        get(MUISurround, MUIA_Cycle_Active, &surround);
        get(MUIEcho, MUIA_Cycle_Active, &echo);
        get(MUIClipvol, MUIA_Cycle_Active, &clip);
        get(MUICpu, MUIA_Numeric_Value, &cpu);
        get(MUIACTime, MUIA_Numeric_Value, &actime);
        get(MUIScalemode, MUIA_Cycle_Active, &scalemode);

        globalprefs.ahigp_DebugLevel       = debug;
        globalprefs.ahigp_DisableSurround  = surround;
        globalprefs.ahigp_DisableEcho      = (echo == 2);
        globalprefs.ahigp_FastEcho         = (echo == 1);
        globalprefs.ahigp_MaxCPU           = ((cpu << 16) + 50) / 100;
        globalprefs.ahigp_ClipMasterVolume = clip;
        globalprefs.ahigp_AntiClickTime    = ((actime << 16) + 500) / 1000;
	globalprefs.ahigp_ScaleMode        = scalemode;

        break;
      }
    }

    if (sigs)
    {
      sigs = Wait(sigs | SIGBREAKF_CTRL_C);
      if (sigs & SIGBREAKF_CTRL_C)
	return;
    }
  }
}

