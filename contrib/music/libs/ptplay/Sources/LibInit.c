#include	<proto/dos.h>
#include	<proto/exec.h>

#include	"LibHeader.h"

#define	SysBase		LibBase->MySysBase
#define	UtilityBase	LibBase->MyUtilBase

/*********************************************************************
 * @LibReserved																		*
 ********************************************************************/

ULONG LibReserved(void)
{
	return 0;
}

/*********************************************************************
 * @LibInit																				*
 ********************************************************************/

struct Library	*LibInit(REG(d0, struct PtPlayLibrary *LibBase), REG(a0, BPTR SegList), REG(a6, struct ExecBase *MySysBase))
{
	LibBase->SegList	= SegList;
	SysBase				= MySysBase;

	if ((UtilityBase	= (APTR)OpenLibrary("utility.library"	, 36)) == NULL)
		LibBase	= NULL;

	return (struct Library *)LibBase;
}

/*********************************************************************
 * @DeleteLib																			*
 ********************************************************************/

static BPTR DeleteLib(struct PtPlayLibrary *LibBase)
{
	BPTR	SegList	= 0;

	if (LibBase->Library.lib_OpenCnt == 0)
	{
		CloseLibrary((struct Library *)UtilityBase);

		SegList	= LibBase->SegList;

		Remove(&LibBase->Library.lib_Node);
		FreeMem((APTR)((ULONG)(LibBase) - (ULONG)(LibBase->Library.lib_NegSize)), LibBase->Library.lib_NegSize + LibBase->Library.lib_PosSize);
	}

	return SegList;
}

/*********************************************************************
 * @LibExpunge																			*
 ********************************************************************/

BPTR NATDECLFUNC_1(LibExpunge, a6, struct PtPlayLibrary *, LibBase)
{
	DECLARG_1(a6, struct PtPlayLibrary *, LibBase)

	LibBase->Library.lib_Flags	|= LIBF_DELEXP;

	return DeleteLib(LibBase);
}

/*********************************************************************
 * @LibClose																			*
 ********************************************************************/

BPTR NATDECLFUNC_1(LibClose, a6, struct PtPlayLibrary *, LibBase)
{
	DECLARG_1(a6, struct PtPlayLibrary *, LibBase)
	BPTR	SegList	= 0;

	LibBase->Library.lib_OpenCnt--;

	if (LibBase->Library.lib_Flags & LIBF_DELEXP)
		SegList	= DeleteLib(LibBase);

	return SegList;
}

/*********************************************************************
 * @LibOpen																				*
 ********************************************************************/

struct Library	*NATDECLFUNC_1(LibOpen, a6, struct PtPlayLibrary *, LibBase)
{
	DECLARG_1(a6, struct PtPlayLibrary *, LibBase)
	struct Library	*base	= &LibBase->Library;

	LibBase->Library.lib_Flags &= ~LIBF_DELEXP;
	LibBase->Library.lib_OpenCnt++;

	return base;
}
