/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    bcm_host for MMAL clients: nothing to set up on AROS, vchiq.library
    connects on first use.
*/

#ifndef BCM_HOST_H
#define BCM_HOST_H

void bcm_host_init(void);
void bcm_host_deinit(void);

#endif /* BCM_HOST_H */
