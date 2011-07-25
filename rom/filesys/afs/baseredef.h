#ifndef BASEREDEF_H
#define BASEREDEF_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifdef DOSBase
#	undef DOSBase
#endif
#define DOSBase afsbase->dosbase
#endif

