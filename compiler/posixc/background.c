/*
    Copyright © 2013, The AROS Development Team. All rights reserved.

    This file is there to provide a background section in the Autodocs
    for the stdc.library
*/

/*******************************************************************************

    NAME
        --background_POSIX--

    NOTES
        On AROS standardized C interfaces are implemented with a few shared
        libraries. A distinction is made between a standard ANSI-C/ISO-C
        part and a POSIX emulation layer.
        Here the POSIX part is documented.

        The posixc.library is a shared library that implements (part of) the C
        interface for POSIX.1-2008 on AROS and the standard document is used
        as guideline for this implementation.
        Purpose of this library is to implement a POSIX compliant compatibility
        layer. Currently no full implementation of the POSIX layer is provided
        but enough to port a lot of POSIX compliant or LINUX code over to AROS.
        As this library has overhead in implementatio 'real' AROS/Amiga programs
        should not depend on this library.

        Also some non-standard or legacy POSIX functions are provided by the
        library. If possible they are put in the static link library so the
        functions can be removed in the future without breaking backwards
        compatibility.
        When porting code it is then preferred to patch the source code to use
        the POSIX.1-2008 version of files or provide a local version of some
        of the functions.

        The include files provided for the POSIX code implement a proper
        separation of the include files. The includes should only define the
        prototypes, constants etc.  as defined by the standard. This means
        includes like proto/posixc.h should not be included in the standard
        POSIX include files. Developers improving or extending these libraries
        should keep this in mind.

        In order to use the posixc.library programs need to properly initialize
        the library using the __posixc_nixmain() function. It is assumed that
        this is taken care of by the startup code provided by the compiler.

    SEE ALSO
        stdc.library/--background_C99--


*******************************************************************************/
