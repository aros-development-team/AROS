/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <diskfont/diskfont.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/diskfont.h>
#include <proto/utility.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct Library *DiskfontBase;
struct UtilityBase *UtilityBase;

UBYTE *buf;

void cleanup(char *msg)
{
    if (msg) printf("aftest: %s\n", msg);
    
    FreeVec(buf);
    
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
    struct AvailFontsHeader *afh;
    struct TAvailFonts *af;
    
    ULONG bufsize = 10000, shortage;
    ULONG numentries;
    ULONG i;
    
    buf = AllocVec(bufsize, MEMF_ANY);
    if (!buf) cleanup("out of memory!");
    
    do
    {
    	shortage = AvailFonts(buf, bufsize, AFF_MEMORY | AFF_DISK | AFF_BITMAP | AFF_TAGGED);
	if (shortage)
	{
	    bufsize += shortage;
	    
    	    FreeVec(buf);
	    buf = AllocVec(bufsize, MEMF_ANY);
	    if (!buf) cleanup("out of memory!");
	}
    } while (shortage);
    
    afh = (struct AvailFontsHeader *)buf;
    numentries = afh->afh_NumEntries;
    
    printf("numentries = %ld\n", (long)numentries);
    
    af = (struct TAvailFonts *)(buf + 2);
    
    for(i = 0; i < numentries;i++)
    {
#if 1
    	printf("%s/%d [%d] flags = %x style = %x\n",
		af->taf_Attr.tta_Name,
		af->taf_Attr.tta_YSize,
		af->taf_Type,
		af->taf_Attr.tta_Flags,
		af->taf_Attr.tta_Style);
	
	printf("  tags = %p  istagged = %d\n", af->taf_Attr.tta_Tags, (af->taf_Attr.tta_Style & FSF_TAGGED));
		
{
	if ((af->taf_Attr.tta_Style & FSF_TAGGED) && (af->taf_Attr.tta_Tags))
	{
	    struct TagItem *tag, *tstate = af->taf_Attr.tta_Tags;
	   
	    printf("tags = %p\n",  af->taf_Attr.tta_Tags);
	    //Delay(1*50);
	    
	    while((tag = NextTagItem(&tstate)))
	    {
	    	printf(" {%lx,%lx}\n", tag->ti_Tag, tag->ti_Data);
	    }
	}
}
#else
    	printf("#%ld: %s/%d [%d] flags = %x style = %x\n",
	    	i,
		af->taf_Attr.tta_Name,
		af->taf_Attr.tta_YSize,
		af->taf_Type,
		af->taf_Attr.tta_Flags,
		af->taf_Attr.tta_Style);
#endif
		
//	Delay(10);
	af++;
    }
}

int main(void)
{
    openlibs();
    action();
    cleanup(0);

    return 0; /* keep compiler happy */
}
