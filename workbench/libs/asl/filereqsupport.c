/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <dos/datetime.h>
#include <dos/filehandler.h>
#include <intuition/gadgetclass.h>
#include <stdio.h>
#include <string.h>
#include <clib/macros.h>

#include "asl_intern.h"
#include "filereqsupport.h"
#include "filereqhooks.h"
#include "specialreq.h"
#include "layout.h"

#define CATCOMP_NUMBERS
#include "strings.h"

#define SDEBUG 0
#define DEBUG 0
//#define ADEBUG 0

#include <aros/debug.h>

/*****************************************************************************************/

static WORD FRCompareDirectoryNodes(struct IntFileReq *ifreq, struct ASLLVFileReqNode *node1,
				    struct ASLLVFileReqNode *node2, struct AslBase_intern *AslBase);


/*****************************************************************************************/

static void fixpath(char *pathstring)
{
    char *start;
    WORD i, len;
    
    /* tricky slash stuff:
    
       work:pictures/     --> work:pictures
       work:pictures//    --> work:
       work:pictures///   --> work:/
       work:pictures////  --> work://
       /pics/		  --> /pics
       /pics//		  --> /
       /pics///		  --> //
       /pics//pics        --> /pics
       /pics//pics/	  --> /pics
       /pics/scans////    --> //

    */

    start = strrchr(pathstring,':');
    if (start)
    {
        start++;
    } else {
        start = pathstring;
    }
            
    for(; (i = len = strlen(start)); )
    {
        char *sp = start;
	
	/* skip leading slashes */
	
	while(i && (*sp == '/'))
	{
	    i--;
	    sp++;
	}
	
	if (i)
	{
            char *sp2;

	    if((sp2 = strstr(sp, "//")))
	    {
		char *sp3 = sp2;

		while(sp3 > sp)
		{
	            char c = sp3[-1];

		    if (c == '/') break;
		    sp3--;
		}

		memmove(sp3, sp2 + 2, strlen(sp2 + 2) + 1);

		continue;
	    }

	} /* if path is something more than just leading slashes */
	
	break;
	
    } /* for(; (i = len = strlen(start)); ) */
    
    /* kill trailing slash */
    
    i = strlen(start);
    if (i >= 2)
    {
        if ((start[i - 1] == '/') &&
	    (start[i - 2] != '/'))
	{
	    start[--i] = '\0';
	}
    }
}

/*****************************************************************************************/

void FRRefreshListview(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct TagItem 	set_tags[] =
    {
	{ASLLV_Labels, (IPTR)&udata->ListviewList	},
	{TAG_DONE					}
    };

    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
}

/*****************************************************************************************/

void FRFreeListviewNode(struct ASLLVFileReqNode *node, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    WORD i;

    for(i = 0; i < ASLLV_MAXCOLUMNS; i++)
    {
	if (node->text[i] && !(node->dontfreetext & (1 << i)))
	{
	    FreePooled(ld->ld_IntReq->ir_MemPool, node->text[i], strlen(node->text[i]) + 1);
	}
    }

#ifdef __MORPHOS__
    if (node->type == ASLLV_FRNTYPE_VOLUMES)
	FreePooled(ld->ld_IntReq->ir_MemPool, node->node.ln_Name, strlen(node->node.ln_Name) + 1);
#endif

    FreePooled(ld->ld_IntReq->ir_MemPool, node, sizeof(struct ASLLVFileReqNode));
} 

/*****************************************************************************************/

void FRFreeListviewList(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 		*udata = (struct FRUserData *)ld->ld_UserData;	
    struct ASLLVFileReqNode 	*node, *succ;
    struct TagItem 		set_tags [] =
    {
        {ASLLV_Labels	, 0	},
	{TAG_DONE		}
    };
    
    if (udata->Listview)
    {
	SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
    }
    
    ForeachNodeSafe(&udata->ListviewList, node, succ)
    {
        FRFreeListviewNode(node, ld, AslBase);
    }
    
    NEWLIST(&udata->ListviewList);
}


/*****************************************************************************************/

void FRReSortListview(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 		*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq 		*ifreq = (struct IntFileReq *)ld->ld_IntReq;
    struct Node		 	*node, *succ;
    struct List			templist;
    struct TagItem 		set_tags [] =
    {
        {ASLLV_Labels	, 0	},
	{ASLLV_Top	, 0	},
	{TAG_DONE		}
    };
    IPTR			old_top;
    
    if (udata->Flags & FRFLG_SHOWING_VOLUMES) return;

    GetAttr(ASLLV_Top, udata->Listview, &old_top);
    
    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
	
    NEWLIST(&templist);
    
    ForeachNodeSafe(&udata->ListviewList, node, succ)
    {
        Remove(node);
	MARK_UNSELECTED(node);
	AddTail(&templist, node);
    }
    
    ForeachNodeSafe(&templist, node, succ)
    {
	SortInNode(ifreq, &udata->ListviewList, node, (APTR)FRCompareDirectoryNodes, AslBase);        
    }
    
    set_tags[0].ti_Data = (IPTR)&udata->ListviewList;
    set_tags[1].ti_Data = old_top;
    
    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);    
}

/*****************************************************************************************/

void FRDirectoryScanSymbolState(struct LayoutData *ld, BOOL on, struct AslBase_intern *AslBase)
{
    
    if (ld->ld_Window)
    {
        struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
	struct TagItem 		set_tags[] =
	{
            {GA_Selected	, on	},
	    {TAG_DONE		}
	};

	SetAttrsA(udata->DirectoryScanSymbol, set_tags);
	RefreshGList((struct Gadget *)udata->DirectoryScanSymbol, ld->ld_Window, NULL, 1);
    }
}

/*****************************************************************************************/

static void FRCalcColumnWidths(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 		*udata = (struct FRUserData *)ld->ld_UserData;
    struct ASLLVFileReqNode 	*node;
    WORD 			i;
    
    for(i = 0; i < ASLLV_MAXCOLUMNS; i++)
    {
        udata->LVColumnWidth[i] = 0;
    }
    
    ForeachNode(&udata->ListviewList, node)
    {
	for(i = 0; i < ASLLV_MAXCOLUMNS; i++)
	{
            WORD w;
	    
	    if (node->text[i])
	    {
	        w = TextLength(&ld->ld_DummyRP, node->text[i], strlen(node->text[i]));
		if (w > udata->LVColumnWidth[i]) udata->LVColumnWidth[i] = w;
	    }
	}    
    }
    
    if (udata->LVColumnWidth[0] < FREQ_MIN_FILECOLUMNWIDTH)
    {
        udata->LVColumnWidth[0] = FREQ_MIN_FILECOLUMNWIDTH;      
    }
}


/*****************************************************************************************/

static WORD FRCompareDirectoryNodes(struct IntFileReq *ifreq, struct ASLLVFileReqNode *node1,
				    struct ASLLVFileReqNode *node2, struct AslBase_intern *AslBase)
{
    WORD pri1 = (node1->subtype > 0) ? 1 : 0;
    WORD pri2 = (node2->subtype > 0) ? 1 : 0;
    WORD diff = (pri2 - pri1) * -(ifreq->ifr_SortDrawers - 1);
    LONG bigdiff;
    
    if (!diff)
    {
        switch(ifreq->ifr_SortBy)
	{
	    case ASLFRSORTBY_Name:
                diff = Stricmp(node1->node.ln_Name, node2->node.ln_Name);
		break;
		
	    case ASLFRSORTBY_Date:
	    	bigdiff = CompareDates((const struct DateStamp *)&node2->date, (const struct DateStamp *)&node1->date);
		if (bigdiff < 0)
		{
		    diff = -1;
		} else if (bigdiff > 0)
		{
		    diff = 1;
		}
	    	break;
		
	    case ASLFRSORTBY_Size:
	    	if (node1->filesize < node2->filesize)
		{
		    diff = -1;
		}
		else if (node1->filesize > node2->filesize)
		{
		    diff = 1;
		}
	        break;
	}
	
	if (ifreq->ifr_SortOrder == ASLFRSORTORDER_Descend) diff = -diff;
    }
    
    return diff;
}

BOOL FRGetDirectory(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 		*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq 		*ifreq = (struct IntFileReq *)ld->ld_IntReq;
    struct FileRequester    	*freq = (struct FileRequester *)ld->ld_Req;
    struct FileInfoBlock 	*fib;
    UBYTE 			parsedpattern[MAX_PATTERN_LEN * 2 + 3];
    BPTR 			lock;
    BOOL 			dopatternstring = FALSE, success = FALSE;

    FRDirectoryScanSymbolState(ld, TRUE, AslBase);

    FRFreeListviewList(ld, AslBase);
    FRMultiSelectOnOff(ld, (ifreq->ifr_Flags1 & FRF_DOMULTISELECT) ? TRUE : FALSE, AslBase);

    if (ifreq->ifr_Flags1 & FRF_DOPATTERNS)
    {
        char *pat;
	
	GetAttr(STRINGA_TextVal, udata->PatternGad, (IPTR *)&pat);
	if (pat[0])
	{
	    if (ParsePatternNoCase(pat, parsedpattern, MAX_PATTERN_LEN * 2 + 3) != -1)
	    {
		dopatternstring = TRUE;
	    }
	}
    }
        
    lock = Lock(path, ACCESS_READ);
    if (!lock)
    {
     	D(bug("Could not lock directory \"%s\"\n", path));
    }
    else
    {	
    	fib = AllocDosObject(DOS_FIB, NULL);
	if (fib)
    	{
	    success = Examine(lock, fib);

	    if (success && fib->fib_DirEntryType > 0) for(;;)
	    {
		struct ASLLVFileReqNode *node;
		BOOL 			ok, addentry = TRUE;
		
		ok = ExNext(lock, fib);
		
	    	if (!ok)
	    	{
		    if (IoErr() == ERROR_NO_MORE_ENTRIES) break;
		    success = FALSE;
		    continue;
	   	}

		/* add to list checks which affect only files */
		
		if (fib->fib_DirEntryType < 0)
		{
		    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY) addentry = FALSE;
		    
		    if (addentry && (ifreq->ifr_Flags2 & FRF_REJECTICONS))
		    {
			WORD len = strlen(fib->fib_FileName);
			if (len >= 5)
			{
		            if (Stricmp(fib->fib_FileName + len - 5, ".info") == 0) addentry = FALSE;
			}
		    }

		    if (addentry && dopatternstring)
		    {
			if (!MatchPatternNoCase(parsedpattern, fib->fib_FileName)) addentry = FALSE;
		    }

		    if (addentry && ifreq->ifr_AcceptPattern)
		    {
			if (!MatchPatternNoCase(ifreq->ifr_AcceptPattern, fib->fib_FileName)) addentry = FALSE;
		    }

		    if (addentry && ifreq->ifr_RejectPattern)
		    {
			if (MatchPatternNoCase(ifreq->ifr_RejectPattern, fib->fib_FileName)) addentry = FALSE;
		    }
		    
		} /* if (fib->fib_DirEntryType < 0) */
		
		/* add to list checks which affect both files and drawers */
		
		if (addentry && ifreq->ifr_HookFunc && (ifreq->ifr_Flags1 & FRF_FILTERFUNC))
		{
		    struct AnchorPath ap;
		    STRPTR  	      old_frdrawer;
		    
		    /* FIXME: is ap.ap_Info enough for the hookfunc */
		    
		    memset(&ap, 0, sizeof(ap));
		    ap.ap_Info = *fib;
		    
		    /* Some user filter functions access freq->fr_Drawer :-( */
		    
		    old_frdrawer = freq->fr_Drawer;
		    freq->fr_Drawer = path;
		    
		    D(bug("FRGetDirectory: 1 fr_Drawer 0x%lx <%s>\n",path,path));

		    /* return code 0 means, add to list */

#ifdef __MORPHOS__
		    {
			ULONG ret;
			UWORD *funcptr = ifreq->ifr_HookFunc;
			ULONG *p = (ULONG *)REG_A7 - 3;

			p[0] = (ULONG)FRF_FILTERFUNC;
			p[1] = (ULONG)&ap;
			p[2] = (ULONG)freq;
			REG_A7 = (ULONG)p;

			if (*funcptr >= (UWORD)0xFF00)
			    REG_A7 -= 4;

			REG_A4 = (ULONG)ifreq->ifr_IntReq.ir_BasePtr;	/* Compatability */

			ret = (ULONG)(*MyEmulHandle->EmulCallDirect68k)(funcptr);

			if (*funcptr >= (UWORD)0xFF00)
			    REG_A7 += 4;

			REG_A7 += (3*4);

			if (ret != 0)
			    addentry = FALSE;
		    }
#else
		    if (ifreq->ifr_HookFunc(FRF_FILTERFUNC, &ap, freq) != 0)
			addentry = FALSE;
#endif
			
		    freq->fr_Drawer = old_frdrawer;
		}
		
		if (addentry && ifreq->ifr_FilterFunc)
		{
		    struct AnchorPath ap;
		    STRPTR  	      old_frdrawer;

		    /* FIXME: is ap.ap_Info enough for the filterfunc */
		    
		    memset(&ap, 0, sizeof(ap));
		    ap.ap_Info = *fib;

		    /* Some user filter functions access freq->fr_Drawer :-( */

		    old_frdrawer = freq->fr_Drawer;
		    freq->fr_Drawer = path;

		    D(bug("FRGetDirectory: 2 fr_Drawer 0x%lx <%s>\n",path,path));

		    /* return code TRUE (!= 0) means, add to list */
		    
#ifdef __MORPHOS__
		    {
			ULONG ret;

			REG_A4 = (ULONG)ifreq->ifr_IntReq.ir_BasePtr;	/* Compatability */
			REG_A0 = (ULONG)ifreq->ifr_FilterFunc;
			REG_A2 = (ULONG)freq;
			REG_A1 = (ULONG)&ap;
			ret = (*MyEmulHandle->EmulCallDirect68k)(ifreq->ifr_FilterFunc->h_Entry);

			if (ret == 0)
			    addentry = FALSE;
		    }
#else
		    if (CallHookPkt(ifreq->ifr_FilterFunc, freq, &ap) == 0)
			addentry = FALSE;
#endif

		    freq->fr_Drawer = old_frdrawer;

		    D(bug("FRGetDirectory: 3 fr_Drawer 0x%lx <%s>\n",old_frdrawer,old_frdrawer));

		}
				
		if (addentry)
		{
		    if ((node = AllocPooled(ld->ld_IntReq->ir_MemPool, sizeof(struct ASLLVFileReqNode))))
		    {
			struct DateTime dt;
			char 		datebuffer[LEN_DATSTRING];
			char 		timebuffer[LEN_DATSTRING];			

			if (fib->fib_FileName[0])
			{
			    node->text[0] = node->node.ln_Name = PooledCloneString(fib->fib_FileName,
			    							   NULL,
										   ld->ld_IntReq->ir_MemPool,
										   AslBase);
			}

			if (fib->fib_DirEntryType > 0)
			{
			    node->text[1] = GetString(MSG_FILEREQ_LV_DRAWER, GetIR(ifreq)->ir_Catalog, AslBase);
			    node->dontfreetext |= (1 << 1);
			} else {
			    node->text[1] = PooledUIntegerToString(fib->fib_Size,
			    					  ld->ld_IntReq->ir_MemPool,
								  AslBase);
			    
			    MARK_DO_MULTISEL(node);
			}
			
			dt.dat_Stamp = fib->fib_Date;
			dt.dat_Format = FORMAT_DOS;
			dt.dat_Flags = 0;
			dt.dat_StrDay = NULL;
			dt.dat_StrDate = datebuffer;
			dt.dat_StrTime = timebuffer;
			
			if (DateToStr(&dt))
			{
			    //sprintf(datebuffer, "%x8", fib->fib_Date.ds_Days);
			    //sprintf(timebuffer, "%x8", fib->fib_Date.ds_Minute);
			    
			    node->text[2] = PooledCloneString(datebuffer, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
			    node->text[3] = PooledCloneString(timebuffer, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
			}
			
			if (fib->fib_Comment[0])
			{
			    node->text[4] = PooledCloneString(fib->fib_Comment, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
			}
			
			node->userdata = ld;
			node->date     = fib->fib_Date;
			node->filesize = fib->fib_Size;
			node->type     = ASLLV_FRNTYPE_DIRECTORY;
			node->subtype  = fib->fib_DirEntryType;

			SortInNode(ifreq, &udata->ListviewList, &node->node, (APTR)FRCompareDirectoryNodes, AslBase);
		    }
		    
		} /* if (addentry) */
		
	    } /* foreach file in directory */

	    FreeDosObject(DOS_FIB, fib);

    	} /* if (DosObject allocated) */

    	UnLock(lock);

    } /* if (directory locked) */

    udata->LVColumnAlign[0] = ASLLV_ALIGN_LEFT; 	/* File/Dir name */
    udata->LVColumnAlign[1] = ASLLV_ALIGN_RIGHT;	/* Size/"Drawer" */
    udata->LVColumnAlign[2] = ASLLV_ALIGN_LEFT;		/* Date */
    udata->LVColumnAlign[3] = ASLLV_ALIGN_LEFT;		/* Time */
    udata->LVColumnAlign[4] = ASLLV_ALIGN_LEFT;		/* Comment */
    	
    FRCalcColumnWidths(ld, AslBase);
    FRRefreshListview(ld, AslBase);
    
    udata->Flags &= ~FRFLG_SHOWING_VOLUMES;

    FRDirectoryScanSymbolState(ld, FALSE, AslBase);
   
    return success;		
}

/*****************************************************************************************/

static WORD FRCompareVolumeNodes(struct IntFileReq *ifreq, struct ASLLVFileReqNode *node1,
				 struct ASLLVFileReqNode *node2, struct AslBase_intern *AslBase)
{
    WORD pri1, pri2, diff;
    
    switch(node1->subtype)
    {
	case DLT_DIRECTORY:
	case DLT_LATE:
	case DLT_NONBINDING:
	    pri1 = 0;
	    break;
	    
	default:
	    pri1 = 1;
	    break;
    }

    switch(node2->subtype)
    {
	case DLT_DIRECTORY:
	case DLT_LATE:
	case DLT_NONBINDING:
	    pri2 = 0;
	    break;
	    
	default:
	    pri2 = 1;
	    break;
    }
    
    diff = pri2 - pri1;
    if (!diff) diff = Stricmp(node1->node.ln_Name, node2->node.ln_Name);
    
    return diff;
}

/*****************************************************************************************/

BOOL FRGetVolumes(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq 	*ifreq = (struct IntFileReq *)ld->ld_IntReq;
    struct DosList 	*dlist;
    BOOL 		success = TRUE;
    
    FRDirectoryScanSymbolState(ld, TRUE, AslBase);
    
    FRFreeListviewList(ld, AslBase);
    FRMultiSelectOnOff(ld, FALSE, AslBase);
    
    dlist = LockDosList(LDF_READ | LDF_VOLUMES | LDF_ASSIGNS);
    
    while ((dlist = NextDosEntry(dlist, LDF_VOLUMES | LDF_ASSIGNS)) != NULL)
    {
	struct ASLLVFileReqNode *node;

	if ((node = AllocPooled(ld->ld_IntReq->ir_MemPool, sizeof(struct ASLLVFileReqNode))))
	{
#ifdef __MORPHOS__
	    char *name = BADDR(dlist->dol_Name);
	    if (name)
	    {
		node->text[0] = PooledCloneStringLen(&name[1], name[0], ":", sizeof(":"), ld->ld_IntReq->ir_MemPool, AslBase);
		node->node.ln_Name = PooledCloneStringLen(&name[1], name[0], NULL, 0, ld->ld_IntReq->ir_MemPool, AslBase);
	    }
#else
	    char *name = dlist->dol_DevName;
	    if (name)
	    {
		node->text[0] = node->node.ln_Name = PooledCloneString(name, ":", ld->ld_IntReq->ir_MemPool, AslBase);
	    }
#endif
	    
	    switch(dlist->dol_Type)
	    {
		case DLT_DIRECTORY:
		case DLT_LATE:
		case DLT_NONBINDING:
		    node->text[1] = GetString(MSG_FILEREQ_LV_ASSIGN, GetIR(ifreq)->ir_Catalog, AslBase);
		    node->dontfreetext |= (1 << 1);
		    break;

#ifdef __MORPHOS__
		case DLT_VOLUME:
		    {
			struct DosList *dl = LockDosList(LDF_DEVICES | LDF_READ);

			while ((dl = NextDosEntry(dl, LDF_DEVICES)))
			{
			    /* This works for most filesystems */
			    if (dl->dol_Task == dlist->dol_Task)
			    {
				STRPTR devname = (STRPTR)BADDR(dl->dol_Name);
				ULONG devlen = *devname++;

				if (devname[devlen-1] == 0) devlen--;

				node->text[1] = AllocPooled(ld->ld_IntReq->ir_MemPool, devlen + 3);
				node->text[1][0] = '(';
				strncpy(&node->text[1][1], devname, devlen);
				node->text[1][devlen+1] = ')';
				node->text[1][devlen+2] = 0;

				break;
			    }
			}

			UnLockDosList(LDF_DEVICES | LDF_READ);
		    }
		    break;
#endif
	    }
	    	    
	    node->userdata = ld;
	    
	    node->filesize    = 0;
	    node->type	      = ASLLV_FRNTYPE_VOLUMES;
	    node->subtype     = dlist->dol_Type;

	    SortInNode(ifreq, &udata->ListviewList, &node->node, (APTR)FRCompareVolumeNodes, AslBase);

	}

    }
    
    UnLockDosList(LDF_READ | LDF_VOLUMES | LDF_ASSIGNS);

    udata->LVColumnAlign[0] = ASLLV_ALIGN_LEFT;
    udata->LVColumnAlign[1] = ASLLV_ALIGN_RIGHT;

    FRCalcColumnWidths(ld, AslBase);
    FRRefreshListview(ld, AslBase);
    
    udata->Flags |= FRFLG_SHOWING_VOLUMES;

    FRDirectoryScanSymbolState(ld, FALSE, AslBase);
    
    return success;
}

/*****************************************************************************************/

void FRSetPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct TagItem 	set_tags[] =
    {
	{STRINGA_TextVal	, path ? (IPTR)path : (IPTR)""	},
	{TAG_DONE						}
    };    
    
    SetGadgetAttrsA((struct Gadget *)udata->PathGad, ld->ld_Window, NULL, set_tags);
}

/*****************************************************************************************/

BOOL FRNewPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    char 		pathstring[MAX_PATH_LEN];
    BOOL 		result = FALSE;
	
    strcpy(pathstring, path);
    fixpath(pathstring);
    
    FRSetPath(pathstring, ld, AslBase);
    
    result = FRGetDirectory(pathstring, ld, AslBase);

    return result;
}

/*****************************************************************************************/

BOOL FRAddPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;
    char 		pathstring[MAX_PATH_LEN], *gadpath;
    BOOL 		result;
    
    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&gadpath);
    
    strcpy(pathstring, gadpath);
    AddPart(pathstring, path, MAX_PATH_LEN);
    
    result = FRNewPath(pathstring, ld, AslBase);
    
    return result;
   
}

/*****************************************************************************************/

BOOL FRParentPath(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;
    char 		pathstring[MAX_PATH_LEN], *gadpath;
    WORD 		len;
    BOOL 		result;
    
    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&gadpath);
    
    strcpy(pathstring, gadpath);
    fixpath(pathstring);
    
    len = strlen(pathstring);
    if (len == 0)
    {
        strcpy(pathstring, "/");
    }
    else if (len < MAX_PATH_LEN - 3)
    {
        switch(pathstring[len - 1])
	{
	    case '/':
	    case ':':
	        strcat(pathstring, "/");
		break;
		
	    default:
	        strcat(pathstring, "//");
		fixpath(pathstring);
		break;
	}
    }
    
    result = FRNewPath(pathstring, ld, AslBase);
    
    return result;
   
}

/*****************************************************************************************/

void FRSetFile(STRPTR file, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct TagItem 	set_tags[] =
    {
	{STRINGA_TextVal	, file ? (IPTR)file : (IPTR)""	},
	{TAG_DONE						}
    };

    SetGadgetAttrsA((struct Gadget *)udata->FileGad, ld->ld_Window, NULL, set_tags);
}

/*****************************************************************************************/

void FRChangeActiveLVItem(struct LayoutData *ld, WORD delta, UWORD quali, struct Gadget *gad, struct AslBase_intern *AslBase)
{
    struct FRUserData 		*udata = (struct FRUserData *)ld->ld_UserData;    
    struct IntFileReq 		*ifreq = (struct IntFileReq *)ld->ld_IntReq;
    struct ASLLVFileReqNode 	*node;
    IPTR 			active, total, visible;
    struct Gadget		*mainstrgad;
    struct TagItem		set_tags[] =
    {
    	{ASLLV_Active		, 0		},
	{ASLLV_MakeVisible	, 0		},
	{TAG_DONE				}
    };
    
    GetAttr(ASLLV_Active , udata->Listview, &active );
    GetAttr(ASLLV_Total  , udata->Listview, &total  );
    GetAttr(ASLLV_Visible, udata->Listview, &visible);
    
    mainstrgad = (struct Gadget *)((ifreq->ifr_Flags2 & FRF_DRAWERSONLY) ? udata->PathGad : udata->FileGad);
    
    if (total < 1) return;
    
    if (quali & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
    {
        delta *= (visible - 1);
    }
    else if (quali & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL))
    {
        delta *= total;
    }
    else if (gad && (gad == mainstrgad))
    {
        /* try to jump to first item which matches text in string gadget,
	   but only if text in string gadget mismatches actual active
	   item's text (in this case move normally = one step)) */
	   
	char buffer[MAX_FILE_LEN];
	char *val;
	WORD i, len;
	BOOL dojump = TRUE;
		
	GetAttr(STRINGA_TextVal, (Object *)gad, (IPTR *)&val);
	strcpy(buffer, val);

	len = strlen(buffer);
	if (len > 0) if (buffer[len - 1] == '/') buffer[--len] = '\0';
	
	if (len)
	{
	    if (((LONG)active) >= 0)
	    {
		if ((node = (struct ASLLVFileReqNode *)FindListNode(&udata->ListviewList, (WORD)active)))
		{
	            if (stricmp(node->node.ln_Name, buffer) == 0) dojump = FALSE;
		}     
	    }

	    if (dojump)
	    {
		i = 0;
		ForeachNode(&udata->ListviewList, node)
		{
		    if (Strnicmp((CONST_STRPTR)node->node.ln_Name, (CONST_STRPTR)buffer, len) == 0)
		    {
			active = i;
			delta = 0;
			break;
		    }
		    i++;
		    
		}
		
	    } /* if (dojump) */
	    
	} /* if (len) */
	    
    }
    
    active += delta;
    
    if (((LONG)active) < 0) active = 0;
    if (active >= total) active = total - 1;
    
    set_tags[0].ti_Data = set_tags[1].ti_Data = active;
    
    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
    
    if ((node = (struct ASLLVFileReqNode *)FindListNode(&udata->ListviewList, (WORD)active)))
    {
	if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
	{
            FRSetPath(node->node.ln_Name, ld, AslBase);
	} else {
	    char pathstring[MAX_FILE_LEN];
	    
	    strcpy(pathstring, node->node.ln_Name);
	    if ((node->subtype > 0) && (!(udata->Flags & FRFLG_SHOWING_VOLUMES)))
	    {
	        strcat(pathstring, "/");
	    }
	    
	    FRSetFile(pathstring, ld, AslBase);
	}
    }
 
    if (gad)
    {
        ActivateGadget(gad, ld->ld_Window, NULL);
    } else {
        FRActivateMainStringGadget(ld, AslBase);
    }
}

/*****************************************************************************************/

void FRActivateMainStringGadget(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData *udata = (struct FRUserData *)ld->ld_UserData;    
    struct IntFileReq *ifreq = (struct IntFileReq *)ld->ld_IntReq;

    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
    {
        ActivateGadget((struct Gadget *)udata->PathGad, ld->ld_Window, NULL);
    } else {
        ActivateGadget((struct Gadget *)udata->FileGad, ld->ld_Window, NULL);
    }   
}

/*****************************************************************************************/

void FRMultiSelectOnOff(struct LayoutData *ld, BOOL onoff, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct TagItem 	set_tags[] =
    {
	{ASLLV_DoMultiSelect	, onoff	},
	{TAG_DONE			}
    };    
    
    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
}

/*****************************************************************************************/

void FRSetPattern(STRPTR pattern, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct TagItem 	set_tags[] =
    {
	{STRINGA_TextVal	, pattern ? (IPTR)pattern : (IPTR)""	},
	{TAG_DONE							}
    };    
    
    SetGadgetAttrsA((struct Gadget *)udata->PatternGad, ld->ld_Window, NULL, set_tags); 
}

/*****************************************************************************************/

void FRSelectRequester(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq   *ifreq = (struct IntFileReq *)ld->ld_IntReq;
    STRPTR 		oldpattern = udata->SelectPattern;

    udata->SelectPattern = REQ_String(GetString(MSG_FILEREQ_SELECT_TITLE, GetIR(ifreq)->ir_Catalog, AslBase),
				      oldpattern ? oldpattern : (STRPTR)"#?", 
				      GetString(MSG_FILEREQ_SELECT_OK, GetIR(ifreq)->ir_Catalog, AslBase),
				      GetString(MSG_FILEREQ_SELECT_CANCEL, GetIR(ifreq)->ir_Catalog, AslBase),
				      ld,
				      AslBase);

    if (oldpattern) MyFreeVecPooled(oldpattern, AslBase);

    if (udata->SelectPattern)
    {
	LONG patternlen = strlen(udata->SelectPattern);

	if (patternlen > 0)
	{
	    STRPTR parsedpattern;

	    parsedpattern = MyAllocVecPooled(ld->ld_IntReq->ir_MemPool, patternlen * 2 + 3, AslBase);
	    if (parsedpattern)
	    {
		if (ParsePatternNoCase(udata->SelectPattern, parsedpattern, patternlen * 2 + 3) != -1)
		{
		    struct ASLLVFileReqNode *node;

		    if (udata->Flags & FRFLG_SHOWING_VOLUMES)
		    {
		    	UBYTE *dir;

			GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&dir);
			FRGetDirectory(dir, ld, AslBase);
		    }
		    		    
		    SetGadgetAttrs((struct Gadget *)udata->Listview, ld->ld_Window, NULL, ASLLV_Active, -1,
		    									  TAG_DONE);
											  
		    ForeachNode(&udata->ListviewList, node)
		    {
			if (IS_MULTISEL(node) && node->node.ln_Name)
			{
			    if (MatchPatternNoCase(parsedpattern, node->node.ln_Name))
			    {
				MARK_SELECTED(node);
			    } else {
				MARK_UNSELECTED(node);
			    }
			}
		    }

		    RefreshGList((struct Gadget *)udata->Listview, ld->ld_Window, NULL, 1);

		}
		MyFreeVecPooled(parsedpattern, AslBase);

	    } /* if (parsedpattern) */

	} /* if (udata->SelectPattern[0]) */

    } /* if (udata->SelectPattern) */

} /**/

/*****************************************************************************************/

void FRDeleteRequester(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq   *ifreq = (struct IntFileReq *)ld->ld_IntReq;
    STRPTR 		name = NULL, name2, s;

    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&name);

    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY)
    {
	name = name2 = VecPooledCloneString(name, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
    } else {
        GetAttr(STRINGA_TextVal, udata->FileGad, (IPTR *)&name2);
	
	if ((s = MyAllocVecPooled(ld->ld_IntReq->ir_MemPool, strlen(name) + strlen(name2) + 4, AslBase)))
	{
	    strcpy(s, name);
	    AddPart(s, name2, 10000);
	    name = s;
	}
    }
    
    if (name) if (name[0])
    {
	struct EasyStruct es;

	es.es_StructSize   = sizeof(es);
	es.es_Flags 	   = 0;
	es.es_Title 	   = GetString(MSG_FILEREQ_DELETE_TITLE   , GetIR(ifreq)->ir_Catalog, AslBase);
	es.es_TextFormat   = GetString(MSG_FILEREQ_DELETE_MSG     , GetIR(ifreq)->ir_Catalog, AslBase);
	es.es_GadgetFormat = GetString(MSG_FILEREQ_DELETE_OKCANCEL, GetIR(ifreq)->ir_Catalog, AslBase);

	if (EasyRequestArgs(ld->ld_Window, &es, NULL, &name) == 1)
	{
	    if (DeleteFile(name) && !(udata->Flags & FRFLG_SHOWING_VOLUMES))
	    {
		struct ASLLVFileReqNode *node;
	        struct TagItem 		set_tags[] =
		{
		    {ASLLV_Labels	, 0	},
		    {ASLLV_Top		, 0	},
		    {TAG_DONE			}
		};
		IPTR			top;
		
		GetAttr(ASLLV_Top, udata->Listview, &top);		
		SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
		
		ForeachNode(&udata->ListviewList, node)
		{
		    if (node->node.ln_Name)
		    {
		        if (stricmp(node->node.ln_Name, name2) == 0)
			{
			    Remove(&node->node);
			    FRFreeListviewNode(node, ld, AslBase);
			    break;
			}
		    }
		}
		
		set_tags[0].ti_Data = (IPTR)&udata->ListviewList;
		set_tags[1].ti_Data = top;
   		SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);

		
	    } /* if (DeleteFile(name) && !(udata->Flags & FRFLG_SHOWING_VOLUMES)) */

	} /* if (EasyRequestArgs(ld->ld_Window, &es, NULL, &name) == 1) */

    } /* if (name) if (name[0]) */

    if (name) MyFreeVecPooled(name, AslBase);
}		

/*****************************************************************************************/

void FRNewDrawerRequester(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq   *ifreq = (struct IntFileReq *)ld->ld_IntReq;
    STRPTR		path, dirname;
    BPTR		lock, lock2, olddir;
    
    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&path);
    
    lock = Lock(path, SHARED_LOCK);
    if (lock)
    {
        olddir = CurrentDir(lock);
	
	if ((dirname = REQ_String(GetString(MSG_FILEREQ_CREATEDRAWER_TITLE, GetIR(ifreq)->ir_Catalog, AslBase),
				  GetString(MSG_FILEREQ_CREATEDRAWER_DEFNAME, GetIR(ifreq)->ir_Catalog, AslBase), 
				  GetString(MSG_FILEREQ_CREATEDRAWER_OK, GetIR(ifreq)->ir_Catalog, AslBase),
				  GetString(MSG_FILEREQ_CREATEDRAWER_CANCEL, GetIR(ifreq)->ir_Catalog, AslBase),
				  ld,
				  AslBase)))
	{
	    if ((lock2 = CreateDir(dirname)))
	    {		    
		if (!(udata->Flags & FRFLG_SHOWING_VOLUMES))
		{
		    struct TagItem 	set_tags[] =
		    {
		        {ASLLV_Top	, 0	},
			{TAG_DONE		}
		    };
		    IPTR 		top;

		    GetAttr(ASLLV_Top, udata->Listview, &top);

		    FRGetDirectory(path, ld, AslBase); 

		    set_tags[0].ti_Data = top;
		    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
		}

		UnLock(lock2);
		
	    } /* if ((lock2 = CreateDir(dirname))) */

            MyFreeVecPooled(dirname, AslBase);

	}; /* if ((dirname = REQ_String(... */
	    
	CurrentDir(olddir);
	UnLock(lock);
	
    } /* if (lock) */

}

/*****************************************************************************************/

void FRRenameRequester(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData 	*udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq   *ifreq = (struct IntFileReq *)ld->ld_IntReq;
    STRPTR		path, file, newname;
    BPTR		lock, lock2, olddir;
    
    if (ifreq->ifr_Flags2 & FRF_DRAWERSONLY) return;
    
    GetAttr(STRINGA_TextVal, udata->FileGad, (IPTR *)&file);	
    if (!file[0]) return;

    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&path);
    
    lock = Lock(path, SHARED_LOCK);
    if (lock)
    {
        olddir = CurrentDir(lock);
	
	lock2 = Lock(file, SHARED_LOCK);
	if (lock2)
	{
	    UnLock(lock2);
	    
	    if ((newname = REQ_String(GetString(MSG_FILEREQ_RENAME_TITLE, GetIR(ifreq)->ir_Catalog, AslBase),
				      file, 
				      GetString(MSG_FILEREQ_RENAME_OK, GetIR(ifreq)->ir_Catalog, AslBase),
				      GetString(MSG_FILEREQ_RENAME_CANCEL, GetIR(ifreq)->ir_Catalog, AslBase),
				      ld,
				      AslBase)))
	    {
	        if (Rename(file, newname))
		{		    
		    FRSetFile(newname, ld, AslBase);

		    if (!(udata->Flags & FRFLG_SHOWING_VOLUMES))
		    {
			struct TagItem 	set_tags[] =
			{
		            {ASLLV_Top	, 0	},
			    {TAG_DONE		}
			};
			IPTR 		top;

			GetAttr(ASLLV_Top, udata->Listview, &top);

			FRGetDirectory(path, ld, AslBase); 

			set_tags[0].ti_Data = top;
			SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
		    }
		    
		} /* if (Rename(file, newname)) */
		
        	MyFreeVecPooled(newname, AslBase);
		
	    }; /* if ((newname = REQ_String(... */
	    
	} /* if (lock2) */
	
	CurrentDir(olddir);
	UnLock(lock);
	
    } /* if (lock) */

}

/*****************************************************************************************/

