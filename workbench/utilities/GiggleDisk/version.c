
/*
** version.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#include "version.h"

/*************************************************************************/

__entry const char VERSION_LABEL[] = "$VER: " APPLICATIONNAME " " I2S(VERSION) "." I2S(REVISION) " (" I2S(DAY) "." I2S(MONTH) "." I2S(YEAR) ") © " AUTHORNAME " " VERSION_EXT;



/*
** just some declarations to make code smaller
*/

char          app_appname[]        = APPLICATIONNAME;

