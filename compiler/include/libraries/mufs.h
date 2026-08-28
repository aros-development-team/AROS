#ifndef LIBRARIES_MUFS_H
#define LIBRARIES_MUFS_H

/*
    Copyright (C) 2019-2026, The AROS Development Team. All rights reserved.

    Desc: DEPRECATED compatibility header. Everything now lives in
          <libraries/security.h>; this file only provides the historical
          multiuser.library names for sources that still include it.
*/

#include <libraries/security.h>

#define MULTIUSERNAME           "multiuser.library"
#define MULTIUSERVERSION        (40)

#define MULTIUSERCATALOGNAME    "multiuser.catalog"
#define MULTIUSERCATALOGVERSION (1)

#endif /* LIBRARIES_MUFS_H */
