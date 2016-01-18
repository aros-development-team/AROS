/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/iffparse.h>
#include <dos/dos.h>
#include <libraries/iffparse.h>
#include <datatypes/pictureclass.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct Library *IFFParseBase;

static struct IFFHandle *iff;
static LONG err;

int main(void)
{
    IFFParseBase = OpenLibrary("iffparse.library",0);
    if (!IFFParseBase)
    {
        printf("Could not open iffparse.library!");
	exit(0);
    }
    
    printf("Trying to write a test file called \"ram:test.iff\" ...\n\n");
    
    if ((iff = AllocIFF()))
    {
        if ((iff->iff_Stream = (IPTR) Open("ram:test.iff", MODE_NEWFILE)))
	{
	    InitIFFasDOS(iff);
	    
	    err = OpenIFF(iff, IFFF_WRITE);
	    if (err == 0)
	    {
	        err = PushChunk(iff, ID_ILBM, ID_FORM, IFFSIZE_UNKNOWN);
		if (err == 0)
		{
		    err = PushChunk(iff, ID_ILBM, MAKE_ID('A', 'B', 'C', 'D'), IFFSIZE_UNKNOWN);
		    if (err == 0)
		    {
		        char *buffer = "1234567890";
			
			err = WriteChunkBytes(iff, buffer, strlen(buffer));
			if (err == strlen(buffer))
			{
			    printf("********* No errors during writing :-) **************\n");
						    
			} else printf("WriteChunkBytes(iff, buffer, strlen(buffer)) returned %ld\n",(long)err);
		        
			PopChunk(iff);
			
		    } else printf("PushChunk(iff, ID_ILBM, MAKE_ID('A', 'B', 'C', 'D'), IFFSIZE_UNKNOWN) returned error %ld\n",(long)err);
		    
		    PopChunk(iff);
		    
		} else printf("PushChunk(iff, ID_ILBM, ID_FORM, IFFSIZE) returned error %ld\n",(long)err);
		
	        CloseIFF(iff);
		
	    } else printf("OpenIFF returned error %ld\n",(long)err);
	    
	    Close((BPTR) iff->iff_Stream);
	    
	} else printf("Could not open ram:test.iff for write!\n");
        
	FreeIFF(iff);
	
    } else printf("AllocIFF failed!\n");
    
    CloseLibrary(IFFParseBase);

    return 0;
}
