/* SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2025, NVIDIA CORPORATION. All rights reserved.
 */
#include <rm/rm.h>

#include "nvrm/gsp.h"

static const struct nvkm_rm_wpr
r580_wpr_libos2 = {
	.os_carveout_size = GSP_FW_HEAP_PARAM_OS_SIZE_LIBOS2,
	.base_size = GSP_FW_HEAP_PARAM_BASE_RM_SIZE_TU10X,
	.heap_size_min = GSP_FW_HEAP_SIZE_OVERRIDE_LIBOS2_MIN_MB,
};

static const struct nvkm_rm_wpr
r580_wpr_libos3 = {
	.os_carveout_size = GSP_FW_HEAP_PARAM_OS_SIZE_LIBOS3_BAREMETAL,
	.base_size = GSP_FW_HEAP_PARAM_BASE_RM_SIZE_TU10X,
	.heap_size_min = GSP_FW_HEAP_SIZE_OVERRIDE_LIBOS3_BAREMETAL_MIN_MB,
};

static const struct nvkm_rm_wpr
r580_wpr_libos3_gh100 = {
	.os_carveout_size = GSP_FW_HEAP_PARAM_OS_SIZE_LIBOS3_BAREMETAL,
	.base_size = GSP_FW_HEAP_PARAM_BASE_RM_SIZE_GH100,
	.heap_size_min = GSP_FW_HEAP_SIZE_OVERRIDE_LIBOS3_BAREMETAL_MIN_MB,
	.heap_size_non_wpr = 0x200000,
	.offset_set_by_acr = true,
};

static const struct nvkm_rm_wpr
r580_wpr_libos3_gb10x = {
	.os_carveout_size = GSP_FW_HEAP_PARAM_OS_SIZE_LIBOS3_BAREMETAL,
	.base_size = GSP_FW_HEAP_PARAM_BASE_RM_SIZE_GH100,
	.heap_size_min = GSP_FW_HEAP_SIZE_OVERRIDE_LIBOS3_BAREMETAL_MIN_MB,
	.heap_size_non_wpr = 0x200000,
	.rsvd_size_pmu = ALIGN(0x0800000 + 0x1000000 + 0x0001000, 0x20000),
	.offset_set_by_acr = true,
};

static const struct nvkm_rm_wpr
r580_wpr_libos3_gb20x = {
	.os_carveout_size = GSP_FW_HEAP_PARAM_OS_SIZE_LIBOS3_BAREMETAL,
	.base_size = GSP_FW_HEAP_PARAM_BASE_RM_SIZE_GH100,
	.heap_size_min = GSP_FW_HEAP_SIZE_OVERRIDE_LIBOS3_BAREMETAL_MIN_MB,
	.heap_size_non_wpr = 0x220000,
	.rsvd_size_pmu = ALIGN(0x0800000 + 0x1000000 + 0x0001000, 0x20000),
	.offset_set_by_acr = true,
};

static const struct nvkm_rm_api
r580_api = {
	.gsp = &r580_gsp,
	.rpc = &r535_rpc,
	.ctrl = &r580_ctrl,
	.alloc = &r535_alloc,
	.client = &r580_client,
	.device = &r535_device,
	.fbsr = &r580_fbsr,
	.disp = &r580_disp,
	.fifo = &r580_fifo,
	.ce = &r535_ce,
	.gr = &r580_gr,
	.nvdec = &r535_nvdec,
	.nvenc = &r535_nvenc,
	.nvjpg = &r535_nvjpg,
	.ofa = &r580_ofa,
	.vmm = &r580_vmm,
};

const struct nvkm_rm_impl
r580_rm_tu102 = {
	.wpr = &r580_wpr_libos2,
	.api = &r580_api,
};

const struct nvkm_rm_impl
r580_rm_ga102 = {
	.wpr = &r580_wpr_libos3,
	.api = &r580_api,
};

const struct nvkm_rm_impl
r580_rm_gh100 = {
	.wpr = &r580_wpr_libos3_gh100,
	.api = &r580_api,
};

const struct nvkm_rm_impl
r580_rm_gb10x = {
	.wpr = &r580_wpr_libos3_gb10x,
	.api = &r580_api,
};

const struct nvkm_rm_impl
r580_rm_gb20x = {
	.wpr = &r580_wpr_libos3_gb20x,
	.api = &r580_api,
};
