#ifndef DESKTOP_INTERN_H
#define DESKTOP_INTERN_H

#include <aros/libcall.h>
#include <exec/execbase.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <intuition/intuitionbase.h>

struct DesktopBase
{
    struct Library db_Library;

	BPTR db_SegList;

	struct ExecBase *db_SysBase;
	struct Library *db_DOSBase;
	struct IntuitionBase *db_IntuitionBase;
	struct Library *db_GfxBase;
	struct Library *db_LayersBase;
	struct Library *db_UtilityBase;
	struct Library *db_MUIMasterBase;

	struct MUI_CustomClass *db_IconContainer;
	struct MUI_CustomClass *db_IconContainerObserver;

	struct SignalSemaphore  db_BaseMutex;
	struct SignalSemaphore  db_HandlerSafety;

	struct MsgPort *db_HandlerPort;

	BOOL db_libsOpen;
};

extern struct DesktopBase *DesktopBase;

#define init(dummybase, segList) \
AROS_LC2(LIBBASETYPEPTR, init, AROS_LHA(LIBBASETYPEPTR, BASENAME, D0), AROS_LHA(BPTR, segList, A0), struct ExecBase *, SysBase, 0, BASENAME)

#define open(version) \
AROS_LC1(LIBBASETYPEPTR, open, AROS_LHA(ULONG, version, D0), LIBBASETYPEPTR, LIBBASE, 1, BASENAME)

#define close() \
AROS_LC0(BPTR, close, LIBBASETYPEPTR, LIBBASE, 2, BASENAME)

#define expunge() \
AROS_LC0(BPTR, expunge, LIBBASETYPEPTR, LIBBASE, 3, BASENAME)

#define null() \
AROS_LC0(int, null, LIBBASETYPEPTR, LIBBASE, 4, BASENAME)

#define add(a, b) \
AROS_LC2(ULONG, add, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct DesktopBase *,DesktopBase,5,Desktop)

#define asl(a, b) \
AROS_LC2(ULONG, asl, AROS_LHA(ULONG,a,D0), AROS_LHA(ULONG,b,D1), struct DesktopBase *,DesktopBase,6,Desktop)

#define SysBase DesktopBase->db_SysBase
#define DOSBase DesktopBase->db_DOSBase
#define GfxBase DesktopBase->db_GfxBase
#define IntuitionBase DesktopBase->db_IntuitionBase
#define LayersBase DesktopBase->db_LayersBase
#define UtilityBase DesktopBase->db_UtilityBase
#define MUIMasterBase DesktopBase->db_MUIMasterBase
#define IconContainer DesktopBase->db_IconContainer
#define IconContainerObserver DesktopBase->db_IconContainerObserver

#endif /* DESKTOP_INTERN_H */
