/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc: ReqTools initialization code.
    Lang: English.
*/

/****************************************************************************************/

#define  AROS_ALMOST_COMPATIBLE

#include "reqtools_intern.h"
#include <exec/types.h>
#include <exec/resident.h>
#include <intuition/intuition.h>
#include <devices/conunit.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <intuition/classes.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>

#include "initstruct.h"
#include <stddef.h>

#define DEBUG 1
#include <aros/debug.h>

#include <exec/libraries.h>
#include <exec/alerts.h>
#include "libdefs.h"

#include "general.h"
#include "boopsigads.h"

/****************************************************************************************/

#define INIT	AROS_SLIB_ENTRY(init, ReqTools)

struct inittable;
extern const char name[];
extern const char version[];
extern const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];
extern const struct inittable datatable;
extern struct IntReqToolsBase *INIT();
extern struct IntReqToolsBase *AROS_SLIB_ENTRY(open, ReqTools)();
extern BPTR AROS_SLIB_ENTRY(close, ReqTools)();
extern BPTR AROS_SLIB_ENTRY(expunge, ReqTools)();
extern int AROS_SLIB_ENTRY(null, ReqTools)();
extern const char LIBEND;

/****************************************************************************************/

int entry(void)
{
    /* If the library was executed by accident return error code. */
    return -1;
}

/****************************************************************************************/

const struct Resident resident=
{
    RTC_MATCHWORD,
    (struct Resident *)&resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT,
    VERSION_NUMBER,
    NT_LIBRARY,
    0,
    (char *)name,
    (char *)&version[6],
    (ULONG *)inittabl
};

const char name[] = NAME_STRING;

const char version[] = VERSION_STRING;

const APTR inittabl[4]=
{
    (APTR)sizeof(struct IntReqToolsBase),
    (APTR)LIBFUNCTABLE,
    (APTR)&datatable,
    &INIT
};

struct inittable
{
    S_CPYO(1,1,B);
    S_CPYO(2,1,L);
    S_CPYO(3,1,B);
    S_CPYO(4,1,W);
    S_CPYO(5,1,W);
    S_CPYO(6,1,L);
    S_END (LIBEND);
};

#define O(n) offsetof(struct ReqToolsBase, n)

const struct inittable datatable=
{
    { { I_CPYO(1,B,O(LibNode.lib_Node.ln_Type)), { NT_LIBRARY } } },
    { { I_CPYO(1,L,O(LibNode.lib_Node.ln_Name)), { (IPTR)name } } },
    { { I_CPYO(1,B,O(LibNode.lib_Flags       )), { LIBF_SUMUSED|LIBF_CHANGED } } },
    { { I_CPYO(1,W,O(LibNode.lib_Version     )), { 1 } } },
    { { I_CPYO(1,W,O(LibNode.lib_Revision    )), { 0 } } },
    { { I_CPYO(1,L,O(LibNode.lib_IdString    )), { (IPTR)&version[6] } } },
	I_END ()
};

#undef O

/****************************************************************************************/

#ifdef _AROS
AROS_UFP3(IPTR, myBoopsiDispatch,
	  AROS_UFPA(Class *, cl, A0),
	  AROS_UFPA(struct Image *, im, A2),
	  AROS_UFPA(Msg, msg, A1));
#endif

/****************************************************************************************/

struct ReqToolsBase 	*ReqToolsBase, *RTBase;
struct ReqToolsBase	**RTBasePtr = &RTBase;

struct ExecBase 	*SysBase;
struct DosLibrary 	*DOSBase;
struct UtilityBase 	*UtilityBase;
struct IntuitionBase	*IntuitionBase;
struct GfxBase 		*GfxBase;
struct LocaleBase 	*LocaleBase;
struct Library 		*LayersBase;
struct Library 		*GadToolsBase;
struct Device 		*ConsoleDevice;
struct IOStdReq		iorequest;
Class			*ButtonImgClass;

/****************************************************************************************/

AROS_LH2(struct IntReqToolsBase *, init,
 AROS_LHA(struct IntReqToolsBase *, RTBase, D0),
 AROS_LHA(BPTR,               segList,   A0),
	   struct ExecBase *, sysBase, 0, ReqTools)
{
    AROS_LIBFUNC_INIT
    
    *RTBasePtr = ReqToolsBase = (struct ReqToolsBase *)RTBase;
        
    /* This function is single-threaded by exec by calling Forbid. */

    /* Store arguments */
    RTBase->rt_SysBase = SysBase = sysBase;
    RTBase->rt.SegList = segList;

    D(bug("reqtools.library: Inside libinit func\n"));
    
    InitSemaphore(&RTBase->rt.ReqToolsPrefs.PrefsSemaphore);
    RTBase->rt.ReqToolsPrefs.PrefsSize = RTPREFS_SIZE;

    /* Set default preferences */
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].Size = 75;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].LeftOffset = 25;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].TopOffset = 18;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].MinEntries = 10;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FILEREQ].MaxEntries = 50;

    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].Size = 65;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].LeftOffset = 25;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].TopOffset = 18;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].MinEntries = 6;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_FONTREQ].MaxEntries = 10;

    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].Size = 65;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].LeftOffset = 25;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].TopOffset = 18;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].MinEntries = 6;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_PALETTEREQ].MaxEntries = 10;

    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].Size = 65;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].LeftOffset = 25;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].TopOffset = 18;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].MinEntries = 6;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_SCREENMODEREQ].MaxEntries = 10;

    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].Size = 65;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].LeftOffset = 25;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].TopOffset = 18;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].MinEntries = 6;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_VOLUMEREQ].MaxEntries = 10;

    //    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].Size = 65;
    //    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].ReqPos = REQPOS_TOPLEFTSCR;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].LeftOffset = 25;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].TopOffset = 18;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].MinEntries = 6;
    RTBase->rt.ReqToolsPrefs.ReqDefaults[RTPREF_OTHERREQ].MaxEntries = 10;

    return RTBase;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH1(struct IntReqToolsBase *, open,
 AROS_LHA(ULONG, version, D0),
	   struct IntReqToolsBase *, RTBase, 1, ReqTools)
{
    AROS_LIBFUNC_INIT
    
    D(bug("reqtools.library: Inside libopen func\n"));
 
    /*
      This function is single-threaded by exec by calling Forbid.
      If you break the Forbid() another task may enter this function
      at the same time. Take care.
    */
    
    /* Keep compiler happy */
    version = 0;
    
    if (DOSBase == NULL)
        DOSBase = RTBase->rt.rt_DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 37);
    if (DOSBase == NULL)
        return NULL;
	
    if(IntuitionBase == NULL)
	IntuitionBase = RTBase->rt.rt_IntuitionBase = (IntuiBase *)OpenLibrary("intuition.library", 37);
    if(IntuitionBase == NULL)
	return NULL;

    if(GfxBase == NULL)
	GfxBase = RTBase->rt.rt_GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
    if(GfxBase == NULL)
	return NULL;
    
    if(UtilityBase == NULL)
	UtilityBase = RTBase->rt.rt_UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37);
    if(UtilityBase == NULL)
	return NULL;

    if(GadToolsBase == NULL)
	GadToolsBase = RTBase->rt.rt_GadToolsBase = OpenLibrary("gadtools.library", 37);
    if(GadToolsBase == NULL)
	return NULL;

    if(LayersBase == NULL)
	LayersBase = OpenLibrary("layers.library", 37);
    if(LayersBase == NULL)
	return NULL;

    if(LocaleBase == NULL)
	LocaleBase = (struct LocaleBase *)OpenLibrary("locale.library", 37);
    if(LocaleBase == NULL)
	return NULL;

    D(bug("reqtools.library: Inside libopen func. Libraries opened successfully.\n"));

    if (ConsoleDevice == NULL)
    {
        iorequest.io_Message.mn_Length = sizeof(iorequest);
	
        if (OpenDevice("console.device", CONU_LIBRARY, (struct IORequest *)&iorequest, 0))
	{
	    return NULL;
	}
	ConsoleDevice = iorequest.io_Device;
    }
    if (ConsoleDevice == NULL)
        return NULL;

    D(bug("reqtools.library: Inside libopen func. Console.device opened successfully.\n"));
	
    if (ButtonImgClass == NULL)
    {
        ButtonImgClass = MakeClass(NULL, IMAGECLASS, NULL, sizeof(struct LocalObjData), 0);
	if (ButtonImgClass)
	{
	    ButtonImgClass->cl_Dispatcher.h_Entry = (APTR)AROS_ASMSYMNAME(myBoopsiDispatch);
	    ButtonImgClass->cl_Dispatcher.h_SubEntry = NULL;
	    ButtonImgClass->cl_UserData = (IPTR)RTBase;
	}
    }
    if (ButtonImgClass == NULL)
        return NULL;
    
    D(bug("reqtools.library: Inside libopen func. ButtonImgClass create successfully.\n"));

    /* I have one more opener. */
    RTBase->rt.LibNode.lib_Flags &= ~LIBF_DELEXP;
    RTBase->rt.RealOpenCnt++;

    return RTBase;

    D(bug("reqtools.library: Inside libopen func. Returning success.\n"));

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, close, struct IntReqToolsBase *, RTBase, 2, ReqTools)
{
    AROS_LIBFUNC_INIT
    /*
	This function is single-threaded by exec by calling Forbid.
	If you break the Forbid() another task may enter this function
	at the same time. Take care.
    */

    D(bug("reqtools.library: Inside libclose func.\n"));

    /* I have one fewer opener. */
    RTBase->rt.RealOpenCnt--;
    
    if((RTBase->rt.LibNode.lib_Flags & LIBF_DELEXP) != 0)
    {
	if(RTBase->rt.LibNode.lib_OpenCnt == 0)
	    return expunge();
	
	RTBase->rt.LibNode.lib_Flags &= ~LIBF_DELEXP;
    }

    return NULL;

    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0(BPTR, expunge, struct IntReqToolsBase *, RTBase, 3, ReqTools)
{
    AROS_LIBFUNC_INIT

    BPTR ret;
    /*
	This function is single-threaded by exec by calling Forbid.
	Never break the Forbid() or strange things might happen.
    */

    /* Test for openers. */
 
    D(bug("reqtools.library: Inside libexpunge func.\n"));

    if(RTBase->rt.RealOpenCnt != 0)
    {
	/* Set the delayed expunge flag and return. */
	RTBase->rt.LibNode.lib_Flags |= LIBF_DELEXP;
	return NULL;
    }

    /* Get rid of the library. Remove it from the list. */
    Remove(&RTBase->rt.LibNode.lib_Node);

    /* Get returncode here - FreeMem() will destroy the field. */
    ret = RTBase->rt.SegList;

    D(bug("reqtools.library: Inside libexpunge func. Freeing ButtonImgClass.\n"));

    if (ButtonImgClass) FreeClass(ButtonImgClass);

    D(bug("reqtools.library: Inside libexpunge func. Closing console.device.\n"));
    
    if (ConsoleDevice) CloseDevice((struct IORequest *)&iorequest);

    D(bug("reqtools.library: Inside libexpunge func. Closing libraries.\n"));
    
    CloseLibrary((struct Library *)DOSBase);
    CloseLibrary((struct Library *)IntuitionBase);
    CloseLibrary((struct Library *)UtilityBase);
    CloseLibrary((struct Library *)GfxBase);
    CloseLibrary((struct Library *)LocaleBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary(LayersBase);

    D(bug("reqtools.library: Inside libexpunge func. Freeing libbase.\n"));

    /* Free the memory. */
    FreeMem((char *)RTBase-RTBase->rt.LibNode.lib_NegSize,
	    RTBase->rt.LibNode.lib_NegSize + RTBase->rt.LibNode.lib_PosSize);

    return ret;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/

AROS_LH0I(int, null, struct ReqToolsBase *, RTBase, 4, ReqTools)
{
    AROS_LIBFUNC_INIT
    return 0;
    AROS_LIBFUNC_EXIT
}

/****************************************************************************************/
