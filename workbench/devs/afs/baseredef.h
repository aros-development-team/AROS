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
#define SysBase afsbase->sysbase
#define DOSBase afsbase->dosbase
#define IntuitionBase afsbase->intuitionbase

#endif

