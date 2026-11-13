/**************************************************************
**** Print.c : procedures for printing                     ****
**** Free software under GNU license, started on 2/2/2012  ****
**** © AROS Team                                           ****
**************************************************************/

#include <intuition/intuition.h>
#include <libraries/asl.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "Gui.h"
#include "Project.h"
#include "Utility.h"
#include "ProtoTypes.h"
#include "Print.h"
#include "DiskIO.h"

#define  CATCOMP_NUMBERS			/* Error msg use strings id */
#include "strings.h"

static inline BYTE PWrite(struct IORequest *io, CONST_STRPTR buffer, LONG len)
{
    struct IOStdReq *sio = (struct IOStdReq *)io;

    sio->io_Command = CMD_WRITE;
    sio->io_Data = (APTR)buffer;
    sio->io_Length = len;
    return (DoIO(io) == 0 && sio->io_Actual == len) ? 1 : 0;
}

/* Print a file
 */
BYTE print_file(LINE *svg, unsigned char eol)
{
    STRPTR buf;
    LONG   i;
    BYTE   szeol = szEOL[eol];
    LINE *ln;
    BYTE retval = 0;
    struct IORequest *io;
    struct MsgPort *mp;

    if ((mp = CreateMsgPort())) {
        if ((io = CreateIORequest(mp, sizeof(struct IOStdReq)))) {
            if (0 == OpenDevice("printer.device", print_unit(-1), io, 0)) {
                BusyWindow(Wnd);

                for(ln=svg, buf=NULL, i=0; ln; ln=ln->next)
                {
                    if (i == 0)
                        buf = ln->stream;

                    /* An unmodified line (2nd cond. is for deleted lines) */
                    if (ln->max == 0 && ln->stream-buf == i) {
                        i+=ln->size+szeol;
                    } else {
                        /* Flush preceding unmodified buffer */
                        i -= szeol;
                        if( i>=0 && (PWrite(io, buf, i) != 1 ||
                                PWrite(io, &chEOL[eol], szeol) != 1 ) )
                        {
                                retval = 0;
                                break;
                        }

                        /* Writes the modified line */
                        if( PWrite(io, ln->stream, ln->size) != 1 || (ln->next != NULL &&
                            PWrite(io, &chEOL[eol], szeol) != 1 ) ) {
                            retval = 0;
                            break;
                        }
                        i=0;
                    }
                }
                /* Flush buffer */
                if( i>szeol && PWrite(io, buf, i-szeol) !=1 ) {
                    retval = 0;
                }

                WakeUp(Wnd);

                CloseDevice(io);
            }
            DeleteIORequest(io);
        }
        DeleteMsgPort(mp);
    }

    return retval;
}

/* Get/set current printer.device unit
 *   If unit < 0, do not change current unit
 * Return:
 *   printer unit to be used for printing
 *
 */
BYTE print_unit(BYTE unit)
{
    static BYTE current_unit = 0;

    if (unit >= 0)
        current_unit = unit;

    return current_unit;
}
