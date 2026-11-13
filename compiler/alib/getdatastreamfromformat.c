/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    Internal utility functions.
*/

#include <aros/debug.h>
#include <aros/system.h>
#include "alib_intern.h"

/******************************************************************************

    NAME */
        VOID GetDataStreamFromFormat (

/*  SYNOPSIS */
        CONST_STRPTR format,
        va_list args,
        RAWARG dataStream, ULONG *dataSize,
        ULONG *indexStream, ULONG *indexSize)

/*  FUNCTION
        Builds an array of parameters which are passed on the stack.
        This function is used on machines which have compilers which
        don't pass the arguments to a varargs function unlike the
        Amiga ones.

    INPUTS
        format - Exec/RawDoFmt or Locale/FormatString format string
        args   - This has to be initialized by va_start()
                 (not used if dataStream is NULL)
        dataStream - data buffer to write to
                     (can be NULL for sizing)
        dataSize  - size of the buffer
                     (can be NULL, or pointer to 0 for sizing)
                     updated to the actual size required at exit.
        indexStream- array of offsets to the Nth element in the dataStream
                     (can be NULL for sizing)
        indexSize  - size of the index, in bytes
                     (can be NULL, or pointer to 0 for sizing)
                     updated to the actual size required at exit.

    RESULT
        An array which can be passed to any function which expects the
        structure or NULL if something failed. This call may fail for
        different reasons on different systems. On some systems, NULL
        indicates that there was not enough memory.

    NOTES
        This structure converts from format types to the following
        dataStream values in the returned structure. The dataStream
        is suitable for use by Exec/RawDoFmt or Locale/FormatString,
        and is packed on WORD alignment. The indexStream is used
        internally by Locale/FormatString for format repositioning.

        Format code     GET_ARG() type           Datastream type
        -----------     -------------           ---------------
        %c              char (really int)       WORD
        %d/%D           int                     WORD
        %u/%U           unsigned int            UWORD
        %x/%X           unsigned int            UWORD
        %b              BPTR                    IPTR
        %s              const char *            IPTR
        %p/%P           void *                  IPTR
        %l<cdDuUxX>     LONG                    LONG
        %ll<cdDuUxX>    QUAD                    QUAD
        %i<cdDuUxX>     IPTR                    IPTR
        %%              -                       -

    EXAMPLE

    BUGS

    SEE ALSO
        exec.library/RawDoFmt(), locale.library/FormatString()

    INTERNALS

******************************************************************************/
{
    size_t len = dataSize ? *dataSize : 0;
    int    ilen = (indexSize ? *indexSize : 0) / sizeof(indexStream[0]);
    int    imax = 0;
    size_t size = 0;
    size_t isize = 0;
    size_t size_force = 0;
    BOOL in_format = FALSE;
    ULONG iflags = 0;
    size_t vsize = 0, dsize = 0;
    unsigned int argpos = 0;

#define IFLAG_SIGNED    (1UL << 31)
#define IFLAG_VSIZE(x)  ((x) << 16)
#define IFLAG_VSIZE_of(x)  (((x) >> 16) & 0xf)
#define IFLAG_DSIZE(x)  ((x) << 0)
#define IFLAG_DSIZE_of(x)  (((x) >> 0) & 0xf)
#define GET_ARG(args, type) (dataStream ? va_arg(args, type) : 0)

    for (size = 0; format && *format; format++) {
        if (!in_format) {
            if (*format == '%') {
                in_format = TRUE;
                vsize = dsize = 0;
                size_force = 0;
                argpos = 0;
                iflags = 0;
            }
        } else {
            /* Ignore '%%' */
            if (*format == '%') {
                in_format = FALSE;
                continue;
            }

            /* Ignore non-argument characters */
            if ((*format >= '0' && *format <= '9')) {
                argpos = (argpos * 10) + (*format - '0');
                continue;
            }

            switch (*format) {
            case '.':
                break;
            case '-':
                break;
            case '$':
                isize = argpos - 1;
                argpos = 0;
                break;
            case 'i':
                size_force = sizeof(IPTR);
                break;
            case 'l':
                if (size_force >= sizeof(LONG))
                    size_force = sizeof(QUAD);
                else
                    size_force = sizeof(LONG);
                break;
            case 'd':
            case 'D':
                vsize = sizeof(int);
                dsize = sizeof(WORD);
                iflags |= IFLAG_SIGNED;
                in_format = FALSE;
                break;
            case 'c':   /* char is promoted to int through (fmt, ...) */
            case 'u':
            case 'U':
            case 'x':
            case 'X':
                vsize = sizeof(unsigned int);
                dsize = sizeof(WORD);
                in_format = FALSE;
                break;
            case 'p':
            case 'P':
            case 's':
                vsize = sizeof(void *);
                dsize = sizeof(IPTR);
                in_format = FALSE;
                break;
            case 'b':
            case 'B':
                vsize = sizeof(BPTR);
                dsize = sizeof(BPTR);
                in_format = FALSE;
                break;
            default:
                vsize = sizeof(int);
                dsize = sizeof(WORD);
                in_format = FALSE;
                break;
            }

            if (in_format == FALSE) {
                D(bug("%s: '%c' l=%d (v=%d, d=%d)\n", __func__, *format, size_force, vsize, dsize));
                if (size_force) {
                    vsize = size_force;
                    dsize = size_force;
                }

                iflags |= IFLAG_VSIZE(vsize);
                iflags |= IFLAG_DSIZE(dsize);

                if (ilen > isize)
                    indexStream[isize] = iflags;

                isize++;
                if (isize > imax)
                    imax = isize;
            }
        }
    }

    /* Convert indexStream flags into offsets
     * into the datastream. If the datastream
     * is present, assume we have to pull its
     * data from the va_list.
     */
    if (indexStream) {
        int i;

        for (i = 0; i < imax && i < ilen; i++) {
            IPTR arg_val;
            CONST_APTR buff = (CONST_APTR)((IPTR)dataStream + size);
            ULONG iflags = indexStream[i];

            D(bug("%s: indexStream[%d] = %d\n", __func__, i, (int)size));
            indexStream[i] = size;
            size += IFLAG_DSIZE_of(iflags);

            if (!dataStream)
                continue;

            /* dataStream present - pull its data from the va_list.
             */
            switch (IFLAG_VSIZE_of(iflags)) {
            case sizeof(LONG):
                if (iflags & IFLAG_SIGNED)
                    arg_val = (SIPTR)va_arg(args, LONG);
                else
                    arg_val = (IPTR)va_arg(args, ULONG);
                break;
           case sizeof(QUAD):
                if (iflags & IFLAG_SIGNED)
                    arg_val = (SIPTR)va_arg(args, QUAD);
                else
                    arg_val = (IPTR)va_arg(args, UQUAD);
                break;
           default:
                if (iflags & IFLAG_SIGNED)
                    arg_val = (SIPTR)va_arg(args, int);
                else
                    arg_val = (IPTR)va_arg(args, int);
                break;
            }

            D(bug("%s: dataStream len = 0x%x (%d)\n", __func__, (unsigned int)len, dataSize ? (int)*dataSize : -1));
            D(bug("%s: dataStream + 0x%x (%d) = 0x%p\n", __func__, (int)((IPTR)buff - (IPTR)dataStream), IFLAG_DSIZE_of(iflags), (void *)arg_val));
            if (len >= size) {
                switch (IFLAG_DSIZE_of(iflags)) {
                case sizeof(UWORD):
                    *((UWORD *)buff) = (UWORD)arg_val;
                    break;
                case sizeof(ULONG):
                    *((ULONG *)buff) = (ULONG)arg_val;
                    break;
                case sizeof(UQUAD):
                    *((UQUAD *)buff) = (UQUAD)arg_val;
                    break;
                }
            }
        }
    }

    if (dataSize)
        *dataSize = size;

    if (indexSize)
        *indexSize = imax * sizeof(indexStream[0]);
} /* GetDataStreamFromFormat */
