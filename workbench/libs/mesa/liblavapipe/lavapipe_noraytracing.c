/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The acceleration structure build in Mesa's Vulkan runtime
    (vk_acceleration_structure.c) and its radix sort run compute shaders
    that meson compiles from GLSL with glslang at build time, which this
    build has no way to do. lavapipe links the build entry points anyway;
    with LVP_NO_RAY_TRACING it advertises none of the ray tracing
    extensions, so these are never reached.
*/

#include "vk_acceleration_structure.h"
#include "radix_sort/radix_sort_u64.h"

radix_sort_vk_t *
vk_create_radix_sort_u64(VkDevice device, VkAllocationCallbacks const *ac,
                         VkPipelineCache pc,
                         struct radix_sort_vk_target_config config)
{
   return NULL;
}

void
vk_get_as_build_sizes(VkDevice _device, VkAccelerationStructureBuildTypeKHR buildType,
                      const VkAccelerationStructureBuildGeometryInfoKHR *pBuildInfo,
                      const uint32_t *pMaxPrimitiveCounts,
                      VkAccelerationStructureBuildSizesInfoKHR *pSizeInfo,
                      const struct vk_acceleration_structure_build_args *args)
{
   pSizeInfo->accelerationStructureSize = 0;
   pSizeInfo->updateScratchSize = 0;
   pSizeInfo->buildScratchSize = 0;
}

void
vk_cmd_build_acceleration_structures(VkCommandBuffer commandBuffer,
                                     struct vk_device *device,
                                     struct vk_meta_device *meta,
                                     uint32_t infoCount,
                                     const VkAccelerationStructureBuildGeometryInfoKHR *pInfos,
                                     const VkAccelerationStructureBuildRangeInfoKHR *const *ppBuildRangeInfos,
                                     const struct vk_acceleration_structure_build_args *args)
{
}

bool
vk_acceleration_struct_vtx_format_supported(VkFormat format)
{
   return false;
}

void
radix_sort_vk_destroy(struct radix_sort_vk *rs, VkDevice d,
                      VkAllocationCallbacks const *const ac)
{
}
