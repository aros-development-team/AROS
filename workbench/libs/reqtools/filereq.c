/**f************************************************************
*                                                             *
*      File/Font/Screenmode requester                         *
*                                                             *
*                                 (c) Nico François 1991-1994 *
**************************************************************/

#include <proto/wb.h>

#include "filereq.h"

/****************************************************************************************/

struct Library		*WorkbenchBase;

char TOPAZSTR[] = "topaz.font";
char DOTINFOSTR[] = ".info";

struct TextAttr topaz80 = { "topaz.font",8,FS_NORMAL,FPF_ROMFONT|FPF_DESIGNED };


/****************************************************************************************/

#define FILEREQ_FLAGS \
	(FREQF_NOBUFFER|FREQF_DOWILDFUNC|FREQF_MULTISELECT|FREQF_SAVE|FREQF_NOFILES|\
	 FREQF_PATGAD|FREQF_SELECTDIRS)
#define FONTREQ_FLAGS \
	(FREQF_NOBUFFER|FREQF_DOWILDFUNC|FREQF_FIXEDWIDTH|FREQF_COLORFONTS|\
	 FREQF_CHANGEPALETTE|FREQF_LEAVEPALETTE|FREQF_SCALE|FREQF_STYLE)
#define SCREENMODEREQ_FLAGS \
	(SCREQF_SIZEGADS|SCREQF_DEPTHGAD|SCREQF_NONSTDMODES|SCREQF_GUIMODES|\
	 SCREQF_AUTOSCROLLGAD|SCREQF_OVERSCANGAD)

/****************************************************************************************/

/********************
*                   *
*  REQUESTER ENTRY  *
*                   *
********************/

/* This is also FontRequestA and ScreenModeRequestA! */

APTR ASM SAVEDS FileRequestA (
    REGPARAM(a1, struct RealFileRequester *, freq),
    REGPARAM(a2, char *, filename),
    REGPARAM(a3, char *, title),
    REGPARAM(a0, struct TagItem *, taglist))
{
    GlobData 				*glob;
    struct ReqEntry 			*entry;
    struct TagItem			*tag;
    const struct TagItem *tstate = taglist;
    struct RealFontRequester 		*fontreq;
    struct RealScreenModeRequester 	*scrmodereq = NULL;
    struct DiskfontBase 		*DiskfontBase;
    struct TextAttr 			*fontattr = NULL;
    struct TextFont 			*deffont;
    struct Locale 			*locale = NULL;
    char 				*pubname = NULL;
    int 				reqhandler = FALSE, mon, propmaskset = FALSE;
    ULONG 				tagdata;


    if (!(glob = AllocVec (sizeof(GlobData), MEMF_PUBLIC|MEMF_CLEAR)))
	 return ((APTR)FALSE);

    glob->reqtype = REQTYPE(freq);
    if (glob->reqtype == RT_FILEREQ)
    {

	/* AROS timer.device checks IO length to make sure apps
	   dont use a too small/wrong iorequest structure */
	   
	glob->timereq.tr_node.io_Message.mn_Length = sizeof(glob->timereq);
	
	if (OpenDevice ("timer.device", UNIT_VBLANK, (struct IORequest *)&glob->timereq, 0))
	{

	    FreeVec (glob);
	    return ((APTR)FALSE);
	}
	
	glob->buff = &freq->buff;
	glob->freq = freq;
	freq->filename = filename;
	glob->wilddotinfo = EndsInDotInfo (freq->patstr, strlen (freq->patstr));
	ParsePatternNoCase (freq->patstr, glob->matchpat, sizeof( glob->matchpat ) );
    }
    else if (glob->reqtype == RT_FONTREQ)
    {

	DiskfontBase = (struct DiskfontBase *)OldOpenLibrary ("diskfont.library");
	glob->diskfontbase = DiskfontBase;
	
	if (!DiskfontBase)
	{
	    FreeAll (glob);
	    return ((APTR)FALSE);
	}
	
	glob->fontreq = fontreq = (struct RealFontRequester *)freq;
	glob->buff = &fontreq->buff;
	filename = fontreq->fontname;
	glob->sampleheight = 24;
	glob->maxsize = MAXINT;
    }
    else
    { /* RT_SCREENMODEREQ */

	glob->scrmodereq = scrmodereq = (struct RealScreenModeRequester *)freq;
	glob->buff = &scrmodereq->buff;
    }
    
    /* defaults */
    glob->flags = freq->Flags;	            /* = [scmd|font]req->Flags */
    glob->reqpos = freq->ReqPos;           /* = [scmd|font]req->ReqPos */
    glob->leftedge = freq->LeftOffset;     /* = [scmd|font]req->LeftOffset */
    glob->topedge = freq->TopOffset;       /* = [scmd|font]req->TopOffset */
    deffont = freq->DefaultFont;           /* = [scmd|font]req->DefaultFont */
    glob->waitpointer = freq->WaitPointer; /* = [scmd|font]req->WaitPointer */
    glob->lockwindow = freq->LockWindow;	/* = [scmd|font]req->LockWindow */
    glob->shareidcmp = freq->ShareIDCMP; 	/* = [scmd|font]req->ShareIDCMP */
    glob->reqheight = freq->ReqHeight;		/* = [scmd|font]req->ReqHeight */

    /* These must be (but automatically are, MEMF_CLEAR) NULL */
//	glob->gadtxt[4] = NULL;
// glob->underchar = 0;

    /* init global vars */
    glob->firstentry = glob->buff->firstname;
    glob->bufferentry = (glob->firstentry != NULL);
    glob->req = freq;
    glob->newdir = TRUE;
    glob->maxdepth = glob->maxwidth = glob->maxheight = MAXINT;



    /* parse tags */
    while ((tag = NextTagItem (&tstate)))
    {
	tagdata = tag->ti_Data;
	if (tag->ti_Tag > RT_TagBase)
	{
	    switch (tag->ti_Tag)
	    {
		case RT_Window:		glob->prwin = (struct Window *)tagdata;
					break;
		case RT_ReqPos:		glob->reqpos = tagdata; break;
		case RT_LeftOffset:	glob->leftedge = tagdata; break;
		case RT_TopOffset:	glob->topedge = tagdata; break;
		case RT_PubScrName:	pubname = (char *)tagdata; break;
		case RT_Screen:		glob->scr = (struct Screen *)tagdata; break;
		case RT_ReqHandler:	*(APTR *)tagdata = glob;
					reqhandler = TRUE;
					break;
		case RT_DefaultFont:	deffont = (struct TextFont *)tagdata; break;
		case RT_WaitPointer: 	glob->waitpointer = tagdata; break;
		case RT_Underscore:	glob->underchar = tagdata; break;
		case RT_ShareIDCMP:	glob->shareidcmp = tagdata; break;
		case RT_LockWindow:	glob->lockwindow = tagdata; break;
		case RT_ScreenToFront: 	glob->noscreenpop = !tagdata; break;
		case RT_TextAttr:		fontattr = (struct TextAttr *)tagdata; break;
		case RT_IntuiMsgFunc: 	glob->imsghook = (struct Hook *)tagdata; break;
		case RT_Locale:		locale = (struct Locale *)tagdata; break;
		/* RTFO_Flags, RTSC_Flags */
		case RTFI_Flags:	if (glob->reqtype == RT_FILEREQ)
						tagdata &= FILEREQ_FLAGS;
					else if (glob->reqtype == RT_FONTREQ)
						tagdata &= FONTREQ_FLAGS;
					else
					{
					    tagdata &= SCREENMODEREQ_FLAGS;
					    tagdata |= FREQF_NOBUFFER;
					}
					glob->flags = tagdata;
					break;
		/* RTFO_Height, RTSC_Height */
		case RTFI_Height:	glob->reqheight = tagdata;
					break;
		/* RTFO_OkText, RTSC_OkText */
		case RTFI_OkText:	glob->gadtxt[4] = (char *)tagdata; break;
		case RTFI_VolumeRequest: glob->volumerequest = 0x80000000 | tagdata; break;
		/* RTFO_FilterFunc, RTSC_FilterFunc */
		case RTFI_FilterFunc: 	glob->filterhook = (struct Hook *)tagdata; break;
		case RTFI_AllowEmpty: 	glob->allowempty = tagdata; break;
		case RTFO_SampleHeight: glob->sampleheight = tagdata; break;
		case RTFO_MinHeight:	glob->minsize = tagdata; break;
		case RTFO_MaxHeight:	glob->maxsize = tagdata; break;
		case RTSC_PropertyFlags: glob->propertyflags = tagdata;
					 if (!propmaskset) glob->propertymask = 0xFFFFFFFF;
					 break;
		case RTSC_PropertyMask: glob->propertymask = tagdata;
					propmaskset = TRUE;
					break;
		case RTSC_MinWidth:	glob->minwidth = tagdata; break;
		case RTSC_MaxWidth:	glob->maxwidth = tagdata; break;
		case RTSC_MinHeight:	glob->minheight = tagdata; break;
		case RTSC_MaxHeight:	glob->maxheight = tagdata; break;
		case RTSC_MinDepth:	glob->mindepth = tagdata; break;
		case RTSC_MaxDepth:	glob->maxdepth = tagdata; break;

	    }
	}
    }



    glob->catalog = RT_OpenCatalog (locale);
    if (!glob->gadtxt[4])
    {
	glob->gadtxt[4] = GetStr (glob->catalog, MSG_OK);
	glob->underchar = '_';
    }


    if (glob->volumerequest)
    {
	FreeReqBuffer (glob->req);
	glob->flags |= FREQF_NOFILES|FREQF_NOBUFFER;
	glob->flags &= ~(FREQF_SAVE|FREQF_PATGAD|FREQF_SELECTDIRS);
    }


    if (glob->reqtype == RT_FILEREQ)
    {
	if (glob->flags & FREQF_NOFILES)
		SetFileDirMode (glob->buff, glob->flags);
	glob->file_id = glob->buff->file_id;
	glob->directory_id = glob->buff->directory_id;
    }


    if (!glob->prwin || !glob->prwin->UserPort
		     || (glob->prwin->UserPort->mp_SigTask != ThisProcess()))
	glob->shareidcmp = FALSE;


    if (!(glob->scr = GetReqScreen (&glob->newreqwin, &glob->prwin,
						      glob->scr, pubname)))
    {
	FreeAll (glob);
	return ((APTR)FALSE);
    }

    
    glob->vp = &glob->scr->ViewPort;
    if (glob->flags & FREQF_CHANGEPALETTE)
    {
	if (!(glob->colcount = GetVpCM (glob->vp, &glob->colormap)))
	{
	    FreeAll (glob);
	    return ((APTR)FALSE);
	}
	glob->colcount = (1 << glob->colcount);
    }

    
    if (fontattr) glob->font = *fontattr;
    else glob->font = *glob->scr->Font;

    if (glob->reqtype == RT_SCREENMODEREQ)
    {

	if (scrmodereq->DisplayID == INVALID_ID)
	{
	    glob->modeid = GetVPModeID (glob->vp);
	    glob->depth = glob->scr->BitMap.Depth;
	    glob->width = glob->scr->Width;
	    glob->height = glob->scr->Height;
	    glob->autoscroll = (glob->scr->Flags & AUTOSCROLL);
	}
	else
	{
	    glob->modeid = scrmodereq->DisplayID;
	    glob->depth = scrmodereq->DisplayDepth;
	    glob->width = scrmodereq->DisplayWidth;
	    glob->height = scrmodereq->DisplayHeight;
	    glob->autoscroll = scrmodereq->AutoScroll;
	}
	
	glob->overscantype = scrmodereq->OverscanType;
	if (!GetModeData (glob, glob->modeid, &mon))
	    glob->modeid = INVALID_ID;
	else
	{
	    GetModeDimensions (glob);
	    if (glob->width == 0xffff) glob->width = glob->defwidth;
	    if (glob->height == 0xffff) glob->height = glob->defheight;
	    if (glob->depth == 0xffff) glob->depth = glob->diminfo.MaxDepth;
	    if (!(glob->flags & SCREQF_SIZEGADS))
		glob->usedefwidth = glob->usedefheight = TRUE;
	    else
	    {
		glob->usedefwidth = (glob->width == glob->defwidth);
		glob->usedefheight = (glob->height == glob->defheight);
	    }
	}
    }


    if (!(glob->visinfo = GetVisualInfoA (glob->scr, NULL))
	|| !(glob->drinfo = GetScreenDrawInfo (glob->scr)))
    {
	FreeAll (glob);
	return ((APTR)FALSE);
    }


    glob->pens = glob->drinfo->dri_Pens;
    glob->title = title;
    glob->os30 = (IntuitionBase->LibNode.lib_Version >= 39);

    if (!glob->buff->firstname)
    {
	glob->buff->gotopos = glob->buff->pos = glob->buff->currentnum = 0;
    }


    entry = glob->buff->firstname;
    while (entry)
    {
	entry->re_EntryLen = entry->re_SizeLenPix = 0;
	entry = (struct ReqEntry *)entry->re_Next;
    }


retryopenwin:

    if (!(glob->reqfont = GetReqFont (&glob->font, deffont, &glob->fontheight,
				      &glob->fontwidth, TRUE)))
    {
	FreeAll (glob);
	return ((APTR)FALSE);
    }
    glob->fontbase = glob->reqfont->tf_Baseline;


    if (!SetupReqWindow (glob, FALSE))
    {

	if (glob->font.ta_YSize > 8)
	{
	    glob->font = topaz80;
	    CloseFont (glob->reqfont);
	    goto retryopenwin;
	}
	FreeAll (glob);
	return ((APTR)FALSE);
    }


    if (glob->reqtype == RT_SCREENMODEREQ) DisplayModeAttrs (glob);
    RenderReqWindow (glob, FALSE, TRUE);
    glob->winlock = DoLockWindow (glob->prwin, glob->lockwindow, NULL, TRUE);
    DoWaitPointer (glob->prwin, glob->waitpointer, TRUE);

    /* initialize hook structure */
    glob->intuihook.h_Entry = (ULONG (*)())IntuiMsgFunc;
    glob->intuihook.h_Data = (void *)glob;

    glob->frontscr = IntuitionBase->FirstScreen;
    DoScreenToFront (glob->scr, glob->noscreenpop, TRUE);

    my_SetStringGadget (glob->reqwin, glob->filegad, filename);

    /* fill in RealHandlerInfo */
    glob->func = (ULONG (*)())PropReqHandler;
    glob->WaitMask = glob->winmask = (1 << glob->reqwin->UserPort->mp_SigBit);
    if (glob->appwindow) glob->WaitMask |= (1 << glob->appwinport->mp_SigBit);
    glob->DoNotWait = TRUE;

    if (reqhandler) return ((APTR)CALL_HANDLER);
    return ((APTR)LoopReqHandler ((struct rtHandlerInfo *)glob));
}

/****************************************************************************************/

/**************
*             *
*  MAIN LOOP  *
*             *
***************/

void STDARGS SAVEDS FreeReqToolsFonts (void)
{
    struct AssignList *list, *next;

    FreeVec (ReqToolsBase->AvailFontsHeader);
    list = ReqToolsBase->FontsAssignList;
    while (list)
    {
	next = list->al_Next;
	FreeVec (list);
	list = next;
    }
    ReqToolsBase->AvailFontsHeader = NULL;
    ReqToolsBase->FontsAssignList = NULL;
}

/****************************************************************************************/

int REGARGS CalcClicked (GlobData *glob, struct IntuiMessage *im)
{
    return ((im->MouseY - glob->boxtop) / glob->entryheight);
}

/****************************************************************************************/

void REGARGS CompClicked (GlobData *glob)
{
    glob->displaylist[glob->clicked]->re_Flags ^= ENTRYF_SELECTED|ENTRYF_HIGHLIGHTED;
    PrintEntry (glob, glob->clicked);
}

/****************************************************************************************/

/* TIMER STUFF */

void REGARGS StopTimer (GlobData *glob)
{
    struct Node *node;
    int 	othermsgs = FALSE, gotreply = FALSE;

   if (glob->timerstarted)
    {
	AbortIO ((struct IORequest *)&glob->timereq);
	/* We don't use WaitIO() since not sure it leaves other messages
		intact. */
	while (!gotreply)
	{
	    Wait (glob->winmask);
	    /* Traverse message list and look for timereq msg */
	    Disable();
	    for (node = glob->reqwin->UserPort->mp_MsgList.lh_Head;
		 node->ln_Succ; node = node->ln_Succ)
	    {
		if (node == (struct Node *)&glob->timereq)
		{
		    Remove (node);
		    gotreply = TRUE;
		    break;
		}
		else othermsgs = TRUE;
	    }
	    Enable();
	}
	
	if (othermsgs) Signal ((struct Task *)ThisProcess(), glob->winmask);
	glob->timerstarted = FALSE;
    }

}

/****************************************************************************************/

void REGARGS StartTimer (GlobData *glob, int micros)
{
    StopTimer (glob);
    
    glob->timereq.tr_node.io_Command = TR_ADDREQUEST;
    glob->timereq.tr_time.tv_secs = 0;
    glob->timereq.tr_time.tv_micro = micros;
    SendIO ((struct IORequest *)&glob->timereq);
    
    glob->timerstarted = TRUE;
}

/****************************************************************************************/

void REGARGS EndQuiet (GlobData *glob)
{
    int i;

    if (glob->quiet)
    {
	if (!glob->exnext)
	{
	    if (rtLockPrefs()->Flags & RTPRF_IMMSORT) glob->firsttimer = TRUE;
	    rtUnlockPrefs();
	}
	
	if (glob->firsttimer)
	{
	    ScrollerMoved (glob, glob->buff->gotopos);
	    glob->firsttimer = FALSE;
	}
	else
	{
	    if (glob->lastdisplaylistnum != -1)
	    {
		if (glob->lastdisplaylistnum < glob->numentries)
		{
		    i = glob->lastdisplaylistnum;
		    glob->lastdisplaylistnum = -1;
		    while (i < glob->numentries) PrintEntry (glob, i++);
		}
	    }
	    AdjustScroller (glob);
	}
    }
    
    if (glob->exnext) StartTimer (glob, 200000);
    else
    {
	StopTimer (glob);
	glob->quiet = FALSE;
    }
}

/****************************************************************************************/

/* MAIN LOOP */

struct IntuiMessage *REGARGS ProcessWin_Msg_Freq (GlobData *glob, struct IntuiMessage *imsg)
{
    struct IntuiMessage *reqmsg;

    if (imsg->IDCMPWindow == glob->reqwin)
    {
	reqmsg = GT_FilterIMsg (imsg);
	if (reqmsg) return (reqmsg);
	ReplyMsg ((struct Message *)imsg);
    }
    else
    {
	if (glob->imsghook)
	{
	    SetDrawerAndFileFields (glob);
	    CallHookPkt (glob->imsghook, glob->req, imsg);
	    ResetDrawerAndFileFields (glob);
	}
	ReplyMsg ((struct Message *)imsg);
    }
    
    return (NULL);
}

/****************************************************************************************/

void REGARGS SetDrawerAndFileFields (GlobData *glob)
{
    if (REQTYPE(glob->req) == RT_FILEREQ)
    {
	glob->tempdir = glob->freq->Dir;
	glob->freq->Dir = glob->freq->dirname;
	if (!(glob->flags & FREQF_NOFILES))
	{
	    strcpy (glob->tempfname, glob->freq->filename);
	    strcpy (glob->freq->filename, ((struct StringInfo *)glob->filegad->SpecialInfo)->Buffer);
	}
    }
}

/****************************************************************************************/

void REGARGS ResetDrawerAndFileFields (GlobData *glob)
{
    if (REQTYPE(glob->req) == RT_FILEREQ)
    {
	glob->freq->Dir = glob->tempdir;
	if (!(glob->flags & FREQF_NOFILES))
		strcpy (glob->freq->filename, glob->tempfname);
    }
}

/****************************************************************************************/

#ifdef __AROS__
AROS_UFH3(void, IntuiMsgFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(APTR, req, A2),
    AROS_UFHA(struct IntuiMessage *, imsg, A1))
{
    AROS_USERFUNC_INIT
#else
void ASM SAVEDS IntuiMsgFunc (
    REGPARAM(a0, struct Hook *, hook),
    REGPARAM(a2, APTR, req),
    REGPARAM(a1, struct IntuiMessage *,imsg))
{
#endif
    GlobData *glob = (GlobData *)hook->h_Data;

    if (imsg->IDCMPWindow == glob->reqwin)
    {
	if (imsg->Class == IDCMP_REFRESHWINDOW) RenderReqWindow (glob, TRUE, FALSE);
    }
    else if ((glob->imsghook) && (glob->imsghook != hook))
    {
	SetDrawerAndFileFields (glob);
	CallHookPkt (glob->imsghook, glob->req, imsg);
	ResetDrawerAndFileFields (glob);
    }
#ifdef __AROS__
    AROS_USERFUNC_EXIT
#endif
}

/****************************************************************************************/

/* Improve this function to handle:
 * 1) Volume lists in the file requester
 * 2) The font requester
 */
int REGARGS FindEntryPos (GlobData *glob, char *name, int entry_id)
{
    struct ReqEntry *entry, *entry2;
    int i, val;

    if (!glob->buff->firstname) return (0);
    i = 0;
    if (*name && (entry = FindEntry (glob->buff, name, -1, entry_id, NULL, FIND_VOLUMENAME)))
    {
	entry2 = (struct ReqEntry *)glob->buff->firstname->re_Next;
	while (entry2 != (struct ReqEntry *)entry->re_Next)
	{
	    if (!(entry2->re_Flags & ENTRYF_HIDDEN)) i++;
	    entry2 = (struct ReqEntry *)entry2->re_Next;
	}
    }
    
    glob->buff->gotopos = i;
    val = glob->buff->currentnum - glob->numentries;
    
    if (glob->buff->gotopos > val) glob->buff->gotopos = val;
    if (glob->buff->gotopos < 0) glob->buff->gotopos = 0;
    
    if (glob->buff->pos != glob->buff->gotopos)
    {
	glob->buff->pos = glob->buff->gotopos;
	AdjustScroller (glob);
	UpdateDisplayList (glob);
	PrintFiles (glob);
    }
    
    if (i >= glob->buff->currentnum) i = (glob->buff->currentnum - 1);
    
    return (i);
}

/****************************************************************************************/

void REGARGS DeselectFiles (GlobData *glob, int clicked, int dirsonly)
{
    int i;

    for (i = 0; i < glob->numentries; i++)
    {
	if (i >= glob->buff->currentnum) break;
	
	if (i != clicked)
	{
	    if (dirsonly && (glob->displaylist[i]->re_Type == glob->file_id))
		continue;
		
	    if (glob->displaylist[i]->re_Flags & ENTRYF_SELECTED)
	    {
		glob->numselected--;
		glob->displaylist[i]->re_Flags &= ~ENTRYF_SELECTED;
		PrintEntry (glob, i);
	    }
	}
    }
}

/****************************************************************************************/

int REGARGS
ClickDown( GlobData *glob, int clicked, struct IntuiMessage *reqmsg, int qual )
{
    struct BufferData	*buff;
    struct ReqEntry		*entry;
    char	*str, *str2, tempstr[108];
    int	ctype, val;

    buff = glob->buff;

    if( clicked >= buff->currentnum )
    {
	    return( FALSE );
    }

    glob->clicked = clicked;
    entry = glob->displaylist[ clicked ];

    if( entry->re_Flags & ENTRYF_GHOSTED )
    {
	    entry->re_Flags &= ~ENTRYF_SELECTED;
	    glob->downgadget = 0;
	    return( FALSE );
    }

    str = entry->re_Name;
    str2 = "";
    val = !( ( glob->flags & FREQF_MULTISELECT ) && ( qual & IEQUALIFIER_SHIFT ) );
    ctype = entry->re_Type;

    if( ctype == glob->file_id )
    {
	if( !( entry->re_Flags & ENTRYF_SELECTED ) || val )
	{
	    str2 = str;
	}

	goto filefont;
    }
    else if( ctype == glob->directory_id )
    {
	if( !reqmsg )
	{
	    strcpy( tempstr, str );
	    StrCat( tempstr, "/" );
	    str2 = tempstr;
	}
	else if( !( glob->flags & FREQF_SELECTDIRS ) )
	{
	    goto nodirselect;
	}

filefont:
	my_SetStringGadget( glob->reqwin, glob->filegad, str2 );

	if( !( glob->flags & FREQF_SELECTDIRS ) )
	{
	    DeselectFiles( glob, clicked, TRUE );
	    CountAllDeselect( glob, TRUE );
	}

	if( val )
	{
	    DeselectFiles( glob, clicked, FALSE );
	    CountAllDeselect (glob, FALSE);
	}

	if( reqmsg )
	{
	    if( ( clicked == glob->lastclicked ) &&
		    DoubleClick( glob->sec, glob->mic, reqmsg->Seconds, reqmsg->Micros ) )
	    {
		if( ctype == glob->directory_id )
		{
		    AddPart( glob->freq->dirname, str, sizeof( glob->freq->dirname ) );
		    NewDir( glob );
		    return( FALSE );
		}
		else if( !( glob->flags & ( FREQF_SAVE | FREQF_NOFILES ) ) )
		{
		    entry->re_Flags |= ENTRYF_SELECTED;
		    return( TRUE );
		}
	    }
	}

	if( glob->flags & FREQF_MULTISELECT )
	{
	    if( entry->re_Flags & ENTRYF_SELECTED )
	    {
		glob->numselected--;
	    }
	    else
	    {
		glob->numselected++;
	    }

	    UpdateNumSelGad( glob );
	}

	entry->re_Flags ^= ENTRYF_SELECTED;

	if( ( entry->re_Flags & ENTRYF_SELECTED ) && glob->buff->sorted )
	{
	    glob->selectedpos = ( glob->buff->pos + clicked );
	}

	PrintEntry( glob, clicked );
	glob->downgadget = 0;

	if( reqmsg )
	{
	    glob->sec = reqmsg->Seconds;
	    glob->mic = reqmsg->Micros;
	    glob->lastclicked = clicked;
	}
    }
    else
    {
	if( !reqmsg )
	{
	    str2 = str;

	    if( ctype == VOLUME )
	    {
		while( *str2++ != ' ' )
		{
		}

		while( *str2 == ' ' )
		{
		    str2++;
		}
	    }

	    goto filefont;

	}

nodirselect:
	CompClicked( glob );
    }

    return( FALSE );
}

/****************************************************************************************/

/*****************
* Requester exit *
*****************/

ULONG REGARGS LeaveReq (GlobData *glob, char *filename)
{
    struct rtFileList *selfile = (APTR)TRUE;
    int flags = glob->flags, nodir = glob->nodir;
    int allowempty = glob->allowempty;

    if (glob->filestr) strcpy (filename, glob->filestr);
    
    if (glob->reqtype == RT_FILEREQ)
    {
	if (!(flags & FREQF_MULTISELECT))
	{
	    FreeAllCheckBuffer (glob);
	    /* can't use glob here anymore because it is freed! */
	    if (flags & FREQF_NOFILES) return ((ULONG)!nodir);
	    if (!nodir && (filename[0] || allowempty)) return (TRUE);
	    return 0;
	}
	
	if (!nodir)
	{
	    if (!(glob->flags & FREQF_SELECTDIRS)) CountAllDeselect (glob, TRUE);
	    selfile = AllocSelectedFiles (glob);
	}
	else selfile = NULL;
    }
    else if (glob->reqtype == RT_FONTREQ)
    {
	glob->fontreq->Attr.ta_Style &= ~(FSF_ITALIC|FSF_BOLD|FSF_UNDERLINED);
	glob->fontreq->Attr.ta_Style |= glob->fontstyle;
	selfile = (APTR)(filename[0] != 0);
    }
    else
    {
	if (glob->modeid == INVALID_ID) selfile = FALSE;
	else
	{
	    glob->scrmodereq->DisplayID = glob->modeid;
	    if (glob->modeid & HAM ) glob->depth = (glob->depth == 7 ? 6 : 8 );
	    glob->scrmodereq->DisplayDepth = glob->depth;
	    glob->scrmodereq->DisplayWidth = glob->width;
	    glob->scrmodereq->DisplayHeight = glob->height;
	    glob->scrmodereq->OverscanType = glob->overscantype;
	    glob->scrmodereq->AutoScroll = glob->autoscroll;
	}
    }
    
    FreeAllCheckBuffer (glob);
    
    return ((ULONG)selfile);
}

/****************************************************************************************/

static void REGARGS CloseWinFreeRest (GlobData *glob)
{
    StopTimer (glob);

    if (glob->reqtype == RT_FILEREQ)
	CloseDevice ((struct IORequest *)&glob->timereq);
    FreeVpCM (glob->vp, glob->colormap, !(glob->flags & FREQF_LEAVEPALETTE));
 
    if (glob->newreqwin.Type == PUBLICSCREEN) UnlockPubScreen (NULL, glob->scr);
    DoScreenToFront (glob->frontscr, glob->noscreenpop, FALSE);
 
    if (glob->reqwin)
    {
	*glob->winaddr = glob->oldwinptr;
	DoLockWindow (glob->prwin, glob->lockwindow, glob->winlock, FALSE);
	DoWaitPointer (glob->prwin, glob->waitpointer, FALSE);
	
	if (glob->appwindow) RemoveAppWindow (glob->appwindow);
	
	DeleteMsgPort (glob->appwinport);
	CloseLibrary (WorkbenchBase);
	DoCloseWindow (glob->reqwin, glob->shareidcmp);
    }
    
    my_FreeGadgets (glob->buttoninfo.glist);
    my_FreeLabelImages (&glob->labelimages);
    FreeVisualInfo (glob->visinfo);
    
    if (glob->drinfo) FreeScreenDrawInfo (glob->scr, glob->drinfo);
    if (glob->reqfont) CloseFont (glob->reqfont);
    
    CloseLibrary (glob->diskfontbase);
    FreeVec (glob);
}

/****************************************************************************************/

void REGARGS FreeAllCheckBuffer (GlobData *glob)
{
    if (glob->lock || (glob->disks && !glob->volumerequest)
		   || (glob->flags & FREQF_NOBUFFER))
	FreeAll (glob);
    else CloseWinFreeRest (glob);
}

/****************************************************************************************/

void REGARGS FreeAll (GlobData *glob)
{
    FreeReqBuffer (glob->req);
    UnLockReqLock (glob);
    CloseWinFreeRest (glob);
}

/****************************************************************************************/

struct rtFileList *REGARGS AllocSelectedFiles (GlobData *glob)
{
    struct rtFileList *ptr, **last, *selfile;
    struct ReqEntry *entry;
    int len, isfile, foundstr = FALSE;
    /* We do some checks agains the file gadget if available,
     * or the path gadget, if there is no file gadget, and we thus
     * are a volume requester or something similar
     */
    char *str, *gadstr = glob->filegad ? glob->filestr : glob->patgadstr;

    selfile = NULL; last = &selfile;
    for (entry = (struct ReqEntry *)glob->firstentry->re_Next;;
	 entry = (struct ReqEntry *)entry->re_Next)
   {
	if (!entry)
	{
	    /* We build the filelist, now we check if the filename in
		    the gadget is among the ones in the list */
	    str = gadstr;
	    if (foundstr || !*str) break;
	    /* The filename in the gadget is not is the list!
		    We will discard the list and only return the filename in the
		    gadget!  This is the most intuitive behaviour! */
	    rtFreeFileList (selfile);
	    selfile = NULL; last = &selfile;
	    isfile = TRUE;
	}
	else
	{
	    if (!(entry->re_Flags & ENTRYF_SELECTED)) continue;
	    str = entry->re_Name;
	    isfile = entry->re_Type == glob->file_id;
	    if (!Stricmp (str, gadstr)) foundstr = TRUE;
	}
	
	len = strlen (str);
	
	if (!(ptr = (struct rtFileList *)AllocVec(sizeof (struct rtFileList) + len + 1,
						  MEMF_PUBLIC|MEMF_CLEAR)))
	{
	    rtFreeFileList (selfile);
	    return (NULL);
	}
	
	ptr->StrLen = isfile ? len : -1;
	ptr->Name = (char *)(4 + (ULONG)&ptr->Name);
	strcpy (ptr->Name, str);
	*last = ptr;
	last = &ptr->Next;
	
	if (!entry) break;
    }
    
    return (selfile);
}

/****************************************************************************************/
