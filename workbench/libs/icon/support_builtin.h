#ifndef _SUPPORT_BUILTIN_H_
#define _SUPPORT_BUILTIN_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    Headers for the builtin icon images and support functions.
*/

/*** Prototypes *************************************************************/
struct DiskObject *__GetBuiltinIcon_WB(LONG type, struct IconBase *IconBase);

/*** Macros *****************************************************************/
#define GetBuiltinIcon(type) (__GetBuiltinIcon_WB((type), LB(IconBase)))

#endif /* _SUPPORT_BUILTIN_H_ */
