/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Support functions for console handler. 
    Lang: english
*/

/****************************************************************************************/

#define CYCLIC_HISTORY_WALK 0

/****************************************************************************************/


#include <proto/exec.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <exec/alerts.h>
#include <utility/tagitem.h>
#include <dos/exall.h>
#include <dos/dosasl.h>
#include <intuition/intuition.h>
#include <intuition/sghooks.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/input.h>
#include <devices/conunit.h>

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "con_handler_intern.h"
#include "support.h"
#include "completion.h"

#define ioReq(x) ((struct IORequest *)x)

void replypkt(struct DosPacket *dp, SIPTR res1)
{
	struct MsgPort *mp;
	struct Message *mn;
	
	mp = dp->dp_Port;
	mn = dp->dp_Link;
	mn->mn_Node.ln_Name = (char*)dp;
	dp->dp_Port = &((struct Process*)FindTask(NULL))->pr_MsgPort;
	dp->dp_Res1 = res1;
	PutMsg(mp, mn);
}
void replypkt2(struct DosPacket *dp, SIPTR res1, SIPTR res2)
{
	dp->dp_Res2 = res2;
	replypkt(dp, res1);
}


/******************************************************************************************/

static const UBYTE up_seq[] 			= {'A'};
static const UBYTE down_seq[] 		= {'B'};
static const UBYTE right_seq[]		= {'C'};
static const UBYTE left_seq[]			= {'D'};
static const UBYTE shift_up_seq[]		= {'T'};
static const UBYTE shift_down_seq[]		= {'S'}; 
static const UBYTE shift_right_seq[]		= {' ', '@'};
static const UBYTE shift_left_seq[]		= {' ', 'A'};
static const UBYTE shift_tab_seq[]		= {'Z'};
static const UBYTE help_seq[]			= {'?', '~'};
static const UBYTE f1_seq[]			= {'0', '~'};
static const UBYTE f2_seq[]			= {'1', '~'};
static const UBYTE f3_seq[]			= {'2', '~'};
static const UBYTE f4_seq[]			= {'3', '~'};
static const UBYTE f5_seq[]			= {'4', '~'};
static const UBYTE f6_seq[]			= {'5', '~'};
static const UBYTE f7_seq[]			= {'6', '~'};
static const UBYTE f8_seq[]			= {'7', '~'};
static const UBYTE f9_seq[]			= {'8', '~'};
static const UBYTE f10_seq[]			= {'9', '~'};
static const UBYTE paste_seq[]                = {'0', ' ', 'v'};
static const UBYTE f11_seq[]			= {'2', '0', '~'};
static const UBYTE f12_seq[]			= {'2', '1', '~'};
static const UBYTE shift_f1_seq[]		= {'1', '0', '~'};
static const UBYTE shift_f2_seq[]		= {'1', '1', '~'};
static const UBYTE shift_f3_seq[]		= {'1', '2', '~'};
static const UBYTE shift_f4_seq[]		= {'1', '3', '~'};
static const UBYTE shift_f5_seq[]		= {'1', '4', '~'};
static const UBYTE shift_f6_seq[]		= {'1', '5', '~'};
static const UBYTE shift_f7_seq[]		= {'1', '6', '~'};
static const UBYTE shift_f8_seq[]		= {'1', '7', '~'};
static const UBYTE shift_f9_seq[]		= {'1', '8', '~'};
static const UBYTE shift_f10_seq[]		= {'1', '9', '~'};
static const UBYTE shift_f11_seq[]		= {'3', '0', '~'};
static const UBYTE shift_f12_seq[]		= {'3', '1', '~'};
static const UBYTE insert_seq[] 		= {'4', '0', '~'};
static const UBYTE pageup_seq[] 		= {'4', '1', '~'};
static const UBYTE pagedown_seq[] 		= {'4', '2', '~'};
static const UBYTE pause_seq[] 		= {'4', '3', '~'};
static const UBYTE break_seq[] 		= {'5', '3', '~'};
static const UBYTE home_seq[] 		= {'4', '4', '~'};
static const UBYTE end_seq[]	 		= {'4', '5', '~'};
static const UBYTE shift_insert_seq[] 	= {'5', '0', '~'};
static const UBYTE shift_pageup_seq[] 	= {'5', '1', '~'};
static const UBYTE shift_pagedown_seq[] 	= {'5', '2', '~'};
static const UBYTE shift_home_seq[] 		= {'5', '4', '~'};
static const UBYTE shift_end_seq[]	 	= {'5', '5', '~'};

/* F11, F12, insert, pageup, pagedown, ... seq taken from
   RKRM: Devices/Console/Reading from the Console Device/Information about the Input Stream */
   
static CONST struct csimatch
{
    CONST UBYTE *seq;
    WORD len;
    WORD inp;
}
csimatchtable[] =
{
    {up_seq		, 1, INP_CURSORUP		},
    {down_seq		, 1, INP_CURSORDOWN		},
    {right_seq		, 1, INP_CURSORRIGHT		},
    {left_seq		, 1, INP_CURSORLEFT		},
    {shift_up_seq	, 1, INP_SHIFT_CURSORUP		},
    {shift_down_seq	, 1, INP_SHIFT_CURSORDOWN	},
    {shift_right_seq	, 2, INP_SHIFT_CURSORRIGHT	},
    {shift_left_seq	, 2, INP_SHIFT_CURSORLEFT	},
    {shift_tab_seq	, 1, INP_SHIFT_TAB		},
    {help_seq		, 2, INP_HELP			},
    {f1_seq		, 2, INP_F1			},
    {f2_seq		, 2, INP_F2			},
    {f3_seq		, 2, INP_F3			},
    {f4_seq		, 2, INP_F4			},
    {f5_seq		, 2, INP_F5			},
    {f6_seq		, 2, INP_F6			},
    {f7_seq		, 2, INP_F7			},
    {f8_seq		, 2, INP_F8			},
    {f9_seq		, 2, INP_F9			},
    {f10_seq		, 2, INP_F10			},
    {f11_seq		, 3, INP_F11			},
    {f12_seq		, 3, INP_F12			},
    {shift_f1_seq	, 3, INP_SHIFT_F1		},
    {shift_f2_seq	, 3, INP_SHIFT_F2		},
    {shift_f3_seq	, 3, INP_SHIFT_F3		},
    {shift_f4_seq	, 3, INP_SHIFT_F4		},
    {shift_f5_seq	, 3, INP_SHIFT_F5		},
    {shift_f6_seq	, 3, INP_SHIFT_F6		},
    {shift_f7_seq	, 3, INP_SHIFT_F7		},
    {shift_f8_seq	, 3, INP_SHIFT_F8		},
    {shift_f9_seq	, 3, INP_SHIFT_F9		},
    {shift_f10_seq	, 3, INP_SHIFT_F10		},
    {shift_f11_seq	, 3, INP_SHIFT_F11		},
    {shift_f12_seq	, 3, INP_SHIFT_F12		},
    {insert_seq		, 3, INP_INSERT			},
    {pageup_seq		, 3, INP_PAGEUP			},
    {pagedown_seq	, 3, INP_PAGEDOWN		},
    {pause_seq		, 3, INP_PAUSE			},
    {break_seq		, 3, INP_BREAK			},
    {home_seq		, 3, INP_HOME			},
    {end_seq		, 3, INP_END			},
    {shift_insert_seq	, 3, INP_SHIFT_INSERT		},
    {shift_pageup_seq	, 3, INP_SHIFT_PAGEUP		},
    {shift_pagedown_seq , 3, INP_SHIFT_PAGEDOWN		},
    {shift_home_seq	, 3, INP_SHIFT_HOME		},
    {shift_end_seq	, 3, INP_SHIFT_END		},
    {paste_seq          , 3, INP_PASTE                  },
    {0			, 0, 0				}
};


/******************************************************************************************/

static UBYTE hex2val(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    c &= ~0x20;
    return c - 'A' + 10;
}
/* strtoul() not used because it is much larger and uses .bss
 * TODO: Sanity checks. */
static IPTR string2val(const char *s, WORD len)
{
    IPTR v = 0;
     
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
        while (len-- > 0 && *s) {
            v <<= 4;
            v |= hex2val(*s);
            s++;
        }
    } else {
        while (len-- > 0 && *s) {
            v *= 10;
            v |= *s - '0';
            s++;
        }
    }
    return v;
}

BOOL parse_filename(struct filehandle *fh, char *filename, struct NewWindow *nw)
{
    char   *param;
    UBYTE   c;
    WORD    paramid = 1;
    LONG    paramval = 0;
    BOOL    ok = TRUE, done = FALSE, paramok = FALSE;

    ASSERT_VALID_PTR(fh);
    ASSERT_VALID_PTR(nw);

    param = filename;
    ASSERT_VALID_PTR(param);

    while (!done)
    {
	c = *filename++;
	
        switch(c)
	{
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	    	if (paramid <= 4)
		{
		    paramval *= 10;
		    paramval += c - '0';
		}
		paramok = TRUE;
		break;
	
	    case '\0':
	    	done = TRUE;
		/* fall through */
		
	    case '/':
	    	if (paramok)
		{
		    UWORD paramlen = filename - param - 1;

		    switch(paramid)
		    {
			case 1:
			    nw->LeftEdge = paramval;
			    break;
			    
			case 2:
			    nw->TopEdge = paramval;
			    break;
			    
			case 3:
			    nw->Width = paramval;
			    break;
			    
			case 4:
			    nw->Height = paramval;
			    break;
			    
			case 5:			
			    if ((fh->wintitle = AllocVec(paramlen + 1, MEMF_PUBLIC)))
			    {
				CopyMem(param, fh->wintitle, paramlen);
				fh->wintitle[paramlen] = '\0';
				nw->Title = fh->wintitle;
			    }
			    break;
			    
			default:
			    if (!strnicmp(param, "WAIT", paramlen))
			    {
				fh->flags |= FHFLG_WAIT;
			    }
			    else if (!strnicmp(param, "CLOSE", paramlen))
			    {
				nw->Flags |= WFLG_CLOSEGADGET;
			    }
			    else if (!strnicmp(param, "NOCLOSE", paramlen))
			    {
				nw->Flags &= ~WFLG_CLOSEGADGET;
			    }
			    else if (!strnicmp(param, "AUTO", paramlen))
			    {
				fh->flags |= FHFLG_AUTO;
			    }
			    else if (!strnicmp(param, "INACTIVE", paramlen))
			    {
				nw->Flags &= ~WFLG_ACTIVATE;
			    }
			    else if (!strnicmp(param, "NODEPTH", paramlen))
			    {
				nw->Flags &= ~WFLG_DEPTHGADGET;
			    }
			    else if (!strnicmp(param, "NOSIZE", paramlen))
			    {
				nw->Flags &= ~WFLG_SIZEGADGET;
			    }
			    else if (!strnicmp(param, "NODRAG", paramlen))
			    {
				nw->Flags &= ~WFLG_DRAGBAR;
			    }
			    else if (!strnicmp(param, "NOBORDER", paramlen))
			    {
				nw->Flags |= WFLG_BORDERLESS;
			    }
			    else if (!strnicmp(param, "BACKDROP", paramlen))
			    {
				nw->Flags |= WFLG_BACKDROP;
				nw->Flags &= ~(WFLG_DRAGBAR | WFLG_SIZEGADGET);
			    }
			    else if (!strnicmp(param, "SIMPLE", paramlen))
			    {
				/* TODO */
			    }
			    else if (!strnicmp(param, "SMART", paramlen))
			    {
				/* TODO */
			    }
			    else if (!strnicmp(param, "ALT", paramlen))
			    {
				/* TODO: style "ALT30/30/200/200" */
			    }
			    else if (!strnicmp(param, "WINDOW", 6))
			    {
			        /* Do we need some sanity checks here? */
				fh->otherwindow = (struct Window*)string2val(param + 6, paramlen - 6);
			    }
			    else if (!strnicmp(param, "SCREEN", 6))
			    {
				if ((fh->screenname = AllocVec(paramlen - 5, MEMF_PUBLIC)))
				{
				    CopyMem(param + 6, fh->screenname, paramlen - 6);
				    fh->screenname[paramlen - 6] = '\0';
				}
			    }
		    	    break;
			  
		    } /* switch(paramid) */
		    
		    paramok = FALSE;
		    
		} /* if (paramok) */
		
		paramval = 0;
		paramid++;
		param = filename;
		break;
		
	    default:
		if (paramid < 5)
		{
		    done = TRUE;
		    ok = FALSE;
		}
		else
		    paramok = TRUE;
		break;

	} /* switch(c) */
	
    } /* while (!done) */

    return ok;
}

/******************************************************************************************/


void do_write(struct filehandle *fh, APTR data, ULONG length)
{
    fh->conwriteio.io_Command = CMD_WRITE;
    fh->conwriteio.io_Data    = data;
    fh->conwriteio.io_Length  = length;

    DoIO((struct IORequest *)&fh->conwriteio);
}

/******************************************************************************************/

void do_movecursor(struct filehandle *fh, UBYTE direction, UBYTE howmuch)
{
    UBYTE seq[6]; /* 9B <N> <N> <N> <dir> <0> */
    ULONG size;
    
    if (howmuch > 0)
    {
	seq[0] = 0x9B;

	if (howmuch == 1)
	{
	    seq[1] = direction;
	    size = 2;
	} else {
            sprintf(&seq[1],"%d%c", howmuch, direction);
	    size = strlen(seq);
	}
    	
	do_write(fh, seq, size);
    }
}

/******************************************************************************************/

void do_cursorvisible(struct filehandle *fh, BOOL on)
{
    UBYTE seq[4];
    ULONG size = 0;
    
    seq[size++] = 0x9B;
    if (!on) seq[size++] = '0';
    seq[size++] = ' ';
    seq[size++] = 'p';
    
    do_write(fh, seq, size);
}

/******************************************************************************************/

void do_deletechar(struct filehandle *fh)
{
    UBYTE seq[] = {0x9B, 'P'};
    
    do_write(fh, seq, 2);
}

/******************************************************************************************/

void do_eraseinline(struct filehandle *fh)
{
    UBYTE seq[] = {0x9B, 'K'};
    
    do_write(fh, seq, 2);
}

/******************************************************************************************/

void do_eraseindisplay(struct filehandle *fh)
{
    UBYTE seq[] = {0x9B, 'J'};
    
    do_write(fh, seq, 2);
}
/******************************************************************************************/
static void copy_from_pastebuf(struct filehandle * fh)
{
  if (fh->conbufferpos >= fh->conbuffersize && 
      fh->pastebuffer &&
      fh->pastebufferpos < fh->pastebuffersize)
    {
      ULONG len = CONSOLEBUFFER_SIZE;
      ULONG pastelen = fh->pastebuffersize - fh->pastebufferpos;
      if (pastelen < len) len = pastelen;
      
      D(bug("Copying %d bytes from paste buffer\n",len));
      
      fh->conbufferpos = 0;
      CopyMem(fh->pastebuffer + fh->pastebufferpos, 
	      &fh->consolebuffer,
	      len);
      fh->conbuffersize = len;
      fh->pastebufferpos += len;
      if (fh->pastebufferpos >= fh->pastebuffersize) {
	FreeMem(fh->pastebuffer,PASTEBUFSIZE);
	fh->pastebuffer = 0;
      }
    }
}

WORD scan_input(struct filehandle *fh, UBYTE *buffer)
{
  CONST struct csimatch *match;
  UBYTE c;
  WORD result = INP_DONE;

  copy_from_pastebuf(fh);

    if (fh->conbufferpos < fh->conbuffersize)
    {
        c = fh->consolebuffer[fh->conbufferpos++];
	D(bug("scan_input: check char %d\n",c));
	switch(c)
	{
	    case 3:
	    case 4:
	    case 5:
	    case 6:
	        result = INP_CTRL_C - 3 + (WORD)c;
		break;
		
	    case 8:
	      /* FIXME: Ugh... Race condition, anyone? The qualifier might
		 have changed between the keypress and the time we do this */
	    	if (PeekQualifier() & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
		{
		    result = INP_SHIFT_BACKSPACE;
		} else {
	           result = INP_BACKSPACE;
		}
		break;
	
	    case 9:
	    	result = INP_TAB;
		break;

	    case 10:
	    	result = INP_LINEFEED;
		break;
		
	    case 12: /* CTRL-L */
		*buffer = c;		
	    	result = INP_ECHO_STRING;
		break;
		
	    case 13:
	    	result = INP_RETURN;
		break;
		
	    case 127:
	    	if (PeekQualifier() & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
		{
		   result = INP_SHIFT_DELETE;
		} else {
	           result = INP_DELETE;
		}
		break;
	
	    case 24:
	        result = INP_CONTROL_X;
		break;
	
	    case 28: /* CTRL-\ */
	    	result = INP_EOF;
		break;
		
	    case 0x9B: /* CSI */
	    	result = INP_UNKNOWN;
		
		match = csimatchtable;
		for(;match->seq;match++)
		{
		    if (!strncmp(match->seq, &(fh->consolebuffer[fh->conbufferpos]), match->len))
		    {
		        result = match->inp;
			fh->conbufferpos += match->len;
			break;
		    }
		}
	    	break;
	
	    default:
	        /* normal keys */		
		*buffer = c;		
		result = INP_STRING;
	        break;
		
	} /* switch(c) */
	
    } /* if (fh->conbufferpos < fh->conbuffersize) */
    
    D(bug("scan_input: result %d\n",result));
    
    return result;
}

/******************************************************************************************/

void add_to_history(struct filehandle *fh)
{
    BOOL add_to_history = FALSE;
    
    fh->inputbuffer[fh->inputsize] = '\0';
    
    /* Don't add emptry strings */
    if (fh->inputbuffer[fh->inputstart] == '\0') return;
    		    
    if (fh->historysize == 0)
    {
	add_to_history = TRUE;
    }
    else
    {
        WORD old_historypos;
	
	old_historypos = fh->historypos - 1;	
	if (old_historypos < 0) old_historypos = fh->historysize - 1;
	
	if (strcmp(&fh->inputbuffer[fh->inputstart], fh->historybuffer[old_historypos]))
	{
	    /* add to history only if different from last history entry */
	    
	    add_to_history = TRUE;
	}
    }
    
    if (add_to_history)
    {
	if (fh->historysize < CMD_HISTORY_SIZE) fh->historysize++;
	
	strcpy(fh->historybuffer[fh->historypos], &fh->inputbuffer[fh->inputstart]);

        fh->historypos++;
	if (fh->historypos >= CMD_HISTORY_SIZE) fh->historypos = 0;
    }
    
    fh->historyviewpos = fh->historypos;
}

/******************************************************************************************/

void history_walk(struct filehandle *fh, WORD inp)
{
    if (fh->historysize)
    {
    #if !CYCLIC_HISTORY_WALK
    	BOOL walk_to_empty_string = FALSE;
    #endif
	WORD len;

        switch(inp)
	{
	    case INP_SHIFT_CURSORUP:
		fh->historyviewpos = 0;
		break;
	
	    case INP_SHIFT_CURSORDOWN:
	    	fh->historyviewpos = fh->historysize - 1;;
		break;
		
	    case INP_CURSORUP:
    	    #if CYCLIC_HISTORY_WALK
	    	fh->historyviewpos--;
		if (fh->historyviewpos < 0) fh->historyviewpos = fh->historysize - 1;
    	    #else
		if (fh->historyviewpos != -1)
                {
                    fh->historyviewpos--;
		if (fh->historyviewpos < 0
		    && fh->historysize == CMD_HISTORY_SIZE)
		    fh->historyviewpos = CMD_HISTORY_SIZE - 1;
	        if (fh->historyviewpos == fh->historypos)
                    fh->historyviewpos = -1;
                }
                if (fh->historyviewpos == -1)
		    walk_to_empty_string = TRUE;
    	    #endif
		break;
		
	    case INP_CURSORDOWN:
	    #if CYCLIC_HISTORY_WALK
	    	fh->historyviewpos++;
		if (fh->historyviewpos >= fh->historysize) fh->historyviewpos = 0;
	    #else
		if (fh->historyviewpos != fh->historypos)
                {
                    if (fh->historyviewpos == -1
                        && fh->historysize == CMD_HISTORY_SIZE)
                        fh->historyviewpos = fh->historypos;
                    fh->historyviewpos = (fh->historyviewpos + 1)
                        % CMD_HISTORY_SIZE;
                }
		if (fh->historyviewpos == fh->historypos)
		    walk_to_empty_string = TRUE;
	    #endif
		break;
	}

	if (fh->inputpos > fh->inputstart)
	{
	    do_movecursor(fh, CUR_LEFT, fh->inputpos - fh->inputstart);
	}

	do_eraseinline(fh);

	fh->inputsize = fh->inputstart;
	fh->inputpos = fh->inputstart;

    #if !CYCLIC_HISTORY_WALK
    	if (!walk_to_empty_string)
	{
    #endif
	    len = strlen(fh->historybuffer[fh->historyviewpos]);
	    if (len > (INPUTBUFFER_SIZE - fh->inputstart))
	    {
		len = INPUTBUFFER_SIZE - fh->inputstart;
	    }

	    if (len > 0)
	    {
		CopyMem(fh->historybuffer[fh->historyviewpos],
			&fh->inputbuffer[fh->inputstart],
			len);

		fh->inputsize += len;
		fh->inputpos  += len;

		do_write(fh, &fh->inputbuffer[fh->inputstart], len);
	    }

    #if !CYCLIC_HISTORY_WALK
    	} /* if (!walk_to_empty_string) */
    #endif
    
    } /* if (fh->historysize) */
}

/******************************************************************************************/

LONG MakeConWindow(struct filehandle *fh)
{
    LONG err = 0;

    struct TagItem win_tags [] =
    {
	{WA_PubScreen	,0	    },
	{WA_AutoAdjust	,TRUE       },
	{WA_PubScreenName, 0        },
	{WA_PubScreenFallBack, TRUE },
	{TAG_DONE                   }
    };

    win_tags[2].ti_Data = (IPTR)fh->screenname;
    D(bug("[contask] Opening window on screen %s, IntuitionBase = 0x%p\n", fh->screenname, IntuitionBase));
    fh->window = OpenWindowTagList(&fh->nw, (struct TagItem *)win_tags);

    if (fh->window)
    {
    	D(bug("contask: window opened\n"));
	fh->conreadio->io_Data   = (APTR)fh->window;
	fh->conreadio->io_Length = sizeof (struct Window);

	if (0 == OpenDevice("console.device", CONU_SNIPMAP, ioReq(fh->conreadio), 0))
	{
	    const UBYTE lf_on[] = {0x9B, 0x32, 0x30, 0x68 }; /* Set linefeed mode    */

	    D(bug("contask: device opened\n"));

    	    fh->flags |= FHFLG_CONSOLEDEVICEOPEN;

	    fh->conwriteio = *fh->conreadio;
	    fh->conwriteio.io_Message.mn_ReplyPort = fh->conwritemp;

	    /* Turn the console into LF+CR mode so that both
	       linefeed and carriage return is done on
	    */
	    fh->conwriteio.io_Command	= CMD_WRITE;
	    fh->conwriteio.io_Data	= (APTR)lf_on;
	    fh->conwriteio.io_Length	= 4;

	    DoIO(ioReq(&fh->conwriteio));

	} /* if (0 == OpenDevice("console.device", CONU_STANDARD, ioReq(fh->conreadio), 0)) */
	else
	{
	    err = ERROR_INVALID_RESIDENT_LIBRARY;
	}
	if (err) CloseWindow(fh->window);

    } /* if (fh->window) */
    else
    {
        D(bug("[contask] Failed to open a window\n"));
	err = ERROR_NO_FREE_STORE;
    }
    
    return err;
}

/****************************************************************************************/

BOOL MakeSureWinIsOpen(struct filehandle *fh)
{
    D(bug("[contask] Console window handle: 0x%p\n", fh->window));
    if (fh->window)
    	return TRUE;
    return MakeConWindow(fh) ? TRUE : FALSE;
}

/****************************************************************************************/

static const STRPTR CONCLIP_PORTNAME = "ConClip.rendezvous";

struct MyEditHookMsg
{
    struct Message 	msg;
    struct SGWork	*sgw;
    WORD 		code;
};

static void do_paste(struct filehandle * fh)
{
  struct MsgPort replyport, *port;
  struct SGWork sgw;
  struct MyEditHookMsg msg;
  struct StringInfo sinfo;

  if (!(port = FindPort(CONCLIP_PORTNAME))) {
    D(bug("ConClip not running, but we got a ConClip paste request"));
    return;
  }

  D(bug("PASTE REQUEST!\n"));

  replyport.mp_Node.ln_Type	= NT_MSGPORT;
  replyport.mp_Node.ln_Name 	= NULL;
  replyport.mp_Node.ln_Pri 	= 0;
  replyport.mp_Flags 		= PA_SIGNAL;
  replyport.mp_SigBit 	= SIGB_SINGLE;
  replyport.mp_SigTask 	= FindTask(NULL);
  NEWLIST(&replyport.mp_MsgList);
			    
  msg.msg.mn_Node.ln_Type 	= NT_MESSAGE;
  msg.msg.mn_ReplyPort 	= &replyport;
  msg.msg.mn_Length 		= sizeof(msg);
  
  msg.code = 'V';
  msg.sgw  = &sgw;

  /* FIXME: Ensure no fields are left uninitialized */

  sgw.Gadget = 0;
  sgw.WorkBuffer = AllocMem(PASTEBUFSIZE,MEMF_CLEAR | MEMF_ANY);
  sgw.PrevBuffer = 0;
  sgw.IEvent = 0;
  sgw.Code = 'V';
  sgw.Actions = 0;
  sgw.LongInt = 0;
  sgw.GadgetInfo = 0;
  sgw.EditOp = EO_BIGCHANGE;
  sgw.BufferPos = 0;
  sgw.NumChars = 0;

  /* ConClip only ever looks at MaxChars in StringInfo */
  sinfo.MaxChars = PASTEBUFSIZE;
  sgw.StringInfo = &sinfo;
  
  SetSignal(0, SIGF_SINGLE);
  PutMsg(port, &msg.msg);
  WaitPort(&replyport);

  D(bug("Pasting %d bytes\n",sgw.BufferPos));

  if (fh->pastebuffer) FreeMem(fh->pastebuffer,PASTEBUFSIZE);
  fh->pastebuffer = sgw.WorkBuffer;
  fh->pastebuffersize = sgw.BufferPos;
  fh->pastebufferpos = 0;
}


/****************************************************************************************/

BOOL process_input(struct filehandle *fh)
{
  UBYTE c;
  WORD inp;
  while((inp = scan_input(fh, &c)) != INP_DONE)
    {
      D(bug("Input Code: %d\n",inp));
      
      switch(inp)
	{
	case INP_CURSORLEFT:
	  if (fh->inputpos > fh->inputstart)
	    {
	      fh->inputpos--;
	      do_movecursor(fh, CUR_LEFT, 1);
	    }
	  break;
	  
	case INP_SHIFT_CURSORLEFT: /* move to beginning of line */
	case INP_HOME:
	  if (fh->inputpos > fh->inputstart)
	    {
	      do_movecursor(fh, CUR_LEFT, fh->inputpos - fh->inputstart);
	      fh->inputpos = fh->inputstart;
	    }
	  break;
	  
	case INP_CURSORRIGHT:
	  if (fh->inputpos < fh->inputsize)
	    {
	      fh->inputpos++;
	      do_movecursor(fh, CUR_RIGHT, 1);
	    }
	  break;
	  
	case INP_SHIFT_CURSORRIGHT: /* move to end of line */
	case INP_END:
	  if (fh->inputpos != fh->inputsize)
	    {
	      do_movecursor(fh, CUR_RIGHT, fh->inputsize - fh->inputpos);
	      fh->inputpos = fh->inputsize;
	    }
	  break;
	  
	case INP_CURSORUP: /* walk through cmd history */
	case INP_CURSORDOWN:
	case INP_SHIFT_CURSORUP:
	case INP_SHIFT_CURSORDOWN:
	  history_walk(fh, inp);
	  break;
	  
	case INP_BACKSPACE:
	  if (fh->inputpos > fh->inputstart)
	    {
	      do_movecursor(fh, CUR_LEFT, 1);
	      
	      if (fh->inputpos == fh->inputsize)
		{
		  do_deletechar(fh);
		  
		  fh->inputsize--;
		  fh->inputpos--;
		} else {
		WORD chars_right = fh->inputsize - fh->inputpos;
		
		fh->inputsize--;
		fh->inputpos--;
		
		do_cursorvisible(fh, FALSE);
		do_write(fh, &fh->inputbuffer[fh->inputpos + 1], chars_right);
		do_deletechar(fh);
		do_movecursor(fh, CUR_LEFT, chars_right);
		do_cursorvisible(fh, TRUE);
		
		memmove(&fh->inputbuffer[fh->inputpos], &fh->inputbuffer[fh->inputpos + 1], chars_right);
		
	      }
	    }
	  break;
	  
	case INP_SHIFT_BACKSPACE:
	  if (fh->inputpos > fh->inputstart)
	    {
	      do_movecursor(fh, CUR_LEFT, fh->inputpos - fh->inputstart);
	      if (fh->inputpos == fh->inputsize)
		{
		  do_eraseinline(fh);
		  
		  fh->inputpos = fh->inputsize = fh->inputstart;
		} else {
		WORD chars_right = fh->inputsize - fh->inputpos;
		
		do_cursorvisible(fh, FALSE);
		do_write(fh, &fh->inputbuffer[fh->inputpos], chars_right);
		do_eraseinline(fh);
		do_movecursor(fh, CUR_LEFT, chars_right);
		do_cursorvisible(fh, TRUE);
		
		memmove(&fh->inputbuffer[fh->inputstart], &fh->inputbuffer[fh->inputpos], chars_right);
		
		fh->inputsize -= (fh->inputpos - fh->inputstart);
		fh->inputpos = fh->inputstart;
	      }
	    }
	  break;
	  
	case INP_DELETE:
	  if (fh->inputpos < fh->inputsize)
	    {
	      fh->inputsize--;
	      
	      if (fh->inputpos == fh->inputsize)
		{
		  do_deletechar(fh);
		} else {
		WORD chars_right = fh->inputsize - fh->inputpos;
		
		do_cursorvisible(fh, FALSE);
		do_write(fh, &fh->inputbuffer[fh->inputpos + 1], chars_right);
		do_deletechar(fh);
		do_movecursor(fh, CUR_LEFT, chars_right);
		do_cursorvisible(fh, TRUE);
		
		memmove(&fh->inputbuffer[fh->inputpos], &fh->inputbuffer[fh->inputpos + 1], chars_right);
	      }
	    }
	  break;
	  
	case INP_SHIFT_DELETE:
	  if (fh->inputpos < fh->inputsize)
	    {
	      fh->inputsize = fh->inputpos;
	      do_eraseinline(fh);
	    }
	  break;
	  
	case INP_CONTROL_X:
	  if ((fh->inputsize - fh->inputstart) > 0)
	    {
	      if (fh->inputpos > fh->inputstart)
		{
		  do_movecursor(fh, CUR_LEFT, fh->inputpos - fh->inputstart);
		}
	      do_eraseinline(fh);
	      
	      fh->inputpos = fh->inputsize = fh->inputstart;
	    }
	  break;
	  
	case INP_ECHO_STRING:
	  do_write(fh, &c, 1);
	  break;
	  
	case INP_STRING:
	  if (fh->inputsize < INPUTBUFFER_SIZE)
	    {
	      do_write(fh, &c, 1);
	      
	      if (fh->inputpos == fh->inputsize)
		{
		  fh->inputbuffer[fh->inputpos++] = c;
		  fh->inputsize++;
		} else {
		WORD chars_right = fh->inputsize - fh->inputpos;
		
		do_cursorvisible(fh, FALSE);
		do_write(fh, &fh->inputbuffer[fh->inputpos], chars_right);
		do_movecursor(fh, CUR_LEFT, chars_right);
		do_cursorvisible(fh, TRUE);
		
		memmove(&fh->inputbuffer[fh->inputpos + 1], &fh->inputbuffer[fh->inputpos], chars_right);
		fh->inputbuffer[fh->inputpos++] = c;
		fh->inputsize++;
	      }
	    }
	  break;
	  
	case INP_EOF:
	  D(bug("[CON] Read EOF (window closing)\n"));

	  if (fh->flags & FHFLG_WAITFORCLOSE)
	  	return TRUE;

	  fh->flags |= FHFLG_EOF;
	  if (fh->flags & FHFLG_AUTO && fh->window)
	  {
	      CloseWindow(fh->window);
	      fh->window = NULL;
	  }
	  
	  /* fall through */
	  
	case INP_RETURN:
	  if (fh->inputsize < INPUTBUFFER_SIZE)
	  {
	      if (inp != INP_EOF)
	      {
		  c = '\n';
		  do_write(fh, &c, 1);
		  add_to_history(fh);
		  
		  fh->inputbuffer[fh->inputsize++] = '\n';
	      }
	      
	      fh->inputstart = fh->inputsize;
	      fh->inputpos = fh->inputstart;
	      
	      if (fh->inputsize)
	      	HandlePendingReads(fh);
	      
	      if ((fh->flags & FHFLG_EOF) && (fh->flags & FHFLG_READPENDING))
	      {
		  struct Message *msg = (struct Message*)RemHead((struct List *)&fh->pendingReads);
		  struct DosPacket *dp = (struct DosPacket*)msg->mn_Node.ln_Name;
		  
		  if (dp)
		  {
		  	replypkt2(dp, 0, 0);
		        fh->flags &= ~FHFLG_EOF;
		  }
		  
		  if (IsListEmpty(&fh->pendingReads))
		      fh->flags &= ~FHFLG_READPENDING;
	      }
	      
	  } /* if (fh->inputsize < INPUTBUFFER_SIZE) */
	  break;
	  
	case INP_LINEFEED:
	  if (fh->inputsize < INPUTBUFFER_SIZE)
	    {
	      c = '\n';
	      do_write(fh, &c, 1);
	      add_to_history(fh);
	      
	      fh->inputbuffer[fh->inputsize++] = c;
	      fh->inputstart = fh->inputsize;
	      fh->inputpos = fh->inputsize;
	    }
	  break;
	  
	case INP_CTRL_C:
	case INP_CTRL_D:
	case INP_CTRL_E:
	case INP_CTRL_F:
	  if (fh->breaktask)
	    {
	      Signal(fh->breaktask, 1L << (12 + inp - INP_CTRL_C));
	    }
	  break;
	  
	case INP_TAB:
	  Completion(fh);
	  break;

	case INP_PASTE:
	  do_paste(fh);
	  break;
	  
	} /* switch(inp) */
      
    } /* while((inp = scan_input(fh, &c)) != INP_DONE) */

    return FALSE;
}


BOOL answer_write_request(struct filehandle *fh, struct DosPacket *dp)
{
    UBYTE *buffer = (UBYTE*)dp->dp_Arg2;
    LONG length = dp->dp_Arg3;

#if RMB_FREEZES_OUTPUT
    struct Window 	*conwindow;
    
    conwindow = ((struct ConUnit *)fh->conwriteio.io_Unit)->cu_Window;
    
    while((PeekQualifier() & IEQUALIFIER_RBUTTON) &&
    	  conwindow && (conwindow == IntuitionBase->ActiveWindow))
    {
        Delay(2);
    }
#endif

    if ((dp->dp_Port->mp_Flags & PF_ACTION) == PA_SIGNAL &&
        dp->dp_Port->mp_SigTask)
    {
    	fh->lastwritetask = dp->dp_Port->mp_SigTask;
    }


    do_write(fh, buffer, length);
    replypkt2(dp, length, 0);
    
    return TRUE;
}

void answer_read_request(struct filehandle *fh, struct DosPacket *dp, ULONG dp_Arg3)
{
    ULONG readlen;

    readlen = (fh->inputsize < dp_Arg3) ? fh->inputsize : dp_Arg3;

    CopyMem(fh->inputbuffer, (UBYTE*)dp->dp_Arg2, readlen);
    CopyMem(fh->inputbuffer + readlen, fh->inputbuffer, fh->inputsize - readlen);

    fh->inputsize  -= readlen;
    fh->inputpos   -= readlen;
    fh->inputstart -= readlen;
    
    replypkt2(dp, readlen, 0);
}

void HandlePendingReads(struct filehandle *fh)
{
    if (fh->flags & FHFLG_READPENDING)
    {			    
    	struct DosPacket *dp;
    	struct Message *msg, *next_msg;

	ForeachNodeSafe(&fh->pendingReads, msg, next_msg)
	{
            Remove((struct Node *)msg);
            dp = (struct DosPacket*)msg->mn_Node.ln_Name;
	    answer_read_request(fh, dp, dp->dp_Arg3);

	    if (fh->inputsize == 0)
	    	break;

	}
	if (IsListEmpty(&fh->pendingReads))
		fh->flags &= ~FHFLG_READPENDING;
    }

    if (fh->inputsize)
    {
	fh->flags |= FHFLG_CANREAD;
	fh->canreadsize = fh->inputsize;
    }
}

void con_read(struct filehandle *fh, struct DosPacket *dp)
{
    if (fh->flags & FHFLG_CANREAD)
    {
        ULONG readlen = (fh->canreadsize < dp->dp_Arg3) ? fh->canreadsize : dp->dp_Arg3;

	answer_read_request(fh, dp, readlen);

	fh->canreadsize -= readlen;
	if (fh->canreadsize == 0)
		fh->flags &= ~FHFLG_CANREAD;
	
    }
    else
    {    
    	if (fh->flags & FHFLG_EOF)
	{
	    replypkt2(dp, 0, 0);
	    fh->flags &= ~FHFLG_EOF;
	}
    	else if (fh->flags & FHFLG_RAW)
    	{
    	    replypkt2(dp, 0, 0);
    	}
	else
	{
	    AddTail((struct List *)&fh->pendingReads, (struct Node *)dp->dp_Link);
	    fh->flags |= FHFLG_READPENDING;
	}
    }
}
