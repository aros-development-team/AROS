#include <stdio.h>
#include "malloc.h"
#include "hash.h"

void Purify_Init (void)
{
    extern int Purify_Beginrodata;
    extern int Purify_Endrodata;
    extern int Purify_Beginbss;
    extern int Purify_Endbss;
    extern int Purify_Begindata;
    extern int Purify_Enddata;
    extern int * Purify_Beginstaticdata;
    extern int * Purify_Endstaticdata;

    void * ptr;
    int    size;
    MemHash * node;

    printf ("Purify active\n");

    Purify_Filename	= "";
    Purify_Functionname = "_start";
    Purify_Lineno	= 0;

    ptr = (&Purify_Beginrodata)+1;
    size = (long)(&Purify_Endrodata) - (long)(&Purify_Beginrodata) - 4;

    if (size > 0)
    {
	node = Purify_AddMemory (ptr
	    , size
	    , PURIFY_MemFlag_Readable
	    , PURIFY_MemType_Data
	);

	node->data = "rodata";
    }

    ptr = (&Purify_Beginbss)+1;
    size = (long)(&Purify_Endbss) - (long)(&Purify_Beginbss) - 4;

    if (size > 0)
    {
	node = Purify_AddMemory (ptr
	    , size
	    , PURIFY_MemFlag_Readable
		| PURIFY_MemFlag_Writable
	    , PURIFY_MemType_Data
	);

	node->data = "bss";
    }

    ptr = (&Purify_Begindata)+1;
    size = (long)(&Purify_Enddata) - (long)(&Purify_Begindata) - 4;

    if (size > 0)
    {
	node = Purify_AddMemory (ptr
	    , size
	    , PURIFY_MemFlag_Readable
		| PURIFY_MemFlag_Writable
	    , PURIFY_MemType_Data
	);

	node->data = "data";
    }

    size = (long)(Purify_Endstaticdata) - (long)(Purify_Beginstaticdata) - 4;

    if (size > 0)
    {
	node = Purify_AddMemory (Purify_Beginstaticdata+1
	    , size
	    , PURIFY_MemFlag_Readable
		| PURIFY_MemFlag_Writable
	    , PURIFY_MemType_Data
	);

	node->data = "static data";
    }
}

void Purify_Exit (void)
{
    printf ("Purify finished\n");

    Purify_MemoryExit ();
}
