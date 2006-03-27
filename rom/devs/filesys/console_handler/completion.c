/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: 
    Lang: english
*/

#include <exec/memory.h>
#include <dos/dos.h>
#include <utility/utility.h>
#include <intuition/intuition.h>
#include <graphics/rastport.h>
#include <libraries/gadtools.h>
#include <libraries/asl.h>
#include <devices/rawkeycodes.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/alib.h>
#define __GADTOOLS_NOLIBBASE__
#include <proto/gadtools.h>
#include <proto/asl.h>
#include "con_handler_intern.h"
#include "support.h"
#include "completion.h"

#include <string.h>
#include <stdlib.h>

#define DEBUG 0
#include <aros/debug.h>

/****************************************************************************************/

struct matchnode
{
    struct Node     node;
    UBYTE   	    name[1];
};

/****************************************************************************************/

struct completioninfo
{
    struct conbase     *conbase;
    struct filehandle  *fh;
    struct List     	matchlist;
    APTR    	    	pool;
    WORD    	    	nummatchnodes;
    WORD    	    	wordstart;
    BOOL    	    	wordquoted;
    UBYTE   	    	filepart[256];
    UBYTE   	    	dirpart[256];
    UBYTE   	    	pattern[256*2+3];
    UBYTE   	    	match[256];
};

/****************************************************************************************/

/* Delay opening of the gadtools.library to the first time InitCompletion is called */

static struct Library *GadToolsBase = NULL;

static struct completioninfo *InitCompletion(struct conbase *conbase, struct filehandle *fh)
{
    struct completioninfo *ci = NULL;
    APTR    	    	   pool;

    if (GadToolsBase == NULL)
	GadToolsBase = OpenLibrary("gadtools.library", 39);
    
    if (fh->lastwritetask && GadToolsBase)
    if (fh->lastwritetask->tc_Node.ln_Type == NT_PROCESS)
    {    
	if ((pool = CreatePool(MEMF_CLEAR, 1024, 1024)))
	{
    	    BOOL ok = FALSE;

    	    if ((ci = (struct completioninfo *)AllocPooled(pool, sizeof(*ci))))
	    {
		ci->pool = pool;
		ci->conbase = conbase;
		ci->fh = fh;
		NewList(&ci->matchlist);
		
		ok = TRUE;
	    }

	    if (!ok)
	    {
		DeletePool(pool);
		ci = NULL;
	    }
	}
    }
    
    return ci;
}

/****************************************************************************************/

static void CleanupCompletion(struct conbase *conbase, struct completioninfo *ci)
{
    DeletePool(ci->pool);
}

/****************************************************************************************/

static void PrepareCompletion(struct conbase *conbase, struct completioninfo *ci)
{
    WORD i;
    BOOL in_quotes = FALSE;
    
    /* Find word start */
    
    ci->wordstart = i = ci->fh->inputstart;

    while(i != ci->fh->inputpos)
    {
    	switch(ci->fh->inputbuffer[i++])
	{
	    case '"':
	    	in_quotes = !in_quotes;
		if (in_quotes) ci->wordstart = i;
		break;
		
	    case ' ':
	    	if (!in_quotes) ci->wordstart = i;
		break;
	}
    }
    
    strncpy(ci->dirpart,
    	    &ci->fh->inputbuffer[ci->wordstart], 
	    ci->fh->inputpos - ci->wordstart);
    strcpy(ci->filepart, FilePart(ci->dirpart));
	    
    *(PathPart(ci->dirpart)) = '\0';
    
    ci->wordquoted = in_quotes;
    
    D(bug("PrepareCompletion: dirpart = \"%s\"  filepart = \"%s\"\n", ci->dirpart, ci->filepart));
}

/****************************************************************************************/

static void AddQuotes(struct completioninfo *ci, STRPTR s, LONG s_size)
{
    struct conbase *conbase = ci->conbase;
    
    LONG len = strlen(s);
    
    if (!ci->wordquoted)
    {
    	if (len < s_size - 1)
    	memmove(s + 1, s, len + 1);
	s[0] = '"';	
    }
    else
    {
    	len--;
    }

    if (len < s_size - 3)
    {
	if ((s[len] != '/') && (s[len] != ':'))
	{
	    s[len + 1] = '"';
	    s[len + 2] = '\0';
	}
    }
}

/****************************************************************************************/

static void InsertIntoConBuffer(struct conbase *conbase, struct completioninfo *ci, STRPTR s)
{
    WORD i = ci->fh->conbufferpos;
    
    /* Lame code, but speed is no issue here */
    while((ci->fh->conbuffersize < CONSOLEBUFFER_SIZE) && *s)
    {
    	memmove(&ci->fh->consolebuffer[i + 1],
	    	&ci->fh->consolebuffer[i],
		ci->fh->conbuffersize - i);
		
	ci->fh->consolebuffer[i++] = *s++;
    	ci->fh->conbuffersize++;
    }
}

/****************************************************************************************/

static void DoFileReq(struct conbase *conbase, struct completioninfo *ci)
{
    struct Library *AslBase;
    BPTR    	    lock, olddir;
    
    if ((lock = DupLock(((struct Process *)ci->fh->lastwritetask)->pr_CurrentDir)))
    {
    	olddir = CurrentDir(lock);
	
	if ((AslBase = OpenLibrary("asl.library", 36)))
	{
    	    struct FileRequester *fr;
	    struct TagItem tags[] =
	    {
		{ASLFR_Window, (IPTR)ci->fh->window },
		{TAG_DONE	    	    	    	}
	    };

	    if ((fr = AllocAslRequest(ASL_FileRequest, tags)))
	    {
		if (AslRequest(fr, NULL))
		{
		    UBYTE c;
		    
	    	    strcpy(ci->match, fr->fr_Drawer);
		    AddPart(ci->match, fr->fr_File, sizeof(ci->match));

    	    	    if (ci->match[0])
		    {
			if (strchr(ci->match, ' ')) AddQuotes(ci, ci->match, sizeof(ci->match));

			c = ci->match[strlen(ci->match) - 1];		    
			if ((c != '/') && (c != ':'))
    	    		{
		    	    strncat(ci->match, " ", sizeof(ci->match));
			}

	    		InsertIntoConBuffer(conbase, ci, ci->match);
		    }
		    
		}
		FreeAslRequest(fr);
	    }

    	    CloseLibrary(AslBase);

	} /* if ((AslBase = OpenLibrary("asl.library", 36))) */
	
	CurrentDir(olddir);
	UnLock(lock);
    }
}

/****************************************************************************************/

static BOOL PreparePattern(struct conbase *conbase, struct completioninfo *ci)
{
    WORD parsecode;
    
    parsecode = ParsePatternNoCase(ci->filepart, ci->pattern, sizeof(ci->pattern));
    if (parsecode != -1)
    {
    	if (parsecode == 0)
	{
	    strncat(ci->filepart, "#?", sizeof(ci->filepart));
	    parsecode = ParsePatternNoCase(ci->filepart, ci->pattern, sizeof(ci->pattern));	    
	}
    }
       
    return (parsecode == 1);
    
}

/****************************************************************************************/

static void AddMatchNode(struct conbase *conbase, struct completioninfo *ci,
    	    	    	 STRPTR name, WORD type)
{
    struct matchnode 	*mn;
    struct Node     	*prev, *check;
    WORD    	    	 size;
    BOOL    	    	 exists = FALSE;
    
    size = strlen(name) + 1 + sizeof(struct matchnode) + ((type > 0) ? 1 : 0);
    
    if ((mn = AllocPooled(ci->pool, size)))
    {
    	strcpy(mn->name, name);
	mn->node.ln_Name = mn->name;
	
	if (type == 1) strcat(mn->name, "/");
	if (type == 2) strcat(mn->name, ":");
	
	/* Sort into matchlist */
	
	prev = NULL;
	ForeachNode(&ci->matchlist, check)
	{
	    WORD match;
	    
	    match = Stricmp(mn->name, ((struct matchnode *)check)->name);
	    if (match < 0) break;
	    if (match == 0)
	    {
	    	exists = TRUE;
		break;
	    }
	    
	    prev = check;
	}
	
	if (!exists)
	{
	    Insert(&ci->matchlist, (struct Node *)mn, prev);
	    ci->nummatchnodes++;
	}
	else
	{
	    FreePooled(ci->pool, mn, size);
	}
    }
}

/****************************************************************************************/

static void ScanDir(struct conbase *conbase, struct completioninfo *ci)
{
    struct FileInfoBlock *fib;
    BPTR    	    	  lock;

    if ((fib = AllocDosObject(DOS_FIB, 0)))
    {
	if ((lock = Lock(ci->dirpart, SHARED_LOCK)))
	{
    	    if (Examine(lock, fib))
	    {
		while(ExNext(lock, fib))
		{
		    if (MatchPatternNoCase(ci->pattern, fib->fib_FileName))
		    {
		    	BOOL isdir = (fib->fib_DirEntryType > 0);

    	    	    	AddMatchNode(conbase, ci, fib->fib_FileName, (isdir ? 1 : 0));
		    }
		}
	    }

	    UnLock(lock);
	}

	FreeDosObject(DOS_FIB, fib);
    }    
}

/****************************************************************************************/


static void ScanVol(struct conbase *conbase, struct completioninfo *ci)
{
    struct DosList *dlist;

    dlist = LockDosList(LDF_READ | LDF_VOLUMES | LDF_DEVICES | LDF_ASSIGNS);
    
    while ((dlist = NextDosEntry(dlist, LDF_VOLUMES | LDF_ASSIGNS | LDF_DEVICES)) != NULL)
    {
	if (MatchPatternNoCase(ci->pattern, dlist->dol_DevName))
	{
    	    AddMatchNode(conbase, ci, dlist->dol_DevName, 2);

	}
    }
    UnLockDosList(LDF_READ | LDF_VOLUMES | LDF_DEVICES | LDF_ASSIGNS);
}

static void DoScan(struct conbase *conbase, struct completioninfo *ci)
{
    BPTR    	    lock, olddir;
    
    if ((lock = DupLock(((struct Process *)ci->fh->lastwritetask)->pr_CurrentDir)))
    {
    	olddir = CurrentDir(lock);
	
	if (ci->dirpart[0] == 0) ScanVol(conbase, ci);
	ScanDir(conbase, ci);
		
	CurrentDir(olddir);
	UnLock(lock);
    }
}

/****************************************************************************************/

#define BUTTON_EXTRA_WIDTH  	16
#define BUTTON_EXTRA_HEIGHT 	6
#define BORDER_X    	    	4
#define BORDER_Y    	    	4
#define BUTTON_SPACING_X    	8
#define LV_BUTTON_SPACING_Y 	4
#define LV_EXTRA_HEIGHT     	4

#define ID_LISTVIEW 1
#define ID_OK       2
#define ID_CANCEL   3

/****************************************************************************************/

static BOOL DoChooseReq(struct conbase *conbase, struct completioninfo *ci)
{
    static char *oktext = "Ok";
    static char *canceltext = "Cancel";
    static char *titletext = "Select filename";
    
    struct RastPort 	temprp;
    struct DrawInfo 	*dri;
    struct Window    	*win;
    struct Gadget   	*gadlist, *gad, *lvgad;
    struct NewGadget 	ng;
    APTR    	    	vi;  
    WORD    	    	i, buttonwidth, buttonheight, visible_lv_lines;
    WORD    	    	winwidth, winheight, winleft, wintop;
    LONG    	    	sec, micro, secold, microold;
    BOOL    	    	retval = FALSE;
    
    if ((dri = GetScreenDrawInfo(ci->fh->window->WScreen)))
    {
    	if ((vi = GetVisualInfoA(ci->fh->window->WScreen, NULL)))
	{
    	    InitRastPort(&temprp);
	    SetFont(&temprp, dri->dri_Font);

    	    buttonwidth = TextLength(&temprp, oktext, strlen(oktext));
	    i = TextLength(&temprp, canceltext, strlen(canceltext));
	    buttonwidth = (buttonwidth > i) ? buttonwidth : i;
    	    buttonwidth += BUTTON_EXTRA_WIDTH;
	    
	    buttonheight = dri->dri_Font->tf_YSize + BUTTON_EXTRA_HEIGHT;

	    i = ci->nummatchnodes > 15 ? 15 : ci->nummatchnodes;
	    if (i < 4) i = 4;

    	    winheight = i * (dri->dri_Font->tf_YSize + 1) + 
	    		LV_EXTRA_HEIGHT +
			LV_BUTTON_SPACING_Y +
			buttonheight +
			BORDER_Y * 2;

	    winwidth = buttonwidth * 2 + BUTTON_SPACING_X + BORDER_X * 2;
	    i = ci->fh->window->WScreen->Width * 1 / 3;
	    if (i > winwidth) winwidth = i;

    	    winleft = ci->fh->window->WScreen->MouseX -
	    	      (winwidth + ci->fh->window->WScreen->WBorLeft +
		      ci->fh->window->WScreen->WBorRight) / 2;
	    wintop = ci->fh->window->WScreen->MouseY -
	    	      (winheight + ci->fh->window->WScreen->WBorTop +
		      dri->dri_Font->tf_YSize + 1 +
		      ci->fh->window->WScreen->WBorBottom) / 2;
		      
	    gad = CreateContext(&gadlist);
	    
	    ng.ng_LeftEdge   = ci->fh->window->WScreen->WBorLeft + BORDER_X;
	    ng.ng_TopEdge    = ci->fh->window->WScreen->WBorTop + dri->dri_Font->tf_YSize + 1 + BORDER_Y;
	    ng.ng_Width      = winwidth - BORDER_X * 2;
	    ng.ng_Height     = winheight - BORDER_Y * 2 - buttonheight - LV_BUTTON_SPACING_Y;
	    ng.ng_GadgetText = NULL;
	    ng.ng_TextAttr   = NULL;
	    ng.ng_GadgetID   = ID_LISTVIEW;
	    ng.ng_Flags      = 0;
	    ng.ng_VisualInfo = vi;
	    ng.ng_UserData   = 0;
	    
	    {
	    	struct TagItem lvtags[] =
		{
		    {GTLV_Labels    	, (IPTR)&ci->matchlist	},
		    {GTLV_ShowSelected	, 0 	    	    	},
		    {GTLV_Selected  	, 0 	    	    	},
		    {TAG_DONE	    	    	    	    	}
		};
	    	
		gad = lvgad = CreateGadgetA(LISTVIEW_KIND, gad, &ng, lvtags);

    	    	visible_lv_lines = (ng.ng_Height - LV_EXTRA_HEIGHT) / (dri->dri_Font->tf_YSize + 1);
	    }
	    
	    ng.ng_TopEdge += ng.ng_Height + LV_BUTTON_SPACING_Y;
	    ng.ng_Width = buttonwidth;
	    ng.ng_Height = buttonheight;
	    ng.ng_GadgetText = oktext;
	    ng.ng_GadgetID = ID_OK;
	    
	    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, NULL);
	    
	    ng.ng_LeftEdge += winwidth - buttonwidth - BORDER_X * 2;
	    ng.ng_GadgetText = canceltext;
	    ng.ng_GadgetID = ID_CANCEL;

	    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, NULL);
	    
	    if (gad)
	    {
	    	struct TagItem wintags[] =
		{
		    {WA_CustomScreen, (IPTR)ci->fh->window->WScreen },
		    {WA_Title	    , (IPTR)titletext	    	    },
		    {WA_CloseGadget , TRUE  	    	    	    },
		    {WA_DragBar     , TRUE  	    	    	    },
		    {WA_DepthGadget , TRUE  	    	    	    },
		    {WA_Activate    , TRUE  	    	    	    },
		    {WA_AutoAdjust  , TRUE  	    	    	    },
		    {WA_Left        , winleft	    	    	    },
		    {WA_Top 	    , wintop	    	    	    },
		    {WA_InnerWidth  , winwidth	    	    	    },
		    {WA_InnerHeight , winheight     	    	    },
		    {WA_IDCMP	    , IDCMP_CLOSEWINDOW |
		    	    	      IDCMP_RAWKEY  	|
				      IDCMP_VANILLAKEY	|
				      LISTVIEWIDCMP     |
				      BUTTONIDCMP   	    	    },
		    {WA_Gadgets     , (IPTR)gadlist 	    	    },
		    {TAG_DONE	    	    	    	    	    }
	    
		};
		
	    	if ((win = OpenWindowTagList(NULL, wintags)))
		{
		    BOOL done = FALSE;
		    BOOL doit = FALSE;
		    
		    CurrentTime(&secold, &microold);
		    
		    while (!done && !doit)
		    {
		    	struct IntuiMessage *msg;
			
		    	WaitPort(win->UserPort);
			
			while((msg = GT_GetIMsg(win->UserPort)))
			{
			    switch(msg->Class)
			    {
			    	case IDCMP_CLOSEWINDOW:
				    done = TRUE;
				    break;
				    
				case IDCMP_VANILLAKEY:
				    switch(msg->Code)
				    {
				    	case 27:
					    done = TRUE;
					    break;
					    
					case 13:
					    doit = TRUE;
					    break;
				    }
				    break;
				 
				case IDCMP_RAWKEY:
				    {
				    	WORD scroll = 0;
					BOOL page = FALSE;
					BOOL extreme = FALSE;
					
					switch(msg->Code)
					{
					    case RAWKEY_UP:
					    	scroll = -1;
						break;
						
					    case RAWKEY_DOWN:
					    	scroll = 1;
						break;
						
					    case RAWKEY_PAGEUP:
					    	scroll = -1;
						page = TRUE;
						break;
						
					    case RAWKEY_PAGEDOWN:
					    	scroll = 1;
						page = TRUE;
						break;
						
					    case RAWKEY_HOME:
					    	scroll = -1;
						extreme = TRUE;
						break;
						
					    case RAWKEY_END:
					    	scroll = 1;
						extreme = TRUE;
						break;
						
					    case RAWKEY_NM_WHEEL_UP:
					    	scroll = -3;
						break;
						
					    case RAWKEY_NM_WHEEL_DOWN:
					    	scroll = 3;
						break;
					}
					
					if (msg->Qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
					{
					    page = TRUE;					    
					}
					
					if (msg->Qualifier & (IEQUALIFIER_LALT | IEQUALIFIER_RALT | IEQUALIFIER_CONTROL))
					{
					    extreme = TRUE;					    
					}
					
					if (scroll)
					{
					    IPTR getsel;
					    struct TagItem tags[] =
					    {
					    	{GTLV_Selected, (IPTR)&getsel	},
						{TAG_IGNORE   , TRUE	    	},
						{TAG_DONE   	    	    	}
					    };
					    WORD sel;
					    
					    GT_GetGadgetAttrsA(lvgad, win, NULL, tags);
					    sel = (WORD)getsel;
					    
					    if (extreme)
					    {
					    	scroll *= ci->nummatchnodes;
					    }
					    else if (page)
					    {
					    	scroll *= visible_lv_lines;
					    }
					    
					    sel += scroll;
					    if (sel < 0) sel = 0;
					    if (sel >= ci->nummatchnodes) sel = ci->nummatchnodes - 1;
					    
					    tags[0].ti_Data = (IPTR)sel;
					    tags[1].ti_Tag  = GTLV_MakeVisible;
					    tags[1].ti_Data = (IPTR)sel;
					    
					    GT_SetGadgetAttrsA(lvgad, win, NULL, tags);
					}
				    }
				    break;
				       
				case IDCMP_GADGETUP:
				    gad = (struct Gadget *)msg->IAddress;
				    
				    switch(gad->GadgetID)
				    {
				    	case ID_OK:
					    doit = TRUE;
					    break;
					    
					case ID_CANCEL:
					    done = TRUE;
					    break;
					    
					case ID_LISTVIEW:
					    CurrentTime(&sec, &micro);
					    if (DoubleClick(secold, microold, sec, micro)) doit = TRUE;					    
					    secold = sec; microold = micro;
					    break;					    
				    }
				    break;
				    
			    } /* switch(msg->Class) */
			    
			    GT_ReplyIMsg(msg);
			    
			} /* while((msg = GT_GetIMsg(win->UserPort))) */
			
		    } /* while (!done && !doit) */
		    
		    if (doit)
		    {
		    	struct Node *node;
		    	IPTR sel;
			
			struct TagItem gettags[] =
			{
			    {GTLV_Selected, (IPTR)&sel	},
			    {TAG_DONE	    	    	}
			};
			
			GT_GetGadgetAttrsA(lvgad, win, NULL, gettags);
			
			i = 0;
			ForeachNode(&ci->matchlist, node)
			{
			    if ((WORD)sel == i)
			    {
		    	    	AddPart(ci->match, node->ln_Name, sizeof(ci->match));

			    	retval = TRUE;
			    	break;
			    }
			    i++;
			}
			
		    } /* if (doit) */
		    
		    CloseWindow(win);
		    
		} /* if ((win = OpenWindowTagList(NULL, wintags))) */
		
	    } /* if (gad) */

    	    FreeGadgets(gadlist);
	    
    	    DeinitRastPort(&temprp);
	    
	    FreeVisualInfo(vi);	
	    
	} /* if ((vi = GetVisualInfoA(ci->fh->window->WScreen, NULL))) */

    	FreeScreenDrawInfo(ci->fh->window->WScreen, dri);
	
    } /* if ((dri = GetScreenDrawInfo(fh->window->WScreen))) */
    
    return retval;
}

/****************************************************************************************/

void Completion(struct conbase *conbase, struct filehandle *fh)
{
    struct completioninfo *ci;

    if ((ci = InitCompletion(conbase, fh)))
    {
    	PrepareCompletion(conbase, ci);
    	
	if (!ci->dirpart[0] && !ci->filepart[0])
	{
	    DoFileReq(conbase, ci);
	}
	else
	{
	    if (PreparePattern(conbase, ci))
	    {
	    	BOOL doprint = FALSE;
		
	    	DoScan(conbase, ci);
		
		strncpy(ci->match, ci->dirpart, sizeof(ci->match));
		
		if (ci->nummatchnodes == 1)
		{
		    AddPart(ci->match,
		    	    ((struct matchnode *)GetHead(&ci->matchlist))->name,
			    sizeof(ci->match));
			    
		    doprint = TRUE;
		}
		else if (ci->nummatchnodes > 1)
		{
		    doprint = DoChooseReq(conbase, ci);
		}
		
		if (doprint)
		{
		    WORD backspaces;
    	    	    UBYTE c;
		    
		    if (strchr(ci->match, ' ')) AddQuotes(ci, ci->match, sizeof(ci->match));
		    
		    /* Insert as many backspaces in front of the string,
		       to erase whole "word" first (starting at ci->wordstart)
		       before reprinting expanded filename */
		       
		    backspaces = ci->fh->inputpos - ci->wordstart;
		    
		    memmove(ci->match + backspaces, ci->match, sizeof(ci->match) - backspaces);
		    memset(ci->match, 8, backspaces);

		    c = ci->match[strlen(ci->match) - 1];		    
		    if ((c != '/') && (c != ':'))
    	    	    {
		    	strncat(ci->match, " ", sizeof(ci->match));
		    }
		    
		    InsertIntoConBuffer(conbase, ci, ci->match);

		}
	    }
	}
	
    	CleanupCompletion(conbase, ci);
	
    } /* if ((ci = InitCompletion(conbase))) */
    
}

/****************************************************************************************/
