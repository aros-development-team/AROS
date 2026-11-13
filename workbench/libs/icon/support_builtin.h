#ifndef _SUPPORT_BUILTIN_H_
#define _SUPPORT_BUILTIN_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$

    Headers for the builtin icon images and support functions.
*/

/*** Prototypes *************************************************************/
BOOL __GetBuiltinImage_WB(struct Image *image, LONG type, BOOL selected, struct IconBase *IconBase);
struct DiskObject *__GetBuiltinIcon_WB(LONG type, struct IconBase *IconBase);

/*** Macros *****************************************************************/
#define GetBuiltinImage(image, type, selected) (__GetBuiltinImage_WB((image), (type), (selected), LB(IconBase)))
#define GetBuiltinIcon(type) (__GetBuiltinIcon_WB((type), LB(IconBase)))

#endif /* _SUPPORT_BUILTIN_H_ */
