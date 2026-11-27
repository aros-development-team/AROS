/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Conclip CLI command
*/
/******************************************************************************

    NAME

        Conclip

    FORMAT

        CONCLIP [ [UNIT | CLIPUNIT] <unitnumber>] [OFF]
  
    SYNOPSIS

        CLIPUNIT=UNIT/N, ON/S, OFF/S

    LOCATION

        C:

    FUNCTION

          Enable clipboard cut/copy/paste functionality in console windows
      and string gadgets. This enables the use of a system global clipboard.

    INPUTS

        CLIPUNIT=UNIT -- the number associated to the clipboard exchange unit
        ON/S    --  Activates conclip (default unit will be set to 1)
        OFF/S   --  Deactivates Conclip

    RESULT

    NOTES

    EXAMPLE

        Conclip CLIPUNIT=1

        This will set the global clipboard unit to 1. The available clipboards
        can be checked in clips: dirDir Clips:

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************//*****************************************************************************************/

#include <aros/asmcall.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <intuition/intuition.h>
#include <intuition/sghooks.h>
#include <intuition/classusr.h>
#include <libraries/iffparse.h>
#include <devices/inputevent.h>
#include <datatypes/textclass.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/iffparse.h>
#include <proto/alib.h>
#include <proto/utility.h>

#include <string.h>

#undef DEBUG
#define DEBUG 0
#include <aros/debug.h>

/*****************************************************************************************/

#define ARG_TEMPLATE    "CLIPUNIT=UNIT/N,OFF/S"
#define ARG_CLIPUNIT    0
#define ARG_OFF         1
#define NUM_ARGS        2

/*****************************************************************************************/

#define CODE_COPY       'C'
#define CODE_PASTE      'V'
#define CODE_READ_TEXT  0x100
#define CODE_WRITE_TEXT 0x101

#define TEXT_VERSION     1
#define TEXT_UNSUPPORTED 0
#define TEXT_OK          1
#define TEXT_ERROR       2
#define TEXT_WRITE_MAX   (1024UL * 1024UL)

/* The encoding of the text in the message. Reading, it is what the caller
   wants and, in the reply, what it got; writing, what the caller's bytes
   are. Legacy text is the CHRS bytes as they are on the clipboard. */
#define TEXT_ENC_LEGACY  0
#define TEXT_ENC_UTF8    1

/* How text in UTF-8 sits on the clipboard: a CHRS chunk with the Latin-1
   rendering for readers that know nothing else, and the full text in a UTF8
   chunk; a CSET chunk whose first long is 106 instead marks the CHRS bytes
   themselves as UTF-8. Both are read. */
#define ID_UTF8          MAKE_ID('U','T','F','8')
#define ID_CSET          MAKE_ID('C','S','E','T')
#define CSET_UTF8        106

const TEXT version[] = "$VER: Conclip 42.4 (29.10.2026)\n";

struct MyEditHookMsg
{
    struct Message      msg;
    struct SGWork       *sgw;
    WORD                code;
};

struct ConClipTextMsg
{
    struct Message      msg;
    struct SGWork       *sgw;
    WORD                code;
    ULONG               version;
    ULONG               status;
    UBYTE               *data;
    ULONG               length;
    ULONG               capacity;
    ULONG               encoding;
};

/*****************************************************************************************/

const STRPTR CONCLIP_TASKNAME = "\xAB ConClip \xBB";
const STRPTR CONCLIP_PORTNAME = "ConClip.rendezvous";

/*****************************************************************************************/

static struct MsgPort   *progport;
static struct Hook      edithook, *oldedithook;
static struct Task      *progtask;
static ULONG            clipunit, portmask;
static BOOL             off;
static UBYTE            s[256];

/*****************************************************************************************/

static void cleanup(STRPTR msg)
{
    if (msg) Printf("ConClip: %s\n", msg);

    if (oldedithook) SetEditHook(oldedithook);
    if (progport) DeletePort(progport);
}

/*****************************************************************************************/

static void init(void)
{
    progtask = FindTask(NULL);
    ((struct Process *)progtask)->pr_WindowPtr = (APTR)-1;
}

/*****************************************************************************************/

static BOOL getarguments(void)
{
    struct RDArgs       *myargs;
    IPTR                args[NUM_ARGS] = {0, 0};
    
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, NULL)))
    {
        Fault(IoErr(), 0, s, 255);
        cleanup(s);
        return FALSE;
    }
    
    if (args[ARG_CLIPUNIT]) clipunit = (ULONG)(*(IPTR *)args[ARG_CLIPUNIT]);
    if (args[ARG_OFF]) off = args[ARG_OFF] ? TRUE : FALSE;
    
    FreeArgs(myargs);
    return TRUE;
}

/*****************************************************************************************/

enum CheckPortResult
{
    CHECKPORT_FAIL = -1,
    CHECKPORT_DONE = 0,
    CHECKPORT_RUN  = 1
};

static int checkport(void)
{
    Forbid();
    progport = FindPort(CONCLIP_PORTNAME);
    if (progport)
    {
        if (off) Signal(progport->mp_SigTask, SIGBREAKF_CTRL_C);
        Permit();
        progport = NULL;
        cleanup(NULL);
        return CHECKPORT_DONE;
    }
    progport = CreatePort(CONCLIP_PORTNAME, 1);
    Permit();

    if (!progport)
    {
        cleanup("Could not create MsgPort!");
        return CHECKPORT_FAIL;
    }
    
    portmask = 1L << progport->mp_SigBit;
    return CHECKPORT_RUN;
}

/*****************************************************************************************/

AROS_UFH3(ULONG, conclipeditfunc,
    AROS_UFHA(struct Hook *,            hook,           A0),
    AROS_UFHA(struct SGWork *,          sgw,            A2),
    AROS_UFHA(ULONG *,                  command,        A1)
)
{
    AROS_USERFUNC_INIT

    struct MsgPort       *port, replyport;
    struct MyEditHookMsg msg;
    BOOL                 calloldhook = TRUE;
    ULONG                retcode = 0;

    switch (*command)
    {
        case SGH_KEY:
            D(bug("ConClip/conclipeditfunc: is SGH_KEY\n"));

            if (sgw->IEvent->ie_Qualifier & IEQUALIFIER_RCOMMAND)
            {
                D(bug("ConClip/conclipeditfunc: qualifier RCOMMAND okay\n"));

                switch(ToUpper(sgw->Code))
                {
                    case 'C':
                        if (!sgw->NumChars) break;
                        /* fall through */

                    case 'V':
                        D(bug("ConClip/conclipeditfunc: key = %c\n", toupper(sgw->Code)));

                        if ((port = FindPort(CONCLIP_PORTNAME)))
                        {
                            calloldhook = FALSE;

                            memset( &replyport, 0, sizeof( replyport ) );
                            replyport.mp_Node.ln_Type   = NT_MSGPORT;
                            replyport.mp_Flags          = PA_SIGNAL;
                            replyport.mp_SigBit         = SIGB_SINGLE;
                            replyport.mp_SigTask        = FindTask(NULL);
                            NewList(&replyport.mp_MsgList);

                            msg.msg.mn_Node.ln_Type     = NT_MESSAGE;
                            msg.msg.mn_ReplyPort        = &replyport;
                            msg.msg.mn_Length           = sizeof(msg);

                            msg.code = ToUpper(sgw->Code);
                            msg.sgw  = sgw;

                            if ((msg.code == CODE_COPY) || (sgw->NumChars < sgw->StringInfo->MaxChars - 1))
                            {
                                SetSignal(0, SIGF_SINGLE);
                                PutMsg(port, &msg.msg);
                                WaitPort(&replyport);
                            }

                            if (msg.code == CODE_PASTE)
                            {
                                WORD len = strlen(sgw->WorkBuffer);
                                
                                if (len != sgw->NumChars)
                                {
                                    sgw->NumChars = len;
                                    sgw->EditOp   = EO_BIGCHANGE;
                                    sgw->Actions  = SGA_USE | SGA_REDISPLAY;
                                    
                                    retcode = 1;
                                }

                            } /* if (msg.code == CODE_COPY) */

                        } /* if ((port = FindPort(CONCLIP_PORTNAME))) */

                        break;

                } /* switch(ToUpper(sgw->Code)) */

            } /* if (sgw->IEvent->ie_Qualifier & IEQUALIFIER_RCOMMAND) */
            break;

    } /* switch (*command) */

    if (calloldhook) retcode = CallHookPkt(oldedithook, sgw, command);

    return retcode;

    AROS_USERFUNC_EXIT
}

/*****************************************************************************************/

static void installedithook(void)
{
    edithook.h_Entry = (HOOKFUNC)AROS_ASMSYMNAME(conclipeditfunc);
    edithook.h_SubEntry = NULL;
    edithook.h_Data = NULL;
    
    oldedithook = SetEditHook(&edithook);
}

/*****************************************************************************************/
/*****************************************************************************************/
/*****************************************************************************************/

static void savetoclipboard(struct SGWork *sgw)
{
    struct IFFHandle *iff;
    
    if((iff = AllocIFF()))
    {
        if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit)))
        {
            InitIFFasClip(iff);
            if(!OpenIFF(iff,IFFF_WRITE))
            {
                if(!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))
                {
                    if(!PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN))
                    {
                        WriteChunkBytes(iff, sgw->WorkBuffer, sgw->NumChars);
                        
                        PopChunk(iff);
                        
                    } /* if(!PushChunk(iff, ID_FTXT, ID_CHRS, IFFSIZE_UNKNOWN)) */
                    PopChunk(iff);
                    
                } /* if(!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN)) */
                CloseIFF(iff);
                
            } /* if(!OpenIFF(iff,IFFF_WRITE)) */
            CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
            
        } /* if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit))) */
        FreeIFF(iff);
        
    } /* if((iff = AllocIFF()))) */
}

/*****************************************************************************************/

static void readfromclipboard(struct SGWork *sgw)
{
    struct IFFHandle    *iff;
    struct ContextNode  *cn;
    
    if((iff = AllocIFF()))
    {
        D(bug("ConClip/conclipeditfunc: AllocIFF okay\n"));
        
        if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit)))
        {
            D(bug("ConClip/conclipeditfunc: OpenClipboard okay\n"));
            
            InitIFFasClip(iff);
            if(!OpenIFF(iff, IFFF_READ))
            {
                D(bug("ConClip/conclipeditfunc: OpenIff okay\n"));
                
                if (!(StopChunk(iff, ID_FTXT, ID_CHRS)))
                {
                    D(bug("ConClip/conclipeditfunc: StopChunk okay\n"));
                    
                    if (!ParseIFF(iff, IFFPARSE_SCAN))
                    {
                        D(bug("ConClip/conclipeditfunc: ParseIFF okay\n"));
                        
                        cn = CurrentChunk(iff);
                        if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS) && (cn->cn_Size > 0))
                        {
                            WORD readsize;
                            
                            D(bug("ConClip: readfromclipboard: Found FTXT CHRS Chunk\n"));
                            D(bug("ConClip: readfromclipboard: Old text = \"%s\"\n", sgw->WorkBuffer));

                            readsize = sgw->StringInfo->MaxChars - 1 - sgw->BufferPos;
                            if (cn->cn_Size < readsize) readsize = cn->cn_Size;
                            if (readsize > 0)
                            {
                                memmove(sgw->WorkBuffer + sgw->BufferPos + readsize,
                                        sgw->WorkBuffer + sgw->BufferPos,
                                        sgw->StringInfo->MaxChars - sgw->BufferPos - readsize);
                                ReadChunkBytes(iff, sgw->WorkBuffer + sgw->BufferPos, readsize);
                                
                                D(bug("ConClip: readfromclipboard: New text = \"%s\"\n", sgw->WorkBuffer));
                                
                                sgw->BufferPos += readsize;
                            }
                            
                        } /* if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS) && (cn->cn_Size > 0)) */
                        
                    } /* if (!ParseIFF(iff, IFFPARSE_SCAN)) */
                    
                } /* if (!(StopChunk(iff, ID_FTXT, ID_CHRS))) */
                
                CloseIFF(iff);
                
            } /* if(!OpenIFF(iff, IFFF_READ)) */
            CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
            
        } /* if((iff->iff_Stream = (IPTR)OpenClipboard(clipunit))) */
        FreeIFF(iff);
        
    } /* if((iff = AllocIFF()))) */
}

/*****************************************************************************************/

struct textbuf
{
    UBYTE *data;
    ULONG  used, capacity;
};

static BOOL textbuf_grow(struct textbuf *b, ULONG more)
{
    ULONG grown = b->capacity ? b->capacity : 256;
    UBYTE *newdata;

    if (more > 0x7fffffffUL - b->used)
        return FALSE;
    if (b->used + more <= b->capacity)
        return TRUE;
    while (grown < b->used + more)
        grown = (grown > 0x3fffffffUL) ? b->used + more : grown * 2;
    if (!(newdata = AllocMem(grown, MEMF_PUBLIC)))
        return FALSE;
    if (b->used)
        CopyMem(b->data, newdata, b->used);
    if (b->data)
        FreeMem(b->data, b->capacity);
    b->data = newdata;
    b->capacity = grown;
    return TRUE;
}

static void textbuf_free(struct textbuf *b)
{
    if (b->data)
        FreeMem(b->data, b->capacity);
    b->data = NULL;
    b->used = b->capacity = 0;
}

/* the length of a well-formed UTF-8 sequence at p, 0 for a bad one */
static ULONG utf8_sequence(const UBYTE *p, ULONG left, ULONG *cp)
{
    ULONG n, c = p[0], i;

    if (c < 0x80) { *cp = c; return 1; }
    if (c >= 0xC2 && c <= 0xDF) { n = 2; c &= 0x1F; }
    else if (c >= 0xE0 && c <= 0xEF) { n = 3; c &= 0x0F; }
    else if (c >= 0xF0 && c <= 0xF4) { n = 4; c &= 0x07; }
    else return 0;
    if (left < n)
        return 0;
    for (i = 1; i < n; i++)
    {
        if ((p[i] & 0xC0) != 0x80)
            return 0;
        c = (c << 6) | (p[i] & 0x3F);
    }
    if ((n == 3 && (c < 0x800 || (c >= 0xD800 && c <= 0xDFFF))) || (n == 4 && (c < 0x10000 || c > 0x10FFFF)))
        return 0;
    *cp = c;
    return n;
}

static BOOL utf8_valid(const UBYTE *p, ULONG length)
{
    ULONG cp, n;

    while (length)
    {
        if (!(n = utf8_sequence(p, length, &cp)))
            return FALSE;
        p += n;
        length -= n;
    }
    return TRUE;
}

/* Latin-1 bytes as UTF-8 */
static BOOL latin1_to_utf8(const UBYTE *p, ULONG length, struct textbuf *out)
{
    ULONG i, more = length;

    for (i = 0; i < length; i++)
        if (p[i] >= 0x80)
            more++;
    if (!textbuf_grow(out, more))
        return FALSE;
    for (i = 0; i < length; i++)
    {
        if (p[i] < 0x80)
            out->data[out->used++] = p[i];
        else
        {
            out->data[out->used++] = 0xC0 | (p[i] >> 6);
            out->data[out->used++] = 0x80 | (p[i] & 0x3F);
        }
    }
    return TRUE;
}

/* UTF-8 as Latin-1, a '?' for what Latin-1 has not got */
static BOOL utf8_to_latin1(const UBYTE *p, ULONG length, struct textbuf *out)
{
    ULONG cp, n;

    if (!textbuf_grow(out, length))
        return FALSE;
    while (length)
    {
        n = utf8_sequence(p, length, &cp);
        if (!n)
            return FALSE;
        out->data[out->used++] = cp < 0x100 ? (UBYTE)cp : '?';
        p += n;
        length -= n;
    }
    return TRUE;
}

static void readtext(struct ConClipTextMsg *msg)
{
    struct IFFHandle *iff;
    struct textbuf chrs = { NULL, 0, 0 }, utf8 = { NULL, 0, 0 }, *result = NULL, conv = { NULL, 0, 0 };
    LONG error = 0, cset = 0;
    BOOL found = FALSE;
    ULONG want = msg->encoding;

    msg->status = TEXT_ERROR;
    msg->data = NULL;
    msg->length = msg->capacity = 0;

    if (want != TEXT_ENC_LEGACY && want != TEXT_ENC_UTF8)
        return;
    if (!(iff = AllocIFF()))
        return;

    if ((iff->iff_Stream = (IPTR)OpenClipboard(clipunit)))
    {
        InitIFFasClip(iff);
        if (!OpenIFF(iff, IFFF_READ))
        {
            error = StopChunk(iff, ID_FTXT, ID_CHRS);
            if (!error) error = StopChunk(iff, ID_FTXT, ID_UTF8);
            if (!error) error = StopChunk(iff, ID_FTXT, ID_CSET);
            while (!error && !(error = ParseIFF(iff, IFFPARSE_SCAN)))
            {
                struct ContextNode *cn = CurrentChunk(iff);
                struct textbuf *b;
                ULONG length;

                if (!cn || cn->cn_Size < 0)
                {
                    error = IFFERR_MANGLED;
                    break;
                }
                length = cn->cn_Size;
                if (cn->cn_ID == ID_CSET)
                {
                    if (length >= 4 && ReadChunkBytes(iff, &cset, 4) != 4)
                        error = IFFERR_MANGLED;
                    continue;
                }
                b = cn->cn_ID == ID_UTF8 ? &utf8 : &chrs;
                if (!textbuf_grow(b, length))
                {
                    error = IFFERR_NOMEM;
                    break;
                }
                if (ReadChunkBytes(iff, b->data + b->used, length) != (LONG)length)
                {
                    error = IFFERR_MANGLED;
                    break;
                }
                b->used += length;
                found = TRUE;
            }
            CloseIFF(iff);
        }
        CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);

    if (error == IFFERR_EOF && found)
    {
        if (want == TEXT_ENC_LEGACY)
        {
            result = &chrs;
            msg->encoding = cset == CSET_UTF8 ? TEXT_ENC_UTF8 : TEXT_ENC_LEGACY;
        }
        else if (utf8.used || cset == CSET_UTF8)
        {
            result = utf8.used ? &utf8 : &chrs;
            msg->encoding = TEXT_ENC_UTF8;
            if (!utf8_valid(result->data, result->used))
                result = NULL;
        }
        else if (latin1_to_utf8(chrs.data, chrs.used, &conv))
        {
            result = &conv;
            msg->encoding = TEXT_ENC_UTF8;
        }
    }
    if (result)
    {
        if (!result->data && !textbuf_grow(result, 1))
            result = NULL;
    }
    if (result)
    {
        msg->data = result->data;
        msg->length = result->used;
        msg->capacity = result->capacity;
        msg->status = TEXT_OK;
        result->data = NULL;
    }
    textbuf_free(&chrs);
    textbuf_free(&utf8);
    textbuf_free(&conv);
}

/*****************************************************************************************/

static BOOL writechunk(struct IFFHandle *iff, ULONG id, const UBYTE *data, ULONG length)
{
    BOOL ok;

    if (PushChunk(iff, ID_FTXT, id, IFFSIZE_UNKNOWN))
        return FALSE;
    ok = !length || WriteChunkBytes(iff, (APTR)data, length) == (LONG)length;
    return !PopChunk(iff) && ok;
}

static void writetext(struct ConClipTextMsg *msg)
{
    struct IFFHandle *iff;
    struct textbuf latin1 = { NULL, 0, 0 };
    BOOL written = FALSE, utf8 = msg->encoding == TEXT_ENC_UTF8;

    msg->status = TEXT_ERROR;

    if (msg->length > TEXT_WRITE_MAX || msg->length > msg->capacity || (msg->length && !msg->data))
        return;
    if (msg->encoding != TEXT_ENC_LEGACY && !utf8)
        return;
    if (utf8 && (!utf8_valid(msg->data, msg->length) || !utf8_to_latin1(msg->data, msg->length, &latin1)))
        return;

    if (!(iff = AllocIFF()))
    {
        textbuf_free(&latin1);
        return;
    }

    if ((iff->iff_Stream = (IPTR)OpenClipboard(clipunit)))
    {
        InitIFFasClip(iff);
        if (!OpenIFF(iff, IFFF_WRITE))
        {
            if (!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))
            {
                if (utf8)
                    written = writechunk(iff, ID_CHRS, latin1.data, latin1.used) &&
                              writechunk(iff, ID_UTF8, msg->data, msg->length);
                else
                    written = writechunk(iff, ID_CHRS, msg->data, msg->length);
                if (PopChunk(iff))
                    written = FALSE;
            }
            CloseIFF(iff);
        }
        CloseClipboard((struct ClipboardHandle *)iff->iff_Stream);
    }
    FreeIFF(iff);
    textbuf_free(&latin1);

    if (written)
        msg->status = TEXT_OK;
}

/*****************************************************************************************/

static BOOL istextmsg(struct MyEditHookMsg *msg)
{
    return msg->msg.mn_Length >= sizeof(struct ConClipTextMsg) &&
        ((struct ConClipTextMsg *)msg)->version == TEXT_VERSION;
}

/*****************************************************************************************/

static void handleall(void)
{
    BOOL quitme = FALSE;
    ULONG sigs;
    
    while(!quitme)
    {
        sigs = Wait(SIGBREAKF_CTRL_C | portmask);
        
        if (sigs & portmask)
        {
            struct MyEditHookMsg *msg;
            
            while((msg = (struct MyEditHookMsg *)GetMsg(progport)))
            {
                switch(msg->code)
                {
                    case CODE_READ_TEXT:
                        if (istextmsg(msg))
                            readtext((struct ConClipTextMsg *)msg);
                        break;

                    case CODE_WRITE_TEXT:
                        if (istextmsg(msg))
                            writetext((struct ConClipTextMsg *)msg);
                        break;

                    case CODE_COPY:
                        D(bug("ConClip: Received CODE_COPY message\n"));
                        savetoclipboard(msg->sgw);
                        break;
                        
                    case CODE_PASTE:
                        D(bug("ConClip: Received CODE_PASTE message\n"));
                        readfromclipboard(msg->sgw);
                        break;
                        
                } /* switch(msg->code) */
                
                ReplyMsg(&msg->msg);
                
            } /* while((msg = (struct MyEditHookMsg *)GetMsg(progport))) */
            
        } /* if (sigs & portmask) */

        if (sigs & SIGBREAKF_CTRL_C) quitme = TRUE;
        
    } /* while(!quitme) */
}

/*****************************************************************************************/

int main(void)
{
    int portResult;

    init();

    if (!getarguments())
        return RETURN_FAIL;

    portResult = checkport();
    if (portResult == CHECKPORT_FAIL)
        return RETURN_FAIL;
    if (portResult == CHECKPORT_DONE)
        return RETURN_OK;

    installedithook();
    handleall();

    cleanup(NULL);
    return RETURN_OK;
}

/*****************************************************************************************/
