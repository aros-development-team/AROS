/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Read a big endian structure from a file
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <string.h>
#include <exec/memory.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>

struct ReadLevel
{
    struct MinNode   node;
    IPTR	   * sd;
    UBYTE	   * s;
    int 	     pos;
};

/******************************************************************************

    NAME */
#include <stdio.h>
#include <aros/structdesc.h>
#include <clib/alib_protos.h>

	BOOL ReadStruct (

/*  SYNOPSIS */
	BPTR   fh,
	IPTR * sd,
	APTR * dataptr)

/*  FUNCTION
	Reads one big endian structure from a file.

    INPUTS
	fh - Read from this file
	sd - Description of the structure to be read. The first element
		is the size of the structure.
	data - Put the data here

    RESULT
	The function returns TRUE on success. On success, the value
	read is written into dataptr. On failure, FALSE is returned and the
	contents of dataptr are not changed.

    NOTES
	This function reads big endian values from a file even on little
	endian machines.

    EXAMPLE
	See below.

    BUGS

    SEE ALSO
	Open(), Close(), ReadByte(), ReadWord(), ReadLong(),
	ReadFloat(), ReadString(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString(), WriteStruct()

    HISTORY
	14.09.93    ada created

******************************************************************************/
{
    struct MinList     _list;
    struct ReadLevel * curr;

#   define list     ((struct List *)&_list)

    NEWLIST(list);

    if (!(curr = AllocMem (sizeof (struct ReadLevel), MEMF_ANY)) )
	return FALSE;

    AddTail (list, (struct Node *)curr);

    curr->sd  = sd;
    curr->pos = 0;
    curr->s   = NULL;

#   define DESC     curr->sd[curr->pos]
#   define IDESC    curr->sd[curr->pos ++]

    while (DESC != SDT_END)
    {
	if (!curr->pos)
	{
	    if (!(curr->s = AllocMem (IDESC, MEMF_CLEAR)) )
		goto error;
	}

	switch (IDESC)
	{
	case SDT_UBYTE:      /* Read one  8bit byte */
	    if (!ReadByte (fh, curr->s + IDESC))
		goto error;

	    break;

	case SDT_UWORD:      /* Read one 16bit word */
	    if (!ReadWord (fh, (UWORD *)curr->s + IDESC))
		goto error;

	    break;

	case SDT_ULONG:      /* Read one 32bit long */
	    if (!ReadLong (fh, (ULONG *)curr->s + IDESC))
		goto error;

	    break;

	case SDT_FLOAT:      /* Read one 32bit IEEE */
	    if (!ReadFloat (fh, (FLOAT *)curr->s + IDESC))
		goto error;

	    break;

	case SDT_DOUBLE:     /* Read one 64bit IEEE */
	    if (!ReadDouble (fh, (DOUBLE *)curr->s + IDESC))
		goto error;

	    break;

	case SDT_STRING: {   /* Read a string */
	    UBYTE valid_ptr;

	    if (!ReadByte (fh, &valid_ptr))
		goto error;

	    if (valid_ptr)
	    {
		if (!ReadString (fh, (STRPTR *)curr->s + IDESC))
		    goto error;
	    }
	    else
	    {
		*((APTR *)curr->s + IDESC) = NULL;
	    }

	    break; }

	case SDT_STRUCT: {    /* Read a structure */
	    struct ReadLevel * next;
	    UBYTE valid_ptr;

	    if (!ReadByte (fh, &valid_ptr))
		goto error;

	    if (valid_ptr)
	    {
		if (!(next = AllocMem (sizeof (struct ReadLevel), MEMF_ANY)) )
		    goto error;

		AddTail (list, (struct Node *)next);
		next->sd  = (IPTR *)IDESC;
		next->pos = 0;

		curr = next;
	    }
	    else
	    {
		*((APTR *)curr->s + IDESC) = NULL;
	    }

	    break; }

	case SDT_IGNORE:     /* Ignore x bytes */
	    if (Seek (fh, IDESC, OFFSET_CURRENT) == EOF)
		goto error;

	    break;

	case SDT_FILL_BYTE: { /* Fill x bytes */
	    IPTR  offset;
	    UBYTE value;
	    IPTR  count;

	    offset = IDESC;
	    value  = IDESC;
	    count  = IDESC;

	    memset (curr->s + offset, value, count);

	    break; }

	case SDT_FILL_LONG: { /* Fill x longs */
	    ULONG * ulptr;
	    ULONG   value;
	    IPTR    count;

	    ulptr = (ULONG *)(curr->s + IDESC);
	    value = IDESC;
	    count = IDESC;

	    while (count --)
		*ulptr ++ = value;

	    break; }

	case SDT_IFILL_BYTE: { /* Fill x bytes */
	    IPTR  offset;
	    UBYTE value;
	    IPTR  count;

	    offset = IDESC;
	    value  = IDESC;
	    count  = IDESC;

	    if (Seek (fh, count, OFFSET_CURRENT) == EOF)
		goto error;

	    memset (curr->s + offset, value, count);

	    break; }

	case SDT_IFILL_LONG: { /* Fill x longs */
	    ULONG * ulptr;
	    ULONG   value;
	    IPTR    count;

	    ulptr = (ULONG *)(curr->s + IDESC);
	    value = IDESC;
	    count = IDESC;

	    if (Seek (fh, count<<2, OFFSET_CURRENT) == EOF)
		goto error;

	    while (count --)
		*ulptr ++ = value;

	    break; }

	default:
	    goto error;

	} /* switch */

	/* End of the description list ? */
	if (DESC == SDT_END)
	{
	    struct ReadLevel * last;

	    /* Remove the current level */
	    last = curr;
	    Remove ((struct Node *)last);

	    /* Get the last level */
	    if ((curr = GetTail (list)))
	    {
		/*
		    Now put the result of the current level in the
		    struct of the previous level.
		*/
		*((APTR *)(last->s + IDESC)) = last->s;

		FreeMem (last, sizeof (struct ReadLevel));
	    }
	    else
	    {
		curr = last;
	    }
	}
    } /* while */

    *dataptr = curr->s;

    FreeMem (curr, sizeof (struct ReadLevel));

    return TRUE;

error:
    curr = GetHead (list);

    if (curr && curr->s)
	FreeStruct (curr->s, curr->sd);

    while ((curr = (struct ReadLevel *)RemTail (list)))
	FreeMem (curr, sizeof (struct ReadLevel));

    return FALSE;
} /* ReadStruct */

