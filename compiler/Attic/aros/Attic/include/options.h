#ifndef AROS_OPTIONS_H
#define AROS_OPTIONS_H
/* (C) 1995 AROS - The Amiga Replacement OS */

/******************************************************************************

    FILE
	$Id$

    DESCRIPTION
	This files defines a couple of options which change the way
	the OS is compiled.

******************************************************************************/

/*  Should the system compile a working SetFunction()-call ? If you
    enable this, "real" shared libraries (with LVOs and the like)
    will be generated and all Functions will be called via LVOs.
    If you disable this, all functions will be called somewhat more
    directly. To enable the option, change the 0 to 1. */
#ifndef EnableSetFunction
#   define EnableSetFunction   0
#endif
#if EnableSetFunction
#   define UseLVOs		1
#endif

/*  Should the OS use registerized arguments or pass the arguments on
    stack ? */
#ifndef UseRegisterArgs
#   define UseRegisterArgs	0
#endif

/*  Should the OS be compiled with macros that call the actual function
    and with the library base as first parameter or should the library
    base be a global variable (shared by all functions). */
#ifndef LibBaseShouldBeAParameter
#   define LibBaseShouldBeAParameter	0
#endif

/*  Should the OS be compiled with LVO tables or should the functions
    be called directly ? Note that this is superseeded by
    EnableSetFunction. */
#ifndef UseLVOs
#   define UseLVOs		0
#endif


/******************************************************************************
*****  ENDE aros/options.h
******************************************************************************/

#endif /* AROS_OPTIONS_H */
