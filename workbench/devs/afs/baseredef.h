#ifndef BASEREDEF_H
#define BASEREDEF_H

/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

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

