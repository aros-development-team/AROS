
/*
**
**	library code gradientslider
**	AMIGA version
**
*/

#include <exec/resident.h>
#include <exec/initializers.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <graphics/gfxbase.h>

#ifdef __GNUC__
#include <inline/exec.h>
#include <inline/intuition.h>
#include <inline/dos.h>
#else
#include <pragma/exec_lib.h>
#include <pragma/intuition_lib.h>
#include <pragma/dos_lib.h>

#define REG( x, a )	register __## x a
#define __regargs
#endif

#ifdef __GNUC__
#include "BoopsiStubs.h"
#endif

#define GradientSliderBase	GradientSliderBase_intern

#include "gradientslider_intern.h"

/****************************************************************************/

#ifndef BPTR
#define BPTR		ULONG
#endif

/****************************************************************************/

STATIC struct Library *LibInit(REG(a0, ULONG seglist), REG(d0, struct GradientSliderBase_intern *ClassBase), REG(a6, struct Library *) );
STATIC struct Library *LibOpen( REG(a6, struct GradientSliderBase_intern *ClassBase ) );
STATIC ULONG LibExpunge( REG(a6, struct GradientSliderBase_intern *ClassBase ) );
STATIC ULONG LibClose( REG(a6, struct GradientSliderBase_intern *ClassBase ) );
STATIC LONG LibExtFunc(void);


BOOL __regargs L_OpenLibs(struct GradientSliderBase_intern *);
void __regargs L_CloseLibs(struct GradientSliderBase_intern *);
Class * __regargs initClass (struct GradientSliderBase_intern *);
Class  * ObtainClass(REG(a6, struct GradientSliderBase_intern *) );

extern ULONG   dispatch_gradientsliderclass(REG(a0, Class *), REG(a2, Object *), REG(a1, Msg ) );

/****************************************************************************/

UBYTE	LibName[] = NAME_STRING,
		LibID[] = VERSION_STRING;

LONG	LibVersion = VERSION_NUMBER,
		LibRevision = REVISION_NUMBER;

APTR LibVectors[] =
{
	(APTR) LibOpen,
	(APTR) LibClose,
	(APTR) LibExpunge,
	(APTR) LibExtFunc,
	(APTR) ObtainClass,
	(APTR) -1L
};

ULONG LibInitTab[] =
{
	(ULONG) sizeof(struct GradientSliderBase_intern),
	(ULONG) LibVectors,
	(ULONG) NULL,
	(ULONG) LibInit
};

struct Resident ROMTag =     /* do not change */
{
	RTC_MATCHWORD,
 	&ROMTag,
 	&ROMTag + sizeof(ROMTag),
	RTF_AUTOINIT,
 	VERSION_NUMBER,
	NT_LIBRARY,
 	0,
	LibName,
	LibID,
	(APTR)&LibInitTab
};

/****************************************************************************/

LONG LibExtFunc(void)
{
	return(-1L);
}

/****************************************************************************/

struct Library *
  __saveds LibInit( REG(a0, ULONG seglist), REG(d0, struct GradientSliderBase_intern *GradientSliderBase), REG(a6, struct Library *ExecBase ) )
{
	SysBase = (struct ExecBase *) ExecBase;
	GradientSliderBase->seglist = seglist;
	GradientSliderBase->library.lib_Revision = REVISION_NUMBER;

	if( ( ExecBase->lib_Version >= 39L ) && ( ((struct ExecBase *)SysBase)->AttnFlags & AFF_68020 ) )
	{
		if(L_OpenLibs(GradientSliderBase))
		{
			if(GradientSliderBase->classptr = InitGradientSliderClass(GradientSliderBase))
			{
				return((struct Library *)GradientSliderBase);
			}
		}

		L_CloseLibs( GradientSliderBase );
	}

	FreeMem((BYTE *)GradientSliderBase-GradientSliderBase->library.lib_NegSize,
		GradientSliderBase->library.lib_NegSize + GradientSliderBase->library.lib_PosSize);

	ROMTag.rt_Version = VERSION_NUMBER;

	return(NULL);
}

/****************************************************************************/

struct Library * __saveds  LibOpen(REG(a6, struct GradientSliderBase_intern *GradientSliderBase ) )
{
	GradientSliderBase->library.lib_Flags &= ~LIBF_DELEXP;
	GradientSliderBase->library.lib_OpenCnt++;

	return((struct Library *)GradientSliderBase);
}

/****************************************************************************/

ULONG __saveds LibExpunge(REG(a6, struct GradientSliderBase_intern *GradientSliderBase) )
{
 	if(!GradientSliderBase->library.lib_OpenCnt)
 	{
		if(GradientSliderBase->classptr)
		{
			if( ! FreeClass(GradientSliderBase->classptr) )
			{
				AddClass( GradientSliderBase->classptr );
				return NULL;
			}
		}

		L_CloseLibs(GradientSliderBase);

 		Remove((struct Node *)GradientSliderBase);

  		FreeMem((BYTE *)GradientSliderBase-GradientSliderBase->library.lib_NegSize,
	          GradientSliderBase->library.lib_NegSize + GradientSliderBase->library.lib_PosSize);

		return( GradientSliderBase->seglist );
 	}
 	else
	{
		GradientSliderBase->library.lib_Flags |= LIBF_DELEXP;
	}

 	return( NULL );
}

/****************************************************************************/

ULONG __saveds  LibClose(REG(a6, struct GradientSliderBase_intern *GradientSliderBase) )
{
 	if(GradientSliderBase->library.lib_OpenCnt)
	{
		GradientSliderBase->library.lib_OpenCnt--;
	}

 	return((!GradientSliderBase->library.lib_OpenCnt &&
        	 GradientSliderBase->library.lib_Flags & LIBF_DELEXP)?LibExpunge(GradientSliderBase):NULL);
}

/****************************************************************************/

struct GfxBase *GfxBase;
struct IntuitionBase *IntuitionBase;
struct Library *LayersBase;
struct UtilityBase *UtilityBase;
struct Library *CyberGfxBase;

BOOL __regargs L_OpenLibs( struct GradientSliderBase_intern *GradientSliderBase )
{
	if(	( GfxBase = OpenLibrary( "graphics.library", 39L ) ) &&
		( IntuitionBase = OpenLibrary( "intuition.library", 39L ) ) &&
		( LayersBase = OpenLibrary( "layers.library", 39L ) ) &&
		( UtilityBase = OpenLibrary( "utility.library", 39L ) )	)
	{
		CyberGfxBase = OpenLibrary( "cybergraphics.library", 40L );

		return( TRUE );
	}

	return FALSE;
}

/****************************************************************************/

void __regargs L_CloseLibs( struct GradientSliderBase_intern *GradientSliderBase )
{
	CloseLibrary( (struct Library *)GfxBase );
	CloseLibrary( (struct Library *)IntuitionBase );
	CloseLibrary( UtilityBase );
	CloseLibrary( CyberGfxBase );
	CloseLibrary( LayersBase );
}

/****************************************************************************/

Class * __saveds ObtainClass(REG(a6, struct GradientSliderBase_intern *GradientSliderBase) )
{
	return(GradientSliderBase->classptr);
}

/****************************************************************************/

ULONG __saveds dispatch_gradientsliderclass( REG(a0, Class *cl), REG(a2, Object *o), REG(a1, Msg msg ) )
{
    IPTR retval = 0UL;
    
    switch(msg->MethodID)
    {
	case GM_HANDLEINPUT:
	    retval = GradientSlider__GM_HANDLEINPUT(cl, o, (struct gpInput *)msg);
	    break;

	case OM_SET:
	case OM_UPDATE:
	    retval = (IPTR)GradientSlider__OM_SET(cl, o, (struct opSet *)msg);
	    break;

	case GM_RENDER:
	    GradientSlider__GM_RENDER(cl, o, (struct gpRender *)msg);
	    break;
	    
	case GM_HITTEST:
	    retval = GradientSlider__GM_HITTEST(cl, o, (struct gpHitTest *)msg);
	    break;
	    
	case GM_GOACTIVE:
	    retval = GradientSlider__GM_GOACTIVE(cl, o, (struct gpInput *)msg);
	    break;

	case OM_GET:
	    retval = GradientSlider__OM_GET(cl, o, (struct opGet *)msg);
	    break;
	    
	case OM_NEW:
	    retval = (IPTR)GradientSlider__OM_NEW(cl, o, (struct opSet *)msg);
	    break;
	
	case OM_DISPOSE:
	    GradientSlider__OM_DISPOSE(cl, o, msg);
	    break;

	case GM_DOMAIN:
	    retval = GradientSlider__GM_DOMAIN(cl, o, (struct gpDomain *)msg);
	    break;
	    
	default:
	    retval = DoSuperMethodA(cl, o, msg);
	    break;
	    
    } /* switch */

    return (retval);
}  /* dispatch_gradientsliderclass */

/***************************************************************************************************/

struct IClass *InitGradientSliderClass (struct GradientSliderBase_intern * GradientSliderBase)
{
    struct IClass *cl = NULL;

    if ((cl = MakeClass("gradientslider.gadget", GADGETCLASS, NULL, sizeof(struct GradientSliderData), 0)))
    {
	cl->cl_Dispatcher.h_Entry    = (HOOKFUNC)dispatch_gradientsliderclass;
	cl->cl_Dispatcher.h_SubEntry = NULL;
	cl->cl_UserData 	     = (IPTR)GradientSliderBase;

	AddClass (cl);
    }

    return (cl);
}

/***************************************************************************************************/
