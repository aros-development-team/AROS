/*
**    © Copyright 1996-97 Andreas R. Kleinert
**    All Rights Reserved.
** 
**    Copyright © 1997-2001, The AROS Development Team. All rights reserved.
**    $Id$
**
**    This file needs to be at the end of a compiled module. Most of the
**    time this is done automatically by the support tools for creating 
**    modules.
**
**    This file is based on a file from the CLib37x.lha package of Andreas R.
**    Kleinert (of which a more recent version is available on aminet) and
**    adapted to fit in the AROS build framework.
** 
**    To be able to compile modules with a license incompatible with the AROS
**    Public License users may relicense this file under any license of their
**    choice.
*/

/* If the file with the #defines for this library is not "libdefs.h",
    then you can redefine it. */
#ifndef LC_LIBDEFS_FILE
#   define LC_LIBDEFS_FILE "libdefs.h"
#endif

/* Include the file with the #defines for this library */
#include LC_LIBDEFS_FILE

//const int LIBEND = 1;	       /* The end of the library */

#define __STR(x) #x
#define STR(x) __STR(x)

/*
    M.Schulz:
	I have changed definition of LIBEND a little bit. Because we do not
	need any physical form of LIBEND and use only pointer to it, I have
	defined symbol of value equal to current address. It takes 0 bytes
	after compilation process and is globally accessible. :)

	The same trick may be used to redirect all undefined references to
	SysBase. One may write ".globl SysBase \n\t .set SysBase,4"
*/
asm (".globl "STR(LIBEND)"\n\t.section \".sdata\"\n\t.set "STR(LIBEND)",.");

