/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Making and destroying a VkDevice must give back all its memory. The first
    cycles fill the driver's caches; the measured ones must not lose a byte.

    AvailMem() counts every task's memory, so one measurement can show a loss
    that is not the driver's. A leak loses memory in every round; the test
    fails only if the round that lost least still lost some.
*/

#include <exec/memory.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include <vulkan/vulkan.h>

#include <stdio.h>

#define WARM_CYCLES     2
#define MEASURED_CYCLES 3
#define ROUNDS          3

/* The driver compiles shaders on this task's stack */
ULONG __stack = 1024 * 1024;

static BOOL cycle(void)
{
    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "devicememory",
        .apiVersion = VK_API_VERSION_1_2,
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
    VkDevice device = VK_NULL_HANDLE;
    uint32_t count = 1;
    BOOL ok = FALSE;

    if (vkCreateInstance(&ici, NULL, &instance) != VK_SUCCESS)
        return FALSE;
    vkEnumeratePhysicalDevices(instance, &count, &physical);
    if (count && physical && vkCreateDevice(physical, &dci, NULL, &device) == VK_SUCCESS)
    {
        vkDestroyDevice(device, NULL);
        ok = TRUE;
    }
    vkDestroyInstance(instance, NULL);
    return ok;
}

int main(void)
{
    IPTR before;
    SIPTR lost = 0, least = 0;
    int round, i;

    for (i = 0; i < WARM_CYCLES; i++)
    {
        if (!cycle())
        {
            printf("devicememory: no Vulkan device FAIL\n");
            return RETURN_FAIL;
        }
    }

    for (round = 0; round < ROUNDS; round++)
    {
        before = AvailMem(MEMF_ANY);
        for (i = 0; i < MEASURED_CYCLES; i++)
        {
            if (!cycle())
            {
                printf("devicememory: no Vulkan device FAIL\n");
                return RETURN_FAIL;
            }
        }
        lost = (SIPTR)(before - AvailMem(MEMF_ANY));
        if (round == 0 || lost < least)
            least = lost;
    }

    printf("devicememory: lost=%ld over %d cycles, least of %d rounds %s\n",
        (long)least, MEASURED_CYCLES, ROUNDS, least > 0 ? "FAIL" : "PASS");
    return least > 0 ? RETURN_FAIL : RETURN_OK;
}
