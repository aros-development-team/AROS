/*
    Copyright 2026, The AROS Development Team. All rights reserved.
*/

/* Virtual display is not provided; the block registers and does nothing. */

#include "amdgpu.h"
#include "amdgpu_vkms.h"

static int amdgpu_vkms_aros_nop(struct amdgpu_ip_block *ip_block)
{
    return 0;
}

static const struct amd_ip_funcs amdgpu_vkms_ip_funcs = {
    .name = "amdgpu_vkms",
    .sw_init = amdgpu_vkms_aros_nop,
    .sw_fini = amdgpu_vkms_aros_nop,
    .hw_init = amdgpu_vkms_aros_nop,
    .hw_fini = amdgpu_vkms_aros_nop,
    .suspend = amdgpu_vkms_aros_nop,
    .resume = amdgpu_vkms_aros_nop,
};

const struct amdgpu_ip_block_version amdgpu_vkms_ip_block = {
    .type = AMD_IP_BLOCK_TYPE_DCE,
    .major = 1,
    .minor = 0,
    .rev = 0,
    .funcs = &amdgpu_vkms_ip_funcs,
};
