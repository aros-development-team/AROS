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


#ifdef   __AROS__

    #define  DEBUG  0
    #include <aros/debug.h>

#else

    #define  D(x)  

#endif


/****************************************************************************************/

extern struct LocaleBase *LocaleBase;

/****************************************************************************************/

static void REGARGS MarkHidden (GlobData *);
static int REGARGS SkipEntry (GlobData *, struct ReqEntry *);

/****************************************************************************************/

/********************
* Requester Display *
********************/

/****************************************************************************************/

void REGARGS AdjustScroller (GlobData *glob)
{
    myGT_SetGadgetAttrs (glob->scrollergad, glob->reqwin, NULL, GTSC_Total, glob->buff->currentnum,
    								GTSC_Top, glob->buff->gotopos,
								TAG_END);
}

/****************************************************************************************/

void REGARGS ClearFilesRect (GlobData *glob)
{
    SetAPen (glob->reqrp, glob->pens[BACKGROUNDPEN]);
    mySetWriteMask (glob->reqrp, glob->entrymask);
    RectFill (glob->reqrp, glob->boxleft, glob->boxtop - 1, glob->boxright,
			     glob->boxtop + glob->boxheight);
    mySetWriteMask (glob->reqrp, glob->rpmask);
}

/****************************************************************************************/

void REGARGS ScrollerMoved (GlobData *glob, int code)
{
    glob->buff->gotopos = code;
    if (!glob->buff->sorted)
    {
	glob->buff->pos = glob->buff->gotopos;
	UpdateDisplayList (glob);
	PrintFiles (glob);
	AdjustScroller (glob);
	glob->buff->sorted = TRUE;
    }
}

/****************************************************************************************/

static void REGARGS MarkHidden (GlobData *glob)
{
    struct ReqEntry 	*entry;
    int 		skipflags;
	
    if (glob->nodir) return;
    
    for (entry = (struct ReqEntry *)glob->firstentry->re_Next;
	 entry;
	 entry = (struct ReqEntry *)entry->re_Next)
    {
	entry->re_Flags &= ~(ENTRYF_HIDDEN|ENTRYF_GHOSTED);
	if ((skipflags = SkipEntry (glob, entry))) entry->re_Flags |= skipflags;
    }
}

/****************************************************************************************/

void REGARGS RethinkReqDisplay (GlobData *glob)
{
    int num;

    if (glob->nodir) return;
    
    MarkHidden (glob);
    num = (glob->buff->currentnum = CountAllDeselect (glob, FALSE))
																			    - glob->numentries;
    if (num < 0) glob->buff->gotopos = glob->buff->pos = 0;
    else if (glob->buff->pos > num) glob->buff->gotopos = glob->buff->pos = num;
    
    AdjustScroller (glob);
    UpdateDisplayList (glob);
    PrintFiles (glob);
}

/****************************************************************************************/

void REGARGS ClearDisplayList (GlobData *glob)
{
    memset (glob->displaylist, 0, 50*4);
}

/****************************************************************************************/

void REGARGS UpdateDisplayList (GlobData *glob)
{
    struct ReqEntry 	*entry;
    int 		i;

    i = 0;

    if (glob->buff->currentnum &&
	(entry = (struct ReqEntry *)glob->firstentry->re_Next))
    {
	while (i < glob->buff->pos)
	{
	    if (!(entry->re_Flags & ENTRYF_HIDDEN))
	    {
		i++;
	    }
	    
	    entry = (struct ReqEntry *)entry->re_Next;
	}

	
	for (i = 0; entry && (i < glob->numentries); 
	     entry = (struct ReqEntry *)entry->re_Next)
	{
	    if (!(entry->re_Flags & ENTRYF_HIDDEN))
	    {
		glob->displaylist[i++] = entry;
	    }
	}
    }
	
    while (i < glob->numentries)
    {
	glob->displaylist[i++] = NULL;
    }
}

/****************************************************************************************/

static UWORD GhostPattern[] = { 0x4444, 0x1111 };

/****************************************************************************************/

/* Apply a ghosting pattern to a given rectangle in a rastport */
static void REGARGS Ghost (struct RastPort *rp, UWORD pen, UWORD x, UWORD y, UWORD w, UWORD h)
{
    SetAPen (rp, pen);
    SetBPen (rp, pen);
    SetDrMd (rp, JAM1);
    SetAfPt (rp, GhostPattern, 1);
    RectFill (rp, x, y, x + w - 1, y + h - 1);
    SetAfPt (rp, NULL, 0);
}

/****************************************************************************************/

void REGARGS PrintEntry (GlobData *glob, int i)
{
    struct TextExtent 	extent;
    struct ReqEntry 	*entry;
    struct RastPort 	*reqrp = glob->reqrp;
    char 		sizestr[16], tempstr[108], *volname = NULL, *str;
    int 		apen = 0, bpen = 0, top, len = 0, sizelen = 0, sizelenpix = 0, type, rectpen, left, entrytop;
    LONG 		size;

    mySetWriteMask (glob->reqrp, glob->entrymask);
    rectpen = BACKGROUNDPEN;
    
    if ((entry = glob->displaylist[i]))
    {
	strcpy (tempstr, entry->re_Name);
	apen = TEXTPEN; bpen = BACKGROUNDPEN;
	sizestr[0] = 0;
	size = entry->re_Size;
	type = entry->re_Type;
	if (type == glob->directory_id)
	{
	    strcpy (sizestr, GetStr (glob->catalog, MSG_DRAWER));
	    apen = HIGHLIGHTTEXTPEN;
	}
	else if (type == FONT || (type == glob->file_id))
	{
#ifdef __AROS__
#warning AROS LocaleLibrary does not yet patch RawDoFmt. So "%lD" (uppercase D) does not work yet here!
	    Dofmt (sizestr, " %ld", &size);
#else
	    Dofmt (sizestr, LocaleBase ? " %lD" : " %ld", &size);
#endif
	    if (type == FONT)
	    {
		StrCat (tempstr, sizestr);
		sizestr[0] = 0;
	    }
	}
	else if (type == ASSIGN)
	{
	    strcpy (sizestr, GetStr (glob->catalog, MSG_ASSIGN));
	    apen = HIGHLIGHTTEXTPEN;
	}
	else if (type == VOLUME)
	{
	    if (size >= 0) Dofmt (sizestr, GetStr (glob->catalog, MSG_FULL), &size);
	}

	if (type == VOLUME && entry->re_VolLen)
	{
	     len = entry->re_VolLen;
	     volname = entry->re_Name + len + 1;
	}
	else
	    len = strlen (tempstr);

	sizelen = strlen (sizestr);
	/* Cache the pixel size for each entry so
		we only have to calculate this once. */
	if (entry->re_SizeLenPix)
	    sizelenpix = entry->re_SizeLenPix;
	else
	{
	    if (sizestr[0])
	        sizelenpix = StrWidth_noloc (&glob->itxt, sizestr);
	    else
	        sizelenpix = 0;
		
	    if (sizelenpix < 256) entry->re_SizeLenPix = sizelenpix;
	}

	if (entry->re_Flags & ENTRYF_SELECTED)
	{
	    bpen = FILLPEN;
	    if (type == glob->directory_id || type == ASSIGN)
	    {
		apen = BACKGROUNDPEN;
		bpen = HIGHLIGHTTEXTPEN;
	    }
	    else /* VOLUME, FILE, FONT */
		apen = FILLTEXTPEN;
		
	    rectpen = bpen;
	}
		
    } /* if ((entry = glob->displaylist[i])) */
    else
    {
	if (glob->lastdisplaylistnum == -1) glob->lastdisplaylistnum = i;
    }
    
    top = glob->boxtop + i * glob->entryheight;
    SetDrMd (glob->reqrp, JAM2);
    SetAPen (reqrp, glob->pens[rectpen]);
    entrytop = top;
    RectFill (reqrp, glob->boxleft, entrytop, glob->boxright, entrytop + glob->entryheight - 1);

    if (entry)
    {
	SetAPen (reqrp, glob->pens[apen]);
	SetBPen (reqrp, glob->pens[bpen]);
	top += glob->fontbase + 1;
	str = tempstr;
	left = glob->boxleft + 2;
	Move (reqrp, left, top);
	
	if (volname)
	{
	    Text (reqrp, volname, strlen (volname));
	    left += glob->maxvolwidth;
	    Move (reqrp, left, top);
	}
	
	/* Cache entry length, so we only need to calculate this once */
	if (!entry->re_EntryLen)
	{
	     len = TextFit (reqrp, str, len, &extent, NULL, 1,
						     glob->boxright - left - sizelenpix - 5, glob->entryheight);
	     entry->re_EntryLen = len;
	}
	else len = entry->re_EntryLen;
	
	Text (reqrp, str, len);
	
	if (sizelenpix)
	{
	     Move (reqrp, glob->boxright - sizelenpix - 1, top);
	     Text (reqrp, sizestr, sizelen);
	}
	
	if (entry->re_Flags & ENTRYF_GHOSTED)
	{
	    Ghost (reqrp, glob->pens[BACKGROUNDPEN],
			  glob->boxleft, entrytop,
			  glob->boxright - glob->boxleft + 1, glob->entryheight);
	}
	
    } /* if (entry) */
	    
    mySetWriteMask (glob->reqrp, glob->rpmask);
}

/****************************************************************************************/

void REGARGS PrintFiles (GlobData *glob)
{
    int i;

    glob->lastdisplaylistnum = -1;
    for (i = 0; i < glob->numentries; i++)
    {
	PrintEntry (glob, i);
    }
}

/****************************************************************************************/

/********************
* Requester entries *
********************/

/****************************************************************************************/

void REGARGS ClearAndInitReqBuffer (GlobData *glob)
{
    glob->buff->gotopos = glob->buff->pos = glob->buff->currentnum = glob->buff->numfiles = 0;
    glob->firstentry = (struct ReqEntry *)AllocVecPooled(glob->buff->pool, sizeof (struct ReqEntry));
    glob->buff->firstname = glob->firstentry;
}

/****************************************************************************************/

static UBYTE *REGARGS SkipDevName (UBYTE *entryname)
{
    while (*entryname != ' ') entryname++;
    while (*entryname == ' ') entryname++;

    return (entryname);
}

/****************************************************************************************/

struct ReqEntry *REGARGS FindEntry (struct BufferData *buff, char *name, int size,
				    int ctype, int *pos, ULONG flags)
{
    struct ReqEntry 	*curr, *lastname;
    int 		lastcmp, cmp, i = 0, cmptype;
    UBYTE 		*entryname;

    lastname = buff->firstname;
    curr = (struct ReqEntry *)lastname->re_Next;
    if ((ctype <= MAX_FILE_DIRECTORY) && buff->dirsmixed)
    {
	cmptype = 1;
    }
    else
    {
	while (curr && curr->re_Type < ctype)
	{
	    lastname = curr;
	    if (!(lastname->re_Flags & ENTRYF_HIDDEN)) i++;
	    curr = (struct ReqEntry *)curr->re_Next;
	}
	cmptype = ctype;
    }
    
    lastcmp = 1;
    while (curr)
    {
	if (ctype >= 0 && curr->re_Type > cmptype) break;
	entryname = curr->re_Name;
	
	if (curr->re_Type == VOLUME && !(flags & FIND_VOLUMENAME))
	{
	    cmp = Strnicmp (name, entryname, strlen(name));
	}
	else
	{
	    if (curr->re_Type == VOLUME && (flags & FIND_VOLUMENAME))
		    entryname = SkipDevName (entryname);
	    cmp = Stricmp (name, entryname);
	}
	
	if ((cmp < 0) || (!cmp && (size < curr->re_Size))) break;
	
	lastcmp = cmp;
	lastname = curr;
	
	if (!(lastname->re_Flags & ENTRYF_HIDDEN)) i++;
	
	curr = (struct ReqEntry *)curr->re_Next;
    }
    
    if (pos)
    {
	if (lastname && (!(flags & FIND_EXACT) || !lastcmp))
	{
	    *pos = i - 1;
	    return (lastname);
	}
	else 
	{
	    *pos = 0;
	    return (NULL);
	}
    }
    
    return (lastname);
}

/****************************************************************************************/

struct ReqEntry *REGARGS AddEntry (GlobData *glob, struct BufferData *buff,
				char *name, int size, int ctype)
{
    struct ReqEntry	*lastname, *newentry;
    int			pos, len = strlen( name ) + 1, currnum, skipflags;
    STRPTR		str, findname = name;

    if( ctype == VOLUME )
    {
	findname = SkipDevName( findname );
    }

    lastname = FindEntry( buff, findname, ( ( ctype == FONT ) ? size : MAXINT) ,
	    		  ctype, &pos, FIND_VOLUMENAME );

    str = lastname->re_Name;

    if( str && !Stricmp( name, str ) )
    {
	if( ctype <= MAX_FILE_DIRECTORY )
	{
	    lastname->re_Size = size;
	    return( lastname );
	}

	len = 0;
    }

    if( !( newentry = ( struct ReqEntry * ) AllocVecPooled(buff->pool, sizeof( struct ReqEntry ) + len ) ) )
    {
	return( NULL );
    }

    if( len )
    {
	str = ( STRPTR ) newentry + sizeof( struct ReqEntry );
	strcpy( str, name );
    }

    newentry->re_Name = str;
    newentry->re_Size = size;
    newentry->re_Type = ctype;
    newentry->re_Next = lastname->re_Next;
    lastname->re_Next = ( struct Node * ) newentry;

    buff->numfiles++;
    buff->sorted = FALSE;

    if( glob )
    {
	skipflags = SkipEntry( glob, newentry );
	newentry->re_Flags |= skipflags;

	if( skipflags != ENTRYF_HIDDEN )
	{
	    /* If file requester keep activated itempos up to date */
	    if( ctype <= MAX_FILE_DIRECTORY )
	    {
		if( pos < glob->selectedpos )
		{
		    glob->selectedpos++;
		}
	    }

	    currnum = buff->currentnum;
	    buff->currentnum++;

	    if( ctype <= MAX_FILE_DIRECTORY )
	    {	/* display now ? */
		if (currnum < glob->numentries)
		{
		    glob->displaylist[currnum] = newentry;

		    if( !glob->quiet )
		    {
			PrintEntry( glob, currnum );
		    }
		}
		else if( !glob->quiet )
		{
		    AdjustScroller( glob );
		}
	    }
	}
    }

    return( newentry );
}

/****************************************************************************************/

int REGARGS EndsInDotInfo (char *str, int len)
{
    if (len >= 5 && !Stricmp (&str[len-5], DOTINFOSTR)) return (TRUE);
    return (FALSE);
}

/****************************************************************************************/

static int REGARGS SkipEntry (GlobData *glob, struct ReqEntry *entry)
{
    char *str = entry->re_Name;
    int  len = strlen (str);

    if (glob->reqtype == RT_FILEREQ)
    {
	if (entry->re_Type == glob->file_id)
	{
	    if (!glob->wilddotinfo)
	    {
		if (EndsInDotInfo (str, len))
		{
		    if (glob->freq->hideinfo) return (ENTRYF_HIDDEN);
		    
		    if (glob->matchpat[0])
		    {
			int ret;

			str[len-5] = '\0';
			ret = !MatchPatternNoCase (glob->matchpat, str);
			str[len-5] = '.';
			
			return (ret ? ENTRYF_HIDDEN : 0);
		    }
		}
	    }
	    
	    if (glob->flags & FREQF_NOFILES) return (ENTRYF_GHOSTED);
	    
	    if (glob->matchpat[0])
		if (!MatchPatternNoCase (glob->matchpat, str))
		    return (ENTRYF_HIDDEN);
	}
    }
    else if (glob->reqtype == RT_FONTREQ)
    {
	    if (entry->re_Size < glob->minsize || entry->re_Size > glob->maxsize ||
		((glob->flags & FREQF_FIXEDWIDTH) && (entry->re_Flags & FPF_PROPORTIONAL)) ||
		(!(glob->flags & FREQF_COLORFONTS) && (entry->re_Style & FSF_COLORFONT)))
	 	return (ENTRYF_HIDDEN);
    }
    else
    {
	;
    }
    
    return (0);
}

/****************************************************************************************/

int REGARGS CountAllDeselect (GlobData *glob, int dirsonly)
{
    struct ReqEntry 	*entry;
    int 		count = 0;

    for (entry = (struct ReqEntry *)glob->firstentry->re_Next;
	 entry;
	 entry = (struct ReqEntry *)entry->re_Next)
    {
	if (!(entry->re_Flags & ENTRYF_HIDDEN)) count++;
	if (!dirsonly || entry->re_Type != glob->file_id)
	{
	    if (entry->re_Flags & ENTRYF_SELECTED)
	    {
		glob->numselected--;
		entry->re_Flags &= ~ENTRYF_SELECTED;
	    }
	}
    }
    
    glob->selectedpos = -1;
    
    return (count);
}

/****************************************************************************************/

void REGARGS SelectAll (GlobData *glob, char *pattern)
{
    struct ReqEntry 	*entry;
    char 		*selpat;
    LONG    	    	pattern_len;
    
    pattern_len = strlen(pattern) * 2 + 2;
    selpat = AllocVec(pattern_len, MEMF_PUBLIC);
    if (selpat)
    {
	ParsePatternNoCase (pattern, selpat, pattern_len);

	for (entry = (struct ReqEntry *)glob->firstentry->re_Next;
	     entry;
	     entry = (struct ReqEntry *)entry->re_Next)
	{ 
	    if (!(entry->re_Flags & (ENTRYF_HIDDEN|ENTRYF_GHOSTED)))
	    {
		if ((entry->re_Type == glob->file_id) ||
	            (entry->re_Type == glob->directory_id && (glob->flags & FREQF_SELECTDIRS)))
		{
		    if (MatchPatternNoCase (selpat, entry->re_Name))
		    {
			if (!(entry->re_Flags & ENTRYF_SELECTED)) glob->numselected++;
			entry->re_Flags |= ENTRYF_SELECTED;
		    }
		}
	    }
	}
	FreeVec(selpat);
    }
    
    UpdateNumSelGad (glob);
    PrintFiles (glob);
    my_SetStringGadget (glob->reqwin, glob->filegad, NULL);
}

/****************************************************************************************/

/*****************
* File requester *
*****************/

/****************************************************************************************/

void REGARGS UnLockReqLock (GlobData *glob)
{
    glob->ledon = FALSE;
    RenderLED (glob);
    if (glob->lock) UnLock (glob->lock);
    glob->lock = NULL;
}

/****************************************************************************************/

BOOL REGARGS FindVolume (GlobData *glob, UBYTE *str, struct ReqEntry *curr)
{
    struct ReqEntry 	*entry;
    UBYTE 		*s;

    for (entry = (struct ReqEntry *)glob->firstentry->re_Next;
	 entry;
	 entry = (struct ReqEntry *)entry->re_Next)
    {
	if (entry != curr && entry->re_Type == VOLUME)
	{
	    s = entry->re_Name;
	    while (*s != ' ') s++;
	    while (*s == ' ') s++;
	    if (!Stricmp (s, str)) return (TRUE);
	}
    }
    
    return (FALSE);
}

/****************************************************************************************/

static BOOL
IsDosVolume(struct DosList *dlist)
{
#ifdef __AROS__
    struct InfoData id;
    BPTR            lock;
    STRPTR          volName = AllocVec(strlen(dlist->dol_DevName) + 2,
				       MEMF_ANY);

    /* Create a colon ended volume name, lock it and call Info() */

    if (volName == NULL)
    {
	return FALSE;
    }

    strcpy(volName, dlist->dol_DevName);
    strcat(volName, ":");

    lock = Lock(volName, SHARED_LOCK);

    if (lock != NULL)
    {
	BOOL result = Info(lock, &id);

	UnLock(lock);
	FreeVec(volName);

	return result;
    } 
    else
    {
	FreeVec(volName);

	return FALSE;
    }

#else

    BOOL	isdev = FALSE;
    D_S( struct InfoData,id );

    if( !dlist->dol_Task || DoPkt1( dlist->dol_Task, ACTION_DISK_INFO, MKBADDR( id ) ) )
    {
	isdev = TRUE;
    }

    return (isdev);
#endif

}

/****************************************************************************************/

static BOOL
IsDosDevice( struct DosList *dlist )
{
    struct FileSysStartupMsg *fssm;
    struct DosEnvec *de;
    BOOL isdev = FALSE;

    /* If it`s a device, do the hard check for a filesystem */

    if (dlist->dol_Type == DLT_DEVICE)
    {
	fssm = ( struct FileSysStartupMsg * )BADDR( dlist->dol_misc.dol_handler.dol_Startup );

	if( fssm )
	{
	    if( TypeOfMem( fssm ) && TypeOfMem( BADDR( fssm->fssm_Device ) )
		    && TypeOfMem( BADDR( fssm->fssm_Environ ) ) )
	    {
		if( *( ( STRPTR ) BADDR( fssm->fssm_Device ) ) != 255 )
		{
		    de = ( struct DosEnvec * ) BADDR( fssm->fssm_Environ );

		    if( de && TypeOfMem( de ) )
		    {
			if( de->de_Surfaces && de->de_BlocksPerTrack )
			{
			    isdev = TRUE;
			}
		    }
		}
	    }
	}
#ifdef TASK_DEVICE
	else
	{
	    isdev = ( dlist->dol_Task != NULL );
	}
#endif
    }

    return( isdev );
}

/****************************************************************************************/

struct DeviceEntry
{
    struct DeviceEntry	*next;
    struct MsgPort	*task;
    BOOL		resolved;
    UBYTE		name[ 42 ];
};

/****************************************************************************************/

#define SIZE_NONE	( ( ULONG ) -1 )
#define SIZE_CALCULATE	( ( ULONG ) -2 )

/****************************************************************************************/

static void
AddDisk(
    GlobData *glob,
    struct DeviceEntry *deventry,
    UBYTE *devname,
    ULONG size,
    ULONG volreqflags )
{
    struct rtVolumeEntry	volentry;
    struct ReqEntry		*entry;
    UBYTE			buffer[80];
    int 			i, addentry;
    
    D_S( struct InfoData, infodata );

    addentry = TRUE;

    if( volreqflags && glob->filterhook )
    {
	volentry.Type = DLT_DEVICE;
	volentry.Name = devname;
	addentry = CallHookPkt( glob->filterhook, glob->freq, &volentry );
    }

    if( addentry )
    {
	if( size == SIZE_CALCULATE )
	{
	    size = SIZE_NONE;

#ifdef __AROS__
	    {
		BPTR lock = Lock((CONST_STRPTR) &deventry->name, SHARED_LOCK);

		if (lock != NULL)
		{
		    if (Info(lock, infodata))
		    {
			size = ( infodata->id_NumBlocksUsed * 100 +
				 infodata->id_NumBlocks / 2 ) /
			    infodata->id_NumBlocks;
		    }
		    UnLock(lock);
		}
	    }
#else
	    if( deventry->task &&
		    DoPkt1( deventry->task, ACTION_DISK_INFO, MKBADDR( infodata ) ) )
	    {
		size = ( infodata->id_NumBlocksUsed * 100 +
			 infodata->id_NumBlocks / 2 ) /
		    infodata->id_NumBlocks;
	    }
#endif
	}

	i = StrWidth_noloc( &glob->itxt, deventry->name ) + StrWidth_noloc( &glob->itxt, "W" );

	if( i > glob->maxvolwidth )
	{
	    glob->maxvolwidth = i;
	}

	DofmtArgs( buffer, "%s %s", devname, deventry->name );

	if( ( entry = AddEntry( glob, glob->buff, buffer, size, VOLUME ) ) )
	{
	    entry->re_VolLen = strlen( devname );
	}
    }
}

/****************************************************************************************/

static struct DeviceEntry *
AllocDevEntry( GlobData *glob, struct DosList *dlist, STRPTR name )
{
    struct DeviceEntry	*deventry;

    if( ( deventry = ( struct DeviceEntry * ) AllocVecPooled(
	  glob->buff->pool, sizeof( struct DeviceEntry ) ) ) )
    {
	strcpy( deventry->name, name );
#ifdef __AROS__
#warning FIXME: dlist->dol_Task does not exist in AROS. For now assuming 0 here.
	deventry->task = NULL;
#else
	deventry->task = dlist->dol_Task;
#endif
	deventry->resolved = FALSE;
    }

    return( deventry );
}

/****************************************************************************************/

static void
GetVolName( BSTR bstr, STRPTR cstr )
{
#ifdef __AROS__
    /* In AROS, BPTR:s are handled differently on different systems
       (to be binary compatible on Amiga) */

    if (bstr != NULL)
    {
	LONG    length = AROS_BSTR_strlen(bstr);
	LONG    i;		/* Loop variable */

	for (i = 0; i < length; i++)
	{
	    cstr[i] = AROS_BSTR_getchar(bstr, i);
	}
	
	cstr[i++] = ':';
	cstr[i] = 0;

	D(bug("Found volume %s\n", cstr));
    }
#else
    STRPTR str;

    *cstr = 0;

    if( ( str = BADDR( bstr ) ) )
    {
	LONG i;

	for( i = *str++; i--; )
	{
	    *cstr++ = *str++;
	}

	*cstr++ = ':';
	*cstr = 0;
    }
#endif

}

/****************************************************************************************/

void REGARGS
AddDiskNames( GlobData *glob, ULONG volreqflags )
{
    struct ReqEntry		*entry, *temp;
    struct DosList		*dlist;
    struct DeviceEntry		*deventry, *lastdeventry = NULL;
    struct rtVolumeEntry	volentry;
    UBYTE			/* *bstr, *cstr, */ name[ 42 ], devname[ 36 ];
    /* WORD			i; */

    *glob->winaddr = ( APTR ) -1;
    glob->maxvolwidth = 0;
    dlist = LockDosList( LDF_VOLUMES | LDF_ASSIGNS | LDF_READ );

    /* Add PROGDIR: assign is possible, just like reqtools_patch.lha
       from Aminet (by Mikolaj Calusinski) does. */
       
    if (!(volreqflags & VREQF_NOASSIGNS))
    {
    	if (((struct Process *)FindTask(NULL))->pr_HomeDir)
	{
    	    AddEntry(glob, glob->buff, "PROGDIR:", -1, ASSIGN);
	}
    }
    
    while( ( dlist = NextDosEntry( dlist, LDF_VOLUMES | LDF_ASSIGNS | LDF_READ ) ) )
    {
#ifdef __AROS__
	GetVolName(dlist->dol_OldName, name);
#else
	GetVolName(dlist->dol_Name, name);
#endif
	if (dlist->dol_Type == DLT_VOLUME)
	{
	    if (IsDosVolume(dlist) && !(volreqflags & VREQF_NODISKS))
	    {
		if ((deventry = AllocDevEntry(glob, dlist, name)))
		{
		    deventry->next = lastdeventry;
		    lastdeventry = deventry;
		}
	    }
	}
	else if (dlist->dol_Type == DLT_DIRECTORY ||
		 dlist->dol_Type == DLT_NONBINDING)
	{
	    if (!(volreqflags & VREQF_NOASSIGNS))
	    {
		AddEntry(glob, glob->buff, name, -1, ASSIGN);
	    }
	}
    }

    /* Append device names to volumes */
    dlist = LockDosList( LDF_DEVICES | LDF_READ );

    while( ( dlist = NextDosEntry( dlist, LDF_DEVICES | LDF_READ ) ) )
    {
	BOOL	devmatch;

	deventry = lastdeventry;
	devmatch = FALSE;

	while( deventry )
	{
#ifdef __AROS__
#warning FIXME: AROS has no dol_Task!
#else
	    devmatch |= ( deventry->task == dlist->dol_Task );
#endif
#ifdef __AROS__
#warning FIXME: AROS again has no dol_Task!
	    if( !deventry->resolved && deventry->task && devmatch )
#else
	    if( !deventry->resolved && deventry->task && ( deventry->task == dlist->dol_Task ) )
#endif
	    {
#ifdef __AROS__
		GetVolName( dlist->dol_OldName, devname );
#else
		GetVolName( dlist->dol_Name, devname );
#endif
		AddDisk( glob, deventry, devname, SIZE_CALCULATE, volreqflags );
		deventry->resolved = TRUE;
	    }

	    deventry = deventry->next;
	}

	if( !devmatch && ( volreqflags & VREQF_ALLDISKS ) &&
		!( volreqflags & VREQF_NODISKS ) && IsDosDevice( dlist ) )
	{
	    /* Device seems to be a DOS disk device, and it didn't belong
	     * to a volume, and we should really show all devices in a
	     * volume requester.
	     */
#ifdef __AROS__
	    GetVolName( dlist->dol_OldName, devname );
#else
	    GetVolName( dlist->dol_Name, devname );
#endif

	    if( ( deventry = AllocDevEntry( glob, dlist, "-" ) ) )
	    {
		deventry->next = lastdeventry;
		lastdeventry = deventry;
		AddDisk( glob, deventry, devname, SIZE_NONE, volreqflags );
		deventry->resolved = TRUE;
	    }
	}
    }

    UnLockDosList( LDF_DEVICES | LDF_READ );
    UnLockDosList( LDF_VOLUMES | LDF_ASSIGNS | LDF_READ );

    /* Free temp volume list (and add remaining names if ALLDISKS is on) */
    while( ( deventry = lastdeventry ) )
    {
	if( !deventry->resolved /* && ( !volreqflags || ( volreqflags & VREQF_ALLDISKS ) ) */ )
	{
	    AddDisk( glob, deventry, "-", SIZE_CALCULATE, volreqflags );
	}

	lastdeventry = deventry->next;
	FreeVecPooled( glob->buff->pool, deventry );
    }

    /* If volume requester filter assigns */
    if( volreqflags && glob->filterhook )
    {
	entry = glob->buff->firstname;

	while( ( temp = entry ) )
	{
	    entry = ( struct ReqEntry * ) entry->re_Next;

	    if( !entry )
	    {
		break;
	    }

	    if( entry->re_Type == ASSIGN )
	    {
		volentry.Type = DLT_DIRECTORY;
		volentry.Name = entry->re_Name;

		if( !CallHookPkt( glob->filterhook, glob->freq, &volentry ) )
		{
		    temp->re_Next = entry->re_Next;
		    FreeVecPooled( glob->buff->pool, entry );
		    entry = temp;
		    glob->buff->currentnum--;
		}
	    }
	}
    }

    *glob->winaddr = glob->reqwin;
}

/****************************************************************************************/

BOOL REGARGS FindCurrentPos (GlobData *glob, char *name, int size, int ctype)
{
    BOOL success;

    MarkHidden (glob);
    success = FindEntry (glob->buff, name, size, ctype, ( int * ) &glob->buff->pos, FIND_EXACT) ? TRUE : FALSE;
    glob->buff->gotopos = glob->buff->pos;
    
    return (success);
}

/****************************************************************************************/

void REGARGS NewDir (GlobData *glob)
{
    UnLockReqLock (glob);
    SetWindowTitles (glob->reqwin, glob->title, (char *)~0);
    FreeReqBuffer (glob->req);
    ClearDisplayList (glob);
    glob->newdir = TRUE;
    glob->clicked = ~0;
    
    if (glob->freq)
    {
	SetFileDirMode (glob->buff, glob->flags);
	glob->file_id = glob->buff->file_id;
	glob->directory_id = glob->buff->directory_id;
    }
}

/****************************************************************************************/

void REGARGS ShowDisks(GlobData *glob)
{

    if (!glob->disks)
    {
	UnLockReqLock (glob);
	FreeReqBuffer (glob->req);
	ClearAndInitReqBuffer (glob);
	glob->exnext = FALSE;
	glob->disks = TRUE;
	AddDiskNames (glob, glob->volumerequest);
	AdjustScroller (glob);
	UpdateDisplayList (glob);
	PrintFiles (glob);
	glob->buff->sorted = TRUE;
	glob->selectedpos = -1;
    }
}

/****************************************************************************************/

void REGARGS UpdateNumSelGad (GlobData *glob)
{
    int  top;
    char str[6];

    if (glob->numselectedgad)
    {
	Dofmt (str, "%4ld", &glob->numselected);
	
	SetAPen (glob->reqrp, glob->pens[TEXTPEN]);
	SetBPen (glob->reqrp, glob->pens[BACKGROUNDPEN]);
	SetDrMd (glob->reqrp, JAM2);
	
	top = glob->numselectedgad->TopEdge + 3;
	
	Move (glob->reqrp, glob->numselectedgad->LeftEdge + glob->numselectedoff + 8,
			top + glob->reqfont->tf_Baseline);
	Text (glob->reqrp, str, 4);
	
	SetDrMd (glob->reqrp, JAM2);
	SetAPen (glob->reqrp, glob->pens[BACKGROUNDPEN]);
	
	RectFill (glob->reqrp, glob->reqrp->cp_x,
			       top,
			       glob->numselectedgad->LeftEdge + glob->numselectedgad->Width - 3,
			       top + glob->fontheight - 1);
    }
}

/****************************************************************************************/

/*****************
* Font requester *
*****************/

/****************************************************************************************/

static struct Region *MyInstallRegion (struct Window *win, struct Region *reg, int refresh)
{
    struct Region *oldregion;

    LockLayerInfo(&win->WScreen->LayerInfo);
    
    if (refresh) GT_EndRefresh (win, FALSE);
    
    oldregion = InstallClipRegion (win->WLayer, reg);
    if (refresh) GT_BeginRefresh (win);

    UnlockLayerInfo(&win->WScreen->LayerInfo);
    
    return (oldregion);
}

/****************************************************************************************/

void REGARGS ShowFontSample (GlobData *glob, int refresh, int dowait)
{
    struct DiskfontBase *DiskfontBase = glob->diskfontbase;
    struct TextFont 	*font;
    struct Rectangle 	rect;
    struct Region 	*oldregion, *region;
    char 		*message = "0123 aAbBcCdDeEfFgGhHiIjJkKlLmMnNoOpPqQrRsStTuUvVwWxXyYzZ";
    UWORD 		*cmap = NULL;
    ULONG 		style = glob->fontstyle, count = glob->colcount;
    APTR 		winlock = NULL;

    if (dowait) winlock = rtLockWindow (glob->reqwin);
    
    if (glob->flags & FREQF_SCALE)
	glob->fontreq->Attr.ta_Flags &= ~FPF_DESIGNED;
	
    if (!(font = OpenDiskFont (&glob->fontreq->Attr)))
    {
	font = glob->reqfont;
	message = GetStr (glob->catalog, MSG_COULDNT_OPEN_FONT);
	style = 0;
    }
    
    rect.MinX = glob->fontdisplayleft;
    rect.MaxX = glob->fontdisplayright;
    rect.MinY = glob->fontdisplaytop;
    rect.MaxY = glob->fontdisplaytop + glob->sampleheight - 1;

    if ( ( region = NewRegion() ) )
    {
	OrRectRegion (region, &rect);

	oldregion = MyInstallRegion (glob->reqwin, region, refresh);

	SetFont (glob->reqrp, font);
	SetSoftStyle (glob->reqrp, style, ~0);
	SetAPen (glob->reqrp, glob->pens[TEXTPEN]);
	SetBPen (glob->reqrp, glob->pens[BACKGROUNDPEN]);
	
	if (font->tf_Style & FSF_COLORFONT)
	{
	    struct ColorFontColors *cfc;

	    cfc = ((struct ColorTextFont *)font)->ctf_ColorFontColors;
	    if (cfc)
	    {
	    	count = cfc->cfc_Count;
	    	cmap = cfc->cfc_ColorTable;
	    }
	}
	
	if (glob->flags & FREQF_CHANGEPALETTE)
	{
	    if (cmap) LoadRGB4 (glob->vp, cmap, count);
	    else LoadCMap (glob->vp, glob->colormap);
	}
	
	if (!refresh) SetRast (glob->reqrp, glob->pens[BACKGROUNDPEN]);
	
	Move (glob->reqrp, glob->fontdisplayleft,
			   glob->fontdisplaytop + (glob->sampleheight - font->tf_YSize) / 2
						+ font->tf_Baseline);
	Text (glob->reqrp, message, strlen(message));

	MyInstallRegion (glob->reqwin, oldregion, refresh);
	DisposeRegion (region);

	SetFont (glob->reqrp, glob->reqfont);
	SetSoftStyle (glob->reqrp, 0, ~0);
    }

/*endshowsample:*/
    CloseFont (font);
    if (dowait) rtUnlockWindow (glob->reqwin, winlock);
}

/****************************************************************************************/

/***********************
* ScreenMode requester *
***********************/

/****************************************************************************************/

ULONG ASM myGetDisplayInfoData (
    OPT_REGPARAM(a1, UBYTE *, buf),
    OPT_REGPARAM(d0, unsigned long, size),
    OPT_REGPARAM(d1, unsigned long, tagID),
    OPT_REGPARAM(d2, unsigned long, displayID))
{
    return (GetDisplayInfoData (FindDisplayInfo (displayID),
		     		buf, size, tagID, displayID));
}

/****************************************************************************************/

int REGARGS GetModeData (GlobData *glob, ULONG id, int *mon)
{
    struct MonitorInfo 	monitorinfo;
    ULONG 		propflags;
    int 		modewidth, stdmode = TRUE;
    char 		*str, *monstr;

    if ((myGetDisplayInfoData ((UBYTE *)&glob->diminfo,
    				sizeof (struct DimensionInfo), DTAG_DIMS, id) <= 0) ||
	(myGetDisplayInfoData ((UBYTE *)&glob->dispinfo,
				sizeof (struct DisplayInfo), DTAG_DISP, id) <= 0))
    {
        return (FALSE);
    }

    modewidth = glob->diminfo.Nominal.MaxX - glob->diminfo.Nominal.MinX + 1;
    propflags = glob->dispinfo.PropertyFlags;

    if ((glob->flags & SCREQF_GUIMODES) && !(propflags & DIPF_IS_WB))
	return (FALSE);

    *mon = ((id & MONITOR_ID_MASK) >> 16) - 1;

    if (myGetDisplayInfoData ((UBYTE *)&glob->nameinfo,
	      		      sizeof (struct NameInfo), DTAG_NAME, id) <= 0)
    {
	str = glob->nameinfo.Name;
	if ((myGetDisplayInfoData ((UBYTE *)&monitorinfo, sizeof (struct MonitorInfo), DTAG_MNTR, id) > 0)
	    && monitorinfo.Mspc)
	{
	    monstr = monitorinfo.Mspc->ms_Node.xln_Name;
	    while (*monstr > '.') *str++ = ToUpper (*monstr++);
	    *str++ = ':';
	}
	
	DofmtArgs (str, "%ld x %ld", modewidth,
			             glob->diminfo.Nominal.MaxY - glob->diminfo.Nominal.MinY + 1);
				     
	if (propflags & DIPF_IS_HAM)
	{
	    StrCat (str, GetStr (glob->catalog, MSG_DASH_HAM));
	    stdmode = FALSE;
	}
	
	if (propflags & DIPF_IS_EXTRAHALFBRITE)
	{
	    StrCat (str, GetStr (glob->catalog, MSG_DASH_EHB));
	    stdmode = FALSE;
	}
	
	if (propflags & DIPF_IS_LACE)
	    StrCat (str, GetStr (glob->catalog, MSG_DASH_INTERLACED));
 
    }

    /* check property flags using mask */
    if ((propflags & glob->propertymask) != (glob->propertyflags & glob->propertymask))
	return (FALSE);

    /* check if depth larger than minimum depth */
    if (glob->diminfo.MaxDepth < glob->mindepth) return (FALSE);

    /* GUI modes ? */
    if (glob->flags & SCREQF_GUIMODES)
    {
	return (stdmode && (modewidth >= 640));
    }

    /* include non-standard modes ? */
    if (!(glob->flags & SCREQF_NONSTDMODES)) return (stdmode);

    return (TRUE);
}

/****************************************************************************************/

void REGARGS SetTextGad (GlobData *glob, struct Gadget *gad, char *text)
{
    if (!gad) return;
    
    myGT_SetGadgetAttrs (gad, glob->reqwin, NULL, GTTX_Text, (IPTR) text,
    						  TAG_END);
}

/****************************************************************************************/

void REGARGS SetSizeGads (GlobData *glob)
{
    if (glob->usedefwidth) glob->width = glob->defwidth;
    if (glob->usedefheight) glob->height = glob->defheight;
    if (glob->width > glob->maxwidth) glob->width = glob->maxwidth;
    if (glob->width < glob->minwidth) glob->width = glob->minwidth;
    
    my_SetIntegerGadget (glob->reqwin, glob->widthgad, glob->width);
    
    if (glob->height > glob->maxheight) glob->height = glob->maxheight;
    if (glob->height < glob->minheight) glob->height = glob->minheight;
    
    my_SetIntegerGadget (glob->reqwin, glob->heightgad, glob->height);
}

/****************************************************************************************/

LONG REGARGS IntGadgetBounds (GlobData *glob, struct Gadget *gad,
						  LONG min, LONG max)
{
    LONG val;

    val = ((struct StringInfo *)gad->SpecialInfo)->LongInt;
    
    if (val > max)
    {
	val = max;
	my_SetIntegerGadget (glob->reqwin, gad, val);
    }
    
    if (val < min)
    {
	val = min;
	my_SetIntegerGadget (glob->reqwin, gad, val);
    }
    
    return (val);
}

/****************************************************************************************/

int overscantrans[] = { 0, 3, 4, 1 };

/****************************************************************************************/

void REGARGS GetModeDimensions (GlobData *glob)
{
    struct Rectangle *rect;

    if (glob->modeid != INVALID_ID)
    {
	rect = &(&glob->diminfo.Nominal)[overscantrans[glob->overscantype]];
	glob->defwidth = rect->MaxX - rect->MinX + 1;
	glob->defheight = rect->MaxY - rect->MinY + 1;
    }
    else glob->defwidth = glob->defheight = 0;
}

/****************************************************************************************/

void BuildColStr (char *colstr, LONG colors, ULONG id)
{
    colors = 1 << colors;

    if (id & HAM)
	colors = (colors == 128 ? 4096 : 16777216L);

    if (id & EXTRA_HALFBRITE )
	colors = 64;

    if (colors <= 9999)
    {
	DofmtArgs (colstr, "%ld", colors);
	return;
    }
    
    colors /= 1024;
    if (colors <= 999)
    {
	DofmtArgs (colstr, "%ldK", colors);
	return;
    }
    
    colors /= 1024;
    
    DofmtArgs (colstr, "%ldM", colors);
}

/****************************************************************************************/

void REGARGS UpdateDepthDisplay (GlobData *glob, int depth, ULONG id)
{
    BuildColStr (glob->currcolstr, depth, id);
    
    myGT_SetGadgetAttrs (glob->currcolgad, glob->reqwin, NULL, GTTX_Text, (IPTR) glob->currcolstr,
    							       TAG_END);
}

/****************************************************************************************/

void REGARGS UpdateDepthGad (GlobData *glob)
{
    UpdateDepthDisplay (glob, glob->depth, glob->modeid);
    
    myGT_SetGadgetAttrs (glob->depthgad, glob->reqwin, NULL, GTSL_Min, glob->currmindepth,
    						             GTSL_Max, glob->currmaxdepth,
							     GTSL_Level, glob->depth,
							     GA_Disabled, (glob->currmindepth == glob->currmaxdepth),
							     TAG_END);
}

/****************************************************************************************/

void REGARGS DisplayModeAttrs (GlobData *glob)
{
    if (glob->modeid != INVALID_ID)
    {
	int maxdepth = glob->diminfo.MaxDepth, mindepth = 1;

	if( maxdepth < 0 || maxdepth > 24 )
	{
	    maxdepth = 24;
	}

	if (glob->depth > maxdepth || !(glob->flags & SCREQF_DEPTHGAD))
	    glob->depth = maxdepth;

	/* if (glob->modeid & HAM) mindepth = maxdepth = glob->depth = 12; */
	/* if (glob->modeid & EXTRA_HALFBRITE) mindepth = maxdepth = glob->depth = 6; */
	
	if (glob->depthgad)
	{
	    if (glob->modeid & EXTRA_HALFBRITE)
	    {
		mindepth = maxdepth;
	    }

	    if (glob->modeid & HAM)
	    {
		mindepth = 7;

		if (maxdepth == 6)
		    ++maxdepth;
	    }

	    glob->currmaxdepth = maxdepth; glob->currmindepth = mindepth;
	    
	    if (glob->currmaxdepth > glob->maxdepth) glob->currmaxdepth = glob->maxdepth;
	    if (glob->currmindepth < glob->mindepth) glob->currmindepth = glob->mindepth;
	    if (glob->depth > glob->currmaxdepth) glob->depth = glob->currmaxdepth;
	    if (glob->depth < glob->currmindepth) glob->depth = glob->currmindepth;
	    
	    UpdateDepthGad (glob);
	    BuildColStr (glob->maxcolstr, glob->currmaxdepth, glob->modeid);
	    myGT_SetGadgetAttrs (glob->maxcolgad, glob->reqwin, NULL,
			    GTTX_Text, (IPTR) glob->maxcolstr, TAG_END);
	}
	SetSizeGads (glob);
	
    } /* if (glob->modeid != INVALID_ID) */
}

/****************************************************************************************/
