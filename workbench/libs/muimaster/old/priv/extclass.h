/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#ifndef __EXTCLASS_H__
#define __EXTCLASS_H__

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

/* About loading Zune classes in external files ...
 */

#ifndef __AROS__
typedef ULONG (*QueryFunc) (LONG which);
#endif

Class *_zune_class_load(CONST_STRPTR className);



#endif
