#include <libraries/reqtools.h>
#include <proto/exec.h>
#include <proto/reqtools.h>

#include <stdio.h>
#include <string.h>

#define PROGNAME "rtpalette"

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
    	{TAG_DONE								}
    };
    
    rtPaletteRequestA("Title", NULL, tags);
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);
}
