#ifndef INTUITION_ICCLASS_H
#define INTUITION_ICCLASS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: icclass defines
    Lang: english
*/

#ifndef UTILITY_TAGITEM_H
#   include <utility/tagitem.h>
#endif

#define ICM_Dummy     0x00000401
#define ICM_SETLOOP   (ICM_Dummy + 1)
#define ICM_CLEARLOOP (ICM_Dummy + 2)
#define ICM_CHECKLOOP (ICM_Dummy + 3)

#define ICA_Dummy      (TAG_USER + 0x00040000)
#define ICA_TARGET     (ICA_Dummy + 1)
#define ICA_MAP        (ICA_Dummy + 2)
#define ICSPECIAL_CODE (ICA_Dummy + 3)

#define ICTARGET_IDCMP (~0L)

#endif /* INTUITION_ICCLASS_H */
