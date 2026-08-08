/*
    Copyright (C) 2010-2019, The AROS Development Team. All rights reserved.
*/

#ifndef GALLIUM_GALLIUM_H
#define GALLIUM_GALLIUM_H

#define GALLIUM_INTERFACE_VERSION   5

/* Tags for CreatePipeScreen() function */
#define CPS_Dummy                       (TAG_USER)
#define CPS_GalliumInterfaceVersion     (CPS_Dummy + 1)
#define CPS_PipeFriendBitMap            (CPS_Dummy + 2)
#define CPS_PipeScreenDriver            (CPS_Dummy + 3)
/* mesa3dgl's GalliumCoreAPI table (struct GalliumCoreAPI *). Forwarded to
 * the pipe hidd as aHidd_Gallium_CoreAPI so a runtime-bound pipe driver
 * can call the one Mesa compiler core. 0 when the build carries no
 * table; drivers that need it refuse CreatePipeScreen. */
#define CPS_GalliumCoreAPI              (CPS_Dummy + 4)

/* A special version of CreatePipe function with version embeded in call */
#define CreatePipeV(tags)                                               \
    ({                                                                  \
        struct TagItem cpsvtags [] =                                    \
        {                                                               \
            { CPS_GalliumInterfaceVersion, GALLIUM_INTERFACE_VERSION }, \
            { TAG_MORE, (IPTR)tags }                                    \
        };                                                              \
        CreatePipe(cpsvtags);                                           \
    })

typedef APTR PipeHandle_t;
typedef APTR PipeScreen_t;

#endif
