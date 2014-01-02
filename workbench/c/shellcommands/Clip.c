/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Clip

    TEMPLATE

        U=UNIT/N/K,W=WAIT/S,G=GET/S,P=PUT=S=SET/S,C=COUNT/S,TEXT

    LOCATION

        C:

    FUNCTION

        Handle the clipboard's units (read or write text) from the Shell.

    FORMAT

        CLIP [COUNT] [UNIT <unit>] [ GET [WAIT] ] [ SET [TEXT] ]

    RESULT

        Standard DOS return codes.


******************************************************************************/

#include <datatypes/textclass.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <proto/alib.h> // __sprintf()

//#define DEBUG 1
#include <aros/debug.h>

#define SH_GLOBAL_SYSBASE       1
#include <aros/shcommands.h>

/*
 * otigreat: Without having used the original Clip, it's not clear if
 * > C:Clip SET
 * has to set an empty clip unit, or to delete the said clip unit. Though the
 * latter seems more elegant. Without CLIP_SET_NO_TEXT_MEANS_DELETE_UNIT you
 * get the other behaviour. The current one, however, also allows to obtain an
 * empty clip unit by doing:
 * > C:Clip SET ""
 */
#define CLIP_SET_NO_TEXT_MEANS_DELETE_UNIT 1

BOOL   toClip(STRPTR text, UBYTE clipUnit, struct Library *IFFParseBase);
STRPTR fromClip(UBYTE clipUnit, struct Library *IFFParseBase);

AROS_SH6H(Clip,50.1, "read from or write to clipboard\n",
AROS_SHAH(LONG *,U=   ,UNIT,/N/K,NULL ,  "Clipboard unit to be used by GET or SET (default = 0)\n"
                                     "\t\tUNIT must be between 0 and 255"),
AROS_SHAH(BOOL  ,W=   ,WAIT  ,/S,FALSE,  "Make GET wait for the UNIT to be filled with text data"),
AROS_SHAH(BOOL  ,G=   ,GET   ,/S,FALSE,"\tRetrieve text data from the UNIT"),
AROS_SHAH(BOOL  ,P=PUT=S=,SET,/S,FALSE,  "Store supplied TEXT data in the UNIT"),
AROS_SHAH(BOOL  ,C=   ,COUNT ,/S,FALSE,  "Output the number of filled units"),
AROS_SHAH(STRPTR,     ,TEXT  ,  ,NULL ,"\tText data to be SET in the UNIT, requires to be quoted\n"
                                     "\t\tif there is more than a single word\n") )
{
    AROS_SHCOMMAND_INIT

    int                   rc     = RETURN_FAIL;
    BOOL                  get    = ((SHArg(GET)) || (!SHArg(SET) && !SHArg(COUNT)));
    BYTE                  waitSig;
    UBYTE                 unit, clipUnitPath[10]; /* "CLIPS:255\0" */
    ULONG                 waitMask, sigs;
    STRPTR                outstr = NULL;
    struct Library       *IFFParseBase;
    struct NotifyRequest *clipUnitNR;

    if (!SHArg(UNIT))
        unit = PRIMARY_CLIP;
    else if ((*SHArg(UNIT) < 0L) || (*SHArg(UNIT) > 255L))
    {
        PrintFault(ERROR_BAD_NUMBER, (CONST_STRPTR)"Clip");
        return (RETURN_ERROR);
    }
    else
        unit = *SHArg(UNIT);

    if ((IFFParseBase = OpenLibrary((STRPTR)"iffparse.library", 36)))
    {
        __sprintf(clipUnitPath, (const UBYTE *)"CLIPS:%d", unit);
        D(bug("[Clip] clipUnitPath == '%s'\n", clipUnitPath));

        if(get && SHArg(WAIT))
        {
            if((waitSig = AllocSignal(-1L)) != -1)
            {
                if ((clipUnitNR = AllocMem(sizeof(struct NotifyRequest), MEMF_CLEAR)))
                {
                    waitMask = 1L << waitSig;

                    clipUnitNR->nr_Name = clipUnitPath;
                    clipUnitNR->nr_Flags = NRF_SEND_SIGNAL;
                    clipUnitNR->nr_stuff.nr_Signal.nr_Task = FindTask(NULL);
                    clipUnitNR->nr_stuff.nr_Signal.nr_SignalNum = waitSig;

                    StartNotify(clipUnitNR);
                    sigs = Wait(waitMask | SIGBREAKF_CTRL_C);
                    EndNotify(clipUnitNR);
                    FreeMem(clipUnitNR, sizeof(struct NotifyRequest));

                    if (sigs & SIGBREAKF_CTRL_C)
                    {
                        SetIoErr(ERROR_BREAK);
                        rc = RETURN_ERROR;
                    }
                    else
                        rc = RETURN_OK;
                } /* if ((clipUnitNR = AllocMem(sizeof(struct NotifyRequest), MEMF_CLEAR))) */
                FreeSignal(waitSig);
            } /* if((waitSig = AllocSignal(-1L)) != -1) */
            else
            {
                /* :-/ AllocSignal() doesn't SetIoErr() on error... but what is the right error to set? */
                SetIoErr(ERROR_NO_FREE_STORE);
            }
        } /* if(SHArg(GET) && SHArg(WAIT)) */

        /* We don't want to proceed if we had to WAIT and it failed... */
        if (!(SHArg(WAIT) && (rc != RETURN_OK)))
        {
            if (get)
            {
                if ((outstr = fromClip(unit, IFFParseBase)))
                {
                    PutStr(outstr);
                    FreeVec(outstr);
                    FPutC(Output(), '\n');
                    rc = RETURN_OK;
                }
                else
                    rc = RETURN_ERROR;
            }

            if (SHArg(SET))
            {
#if CLIP_SET_NO_TEXT_MEANS_DELETE_UNIT
                if (!SHArg(TEXT))
                {
                    if (DeleteFile(clipUnitPath) || (IoErr() == ERROR_OBJECT_NOT_FOUND))
                        rc = RETURN_OK;
                }
                else
#endif
                {
                    if (toClip(SHArg(TEXT), unit, IFFParseBase))
                        rc = RETURN_OK;
                    else
                        rc = RETURN_ERROR;
                }
            }

            /* There must be a better way??? */
            if (SHArg(COUNT))
            {
                UBYTE count = 0;
                BPTR  handle;
                unit = PRIMARY_CLIP;
                do
                {
                    __sprintf(clipUnitPath, (const UBYTE *)"CLIPS:%d", unit);
                    if ((handle = Open(clipUnitPath, MODE_OLDFILE)) || (IoErr() != ERROR_OBJECT_NOT_FOUND))
                    {
                        count++;
                        if (handle)
                            Close(handle);
                    }
                } while (unit++ < 255);
                Printf((STRPTR)"%d\n", count);
                rc = RETURN_OK;
            }
        } /* if (!(SHArg(WAIT) && (rc != RETURN_OK))) */
        CloseLibrary(IFFParseBase);
    } /* if ((IFFParseBase = OpenLibrary((STRPTR)"iffparse.library", 36))) */

    if (rc != RETURN_OK)
        PrintFault(IoErr(), (CONST_STRPTR)"Clip");

    return (rc);

    AROS_SHCOMMAND_EXIT
}


BOOL toClip(STRPTR text, UBYTE clipUnit, struct Library *IFFParseBase)
{
    struct IFFHandle *iff;
    ULONG             len;
    BOOL              ok  = FALSE;

    if ((iff = AllocIFF()))
    {
        if ((iff->iff_Stream = (IPTR)OpenClipboard(clipUnit)))
        {
            InitIFFasClip(iff);

            if (!OpenIFF(iff, IFFF_WRITE))
            {
                if (!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN))
                {
                    if (!PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN))
                    {
                        len = text ? strlen((char *)text) : 0; /* strlen() crashes if text == NULL */

                        if (WriteChunkBytes(iff, text, len) == len)
                        {
                            ok = TRUE;
                        }
                        PopChunk(iff);
                    } /* if (!PushChunk(iff, 0, ID_CHRS, IFFSIZE_UNKNOWN)) */
                    PopChunk(iff);
                } /* if (!PushChunk(iff, ID_FTXT, ID_FORM, IFFSIZE_UNKNOWN)) */
                CloseIFF(iff);
            } /* if (!OpenIFF(iff, IFFF_WRITE)) */
            CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
        } /* if ((iff->iff_Stream = (IPTR)OpenClipboard(clipUnit))) */
        FreeIFF(iff);
    } /* if (iff) */

    return (ok);
}


STRPTR fromClip(UBYTE clipUnit, struct Library *IFFParseBase)
{
    struct IFFHandle   *iff;
    struct ContextNode *cn;
    STRPTR              filebuffer      = NULL,
                        new_filebuffer  = NULL;
    ULONG               filebuffer_size = 0;
    LONG                error;
    BOOL                ok              = FALSE;
    
    iff = AllocIFF();
    if (iff)
    {
        if ((iff->iff_Stream = (IPTR)OpenClipboard(clipUnit)))
        {
            InitIFFasClip(iff);

            if (!OpenIFF(iff, IFFF_READ))
            {
                if (!StopChunk(iff, ID_FTXT, ID_CHRS))
                {
                    for(;;)
                    {
                        error = ParseIFF(iff, IFFPARSE_SCAN);

                        if ((error != 0) && (error != IFFERR_EOC))
                            break;

                        if (NULL == (cn = CurrentChunk(iff)))
                        {
                            kprintf("[Clip] ZERO CONTEXTNODE!!!\n\n");
                            continue;
                        }
                        
                        if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS))
                        {
                            if (!filebuffer)
                            {
                                if (NULL == (filebuffer = AllocVec(cn->cn_Size + 1, MEMF_ANY)))
                                    break;

                                ok = TRUE;
                            }
                            else
                            {
                                if (NULL == (new_filebuffer = AllocVec(filebuffer_size + cn->cn_Size + 1, MEMF_ANY)))
                                {
                                    ok = FALSE;
                                    break;
                                }
                                
                                CopyMem(filebuffer, new_filebuffer, filebuffer_size);
                                FreeVec(filebuffer);
                                filebuffer = new_filebuffer;
                            }
                            
                            if (ReadChunkBytes(iff, filebuffer + filebuffer_size, cn->cn_Size) != cn->cn_Size)
                            {
                                ok = FALSE;
                                break;
                            }

                            filebuffer_size += cn->cn_Size;
                            filebuffer[filebuffer_size] = '\0';
                        } /* if ((cn->cn_Type == ID_FTXT) && (cn->cn_ID == ID_CHRS)) */
                    } /* for(;;) */

                    if (filebuffer && !ok) {
                        FreeVec(filebuffer);
                        filebuffer = NULL;
                    }
                } /* if (!StopChunk(iff, ID_FTXT, ID_CHRS)) */
                CloseIFF(iff);
            } /* if (!OpenIFF(iff, IFFF_READ)) */
            CloseClipboard((struct ClipboardHandle*)iff->iff_Stream);
        } /* if ((iff->iff_Stream = (IPTR)OpenClipboard(clipUnit))) */
        FreeIFF(iff);
    } /* if (iff) */
    
    return (filebuffer);
}


