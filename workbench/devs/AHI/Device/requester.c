/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2005 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>

// Fix broken includes
struct VSPrite;

#include <exec/memory.h>
#include <graphics/rpattr.h>
#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#define __NOLIBBASE__
#define __NOGLOBALIFACE__
#include <proto/ahi.h>
#undef  __NOLIBBASE__
#undef  __NOGLOBALIFACE__
#include <proto/ahi_sub.h>

#include <math.h>
#include <string.h>
#include <stddef.h>

#include "ahi_def.h"
#include "localize.h"
#include "misc.h"
#include "modeinfo.h"
#include "debug.h"
#include "gateway.h"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

struct AHIAudioModeRequesterExt;

static void OpenInfoWindow( struct AHIAudioModeRequesterExt * );
static void CloseInfoWindow( struct AHIAudioModeRequesterExt * );
static void UpdateInfoWindow( struct AHIAudioModeRequesterExt * );


/******************************************************************************
** Audio mode requester  ******************************************************
******************************************************************************/

#define MY_IDCMPS (LISTVIEWIDCMP|SLIDERIDCMP|BUTTONIDCMP|IDCMP_SIZEVERIFY|IDCMP_NEWSIZE|IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW|IDCMP_MENUPICK|IDCMP_VANILLAKEY|IDCMP_RAWKEY)
#define MY_INFOIDCMPS (LISTVIEWIDCMP|IDCMP_REFRESHWINDOW|IDCMP_CLOSEWINDOW)

#define haveIDCMP   0x0001
#define lockwin     0x0002
#define freqgad     0x0004
#define ownIDCMP    0x0008
#define defaultmode 0x0010

static const struct TagItem reqboolmap[] =
{
  { AHIR_PrivateIDCMP,   haveIDCMP   },
  { AHIR_SleepWindow,    lockwin     },
  { AHIR_DoMixFreq,      freqgad     },
  { AHIR_DoDefaultMode,  defaultmode },
  { TAG_DONE,            0           }
};

#define LASTMODEITEM  1
#define NEXTMODEITEM  2
#define PROPERTYITEM  3
#define RESTOREITEM   4
#define OKITEM        5
#define CANCELITEM    6

/* Node for audio mode requester */

struct IDnode
{
  struct Node node;
  ULONG       ID;
  char        name[80];
};

/* The attribues list */

#define ATTRNODES 6

struct Attrnode
{
  struct Node node;
  UBYTE       text[80];
};

 /* AHIAudioModeRequester extension */
struct AHIAudioModeRequesterExt
{
  struct AHIAudioModeRequester  Req;
  ULONG                         tempAudioID;
  ULONG                         tempFrequency;
  struct Window                *SrcWindow;
  STRPTR                        PubScreenName;
  struct Screen                *Screen;
  ULONG                         Flags;
  struct Hook                  *IntuiMsgFunc;
  struct TextAttr              *TextAttr;
  struct Locale                *Locale;
  STRPTR                        TitleText;
  STRPTR                        PositiveText;
  STRPTR                        NegativeText;
  struct TagItem               *FilterTags;
  struct Hook                  *FilterFunc;

//  struct Screen                *PubScreen;
  struct Window                *Window;
  struct Window                *InfoWindow;
  WORD                          gx,gy,gw,gh;
  APTR                          vi;
  struct Gadget                *Gadgets;
  struct Gadget                *InfoGadgets;
  struct Gadget                *InfoListViewGadget;
  struct Gadget                *listviewgadget;
  struct Gadget                *slidergadget;
  struct MinList               *list;
  struct MinList                InfoList;
  struct Menu                  *Menu;
  struct Catalog               *Catalog;
  struct Attrnode               AttrNodes[ATTRNODES];
};


static const struct TextAttr Topaz80 = { "topaz.font", 8, 0, 0, };

#define MINSLIDERWIDTH 40

#define OKBUTTON      1
#define CANCELBUTTON  2
#define FREQSLIDER    3
#define LISTVIEW      4

#define FREQTEXT2     GetString(msgFreqFmt, req->Catalog) //"%lu Hz"
#define FREQLEN2      (5+3) // 5 digits + space + "Hz"

LONG IndexToFrequency( struct Gadget *gad, WORD level )
{
  LONG  freq = 0;
  ULONG id;

  id = ((struct AHIAudioModeRequesterExt *)gad->UserData)->tempAudioID;

  if(id != AHI_DEFAULT_ID)
  {
    AHI_GetAudioAttrs( id, NULL,
                       AHIDB_FrequencyArg, level,
                       AHIDB_Frequency,    &freq,
                       TAG_DONE );
  }
  else
  {
    freq = AHIBase->ahib_Frequency;
  }
  return freq;
}


static void FillReqStruct(struct AHIAudioModeRequesterExt *req, struct TagItem *tags)
{
  ULONG obsolete_userdata;

// Check all known tags
  req->SrcWindow=(struct Window *)GetTagData(AHIR_Window,(IPTR)req->SrcWindow,tags);
  req->PubScreenName=(STRPTR)GetTagData(AHIR_PubScreenName,(IPTR)req->PubScreenName,tags);
  req->Screen=(struct Screen *)GetTagData(AHIR_Screen,(IPTR)req->Screen,tags);
  req->IntuiMsgFunc=(struct Hook *)GetTagData(AHIR_IntuiMsgFunc,(IPTR)req->IntuiMsgFunc,tags);

  obsolete_userdata = GetTagData( AHIR_ObsoleteUserData, 0, tags );
  req->Req.ahiam_ObsoleteUserData[ 0 ] = obsolete_userdata >> 16;
  req->Req.ahiam_ObsoleteUserData[ 1 ] = obsolete_userdata & 0xffff;

  req->Req.ahiam_UserData=(void *)GetTagData(AHIR_UserData,(IPTR)req->Req.ahiam_UserData,tags);
  req->TextAttr=(struct TextAttr *)GetTagData(AHIR_TextAttr,(IPTR)req->TextAttr,tags);
  req->Locale=(struct Locale *)GetTagData(AHIR_Locale,(IPTR)req->Locale,tags);
  req->TitleText=(STRPTR)GetTagData(AHIR_TitleText,(IPTR)req->TitleText,tags);
  req->PositiveText=(STRPTR)GetTagData(AHIR_PositiveText,(IPTR)req->PositiveText,tags);
  req->NegativeText=(STRPTR)GetTagData(AHIR_NegativeText,(IPTR)req->NegativeText,tags);
  req->Req.ahiam_LeftEdge=GetTagData(AHIR_InitialLeftEdge,req->Req.ahiam_LeftEdge,tags);
  req->Req.ahiam_TopEdge=GetTagData(AHIR_InitialTopEdge,req->Req.ahiam_TopEdge,tags);
  req->Req.ahiam_Width=GetTagData(AHIR_InitialWidth,req->Req.ahiam_Width,tags);
  req->Req.ahiam_Height=GetTagData(AHIR_InitialHeight,req->Req.ahiam_Height,tags);
  req->Req.ahiam_AudioID=GetTagData(AHIR_InitialAudioID,req->Req.ahiam_AudioID,tags);
  req->Req.ahiam_MixFreq=GetTagData(AHIR_InitialMixFreq,req->Req.ahiam_MixFreq,tags);
  req->Req.ahiam_InfoOpened=GetTagData(AHIR_InitialInfoOpened,req->Req.ahiam_InfoOpened,tags);
  req->Req.ahiam_InfoLeftEdge=GetTagData(AHIR_InitialInfoLeftEdge,req->Req.ahiam_InfoLeftEdge,tags);
  req->Req.ahiam_InfoTopEdge=GetTagData(AHIR_InitialInfoTopEdge,req->Req.ahiam_InfoTopEdge,tags);
//  req->Req.ahiam_InfoWidth=GetTagData(AHIR_InitialInfoWidth,req->Req.ahiam_InfoWidth,tags);
//  req->Req.ahiam_InfoHeight=GetTagData(AHIR_InitialInfoHeight,req->Req.ahiam_InfoHeight,tags);
  req->FilterTags=(struct TagItem *)GetTagData(AHIR_FilterTags,(IPTR)req->FilterTags,tags);
  req->FilterFunc=(struct Hook *)GetTagData(AHIR_FilterFunc,(IPTR)req->FilterFunc,tags);
  req->Flags=PackBoolTags(req->Flags,tags,reqboolmap);
}


static void
CalculateWindowSizePos( struct AHIAudioModeRequesterExt* req,
			struct Screen* screen )
{
  struct AslSemaphore* asl_semaphore = NULL;

  // Default position and size
  WORD left   = 30;
  WORD top    = 20;
  WORD width  = 318;
  WORD height = 198;

#ifdef ASL_SEMAPHORE_NAME /* Don't break if no v45 includes found */
  Forbid();
  asl_semaphore = (struct AslSemaphore*) FindSemaphore( ASL_SEMAPHORE_NAME );

  if( asl_semaphore != NULL )
  {
    ObtainSemaphore( (struct SignalSemaphore*) asl_semaphore );
  }
  Permit();
 
  if( asl_semaphore != NULL )
  {
    if( asl_semaphore->as_Version >= 45 &&
	asl_semaphore->as_Size > offsetof( struct AslSemaphore,
					   as_RelativeHeight ) )
    {
      if( asl_semaphore->as_SizePosition & ASLOPTION_ASLOverrides )
      {
	// Force default settings
	req->Req.ahiam_LeftEdge = -1;
	req->Req.ahiam_TopEdge  = -1;
	req->Req.ahiam_Width    = -1;
	req->Req.ahiam_Height   = -1;
      }

      if( ( asl_semaphore->as_SizePosition & ASLSIZE_MASK ) ==
	  ASLSIZE_RelativeSize )
      {
	// FIXME: Is it correct to use screen size only? The include file
	// says something about parent window??
	  
	width  = (WORD) ( (LONG) screen->Width *
			  asl_semaphore->as_RelativeWidth / 100 );
	height = (WORD) ( (LONG) screen->Height *
			  asl_semaphore->as_RelativeHeight / 100 );
      }
    }
    else
    {
      // Not valid
      ReleaseSemaphore( (struct SignalSemaphore*) asl_semaphore );
      asl_semaphore = NULL;
    }
  }
#endif

  // Set default main window size
  if( req->Req.ahiam_Width == -1 )
    req->Req.ahiam_Width = width;

  if( req->Req.ahiam_Height == -1 )
    req->Req.ahiam_Height = height;


#ifdef ASL_SEMAPHORE_NAME /* Don't break if no v45 includes found */
  if( asl_semaphore != NULL )
  {
    switch( asl_semaphore->as_SizePosition & ASLPOS_MASK )
    {
      case ASLPOS_CenterWindow:
	if( req->SrcWindow != NULL )
	{
	  top  = ( req->SrcWindow->TopEdge +
		   req->SrcWindow->Height / 2 -
		   req->Req.ahiam_Height / 2 );
	  left = ( req->SrcWindow->LeftEdge +
		   req->SrcWindow->Width / 2 -
		   req->Req.ahiam_Width / 2 );
	  break;
	}
	else
	{
	  // Fall through and use ASLPOS_CenterScreen instead
	}

      case ASLPOS_CenterScreen:
	top  = ( screen->Height / 2 -
		 req->Req.ahiam_Height / 2 );
	left = ( screen->Width / 2 -
		 req->Req.ahiam_Width / 2 );
	break;
	
      case ASLPOS_WindowPosition:
	if( req->SrcWindow != NULL )
	{
	  top  = ( req->SrcWindow->TopEdge +
		   asl_semaphore->as_RelativeTop );
	  left = ( req->SrcWindow->LeftEdge +
		   asl_semaphore->as_RelativeLeft );
	  break;
	}
	else
	{
	  // Fall through and use ASLPOS_ScreenPosition instead
	}

      case ASLPOS_ScreenPosition:
	top  = asl_semaphore->as_RelativeTop;
	left = asl_semaphore->as_RelativeLeft;
	break;

      case ASLPOS_CenterMouse:
	top  = ( screen->MouseY -
		  req->Req.ahiam_Height / 2 );
	left = ( screen->MouseX -
		 req->Req.ahiam_Width / 2 );
	break;
	
      case ASLPOS_DefaultPosition:
      default:
	// Do nothing (use hardcoded defaults)
	break;
    }
  } 
#endif

  // Set default main window position
  if( req->Req.ahiam_LeftEdge == -1 )
    req->Req.ahiam_LeftEdge = left;

  if( req->Req.ahiam_TopEdge == -1 )
    req->Req.ahiam_TopEdge = top;

  
  // Set default info window position (size is fixed)
  if( req->Req.ahiam_InfoLeftEdge == -1 )
    req->Req.ahiam_InfoLeftEdge = req->Req.ahiam_LeftEdge + 16;
  
  if( req->Req.ahiam_InfoTopEdge == -1 )
    req->Req.ahiam_InfoTopEdge = req->Req.ahiam_TopEdge + 25;


  // Clean up
  if( asl_semaphore != NULL )
  {
    ReleaseSemaphore( (struct SignalSemaphore*) asl_semaphore );
  }
}

/*
** Returns the ordinal number of the current audio id.
*/

static LONG GetSelected(struct AHIAudioModeRequesterExt *req)
{
  struct IDnode *idnode;
  LONG valt=0;

  for(idnode=(struct IDnode *)req->list->mlh_Head;
      idnode->node.ln_Succ;
      idnode=(struct IDnode *) idnode->node.ln_Succ)
  {
    if(idnode->ID == req->tempAudioID)
      break;
    else
      valt++;
  }
  if(idnode->node.ln_Succ == NULL)
  {
    valt=~0;
    req->tempAudioID=AHI_INVALID_ID;    // Crashed if this is not done! FIXIT!
  }
  return valt;
}

/*
** Calculates what the current slider level shoud be and how many levels total
*/

static void GetSliderAttrs(struct AHIAudioModeRequesterExt *req, LONG *levels, LONG *level)
{
  *levels=0;
  *level=0;
  
  AHI_GetAudioAttrs(req->tempAudioID, NULL,
      AHIDB_Frequencies,  levels,
      AHIDB_IndexArg,     (req->tempAudioID == AHI_DEFAULT_ID ? 
                              AHIBase->ahib_Frequency : req->tempFrequency),
      AHIDB_Index,        level,
      TAG_DONE);

  if(*level >= *levels)
    *level = *levels-1;

  AHI_GetAudioAttrs(req->tempAudioID, NULL,
      AHIDB_FrequencyArg, *level,
      AHIDB_Frequency,    &req->tempFrequency,
      TAG_DONE);
}

/*
** Updates the requester to the current frequency and if 'all'==TRUE, audio mode.
*/

static void SetSelected(struct AHIAudioModeRequesterExt *req, BOOL all)
{
  LONG  sliderlevels,sliderlevel,selected;
  BOOL  disabled    = FALSE;
  ULONG top         = GTLV_Top;
  ULONG makevisible = GTLV_MakeVisible;

  if(all)
  {
    //Set listview
    selected=GetSelected(req);

    if( selected == ~0 || GadToolsBase->lib_Version >= 39 )
    {
      top = TAG_IGNORE;
    }

    if( selected == ~0 )
    {
      makevisible = TAG_IGNORE;
    }

    GT_SetGadgetAttrs(req->listviewgadget, req->Window, NULL,
        top,           selected,
        makevisible,   selected,
        GTLV_Selected, selected,
        TAG_DONE);
  }

  //Set slider
  GetSliderAttrs(req,&sliderlevels,&sliderlevel);

  if( sliderlevels == 0 || req->tempAudioID == AHI_DEFAULT_ID )
  {
    disabled = TRUE;
  }

  GT_SetGadgetAttrs(req->slidergadget, req->Window, NULL,
      GTSL_Max,    sliderlevels-1,
      GTSL_Level,  sliderlevel,
      GA_Disabled, disabled,
      TAG_DONE);

  UpdateInfoWindow(req);
}


/*
** Positions all gadgets in the requester.
*/

static BOOL LayOutReq (struct AHIAudioModeRequesterExt *req, const struct TextAttr *TextAttr)
{
  struct Gadget *gad;
  struct NewGadget ng;

  struct TextAttr *gadtextattr;
  struct TextFont *font;
  LONG   fontwidth,buttonheight,buttonwidth,pixels;
  struct IntuiText intuitext = {1,0,JAM1,0,0,NULL,NULL,NULL};
  LONG  sliderlevels,sliderlevel;
  LONG  selected;

  selected=GetSelected(req);
  GetSliderAttrs(req,&sliderlevels,&sliderlevel);

// Calculate gadget area
  req->gx=req->Window->BorderLeft+4;
  req->gy=req->Window->BorderTop+2;
  req->gw=req->Window->Width-req->gx-(req->Window->BorderRight+4);
  req->gh=req->Window->Height-req->gy-(req->Window->BorderBottom+2);

  if(req->Gadgets)
  {
    RemoveGList(req->Window,req->Gadgets,-1);
    FreeGadgets(req->Gadgets);
    SetAPen(req->Window->RPort,0);
    SetDrMd(req->Window->RPort,JAM1);
    EraseRect(req->Window->RPort, req->Window->BorderLeft, req->Window->BorderTop,
        req->Window->Width-req->Window->BorderRight-1,req->Window->Height-req->Window->BorderBottom-1);
    RefreshWindowFrame(req->Window);
  }
  req->Gadgets=NULL;
  if((gad=CreateContext(&req->Gadgets)))
  {
    if(TextAttr)
      gadtextattr=(struct TextAttr *)TextAttr;
    else
      gadtextattr=req->Window->WScreen->Font;

    if((font=OpenFont(gadtextattr)))
    {
      fontwidth=font->tf_XSize;
      CloseFont(font);
    }
    else
      return FALSE;

    buttonheight=gadtextattr->ta_YSize+6;
    intuitext.ITextFont=gadtextattr;
    intuitext.IText=req->PositiveText;
    buttonwidth=IntuiTextLength(&intuitext);
    intuitext.IText=req->NegativeText;
    pixels=IntuiTextLength(&intuitext);
    buttonwidth=max(pixels,buttonwidth);
    buttonwidth+=4+fontwidth;

// Create gadgets and check if they fit
    // Do the two buttons fit?
    if(2*buttonwidth > req->gw)
      return FALSE;
    ng.ng_TextAttr=gadtextattr;
    ng.ng_VisualInfo=req->vi;
    ng.ng_UserData=req;
// OK button
    ng.ng_LeftEdge=req->gx;
    ng.ng_TopEdge=req->gy+req->gh-buttonheight;
    ng.ng_Width=buttonwidth;
    ng.ng_Height=buttonheight;
    ng.ng_GadgetText=req->PositiveText;
    ng.ng_GadgetID=OKBUTTON;
    ng.ng_Flags=PLACETEXT_IN;
    gad=CreateGadget(BUTTON_KIND,gad,&ng,TAG_END);
// Cancel button
    ng.ng_LeftEdge=req->gx+req->gw-ng.ng_Width;
    ng.ng_GadgetText=req->NegativeText;
    ng.ng_GadgetID=CANCELBUTTON;
    gad=CreateGadget(BUTTON_KIND,gad,&ng,TAG_END);
// Frequency
    if(req->Flags & freqgad)
    {
      intuitext.IText = GetString(msgReqFrequency, req->Catalog);
      pixels=IntuiTextLength(&intuitext)+INTERWIDTH;
      if(pixels+MINSLIDERWIDTH+INTERWIDTH+FREQLEN2*fontwidth > req->gw)
        return FALSE;
      ng.ng_Width=req->gw-pixels-INTERWIDTH-FREQLEN2*fontwidth;
      ng.ng_LeftEdge=req->gx+pixels;
      ng.ng_TopEdge-=2+buttonheight;
      ng.ng_GadgetText = GetString(msgReqFrequency, req->Catalog);
      ng.ng_GadgetID=FREQSLIDER;
      ng.ng_Flags=PLACETEXT_LEFT;
      gad=CreateGadget(SLIDER_KIND,gad,&ng,
          GTSL_Min,0,
          GTSL_Max,sliderlevels-1,
          GTSL_Level,sliderlevel,
          GTSL_LevelFormat, FREQTEXT2,
          GTSL_MaxLevelLen,FREQLEN2,
          GTSL_LevelPlace,PLACETEXT_RIGHT,
          GTSL_DispFunc, m68k_IndexToFrequency,
          GA_RelVerify,TRUE,
          GA_Disabled,!sliderlevels || (req->tempAudioID == AHI_DEFAULT_ID),
          TAG_DONE);
      req->slidergadget=gad;   // Save for HadleReq()...
    }
// ListView
    if((ng.ng_Height=ng.ng_TopEdge-2-req->gy) < buttonheight)
      return FALSE;
    ng.ng_LeftEdge=req->gx;
    ng.ng_TopEdge=req->gy;
    ng.ng_Width=req->gw;
    ng.ng_GadgetText=NULL,
    ng.ng_GadgetID=LISTVIEW;
    ng.ng_Flags=PLACETEXT_ABOVE;
    gad=CreateGadget(LISTVIEW_KIND,gad,&ng,
        GTLV_ScrollWidth,(fontwidth>8 ? fontwidth*2 : 18),
        GTLV_Labels, req->list,
        GTLV_ShowSelected,0,
        ((selected == ~0) || (GadToolsBase->lib_Version >= 39) ? TAG_IGNORE : GTLV_Top),selected,
        (selected == ~0 ? TAG_IGNORE : GTLV_MakeVisible),selected,
        GTLV_Selected,selected,
        TAG_DONE);
    req->listviewgadget=gad;   // Save for HadleReq()...

    if(!gad)
      return FALSE;
  }
  else
    return FALSE;

  AddGList(req->Window,req->Gadgets,~0,-1,NULL);
  RefreshGList(req->Gadgets,req->Window,NULL,-1);
  GT_RefreshWindow(req->Window,NULL);

  return TRUE;
}



/* these functions close an Intuition window
 * that shares a port with other Intuition
 * windows or IPC customers.
 *
 * We are careful to set the UserPort to
 * null before closing, and to free
 * any messages that it might have been
 * sent.
 */

/* remove and reply all IntuiMessages on a port that
 * have been sent to a particular window
 * (note that we don't rely on the ln_Succ pointer
 *  of a message after we have replied it)
 */
static void StripIntuiMessagesAHI( struct MsgPort *mp, struct Window *win )
{
    struct IntuiMessage *msg;
    struct Node *succ;

    msg = (struct IntuiMessage *) mp->mp_MsgList.lh_Head;

    while((succ =  msg->ExecMessage.mn_Node.ln_Succ)) {

        if( msg->IDCMPWindow ==  win ) {

            /* Intuition is about to free this message.
             * Make sure that we have politely sent it back.
             */
            Remove( (struct Node *) msg );

            ReplyMsg( (struct Message *) msg );
        }
            
        msg = (struct IntuiMessage *) succ;
    }
}

static void CloseWindowSafely( struct Window *win )
{
    /* we forbid here to keep out of race conditions with Intuition */
    Forbid();

    /* send back any messages for this window 
     * that have not yet been processed
     */
    StripIntuiMessagesAHI( win->UserPort, win );

    /* clear UserPort so Intuition will not free it */
    win->UserPort = NULL;

    /* tell Intuition to stop sending more messages */
    ModifyIDCMP( win, 0L );

    /* turn multitasking back on */
    Permit();

    /* and really close the window */
    CloseWindow( win );
}

static BOOL HandleReq( struct AHIAudioModeRequesterExt *req )

// Returns FALSE if requester was cancelled

{
  BOOL done=FALSE,rc=TRUE;
  ULONG class,sec,oldsec=0,micro,oldmicro=0,oldid=AHI_INVALID_ID;
  UWORD code;
  UWORD qual;
  struct Gadget *pgsel;
  struct IntuiMessage *imsg;
  struct IDnode *idnode;
  LONG   sliderlevels,sliderlevel,i,selected;
  struct MenuItem *item;

  while(!done)
  {
    Wait(1L << req->Window->UserPort->mp_SigBit);

    while ((imsg=GT_GetIMsg(req->Window->UserPort)) != NULL )
    {

      if(imsg->IDCMPWindow == req->InfoWindow)
      {
        class = imsg->Class;
        GT_ReplyIMsg(imsg);

        switch(class)
        {
        case IDCMP_CLOSEWINDOW:
          CloseInfoWindow(req);
          break;
        case IDCMP_REFRESHWINDOW :
          GT_BeginRefresh(req->InfoWindow);
          GT_EndRefresh(req->InfoWindow,TRUE);
          break;
        }
        continue; // Get next IntuiMessage
      }

      else if(imsg->IDCMPWindow != req->Window) // Not my window!
      {
        if(req->IntuiMsgFunc)
          CallHookPkt(req->IntuiMsgFunc,req,imsg);
        // else what to do??? Reply and forget? FIXIT!
        continue;
      }

      sec=imsg->Seconds;
      micro=imsg->Micros;
      qual=imsg->Qualifier;
      class=imsg->Class;
      code=imsg->Code;
      pgsel=(struct Gadget *)imsg->IAddress; // pgsel illegal if not gadget
      GT_ReplyIMsg(imsg);

      switch ( class )
      {
      case IDCMP_RAWKEY:
        switch (code)
        {
        case 0x4c: // Cursor Up
          selected=GetSelected(req);
          if(selected == ~0)
            selected=0;
          if(selected > 0)
            selected--;
          idnode=(struct IDnode *)req->list->mlh_Head;
          for(i=0;i<selected;i++)
            idnode=(struct IDnode *)idnode->node.ln_Succ;
          req->tempAudioID=idnode->ID;
          SetSelected(req,TRUE);
          break;
        case 0x4d: // Cursor Down
          selected=GetSelected(req);
          selected++; // ~0 => 0
          idnode=(struct IDnode *)req->list->mlh_Head;
          for(i=0;i<selected;i++)
            if(idnode->node.ln_Succ->ln_Succ)
              idnode=(struct IDnode *)idnode->node.ln_Succ;
          req->tempAudioID=idnode->ID;
          SetSelected(req,TRUE);
          break;
        case 0x4e: // Cursor Right
          GetSliderAttrs(req,&sliderlevels,&sliderlevel);
          sliderlevel += (qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) ? 10 :1);
          if(sliderlevel >= sliderlevels)
            sliderlevel=sliderlevels-1;
          AHI_GetAudioAttrs(req->tempAudioID, NULL,
              AHIDB_FrequencyArg,sliderlevel,
              AHIDB_Frequency, &req->tempFrequency,
              TAG_DONE);
          SetSelected(req,FALSE);
          break;
        case 0x4f: // Cursor Left
          GetSliderAttrs(req,&sliderlevels,&sliderlevel);
          sliderlevel -= (qual & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) ? 10 :1);
          if(sliderlevel < 0)
            sliderlevel=0;
          AHI_GetAudioAttrs(req->tempAudioID, NULL,
              AHIDB_FrequencyArg,sliderlevel,
              AHIDB_Frequency, &req->tempFrequency,
              TAG_DONE);
          SetSelected(req,FALSE);
          break;
        }
        break;
      case IDCMP_GADGETUP :
        switch ( pgsel->GadgetID )
        {
        case OKBUTTON:
          done=TRUE;
          break;
        case CANCELBUTTON:
          done=TRUE;
          rc=FALSE;
          break;
        case FREQSLIDER:
          AHI_GetAudioAttrs(req->tempAudioID, NULL,
              AHIDB_FrequencyArg,code,
              AHIDB_Frequency, &req->tempFrequency,
              TAG_DONE);
          break;
        case LISTVIEW:
          idnode=(struct IDnode *)req->list->mlh_Head;
          for(i=0;i<code;i++)
            idnode=(struct IDnode *)idnode->node.ln_Succ;
          req->tempAudioID=idnode->ID;
          SetSelected(req,FALSE);
          // Test doubleclick and save timestamp
          if( (oldid == req->tempAudioID) && DoubleClick(oldsec,oldmicro,sec,micro))
            done=TRUE;
          oldsec=sec;
          oldmicro=micro;
          oldid=req->tempAudioID;

          break;
        }
        break;

      case IDCMP_NEWSIZE:
        if(!(LayOutReq(req,req->TextAttr)))
          if(!(LayOutReq(req,&Topaz80)))
          {
            // ERROR! Quit.
            done=TRUE;
            break;
          }
        break;
      case IDCMP_CLOSEWINDOW:
        done=TRUE;
        rc=FALSE;
        break;
      case IDCMP_REFRESHWINDOW :
        GT_BeginRefresh(req->Window);
        GT_EndRefresh(req->Window,TRUE);
        break;
      case IDCMP_SIZEVERIFY:
        break;
      case IDCMP_MENUPICK:
        while((code != MENUNULL) && !done)
        {
          item=ItemAddress(req->Menu, code);
          switch((IPTR)GTMENUITEM_USERDATA(item))
          {
          case LASTMODEITEM:
            selected=GetSelected(req);
            if(selected == ~0)
              selected=0;
            if(selected > 0)
              selected--;
            idnode=(struct IDnode *)req->list->mlh_Head;
            for(i=0;i<selected;i++)
              idnode=(struct IDnode *)idnode->node.ln_Succ;
            req->tempAudioID=idnode->ID;
            SetSelected(req,TRUE);
            break;
          case NEXTMODEITEM:
            selected=GetSelected(req);
            selected++; // ~0 => 0
            idnode=(struct IDnode *)req->list->mlh_Head;
            for(i=0;i<selected;i++)
              if(idnode->node.ln_Succ->ln_Succ)
                idnode=(struct IDnode *)idnode->node.ln_Succ;
            req->tempAudioID=idnode->ID;
            SetSelected(req,TRUE);
            break;
          case PROPERTYITEM:
            OpenInfoWindow(req);
            break;
          case RESTOREITEM:
            req->tempAudioID=req->Req.ahiam_AudioID;
            req->tempFrequency=req->Req.ahiam_MixFreq;
            SetSelected(req,TRUE);
            break;
          case OKITEM:
            done=TRUE;
            break;
          case CANCELITEM:
            done=TRUE;
            rc=FALSE;
            break;
          }
          code = item->NextSelect;
        }
        break;
      }
    }
  }

  if(rc)
  {
    req->Req.ahiam_AudioID = req->tempAudioID;

    if(req->tempAudioID != AHI_DEFAULT_ID)
    {
      req->Req.ahiam_MixFreq = req->tempFrequency;
    }
    else
    {
      req->Req.ahiam_MixFreq = AHI_DEFAULT_FREQ;
    }
  }
  return rc;
}


static void OpenInfoWindow( struct AHIAudioModeRequesterExt *req )
{
  struct Gadget *gad;
  struct NewGadget ng;


  if(req->InfoWindow == NULL)
  {
    req->InfoWindow=OpenWindowTags(NULL,
      WA_Left,              req->Req.ahiam_InfoLeftEdge,
      WA_Top,               req->Req.ahiam_InfoTopEdge,
      WA_Width,             req->Req.ahiam_InfoWidth,
      WA_Height,            req->Req.ahiam_InfoHeight,
      WA_Title,             GetString(msgReqInfoTitle, req->Catalog),
      WA_CustomScreen,      req->Window->WScreen,
      WA_PubScreenFallBack, TRUE,
      WA_DragBar,           TRUE,
      WA_DepthGadget,       TRUE,
      WA_CloseGadget,       TRUE,
      WA_Activate,          FALSE,
      WA_SimpleRefresh,     TRUE,
      WA_AutoAdjust,        TRUE,
      WA_IDCMP,             0,
      WA_NewLookMenus,      TRUE,
      TAG_DONE);

    if(req->InfoWindow)
    {
      req->InfoWindow->UserPort = req->Window->UserPort;
      ModifyIDCMP(req->InfoWindow, MY_INFOIDCMPS);

      if((gad = CreateContext(&req->InfoGadgets)))
      {
        ng.ng_TextAttr    = req->TextAttr;
        ng.ng_VisualInfo  = req->vi;
        ng.ng_LeftEdge    = req->InfoWindow->BorderLeft+4;
        ng.ng_TopEdge     = req->InfoWindow->BorderTop+2;
        ng.ng_Width       = req->InfoWindow->Width
                          - (req->InfoWindow->BorderLeft+4)
                          - (req->InfoWindow->BorderRight+4);
        ng.ng_Height      = req->InfoWindow->Height
                          - (req->InfoWindow->BorderTop+2)
                          - (req->InfoWindow->BorderBottom+2);
    
        ng.ng_GadgetText  = NULL;
        ng.ng_GadgetID    = 0;
        ng.ng_Flags       = PLACETEXT_ABOVE;
        gad = CreateGadget(LISTVIEW_KIND, gad, &ng,
            GTLV_ReadOnly,  TRUE,
            TAG_DONE);
        req->InfoListViewGadget = gad;

        if(gad)
        {
          AddGList(req->InfoWindow, req->InfoGadgets, ~0, -1, NULL);
          RefreshGList(req->InfoGadgets, req->InfoWindow, NULL, -1);
          GT_RefreshWindow(req->InfoWindow, NULL);
          UpdateInfoWindow(req);
        }
      }
    }
  }
}


static void UpdateInfoWindow( struct AHIAudioModeRequesterExt *req )
{
  LONG id=0, bits=0, stereo=0, pan=0, hifi=0, channels=0, minmix=0, maxmix=0,
       record=0, fullduplex=0, multichannel=0;
  int i;

  id = req->tempAudioID;
  if(id == AHI_DEFAULT_ID)
  {
    id = AHIBase->ahib_AudioMode;
  }
  if(req->InfoWindow)
  {
    AHI_GetAudioAttrs(id, NULL,
      AHIDB_MultiChannel, &multichannel,
      AHIDB_Stereo,       &stereo,
      AHIDB_Panning,      &pan,
      AHIDB_HiFi,         &hifi,
      AHIDB_Record,       &record,
      AHIDB_FullDuplex,   &fullduplex,
      AHIDB_Bits,         &bits,
      AHIDB_MaxChannels,  &channels,
      AHIDB_MinMixFreq,   &minmix,
      AHIDB_MaxMixFreq,   &maxmix,
      TAG_DONE);

    GT_SetGadgetAttrs(req->InfoListViewGadget, req->InfoWindow, NULL,
        GTLV_Labels, ~0,
        TAG_DONE);

    NewList((struct List *) &req->InfoList);
    for(i=0; i<ATTRNODES; i++)
    {
      req->AttrNodes[i].node.ln_Name = req->AttrNodes[i].text;
      req->AttrNodes[i].text[0]      = '\0';
      req->AttrNodes[i].node.ln_Type = NT_USER;
      req->AttrNodes[i].node.ln_Pri  = 0;
    }

    i = 0;
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoAudioID, req->Catalog),
        id);
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoResolution, req->Catalog),
        bits, GetString((multichannel ? msgReqInfoMultiChannel : (stereo ?
          (pan ? msgReqInfoStereoPan : msgReqInfoStereo) :
          msgReqInfoMono)), req->Catalog));
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoChannels, req->Catalog),
        channels);
    AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
    Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoMixrate, req->Catalog),
        minmix, maxmix);
    if(hifi)
    {
      AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
      Sprintf(req->AttrNodes[i++].text, GetString(msgReqInfoHiFi, req->Catalog));
    }
    if(record)
    {
      AddTail((struct List *) &req->InfoList,(struct Node *) &req->AttrNodes[i]);
      Sprintf(req->AttrNodes[i++].text, GetString(
          fullduplex ? msgReqInfoRecordFull : msgReqInfoRecordHalf, req->Catalog));
    }

    GT_SetGadgetAttrs(req->InfoListViewGadget, req->InfoWindow, NULL,
        GTLV_Labels, &req->InfoList,
        TAG_DONE);
  }
}

static void CloseInfoWindow( struct AHIAudioModeRequesterExt *req )
{
  if(req->InfoWindow)
  {
    req->Req.ahiam_InfoOpened   = TRUE;
    req->Req.ahiam_InfoLeftEdge = req->InfoWindow->LeftEdge;
    req->Req.ahiam_InfoTopEdge  = req->InfoWindow->TopEdge;
    req->Req.ahiam_InfoWidth    = req->InfoWindow->Width;
    req->Req.ahiam_InfoHeight   = req->InfoWindow->Height;
    CloseWindowSafely(req->InfoWindow);
    req->InfoWindow = NULL;

  }
  else
  {
    req->Req.ahiam_InfoOpened   = FALSE;

  }

  FreeGadgets(req->InfoGadgets);
  req->InfoGadgets = NULL;
}


/******************************************************************************
** AHI_AllocAudioRequestA *****************************************************
******************************************************************************/

/****** ahi.device/AHI_AllocAudioRequestA ***********************************
*
*   NAME
*       AHI_AllocAudioRequestA -- allocate an audio mode requester.
*       AHI_AllocAudioRequest -- varargs stub for AHI_AllocAudioRequestA()
*
*   SYNOPSIS
*       requester = AHI_AllocAudioRequestA( tags );
*       D0                                  A0
*
*       struct AHIAudioModeRequester *AHI_AllocAudioRequestA(
*           struct TagItem * );
*
*       requester = AHI_AllocAudioRequest( tag1, ... );
*
*       struct AHIAudioModeRequester *AHI_AllocAudioRequest( Tag, ... );
*
*   FUNCTION
*       Allocates an audio mode requester data structure.
*
*   INPUTS
*       tags - A pointer to an optional tag list specifying how to initialize
*           the data structure returned by this function. See the
*           documentation for AHI_AudioRequestA() for an explanation of how
*           to use the currently defined tags.
*
*   RESULT
*       requester - An initialized requester data structure, or NULL on
*           failure. 
*
*   EXAMPLE
*
*   NOTES
*       The requester data structure is READ-ONLY and can only be modified
*       by using tags!
*
*   BUGS
*
*   SEE ALSO
*      AHI_AudioRequestA(), AHI_FreeAudioRequest()
*
****************************************************************************
*
*/

struct AHIAudioModeRequester*
_AHI_AllocAudioRequestA( struct TagItem* tags,
			 struct AHIBase* AHIBase )
{
  struct AHIAudioModeRequesterExt *req;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_AllocAudioRequestA(tags);
  }

  if((req=AllocVec(sizeof(struct AHIAudioModeRequesterExt),MEMF_CLEAR)))
  {
// Fill in defaults
    req->Req.ahiam_LeftEdge   = -1;
    req->Req.ahiam_TopEdge    = -1;
    req->Req.ahiam_Width      = -1;
    req->Req.ahiam_Height     = -1;
    
    req->Req.ahiam_AudioID    = AHI_INVALID_ID;
    req->Req.ahiam_MixFreq    = AHIBase->ahib_Frequency;

    req->Req.ahiam_InfoLeftEdge = -1;
    req->Req.ahiam_InfoTopEdge  = -1;
    req->Req.ahiam_InfoWidth    = 280;
    req->Req.ahiam_InfoHeight   = 112;

    req->PubScreenName        = (STRPTR) -1;

    FillReqStruct(req,tags);
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>0x%P\n", req);
  }

  return (struct AHIAudioModeRequester *) req;
}


/******************************************************************************
** AHI_AudioRequestA **********************************************************
******************************************************************************/

/****** ahi.device/AHI_AudioRequestA ****************************************
*
*   NAME
*       AHI_AudioRequestA -- get an audio mode from user using an requester.
*       AHI_AudioRequest -- varargs stub for AHI_AudioRequestA()
*
*   SYNOPSIS
*       success = AHI_AudioRequestA( requester, tags );
*       D0                           A0         A1
*
*       BOOL AHI_AudioRequestA( struct AHIAudioModeRequester *,
*           struct TagItem * );
*
*       result = AHI_AudioRequest( requester, tag1, ... );
*
*       BOOL AHI_AudioRequest( struct AHIAudioModeRequester *, Tag, ... );
*
*   FUNCTION
*       Prompts the user for an audio mode, based on the modifying tags.
*       If the user cancels or the system aborts the request, FALSE is
*       returned, otherwise the requester's data structure reflects the
*       user input.
*
*       Note that tag values stay in effect for each use of the requester
*       until they are cleared or modified by passing the same tag with a
*       new value.
*
*   INPUTS
*       requester - Requester structure allocated with
*           AHI_AllocAudioRequestA(). If this parameter is NULL, this
*           function will always return FALSE with a dos.library/IoErr()
*           result of ERROR_NO_FREE_STORE.
*       tags - Pointer to an optional tag list which may be used to control
*           features of the requester.
*
*   TAGS
*       Tags used for the requester (they look remarkable similar to the
*       screen mode requester in ASL, don't they? ;-) )
*
*       AHIR_Window (struct Window *) - Parent window of requester. If no
*           AHIR_Screen tag is specified, the window structure is used to
*           determine on which screen to open the requesting window.
*
*       AHIR_PubScreenName (STRPTR) - Name of a public screen to open on.
*           This overrides the screen used by AHIR_Window.
*
*       AHIR_Screen (struct Screen *) - Screen on which to open the
*           requester. This overrides the screen used by AHIR_Window or by
*           AHIR_PubScreenName.
*
*       AHIR_PrivateIDCMP (BOOL) - When set to TRUE, this tells AHI to
*           allocate a new IDCMP port for the requesting window. If not
*           specified or set to FALSE, and if AHIR_Window is provided, the
*           requesting window will share AHIR_Window's IDCMP port.
*
*       AHIR_IntuiMsgFunc (struct Hook *) - A function to call whenever an
*           unknown Intuition message arrives at the message port being used
*           by the requesting window. The function receives the following
*           parameters:
*               A0 - (struct Hook *)
*               A1 - (struct IntuiMessage *)
*               A2 - (struct AHIAudioModeRequester *)
*
*       AHIR_SleepWindow (BOOL) - When set to TRUE, this tag will cause the
*           window specified by AHIR_Window to be "put to sleep". That is, a
*           busy pointer will be displayed in the parent window, and no
*           gadget or menu activity will be allowed. This is done by opening
*           an invisible Intuition Requester in the parent window.
*
*       AHIR_UserData (APTR) - A 32-bit value that is simply copied in the
*           ahiam_UserData field of the requester structure.
*
*       AHIR_TextAttr (struct TextAttr *) - Font to be used for the
*           requesting window's gadgets and menus. If this tag is not
*           provided or its value is NULL, the default font of the screen
*           on which the requesting window opens will be used. This font
*           must already be in memory as AHI calls OpenFont() and not
*           OpenDiskFont().
*
*       AHIR_Locale (struct Locale *) - Locale to use for the requesting
*           window. This determines the language used for the requester's
*           gadgets and menus. If this tag is not provided or its value is
*           NULL, the system's current default locale will be used.
*
*       AHIR_TitleText (STRPTR) - Title to use for the requesting window.
*           Default is no title.
*
*       AHIR_PositiveText (STRPTR) - Label of the positive gadget in the
*           requester. English default is "OK".
*
*       AHIR_NegativeText (STRPTR) - Label of the negative gadget in the
*           requester. English default is "Cancel".
*
*       AHIR_InitialLeftEdge (WORD) - Suggested left edge of requesting
*           window.
*
*       AHIR_InitialTopEdge (WORD) - Suggested top edge of requesting
*           window.
*
*       AHIR_InitialWidth (WORD) - Suggested width of requesting window.
*
*       AHIR_InitialHeight (WORD) - Suggested height of requesting window.
*
*       AHIR_InitialAudioID (ULONG) - Initial setting of the Mode list view
*           gadget (ahiam_AudioID). Default is ~0 (AHI_INVALID_ID), which
*           means that no mode will be selected.
*
*       AHIR_InitialMixFreq (ULONG) - Initial setting of the frequency
*           slider. Default is the lowest frequency supported.
*
*       AHIR_InitialInfoOpened (BOOL) - Whether to open the property
*           information window automatically. Default is FALSE.
*
*       AHIR_InitialInfoLeftEdge (WORD) - Initial left edge of information
*           window.
*
*       AHIR_InitialInfoTopEdge (WORD) - Initial top edge of information
*           window.
*
*       AHIR_DoMixFreq (BOOL) - Set this tag to TRUE to cause the requester
*           to display the frequency slider gadget. Default is FALSE.
*
*       AHIR_DoDefaultMode (BOOL) - Set this tag to TRUE to let the user
*           select the mode she has set in the preferences program. If she
*           selects this mode,  ahiam_AudioID will be AHI_DEFAULT_ID and
*           ahiam_MixFreq will be AHI_DEFAULT_FREQ. Note that if you filter
*           the mode list (see below), you must also check the mode (with
*           AHI_BestAudioIDA()) before you use it since the user may change 
*           the meaning of AHI_DEFAULT_MODE anytime, without your knowledge.
*           Default is FALSE. (V4)
*
*       AHIR_FilterFunc (struct Hook *) - A function to call for each mode
*           encountered. If the function returns TRUE, the mode is included
*           in the file list, otherwise it is rejected and not displayed. The
*           function receives the following parameters:
*               A0 - (struct Hook *)
*               A1 - (ULONG) mode id
*               A2 - (struct AHIAudioModeRequester *)
*
*       AHIR_FilterTags (struct TagItem *) - A pointer to a tag list used to
*           filter modes away, like AHIR_FilterFunc does. The tags are the
*           same as AHI_BestAudioIDA() takes as arguments. See that function
*           for an explanation of each tag.
*
*   RESULT
*       result - FALSE if the user cancelled the requester or if something
*           prevented the requester from opening. If TRUE, values in the
*           requester structure will be set.
*
*           If the return value is FALSE, you can look at the result from the
*           dos.library/IoErr() function to determine whether the requester
*           was cancelled or simply failed to open. If dos.library/IoErr()
*           returns 0, then the requester was cancelled, any other value
*           indicates a failure to open. Current possible failure codes are
*           ERROR_NO_FREE_STORE which indicates there was not enough memory,
*           and ERROR_NO_MORE_ENTRIES which indicates no modes were available
*           (usually because the application filter hook filtered them all
*           away).
*
*   EXAMPLE
*
*   NOTES
*       The requester data structure is READ-ONLY and can only be modified
*       by using tags!
*
*       The mixing/recording frequencies that are presented to the user
*       may not be the only ones a driver supports, but just a selection.
*
*   BUGS
*
*   SEE ALSO
*      AHI_AllocAudioRequestA(), AHI_FreeAudioRequest()
*
****************************************************************************
*
*/

ULONG
_AHI_AudioRequestA( struct AHIAudioModeRequester* req_in,
		    struct TagItem*               tags,
		    struct AHIBase*               AHIBase )
{
    struct AHIAudioModeRequesterExt *req=(struct AHIAudioModeRequesterExt *)req_in;
    struct MinList list;
    struct IDnode *node = NULL, *node2 = NULL;
    struct Screen *pub_screen = NULL;
    struct Screen *screen     = NULL;
    ULONG id=AHI_INVALID_ID;
    BOOL  rc=TRUE;
    struct Requester lockreq;
    BOOL  locksuxs = FALSE;
    WORD zipcoords[4];

    if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
    {
	Debug_AudioRequestA(req_in,tags);
    }

    SetIoErr( 0 );

    if(!req)
    {
	SetIoErr( ERROR_REQUIRED_ARG_MISSING );
	return FALSE;
    }

    // Update requester structure
    FillReqStruct(req,tags);
    req->tempAudioID=req->Req.ahiam_AudioID;
    req->tempFrequency=req->Req.ahiam_MixFreq;

    // Open the catalog

    req->Catalog = ExtOpenCatalog(req->Locale, NULL);

    if(req->PositiveText == NULL)
	req->PositiveText = GetString(msgReqOK, req->Catalog);
    if(req->NegativeText == NULL)
	req->NegativeText = GetString(msgReqCancel, req->Catalog);


    // Scan audio database for modes and create list
    req->list=&list;
    NewList((struct List *)req->list);
    while(AHI_INVALID_ID != (id=AHI_NextAudioID(id)))
    {
	// Check FilterTags
	if(req->FilterTags)
	    if(!TestAudioID(id,req->FilterTags))
		continue;
	if(req->FilterFunc)
	    if(!CallHookPkt(req->FilterFunc,req,(APTR)id))
		continue;
	// Add mode to list
	if((node=AllocVec(sizeof(struct IDnode),MEMF_ANY)))
	{
	    node->node.ln_Type=NT_USER;
	    node->node.ln_Pri=0;
	    node->node.ln_Name=node->name;
	    node->ID=id;
#ifndef __AMIGAOS4__
	    Sprintf(node->node.ln_Name, GetString(msgUnknown, req->Catalog),id);
#endif
	    AHI_GetAudioAttrs(id, NULL,
		    AHIDB_BufferLen,80,
		    AHIDB_Name, node->node.ln_Name,
		    TAG_DONE);
	    // Insert node alphabetically
	    for(node2=(struct IDnode *)req->list->mlh_Head;node2->node.ln_Succ;node2=(struct IDnode *) node2->node.ln_Succ)
		if(Stricmp(node->node.ln_Name,node2->node.ln_Name) < 0)
		    break;
	    Insert((struct List *) req->list,(struct Node *)node,node2->node.ln_Pred);
	}
    }

    // Add the users preferred mode

    if((req->Flags & defaultmode) && (AHIBase->ahib_AudioMode != AHI_INVALID_ID)) do
    {
	if(req->FilterTags)
	    if(!TestAudioID(AHIBase->ahib_AudioMode,req->FilterTags))
		continue;
	if(req->FilterFunc)
	    if(!CallHookPkt(req->FilterFunc,req,(APTR)AHIBase->ahib_AudioMode))
		continue;

	if((node=AllocVec(sizeof(struct IDnode),MEMF_ANY)))
	{
	    node->node.ln_Type=NT_USER;
	    node->node.ln_Pri=0;
	    node->node.ln_Name=node->name;
	    node->ID = AHI_DEFAULT_ID;
	    Sprintf(node->node.ln_Name, GetString(msgDefaultMode, req->Catalog));
	    AddTail((struct List *) req->list, (struct Node *)node);
	}
    } while(FALSE);

    if(NULL == ((struct IDnode *)req->list->mlh_Head)->node.ln_Succ)
    {
	// List is empty, no audio modes!
	// Return immediately (no nodes to free)
	SetIoErr(ERROR_NO_MORE_ENTRIES);
	ExtCloseCatalog(req->Catalog);
	req->Catalog = FALSE;
	return FALSE;
    }

    // Find our screen
    if(req->Screen)
    {
	pub_screen = NULL;
	screen     = req->Screen;
    }
    else if(req->PubScreenName != (STRPTR) -1)
    {
	pub_screen = LockPubScreen( req->PubScreenName );
	screen     = pub_screen;
    }
    else if(req->SrcWindow)
    {
	pub_screen = NULL;
	screen     = req->SrcWindow->WScreen;
    }

    if( screen == NULL )
    {
	pub_screen = LockPubScreen( NULL );
	screen     = pub_screen;
    }

    CalculateWindowSizePos( req, screen );

    // Clear ownIDCMP flag
    req->Flags &= ~ownIDCMP;

    if( req->SrcWindow != NULL )
    {
	if(req->Flags & haveIDCMP)
	    req->Flags |= ownIDCMP;
    }

    zipcoords[0]=req->Req.ahiam_LeftEdge;
    zipcoords[1]=req->Req.ahiam_TopEdge;
    zipcoords[2]=1;
    zipcoords[3]=1;

    req->Window=OpenWindowTags(
	    NULL,
	    WA_Left,req->Req.ahiam_LeftEdge,
	    WA_Top,req->Req.ahiam_TopEdge,
	    WA_Width,req->Req.ahiam_Width,
	    WA_Height,req->Req.ahiam_Height,
	    WA_Zoom, zipcoords,
	    WA_MaxWidth,~0,
	    WA_MaxHeight,~0,
	    WA_Title, req->TitleText,
	    ( pub_screen != NULL ? WA_PubScreen : WA_CustomScreen ), screen,
	    WA_PubScreenFallBack,TRUE,
	    WA_SizeGadget,TRUE,
	    WA_SizeBBottom,TRUE,
	    WA_DragBar,TRUE,
	    WA_DepthGadget,TRUE,
	    WA_CloseGadget,TRUE,
	    WA_Activate,TRUE,
	    WA_SimpleRefresh,TRUE,
	    WA_AutoAdjust,TRUE,
	    WA_IDCMP,(req->Flags & ownIDCMP ? 0 : MY_IDCMPS),
	    WA_NewLookMenus, TRUE,
	    TAG_DONE);

    if( pub_screen != NULL )
    {
	UnlockPubScreen( NULL, pub_screen);
    }

    if(req->Window)
    {
	// Topaz80: "Frequency"+INTERWIDTH+MINSLIDERWIDTH+INTERWIDTH+"99999 Hz" gives...
	WORD min_width = (req->Window->BorderLeft+4)+
	    strlen( GetString(msgReqFrequency, req->Catalog))*8+
	    INTERWIDTH+MINSLIDERWIDTH+INTERWIDTH+
	    FREQLEN2*8+
	    (req->Window->BorderRight+4);

	// Topaz80: 5 lines, freq & buttons gives...
	WORD min_height = (req->Window->BorderTop+2)+
	    (5*8+6)+2+(8+6)+2+(8+6)+
	    (req->Window->BorderBottom+2);

	if( req->Window->Width < min_width ||
		req->Window->Height < min_height )
	{
	    ChangeWindowBox( req->Window,
		    req->Window->LeftEdge,
		    req->Window->TopEdge,
		    max( req->Window->Width, min_width ),
		    max( req->Window->Height, min_height ) );
	    Delay( 5 );
	}

	WindowLimits( req->Window, min_width, min_height, 0, 0 );

	if((req->vi=GetVisualInfoA(req->Window->WScreen, NULL)))
	{
	    if(!(LayOutReq(req,req->TextAttr)))
		if(!(LayOutReq(req,&Topaz80)))
		    rc=FALSE;

	    if(rc) // Layout OK?
	    {
		struct NewMenu reqnewmenu[] =
		{
		    { NM_TITLE, NULL        , 0 ,0,0,(APTR) 0,            },
		    {  NM_ITEM, NULL        , 0 ,0,0,(APTR) LASTMODEITEM, },
		    {  NM_ITEM, NULL        , 0 ,0,0,(APTR) NEXTMODEITEM, },
		    {  NM_ITEM, NM_BARLABEL , 0 ,0,0,(APTR) 0,            },
		    {  NM_ITEM, NULL        , 0 ,0,0,(APTR) PROPERTYITEM, },
		    {  NM_ITEM, NULL        , 0 ,0,0,(APTR) RESTOREITEM , },
		    {  NM_ITEM, NM_BARLABEL , 0 ,0,0,(APTR) 0,            },
		    {  NM_ITEM, NULL        , 0 ,0,0,(APTR) OKITEM,       },
		    {  NM_ITEM, NULL        , 0 ,0,0,(APTR) CANCELITEM,   },
		    {   NM_END, NULL        , 0 ,0,0,(APTR) 0,            },
		};
		static const APTR strings[] =
		{
		    msgMenuControl,
		    msgMenuLastMode,
		    msgMenuNextMode,
		    msgMenuPropertyList,
		    msgMenuRestore,
		    msgMenuOK,
		    msgMenuCancel,
		};

		struct NewMenu *menuptr;
		APTR           *stringptr;

		menuptr   = (struct NewMenu *) &reqnewmenu;
		stringptr = (APTR *) &strings;

		while(menuptr->nm_Type != NM_END)
		{
		    if(menuptr->nm_Label == NULL)
		    {
			menuptr->nm_CommKey = GetString(*stringptr, req->Catalog);
			menuptr->nm_Label = menuptr->nm_CommKey + 2;
			stringptr++;
		    }
		    menuptr++;
		}

		if(req->Flags & ownIDCMP)
		{
		    req->Window->UserPort=req->SrcWindow->UserPort;
		    ModifyIDCMP(req->Window,MY_IDCMPS);
		}

		if((req->Flags & lockwin) && req->SrcWindow)
		{
		    InitRequester(&lockreq);
		    locksuxs=Request(&lockreq,req->SrcWindow);
		    if(IntuitionBase->LibNode.lib_Version >= 39)
			SetWindowPointer(req->SrcWindow,
				WA_BusyPointer,TRUE,
				TAG_DONE);
		}

		// Add menus
		if((req->Menu=CreateMenus(reqnewmenu, 
				GTMN_FullMenu, TRUE,
				GTMN_NewLookMenus, TRUE,
				TAG_DONE )))
		{
		    if(LayoutMenus(req->Menu,req->vi, TAG_DONE))
		    {
			if(SetMenuStrip(req->Window, req->Menu))
			{
			    if(req->Req.ahiam_InfoOpened)
			    {
				OpenInfoWindow(req);
			    }

			    rc=HandleReq(req);

			    CloseInfoWindow(req);
			    ClearMenuStrip(req->Window);
			}
		    } // else LayoutMenus failed
		    FreeMenus(req->Menu);
		    req->Menu=NULL;
		} // else CreateMenus failed


		if((req->Flags & lockwin) && req->SrcWindow)
		{
		    if(locksuxs)
			EndRequest(&lockreq,req->SrcWindow);
		    if(IntuitionBase->LibNode.lib_Version >= 39)
			SetWindowPointer(req->SrcWindow,
				WA_BusyPointer,FALSE,
				TAG_DONE);
		}

		req->Req.ahiam_LeftEdge = req->Window->LeftEdge;
		req->Req.ahiam_TopEdge  = req->Window->TopEdge;
		req->Req.ahiam_Width    = req->Window->Width;
		req->Req.ahiam_Height   = req->Window->Height;
	    } // else LayOutReq failed
	}
	else // no vi
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	    rc=FALSE;
	}

	if(req->Flags & ownIDCMP)
	    CloseWindowSafely(req->Window);
	else
	    CloseWindow(req->Window);
	req->Window=NULL;
	FreeVisualInfo(req->vi);
	req->vi=NULL;
	FreeGadgets(req->Gadgets);
	req->Gadgets=NULL;
	FreeVec(node);
    }
    else // no window
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	rc=FALSE;
    }

    ExtCloseCatalog(req->Catalog);
    req->Catalog = NULL;
    req->PositiveText = req->NegativeText = NULL;

    if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
    {
	KPrintF("=>%s\n",rc ? "TRUE" : "FALSE" );
    }
    return (ULONG) rc;
}


/******************************************************************************
** AHI_FreeAudioRequest *******************************************************
******************************************************************************/

/****** ahi.device/AHI_FreeAudioRequest *************************************
*
*   NAME
*       AHI_FreeAudioRequest -- frees requester resources 
*
*   SYNOPSIS
*       AHI_FreeAudioRequest( requester );
*                             A0
*
*       void AHI_FreeAudioRequest( struct AHIAudioModeRequester * );
*
*   FUNCTION
*       Frees any resources allocated by AHI_AllocAudioRequestA(). Once a
*       requester has been freed, it can no longer be used with other calls to
*       AHI_AudioRequestA().
*
*   INPUTS
*       requester - Requester obtained from AHI_AllocAudioRequestA(), or NULL
*       in which case this function does nothing.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_AllocAudioRequestA()
*
****************************************************************************
*
*/

void
_AHI_FreeAudioRequest( struct AHIAudioModeRequester* req,
		       struct AHIBase*               AHIBase )
{

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    Debug_FreeAudioRequest(req);
  }

  if(req)
    FreeVec(req);
}
