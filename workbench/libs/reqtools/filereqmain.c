/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/**************************************************************
*                                                             *
*      File/Font/Screenmode requester                         *
*                                                             *
*                                 (c) Nico François 1991-1994 *
**************************************************************/

#include "filereq.h"

/* The AmigaOS V40 ScrollWindowRaster() function is buggy and
   causes Enforcer Hits */
   
#define NO_SCROLLWINDOWRASTER 1

#ifdef  __AROS__

    #define   DEBUG  1
    #include  <aros/debug.h>
#else

    #define   D(x)  

#endif


/****************************************************************************************/

#ifdef __AROS__
#define fib_EntryType fib_DirEntryType
#endif

/****************************************************************************************/

#ifndef MTYPE_APPWINDOW
#define MTYPE_APPWINDOW		7	/* msg from an app window */
#endif

/****************************************************************************************/

static const IPTR getstringtags[] = { RTGS_Width,180,RT_LockWindow,TRUE,
				  RT_ReqPos,REQPOS_CENTERWIN,RT_ShareIDCMP,TRUE,TAG_END };
static const IPTR ezreqtags[] = { RT_Underscore,'_',RTEZ_Flags,EZREQF_NORETURNKEY,
				 TAG_MORE, (ULONG)&getstringtags[2] };


/****************************************************************************************/

BOOL
ExpandLink( GlobData *glob )
{
#ifndef __AROS__
    struct FileLock  *fl;
#endif
    BOOL              rc = FALSE;
    LONG              res;

    *glob->winaddr = ( APTR ) -1;

#ifdef __AROS__
    res = ReadLink(NULL, glob->lock, glob->fib.fib_FileName, glob->linkbuf,
		   sizeof(glob->linkbuf));
#else
    fl = BADDR( glob->lock );
    res = ReadLink(fl->fl_Task, glob->lock, glob->fib.fib_FileName, glob->linkbuf, sizeof(glob->linkbuf));
#endif

    if(res != 0)
    {
	BPTR lock;

	if( ( lock = Lock( glob->linkbuf, SHARED_LOCK ) ) )
	{
	    STRPTR end = FilePart( glob->linkbuf );

	    if( end != glob->linkbuf )
	    {
		*end = 0;
	    }

	    Examine( lock, &glob->linkfib );
	    UnLock( lock );
	    rc = TRUE;
	}
    }

    *glob->winaddr = glob->reqwin;

    return( rc );
}

/****************************************************************************************/

ULONG ASM SAVEDS PropReqHandler (
    REGPARAM(a1, struct RealHandlerInfo *, glob),
    REGPARAM(d0, ULONG, sigs),
    REGPARAM(a0, struct TagItem *, taglist))
{
    struct IntuiMessage 	*reqmsg = NULL, *imsg, im;
    struct Gadget 		*gad;
    struct RealFileRequester 	*freq = NULL;
    struct RealFontRequester 	*fontreq = NULL;
    struct BufferData 		*buff;
    struct DiskfontBase 	*DiskfontBase = glob->diskfontbase;
    struct TagItem 		*tag;
    const struct TagItem    *tstate = taglist;
    struct AvailFontsHeader 	*afh;
    struct AvailFonts 		*af;
    struct ReqEntry 		*entry;
    struct DosList 		*dlist;
    struct AppMessage 		*appmsg;
    struct AssignList 		*assignlist, *fontslist, **prevassign;
    int 			clicked, ctype, sel, val, code, qual, doubleclick, checkbox;
    int 			i, step, start, stop, shortage, buffsize, mon, doactgad, lastpos;
    UBYTE 			*fdir = NULL, *filename = NULL, *str, *str2, *str3, key;
    ULONG 			tagdata;
    BPTR 			parent;
    APTR 			winlock;
    ULONG 			id;

    /* uncomment if sigs is no longer ignored */
//  if (glob->DoNotWait) sigs = SetSignal (0, 0);

    doactgad = !(glob->buttoninfo.lastcode);

    if (glob->reqtype == RT_FILEREQ)
    {
	freq = glob->freq;
	filename = freq->filename;
	fdir = freq->dirname;
    }
	    
    if (glob->reqtype == RT_FONTREQ)
    {
	fontreq = glob->fontreq;
	filename = fontreq->fontname;
    }
    
    buff = glob->buff;

    /* parse tags */
    while ((tag = NextTagItem (&tstate)))
    {
	tagdata = tag->ti_Data;
	if (tag->ti_Tag > RT_TagBase)
	{
	    switch (tag->ti_Tag)
	    {
		case RTRH_EndRequest:
		    if (tagdata == REQ_OK)
			return (LeaveReq (glob, filename));
			
		    FreeAllCheckBuffer (glob);
		    
		    return (FALSE);
		   
	    }
	}
    }

    /* MAIN LOOP */

    if (glob->appwinport)
    {
	while ((appmsg = (struct AppMessage *)GetMsg (glob->appwinport)))
	{
	    if (appmsg->am_Type == MTYPE_APPWINDOW)
	    {
		if (appmsg->am_NumArgs >= 1)
		{
		    NameFromLock (appmsg->am_ArgList->wa_Lock, fdir, 256);
		    my_SetStringGadget (glob->reqwin, glob->filegad,
					appmsg->am_ArgList->wa_Name);
		    NewDir (glob);
		}
		ReplyMsg ((struct Message *)appmsg);
		goto iterate;
	    }
    	    ReplyMsg ((struct Message *)appmsg);	    
	}
    }

    if (glob->newdir)
    {

	glob->disks = FALSE;
	
	if (!glob->bufferentry)
	{
	    ClearFilesRect (glob);
	    ClearAndInitReqBuffer (glob);
	    AdjustScroller (glob);
	}
	else RethinkReqDisplay (glob);
	
	glob->selectedpos = -1;
	glob->numselected = 0;
	UpdateNumSelGad (glob);
	glob->lastclicked = -1;
	glob->exnext = FALSE;
	
	if (!glob->bufferentry)
	{
	    if (glob->reqtype == RT_FILEREQ)
	    {
		if (!glob->volumerequest)
		{
		    /* Render LED before lock, for good behaviour with ArcHandler */
		    glob->ledon = TRUE;
		    RenderLED (glob);
		    if ((glob->lock = Lock (fdir, SHARED_LOCK)))
		    {
			Examine (glob->lock, &glob->fib);
			if (glob->fib.fib_EntryType <= 0)
			    UnLockReqLock (glob);
		    }
		    else if ((glob->flags & FREQF_SAVE) &&
			     fdir[strlen(fdir)-1] != ':' &&
			     IoErr() != ERROR_DEVICE_NOT_MOUNTED)
		    {
			struct TagItem tags[] =
			{
			    {RT_Window		, (IPTR)glob->reqwin	},
			    {RT_IntuiMsgFunc	, (IPTR)&glob->intuihook},
			    {TAG_MORE		, (IPTR)ezreqtags	}
			};

			if (rtEZRequestA (GetStr (glob->catalog, MSG_CREATE_DRAWER),
					  GetStr (glob->catalog, MSG_OK_BAR_CANCEL),
					  NULL, &glob->drawerstr, tags))
			    glob->lock = CreateDir (glob->drawerstr);
		    }
		     
		    if (!glob->lock)
		    {
			glob->ledon = FALSE;
			RenderLED (glob);
			SetWinTitleFlash (glob->reqwin,
				GetStr (glob->catalog, MSG_DIR_ERROR));
			FreeReqBuffer (glob->req);
			ClearDisplayList (glob);
			glob->nodir = TRUE;
		    }
		    else
		    {
			i = strlen (fdir);
			
			if (fdir[i-1] == '/' && i > 1)
			{
			    if (fdir[i-2] != ':' && fdir[i-2] != '/') fdir[i-1] = 0;
			}
			
			my_SetStringGadget (glob->reqwin, glob->drawergad, fdir);
			glob->exnext = TRUE;
			glob->nodir = FALSE;
			/* first examine before ExNext */
			Examine (glob->lock, &glob->fib);
			StartTimer (glob, 500000);
			glob->firsttimer = glob->quiet = TRUE;
			
		    }
		    
		} /* if (glob->reqtype == RT_FILEREQ) */
		else
		{
		    /* Volume requester */
		    AddDiskNames (glob, glob->volumerequest);

		    if (fdir[0])
		    {
			if (!FindCurrentPos (glob, fdir, MAXINT, VOLUME))
			    FindCurrentPos (glob, fdir, MAXINT, ASSIGN);

			glob->buff->pos -= (glob->numentries - 1);

			if (glob->buff->pos < 0) glob->buff->pos = 0;

			glob->buff->gotopos = glob->buff->pos;
		    }

		    RethinkReqDisplay (glob);
		    glob->disks = TRUE;
		    
		} /* if (glob->reqtype == RT_FILEREQ) else ... */
		 
	    } /* if (glob->reqtype == RT_FILEREQ) */	     
	    else if (glob->reqtype == RT_FONTREQ)
	    {
		winlock = rtLockWindow (glob->reqwin);
		ReqToolsBase->AvailFontsLock = TRUE;
		afh = ReqToolsBase->AvailFontsHeader;

		dlist = LockDosList (LDF_DEVICES|LDF_ASSIGNS|LDF_READ);
		dlist = FindDosEntry (dlist, "FONTS", LDF_DEVICES|LDF_ASSIGNS);
		
		if (afh)
		{
		    if (!dlist) afh = NULL;
		    else
		    {
			if (ReqToolsBase->FontsAssignType != dlist->dol_Type) afh = NULL;
			else
			{
			    if (ReqToolsBase->FontsAssignLock != dlist->dol_Lock)
				afh = NULL;
			    else
			    {
				fontslist = ReqToolsBase->FontsAssignList;
				assignlist = dlist->dol_misc.dol_assign.dol_List;
				
				while (assignlist)
				{
				    if (fontslist->al_Lock != assignlist->al_Lock)
				    {
					afh = NULL;
					break;
				    }
				    fontslist = fontslist->al_Next;
				    assignlist = assignlist->al_Next;
				}
				
				if (fontslist) afh = NULL;
				
			    }
			}
		    }
		    if (!afh) FreeReqToolsFonts();
		}
			
		if (!afh)
		{
		    buffsize = 2000;
		    for (;;)
		    {
			if (!(afh = AllocVec (buffsize, 0))) break;
			
			shortage = AvailFonts ((STRPTR)afh, buffsize, AFF_DISK|AFF_MEMORY);
			if (shortage)
			{
			    buffsize += shortage;
			    FreeVec (afh);
			}
			else break;
		    }
		    
		    ReqToolsBase->AvailFontsHeader = afh;
		    if (dlist)
		    {
			ReqToolsBase->FontsAssignType = dlist->dol_Type;
			ReqToolsBase->FontsAssignLock = dlist->dol_Lock;
			
			if (dlist->dol_Type == DLT_DIRECTORY)
			{
			    assignlist = dlist->dol_misc.dol_assign.dol_List;
			    prevassign = &(ReqToolsBase->FontsAssignList);

			    while (assignlist)
			    {
				fontslist = AllocVec (sizeof (struct AssignList), MEMF_CLEAR);
				*prevassign = fontslist;
				
				if (!fontslist) break;
				
				fontslist->al_Lock = assignlist->al_Lock;
				assignlist = assignlist->al_Next;
				prevassign = &(fontslist->al_Next);
				
			    }
			    
			}
			
		    }
		    
		} /* if (!afh) */
			
		UnLockDosList (LDF_DEVICES|LDF_ASSIGNS|LDF_READ);

		if (!afh)
		    DisplayBeep (NULL);
		else
		{
		    af = (struct AvailFonts *)((long)afh + 2);
		    for (i = 0; i < afh->afh_NumEntries; i++, af++)
		    {
			if ((af->af_Type == AFF_MEMORY) &&
			    !(af->af_Attr.ta_Flags & FPF_ROMFONT)) continue;
			    
			if (glob->filterhook)
			{
			    if (!CallHookPkt (glob->filterhook, fontreq, &af->af_Attr))
				continue;
			}
			else
			{
			    if (glob->flags & FREQF_DOWILDFUNC)
			    {
				if (CallHook (fontreq->Hook, (Object *)fontreq,
					      REQHOOK_WILDFONT, &af->af_Attr))
				    continue;
			    }
			}
			
			str = af->af_Attr.ta_Name;
			val = strlen(str) - 5;
			str[val] = 0;
			
			if (!(entry = AddEntry (glob, buff, str, af->af_Attr.ta_YSize, FONT)))
			    break;
			    
			str[val] = '.';
			entry->re_Flags = af->af_Attr.ta_Flags;
			entry->re_Style = af->af_Attr.ta_Style;
		    }
			    
		    str = filename;
		    val = strlen(str) - 5;
		    str[val] = 0;
		    FindCurrentPos (glob, str, glob->fontreq->Attr.ta_YSize, FONT);
		    str[val] = '.';
		    
		}
		
		ReqToolsBase->AvailFontsLock = FALSE;
		RethinkReqDisplay (glob);
		rtUnlockWindow (glob->reqwin, winlock);
		
	    } /* else if (glob->reqtype == RT_FONTREQ) */
	    else
	    {
		id = INVALID_ID;
		while ((id = NextDisplayInfo (id)) != INVALID_ID)
		{
		    if (glob->filterhook)
		    {
			if (!CallHookPkt (glob->filterhook, glob->scrmodereq, (APTR)id))
			    continue;
		    }
		    
		    /* filter out dual playfield modes */
		    if (id & DUALPF) continue;
		    /* filter out modes with default monitor */
		    if ((id & MONITOR_ID_MASK) == DEFAULT_MONITOR_ID) continue;
		    if (!GetModeData (glob, id, &mon)) continue;
		    /* is mode available ? */
		    if (glob->dispinfo.NotAvailable) continue;
		    if (!(entry = AddEntry (glob, buff, glob->nameinfo.Name, id, SCRMODE)))
			break;
		}
		/* get mode data for currently selected mode */
		GetModeData (glob, glob->modeid, &mon);
		
		if (!FindCurrentPos (glob, glob->nameinfo.Name, glob->modeid, SCRMODE))
		{
		    if (glob->firstentry->re_Next)
		    {
			glob->modeid  = (ULONG)(((struct ReqEntry *)glob->firstentry->re_Next)->re_Size);
			SetTextGad (glob, glob->modetxtgad, ((struct ReqEntry *)glob->firstentry->re_Next)->re_Name);
			GetModeData (glob, glob->modeid, &mon);
		    }
		    else
		    {
			glob->modeid = INVALID_ID;
			SetTextGad (glob, glob->modetxtgad, NULL);
			
			if (glob->depthgad)
			{
			    glob->currmindepth = glob->currmaxdepth = glob->maxdepth = glob->mindepth = glob->depth = 0;
			    UpdateDepthGad (glob);
			    myGT_SetGadgetAttrs (glob->maxcolgad, glob->reqwin, NULL, GTTX_Text, (IPTR) "0", TAG_END);
			}
			
			if (glob->flags & SCREQF_SIZEGADS)
			{
			    myGT_SetGadgetAttrs (glob->widthgad, glob->reqwin, NULL, GA_Disabled, TRUE, TAG_END);
			    myGT_SetGadgetAttrs (glob->defwgad, glob->reqwin, NULL, GA_Disabled, TRUE, TAG_END);
			    myGT_SetGadgetAttrs (glob->heightgad, glob->reqwin, NULL, GA_Disabled, TRUE, TAG_END);
			    myGT_SetGadgetAttrs (glob->defhgad, glob->reqwin, NULL, GA_Disabled, TRUE, TAG_END);
			}
			
			if (glob->flags & SCREQF_OVERSCANGAD)
			    myGT_SetGadgetAttrs (glob->overscangad, glob->reqwin, NULL, GA_Disabled, TRUE, TAG_END);
			
			if (glob->flags & SCREQF_AUTOSCROLLGAD)
			    myGT_SetGadgetAttrs (glob->checkboxgad[CHECKBOX_AUTOSCROLL], glob->reqwin, NULL,
						 GA_Disabled, TRUE, TAG_END);
		    }
		    
		} /* else if (glob->reqtype == RT_FONTREQ)*/
		
		GetModeDimensions (glob);
		DisplayModeAttrs (glob);
		RethinkReqDisplay (glob);
	    }

	} /* if (!glob->bufferentry) */
		
	glob->bufferentry = glob->newdir = FALSE;

    } /* if (glob->newdir) */

    if (glob->exnext)
    {
	if ((glob->exnext = ExNext (glob->lock, &glob->fib)))
	{
	    ULONG size = glob->fib.fib_Size;

	    ctype = glob->fib.fib_EntryType;

	    /* Try to examine links */
	    if( ( glob->fib.fib_EntryType == ST_SOFTLINK ) && ExpandLink( glob ) )
	    {
		size = glob->linkfib.fib_Size;
		ctype = glob->linkfib.fib_EntryType;
	    }

	    /* Hooks */
	    if (glob->filterhook)
	    {
		SetDrawerAndFileFields (glob);
		val = CallHookPkt (glob->filterhook, freq, &glob->fib);
		ResetDrawerAndFileFields (glob);
		
		if (!val) goto skipfile;
	    }
	    else
	    {
		if (glob->flags & FREQF_DOWILDFUNC)
		{
		    SetDrawerAndFileFields (glob);
		    val = CallHook (freq->Hook, (Object *)freq, REQHOOK_WILDFILE, &glob->fib);
		    ResetDrawerAndFileFields (glob);
		    
		    if (val) goto skipfile;
		}
	    }

	    ctype = ( ctype > 0 ) ? glob->directory_id : glob->file_id;

	    if( !AddEntry( glob, buff, glob->fib.fib_FileName, size, ctype ) )
		glob->exnext = FALSE;

skipfile:
	    ;
	    
	} /* if ((glob->exnext = ExNext (glob->lock, &glob->fib))) */

	if (!glob->exnext)
	{
	    UnLockReqLock (glob);
	    EndQuiet (glob);
	}
		
    } /* if (glob->exnext) */

    do
    {
	reqmsg = NULL;
	
	while ((imsg = (struct IntuiMessage *)GetMsg (glob->reqwin->UserPort)))
	{
	    /* Reply message from timer.device ? */
	    if ((struct timerequest *)imsg == &glob->timereq)
	    {
		glob->timerstarted = FALSE;
		EndQuiet (glob);
		continue;
	    }
	    
	    /* Message from string gadget hook ? */
	    if (!imsg->Class && !imsg->Seconds)
	    {
		/* Mark message so string gadget hook knows it has been received */
		imsg->Micros = 0;
		
		if (glob->selectedpos != -1)
		{
		    DeselectFiles (glob, -1, FALSE);
		    CountAllDeselect (glob, FALSE);
		    UpdateNumSelGad (glob);
		    glob->selectedpos = -1;
		}
		
		if (*glob->filestr && !glob->disks)
		    FindEntryPos (glob, glob->filestr, glob->file_id);
		continue;
	    }
	    
	    if ((reqmsg = ProcessWin_Msg_Freq (glob, imsg))) break;
	}

	if (reqmsg)
	{
	    memcpy( &im, reqmsg, sizeof( im ) );
	    Reply_GT_Msg (reqmsg);

	    gad = (struct Gadget *)im.IAddress;
	    code = im.Code;
	    qual = im.Qualifier;

	    switch (im.Class)
	    {
		case IDCMP_DISKINSERTED:
		case IDCMP_DISKREMOVED:
		    if (glob->disks)
		    {
			glob->disks = FALSE;
			ShowDisks (glob);
		    }
		    break;
		    
		case IDCMP_REFRESHWINDOW:
		    RenderReqWindow (glob, TRUE, FALSE);
		    break;
		    
		case IDCMP_CLOSEWINDOW:
		    FreeAllCheckBuffer (glob);
		    return (FALSE);
		    
		case IDCMP_NEWSIZE:
		    if (glob->filegad)
		    {
			strcpy (glob->tempfname, glob->filestr);
		    }
		    RemoveGList (glob->reqwin, glob->buttoninfo.glist, -1);
		    my_FreeGadgets (glob->buttoninfo.glist);
		    glob->buttoninfo.glist = NULL;
		    my_FreeLabelImages (&glob->labelimages);
		    glob->labelimages.NextImage = NULL;
		    glob->reqheight = glob->reqwin->Height;
		    
		    if (!SetupReqWindow (glob, TRUE))
		    {
			glob->reqheight = glob->reqwin->MinHeight;
			glob->reqwidth = glob->reqwin->MinWidth;
			
			if (!SetupReqWindow (glob, TRUE))
			{
			    DisplayBeep (NULL);
			    FreeAllCheckBuffer (glob);
			    return (FALSE);
			}
		    }
		    
		    my_SetStringGadget (glob->reqwin, glob->filegad, glob->tempfname);
		    
		    if (!glob->nodir || glob->disks)
		    {
			i = glob->buff->currentnum - glob->numentries;
			
			if (i < 0)
			    glob->buff->gotopos = glob->buff->pos = 0;
			else if (glob->buff->pos > i)
			    glob->buff->gotopos = glob->buff->pos = i;
			    
			AdjustScroller (glob);
			UpdateDisplayList (glob);
			PrintFiles (glob);
		    }
		    RenderReqWindow (glob, FALSE, FALSE);
		    break;
		    
		case IDCMP_MOUSEMOVE:
		    if (!glob->downgadget) break;
		    		    
		    if (glob->downgadget == FILES)
		    {
			if (glob->clicked != ~0)
			{
			    clicked = CalcClicked (glob, &im);
			    sel = !(glob->displaylist[glob->clicked]->re_Flags & ENTRYF_HIGHLIGHTED);
			    val = !(clicked != glob->clicked || im.MouseX < glob->boxleft
				  || im.MouseX > glob->boxright);
			    if (val == sel) CompClicked (glob);
			}
		    }
		    else if (gad->GadgetID == FPROP) ScrollerMoved (glob, code);
		    else if (gad->GadgetID == DEPTH) UpdateDepthDisplay (glob, code, glob->modeid);
		    break;
		    
		case IDCMP_MOUSEBUTTONS:
		    glob->downgadget = 0;

		    if( glob->reqtype == RT_FILEREQ )
		    {
			if( ( code == MENUDOWN ) && !glob->volumerequest )
			{
			    if( !glob->disks )
			    {
				ShowDisks( glob );
			    }
			    else
			    {
				strcpy( fdir, glob->drawerstr );
				NewDir( glob );
			    }
			}
			else if( code == MIDDLEDOWN )
			{
			    BOOL doit;

			    doit = rtLockPrefs()->Flags & RTPRF_MMBPARENT;
			    rtUnlockPrefs();

			    if( doit )
			    {
				goto parentdir;
			    }
			}
		    }

		    break;
		    
		case IDCMP_GADGETDOWN:
		    EndQuiet (glob);
		    glob->downgadget = id = gad->GadgetID;
		    if (id == FILES)
		    {
			glob->clicked = ~0;
			clicked = CalcClicked (glob, &im);
			if (ClickDown (glob, clicked, &im, qual))
			    return (LeaveReq (glob, filename));
		    }
		    else if (id == FPROP) 
		    {
		        ScrollerMoved (glob, code);
		    }
		    break;
		    
		case IDCMP_RAWKEY:
		    if ((glob->reqtype == RT_FILEREQ) && !(glob->flags & FREQF_NOFILES) &&
			 (code == RAWKEY_UP || code == RAWKEY_DOWN))
		    {
			glob->activegadget = glob->mainstrgad;
			
			if (glob->selectcurrpos || !glob->buff->firstname) break;
			
			lastpos = glob->selectedpos;
			
			if (glob->selectedpos == -1)
			{
			    if (qual & IEQUALIFIER_SHIFT)
			    {
				if (code == RAWKEY_UP)
				    glob->selectedpos = glob->buff->pos;
				else
				    glob->selectedpos = glob->buff->pos +
							MIN (glob->numentries, glob->buff->currentnum)  - 1;
			    }
			    else if (qual & IEQUALIFIER_CONTROL)
			    {
				 if (code == RAWKEY_UP) glob->selectedpos = 0;
				 else glob->selectedpos = glob->buff->currentnum - 1;
			    }
			    else
			    {
				if (glob->disks) i = (qual & IEQUALIFIER_ALT) ? ASSIGN : VOLUME;
				else i = (qual & IEQUALIFIER_ALT) ? glob->directory_id : glob->file_id;
				glob->selectedpos = FindEntryPos (glob, glob->filestr, i);
			    }
			}
			else
			{
			    if (code == RAWKEY_UP)
			    {
				if (qual & IEQUALIFIER_SHIFT)
				{
				    if (glob->selectedpos != glob->buff->pos)
					    glob->selectedpos = glob->buff->pos;
				    else glob->selectedpos -= (glob->numentries - 1);
				    if (glob->selectedpos < 0) glob->selectedpos = 0;
				}
				else if (qual & IEQUALIFIER_CONTROL)
				    glob->selectedpos = 0;
				else if (glob->selectedpos) glob->selectedpos--;
			    }
			    else
			    {
				if (qual & IEQUALIFIER_SHIFT)
				{
				    val = (glob->buff->pos + glob->numentries - 1);
				    if (glob->selectedpos != val) glob->selectedpos = val;
				    else glob->selectedpos += glob->numentries - 1;
				    if (glob->selectedpos >= glob->buff->currentnum)
					    glob->selectedpos = glob->buff->currentnum - 1;
				}
				else if (qual & IEQUALIFIER_CONTROL)
				    glob->selectedpos = glob->buff->currentnum - 1;
				else if (glob->selectedpos < (glob->buff->currentnum - 1))
				    glob->selectedpos++;
			    }
			}
			
			if (glob->selectedpos != lastpos)
			{
			    i = (glob->selectedpos - glob->buff->pos);
			    val = glob->buff->gotopos;
			    
			    if (i < 0)
			    {
				val += i;
				if (val < 0) val = 0;
			    }
			    
			    if (i >= glob->numentries)
			    {
				val += (i - glob->numentries) + 1;
				if (val > (glob->buff->currentnum - glob->numentries))
					val = (glob->buff->currentnum - glob->numentries);
			    }
			    ScrollerMoved (glob, val);
			    
			    if (glob->buff->pos != glob->buff->gotopos)
				AdjustScroller (glob);
				
			    glob->selectcurrpos = TRUE;
			}
			break;
		    }
		    
		    if ((id = CheckGadgetKey (code, qual, &key, &glob->buttoninfo)))
			goto dogadgetup;
			    
		    gad = NULL;
		    checkbox = -1;
		    
		    if (key == 27)	/* Esc? */
			goto docancel;
		    else if (key == glob->gadkey[CHECKBOX_AUTOSCROLL])
			checkbox = CHECKBOX_AUTOSCROLL;
		    else if (key == glob->gadkey[CHECKBOX_BOLD])
			checkbox = CHECKBOX_BOLD;
		    else if (key == glob->gadkey[CHECKBOX_ITALIC])
			checkbox = CHECKBOX_ITALIC;
		    else if (key == glob->gadkey[CHECKBOX_UNDERLINE])
			checkbox = CHECKBOX_UNDERLINE;
		    else if (key == glob->patkey) gad = glob->patgad;
		    else if (key == glob->heightkey) gad = glob->heightgad;
		    else if (key == glob->widthkey) gad = glob->widthgad;
		    else if (key == glob->overscankey)
		    {
			if (glob->overscangad)
			{
			    if (qual & IEQUALIFIER_SHIFT)
			        glob->overscantype--;
			    else
			        glob->overscantype++;
				
			    if (glob->overscantype > OSCAN_MAX) glob->overscantype = 0;
			    if (glob->overscantype < 0) glob->overscantype = OSCAN_MAX;
			    
			    code = glob->overscantype;
			    myGT_SetGadgetAttrs (glob->overscangad, glob->reqwin, NULL,
						 GTCY_Active, code, TAG_END);
			    id = OVERSCN;
			    goto dogadgetup;
			}
		    }
		    else if (key == glob->depthkey)
		    {
			if (glob->depthgad)
			{
			    if (qual & IEQUALIFIER_SHIFT)
			    {
				if (glob->depth > glob->currmindepth) glob->depth--;
			    }
			    else
			    {
				if (glob->depth < glob->currmaxdepth) glob->depth++;
			    }
			    UpdateDepthGad (glob);
			}
		    }
		    
		    if (gad) glob->activegadget = gad;
		    
		    if (checkbox != -1)
		    {
			if ((gad = glob->checkboxgad[checkbox]))
			{
			    IPTR checked;
			    struct TagItem get_tags[] =
			    {
			    	{GTCB_Checked, (IPTR)&checked},
				{TAG_DONE   	    	     }
			    };
			    
    	    	    	#ifdef __AROS__
			    /* the other way (checking GFLG_SELECTED) does
			       not work with AROS gadtools.library, and in
			       general not with boopsi gadgets. Only with
			       non-boopsi bool gadgets it works (but not
			       yet in AROS) */
			    
			    GT_GetGadgetAttrsA(gad, glob->reqwin, NULL, get_tags);
			    myGT_SetGadgetAttrs (gad, glob->reqwin, NULL,
					         GTCB_Checked, !checked, TAG_END);
			    
			#else
			    myGT_SetGadgetAttrs (gad, glob->reqwin, NULL,
					         GTCB_Checked, !(gad->Flags & GFLG_SELECTED), TAG_END);
    	    	    	#endif
			    goto fakegadgetup;
			}
		    }
		    break;
		    
		case IDCMP_GADGETUP:
#ifdef __AROS__
		    /* AROS gadtools library gadgets are boopsi gadgets, so checking
		       for GTYP_STRGADGET does not work*/
		    if ((code == KEYB_SHORTCUT) && ((gad->GadgetID == PATSTR) ||
		    				    (gad->GadgetID == DRAWERSTR) ||
						    (gad->GadgetID == FILESTR)) )

		    
#else
		    if (((gad->GadgetType & GTYP_GTYPEMASK) == GTYP_STRGADGET)
			&& (code == KEYB_SHORTCUT))
#endif
		    {
			doactgad = FALSE;
			continue;
		    }

fakegadgetup:
		    id = gad->GadgetID;
dogadgetup:
		    glob->downgadget = 0;
		    switch (id)
		    {
			case FILES:
			    clicked = CalcClicked (glob, &im);
			    
			    if (clicked != glob->clicked) break;
			    
			    entry = glob->displaylist[clicked];
			    str = entry->re_Name;
			    doubleclick = ((clicked == glob->lastclicked)
				    && DoubleClick (glob->sec, glob->mic, im.Seconds, im.Micros));
			    ctype = entry->re_Type;
			    
			    if (ctype == glob->directory_id)
			    {
				if (!(glob->flags & FREQF_SELECTDIRS))
				{
				    AddPart (fdir, str, 256);
				    NewDir (glob);
				    
				    /* CHECKME: added hoping to fix MuForce hit. See below. */
				    
				    entry = NULL;
				    
				    /* END CHECKME */
				    break;
				}
			    }
			    else switch (ctype)
			    {
				case VOLUME:
				case ASSIGN:
				    CompClicked (glob);
				    if (ctype == VOLUME)
				    {
					str2 = str;
					while (*str2 != ' ') str2++;
					str3 = str2;
					while (*str3 == ' ') str3++;
					if ((glob->volumerequest && !( *str == '-' && str[ 1 ] == ' ' ))
					    || *str3 == ':'
					    || FindVolume (glob, str3, entry))
					{
					    strcpy (fdir, str);
					    fdir[str2-str] = 0;
					}
					else strcpy (fdir, str3);
				    }
				    else strcpy (fdir, str);
				    
				    if (!glob->volumerequest)
				    {
					NewDir (glob);

					/* CHECKME: added hoping to fix MuForce hit. See below. */

					entry = NULL;

					/* END CHECKME */

					break;
				    }
				   
				    my_SetStringGadget (glob->reqwin, glob->drawergad, fdir);
checkdoubleclick:
				    if (doubleclick)
					return (LeaveReq (glob, filename));

				    goto rememberclicked;
					
				case FONT:
				    CompClicked (glob);
				    strcpy (filename, str);
				    StrCat (filename, DOTFONTSTR);
				    my_SetStringGadget (glob->reqwin,
							glob->filegad, filename);
				    if (doubleclick)
					return (LeaveReq (glob, filename));

				    fontreq->Attr.ta_YSize = entry->re_Size;
				    fontreq->Attr.ta_Flags = entry->re_Flags;
				    fontreq->Attr.ta_Style = entry->re_Style;
				    fontreq->Attr.ta_Style &= ~(FSF_ITALIC|FSF_BOLD|FSF_UNDERLINED);
				    fontreq->Attr.ta_Style |= glob->fontstyle;
				    my_SetIntegerGadget (glob->reqwin,
							 glob->drawergad, entry->re_Size);
				    ShowFontSample (glob, FALSE, TRUE);
rememberclicked:
				    glob->lastclicked = clicked;
				    glob->sec = im.Seconds; glob->mic = im.Micros;
				    break;
					
				case SCRMODE:
				    CompClicked (glob);
				    SetTextGad (glob, glob->modetxtgad, str);
				    glob->modeid = (ULONG)entry->re_Size;
				    if (!GetModeData (glob, glob->modeid, &mon))
					DisplayBeep (glob->scr);
				    else
				    {
					GetModeDimensions (glob);
					DisplayModeAttrs (glob);
				    }
				    goto checkdoubleclick;
			    }
			    
			    #warning check following line. causes MuForces hits on Amiga when clicking
			    #warning on volume or assign entry. Both with old Amiga reqtools.library and
			    #warning in new reqtools.library compiled from AROS sources.
			    
			#if 0
			    entry->re_Flags &= ~ENTRYF_HIGHLIGHTED;
			#else			    
			    if (entry) entry->re_Flags &= ~ENTRYF_HIGHLIGHTED;			    
			#endif
			    break;
				
			case FONTSIZE:
			    fontreq->Attr.ta_YSize = ((struct StringInfo *)glob->drawergad->SpecialInfo)->LongInt;
			    ShowFontSample (glob, FALSE, TRUE);
			    glob->activegadget = glob->mainstrgad;
			    break;
				
			case ITALIC:
			#ifdef __AROS__
			    {
			    	IPTR checked;
				struct TagItem get_tags[] =
				{
			    	    {GTCB_Checked, (IPTR)&checked},
				    {TAG_DONE   	    	 }
				};
				
				GT_GetGadgetAttrsA(glob->checkboxgad[CHECKBOX_ITALIC], glob->reqwin, NULL, get_tags);
				if (checked) 
    	    	    	    	{
				    glob->fontstyle |= FSF_ITALIC;
				}
				else
				{
				    glob->fontstyle &= ~FSF_ITALIC;
				}
			    }
			    
			#else
			
			    if (glob->checkboxgad[CHECKBOX_ITALIC]->Flags & GFLG_SELECTED)			
				glob->fontstyle |= FSF_ITALIC;
			    else glob->fontstyle &= ~FSF_ITALIC;
			    
    	    	    	#endif
			
			    goto updatestyle;
				
			case UNDERLINE:
			#ifdef __AROS__
			    {
			    	IPTR checked;
				struct TagItem get_tags[] =
				{
			    	    {GTCB_Checked, (IPTR)&checked},
				    {TAG_DONE   	    	 }
				};
				
				GT_GetGadgetAttrsA(glob->checkboxgad[CHECKBOX_UNDERLINE], glob->reqwin, NULL, get_tags);
				if (checked) 
    	    	    	    	{
				    glob->fontstyle |= FSF_UNDERLINED;
				}
				else
				{
				    glob->fontstyle &= ~FSF_UNDERLINED;
				}
			    }
			    
			#else
			
			    if (glob->checkboxgad[CHECKBOX_UNDERLINE]->Flags & GFLG_SELECTED)
				glob->fontstyle |= FSF_UNDERLINED;
			    else glob->fontstyle &= ~FSF_UNDERLINED;
			#endif
			
			    goto updatestyle;
				
			case BOLD:
			#ifdef __AROS__
			    {
			    	IPTR checked;
				struct TagItem get_tags[] =
				{
			    	    {GTCB_Checked, (IPTR)&checked},
				    {TAG_DONE   	    	 }
				};
				
				GT_GetGadgetAttrsA(glob->checkboxgad[CHECKBOX_BOLD], glob->reqwin, NULL, get_tags);
				if (checked) 
    	    	    	    	{
				    glob->fontstyle |= FSF_BOLD;
				}
				else
				{
				    glob->fontstyle &= ~FSF_BOLD;
				}
			    }
			    
			#else
			
			    if (glob->checkboxgad[CHECKBOX_BOLD]->Flags & GFLG_SELECTED)
				    glob->fontstyle |= FSF_BOLD;
			    else glob->fontstyle &= ~FSF_BOLD;
			#endif
updatestyle:
			    ShowFontSample (glob, FALSE, TRUE);
			    break;
				
			case ALL:
			    EndQuiet (glob);
			    if (!glob->nodir && !glob->disks) SelectAll (glob, "#?");
			    break;
				
			case PATTERN:
			    EndQuiet (glob);
			    if (!glob->nodir && !glob->disks)
			    {
				struct TagItem tags[] =
				{
				    {RT_Window		, (IPTR)glob->reqwin	},
				    {RT_IntuiMsgFunc	, (IPTR)&glob->intuihook},
				    {TAG_MORE		, (IPTR)getstringtags	}
				};

				if (rtGetStringA (glob->selpattern, 123,
						  GetStr (glob->catalog, MSG_MATCH_WINTITLE),
						  NULL, tags))
				    SelectAll (glob, glob->selpattern);
			    }
			    break;

			case CLR:
			    EndQuiet (glob);
			    if (!glob->nodir && !glob->disks)
			    {
				CountAllDeselect (glob, FALSE);
				UpdateNumSelGad (glob);
				PrintFiles (glob);
				my_SetStringGadget (glob->reqwin, glob->filegad, "");
			    }
			    break;
				
			case PATSTR:
			    EndQuiet (glob);
			    if (Stricmp (freq->patstr, glob->patgadstr))
			    {
				strcpy (freq->patstr, glob->patgadstr);
				ParsePatternNoCase (freq->patstr, glob->matchpat, sizeof( glob->matchpat ) );
				glob->activegadget = glob->mainstrgad;
				goto refreshlist;
			    }
			    glob->activegadget = glob->mainstrgad;
			    break;
			    
			case INFO:
			    EndQuiet (glob);
			    freq->hideinfo = !freq->hideinfo;
refreshlist:
			    RethinkReqDisplay (glob);
			    UpdateNumSelGad (glob);
			    break;
				
			case DRAWERSTR:
			    glob->activegadget = glob->mainstrgad;
			    
			    if (glob->flags & FREQF_NOFILES)
			    {
				if (glob->volumerequest ||
				    (!glob->nodir && !Stricmp (fdir, glob->drawerstr)))
				{
				    if (qual & IEQUALIFIER_SHIFT) break;
				    
				    strcpy (fdir, glob->drawerstr);
				    val = (glob->volumerequest && !*fdir);
				    my_SelectGadget (!val ? glob->okgad : glob->cancelgad, glob->reqwin);
				    ShortDelay();
				    
				    if (val) goto docancel;
				    
				    return (LeaveReq (glob, filename));
				}
			    }
			    
			    if (!glob->nodir && !glob->disks
				&& !Stricmp (fdir, glob->drawerstr))
				break;
				
			    /* FALLTHROUGH */
				
			case GETDIR:
			    strcpy (fdir, glob->drawerstr);
			    NewDir (glob);
			    break;
				
			case FILESTR:
			    if (code == 0x09) break;
			    
			    if (qual & IEQUALIFIER_SHIFT)
			    {
				glob->activegadget = glob->drawergad;
				break;
			    }
			    
			    if ((qual & IEQUALIFIER_ALT) && (glob->flags & FREQF_PATGAD))
			    {
				glob->activegadget = glob->patgad;
				break;
			    }

			    if (glob->reqtype == RT_FILEREQ)
			    {
				/* extract path from filename if one was entered */
				char tempstr[108];
				BPTR templock = NULL, dirlock, oldcd;

				str = glob->filestr;

				if (*str == '/') {
					strcpy (tempstr, str + 1);
					my_SetStringGadget (glob->reqwin, glob->filegad,
							    tempstr);
					goto parentdir;
					}

				if (*str)
				{
				    *glob->winaddr = (APTR)-1;
				    
				    if ((dirlock = Lock (glob->drawerstr, SHARED_LOCK)))
				    {
					oldcd = CurrentDir (dirlock);
					templock = Lock (str, SHARED_LOCK);
					CurrentDir (oldcd);
				    }
				    
				    *glob->winaddr = glob->reqwin;
				    
				    if (dirlock) UnLock (dirlock);
				    
				    if (templock)
				    {
					Examine (templock, &glob->fib);
					UnLock (templock);
					
					if (glob->fib.fib_EntryType > 0)
					{
					    if (dirlock)
					    {
						strcpy (fdir, glob->drawerstr);
						AddPart (fdir, str, 256);
					    }
					    else strcpy (fdir, str);
					    
					    tempstr[0] = 0;
					    
					    goto setfnamedirgads;
					}
				    }
				}

				if (PathPart (str) != str)
				{
				    val = strlen (str);
				    
				    for (i = 0; i < val; i++)
				    {
					if (str[i] == ':')
					{
					    strcpy (fdir, str);
					    break;
					}
				    }
				    
				    if (i >= val) AddPart (fdir, str, 256);
				    
				    *(PathPart (fdir)) = 0;
				    strcpy (tempstr, FilePart (glob->filestr));
setfnamedirgads:
				    NewDir (glob);
				    my_SetStringGadget (glob->reqwin, glob->filegad, tempstr);
				    my_SetStringGadget (glob->reqwin, glob->drawergad, fdir);
				    break;
				    
				}
				
			    } /* if (glob->reqtype == RT_FILEREQ) */

			    my_SelectGadget (
				(!glob->nodir &&
				    (*glob->filestr || glob->allowempty
						    || ((glob->flags & FREQF_MULTISELECT) &&
							(glob->numselected > 0))
				    )
				) ? glob->okgad : glob->cancelgad, glob->reqwin);
			    ShortDelay();

			case OK:
			    return (LeaveReq (glob, filename));
			    
			case CANCEL:
docancel:
			    FreeAllCheckBuffer (glob);
			    return (FALSE);
				
			case PARENT:
parentdir:
			    UnLockReqLock (glob);
//			    strcpy (fdir, glob->drawerstr);
			    str = PathPart ( fdir );
			    if (*str)
			    {
				*str = 0;
				my_SetStringGadget (glob->reqwin, glob->drawergad, fdir);
				NewDir (glob);
				break;
			    }
			    else 
			    {
				if ((glob->lock = Lock (glob->drawerstr, SHARED_LOCK)))
				{
				    if ((parent = ParentDir (glob->lock)))
				    {
					NameFromLock (parent, fdir, 256);
					UnLock (parent);
					NewDir (glob);
					break;
				    }
				}
			    }

			    if(glob->disks)
				    break;
			    /* FALLTHROUGH! */
				
			case DISKS:
			    if (!glob->disks)
				ShowDisks (glob);
			    else
			    {
				strcpy (fdir, glob->drawerstr);
				NewDir (glob);
			    }
			    break;
				
			case DEPTH:
			    UpdateDepthDisplay (glob, code, glob->modeid);
			    glob->depth = code;
			    break;
				
			case SCRWIDTH:
			    glob->width = IntGadgetBounds (glob, glob->widthgad,
				    MAX (glob->diminfo.MinRasterWidth, glob->minwidth),
				    MIN (glob->diminfo.MaxRasterWidth, glob->maxwidth));
			    glob->usedefwidth = (glob->width == glob->defwidth);
			    myGT_SetGadgetAttrs (glob->defwgad, glob->reqwin, NULL,
						 GTCB_Checked, glob->usedefwidth, TAG_END);
			    glob->activegadget = glob->mainstrgad;
			    break;
				
			case SCRHEIGHT:
			    glob->height = IntGadgetBounds (glob, glob->heightgad,
							    MAX (glob->diminfo.MinRasterHeight, glob->minheight),
							    MIN (glob->diminfo.MaxRasterHeight, glob->maxheight));
			    glob->usedefheight = (glob->height == glob->defheight);
			    myGT_SetGadgetAttrs (glob->defhgad, glob->reqwin, NULL,
						 GTCB_Checked, glob->usedefheight, TAG_END);
			    glob->activegadget = glob->mainstrgad;
			    break;
				
			case DEFWIDTH:
			    glob->usedefwidth = !glob->usedefwidth;
			    SetSizeGads (glob);
			    break;
				
			case DEFHEIGHT:
			    glob->usedefheight = !glob->usedefheight;
			    SetSizeGads (glob);
			    break;
				
			case AUTOSCR:
			    glob->autoscroll = !glob->autoscroll;
			    break;
				
			case OVERSCN:
			    glob->overscantype = code;
			    GetModeDimensions (glob);
			    SetSizeGads (glob);
			    break;
				
		    } /* switch (id) */
		    break;

	    } /* switch (im.Class) */

	} /* if (reqmsg) */

iterate:
	if ((glob->downgadget != FILES) && (glob->buff->gotopos != glob->buff->pos))
	{
	#if NO_SCROLLWINDOWRASTER
	    BOOL refreshall = FALSE;
	#endif
	
	    start = 0; stop = glob->numentries;
	    step = glob->buff->gotopos - glob->buff->pos;
	    
	    if (ABS(step) < stop)
	    {
		if (ABS(step) > 1) step /= 2;
		if (step > 3) step = 3;
		if (step < -3) step = -3;
		
		SetBPen (glob->reqrp, glob->pens[BACKGROUNDPEN]);
		mySetWriteMask (glob->reqrp, glob->entrymask);
		
    	    #if !NO_SCROLLWINDOWRASTER
		if (glob->os30)
		{
		    ScrollWindowRaster (glob->reqwin, 0, step * glob->entryheight,
					glob->boxleft, glob->boxtop, glob->boxright,
					glob->boxtop + glob->boxheight - 1);
		}
		else
		{
	    #endif
		    ScrollRaster (glob->reqrp, 0, step * glob->entryheight,
				  glob->boxleft, glob->boxtop, glob->boxright,
				  glob->boxtop + glob->boxheight - 1);
		    if (glob->reqrp->Layer->Flags & LAYERREFRESH)
		    	refreshall = TRUE; /* RenderReqWindow (glob, TRUE, FALSE); */
			
	    #if !NO_SCROLLWINDOWRASTER
		}
	    #endif
		mySetWriteMask (glob->reqrp, glob->rpmask);

		if (step > 0)
		    start = stop - step;
		else
		    stop = -step;
		    
	    } /* if (ABS(step) < stop) */
	    
	    glob->buff->pos += step;
	    UpdateDisplayList (glob);
	    
	#if NO_SCROLLWINDOWRASTER
	    if (refreshall)
	    {
	    	RenderReqWindow (glob, TRUE, FALSE);
	    }
	#endif

	    for (i = start; i < stop; i++) PrintEntry (glob, i);
	    
	    glob->DoNotWait = TRUE;
	    
	} /* if ((glob->downgadget != FILES) && (glob->buff->gotopos != glob->buff->pos)) */
	else
	{
	    glob->DoNotWait = glob->exnext || glob->newdir;
	}
	
	if (glob->selectcurrpos && (glob->buff->pos == glob->buff->gotopos))
	{
	    if (glob->selectedpos != -1)
	    {
		ClickDown (glob, (glob->selectedpos - glob->buff->pos), NULL, 0);
	    }
	    glob->selectcurrpos = FALSE;
	}

    } while (reqmsg && !glob->newdir);

    if (doactgad)
    {
/*
#ifdef __AROS__
#warning Disabled this gadget activation here, as in Intuition this functions is slow (why? ask stegerg)
#else
*/
	ActivateGadget (glob->activegadget, glob->reqwin, NULL);

/*
#endif
*/
	if (!(glob->reqwin->IDCMPFlags & IDCMP_RAWKEY))
	{
	    /* Add RAWKEY IDCMP only after initialzing and refreshing the window */
	    ModifyIDCMP (glob->reqwin, IDCMP_RAWKEY|REQ_IDCMP);
	}
    }

    return ((ULONG)CALL_HANDLER);
}

/****************************************************************************************/
