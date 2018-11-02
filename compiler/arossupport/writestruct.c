/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Write a big endian structure to a streamhook
    Lang: english
*/

#include <string.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <utility/hooks.h>

struct WriteLevel
{
    struct MinNode   node;
    const IPTR     * sd;
    UBYTE          * s;
    int              pos;
};

/******************************************************************************

    NAME */
#include <stdio.h>
#include <aros/bigendianio.h>
#include <proto/alib.h>

        BOOL WriteStruct (

/*  SYNOPSIS */
        struct Hook * hook,
        void        * data,
        void        * stream,
        const IPTR  * sd)

/*  FUNCTION
        Writes one big endian structure to a streamhook.

    INPUTS
        hook - Write to this streamhook
        data - Data to be written
        stream - Stream passed to streamhook
        sd - Description of the structure to be written. The first element
                is the size of the structure.

    RESULT
        The function returns TRUE on success and FALSE otherwise. In error,
        you can examine IoErr() to find out what was wrong.

    NOTES
        This function writes big endian values to a file even on little
        endian machines.

    EXAMPLE
        See ReadStruct().

    BUGS

    INTERNALS
        The function uses the Write*()-functions to write data into
        the file.

        Pointers are written as <valid><data structure>, where valid is
        a byte with the values 1 (then the full data structure follows)
        or 0 (then nothing follows and the pointer will be intialized as
        NULL when the structure is read back).

    SEE ALSO
        ReadByte(), ReadWord(), ReadLong(), ReadFloat(), ReadDouble(),
        ReadString(), ReadStruct(), WriteByte(), WriteWord(), WriteLong(),
        WriteFloat(), WriteDouble(), WriteString(), WriteStruct()

    HISTORY

******************************************************************************/
{
    struct MinList      _list;
    struct WriteLevel * curr;

#   define list     ((struct List *)&_list)

    NEWLIST(list);

    if (!(curr = AllocMem (sizeof (struct WriteLevel), MEMF_ANY)) )
        return FALSE;

    AddTail (list, (struct Node *)curr);

    curr->sd  = sd;
    curr->pos = 1; /* Ignore size */
    curr->s   = data;

#   define DESC     (curr->sd[curr->pos])
#   define IDESC    (curr->sd[curr->pos ++])

    while (DESC != SDT_END)
    {
        switch (IDESC)
        {
        case SDT_UBYTE:      /* Write one  8bit byte */
            if (!WriteByte (hook, *((UBYTE *)(curr->s + IDESC)), stream))
                goto error;

            break;

        case SDT_UWORD:      /* Write one 16bit word */
            if (!WriteWord (hook, *((UWORD *)(curr->s + IDESC)), stream))
                goto error;

            break;

        case SDT_ULONG:      /* Write one 32bit long */
            if (!WriteLong (hook, *((ULONG *)(curr->s + IDESC)), stream))
                goto error;

            break;

        case SDT_FLOAT:      /* Write one 32bit IEEE */
            if (!WriteFloat (hook, *((FLOAT *)(curr->s + IDESC)), stream))
                goto error;

            break;

        case SDT_DOUBLE:     /* Write one 64bit IEEE */
            if (!WriteDouble (hook, *((DOUBLE *)(curr->s + IDESC)), stream))
                goto error;

            break;

        case SDT_STRING: {   /* Write a string */
            STRPTR str;

            str = *((STRPTR *)(curr->s + IDESC));

            if (str)
            {
                if (!WriteByte (hook, 1, stream))
                    goto error;

                if (!WriteString (hook, str, stream))
                    goto error;
            }
            else
            {
                if (!WriteByte (hook, 0, stream))
                    goto error;

                curr->pos ++;
            }

            break; }

        case SDT_STRUCT: {    /* Write a structure */
            struct WriteLevel * next;

            IPTR * desc;
            APTR   ptr;

            ptr  = (APTR)(curr->s + IDESC);
            desc = (IPTR *)IDESC;

            if (!(next = AllocMem (sizeof (struct WriteLevel), MEMF_ANY)) )
                goto error;

            AddTail (list, (struct Node *)next);
            next->sd  = desc;
            next->pos = 1; /* Ignore size */
            next->s   = ptr;

            curr = next;

            break; }

        case SDT_PTR: {       /* Follow a pointer */
            struct WriteLevel * next;

            IPTR * desc;
            APTR   ptr;

            ptr  = *((APTR *)(curr->s + IDESC));
            desc = (IPTR *)IDESC;

            if (ptr)
            {
                if (!WriteByte (hook, 1, stream))
                    goto error;

                if (!(next = AllocMem (sizeof (struct WriteLevel), MEMF_ANY)) )
                    goto error;

                AddTail (list, (struct Node *)next);
                next->sd  = desc;
                next->pos = 1;
                next->s   = ptr;

                curr = next;
            }
            else
            {
                if (!WriteByte (hook, 0, stream))
                    goto error;

                curr->pos ++;
            }

            break; }

        case SDT_IGNORE: {   /* Ignore x bytes */
            ULONG count;
            struct BEIOM_Write wr = {BEIO_WRITE, 0};
    
            count = IDESC;

            while (count --)
            {
                if (CallHookA (hook, stream, &wr) == EOF)
                    goto error;
            }

            break; }

        case SDT_FILL_BYTE:   /* Fill x bytes */
        case SDT_FILL_LONG:   /* Fill x longs */
            /* ignore */
            break;

        case SDT_IFILL_BYTE: { /* Fill x bytes */
            IPTR  count;

            struct BEIOM_Write wr = {BEIO_WRITE, 0};

            count  = IDESC;

            while (count --)
            {
                if (CallHookA (hook, stream, &wr) == EOF)
                    goto error;
            }

            break; }

        case SDT_IFILL_LONG: { /* Fill x longs */
            IPTR  count;
            struct BEIOM_Write wr = {BEIO_WRITE, 0};

            count  = IDESC;

            count <<= 2;

            while (count --)
            {
                if (CallHookA (hook, stream, &wr) == EOF)
                    goto error;
            }

            break; }

        case SDT_SPECIAL: {   /* Call user hook */
            struct Hook * uhook;
            struct SDData data;

            data.sdd_Dest   = ((APTR)(curr->s + IDESC));
            data.sdd_Mode   = SDV_SPECIALMODE_WRITE;
            data.sdd_Stream = stream;

            uhook = (struct Hook *)IDESC;

            if (!CallHookA (uhook, hook, &data))
                goto error;

            break; }

        default:
            goto error;

        } /* switch */

        /* End of the description list ? */
        if (DESC == SDT_END)
        {
            struct WriteLevel * last;

            /* Remove the current level */
            last = curr;
            Remove ((struct Node *)last);

            /* Get the last level */
            if ((curr = (struct WriteLevel *)GetTail (list)))
            {
                FreeMem (last, sizeof (struct WriteLevel));
            }
            else
            {
                curr = last;
            }
        }
    } /* while */

    FreeMem (curr, sizeof (struct WriteLevel));

    return TRUE;

error:

    while ((curr = (struct WriteLevel *)RemTail (list)))
        FreeMem (curr, sizeof (struct WriteLevel));

    return FALSE;
} /* WriteStruct */
