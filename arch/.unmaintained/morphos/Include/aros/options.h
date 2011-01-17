/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#pragma pack(2)
#ifndef AROS_OPTIONS_H
#define AROS_OPTIONS_H

/******************************************************************************

    FILE
	$Id$

    DESCRIPTION
	This files defines a couple of options which change the way
	the OS is compiled.

******************************************************************************/

/* Should the system compile a working SetFunction()-call? If you
   enable this, "real" shared libraries (with LVOs and the like)
   will be generated and all Functions will be called via LVOs.
   If you disable this, all functions will be called somewhat more
   directly. To enable the option, change the 0 to 1. */
#ifndef EnableSetFunction
#   define EnableSetFunction	1
#endif
#if EnableSetFunction
#   define UseLVOs		1
#endif

/* Should the OS use registerized arguments or pass the arguments on
   stack? */
#ifndef UseRegisterArgs
#   define UseRegisterArgs	0
#endif

/* Should the OS be compiled with macros that call the actual function
   and with the library base as first parameter or should the library
   base be a global variable (shared by all functions). */
#ifndef LibBaseShouldBeAParameter
#   define LibBaseShouldBeAParameter	1
#endif

/* Should the OS be compiled with LVO tables or should the functions
   be called directly? Note that this is superseeded by
   EnableSetFunction. */
#ifndef UseLVOs
#   define UseLVOs		0
#endif

/* If the following is defined, errnos from unix function-calls that could not
   be interpreted as AROS error numbers (ERROR_* defined in <dos/dos.h> are
   passed back via SetIoErr(). The errno is incremented by the value of this
   define. Note that this value should generate unique error numbers! Note
   also that Fault() checks, whether an error-number is in fact an errno, by
   AND'ing this value to the error number! The recommended value is 0x40000000
   (ie only the 30th bit set).

   If this is defined to 0, the errno will not be passed through. Instead
   ERROR_UNKNOWN is returned, if the errno could not be interpreted.

   On AROS systems without any Unix emulation (ie every system without
   underlying POSIX-compatible operating system), this should be set to 0 to
   simplify the resulting code. */
#ifndef PassThroughErrnos
#   define PassThroughErrnos	0
/* #   define PassThroughErrnos	0x40000000 */
#endif

/******************************************************************************
*****  END aros/options.h
******************************************************************************/

#endif /* AROS_OPTIONS_H */

#pragma pack()
