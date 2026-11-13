#ifndef _MUI_IDENTIFIERS_H
#define _MUI_IDENTIFIERS_H

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>

#define MUIB_MUI  (TAG_USER)                /* Base for legacy MUI identifiers   */
#define MUIB_RSVD (MUIB_MUI  | 0x10400000)  /* Base for AROS reserved range      */
#define MUIB_ZUNE (MUIB_RSVD | 0x00020000)  /* Base for Zune core reserved range */
#define MUIB_AROS (MUIB_RSVD | 0x00070000)  /* Base for AROS core reserved range */

#endif /* _MUI_IDENTIFIERS_H */
