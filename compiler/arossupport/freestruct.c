/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Free a structure returned by ReadStruct()
    Lang: english
*/

#include <string.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <utility/hooks.h>

struct FreeLevel
{
    struct MinNode   node;
    const IPTR	   * sd;
    UBYTE	   * s;
    ULONG	     size;
    int 	     pos;
};

/******************************************************************************

    NAME */
#include <stdio.h>
#include <aros/bigendianio.h>
#include <proto/alib.h>

	void FreeStruct (

/*  SYNOPSIS */
	APTR	     data,
	const IPTR * sd)

/*  FUNCTION
	Free a structure which was created by ReadStruct().

    INPUTS
	data - This was returned by ReadStruct() in the dataptr parameter.
		Must be non-NULL.
	sd - Description of the structure to be read. The first element
		is the size of the structure.

    RESULT
	None.

    NOTES

    EXAMPLE
	See ReadStruct()

    BUGS

    SEE ALSO
	exec.library/Open(), exec.library/Close(), ReadByte(), ReadWord(),
	ReadLong(), ReadFloat(),
	ReadString(), ReadStruct(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString(), WriteStruct()

******************************************************************************/
{
    struct MinList     _list;
    struct FreeLevel * curr;

#   define list     ((struct List *)&_list)

    NEWLIST(list);

    if (!(curr = AllocMem (sizeof (struct FreeLevel), MEMF_ANY)) )
	return;

    AddTail (list, (struct Node *)curr);

    curr->sd  = sd;
    curr->pos = 0;
    curr->s   = data;

#   define DESC     curr->sd[curr->pos]
#   define IDESC    curr->sd[curr->pos ++]

    for (;;)
    {
	if (!curr->pos)
	{
	    curr->size = IDESC;
	}

	if (DESC == SDT_END)
	    break;

	switch (IDESC)
	{
	case SDT_UBYTE:      /* Read one  8bit byte */
	case SDT_UWORD:      /* Read one 16bit word */
	case SDT_ULONG:      /* Read one 32bit long */
	case SDT_FLOAT:      /* Read one 32bit IEEE */
	case SDT_DOUBLE:     /* Read one 64bit IEEE */
	case SDT_IGNORE:     /* Ignore x bytes */
	    /* Ignore these */
	    curr->pos ++;
	    break;

	case SDT_STRING: {   /* Read a string */
	    STRPTR sptr;

	    sptr = *(STRPTR *)(curr->s + IDESC);

	FreeVec (sptr);

	    break; }

	case SDT_STRUCT: {    /* Read a structure */
	    /* Ignore two parameters */
	    curr->pos += 2;

	    break; }

	case SDT_PTR: {    /* Follow a pointer */
	    struct FreeLevel * next;

	    IPTR * desc;
	    APTR * aptr;

	    aptr = ((APTR *)(curr->s + IDESC));
	    desc = (IPTR *)IDESC;

	    if (*aptr)
	    {
		if (!(next = AllocMem (sizeof (struct FreeLevel), MEMF_ANY)) )
		    goto error;

		AddTail (list, (struct Node *)next);
		next->sd  = desc;
		next->pos = 0;
		next->s   = *aptr;

		curr = next;
	    }

	    break; }

	case SDT_FILL_BYTE:   /* Fill x bytes */
	case SDT_FILL_LONG:   /* Fill x longs */
	case SDT_IFILL_BYTE:  /* Fill x bytes */
	case SDT_IFILL_LONG:  /* Fill x longs */
	    curr->pos += 3; /* Ignore three parameters */
	    break;

	case SDT_SPECIAL: {   /* Call user hook */
	    struct Hook * hook;
	    struct SDData data;

	    data.sdd_Dest = ((APTR)(curr->s + IDESC));
	    data.sdd_Mode = SDV_SPECIALMODE_FREE;

	    hook = (struct Hook *)IDESC;

	    CallHookA (hook, NULL, &data);

	    break; }

	} /* switch */

	/* End of the description list ? */
	if (DESC == SDT_END)
	{
	    struct FreeLevel * last;

	    /* Remove the current level */
	    last = curr;
	    Remove ((struct Node *)last);

	    /* Get the last level */
	    if ((curr = (struct FreeLevel *)GetTail (list)))
	    {
		FreeMem (last->s, last->size);
		FreeMem (last, sizeof (struct FreeLevel));
	    }
	    else
	    {
		curr = last;
	    }
	}
    } /* while */

    FreeMem (curr->s, curr->size);
    FreeMem (curr, sizeof (struct FreeLevel));

    return;

error:
    while ((curr = (struct FreeLevel *)RemTail (list)))
	FreeMem (curr, sizeof (struct FreeLevel));
} /* FreeStruct */

