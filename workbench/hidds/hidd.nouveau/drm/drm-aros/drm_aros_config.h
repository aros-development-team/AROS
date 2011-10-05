/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#if !defined(DRM_AROS_CONFIG_H)
#define DRM_AROS_CONFIG_H

/*  Define this flag to enable code changes for running nouveau driver under
    linux-hosted AROS with pcimock.hidd. THIS IS PURELLY FOR DEBUG PURPOSES */
/* #define MOCK_HARDWARE */

/* Config */
#define CONFIG_AGP
#define __OS_HAS_AGP    1

#endif /* DRM_AROS_CONFIG_H */
