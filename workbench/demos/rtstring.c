#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>

#define PROGNAME "rtstring"

struct Library *RTBase;

static char s[300];

static void cleanup(char *msg)
{
    if (msg) printf(PROGNAME ": %s\n", msg);
    
    if (RTBase) CloseLibrary(RTBase);
    
    exit(0);
}

static void openlibs(void)
{
    RTBase = OpenLibrary("reqtools.library", 0);
    if (!RTBase) cleanup("Can't open reqtools.library");
}

static void action(void)
{
    struct TagItem tags[] =
    {
        {RT_Underscore		, (IPTR)'_'					},
	{RTGS_TextFmt		, "AROS - The Amiga Research OS\n\nLine2\nLine3"},
    	{RTGS_GadFmt		, (IPTR)"O_k|Hel_lo|_Something"			},
    	{TAG_DONE								}
    };
    
    rtGetStringA(s, 100, "Title", NULL, tags);
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);
}
