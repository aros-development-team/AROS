
#include <proto/exec.h>
#include <proto/intuition.h>

struct IClass 		*dt_class;

struct ExecBase 	*SysBase;
struct IntuitionBase 	*IntuitionBase;
struct GfxBase	 	*GfxBase;
#ifdef _AROS
struct UtilityBase	*UtilityBase;
#else
struct Library		*UtilityBase;
#endif
struct DosLibrary 	*DOSBase;
struct Library 		*DataTypesBase;
struct Library 		*IFFParseBase;
struct Library		*TextBase;

/* inside asciiclass.c */
struct IClass *DT_MakeClass(void);

/**************************************************************************************************/

int __UserLibInit(struct Library *libbase )
{
#ifndef _AROS
    SysBase = *(struct ExecBase**)4;
#endif

    if ((TextBase = OpenLibrary("datatypes/text.datatype", 0)))
    {
	if((GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 39)))
	{
	    if((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 39)))
	    {
		if((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 39)))
		{
#ifdef _AROS
		    if((UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37)))
#else
		    if((UtilityBase = (struct Library *)OpenLibrary("utility.library", 37)))
#endif
		    {
			if((DataTypesBase = OpenLibrary("datatypes.library", 37)))
			{
			    if((IFFParseBase = OpenLibrary("iffparse.library", 37)))
			    {
				if((dt_class = DT_MakeClass()))
				{
				    AddClass(dt_class);

				    return 0;
				}
			    }
			}
		    }
		}
	    }
	}
    }
    
    return -1;
}

/**************************************************************************************************/

void __UserLibCleanup(struct Library *libbase )
{
    if(dt_class)
    {
	RemoveClass(dt_class);
	FreeClass(dt_class);
	dt_class = NULL;
    }

kprintf("ascii.datatype: __UserLibCleanup 1\n");
    
    if(TextBase) CloseLibrary(TextBase);
kprintf("ascii.datatype: __UserLibCleanup 2\n");
    if(IFFParseBase) CloseLibrary(IFFParseBase);
kprintf("ascii.datatype: __UserLibCleanup 3\n");
    if(DataTypesBase) CloseLibrary(DataTypesBase);
kprintf("ascii.datatype: __UserLibCleanup 4\n");
    if(UtilityBase) CloseLibrary((struct Library *)UtilityBase);
    if(DOSBase) CloseLibrary((struct Library *)DOSBase);
    if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
    if(GfxBase) CloseLibrary((struct Library *)GfxBase);
kprintf("ascii.datatype: __UserLibCleanup done\n");
}

/**************************************************************************************************/

struct IClass *ObtainEngine(void)
{
    return dt_class;
}

/**************************************************************************************************/
