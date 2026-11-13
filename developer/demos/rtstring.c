#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define PROGNAME "rtstring"

struct ReqToolsBase *ReqToolsBase;

static char s[300];

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
        {RT_Underscore		, (IPTR)'_'						},
	{RTGS_TextFmt		, (IPTR)"AROS - The AROS Research OS\n\nLine2\nLine3"	},
    	{RTGS_GadFmt		, (IPTR)"O_k|Hel_lo|_Something"				},
    	{TAG_DONE									}
    };
    
    rtGetStringA(s, 100, "Title", NULL, tags);
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);
    return 0;
}
