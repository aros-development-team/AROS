/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc: Write a big endian structure to a file
    Lang: english
*/
#define AROS_ALMOST_COMPATIBLE
#include <string.h>
#include <exec/memory.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <aros/debug.h>

struct WriteLevel
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

	BOOL WriteStruct (

/*  SYNOPSIS */
	BPTR   fh,
	IPTR * sd,
	APTR   data)

/*  FUNCTION
	Writes one big endian structure to a file.

    INPUTS
	fh - Write to this file
	sd - Description of the structure to be written. The first element
		is the size of the structure.
	data - Address of the structure

    RESULT
	The function returns TRUE on success and FALSE otherwise. In error,
	you can examine IoErr() to find out what was wrong.

    NOTES
	This function writes big endian values to a file even on little
	endian machines.

    EXAMPLE
	See below.

    BUGS

    INTERNALS
	The function uses the Write*()-functions to write data into
	the file.

	Pointers are written as <valid><data structure>, where valid is
	a byte with the values 1 (then the full data structure follows)
	or 0 (then nothing follows and the pointer will be intialized as
	NULL when the structure is read back).

    SEE ALSO
	Open(), Close(), ReadByte(), ReadWord(), ReadLong(), ReadFloat(),
	ReadString(), ReadStruct(), WriteByte(), WriteWord(), WriteLong(),
	WriteFloat(), WriteDouble(), WriteString()

    HISTORY
	28.11.96 digulla created

******************************************************************************/
{
    struct MinList	_list;
    struct WriteLevel * curr;

#   define list     ((struct List *)&_list)

    NEWLIST(list);

    if (!(curr = AllocMem (sizeof (struct WriteLevel), MEMF_ANY)) )
	return FALSE;

    AddTail (list, (struct Node *)curr);

    curr->sd  = sd;
    curr->pos = 0;
    curr->s   = data;

#   define DESC     (curr->sd[curr->pos])
#   define IDESC    (curr->sd[curr->pos ++])

    while (DESC != SDT_END)
    {
	switch (IDESC)
	{
	case SDT_UBYTE:      /* Write one  8bit byte */
	    if (!WriteByte (fh, *((UBYTE *)(curr->s + IDESC))))
		goto error;

	    break;

	case SDT_UWORD:      /* Write one 16bit word */
	    if (!WriteWord (fh, *((UWORD *)(curr->s + IDESC))))
		goto error;

	    break;

	case SDT_ULONG:      /* Write one 32bit long */
	    if (!WriteLong (fh, *((ULONG *)(curr->s + IDESC))))
		goto error;

	    break;

	case SDT_FLOAT:      /* Write one 32bit IEEE */
	    if (!WriteFloat (fh, *((FLOAT *)(curr->s + IDESC))))
		goto error;

	    break;

	case SDT_DOUBLE:     /* Write one 64bit IEEE */
	    if (!WriteDouble (fh, *((DOUBLE *)(curr->s + IDESC))))
		goto error;

	    break;

	case SDT_STRING: {   /* Write a string */
	    STRPTR str;

	    str = *((STRPTR *)(curr->s + IDESC));

	    if (str)
	    {
		if (!WriteByte (fh, 1))
		    goto error;

		if (!WriteString (fh, str))
		    goto error;
	    }
	    else
	    {
		if (!WriteByte (fh, 0))
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
	    next->pos = 0;
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
		if (!WriteByte (fh, 1))
		    goto error;

		if (!(next = AllocMem (sizeof (struct WriteLevel), MEMF_ANY)) )
		    goto error;

		AddTail (list, (struct Node *)next);
		next->sd  = desc;
		next->pos = 0;
		next->s   = ptr;

		curr = next;
	    }
	    else
	    {
		if (!WriteByte (fh, 0))
		    goto error;

		curr->pos ++;
	    }

	    break; }

	case SDT_IGNORE: {   /* Ignore x bytes */
	    ULONG count;

	    count = IDESC;

	    while (count --)
	    {
		if (FPutC (fh, 0) == EOF)
		    goto error;
	    }

	    break; }

	case SDT_FILL_BYTE:   /* Fill x bytes */
	case SDT_FILL_LONG:   /* Fill x longs */
	    /* ignore */
	    break;

	case SDT_IFILL_BYTE: { /* Fill x bytes */
	    IPTR  offset;
	    UBYTE value;
	    IPTR  count;

	    offset = IDESC;
	    value  = IDESC;
	    count  = IDESC;

	    while (count --)
	    {
		if (FPutC (fh, 0) == EOF)
		    goto error;
	    }

	    break; }

	case SDT_IFILL_LONG: { /* Fill x longs */
	    IPTR  offset;
	    UBYTE value;
	    IPTR  count;

	    offset = IDESC;
	    value  = IDESC;
	    count  = IDESC;

	    while (count --)
	    {
		if (!WriteLong (fh, 0L))
		    goto error;
	    }

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
	    if ((curr = GetTail (list)))
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

#ifdef TEST
#include <stdio.h>
#include <dos/dos.h>
#include <aros/structdesc.h>
#include <clib/alib_protos.h>

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

IPTR ByteDesc[] = { SDM_UBYTE(0), SDM_END };
IPTR WordDesc[] = { SDM_UWORD(0), SDM_END };
IPTR LongDesc[] = { SDM_ULONG(0), SDM_END };
IPTR FloatDesc[] = { SDM_FLOAT(0), SDM_END };
IPTR DoubleDesc[] = { SDM_DOUBLE(0), SDM_END };
IPTR StringDesc[] = { SDM_STRING(0), SDM_END };

#define O(x)        offsetof(struct Level1,x)
IPTR Level1Desc[] =
{
    SDM_UBYTE(O(l1_Byte)),
    SDM_ULONG(O(l1_Long)),
    SDM_END
};

#undef O
#define O(x)        offsetof(struct MainLevel,x)
IPTR MainDesc[] =
{
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

    if (!WriteStruct (fh, MainDesc, &demo))
    {
	PrintFault (IoErr(), "Failed to write to file\n");
    }

    if (!Close (fh))
    {
	PrintFault (IoErr(), "Failed to close file\n");
    }

    return 0;
} /* main */

#endif /* TEST */
