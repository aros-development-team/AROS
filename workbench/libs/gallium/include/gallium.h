/*
    Copyright 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef GALLIUM_GALLIUM_H
#define GALLIUM_GALLIUM_H

#ifndef P_AROS_VERSION_H
#   include <gallium/pipe/p_aros_version.h>
#endif

/* Tags for CreatePipeScreen() function */
#define CPS_Dummy                   (TAG_USER)
#define CPS_GalliumInterfaceVersion (CPS_Dummy + 1)

/* A special version of CreatePipeScreen function with version embeded in call */
#define CreatePipeScreenV(tags)                                         \
    ({                                                                  \
        struct TagItem cpsvtags [] =                                    \
        {                                                               \
            { CPS_GalliumInterfaceVersion, GALLIUM_INTERFACE_VERSION }, \
            { TAG_MORE, (IPTR)tags }                                    \
        };                                                              \
        CreatePipeScreen(cpsvtags);                                     \
    })

#endif
