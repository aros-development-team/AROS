
#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROGNAME "rtlong"

struct ReqToolsBase *ReqToolsBase;

static void cleanup(char *msg)
{
    if (msg) printf(PROGNAME ": %s\n", msg);
    
    if (ReqToolsBase) CloseLibrary((struct Library *)ReqToolsBase);
    
    exit(0);
}

static void openlibs(void)
{
    ReqToolsBase = (struct ReqToolsBase *)OpenLibrary("reqtools.library", 0);
    if (!ReqToolsBase) cleanup("Can't open reqtools.library");
}

static void action(void)
{
    struct TagItem tags[] =
    {
        {RT_Underscore		, (IPTR)'_'					},
	{RTGL_Min		, -100						},
	{RTGL_Max		, 100						},
	{RTGL_Flags		, GLREQF_CENTERTEXT				},
	{RTGS_TextFmt		, (IPTR)"Enter something\n12345678\nABCDEF"	},
    	{RTGS_GadFmt		, (IPTR)"O_k|Hel_lo|_Something"			},
    	{TAG_DONE								}
    };
    LONG val = 0;
       
    rtGetLongA((ULONG *)&val, "Title", NULL, tags);
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);

    return 0;
}
