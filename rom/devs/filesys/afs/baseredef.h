#ifndef BASEREDEF_H
#define BASEREDEF_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: baseredef.h 27106 2007-10-28 10:49:03Z verhaegs $
*/

#ifdef DOSBase
#	undef DOSBase
#endif
#ifdef IntuitionBase
#	undef IntuitionBase
#endif
#define DOSBase afsbase->dosbase
#define IntuitionBase afsbase->intuitionbase
#endif

