
#include <exec/types.h>
#include <proto/exec.h>
#include <aros/asmcall.h>

#include "dbus_intern.h"
#include "gateproto.h"

struct ExecBase* DBUS_SysBase;

/* Customize libheader.c */
#define LC_LIB_FIELD(lib)       (((LIBBASETYPEPTR)(lib))->libnode)
//#define LC_SYSBASE_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->sysbase)
#define LC_SYSBASE_FIELD(lib)   DBUS_SysBase
#define LC_SEGLIST_FIELD(lib)   (((LIBBASETYPEPTR       )(lib))->seglist)
#define LC_RESIDENTNAME         Dbus_ROMTag
#define LC_RESIDENTPRI          0
#define LC_RESIDENTFLAGS        RTF_AUTOINIT|RTF_COLDSTART
#define LC_LIBBASESIZE          sizeof(struct DBUSBase)
#define LC_LIBHEADERTYPEPTR     LIBBASETYPEPTR

#define LC_NO_OPENLIB
#define LC_NO_EXPUNGELIB
#define LC_NO_CLOSELIB


#define NOEXPUNGE

#include <libcore/libheader.c>

static const APTR LIBFUNCTABLE[] = {
  AROS_SLIB_ENTRY(LC_BUILDNAME(OpenLib),LibHeader),
  AROS_SLIB_ENTRY(LC_BUILDNAME(CloseLib),LibHeader),
  AROS_SLIB_ENTRY(LC_BUILDNAME(ExpungeLib),LibHeader),
  AROS_SLIB_ENTRY(LC_BUILDNAME(ExtFuncLib),LibHeader),

  /* Reserved for ARexx host some day in the distant future */
  NULL,

  /* Reserved for AmigaOS-specific functions */
  NULL, NULL, NULL, NULL, NULL, 
  NULL, NULL, NULL, NULL, NULL,

#include "functable.h"
  
  (APTR) -1
};

ULONG SAVEDS STDARGS LC_BUILDNAME(L_InitLib) (LC_LIBHEADERTYPEPTR lh) {
  InitThreads(lh);
  return TRUE;
}
