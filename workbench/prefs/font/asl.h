#ifndef _ASL_H
#define _ASL_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>

/*** Modes ******************************************************************/
enum
{
    ASL_MODE_IMPORT,
    ASL_MODE_EXPORT
};

/*** Prototypes *************************************************************/
STRPTR ASL_SelectFile();

#endif /* _ASL_H */
