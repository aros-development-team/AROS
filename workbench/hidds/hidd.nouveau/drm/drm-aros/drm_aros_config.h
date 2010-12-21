/*
    Copyright 2009, The AROS Development Team. All rights reserved.
    $Id$
*/

#if !defined(DRM_AROS_CONFIG_H)
#define DRM_AROS_CONFIG_H

/* Enable changes for running under hosted AROS */
/* Procedure: C:LoadResource DEVS:Drivers/nouveau.hidd */
//#define HOSTED_BUILD

#if defined(HOSTED_BUILD)
/* Which type of hardware */
#define HOSTED_BUILD_HARDWARE_NVIDIA    1

#define HOSTED_BUILD_HARDWARE           HOSTED_BUILD_HARDWARE_NVIDIA 

/* Which type of bus */
#define HOSTED_BUILD_BUS_PCI            1
#define HOSTED_BUILD_BUS_AGP            2

#define HOSTED_BUILD_BUS                HOSTED_BUILD_BUS_PCI

/* nVidia defines */
/* CAN BE AGP OR PCI */
//#define HOSTED_BUILD_CHIPSET    5       /* NV05 chip Riva TNT 2 */
//#define HOSTED_BUILD_CHIPSET    16      /* NV10 chip GeForce 256 */
//#define HOSTED_BUILD_CHIPSET    21      /* NV15 chip GeForce 2 GTS */
//#define HOSTED_BUILD_CHIPSET    32      /* NV20 chip GeForce 3 Ti 200 */
//#define HOSTED_BUILD_CHIPSET    37      /* NV25 chip GeForce Ti 4200 */
//#define HOSTED_BUILD_CHIPSET    52      /* NV34 chip GeForce FX 5200 */
#define HOSTED_BUILD_CHIPSET    67      /* NV43 chip GeForce 6200 */
/* MUST BE PCI */
//#define HOSTED_BUILD_CHIPSET    132     /* G84 chip GeForce 8600 GT */ 
//#define HOSTED_BUILD_CHIPSET    134     /* G86 chip GeForce 8400 GS */
//#define HOSTED_BUILD_CHIPSET    163     /* GT215 chip GeForce GT 240 */
//#define HOSTED_BUILD_CHIPSET    175     /* MCP89 chip GeForce 320M */
//#define HOSTED_BUILD_CHIPSET    192     /* GT100 chip GeForce 470 GTX */

#endif

/* Config */
#define CONFIG_AGP
#define __OS_HAS_AGP    1

#endif /* DRM_AROS_CONFIG_H */
