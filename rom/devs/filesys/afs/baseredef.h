#ifndef BASEREDEF_H
#define BASEREDEF_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
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

