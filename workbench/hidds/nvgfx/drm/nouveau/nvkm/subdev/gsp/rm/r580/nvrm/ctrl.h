/* SPDX-License-Identifier: MIT */

/* Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved. */

#ifndef __NVRM_CTRL_H__
#define __NVRM_CTRL_H__
#include <nvrm/nvtypes.h>

/* Excerpt of RM headers from https://github.com/NVIDIA/open-gpu-kernel-modules/tree/580.178.04 */

typedef struct rpc_gsp_rm_control_v03_00
{
    NvHandle   hClient;
    NvHandle   hObject;
    NvU32      cmd;
    NvU32      status;
    NvU32      paramsSize;
    NvU32      rmapiRpcFlags;
    NvU32      rmctrlFlags;
    NvU32      rmctrlAccessRight;
    NvU64      reserved0 NV_ALIGN_BYTES(8);
    NvU8       params[];
} rpc_gsp_rm_control_v03_00;

#define RMAPI_RPC_FLAGS_NONE                 0x00000000
#define RMAPI_RPC_FLAGS_COPYOUT_ON_ERROR     0x00000001
#define RMAPI_RPC_FLAGS_SERIALIZED           0x00000002
#endif
