/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>

/* We use a negative version number because this way the library autoopening code
   knows that we don't want errors to be reported if this library cannot be opened.
   
   Given v is the version number provided, the real version number this library needs
   to have is -(v + 1). The additional unit is needed so that even libraries with
   version number 0 can be opened without the autoopening code reporting errors.  */
ADD2LIBS("locale.library", -40, void *, LocaleBase);
