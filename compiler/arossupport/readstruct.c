/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Read a big endian structure from a streamhook
    Lang: english
*/

#include <string.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/debug.h>
#include <utility/hooks.h>

struct ReadLevel
{
    struct MinNode   node;
    const IPTR	   * sd;
    UBYTE	   * s;
    int 	     pos;
};

/******************************************************************************

    NAME */
#include <stdio.h>
#include <aros/bigendianio.h>
#include <proto/alib.h>

	BOOL ReadStruct (

/*  SYNOPSIS */
	struct Hook * hook,
	APTR	    * dataptr,
	void	    * stream,
	const IPTR  * sd)

/*  FUNCTION
	Reads one big endian structure from a streamhook.

    INPUTS
	hook - Streamhook
	dataptr - Put the data here
	stream - Read from this stream
	sd - Description of the structure to be read. The first element
		is the size of the structure.

    RESULT
	The function returns TRUE on success. On success, the value
	read is written into dataptr. On failure, FALSE is returned and the
	contents of dataptr are not changed.

    NOTES
	This function reads big endian values from a streamhook even on
	little endian machines.

    EXAMPLE
	See below.

    BUGS

    SEE ALSO
	ReadByte(), ReadWord(), ReadLong(), ReadFloat(), ReadDouble(),
	ReadString(), ReadStruct(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString(), WriteStruct()

    HISTORY

******************************************************************************/
{
    AROS_GET_SYSBASE_OK
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

    for (;;)
    {
	if (!curr->pos)
	{
	    if (!(curr->s = AllocMem (IDESC, MEMF_CLEAR)) )
		goto error;
	}

	if (DESC == SDT_END)
	    break;

	switch (IDESC)
	{
	case SDT_UBYTE:      /* Read one  8bit byte */
	    if (!ReadByte (hook, (UBYTE *)(curr->s + IDESC), stream))
		goto error;

	    break;

	case SDT_UWORD:      /* Read one 16bit word */
	    if (!ReadWord (hook, (UWORD *)(curr->s + IDESC), stream))
		goto error;

	    break;

	case SDT_ULONG:      /* Read one 32bit long */
	    if (!ReadLong (hook, (ULONG *)(curr->s + IDESC), stream))
		goto error;

	    break;

	case SDT_FLOAT:      /* Read one 32bit IEEE */
	    if (!ReadFloat (hook, (FLOAT *)(curr->s + IDESC), stream))
		goto error;

	    break;

	case SDT_DOUBLE:     /* Read one 64bit IEEE */
	    if (!ReadDouble (hook, (DOUBLE *)(curr->s + IDESC), stream))
		goto error;

	    break;

	case SDT_STRING: {   /* Read a string */
	    UBYTE    valid_ptr;
	    STRPTR * sptr;

	    sptr = (STRPTR *)(curr->s + IDESC);

	    if (!ReadByte (hook, &valid_ptr, stream))
		goto error;

	    if (valid_ptr)
	    {
		if (!ReadString (hook, sptr, stream))
		    goto error;
	    }
	    else
	    {
		*sptr = NULL;
	    }

	    break; }

	case SDT_STRUCT: {    /* Read a structure */
	    struct ReadLevel * next;
	    IPTR * desc;
	    APTR   aptr;

	    aptr = (APTR)(curr->s + IDESC);
	    desc = (IPTR *)IDESC;

	    curr->pos -= 3; /* Go back to type */

	    if (!(next = AllocMem (sizeof (struct ReadLevel), MEMF_ANY)) )
		goto error;

	    AddTail (list, (struct Node *)next);
	    next->sd  = desc;
	    next->pos = 1;
	    next->s   = aptr;

	    curr = next;

	    break; }

	case SDT_PTR: {    /* Follow a pointer */
	    struct ReadLevel * next;

	    UBYTE  valid_ptr;
	    IPTR * desc;
	    APTR * aptr;

	    aptr = ((APTR *)(curr->s + IDESC));
	    desc = (IPTR *)IDESC;

	    if (!ReadByte (hook, &valid_ptr, stream))
		goto error;

	    if (valid_ptr)
	    {
		curr->pos -= 3;

		if (!(next = AllocMem (sizeof (struct ReadLevel), MEMF_ANY)) )
		    goto error;

		AddTail (list, (struct Node *)next);
		next->sd  = desc;
		next->pos = 0;

		curr = next;
	    }
	    else
	    {
		*aptr = NULL;
	    }

	    break; }

	case SDT_IGNORE:     /* Ignore x bytes */
	    if (CallHook (hook, stream, BEIO_IGNORE, IDESC) == EOF)
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

	    if (CallHook (hook, stream, BEIO_IGNORE, count) == EOF)
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

	    if (CallHook (hook, stream, BEIO_IGNORE, count<<2) == EOF)
		goto error;

	    while (count --)
		*ulptr ++ = value;

	    break; }

	case SDT_SPECIAL: {   /* Call user hook */
	    struct Hook * uhook;
	    struct SDData data;

	    data.sdd_Dest   = ((APTR)(curr->s + IDESC));
	    data.sdd_Mode   = SDV_SPECIALMODE_READ;
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
	    struct ReadLevel * last;

	    /* Remove the current level */
	    last = curr;
	    Remove ((struct Node *)last);

	    /* Get the last level */
	    if ((curr = (struct ReadLevel *)GetTail (list)))
	    {
		switch (IDESC)
		{
		case SDT_STRUCT:
		    curr->pos += 2; /* Skip 2 parameters */
		    break;

		case SDT_PTR: {
		    APTR * aptr;

		    aptr  = ((APTR *)(curr->s + IDESC));
		    curr->pos ++; /* Skip description parameter */

		    /*
			Now put the result of the current level in the
			struct of the previous level.
		    */
		    *aptr = last->s;

		    break; }

		}

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
    curr = (struct ReadLevel *)GetHead (list);

#warning FIXME:
    /* if (curr && curr->s)
	FreeStruct (curr->s, curr->sd); */

    while ((curr = (struct ReadLevel *)RemTail (list)))
	FreeMem (curr, sizeof (struct ReadLevel));

    return FALSE;
} /* ReadStruct */

#ifdef TEST
#include <stdio.h>
#include <dos/dos.h>
#include <aros/structdesc.h>
#include <proto/alib.h>

struct Level1
{
    BYTE l1_Byte;
    LONG l1_Long;
};

struct MainLevel
{
    BYTE   ml_Byte;
    UBYTE  ml_UByte;
    WORD   ml_Word;
    UWORD  ml_UWord;
    LONG   ml_Long;
    ULONG  ml_ULong;
    FLOAT  ml_Float;
    DOUBLE ml_Double;
    STRPTR ml_String;
    struct Level1 ml_Level1;

    BYTE   * ml_BytePtr;
    WORD   * ml_WordPtr;
    LONG   * ml_LongPtr;
    FLOAT  * ml_FloatPtr;
    DOUBLE * ml_DoublePtr;
    STRPTR * ml_StringPtr;
    struct Level1 * ml_Level1Ptr;
};

IPTR ByteDesc[]   = { sizeof(UBYTE),  SDM_UBYTE(0),  SDM_END };
IPTR WordDesc[]   = { sizeof(UWORD),  SDM_UWORD(0),  SDM_END };
IPTR LongDesc[]   = { sizeof(ULONG),  SDM_ULONG(0),  SDM_END };
IPTR FloatDesc[]  = { sizeof(FLOAT),  SDM_FLOAT(0),  SDM_END };
IPTR DoubleDesc[] = { sizeof(DOUBLE), SDM_DOUBLE(0), SDM_END };
IPTR StringDesc[] = { sizeof(STRPTR), SDM_STRING(0), SDM_END };

#define O(x)        offsetof(struct Level1,x)
IPTR Level1Desc[] =
{
    sizeof (struct Level1),
    SDM_UBYTE(O(l1_Byte)),
    SDM_ULONG(O(l1_Long)),
    SDM_END
};

#undef O
#define O(x)        offsetof(struct MainLevel,x)
IPTR MainDesc[] =
{
    sizeof (struct MainLevel),
    SDM_UBYTE(O(ml_Byte)),
    SDM_UBYTE(O(ml_UByte)),
    SDM_UWORD(O(ml_Word)),
    SDM_UWORD(O(ml_UWord)),
    SDM_ULONG(O(ml_Long)),
    SDM_ULONG(O(ml_ULong)),
    SDM_FLOAT(O(ml_Float)),
    SDM_DOUBLE(O(ml_Double)),
    SDM_STRING(O(ml_String)),
    SDM_STRUCT(O(ml_Level1),Level1Desc),

    SDM_PTR(O(ml_BytePtr),ByteDesc),
    SDM_PTR(O(ml_WordPtr),WordDesc),
    SDM_PTR(O(ml_LongPtr),LongDesc),
    SDM_PTR(O(ml_FloatPtr),FloatDesc),
    SDM_PTR(O(ml_DoublePtr),DoubleDesc),
    SDM_PTR(O(ml_StringPtr),StringDesc),
    SDM_PTR(O(ml_Level1Ptr),Level1Desc),

    SDM_END
};

LONG dosstreamhook (struct Hook * hook, BPTR fh, ULONG * msg);

struct Hook dsh =
{
    { NULL, NULL }, HookEntry, (void *)dosstreamhook, NULL
};

LONG dosstreamhook (struct Hook * hook, BPTR fh, ULONG * msg)
{
    LONG rc;

    switch (*msg)
    {
    case BEIO_READ:
	rc = FGetC (fh);
	break;

    case BEIO_WRITE:
	rc = FPutC (fh, ((struct BEIOM_Write *)msg)->Data);
	break;

    case BEIO_IGNORE:
	Flush (fh);

	rc = Seek (fh, ((struct BEIOM_Ignore *)msg)->Count, OFFSET_CURRENT);
	break;

    }

    return rc;
} /* dosstreamhook */

int main (int argc, char ** argv)
{
    struct MainLevel demo =
    {
	(BYTE)0x88,       0xFF,
	(WORD)0x8844,     0xFF77,
	(LONG)0x88442211, 0xFF773311,
	1.5, 1.75,
	"Hallo",
	{ (BYTE)0x88, (LONG)0x88442211 },
	/* ... */
    };
    BYTE b = (BYTE)0x88;
    WORD w = (WORD)0x8844;
    LONG l = (LONG)0x88442211;
    FLOAT f = 1.5;
    DOUBLE d = 1.75;
    STRPTR s = "Hallo";
    struct Level1 l1 =
    {
	(BYTE)0x88, (LONG)0x88442211
    };
    BPTR fh;
    struct MainLevel * readback;

    demo.ml_BytePtr = &b;
    demo.ml_WordPtr = &w;
    demo.ml_LongPtr = &l;
    demo.ml_FloatPtr = &f;
    demo.ml_DoublePtr = &d;
    demo.ml_StringPtr = &s;
    demo.ml_Level1Ptr = &l1;

    fh = Open ("writestruct.dat", MODE_NEWFILE);

    if (!fh)
    {
	PrintFault (IoErr(), "Can't open file\n");
	return 10;
    }

    /*
	This writes the following data stream:

	    0000 88			    ml_Byte
	    0001 ff			    ml_Ubyte
	    0002 88 44			    ml_Word
	    0004 ff 77			    ml_UWord
	    0006 88 44 22 11		    ml_Long
	    000a ff 77 33 11		    ml_ULong
	    000e 3f c0 00 00		    ml_Float
	    0012 3f fc 00 00 00 00 00 00    ml_Double
	    001a 01:48 61 6c 6c 6f 00	    ml_String
	    0021 88			    ml_Level1.l1_Byte
	    0022 88 44 22 11		    ml_Level1.l1_Long
	    0026 01:88			    ml_BytePtr
	    0028 01:88 44		    ml_WordPtr
	    002b 01:88 44 22 11 	    ml_LongPtr
	    0030 01:3f c0 00 00 	    ml_FloatPtr
	    0035 01:3f fc 00 00 00 00 00 00 ml_DoublePtr
	    003e 01:01:48 61 6c 6c 6f 00    ml_StringPtr - Note two 01 !
	    0046 01:88 88 44 22 11	    ml_Level1Ptr
    */

    if (!WriteStruct (&dsh, &demo, fh, MainDesc))
    {
	PrintFault (IoErr(), "Failed to write to file\n");
    }

    if (!Close (fh))
    {
	PrintFault (IoErr(), "Failed to close file\n");
    }

    /* Read the structure back */
    fh = Open ("writestruct.dat", MODE_OLDFILE);

    if (!fh)
    {
	PrintFault (IoErr(), "Can't open file for reading\n");
	return 10;
    }

    if (!ReadStruct (&dsh, (APTR *)&readback, fh, MainDesc))
    {
	PrintFault (IoErr(), "Failed to read from file\n");
    }
    else
    {
	UBYTE * ptr;
	int t;

	ptr = (UBYTE *)readback;
	t = 0;

	kprintf ("readback = %p\n", readback);

	kprintf ("%02X (88) %02X (FF)\n"
	    , (UBYTE)readback->ml_Byte
	    , readback->ml_UByte
	);
	kprintf ("%04X (8844) %04X (FF77)\n"
	    , (UWORD)readback->ml_Word
	    , readback->ml_UWord
	);
	kprintf ("%08lX (88442211) %08lX (FF773311)\n"
	    , readback->ml_Long
	    , readback->ml_ULong
	);
	kprintf ("%08lX (3FC00000) %08lX:%08lX (3FFC0000:00000000)\n"
	    , *(ULONG *)&readback->ml_Float
	    , ((ULONG *)&readback->ml_Double)[1]
	    , ((ULONG *)&readback->ml_Double)[0]
	);
	kprintf ("%s (Hallo)\n"
	    , readback->ml_String
	);
	kprintf ("{ %02X %08X } ({ 88 88442211 })\n"
	    , (UBYTE)readback->ml_Level1.l1_Byte
	    , readback->ml_Level1.l1_Long
	);
	kprintf ("%02X (88)\n"
	    , (UBYTE)*readback->ml_BytePtr
	);
	kprintf ("%04X (8844)\n"
	    , (UWORD)*readback->ml_WordPtr
	);
	kprintf ("%08lX (88442211)\n"
	    , *readback->ml_LongPtr
	);
	kprintf ("%08lX (3FC00000) %08lX:%08lX (3FFC0000:00000000)\n"
	    , *(ULONG *)readback->ml_FloatPtr
	    , ((ULONG *)readback->ml_DoublePtr)[1]
	    , ((ULONG *)readback->ml_DoublePtr)[0]
	);
	kprintf ("%s (Hallo)\n"
	    , *readback->ml_StringPtr
	);
	kprintf ("{ %02X %08X } ({ 88 88442211 })\n"
	    , (UBYTE)readback->ml_Level1Ptr->l1_Byte
	    , readback->ml_Level1Ptr->l1_Long
	);

	FreeStruct (readback, MainDesc);
    }

    if (!Close (fh))
    {
	PrintFault (IoErr(), "Failed to close file after reading\n");
    }

    return 0;
} /* main */

#endif /* TEST */

