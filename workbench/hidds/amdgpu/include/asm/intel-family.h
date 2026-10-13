/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/
#ifndef _ASM_INTEL_FAMILY_H_
#define _ASM_INTEL_FAMILY_H_

#define IFM(family, model)      ((((family) & 0xff) << 8) | ((model) & 0xff))

#define INTEL_ALDERLAKE         IFM(6, 0x97)
#define INTEL_ALDERLAKE_L       IFM(6, 0x9A)
#define INTEL_RAPTORLAKE        IFM(6, 0xB7)
#define INTEL_RAPTORLAKE_P      IFM(6, 0xBA)
#define INTEL_RAPTORLAKE_S      IFM(6, 0xBF)

#endif
