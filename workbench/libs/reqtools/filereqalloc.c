/**************************************************************
*                                                             *
*      File/Font/Screenmode requester                         *
*                                                             *
*                                 (c) Nico François 1991-1994 *
**************************************************************/

#include "filereq.h"

/****************************************************************************************/

#ifdef __AROS__
#define fib_EntryType fib_DirEntryType
#endif

/****************************************************************************************/

ULONG structsize[] =
{
    sizeof (struct RealFileRequester) 		+ sizeof(ULONG),	/* RT_FILEREQ */
    sizeof (struct rtReqInfo) 			+ sizeof(ULONG),	/* RT_REQINFO */
    sizeof (struct RealFontRequester)		+ sizeof(ULONG),	/* RT_FONTREQ */
    sizeof (struct RealScreenModeRequester) 	+ sizeof(ULONG),	/* RT_SCREENMODEREQ */
};

/****************************************************************************************/

/********************
* rtAllocRequestA() *
********************/

void REGARGS
SetFileDirMode (struct BufferData *buff, ULONG flags)
{
    ULONG prefsflags;

    /* Check preferences if we have to display files or directories first */
    prefsflags = rtLockPrefs()->Flags;
    buff->dirsmixed = (prefsflags & RTPRF_DIRSMIXED);

    if (flags & FREQF_NOFILES)
    {
	prefsflags |= RTPRF_DIRSFIRST;
	buff->dirsmixed = FALSE;
    }

    if (prefsflags & RTPRF_DIRSFIRST)
    {
	buff->file_id = 1;
	buff->directory_id = 0;
    }
    else
    {
	buff->file_id = 0;
	buff->directory_id = 1;
    }

    rtUnlockPrefs();
}

/****************************************************************************************/

ASM struct BufferData *GetBufferDataPtr (OPT_REGPARAM(a1, APTR, req))
{
    switch (REQTYPE(req))
    {
	case RT_FILEREQ:
	    return (&((struct RealFileRequester *)req)->buff);
	    
	case RT_FONTREQ:
	    return (&((struct RealFontRequester *)req)->buff);
	    
	case RT_SCREENMODEREQ:
	    return (&((struct RealScreenModeRequester *)req)->buff);
	    
    }
    
    return (NULL);
    
}

/****************************************************************************************/

APTR ASM SAVEDS AllocRequestA (
    REGPARAM(d0, ULONG, type),
    REGPARAM(a0, struct TagItem *,taglist))
{
    struct BufferData 	*buff;
    ULONG 		*reqstruct;

    if (!(reqstruct = AllocVec (structsize[type], MEMF_PUBLIC | MEMF_CLEAR)))
	return (NULL);
	
    *reqstruct++ = type;
    
    switch (type)
    {
	case RT_FILEREQ:
	    ((FI_REQ)reqstruct)->Dir = ((FI_REQ)reqstruct)->dirname;
	    ((FI_REQ)reqstruct)->MatchPat = ((FI_REQ)reqstruct)->patstr;
	    ((FI_REQ)reqstruct)->hideinfo = TRUE;
	    SetFileDirMode (&((FI_REQ)reqstruct)->buff, 0);
	    break;
		
	case RT_FONTREQ:
	    ((FO_REQ)reqstruct)->Attr = topaz80;
	    /* copy 'topaz.font' to name buffer and point name pointer there */
	    strcpy (((FO_REQ)reqstruct)->fontname, TOPAZSTR);
	    ((FO_REQ)reqstruct)->Attr.ta_Name = ((FO_REQ)reqstruct)->fontname;
	    break;
		
	case RT_SCREENMODEREQ:
	    ((SC_REQ)reqstruct)->DisplayID = INVALID_ID;
	    ((SC_REQ)reqstruct)->OverscanType = OSCAN_TEXT;
	    ((SC_REQ)reqstruct)->AutoScroll = TRUE;
	    break;
		
    }
    
    ((struct rtReqInfo *)reqstruct)->ReqPos = REQPOS_DEFAULT;

    buff = GetBufferDataPtr (reqstruct);    
    if (buff && (DOSBase->dl_lib.lib_Version >= 39))
    {
	buff->pool = CreatePool (MEMF_PUBLIC | MEMF_CLEAR, 8192, 4096);
    }
    
    return (reqstruct);
}

/****************************************************************************************/

/********************
* rtFreeReqBuffer() *
********************/

void ASM SAVEDS FreeReqBuffer (REGPARAM(a1, APTR, req))
{
    struct BufferData 	*buff;
    struct ReqEntry 	*entry, *temp;

    if ((buff = GetBufferDataPtr (req)))
    {
	entry = buff->firstname;
	while ((temp = entry))
	{
	    entry = (struct ReqEntry *)entry->re_Next;
	    FreeVecPooled (buff->pool, temp);
	}
	buff->firstname = NULL;
    }
}


/****************************************************************************************/

/******************
* rtFreeRequest() *
******************/

void ASM SAVEDS FreeRequest (REGPARAM(a1, APTR, req))
{
    struct BufferData *buff;

    if (!req) return;
    
    FreeReqBuffer (req);
    
    if ((buff = GetBufferDataPtr (req)))
    {
	if (buff->pool) DeletePool (buff->pool);
    }
    
    FreeVec ((APTR)(((IPTR)req) - sizeof(ULONG)));
}

/****************************************************************************************/

/*********************
* rtChangeReqAttrA() *
*********************/

LONG ASM SAVEDS
ChangeReqAttrA (REGPARAM(a1, APTR, req),
    	        REGPARAM(a0, struct TagItem *, taglist))
{
    UBYTE			fibspace[sizeof(struct FileInfoBlock)+4];
    struct FileInfoBlock	*fib = (struct FileInfoBlock *)(((LONG)&fibspace[3] >> 2) << 2);
    struct TagItem 		*tag;
    const struct TagItem *tstate = taglist;
    struct RealFileRequester 	*freq;
    struct ReqEntry 	 	*entry, *curr;
    struct BufferData 	 	*buff;
    struct Screen 		*scr;
    IPTR 			tagdata;

    /* parse tags */
    while ((tag = NextTagItem (&tstate)))
    {
	tagdata = tag->ti_Data;
	if (tag->ti_Tag > RT_TagBase)
	{
	    switch (tag->ti_Tag)
	    {
		case RTFI_Dir:
		    FreeReqBuffer (req);
		    strcpy (((struct RealFileRequester *)req)->dirname, (char *)tagdata);
		    break;
			
		case RTFI_MatchPat:
		    strcpy (((struct RealFileRequester *)req)->patstr, (char *)tagdata);
		    break;
			
		case RTFI_AddEntry:
		case RTFI_RemoveEntry:
		    freq = (struct RealFileRequester *)req;
		    buff = &(freq->buff);
		    if (!buff->firstname) return (TRUE);
		    if (tag->ti_Tag == RTFI_AddEntry)
		    {
			Examine ((BPTR)tagdata, fib);
			return ((LONG)(AddEntry (NULL, buff, fib->fib_FileName, fib->fib_Size,
									(fib->fib_EntryType > 0)
									? buff->directory_id : buff->file_id) != NULL));
		    }
		    else
		    {
			entry = FindEntry (buff, (char *)tagdata, -1, -1, NULL, 0);
			curr = (struct ReqEntry *)entry->re_Next;
			if (!curr || Stricmp ((char *)tagdata, entry->re_Next->ln_Name))
				return( 0 );
			entry->re_Next = curr->re_Next;
			FreeVecPooled (buff->pool, curr);
		    }
		    break;
			
		case RTFO_FontName:
		    strcpy (((struct RealFontRequester *)req)->fontname, (char *)tagdata);
		    break;
			
		case RTFO_FontHeight:
		    ((struct RealFontRequester *)req)->Attr.ta_YSize = tagdata;
		    break;
			
		case RTFO_FontStyle:
		    ((struct RealFontRequester *)req)->Attr.ta_Style = tagdata;
		    break;
			
		case RTFO_FontFlags:
		    ((struct RealFontRequester *)req)->Attr.ta_Flags = tagdata;
		    break;
			
		case RTSC_ModeFromScreen:
		    scr = (struct Screen *)tagdata;
		    ((SC_REQ)req)->DisplayID = GetVPModeID (&scr->ViewPort);
		    ((SC_REQ)req)->DisplayWidth = scr->Width;
		    ((SC_REQ)req)->DisplayHeight = scr->Height;
		    ((SC_REQ)req)->DisplayDepth = scr->BitMap.Depth;
		    ((SC_REQ)req)->AutoScroll = (scr->Flags & AUTOSCROLL);
		    break;
			
		case RTSC_DisplayID:
		    ((SC_REQ)req)->DisplayID = tagdata & ~(0x00000440);
		    ((SC_REQ)req)->DisplayWidth = ((SC_REQ)req)->DisplayHeight
			= ((SC_REQ)req)->DisplayDepth = 0xffff;
		    break;
			
		case RTSC_DisplayWidth:
		    ((SC_REQ)req)->DisplayWidth = tagdata;
		    break;
			
		case RTSC_DisplayHeight:
		    ((SC_REQ)req)->DisplayHeight = tagdata;
		    break;
			
		case RTSC_DisplayDepth:
		    ((SC_REQ)req)->DisplayDepth = tagdata;
		    break;
			
		case RTSC_OverscanType:
		    ((SC_REQ)req)->OverscanType = tagdata;
		    break;
			
		case RTSC_AutoScroll:
		    ((SC_REQ)req)->AutoScroll = tagdata;
		    break;
			
	    } /* switch (tag->ti_Tag) */
	    
	} /* if (tag->ti_Tag > RT_TagBase) */
	
    } /* while ((tag = NextTagItem (&tstate))) */
 
#warning There was a missing return. For AROS I added return 1!?

    return 1;    
}

/****************************************************************************************/

/*******************
* rtFreeFileList() *
*******************/

void ASM SAVEDS FreeFileList (REGPARAM(a0, struct rtFileList *,selfile))
{
    struct rtFileList *last;

    while (selfile)
    {
	last = selfile;
	selfile = selfile->Next;
	FreeVec (last);
    }
}

/****************************************************************************************/
