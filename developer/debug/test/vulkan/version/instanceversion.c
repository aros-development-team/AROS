/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    The loader must give the driver the application's instance create info. A
    driver hides the functions of any version the instance did not ask for, so
    an instance at Vulkan 1.3 must find functions that are new in 1.3.
*/

#include <exec/types.h>
#include <dos/dos.h>

#include <vulkan/vulkan.h>

#include <stdio.h>

/* The driver compiles shaders on this task's stack */
ULONG __stack = 1024 * 1024;

int main(void)
{
    static const char *const names[] = { "vkCmdBeginRendering", "vkCmdPipelineBarrier2", "vkQueueSubmit2" };
    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "instanceversion",
        .apiVersion = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
    };
    float priority = 1.0f;
    VkDeviceQueueCreateInfo qci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .pQueuePriorities = &priority,
    };
    VkDeviceCreateInfo dci = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &qci,
    };
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties props;
    VkDevice device = VK_NULL_HANDLE;
    uint32_t count = 1;
    int missing = 0, i;

    if (vkCreateInstance(&ici, NULL, &instance) != VK_SUCCESS ||
        (vkEnumeratePhysicalDevices(instance, &count, &physical), count == 0) || !physical)
    {
        printf("instanceversion: no Vulkan device FAIL\n");
        return RETURN_FAIL;
    }
    vkGetPhysicalDeviceProperties(physical, &props);
    if (props.apiVersion < VK_API_VERSION_1_3)
    {
        printf("instanceversion: %s offers below Vulkan 1.3 SKIP\n", props.deviceName);
        vkDestroyInstance(instance, NULL);
        return RETURN_OK;
    }
    if (vkCreateDevice(physical, &dci, NULL, &device) != VK_SUCCESS)
    {
        printf("instanceversion: no logical device FAIL\n");
        vkDestroyInstance(instance, NULL);
        return RETURN_FAIL;
    }
    for (i = 0; i < 3; i++)
    {
        if (!vkGetDeviceProcAddr(device, names[i]))
        {
            printf("instanceversion: %s missing\n", names[i]);
            missing++;
        }
    }
    vkDestroyDevice(device, NULL);
    vkDestroyInstance(instance, NULL);
    printf("instanceversion: device=%s missing=%d %s\n", props.deviceName, missing, missing ? "FAIL" : "PASS");
    return missing ? RETURN_FAIL : RETURN_OK;
}
