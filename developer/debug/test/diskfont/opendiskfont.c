/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <diskfont/diskfont.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <proto/utility.h>
#include <proto/graphics.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct Library *DiskfontBase;
struct UtilityBase *UtilityBase;

UBYTE *buf;

void cleanup(char *msg)
{
    if (msg) printf("aftest: %s\n", msg);
    
    if (buf) FreeVec(buf);
    
    if (UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if (DiskfontBase) CloseLibrary(DiskfontBase);
    
    exit(0);
}

void openlibs(void)
{
    DiskfontBase = OpenLibrary("diskfont.library", 0);
    if (!DiskfontBase) cleanup("Cant open diskfont.library!");
    UtilityBase = (struct UtilityBase *) OpenLibrary("utility.library", 0);
    if (!UtilityBase) cleanup("Cant open utility.library!");
}

void action(void)
{
    struct TextFont *font;
    struct TextAttr ta;

    ta.ta_Name = "Vera Sans Bold Italic.font";
    ta.ta_Name = "xhelvetica.font";
    ta.ta_YSize = 11;
    ta.ta_Style = 0;
    ta.ta_Flags = 0;

    font = OpenDiskFont(&ta);
    if (font)
    {
	CloseFont(font);
    }
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);

    return 0; /* keep compiler happy */
}
