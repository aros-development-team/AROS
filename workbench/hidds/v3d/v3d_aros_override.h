/*
 * v3d_ioctl override for AROS
 *
 * This header is force-included (-include) before all Mesa V3D source files.
 * It redefines v3d_ioctl to route through our DRM shim instead of drmIoctl.
 */
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    V3D Mesa AROS DRM Override
*/

#ifndef V3D_AROS_OVERRIDE_H
#define V3D_AROS_OVERRIDE_H

/* Forward declaration of our shim */
struct V3DData;
extern struct V3DData *g_v3d_data;
extern int v3d_ioctl_aros(struct V3DData *sd, unsigned long request, void *arg);

/* Override the inline v3d_ioctl that Mesa defines in v3d_context.h */
#define drmIoctl(fd, request, arg) v3d_ioctl_aros(g_v3d_data, request, arg)

/* Suppress the simulator path */
#define using_v3d_simulator 0

#endif /* V3D_AROS_OVERRIDE_H */
