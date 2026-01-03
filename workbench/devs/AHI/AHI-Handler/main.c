/*
     AHI-Handler - The AUDIO: DOS device for AHI
     Copyright (C) 2017-2023 The AROS Dev Team
     Copyright (C) 1997-2005 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

/*
 * This code is written using DICE, and is based on the DosHan example
 * source code that came with the compiler. Not all comments are mine,
 * by the way...
 *
 */

#if !defined(DEBUG)
#define DEBUG 0
#endif

#if defined(__AROS__)
#define __NOLIBBASE__
#include <aros/debug.h>
#endif

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/rdargs.h>

#include <devices/ahi.h>
#include <proto/ahi.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <clib/alib_protos.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#if defined(__AROS__)
#include <aros/cpu.h>
#endif

#include "main.h"
#include "version.h"

#if defined(__AROS__)
#define PARAMSYSBASE    struct ExecBase *SysBase,
#define _SysBase            SysBase
#define ATTRIBSYSBASE   SysBase,
#else
#define PARAMSYSBASE
#define _SysBase
#define ATTRIBSYSBASE
#endif

/*
 *  Prototypes
 */

LONG PlayAndSwap(PARAMSYSBASE struct HandlerData *, LONG);
long extended2long(extended *);
void ulong2extended(ULONG, extended *);
void FillAIFFheader(struct HandlerData *);
void FillAIFCheader(struct HandlerData *);
LONG ReadCOMMchunk(struct HandlerData *, UBYTE *, LONG);
long AllocAudio(PARAMSYSBASE int);
#if defined(__AROS__)
void FreeAudio(struct ExecBase *);
#else
void FreeAudio(void);
#endif
long ParseArgs(PARAMSYSBASE struct HandlerData *, char *);
long InitHData(struct HandlerData *);
void FreeHData(PARAMSYSBASE struct HandlerData *);
void returnpacket(PARAMSYSBASE struct DosPacket *);
void Initialize(PARAMSYSBASE struct DosPacket *);
void UnInitialize(void);

/*
 *  Some macros
 */

#define min(a,b) ((a)<=(b)?(a):(b))
#define DOS_TRUE    -1
#define DOS_FALSE   0
#if !defined(__AROS__)
#define BTOC(bptr)  ((void *)((long)(bptr) << 2))
#define CTOB(cptr)  ((BPTR)(((long)cptr) >> 2))
#else
#define BTOC(bptr)  BADDR(bptr)
#define CTOB(cptr)  MKBADDR(cptr)
#endif

static inline UWORD HostToBE16(UWORD value)
{
#if !defined(__AROS__) || AROS_BIG_ENDIAN
    return value;
#else
    return (UWORD)((value << 8) | (value >> 8));
#endif
}

static inline ULONG HostToBE32(ULONG value)
{
#if !defined(__AROS__) || AROS_BIG_ENDIAN
    return value;
#else
    return ((value & 0x000000FFUL) << 24) |
           ((value & 0x0000FF00UL) << 8) |
           ((value & 0x00FF0000UL) >> 8) |
           ((value & 0xFF000000UL) >> 24);
#endif
}

static inline UWORD BE16ToHost(UWORD value)
{
    return HostToBE16(value);
}

static inline ULONG BE32ToHost(ULONG value)
{
    return HostToBE32(value);
}

#if !defined(__AROS__)
/*
 *  My debug stuff....
 */

#ifdef DEBUG

#define HIT(x) {char *a=NULL; *a=x;}

static const UWORD rawputchar_m68k[] = {
    0x2C4B,             // MOVEA.L A3,A6
    0x4EAE, 0xFDFC,     // JSR     -$0204(A6)
    0x4E75              // RTS
};

void
KPrintFArgs(UBYTE *fmt,
            IPTR *args)
{
    RawDoFmt(fmt, args, (void(*)(void)) rawputchar_m68k, SysBase);
}

#define bug( fmt, ... )        \
({                                 \
  IPTR _args[] = { __VA_ARGS__ }; \
  KPrintFArgs( (fmt), _args );     \
})

#endif /* DEBUG */


/*
 *  Global variables
 */

static
#else
__section(".text.romtag")
#endif
const char ID[] = "$VER: AHI-Handler " VERS "\r\n";

struct MsgPort    *PktPort;
struct DeviceNode *DevNode;
int                AllocCnt;

struct MsgPort    *AHImp     = NULL;
struct AHIRequest *AHIio     = NULL;
BYTE               AHIDevice = -1;
struct Library    *AHIBase;

struct AIFCHeader AIFCHeader = {
    ID_FORM, 0, ID_AIFC,
    ID_FVER, sizeof(FormatVersionHeader), {
        AIFCVersion1
    },
    ID_COMM, sizeof(ExtCommonChunk), {
        0,
        0,
        0,
        {0, {0, 0}},
        NO_COMPRESSION,
        {
            sizeof("not compressed") - 1,
            'n', 'o', 't', ' ', 'c', 'o', 'm', 'p', 'r', 'e', 's', 's', 'e', 'd'
        }
    },
    ID_SSND, 0, {0, 0}
};

struct AIFFHeader AIFFHeader = {
    ID_FORM, 0, ID_AIFF,
    ID_COMM, sizeof(CommonChunk), {
        0,
        0,
        0,
        {0, {0, 0}}
    },
    ID_SSND, 0, {0, 0}
};


/******************************************************************************
**** Entry ********************************************************************
******************************************************************************/

#if !defined(__AROS__)
// Disable command line processing
const long __nocommandline = 1;

// We (mis)use this one directly instead
extern struct Message *_WBenchMsg;

int main(void)
{
#else
LONG handler(struct ExecBase *SysBase)
{
#endif
    struct DosPacket *packet;
    struct Process *proc;

    BOOL Running;

    D(bug("[AHI-Handler] %s: The very first call...\n", __func__);)

    proc = (struct Process *) FindTask(NULL);

    PktPort = &proc->pr_MsgPort;
#if !defined(__AROS__)
    Initialize((struct DosPacket *) _WBenchMsg->mn_Node.ln_Name);
    // Make sure startup code does not reply the message on exit
    _WBenchMsg = NULL;
#else
    WaitPort(PktPort);
    Initialize(ATTRIBSYSBASE(struct DosPacket *)GetMsg(PktPort)->mn_Node.ln_Name);
#endif

    Running = TRUE;
    AllocCnt = 0;

    D(bug("[AHI-Handler] %s: Init\n", __func__);)

    /*
     *        Main Loop
     */

    while(Running) {
        struct Message *msg;

        while((msg = GetMsg(PktPort)) == NULL)
            Wait(1 << PktPort->mp_SigBit);
        packet = (struct DosPacket *) msg->mn_Node.ln_Name;

        D(bug("[AHI-Handler] %s: Got packet: %ld\n", __func__, packet->dp_Type);)

        /*
         *  default return value
         */

        packet->dp_Res1 = DOS_TRUE;
        packet->dp_Res2 = 0;

        /*
         *  switch on packet
         */

        switch(packet->dp_Type) {

        case ACTION_DIE: {      /*  ??? */
            break;
        }

        /***********************************************************************/

        case ACTION_FINDUPDATE: /*  FileHandle,Lock,Name        Bool        */
        case ACTION_FINDINPUT:  /*  FileHandle,Lock,Name        Bool        */
        case ACTION_FINDOUTPUT: { /*  FileHandle,Lock,Name        Bool        */
            struct FileHandle *fh = BTOC(packet->dp_Arg1);
            unsigned char *base;
            unsigned int len;
            char buf[128];
            struct HandlerData *data;
            int unit = AHI_DEFAULT_UNIT;

#if !defined(__AROS__)
            base = BTOC(packet->dp_Arg3);
            len = *base;
#else
            base = AROS_BSTR_ADDR(packet->dp_Arg3);
            len = AROS_BSTR_strlen(packet->dp_Arg3);
#endif

            D(
                bug("[AHI-Handler] %s: ACTION_FIND#?: fh @ 0x%p\n", __func__, (char *) fh);
                bug("[AHI-Handler] %s: ACTION_FIND#?: base @ 0x%p (len = %u)\n", __func__, (char *) base, len);
                bug("[AHI-Handler] %s: ACTION_FIND#?:         =  '%s'\n", __func__, (char *) base);
            )
            // Skip volume name and ':'
            if(len > 0) {
                unsigned int cnt;
                for(cnt = 0; cnt < len; cnt++) {
                    if(base[cnt] == ':') {
                        len = len - ((IPTR)&base[cnt] - (IPTR)base);
                        base = &base[cnt + 1];
                        break;
                    }
                }

                {
                    // Convert /'s to blanks
                    char *p = base;

                    while(*++p)
                        if(*p == '/')
                            *p = ' ';
                }

                if(len >= sizeof(buf))
                    len = sizeof(buf) - 1;

                strncpy(buf, base, len);
                if(buf[len - 1] != '\n') {
                    buf[len] = '\n';
                    len += 1;
                }
            }
            buf[len] = 0;

            D(bug("[AHI-Handler] %s: ACTION_FIND#?: '%s'\n", __func__, (char *) buf);)

            data = AllocVec(sizeof(struct HandlerData), MEMF_PUBLIC | MEMF_CLEAR);
            if(! data) {
                packet->dp_Res1 = DOS_FALSE;
                packet->dp_Res2 = ERROR_NO_FREE_STORE;
                break;
            }

            if((packet->dp_Res2 = ParseArgs(ATTRIBSYSBASE data, (char *) buf)) != 0) {
                FreeHData(ATTRIBSYSBASE data);
                packet->dp_Res1 = DOS_FALSE;
                break;
            }

            if(data->args.unit) {
                unit = *data->args.unit;
            }

            if((packet->dp_Res2 = AllocAudio(ATTRIBSYSBASE unit))) {
                FreeAudio(_SysBase);
                FreeHData(ATTRIBSYSBASE data);
                packet->dp_Res1 = DOS_FALSE;
                break;
            }

            fh->fh_Arg1 = (IPTR) data;
            fh->fh_Port = DOS_TRUE;
            break;
        }

        /***********************************************************************/

        case ACTION_READ: {      /*  FHArg1,CPTRBuffer,Length    ActLength   */

            /*
             *   Reading is straightforward except for handling EOF... We
             *  must guarentee a return value of 0 (no bytes left) before
             *  beginning to return EOFs (-1's).  If we return a negative
             *  number right off programs like COPY will assume a failure
             *  (if AUDIO: is the source) and delete the destination file.
             *
             *  The basic idea is to feed the packets from one buffer while
             *  recording asyncroniously to the other. When we have read
             *  the buffer, we wait until the other is filled, and switch
             *  buffer pointers.
             */

            struct HandlerData *data = (struct HandlerData *) packet->dp_Arg1;
            UBYTE *dest   = (void *) packet->dp_Arg2;
            LONG   length, filled;

            D(bug("[AHI-Handler] %s: ACTION_READ: 0x%08lx, %ld\n", __func__, packet->dp_Arg2, packet->dp_Arg3);)

            if(! data->initialized) {
                packet->dp_Res2 = InitHData(data);
                if(packet->dp_Res2) {
                    packet->dp_Res1 = -1;
                    break;
                }
            }

            length = filled = min(data->totallength, packet->dp_Arg3);

            if(length <= 0) {
                packet->dp_Res1 = length;
                data->totallength = -1;
                break;
            }

            if(data->buffer1 == NULL) {

                data->buffer1 = AllocVec(data->buffersize, MEMF_PUBLIC);
                data->buffer2 = AllocVec(data->buffersize, MEMF_PUBLIC);
                data->readreq    = AllocVec(sizeof(struct AHIRequest), MEMF_PUBLIC);

                if((data->buffer1 == NULL)
                        || (data->buffer2 == NULL)
                        || (data->readreq    == NULL)) {
                    packet->dp_Res1 = -1;
                    packet->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }

                CopyMem(AHIio, data->readreq, sizeof(struct AHIRequest));

                // Fill buffer 2
                // Note that io_Offset is always 0 the first time

                data->readreq->ahir_Std.io_Command  = CMD_READ;
                data->readreq->ahir_Std.io_Data     = data->buffer2;
                data->readreq->ahir_Std.io_Length   = data->buffersize;
                data->readreq->ahir_Std.io_Offset   = 0;
                data->readreq->ahir_Type            = data->type;
                data->readreq->ahir_Frequency       = data->freq;
                data->readreq->ahir_Volume          = data->vol;
                data->readreq->ahir_Position        = data->pos;
                SendIO((struct IORequest *) data->readreq);

                // Force buffer switch filling of the other buffer

                data->length = data->offset = 0;

                // Check if we should write a header first

                if(data->format == AIFF) {
                    if(length < (LONG) sizeof(struct AIFFHeader)) {
                        packet->dp_Res1 = -1;
                        packet->dp_Res2 = ERROR_BAD_NUMBER;
                        break;
                    }

                    FillAIFFheader(data);

                    CopyMem(&AIFFHeader, dest, sizeof(struct AIFFHeader));
                    dest              += sizeof(struct AIFFHeader);
                    length            -= sizeof(struct AIFFHeader);
                }

                else if(data->format == AIFC) {
                    if(length < (LONG) sizeof(struct AIFCHeader)) {
                        packet->dp_Res1 = -1;
                        packet->dp_Res2 = ERROR_BAD_NUMBER;
                        break;
                    }

                    FillAIFCheader(data);

                    CopyMem(&AIFCHeader, dest, sizeof(struct AIFCHeader));
                    dest              += sizeof(struct AIFCHeader);
                    length            -= sizeof(struct AIFCHeader);
                }
            }


            while(length > 0) {
                LONG thislength;

                if(data->offset >= data->length) {
                    void *temp;

                    temp          = data->buffer1;
                    data->buffer1 = data->buffer2;
                    data->buffer2 = temp;

                    if(WaitIO((struct IORequest *) data->readreq)) {
                        packet->dp_Res1 = -1;
                        if(data->readreq->ahir_Std.io_Error == AHIE_HALFDUPLEX) {
                            packet->dp_Res2 = ERROR_OBJECT_IN_USE;
                        } else {
                            packet->dp_Res2 = ERROR_READ_PROTECTED;
                        }
                        break;
                    }

                    data->length = data->readreq->ahir_Std.io_Actual;
                    data->offset = 0;

                    data->readreq->ahir_Std.io_Command = CMD_READ;
                    data->readreq->ahir_Std.io_Data    = data->buffer2;
                    data->readreq->ahir_Std.io_Length  = data->buffersize;
                    data->readreq->ahir_Type           = data->type;
                    data->readreq->ahir_Frequency      = data->freq;
                    data->readreq->ahir_Volume         = data->vol;
                    data->readreq->ahir_Position       = data->pos;
                    SendIO((struct IORequest *) data->readreq);
                } /* if */

                thislength = min(data->length - data->offset, length);
                CopyMem(data->buffer1 + data->offset, dest, thislength);
                dest              += thislength;
                length            -= thislength;
                data->offset      += thislength;
                data->totallength -= thislength;
            } /* while */

            if(packet->dp_Res2 == 0) {
                packet->dp_Res1 = filled;
            }
            break;

            } /* ACTION_READ */

        /***********************************************************************/

        case ACTION_WRITE: {      /*  FHArg1,CPTRBuffer,Length    ActLength   */
            struct HandlerData *data  = (struct HandlerData *) packet->dp_Arg1;
            UBYTE *src    = (void *) packet->dp_Arg2;
            LONG   length = packet->dp_Arg3, filled;

            D(bug("[AHI-Handler] %s: ACTION_WRITE: 0x%p, %ld\n", __func__, src, length);)

            if(data->buffer1 == NULL) {
                // Check headers?

                switch(data->args.format) {
                case AIFF:
                    if((((ULONG *) src)[0] != ID_FORM)
                            || (((ULONG *) src)[2] != ID_AIFF)) {
                        packet->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                    }
                    break;

                case AIFC:
                    if((((ULONG *) src)[0] != ID_FORM)
                            || (((ULONG *) src)[2] != ID_AIFC)) {
                        packet->dp_Res2 = ERROR_OBJECT_WRONG_TYPE;
                    }
                    break;

                case 0:
                    if(((ULONG *) src)[0] == ID_FORM) {
                        if(((ULONG *) src)[2] == ID_AIFF) {
                            data->args.format = AIFF;
                        } else if(((ULONG *) src)[2] == ID_AIFC) {
                            data->args.format = AIFC;
                        }
                    }
                    break;

                default:
                    break;
                }

                if(packet->dp_Res2) {
                    packet->dp_Res1 = -1;
                    break;
                }

                if((data->args.format == AIFF) || (data->args.format == AIFC)) {
                    LONG skiplen = 0;

                    skiplen = ReadCOMMchunk(data, src, length);
                    D(bug("[AHI-Handler] %s: ACTION_WRITE: skiplen = %ld\n", __func__, skiplen);)

                    src    += skiplen;
                    length -= skiplen;
                }

                if((packet->dp_Res2 = InitHData(data))) {
                    packet->dp_Res1 = -1;
                    break;
                }

                data->writing = TRUE;

                data->buffer1 = AllocVec(data->buffersize, MEMF_PUBLIC);
                data->buffer2 = AllocVec(data->buffersize, MEMF_PUBLIC);

                if((data->buffer1 == NULL) || (data->buffer2 == NULL)) {
                    packet->dp_Res1 = -1;
                    packet->dp_Res2 = ERROR_NO_FREE_STORE;
                    break;
                }

                data->offset = 0;
                data->length = (data->buffersize / AHI_SampleFrameSize(data->type))
                               * AHI_SampleFrameSize(data->type);

            }

            length = min(data->totallength, length);
            filled = min(data->totallength, packet->dp_Arg3);

            D(bug("[AHI-Handler] %s: ACTION_WRITE: length = %ld, filled = %ld\n", __func__, length, filled);)

            while(length > 0) {
                LONG thislength;

                if(data->offset >= data->length) {
                    packet->dp_Res2 = PlayAndSwap(ATTRIBSYSBASE data, data->length);
                    if(packet->dp_Res2) {
                        packet->dp_Res1 = -1;
                        break;
                    }
                }

                thislength = min(data->length - data->offset, length);
                CopyMem(src, data->buffer1 + data->offset, thislength);
                src               += thislength;
                length            -= thislength;
                data->offset      += thislength;
                data->totallength -= thislength;

            } /* while */

            if(packet->dp_Res2 == 0) {
                packet->dp_Res1 = filled;
            }
            break;
        }

        /***********************************************************************/

        case ACTION_END: {      /*  FHArg1                      Bool:TRUE   */
            struct HandlerData *data = (struct HandlerData *) packet->dp_Arg1;

            D(bug("[AHI-Handler] %s: ACTION_END\n", __func__);)

            // Abort any reading requests

            if(data->readreq) {
                AbortIO((struct IORequest *) data->readreq);
                WaitIO((struct IORequest *) data->readreq);
            }

            // Finish any playing requests

            if(data->writing) {
                PlayAndSwap(ATTRIBSYSBASE data, data->offset);

                if(data->writereq1) {
                    WaitIO((struct IORequest *) data->writereq1);
                }
                if(data->writereq2) {
                    WaitIO((struct IORequest *) data->writereq2);
                }
            }

            FreeHData(ATTRIBSYSBASE data);
            FreeAudio(_SysBase);


            break;
        }

        /***********************************************************************/

        case ACTION_IS_FILESYSTEM:
            packet->dp_Res1 = DOS_FALSE;
            break;

        /***********************************************************************/

        default:
            D(bug("[AHI-Handler] %s: Unknown packet!\n", __func__);)
            packet->dp_Res1 = DOS_FALSE;
            packet->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
            break;

        } /* switch */

        if(AllocCnt == 0)
            Running = FALSE;

        if(packet) {
            returnpacket(ATTRIBSYSBASE packet);
            D(bug("[AHI-Handler] %s: Returned packet\n", __func__);)
        }
    } /* for */

    D(bug("[AHI-Handler] %s: Dying..!\n", __func__);)

    UnInitialize();
#if !defined(__AROS__)
    _exit(0);
#else
    return RETURN_OK;
#endif
}


/******************************************************************************
**** PlayAndSwap **************************************************************
******************************************************************************/

/*
 *  Starts to play the current buffer. Handles double buffering.
 */

LONG PlayAndSwap(PARAMSYSBASE struct HandlerData *data, LONG length)
{
    void *temp;

    temp          = data->buffer1;
    data->buffer1 = data->buffer2;
    data->buffer2 = temp;

    temp            = data->writereq1;
    data->writereq1 = data->writereq2;
    data->writereq2 = temp;


    if(data->writereq1 == NULL) {
        data->writereq1 = AllocVec(sizeof(struct AHIRequest), MEMF_PUBLIC);

        if(data->writereq1 == NULL) {
            return ERROR_NO_FREE_STORE;
        }

        CopyMem(AHIio, data->writereq1, sizeof(struct AHIRequest));
    }

    data->offset = 0;

    data->writereq1->ahir_Std.io_Message.mn_Node.ln_Pri = data->priority;
    data->writereq1->ahir_Std.io_Command = CMD_WRITE;
    data->writereq1->ahir_Std.io_Data    = data->buffer2;
    data->writereq1->ahir_Std.io_Length  = length;
    data->writereq1->ahir_Std.io_Offset  = 0;
    data->writereq1->ahir_Type           = data->type;
    data->writereq1->ahir_Frequency      = data->freq;
    data->writereq1->ahir_Volume         = data->vol;
    data->writereq1->ahir_Position       = data->pos;
    data->writereq1->ahir_Link           = data->writereq2;
    SendIO((struct IORequest *) data->writereq1);

    if(data->writereq2) {
        if(WaitIO((struct IORequest *) data->writereq2)) {
            if(data->writereq2->ahir_Std.io_Error == AHIE_HALFDUPLEX) {
                return ERROR_OBJECT_IN_USE;
            } else {
                return ERROR_WRITE_PROTECTED;
            }
        }
    }

    return 0;
}


/******************************************************************************
**** extended2long ************************************************************
******************************************************************************/

/*
 *  This function translates Apples SANE Extended  used in AIFF/AIFC files
 *  to a LONG. Stolen from Olaf `Olsen' Barthel's AIFF datatype.
 */


long extended2long(extended *ex)
{
    unsigned long mantissa;
    long exponent, sign;

    // We only need 32 bits precision

    mantissa = ex->mantissa[0];

    // Is the mantissa positive or negative?

    exponent = ex->exponent;

    if(exponent & 0x8000)
        sign = -1;
    else
        sign =  1;

    // Unbias the exponent

    exponent = (exponent & 0x7FFF) - 0x3FFF;

    // If the exponent is negative, set the mantissa to zero

    if(exponent < 0)
        mantissa = 0;
    else {
        // Special meaning?

        exponent -= 31;

        // Overflow?

        if(exponent > 0)
            mantissa = 0x7FFFFFFF;
        else
            mantissa >>= -exponent; // Let the point float...
    }

    // That's all...

    return(sign * (long)mantissa);
}


/******************************************************************************
**** ulong2extended ***********************************************************
******************************************************************************/

/*
 *  This function translates an ULONG to Apples SANE Extended
 *  used in AIFF/AIFC files.
 */

void ulong2extended(ULONG in, extended *ex)
{
    ex->exponent = 31 + 16383;
    ex->mantissa[1] = 0;
    while(!(in & 0x80000000)) {
        ex->exponent--;
        in <<= 1;
    }
    ex->mantissa[0] = in;
}


/******************************************************************************
**** FillAIFFheader ***********************************************************
******************************************************************************/

void FillAIFFheader(struct HandlerData *data)
{

    AIFFHeader.FORMsize = HostToBE32(data->totallength - 8);
    AIFFHeader.COMMchunk.numChannels = HostToBE16(data->channels);
    AIFFHeader.COMMchunk.numSampleFrames =
        HostToBE32(data->totallength / AHI_SampleFrameSize(data->type));
    AIFFHeader.COMMchunk.sampleSize = HostToBE16(data->bits);
    ulong2extended(data->freq, &AIFFHeader.COMMchunk.sampleRate);
    AIFFHeader.SSNDsize = HostToBE32(sizeof(SampledSoundHeader) +
                                     data->totallength - sizeof(AIFFHeader));
}


/******************************************************************************
**** FillAIFCheader ***********************************************************
******************************************************************************/

void FillAIFCheader(struct HandlerData *data)
{

    AIFCHeader.FORMsize = HostToBE32(data->totallength - 8);
    AIFCHeader.COMMchunk.numChannels = HostToBE16(data->channels);
    AIFCHeader.COMMchunk.numSampleFrames =
        HostToBE32(data->totallength / AHI_SampleFrameSize(data->type));
    AIFCHeader.COMMchunk.sampleSize = HostToBE16(data->bits);
    ulong2extended(data->freq, &AIFCHeader.COMMchunk.sampleRate);
    AIFCHeader.SSNDsize = HostToBE32(sizeof(SampledSoundHeader) +
                                     data->totallength - sizeof(AIFFHeader));
}


/******************************************************************************
**** ReadCOMMchunk ************************************************************
******************************************************************************/

LONG ReadCOMMchunk(struct HandlerData *data, UBYTE *buffer, LONG length)
{
    UWORD *src = (UWORD *) buffer;
    LONG   len = (length >> 1) - 2;
    ExtCommonChunk *common;

    while(len > 0) {
        if(((ULONG *) src)[0] == ID_COMM) {
            common = (ExtCommonChunk *)(src + 4);
            data->channels    = BE16ToHost(common->numChannels);
            data->bits        = BE16ToHost(common->sampleSize);
            data->totallength = BE32ToHost(common->numSampleFrames) * data->channels *
                                (data->bits <= 8 ? 1 : (data->bits <= 16 ? 2 : (data->bits <= 32 ? 4 : 0)));
            data->freq = extended2long(&common->sampleRate);

            if(!data->args.channels)
                data->args.channels = &data->channels;
            if(!data->args.bits)
                data->args.bits     = &data->bits;
            if(!data->args.length)
                data->args.length   = &data->totallength;
            if(!data->args.freq)
                data->args.freq     = &data->freq;
        } else if(((ULONG *) src)[0] == ID_SSND) {
            src += 8;
            break;
        }
        src++;
        len--;
    }
    return (LONG)((SIPTR)src - (SIPTR) buffer);
}

/******************************************************************************
**** AllocAudio ***************************************************************
******************************************************************************/

/*
 *  If the device isn't already open, open it now
 */

long AllocAudio(PARAMSYSBASE int unit)
{
    long rc = 0;

    if(++AllocCnt == 1) {
        if((AHImp = CreateMsgPort())) {
            if((AHIio = (struct AHIRequest *)CreateIORequest(
                            AHImp, sizeof(struct AHIRequest)))) {
                AHIio->ahir_Version = 4;
                AHIDevice = OpenDevice(AHINAME, unit, (struct IORequest *)AHIio, 0);
            }
        }

        if(AHIDevice) {
            rc = ERROR_OBJECT_NOT_FOUND;
        } else {
            AHIBase = (struct Library *)AHIio->ahir_Std.io_Device;
        }
    }
    return rc;
}


/******************************************************************************
**** FreeAudio ****************************************************************
******************************************************************************/

/*
 *  If we're the last user, close the device now
 */

#if defined(__AROS__)
void FreeAudio(struct ExecBase *SysBase)
#else
void FreeAudio(void)
#endif
{
    if(--AllocCnt == 0) {
        if(AHIDevice == 0)
            CloseDevice((struct IORequest *)AHIio);
        AHIDevice = -1;
        DeleteIORequest((struct IORequest *)AHIio);
        AHIio = NULL;
        DeleteMsgPort(AHImp);
        AHImp = NULL;
    }
}


/******************************************************************************
**** ParseArgs ****************************************************************
******************************************************************************/

/*
 *  Fill out argument array. Returns 0 on success, else a DOS error code.
 */

long ParseArgs(PARAMSYSBASE struct HandlerData *data, char *initstring)
{
#if defined(__AROS__)
    struct Library *DOSBase;
    struct UtilityBase *UtilityBase;
#endif
    long rc = 0;

#if defined(__AROS__)
    if((DOSBase = OpenLibrary("dos.library", 0)) == NULL) {
        D(bug("[AHI-Handler] %s: failed to open dos.library\n", __func__);)

        return ERROR_BAD_TEMPLATE;
    }
    if((UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 0)) == NULL) {
        D(bug("[AHI-Handler] %s: failed to open utility.library\n", __func__);)

        return ERROR_BAD_TEMPLATE;
    }
#endif

    data->rdargs = (struct RDArgs *) AllocDosObjectTags(DOS_RDARGS, TAG_DONE);
    if(data->rdargs) {
        data->rdargs->RDA_Source.CS_Buffer = initstring;
        data->rdargs->RDA_Source.CS_Length = strlen(initstring);
        data->rdargs->RDA_Source.CS_CurChr = 0;
        data->rdargs->RDA_Flags |= RDAF_NOPROMPT;

        data->rdargs2 = ReadArgs(
                            "B=BITS/K/N,C=CHANNELS/K/N,F=FREQUENCY/K/N,T=TYPE/K,V=VOLUME/K/N,P=POSITION/K/N,"
                            "PRI=PRIORITY/K/N,L=LENGTH/K/N,S=SECONDS/K/N,BUF=BUFFER/K/N,UNIT/K/N",
                            (IPTR *) &data->args, data->rdargs);

        if(data->rdargs2 != NULL) {


            if(! data->args.type) {
                data->args.format = 0;
            } else if(Stricmp("SIGNED", data->args.type) == 0) {
                data->args.format = SIGNED;
            } else if(Stricmp("AIFF", data->args.type) == 0) {
                data->args.format = AIFF;
            } else if(Stricmp("AIFC", data->args.type) == 0) {
                data->args.format = AIFC;
            } else {
                D(bug("[AHI-Handler] %s: Unhandled type %s\n", __func__, data->args.type);)

                rc = ERROR_BAD_TEMPLATE;
            }
        } else {
            D(bug("[AHI-Handler] %s: ReadArgs failed\n", __func__);)

            rc = ERROR_BAD_TEMPLATE;
        }

    } else
        rc = ERROR_NO_FREE_STORE;

#if defined(__AROS__)
    CloseLibrary((struct Library *)UtilityBase);
    CloseLibrary(DOSBase);
#endif

    return rc;
}


/******************************************************************************
**** InitHData ****************************************************************
******************************************************************************/

/*
 *  Initialize the HandlerData data structure, based on the args structure
 *  (see ParseArgs()). Returns 0 on success, else a DOS error code.
 */

#define S8bitmode      0
#define S16bitmode     1
#define S32bitmode     8

#define Sstereoflag    2

long InitHData(struct HandlerData *data)
{
    ULONG bits = 8, channels = 1, freq = 8000;
    LONG  volume = 100, position = 0, priority = 0,
          length = MAXINT, buffersize = 32768;
    long rc = 0;

    data->initialized = TRUE;

    // Fill in default values

    if(!data->args.bits)
        data->args.bits = &bits;
    if(!data->args.channels)
        data->args.channels = &channels;
    if(!data->args.freq)
        data->args.freq = &freq;
    if(!data->args.volume)
        data->args.volume = &volume;
    if(!data->args.position)
        data->args.position = &position;
    if(!data->args.priority)
        data->args.priority = &priority;
    if(!data->args.length)
        data->args.length = &length;
    if(!data->args.buffersize)
        data->args.buffersize = &buffersize;

    if(!data->args.format)
        data->args.format = SIGNED;

    // 8, 16 or 32 bit

    if(*data->args.bits <= 8)
        data->type = S8bitmode;
    else if(*data->args.bits <= 16)
        data->type = S16bitmode;
    else if(*data->args.bits > 24 && *data->args.bits <= 32)
        data->type = S32bitmode;
    else {
        rc = ERROR_OBJECT_WRONG_TYPE;
        goto quit;
    }

    // Mono or stereo

    if((*data->args.channels > 2) || (*data->args.channels < 1)) {
        rc = ERROR_OBJECT_WRONG_TYPE;
        goto quit;
    }

    if(*data->args.channels == 2)
        data->type |= Sstereoflag;

    data->bits     = *data->args.bits;
    data->channels = *data->args.channels;
    data->freq     = *data->args.freq;
    data->vol      = *data->args.volume * 0x10000 / 100;
    {
        // Don't ask why... :(
        LONG a;
        a = *data->args.position * 0x8000;
        a = a / 100 + 0x8000;
        data->pos      = a;
    }
    data->priority = *data->args.priority;

    if(data->args.seconds) {
        data->totallength = *data->args.seconds * data->freq
                            * AHI_SampleFrameSize(data->type);
    } else {
        data->totallength = (*data->args.length / AHI_SampleFrameSize(data->type))
                            * AHI_SampleFrameSize(data->type);
    }

    data->format = data->args.format;

    switch(data->format) {
    case AIFF:
    case AIFC:
        data->totallength = data->totallength & ~1;    // Make even
        break;
    }

    data->buffersize = *data->args.buffersize;

quit:
    return rc;
}











/******************************************************************************
**** FreeHData ****************************************************************
******************************************************************************/

/*
 *   Deallocate the HandlerData structure
 */

void FreeHData(PARAMSYSBASE struct HandlerData *data)
{
    if(data) {
#if defined(__AROS__)
        struct Library *DOSBase;
        if((DOSBase = OpenLibrary("dos.library", 0)) != NULL) {
#endif
            if(data->rdargs2)
                FreeArgs(data->rdargs2);
            if(data->rdargs)
                FreeDosObject(DOS_RDARGS, data->rdargs);
#if defined(__AROS__)
            CloseLibrary(DOSBase);
        }
#endif
        FreeVec(data->buffer1);
        FreeVec(data->buffer2);
        FreeVec(data->readreq);
        FreeVec(data->writereq1);
        FreeVec(data->writereq2);
        FreeVec(data);
    }
}


/******************************************************************************
**** returnpacket *************************************************************
******************************************************************************/

/*
 *  PACKET ROUTINES.  Dos Packets are in a rather strange format as you
 *  can see by this and how the PACKET structure is extracted in the
 *  GetMsg() of the main routine.
 */

void returnpacket(PARAMSYSBASE struct DosPacket *packet)
{
    struct Message *mess;
    struct MsgPort *replyPort;

    replyPort = packet->dp_Port;
    mess = packet->dp_Link;
    packet->dp_Port = PktPort;
    mess->mn_Node.ln_Name = (char *) packet;
    PutMsg(replyPort, mess);
}


/******************************************************************************
**** Initialize ***************************************************************
******************************************************************************/

/*
 *  During initialization DOS sends us a packet and sets our dn_SegList
 *  pointer.  If we set our dn_Task pointer than every Open's go to the
 *  same handler (this one).  If we set dn_Task to NULL, every Open()
 *  will create a NEW instance of this process via the seglist, meaning
 *  our process must be reentrant (i.e. -r option).
 *
 *  note: dn_Task points to the MESSAGE PORT portion of the process
 *  (or your own custom message port).
 *
 *  If we clear the SegList then we also force DOS to reload our process
 *  from disk, but we also need some way of then UnLoadSeg()ing it ourselves,
 *  which we CANNOT do from this process since it rips our code out from
 *  under us.
 */

void Initialize(PARAMSYSBASE struct DosPacket *packet)
{
    struct DeviceNode *dn;

    /*
     *        Handle initial message.
     */
    DevNode = dn = BTOC(packet->dp_Arg3);
    dn->dn_Task = NULL;

    D(bug("[AHI-Handler] %s: Replying it ...\n", __func__);)

    packet->dp_Res1 = DOS_TRUE;
    packet->dp_Res2 = 0;
    returnpacket(ATTRIBSYSBASE packet);
}


/******************************************************************************
**** UnInitialize *************************************************************
******************************************************************************/

void UnInitialize(void)
{
    struct DeviceNode *dn = DevNode;

    dn->dn_Task = NULL;
}

#if defined(__mc68000__) && defined(__libnix__)
void __main(void) {}
#endif
