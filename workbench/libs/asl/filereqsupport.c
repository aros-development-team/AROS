/*
    (C) 1995-97 AROS - The Amiga Research OS

    Desc:
    Lang: english
*/

#define AROS_ALMOST_COMPATIBLE

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
#include <stdio.h>
#include <string.h>
#include <clib/macros.h>

#include "asl_intern.h"
#include "filereqsupport.h"
#include "filereqhooks.h"
#include "layout.h"

#define SDEBUG 0
#define DEBUG 1
#define ADEBUG 1

#include <aros/debug.h>

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
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;	
    struct TagItem set_tags[] =
    {
	{ASLLV_Labels, (IPTR)&udata->ListviewList	},
	{TAG_DONE					}
    };

    if (ld->ld_Window == NULL)
    {
	SetAttrsA(udata->Listview, set_tags);
    } else {
	SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
    }
}

/*****************************************************************************************/

void FRFreeListviewList(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;	
    struct ASLLVFileReqNode *node, *succ;
    struct TagItem set_tags [] =
    {
        {ASLLV_Labels	, NULL	},
	{TAG_DONE		}
    };
    
    if (udata->Listview)
    {
	if (ld->ld_Window == NULL)
	{
	    SetAttrsA(udata->Listview, set_tags);
	} else {
	    SetGadgetAttrsA((struct Gadget *)udata->Listview, ld->ld_Window, NULL, set_tags);
	}
    }
    
    ForeachNodeSafe(&udata->ListviewList, node, succ)
    {
        WORD i = 0;
	
	for(i = 0; i < ASLLV_MAXCOLUMNS; i++)
	{
	    if (node->text[i] && !(node->dontfreetext & (1 << i)))
	    {
	        FreePooled(ld->ld_IntReq->ir_MemPool, node->text[i],  strlen(node->text[i]) + 1);
	    }
        }
	
        FreePooled(ld->ld_IntReq->ir_MemPool, node, sizeof(struct ASLLVFileReqNode));
    }
    
    NEWLIST(&udata->ListviewList);
}

/*****************************************************************************************/

void FRDirectoryScanSymbolState(struct LayoutData *ld, BOOL on, struct AslBase_intern *AslBase)
{
    
    if (ld->ld_Window)
    {
        struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;	
	struct TagItem set_tags[] =
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
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;
    struct ASLLVFileReqNode *node;
    WORD i;
    
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

static WORD FRGetDirectoryNodePri(struct ASLLVFileReqNode *node)
{
    return (node->subtype > 0) ? 1 : 0; 
}

BOOL FRGetDirectory(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq *ifreq = (struct IntFileReq *)ld->ld_IntReq;
    struct FileInfoBlock *fib;
    UBYTE parsedpattern[MAX_PATTERN_LEN * 2 + 3];
    BPTR lock;

    BOOL dopatternstring = FALSE, success = FALSE;

    FRDirectoryScanSymbolState(ld, TRUE, AslBase);

    FRFreeListviewList(ld, AslBase);

    if (ifreq->ifr_Flags1 & FRF_DOPATTERNS)
    {
        char *pat;
	
	GetAttr(STRINGA_TextVal, udata->PatternGad, (IPTR *)&pat);
	if (ParsePatternNoCase(pat, parsedpattern, MAX_PATTERN_LEN * 2 + 3) != -1)
	{
	    dopatternstring = TRUE;
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

	    if (success) for(;;)
	    {
		struct ASLLVFileReqNode *node;
		BOOL ok, addentry = TRUE;
		
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
		            if (stricmp(fib->fib_FileName + len - 5, ".info") == 0) addentry = FALSE;
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
		    
		    /* FIXME: is ap.ap_Info enough for the hookfunc */
		    
		    memset(&ap, 0, sizeof(ap));
		    ap.ap_Info = *fib;
		    
		    /* return code 0 means, add to list */
		    
		    if (ifreq->ifr_HookFunc(FRF_FILTERFUNC, &ap, (struct FileRequester *)ld->ld_Req) != 0)
		        addentry = FALSE;
		}
		
		if (addentry && ifreq->ifr_FilterFunc)
		{
		    struct AnchorPath ap;

		    /* FIXME: is ap.ap_Info enough for the filterfunc */
		    
		    memset(&ap, 0, sizeof(ap));
		    ap.ap_Info = *fib;

		    /* return code TRUE (!= 0) means, add to list */
		    
		    if (CallHookPkt(ifreq->ifr_FilterFunc, (struct FileRequester *)ld->ld_Req, &ap) == 0)
		        addentry = FALSE;
		}
				
		if (addentry)
		{
		    if ((node = AllocPooled(ld->ld_IntReq->ir_MemPool, sizeof(struct ASLLVFileReqNode))))
		    {
			char datebuffer[LEN_DATSTRING];
			char timebuffer[LEN_DATSTRING];
			
			struct DateTime dt;

			if (fib->fib_FileName[0])
			{
			    node->text[0] = node->node.ln_Name = PooledCloneString(fib->fib_FileName,
			    							   NULL,
										   ld->ld_IntReq->ir_MemPool,
										   AslBase);
			}

			if (fib->fib_DirEntryType > 0)
			{
			    node->text[1] = ifreq->ifr_LVDrawerText;
			    node->dontfreetext |= (1 << 1);
			} else {
			    node->text[1] = PooledIntegerToString(fib->fib_Size,
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
			    node->text[2] = PooledCloneString(datebuffer, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
			    node->text[3] = PooledCloneString(timebuffer, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
			}
			
			if (fib->fib_Comment[0])
			{
			    node->text[4] = PooledCloneString(fib->fib_Comment, NULL, ld->ld_IntReq->ir_MemPool, AslBase);
			}
			
			node->userdata = ld;
			
			node->filesize = fib->fib_Size;
			node->type     = ASLLV_FRNTYPE_DIRECTORY;
			node->subtype  = fib->fib_DirEntryType;

			SortInNode(&udata->ListviewList, &node->node, (APTR)FRGetDirectoryNodePri);
		    }
		    
		} /* if (addentry) */
		
	    }; /* foreach file in directory */

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

static WORD FRGetVolumeNodePri(struct ASLLVFileReqNode *node)
{
    WORD pri;
    
    switch(node->subtype)
    {
	case DLT_DIRECTORY:
	case DLT_LATE:
	case DLT_NONBINDING:
	    pri = 0;
	    break;
	    
	default:
	    pri = 1;
	    break;
    }
    
    return pri;
}

BOOL FRGetVolumes(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;	
    struct IntFileReq *ifreq = (struct IntFileReq *)ld->ld_IntReq;
    struct DosList *dlist;
    BOOL success = TRUE;
    
    FRDirectoryScanSymbolState(ld, TRUE, AslBase);
    
    FRFreeListviewList(ld, AslBase);
    
    dlist = LockDosList(LDF_READ | LDF_VOLUMES | LDF_ASSIGNS);
    
    while ((dlist = NextDosEntry(dlist, LDF_VOLUMES | LDF_ASSIGNS)) != NULL)
    {
	struct ASLLVFileReqNode *node;

	if ((node = AllocPooled(ld->ld_IntReq->ir_MemPool, sizeof(struct ASLLVFileReqNode))))
	{
	    char *name = dlist->dol_DevName;
	    
	    if (name)
	    {
	    	node->text[0] = node->node.ln_Name = PooledCloneString(name, ":", ld->ld_IntReq->ir_MemPool, AslBase);
	    }
	    
	    switch(dlist->dol_Type)
	    {
		case DLT_DIRECTORY:
		case DLT_LATE:
		case DLT_NONBINDING:
		    node->text[1] = ifreq->ifr_LVAssignText;
		    node->dontfreetext |= (1 << 1);
		    break;
	    }
	    	    
	    node->userdata = ld;
	    
	    node->filesize    = 0;
	    node->type	      = ASLLV_FRNTYPE_VOLUMES;
	    node->subtype     = dlist->dol_Type;

	    SortInNode(&udata->ListviewList, &node->node, (APTR)FRGetVolumeNodePri);

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

BOOL FRNewPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;	
    char pathstring[257];
    struct TagItem set_tags[] =
    {
	{STRINGA_TextVal	, (IPTR)pathstring	},
	{TAG_DONE					}
    };
    
    BOOL result = FALSE;
	
    strcpy(pathstring, path);
    fixpath(pathstring);
    
    if (ld->ld_Window)
    {    
        SetGadgetAttrsA((struct Gadget *)udata->PathGad, ld->ld_Window, NULL, set_tags);
    } else {
        SetAttrsA(udata->PathGad, set_tags);
    }
    
    result = FRGetDirectory(pathstring, ld, AslBase);

    return result;
}

/*****************************************************************************************/

BOOL FRAddPath(STRPTR path, struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;
    char pathstring[257], *gadpath;
    BOOL result;
    
    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&gadpath);
    
    strcpy(pathstring, gadpath);
    AddPart(pathstring, path, 257);
    
    result = FRNewPath(pathstring, ld, AslBase);
    
    return result;
   
}

/*****************************************************************************************/

BOOL FRParentPath(struct LayoutData *ld, struct AslBase_intern *AslBase)
{
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;
    char pathstring[257], *gadpath;
    WORD len;
    BOOL result;
    
    GetAttr(STRINGA_TextVal, udata->PathGad, (IPTR *)&gadpath);
    
    strcpy(pathstring, gadpath);
    fixpath(pathstring);
    
    len = strlen(pathstring);
    if (len == 0)
    {
        strcpy(pathstring, "/");
    }
    else if (len < 254)
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
    struct FRUserData * udata = (struct FRUserData *)ld->ld_UserData;	

    struct TagItem set_tags[] =
    {
	{STRINGA_TextVal	, (IPTR)file	},
	{TAG_DONE				}
    };

    SetGadgetAttrsA((struct Gadget *)udata->FileGad, ld->ld_Window, NULL, set_tags);
}


/*****************************************************************************************/

