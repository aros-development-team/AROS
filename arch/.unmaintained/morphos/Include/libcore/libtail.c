/*
**	$VER: libtail.c 37.15 (14.8.97)
**
**	This file must be compiled and must be passed as the last
**	object to link to the linker.
**
**	(C) Copyright 1996-97 Andreas R. Kleinert
**	All Rights Reserved.
*/

/* If the file with the #defines for this library is not "libdefs.h",
    then you can redefine it. */
#ifndef LIBDEFS_FILE
#   define LIBDEFS_FILE "libdefs.h"
#endif

/* Include the file with the #defines for this library */
#include LIBDEFS_FILE

const int LIBEND = 1;	       /* The end of the library */
