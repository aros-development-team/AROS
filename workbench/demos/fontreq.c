

#include <exec/types.h>
#include <exec/libraries.h>
#include <graphics/displayinfo.h>
#include <libraries/asl.h>
#ifndef _AROS
#include <clib/exec_protos.h>
#include <clib/asl_protos.h>
#else
#include <proto/exec.h>
#include <proto/asl.h>
#endif
#include <stdio.h>

#ifdef LATTICE
int CXBRK(void)     { return(0); }  /* Disable Lattice CTRL/C handling */
void chkabort(void) { return; }     /* really */
#endif

#define MYLEFTEDGE 0
#define MYTOPEDGE  0
#define MYWIDTH    320
#define MYHEIGHT   400

struct Library *AslBase = NULL;

struct TextAttr mytextattr = 
{
    "arial.font", 13, 0, 0
};

struct TagItem fotags1[] =
{
    {ASLFO_TextAttr, (IPTR)&mytextattr},
    {ASLFO_TitleText	, (IPTR)"ASL Font requester"   },
    {TAG_DONE	    	    	    	    	       }
};

static void showrequester(char *msg, struct TagItem *tags)
{
    struct FontRequester *freq;

    printf("\n%s:\n",msg ? msg : "");
    
    if ((freq = (struct FontRequester *)AllocAslRequest(ASL_FontRequest, tags)))
    {
	if (AslRequest(freq, NULL))
	{
	    printf("\n-------------------------------------------------------\n\n");
	    printf("FontName = %s\n", freq->fo_TAttr.tta_Name);
	    printf("FontSize = %d\n", freq->fo_TAttr.tta_YSize);
	    
	} else printf("\nRequester was aborted\n");
	FreeAslRequest(freq);
    }
    else printf("Could not alloc FontRequester\n");
}

int main(int argc, char **argv)
{    
    if ((AslBase = OpenLibrary("asl.library", 37L)))
    {
	showrequester("Default requester with no tags", NULL);
	showrequester(NULL, fotags1);

	CloseLibrary(AslBase);
    } else {
        puts("Could not open asl.library!\n");
    }
    return 0;
}
