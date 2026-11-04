/*
 * VulkanProbe - checks the Vulkan stack on whatever devices it finds.
 *
 * It links -lvulkan: vulkan_loader.c gets its Vulkan object from the
 * display driver of the default public screen (CreateObject asked for the
 * hidd.vulkan class), or the software driver's when the display has none,
 * and bridges each vk* call to it. The probe opens no driver itself.
 *
 * For every physical device the instance enumerates it prints the device's
 * properties, queue families and memory types, creates a logical device
 * with a compute-capable queue, fills a host-visible buffer with
 * vkCmdFillBuffer, waits for the fence and checks the result through a
 * mapping. Output goes to the console and to the debug log.
 *
 * Result: 0 when every device passes, 5 when some do, 20 when none does
 * (or there is no instance or no device).
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <vulkan/vulkan.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX_DEVICES     16
#define MAX_FAMILIES    16
#define FILL_BYTES      4096u
#define FILL_PATTERN    0xA5C3F00Du
#define FENCE_TIMEOUT   (5ULL * 1000000000ULL)

/* Each line to the console and to the debug log */
static void plog(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    PutStr(buf);
    PutStr("\n");
    bug("[VulkanProbe] %s\n", buf);
}

static const char *device_type(VkPhysicalDeviceType t)
{
    switch (t)
    {
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:    return "integrated GPU";
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:      return "discrete GPU";
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:       return "virtual GPU";
    case VK_PHYSICAL_DEVICE_TYPE_CPU:               return "CPU";
    default:                                        return "other";
    }
}

/* The device-level entry points the loader does not export as symbols */
struct devfuncs
{
    PFN_vkCreateBuffer                  CreateBuffer;
    PFN_vkDestroyBuffer                 DestroyBuffer;
    PFN_vkGetBufferMemoryRequirements   GetBufferMemoryRequirements;
    PFN_vkAllocateMemory                AllocateMemory;
    PFN_vkFreeMemory                    FreeMemory;
    PFN_vkBindBufferMemory              BindBufferMemory;
    PFN_vkMapMemory                     MapMemory;
    PFN_vkUnmapMemory                   UnmapMemory;
    PFN_vkCreateCommandPool             CreateCommandPool;
    PFN_vkDestroyCommandPool            DestroyCommandPool;
    PFN_vkAllocateCommandBuffers        AllocateCommandBuffers;
    PFN_vkBeginCommandBuffer            BeginCommandBuffer;
    PFN_vkEndCommandBuffer              EndCommandBuffer;
    PFN_vkQueueSubmit                   QueueSubmit;
    PFN_vkCreateFence                   CreateFence;
    PFN_vkDestroyFence                  DestroyFence;
    PFN_vkWaitForFences                 WaitForFences;
};

static BOOL get_devfuncs(VkDevice device, struct devfuncs *f)
{
#define GET(name) \
    if (!(f->name = (PFN_vk##name)vkGetDeviceProcAddr(device, "vk" #name))) \
    { plog("    missing entry point vk" #name); return FALSE; }
    GET(CreateBuffer) GET(DestroyBuffer) GET(GetBufferMemoryRequirements)
    GET(AllocateMemory) GET(FreeMemory) GET(BindBufferMemory) GET(MapMemory) GET(UnmapMemory)
    GET(CreateCommandPool) GET(DestroyCommandPool) GET(AllocateCommandBuffers)
    GET(BeginCommandBuffer) GET(EndCommandBuffer) GET(QueueSubmit)
    GET(CreateFence) GET(DestroyFence) GET(WaitForFences)
#undef GET
    return TRUE;
}

/* A memory type the buffer may use that the CPU can map coherently */
static int host_memory_type(const VkPhysicalDeviceMemoryProperties *mp, uint32_t allowed)
{
    const VkMemoryPropertyFlags want = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    uint32_t i;

    for (i = 0; i < mp->memoryTypeCount; i++)
        if ((allowed & (1u << i)) && (mp->memoryTypes[i].propertyFlags & want) == want)
            return (int)i;
    return -1;
}

/* Fill a buffer on the queue and check it from the CPU */
static BOOL fill_test(VkPhysicalDevice pdev, VkDevice device, VkQueue queue, uint32_t family)
{
    struct devfuncs f;
    VkPhysicalDeviceMemoryProperties mp;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkMemoryRequirements req;
    uint32_t *map = NULL;
    BOOL ok = FALSE;
    VkResult r;
    int type;

    if (!get_devfuncs(device, &f))
        return FALSE;
    vkGetPhysicalDeviceMemoryProperties(pdev, &mp);

    {
        VkBufferCreateInfo bci = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = FILL_BYTES,
            .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        if ((r = f.CreateBuffer(device, &bci, NULL, &buffer)) != VK_SUCCESS)
        {
            plog("    vkCreateBuffer failed (%d)", (int)r);
            goto out;
        }
    }
    f.GetBufferMemoryRequirements(device, buffer, &req);
    if ((type = host_memory_type(&mp, req.memoryTypeBits)) < 0)
    {
        plog("    no host-visible coherent memory type for the buffer (types 0x%x)",
             (unsigned)req.memoryTypeBits);
        goto out;
    }
    {
        VkMemoryAllocateInfo mai = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = req.size,
            .memoryTypeIndex = (uint32_t)type,
        };
        if ((r = f.AllocateMemory(device, &mai, NULL, &memory)) != VK_SUCCESS ||
            (r = f.BindBufferMemory(device, buffer, memory, 0)) != VK_SUCCESS ||
            (r = f.MapMemory(device, memory, 0, FILL_BYTES, 0, (void **)&map)) != VK_SUCCESS)
        {
            plog("    buffer memory (type %d) failed (%d)", type, (int)r);
            goto out;
        }
    }
    memset(map, 0, FILL_BYTES);

    {
        VkCommandPoolCreateInfo pci = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = family,
        };
        VkCommandBufferAllocateInfo cai = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBufferBeginInfo bi = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        if ((r = f.CreateCommandPool(device, &pci, NULL, &pool)) != VK_SUCCESS)
        {
            plog("    vkCreateCommandPool failed (%d)", (int)r);
            goto out;
        }
        cai.commandPool = pool;
        if ((r = f.AllocateCommandBuffers(device, &cai, &cmd)) != VK_SUCCESS ||
            (r = f.BeginCommandBuffer(cmd, &bi)) != VK_SUCCESS)
        {
            plog("    command buffer failed (%d)", (int)r);
            goto out;
        }
        vkCmdFillBuffer(cmd, buffer, 0, FILL_BYTES, FILL_PATTERN);
        if ((r = f.EndCommandBuffer(cmd)) != VK_SUCCESS ||
            (r = f.CreateFence(device, &fci, NULL, &fence)) != VK_SUCCESS)
        {
            plog("    recording failed (%d)", (int)r);
            goto out;
        }
    }
    {
        VkSubmitInfo si = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &cmd,
        };
        if ((r = f.QueueSubmit(queue, 1, &si, fence)) != VK_SUCCESS)
        {
            plog("    vkQueueSubmit failed (%d)", (int)r);
            goto out;
        }
        if ((r = f.WaitForFences(device, 1, &fence, VK_TRUE, FENCE_TIMEOUT)) != VK_SUCCESS)
        {
            plog("    vkWaitForFences failed (%d)%s", (int)r, (r == VK_TIMEOUT) ? ": timed out" : "");
            goto out;
        }
    }
    {
        uint32_t i, bad = 0, first = 0;

        for (i = 0; i < FILL_BYTES / 4; i++)
            if (map[i] != FILL_PATTERN && bad++ == 0)
                first = i;
        if (bad)
            plog("    fill check: %u of %u words wrong (first at %u: 0x%08x)",
                 (unsigned)bad, FILL_BYTES / 4, (unsigned)first, (unsigned)map[first]);
        else
            plog("    fill check: %u words correct (memory type %d)", FILL_BYTES / 4, type);
        ok = (bad == 0);
    }

out:
    if (fence)
        f.DestroyFence(device, fence, NULL);
    if (pool)
        f.DestroyCommandPool(device, pool, NULL);
    if (map)
        f.UnmapMemory(device, memory);
    if (memory)
        f.FreeMemory(device, memory, NULL);
    if (buffer)
        f.DestroyBuffer(device, buffer, NULL);
    return ok;
}

/* Describe one physical device and run the compute check on it */
static BOOL probe_device(uint32_t index, VkPhysicalDevice pdev)
{
    VkPhysicalDeviceProperties p;
    VkPhysicalDeviceMemoryProperties mp;
    VkQueueFamilyProperties qprops[MAX_FAMILIES];
    uint32_t qCount = MAX_FAMILIES, qi, family = ~0u;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    float prio = 1.0f;
    BOOL ok;
    VkResult r;

    memset(&p, 0, sizeof(p));
    vkGetPhysicalDeviceProperties(pdev, &p);
    plog("device %u: '%s', %s, vendor 0x%04x device 0x%04x, Vulkan %u.%u.%u, driver 0x%08x",
         (unsigned)index, p.deviceName, device_type(p.deviceType),
         (unsigned)p.vendorID, (unsigned)p.deviceID,
         VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion),
         VK_VERSION_PATCH(p.apiVersion), (unsigned)p.driverVersion);

    vkGetPhysicalDeviceQueueFamilyProperties(pdev, &qCount, qprops);
    for (qi = 0; qi < qCount; qi++)
    {
        VkQueueFlags fl = qprops[qi].queueFlags;

        plog("  queue family %u: %u queue(s)%s%s%s", (unsigned)qi, (unsigned)qprops[qi].queueCount,
             (fl & VK_QUEUE_GRAPHICS_BIT) ? " graphics" : "",
             (fl & VK_QUEUE_COMPUTE_BIT) ? " compute" : "",
             (fl & VK_QUEUE_TRANSFER_BIT) ? " transfer" : "");
        if (family == ~0u && (fl & VK_QUEUE_COMPUTE_BIT))
            family = qi;
    }

    vkGetPhysicalDeviceMemoryProperties(pdev, &mp);
    for (qi = 0; qi < mp.memoryHeapCount; qi++)
        plog("  memory heap %u: %u MB%s", (unsigned)qi,
             (unsigned)(mp.memoryHeaps[qi].size >> 20),
             (mp.memoryHeaps[qi].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? " device-local" : "");

    if (family == ~0u)
    {
        plog("  FAIL: no compute-capable queue family");
        return FALSE;
    }

    {
        VkDeviceQueueCreateInfo qci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &prio,
        };
        VkDeviceCreateInfo dci = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &qci,
        };
        r = vkCreateDevice(pdev, &dci, NULL, &device);
    }
    if (r != VK_SUCCESS || device == VK_NULL_HANDLE)
    {
        plog("  FAIL: vkCreateDevice (%d)", (int)r);
        return FALSE;
    }
    vkGetDeviceQueue(device, family, 0, &queue);
    if (queue == VK_NULL_HANDLE)
    {
        plog("  FAIL: no queue from family %u", (unsigned)family);
        vkDestroyDevice(device, NULL);
        return FALSE;
    }
    plog("  device and queue (family %u) created; fill test:", (unsigned)family);

    ok = fill_test(pdev, device, queue, family);
    vkDeviceWaitIdle(device);
    vkDestroyDevice(device, NULL);
    plog("  %s", ok ? "PASS" : "FAIL");
    return ok;
}

int main(void)
{
    VkApplicationInfo app = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "VulkanProbe",
        .applicationVersion = 2,
        .apiVersion = VK_API_VERSION_1_2,
    };
    VkInstanceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
    };
    VkPhysicalDevice devs[MAX_DEVICES];
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t apiVersion = 0, count = MAX_DEVICES, i, passed = 0;
    VkResult r;

    r = vkEnumerateInstanceVersion(&apiVersion);
    plog("instance: Vulkan %u.%u.%u (%d)", VK_VERSION_MAJOR(apiVersion),
         VK_VERSION_MINOR(apiVersion), VK_VERSION_PATCH(apiVersion), (int)r);

    r = vkCreateInstance(&ci, NULL, &instance);
    if (r != VK_SUCCESS || instance == VK_NULL_HANDLE)
    {
        plog("FAIL: vkCreateInstance (%d): no Vulkan driver for this display, and no software one", (int)r);
        return 20;
    }

    r = vkEnumeratePhysicalDevices(instance, &count, devs);
    if (r != VK_SUCCESS && r != VK_INCOMPLETE)
    {
        plog("FAIL: vkEnumeratePhysicalDevices (%d)", (int)r);
        vkDestroyInstance(instance, NULL);
        return 20;
    }
    plog("%u physical device(s)%s", (unsigned)count, (r == VK_INCOMPLETE) ? " (more not listed)" : "");

    for (i = 0; i < count; i++)
        if (probe_device(i, devs[i]))
            passed++;

    vkDestroyInstance(instance, NULL);
    plog("result: %u of %u device(s) passed", (unsigned)passed, (unsigned)count);

    if (count && passed == count)
        return 0;
    return passed ? 5 : 20;
}
