/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Support functions for console handler.
*/

/****************************************************************************************/

#define CYCLIC_HISTORY_WALK 0

/****************************************************************************************/

#define SDEBUG 0
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/input.h>

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
#include <devices/conunit.h>

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
    mn->mn_Node.ln_Name = (char*) dp;
    dp->dp_Port = &((struct Process*) FindTask(NULL))->pr_MsgPort;
    dp->dp_Res1 = res1;
    PutMsg(mp, mn);
}
void replypkt2(struct DosPacket *dp, SIPTR res1, SIPTR res2)
{
    dp->dp_Res2 = res2;
    replypkt(dp, res1);
}


/******************************************************************************************/

/* The character sequences that can follow a CSI in the input stream, with
   the input codes they stand for. The first matching entry counts. An entry
   with length 0 ends the table.

   F11, F12, insert, pageup, pagedown, ... seq taken from
   RKRM: Devices/Console/Reading from the Console Device/Information about the Input Stream */

static CONST struct csimatch
{
    UBYTE seq[3];
    UBYTE len;
    UBYTE inp;
}
csimatchtable[] =
{
    {{'A'          }, 1, INP_CURSORUP          },
    {{'B'          }, 1, INP_CURSORDOWN        },
    {{'C'          }, 1, INP_CURSORRIGHT       },
    {{'D'          }, 1, INP_CURSORLEFT        },
    {{'T'          }, 1, INP_SHIFT_CURSORUP    },
    {{'S'          }, 1, INP_SHIFT_CURSORDOWN  },
    {{' ', '@'     }, 2, INP_SHIFT_CURSORRIGHT },
    {{' ', 'A'     }, 2, INP_SHIFT_CURSORLEFT  },
    {{'Z'          }, 1, INP_SHIFT_TAB         },
    {{'?', '~'     }, 2, INP_HELP              },
    {{'0', '~'     }, 2, INP_F1                },
    {{'1', '~'     }, 2, INP_F2                },
    {{'2', '~'     }, 2, INP_F3                },
    {{'3', '~'     }, 2, INP_F4                },
    {{'4', '~'     }, 2, INP_F5                },
    {{'5', '~'     }, 2, INP_F6                },
    {{'6', '~'     }, 2, INP_F7                },
    {{'7', '~'     }, 2, INP_F8                },
    {{'8', '~'     }, 2, INP_F9                },
    {{'9', '~'     }, 2, INP_F10               },
    {{'2', '0', '~'}, 3, INP_F11               },
    {{'2', '1', '~'}, 3, INP_F12               },
    {{'1', '0', '~'}, 3, INP_SHIFT_F1          },
    {{'1', '1', '~'}, 3, INP_SHIFT_F2          },
    {{'1', '2', '~'}, 3, INP_SHIFT_F3          },
    {{'1', '3', '~'}, 3, INP_SHIFT_F4          },
    {{'1', '4', '~'}, 3, INP_SHIFT_F5          },
    {{'1', '5', '~'}, 3, INP_SHIFT_F6          },
    {{'1', '6', '~'}, 3, INP_SHIFT_F7          },
    {{'1', '7', '~'}, 3, INP_SHIFT_F8          },
    {{'1', '8', '~'}, 3, INP_SHIFT_F9          },
    {{'1', '9', '~'}, 3, INP_SHIFT_F10         },
    {{'3', '0', '~'}, 3, INP_SHIFT_F11         },
    {{'3', '1', '~'}, 3, INP_SHIFT_F12         },
    {{'4', '0', '~'}, 3, INP_INSERT            },
    {{'4', '1', '~'}, 3, INP_PAGEUP            },
    {{'4', '2', '~'}, 3, INP_PAGEDOWN          },
    {{'4', '3', '~'}, 3, INP_PAUSE             },
    {{'5', '3', '~'}, 3, INP_BREAK             },
    {{'4', '4', '~'}, 3, INP_HOME              },
    {{'4', '5', '~'}, 3, INP_END               },
    {{'5', '0', '~'}, 3, INP_SHIFT_INSERT      },
    {{'5', '1', '~'}, 3, INP_SHIFT_PAGEUP      },
    {{'5', '2', '~'}, 3, INP_SHIFT_PAGEDOWN    },
    {{'5', '4', '~'}, 3, INP_SHIFT_HOME        },
    {{'5', '5', '~'}, 3, INP_SHIFT_END         },
    {{'0', ' ', 'v'}, 3, INP_PASTE             },
    {{0}            , 0, 0                     }
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

    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    {
        s += 2;
        while (len-- > 0 && *s)
        {
            v <<= 4;
            v |= hex2val(*s);
            s++;
        }
    }
    else
    {
        while (len-- > 0 && *s)
        {
            v *= 10;
            v += *s - '0';
            s++;
        }
    }
    return v;
}

/* The keywords of a window specification that only set or clear flags.
   A parameter selects the first keyword that it is an abbreviation of, so
   the order of the table matters. BOOT is a private parameter. */
static const struct con_keyword
{
    char name[9];
    UBYTE nodrag;               /* TRUE: the window can not be dragged */
    UWORD fhflags;              /* FHFLG_ bits to set */
    UWORD set;                  /* WFLG_ bits to set */
    UWORD clear;                /* WFLG_ bits to clear */
}
con_keywords[] =
{
    {"WAIT",     FALSE, FHFLG_WAIT,    0,                0},
    {"CLOSE",    FALSE, 0,             WFLG_CLOSEGADGET, 0},
    {"NOCLOSE",  FALSE, 0,             0,                WFLG_CLOSEGADGET},
    {"AUTO",     FALSE, FHFLG_AUTO,    0,                0},
    {"INACTIVE", FALSE, 0,             0,                WFLG_ACTIVATE},
    {"NODEPTH",  FALSE, 0,             0,                WFLG_DEPTHGADGET},
    {"NOSIZE",   FALSE, 0,             0,                WFLG_SIZEGADGET},
    {"NODRAG",   TRUE,  0,             0,                WFLG_DRAGBAR},
    {"NOBORDER", FALSE, 0,             WFLG_BORDERLESS,  0},
    {"BACKDROP", FALSE, 0,             WFLG_BACKDROP,    WFLG_DRAGBAR | WFLG_SIZEGADGET},
    {"BOOT",     FALSE, FHFLG_BOOTCON, 0,                0}
};
#define NUM_CON_KEYWORDS (sizeof(con_keywords) / sizeof(con_keywords[0]))

BOOL parse_filename(struct filehandle *fh, char *filename, struct NewWindow *nw)
{
    char *param;
    UBYTE c;
    WORD paramid = 1;
    LONG paramval = 0;
    BOOL ok = TRUE, done = FALSE, paramok = FALSE, nodrag = FALSE;

    ASSERT_VALID_PTR(fh);
    ASSERT_VALID_PTR(nw);

    param = filename;
    ASSERT_VALID_PTR(param);

    while (!done)
    {
        c = *filename++;

        switch (c)
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
                UWORD i;

                switch (paramid)
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
                    for (i = 0; i < NUM_CON_KEYWORDS; i++)
                    {
                        if (!strnicmp(param, con_keywords[i].name, paramlen))
                            break;
                    }

                    if (i < NUM_CON_KEYWORDS)
                    {
                        fh->flags |= con_keywords[i].fhflags;
                        nw->Flags |= con_keywords[i].set;
                        nw->Flags &= ~(ULONG)con_keywords[i].clear;
                        if (con_keywords[i].nodrag)
                            nodrag = TRUE;
                    }
                    /* TODO: SIMPLE, SMART and ALT (style "ALT30/30/200/200")
                       are not supported. Their names, and abbreviations of
                       them, are ignored. */
                    else if (!strnicmp(param, "WINDOW", 6))
                    {
                        /* Do we need some sanity checks here? */
                        fh->otherwindow = (struct Window*) string2val(param + 6, paramlen - 6);
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

    if (nodrag)
        nw->Flags &= ~WFLG_CLOSEGADGET;

    return ok;
}

/******************************************************************************************/

void do_write(struct filehandle *fh, APTR data, ULONG length)
{
    fh->conwriteio.io_Command = CMD_WRITE;
    fh->conwriteio.io_Data = data;
    fh->conwriteio.io_Length = length;
    fh->conwriteio.io_Actual = 0;
    fh->conwriteio.io_Error = 0;

    DoIO((struct IORequest *) &fh->conwriteio);
}

/******************************************************************************************/

void do_movecursor(struct filehandle *fh, UBYTE direction, UWORD howmuch)
{
    UBYTE seq[8]; /* 9B <N...> <dir> <0> */
    ULONG size;

    if (howmuch > 0)
    {
        seq[0] = 0x9B;

        if (howmuch == 1)
        {
            seq[1] = direction;
            size = 2;
        }
        else
        {
            sprintf(&seq[1], "%d%c", howmuch, direction);
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
    if (!on)
        seq[size++] = '0';
    seq[size++] = ' ';
    seq[size++] = 'p';

    do_write(fh, seq, size);
}

/******************************************************************************************/

/* Send CSI followed by one command character */
static void do_csi(struct filehandle *fh, UBYTE cmd)
{
    UBYTE seq[2];

    seq[0] = 0x9B;
    seq[1] = cmd;
    do_write(fh, seq, 2);
}

/******************************************************************************************/

void do_deletechar(struct filehandle *fh)
{
    do_csi(fh, 'P');
}

/******************************************************************************************/

void do_eraseinline(struct filehandle *fh)
{
    do_csi(fh, 'K');
}

/******************************************************************************************/

void do_eraseindisplay(struct filehandle *fh)
{
    do_csi(fh, 'J');
}

/******************************************************************************************/

/* Redraw the rest of the input line after an edit: with the cursor hidden
   print 'count' characters, send CSI 'erase' if erase is not 0, and move the
   cursor back to where it was. */
static void redraw_tail(struct filehandle *fh, UBYTE *tail, WORD count,
    UBYTE erase)
{
    do_cursorvisible(fh, FALSE);
    do_write(fh, tail, count);
    if (erase)
        do_csi(fh, erase);
    do_movecursor(fh, CUR_LEFT, count);
    do_cursorvisible(fh, TRUE);
}

/******************************************************************************************/
static void copy_from_pastebuf(struct filehandle * fh)
{
    if (fh->conbufferpos >= fh->conbuffersize && fh->pastebuffer && fh->pastebufferpos < fh->pastebuffersize)
    {
        ULONG len = CONSOLEBUFFER_SIZE;
        ULONG pastelen = fh->pastebuffersize - fh->pastebufferpos;
        if (pastelen < len)
            len = pastelen;

        D(bug("Copying %d bytes from paste buffer\n", len));

        fh->conbufferpos = 0;
        CopyMem(fh->pastebuffer + fh->pastebufferpos, &fh->consolebuffer, len);
        fh->conbuffersize = len;
        fh->pastebufferpos += len;
        if (fh->pastebufferpos >= fh->pastebuffersize)
        {
            FreeMem(fh->pastebuffer, PASTEBUFSIZE);
            fh->pastebuffer = 0;
        }
    }
}

/* The input codes of the control characters. A character without an entry
   is passed on like a normal key. */
static const UBYTE ctrl_inp[32] =
{
    [1]  = INP_HOME,            /* CTRL-A */
    [3]  = INP_CTRL_C,
    [4]  = INP_CTRL_D,
    [5]  = INP_CTRL_E,
    [6]  = INP_CTRL_F,
    [8]  = INP_BACKSPACE,       /* INP_SHIFT_BACKSPACE with shift */
    [9]  = INP_TAB,
    [10] = INP_LINEFEED,
    [11] = INP_CONTROL_K,
    [12] = INP_ECHO_STRING,     /* CTRL-L */
    [13] = INP_RETURN,
    [21] = INP_SHIFT_BACKSPACE, /* CTRL-U */
    [23] = INP_CONTROL_W,
    [24] = INP_CONTROL_X,
    [25] = INP_CONTROL_Y,
    [26] = INP_END,             /* CTRL-Z */
    [28] = INP_EOF              /* CTRL-\ */
};

/* scan_input() turns backspace and delete into their shifted codes by
   adding one, and uses INP_DONE for "no entry in ctrl_inp[]" */
_Static_assert(INP_SHIFT_BACKSPACE == INP_BACKSPACE + 1 &&
               INP_SHIFT_DELETE == INP_DELETE + 1 && INP_DONE == 0,
               "scan_input() depends on the order of the INP_ codes");

WORD scan_input(struct filehandle *fh, UBYTE *buffer)
{
    CONST
    struct csimatch *match;
    UBYTE c;
    WORD result = INP_DONE;

    copy_from_pastebuf(fh);

    if (fh->conbufferpos < fh->conbuffersize)
    {
        c = fh->consolebuffer[fh->conbufferpos++];
        D(bug("scan_input: check char %d\n", c));
        if (c == 0x9B) /* CSI */
        {
            result = INP_UNKNOWN;

            match = csimatchtable;
            for (; match->len; match++)
            {
                if (!strncmp(match->seq, &(fh->consolebuffer[fh->conbufferpos]), match->len))
                {
                    result = match->inp;
                    fh->conbufferpos += match->len;
                    break;
                }
            }
        }
        else
        {
            if (c < sizeof(ctrl_inp))
                result = ctrl_inp[c];
            else if (c == 127)
                result = INP_DELETE;

            if (result == INP_DONE)
            {
                /* normal keys */
                result = INP_STRING;
            }

            if (result == INP_BACKSPACE || result == INP_DELETE)
            {
                /* FIXME: Ugh... Race condition, anyone? The qualifier might
                 have changed between the keypress and the time we do this */
                if (PeekQualifier() & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT))
                {
                    /* INP_SHIFT_BACKSPACE or INP_SHIFT_DELETE */
                    result++;
                }
            }
            else if (result == INP_STRING || result == INP_ECHO_STRING)
            {
                *buffer = c;
            }
        }

    } /* if (fh->conbufferpos < fh->conbuffersize) */

    D(bug("scan_input: result %d\n", result));

    return result;
}

/******************************************************************************************/

void add_to_history(struct filehandle *fh)
{
    BOOL add_to_history = FALSE;

    fh->inputbuffer[fh->inputsize] = '\0';

    /* Don't add emptry strings */
    if (fh->inputbuffer[fh->inputstart] == '\0')
        return;

    if (fh->historysize == 0)
    {
        add_to_history = TRUE;
    }
    else
    {
        WORD old_historypos;

        old_historypos = fh->historypos - 1;
        if (old_historypos < 0)
            old_historypos = fh->historysize - 1;

        if (strcmp(&fh->inputbuffer[fh->inputstart], fh->historybuffer[old_historypos]))
        {
            /* add to history only if different from last history entry */

            add_to_history = TRUE;
        }
    }

    if (add_to_history)
    {
        if (fh->historysize < CMD_HISTORY_SIZE)
            fh->historysize++;

        strcpy(fh->historybuffer[fh->historypos], &fh->inputbuffer[fh->inputstart]);

        fh->historypos++;
        if (fh->historypos >= CMD_HISTORY_SIZE)
            fh->historypos = 0;
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

        switch (inp)
        {
        case INP_SHIFT_CURSORUP:
            fh->historyviewpos = (fh->historysize == CMD_HISTORY_SIZE)
                ? fh->historypos : 0;
            break;

        case INP_SHIFT_CURSORDOWN:
            fh->historyviewpos = fh->historypos - 1;
            if (fh->historyviewpos < 0)
                fh->historyviewpos = fh->historysize - 1;
            break;

        case INP_CURSORUP:
#if CYCLIC_HISTORY_WALK
            fh->historyviewpos--;
            if (fh->historyviewpos < 0) fh->historyviewpos = fh->historysize - 1;
#else
            if (fh->historyviewpos != -1)
            {
                fh->historyviewpos--;
                if (fh->historyviewpos < 0 && fh->historysize == CMD_HISTORY_SIZE)
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
                if (fh->historyviewpos == -1 && fh->historysize == CMD_HISTORY_SIZE)
                    fh->historyviewpos = fh->historypos;
                fh->historyviewpos = (fh->historyviewpos + 1) % CMD_HISTORY_SIZE;
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

        do_eraseindisplay(fh);

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
                CopyMem(fh->historybuffer[fh->historyviewpos], &fh->inputbuffer[fh->inputstart], len);

                fh->inputsize += len;
                fh->inputpos += len;

                do_write(fh, &fh->inputbuffer[fh->inputstart], len);
            }

#if !CYCLIC_HISTORY_WALK
        } /* if (!walk_to_empty_string) */
#endif

    } /* if (fh->historysize) */
}

/****************************************************************************************/

static const STRPTR CONCLIP_PORTNAME = "ConClip.rendezvous";

struct MyEditHookMsg
{
    struct Message  msg;
    struct SGWork   *sgw;
    WORD            code;
};

void do_paste(struct filehandle * fh)
{
    struct MsgPort replyport, *port;
    struct SGWork sgw;
    struct MyEditHookMsg msg;
    struct StringInfo sinfo;

    if (!(port = FindPort(CONCLIP_PORTNAME)))
    {
        D(bug("ConClip not running, but we got a ConClip paste request"));
        return;
    }

    D(bug("PASTE REQUEST!\n"));

    SetMem( &replyport, 0, sizeof( replyport ) );
    replyport.mp_Node.ln_Type = NT_MSGPORT;
    replyport.mp_Flags = PA_SIGNAL;
    replyport.mp_SigBit = SIGB_SINGLE;
    replyport.mp_SigTask = FindTask(NULL);
    NEWLIST(&replyport.mp_MsgList);

    /* ConClip only ever looks at MaxChars in StringInfo */
    sinfo.MaxChars = PASTEBUFSIZE;

    memset( &sgw, 0, sizeof( struct SGWork ) );
    sgw.WorkBuffer = AllocMem(PASTEBUFSIZE, MEMF_CLEAR | MEMF_ANY);
    sgw.Code = CODE_PASTE;
    sgw.EditOp = EO_BIGCHANGE;
    sgw.StringInfo = &sinfo;

    msg.msg.mn_Node.ln_Type = NT_MESSAGE;
    msg.msg.mn_ReplyPort = &replyport;
    msg.msg.mn_Length = sizeof(msg);

    msg.code = CODE_PASTE;
    msg.sgw = &sgw;

    SetSignal(0, SIGF_SINGLE);
    PutMsg(port, &msg.msg);
    WaitPort(&replyport);

    D(bug("Pasting %d bytes\n", sgw.BufferPos));

    if (fh->pastebuffer)
        FreeMem(fh->pastebuffer, PASTEBUFSIZE);
    fh->pastebuffer = sgw.WorkBuffer;
    fh->pastebuffersize = sgw.BufferPos;
    fh->pastebufferpos = 0;
}

/****************************************************************************************/

BOOL process_input(struct filehandle *fh)
{
    UBYTE c;
    WORD inp;
    while ((inp = scan_input(fh, &c)) != INP_DONE)
    {
        D(bug("Input Code: %d\n", inp));

        switch (inp)
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
                if (fh->inputpos == fh->inputsize)
                {
                    /* Match the classic CON: handler: submit the cursor
                     * movement and deletion as one console.device write so
                     * no intermediate cursor position becomes visible. */
                    UBYTE seq[] = { 8, 0x9B, 'P' };

                    do_write(fh, seq, sizeof(seq));

                    fh->inputsize--;
                    fh->inputpos--;
                }
                else
                {
                    WORD chars_right = fh->inputsize - fh->inputpos;

                    do_movecursor(fh, CUR_LEFT, 1);

                    fh->inputsize--;
                    fh->inputpos--;

                    redraw_tail(fh, &fh->inputbuffer[fh->inputpos + 1],
                        chars_right, 'P');

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
                    do_eraseindisplay(fh);

                    fh->inputpos = fh->inputsize = fh->inputstart;
                }
                else
                {
                    WORD chars_right = fh->inputsize - fh->inputpos;

                    redraw_tail(fh, &fh->inputbuffer[fh->inputpos],
                        chars_right, 'J');

                    memmove(&fh->inputbuffer[fh->inputstart], &fh->inputbuffer[fh->inputpos], chars_right);

                    fh->inputsize -= (fh->inputpos - fh->inputstart);
                    fh->inputpos = fh->inputstart;
                }
            }
            break;

        case INP_CONTROL_W:
            if (fh->inputpos > fh->inputstart)
            {
                WORD oldpos = fh->inputpos;
                WORD newpos = oldpos;

                while ((newpos > fh->inputstart) &&
                    isspace(fh->inputbuffer[newpos - 1]))
                {
                    newpos--;
                }

                while ((newpos > fh->inputstart) &&
                    !isspace(fh->inputbuffer[newpos - 1]))
                {
                    newpos--;
                }

                if (newpos != oldpos)
                {
                    WORD chars_deleted = oldpos - newpos;
                    WORD chars_right = fh->inputsize - oldpos;

                    do_movecursor(fh, CUR_LEFT, chars_deleted);

                    if (chars_right > 0)
                    {
                        redraw_tail(fh, &fh->inputbuffer[oldpos],
                            chars_right, 'K');

                        memmove(&fh->inputbuffer[newpos],
                            &fh->inputbuffer[oldpos], chars_right);
                    }
                    else
                    {
                        do_eraseinline(fh);
                    }

                    fh->inputsize -= chars_deleted;
                    fh->inputpos = newpos;
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
                }
                else
                {
                    WORD chars_right = fh->inputsize - fh->inputpos;

                    redraw_tail(fh, &fh->inputbuffer[fh->inputpos + 1],
                        chars_right, 'P');

                    memmove(&fh->inputbuffer[fh->inputpos], &fh->inputbuffer[fh->inputpos + 1], chars_right);
                }
            }
            break;

        case INP_SHIFT_DELETE:
            if (fh->inputpos < fh->inputsize)
            {
                fh->inputsize = fh->inputpos;
                do_eraseindisplay(fh);
            }
            break;

        case INP_CONTROL_K:
            fh->killsize = fh->inputsize - fh->inputpos;
            if (fh->killsize > 0)
            {
                CopyMem(&fh->inputbuffer[fh->inputpos],
                    fh->killbuffer, fh->killsize);
                fh->killbuffer[fh->killsize] = '\0';

                fh->inputsize = fh->inputpos;
                do_eraseindisplay(fh);
            }
            else
            {
                fh->killbuffer[0] = '\0';
            }
            break;

        case INP_CONTROL_Y:
            if ((fh->killsize > 0) &&
                (fh->inputsize + fh->killsize <= INPUTBUFFER_SIZE))
            {
                WORD chars_right = fh->inputsize - fh->inputpos;

                if (chars_right > 0)
                {
                    do_cursorvisible(fh, FALSE);
                    do_write(fh, fh->killbuffer, fh->killsize);
                    do_write(fh, &fh->inputbuffer[fh->inputpos], chars_right);
                    do_movecursor(fh, CUR_LEFT, chars_right);
                    do_cursorvisible(fh, TRUE);

                    memmove(&fh->inputbuffer[fh->inputpos + fh->killsize],
                        &fh->inputbuffer[fh->inputpos], chars_right);
                }
                else
                {
                    do_write(fh, fh->killbuffer, fh->killsize);
                }

                CopyMem(fh->killbuffer, &fh->inputbuffer[fh->inputpos],
                    fh->killsize);
                fh->inputpos += fh->killsize;
                fh->inputsize += fh->killsize;
            }
            break;

        case INP_CONTROL_X:
            if ((fh->inputsize - fh->inputstart) > 0)
            {
                if (fh->inputpos > fh->inputstart)
                {
                    do_movecursor(fh, CUR_LEFT, fh->inputpos - fh->inputstart);
                }
                do_eraseindisplay(fh);

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
                }
                else
                {
                    WORD chars_right = fh->inputsize - fh->inputpos;

                    redraw_tail(fh, &fh->inputbuffer[fh->inputpos],
                        chars_right, 0);

                    memmove(&fh->inputbuffer[fh->inputpos + 1], &fh->inputbuffer[fh->inputpos], chars_right);
                    fh->inputbuffer[fh->inputpos++] = c;
                    fh->inputsize++;
                }
            }
            break;

        case INP_EOF:
            D(bug("[con:handler] %s: Read EOF (window closing)\n", __func__));

            if (fh->flags & FHFLG_WAITFORCLOSE)
                return TRUE;

            fh->flags |= FHFLG_EOF;
            if (fh->flags & FHFLG_AUTO && fh->window)
            {
                if (fh->flags & FHFLG_CONSOLEDEVICEOPEN)
                {
                    /* Close the device, it will be re-opened if needed */
                    CloseDevice((struct IORequest *) fh->conreadio);
                    fh->flags &= ~FHFLG_CONSOLEDEVICEOPEN;
                }
                /* Only now drop resources tied to this window */
                close_con_window(fh);
            }

            /* fall through */

        case INP_RETURN:
            if (fh->inputsize <= INPUTBUFFER_SIZE)
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
                    struct Message *msg = (struct Message*) RemHead((struct List *) &fh->pendingReads);
                    struct DosPacket *dp = (struct DosPacket*) msg->mn_Node.ln_Name;

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
            Completion(fh, FALSE);
            break;

        case INP_SHIFT_TAB:
            Completion(fh, TRUE);
            break;

        case INP_PASTE:
            D(bug("[con:handler] %s: INP_PASTE\n", __func__));
            do_paste(fh);
            break;

        } /* switch(inp) */

    } /* while((inp = scan_input(fh, &c)) != INP_DONE) */

    return FALSE;
}

BOOL answer_write_request(struct filehandle *fh, struct DosPacket *dp)
{
    UBYTE *buffer = (UBYTE*) dp->dp_Arg2;
    LONG length = dp->dp_Arg3;

#if RMB_FREEZES_OUTPUT
    if (!(fh->flags & FHFLG_DEVICEMODE))
    {
        struct Window *conwindow =
            ((struct ConUnit *)fh->conwriteio.io_Unit)->cu_Window;

        while ((PeekQualifier() & IEQUALIFIER_RBUTTON) && conwindow &&
               (conwindow == IntuitionBase->ActiveWindow))
        {
            Delay(2);
        }
    }
#endif

    if ((dp->dp_Port->mp_Flags & PF_ACTION) == PA_SIGNAL && dp->dp_Port->mp_SigTask)
    {
        fh->lastwritetask = dp->dp_Port->mp_SigTask;
    }

    do_write(fh, buffer, length);
    if (fh->conwriteio.io_Error)
        /* DOS has no general device-write error code. */
        replypkt2(dp, -1, ERROR_WRITE_PROTECTED);
    else if (fh->conwriteio.io_Actual)
        replypkt2(dp, fh->conwriteio.io_Actual, 0);
    else if (fh->flags & FHFLG_DEVICEMODE)
        /* Some plain devices, including parallel.device, leave io_Actual
         * zero on success. Preserve their existing all-or-error behavior. */
        replypkt2(dp, length, 0);
    else
        /* A nonempty console write must make progress. Returning zero can
         * leave DOS buffered output retrying the same write forever. */
        replypkt2(dp, -1, ERROR_WRITE_PROTECTED);

    return TRUE;
}

void answer_read_request(struct filehandle *fh, struct DosPacket *dp, ULONG dp_Arg3)
{
    ULONG readlen;

    readlen = (fh->inputsize < dp_Arg3) ? fh->inputsize : dp_Arg3;

    CopyMem(fh->inputbuffer, (UBYTE*) dp->dp_Arg2, readlen);
    CopyMem(fh->inputbuffer + readlen, fh->inputbuffer, fh->inputsize - readlen);

    fh->inputsize -= readlen;
    fh->inputpos -= readlen;
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
            Remove((struct Node *) msg);
            dp = (struct DosPacket*) msg->mn_Node.ln_Name;
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
        else
        {
            AddTail((struct List *) &fh->pendingReads, (struct Node *) dp->dp_Link);
            fh->flags |= FHFLG_READPENDING;
        }
    }
}
