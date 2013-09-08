/*
    Copyright © 2013, The AROS Development Team. All rights reserved.

    This file is there to provide a background section in the Autodocs
    for the stdc.library
*/

/*******************************************************************************

    NAME
        --background_C99--

    NOTES
        On AROS standardized C interfaces are implemented with a few shared
        libraries. A distinction is made between a standard ANSI-C/ISO-C
        part and a POSIX emulation layer.
        Here the ANSI-C/ISO-C part is documented.

        The ANSI-C/ISO-C part is implemented by the stdc.library and the
        stdcio.library shared libraries. The former implements the part that
        only depends on exec.library; the latter the parts that depends on
        other libraries like dos.library and contains mostly I/O related
        functions.
        Currently both libraries are disk based but the plan is in the future
        put stdc.library in ROM and be initialized right after exec so that
        all modules, also those in ROM can use it. stdcio.library will likely
        stay disk based.
        Purpose of these libraries is to provide a base implementation that
        can be used by compilers. Compilers are free to override functions
        with their own implementation.

        The reference used for the developing the two libraries is the ISO/IEC
        standard document ISO/IEC 9899:1999 also known as C99. Not all functions
        are implemented but for each function defined in the standard a place
        in the library is reserved. The order of the functions in the library
        lookup table is based on the order that they are defined in the standard
        document.
 
        Not all functions are implemented. Not implemented functions either
        are not available in the library at all or just a stub function is
        provided. The latter can be used to get programs running without having
        a proper implementation of a function. Of course target should be to
        have in the end all functions implemented.
        Consult the include files and the autodocs to see which functions are
        not (fully) implemented.

        The include files provided for the C99 code implement a proper
        separation of the include files. The includes should only define the
        prototypes as defined by the standard. This means includes like
        proto/stdc.h or proto/stdcio.h should not be included in the standard
        C99 include files. Developers improving or extending these libraries
        should keep this in mind.

        In order to use the stdc.library programs need to properly initialize
        the library using the __stdc_progam_startup() and __stdc_program_end()
        functions. It is assumed that this is taken care of by the startup
        code provided by the compiler.

    SEE ALSO
        posixc.library/--background_posixc--

*******************************************************************************/


/*******************************************************************************

    NAME
        --background_locale--

    NOTES
        Currently no real locale support is provided by stdc.library. All locale
        related functions have a minimal implementation assuming only a "C"
        locale.
        Implementing proper locale support will need careful development to have
        a consistent integration with locale.library. People with ideas can
        always post on the AROS development mailing list.

    SEE ALSO
        locale.h, --background_wchar-- --background_c99--

*******************************************************************************/


/*******************************************************************************

    NAME
        --background_wchar--

    NOTES
        wchar.h/wctype.h is not implemented by stdc.library. It is left to
        the compiler to provide their implementation of wchar support.
        No system functions should thus at the moment use wchar as
        implementation is compiler dependent.

    SEE ALSO
        --background_locale-- --background_c99--

*******************************************************************************/


/*******************************************************************************

    NAME
        --background_string--

    NOTES
        Contrary to the other include files; almost all string functions are
        made part of stdc.library include the POSIX and the SAS/C ones.
        These string functions are most of the small and don't depend on other
        code. Doing it this way avoids having code that only uses non C99 string
        functions having a dependency on posixc.library.

    SEE ALSO
        --background_locale-- --background_c99--

*******************************************************************************/
