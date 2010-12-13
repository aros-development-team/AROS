#ifndef BASEREDEF_H
#define BASEREDEF_H

#ifdef SysBase
#	undef SysBase
#endif
#ifdef DOSBase
#	undef DOSBase
#endif
#ifdef IntuitionBase
#	undef IntuitionBase
#endif
#ifdef UtilityBase
#	undef UtilityBase
#endif
#ifdef IconBase
#	undef IconBase
#endif
#ifdef WorkbenchBase
#	undef WorkbenchBase
#endif

#define SysBase acdrbase->SysBase
#define DOSBase acdrbase->DOSBase
#define UtilityBase acdrbase->UtilityBase
#define IntuitionBase acdrbase->IntuitionBase
#define IconBase acdrbase->IconBase
#define WorkbenchBase acdrbase->WorkbenchBase

#endif

