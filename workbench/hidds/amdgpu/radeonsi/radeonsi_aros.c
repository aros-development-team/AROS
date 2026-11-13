/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: Parts of radeonsi's Linux build that are not carried: the ELF
          loader for LLVM-built shaders (ACO emits raw binaries), the VPE
          video processor (needs vpelib) and the winsys for the old radeon
          kernel driver (the device is always driven by amdgpu).
*/

#include "si_pipe.h"
#include "si_vpe.h"
#include "ac_rtld.h"
#include "winsys/radeon_winsys.h"

bool ac_rtld_open(struct ac_rtld_binary *binary, struct ac_rtld_open_info i)
{
    return false;
}

void ac_rtld_close(struct ac_rtld_binary *binary)
{
}

bool ac_rtld_get_section_by_name(struct ac_rtld_binary *binary, const char *name, const char **data,
                                 size_t *nbytes)
{
    return false;
}

int ac_rtld_upload(struct ac_rtld_upload_info *u)
{
    return -1;
}

struct pipe_video_codec *si_vpe_create_processor(struct pipe_context *context,
                                                 const struct pipe_video_codec *templ)
{
    return NULL;
}

struct radeon_winsys *radeon_drm_winsys_create(int fd, const struct pipe_screen_config *config,
                                               radeon_screen_create_t screen_create)
{
    return NULL;
}
