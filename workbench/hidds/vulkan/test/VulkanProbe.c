/*
 * VulkanProbe — ground-truth trigger for the radv (Vulkan) path on real silicon.
 *
 * Analogous to C:AmdgpuProbe (which validated the amdgpu.hidd foundation), this
 * drives the Vulkan client surface end to end and logs every stage to serial, so
 * a boot answers the open question: does the real radv (Mesa 26 libradv) reach
 * the RX 9060 XT (1002:7590) through vulkan.hidd + aros_radv_winsys, create a
 * logical device and a COMPUTE queue on it? That is the prerequisite for the
 * compute path (llama.cpp / Stable Diffusion ggml-vulkan) and for Zink GL.
 *
 * It links -lvulkan: vulkan_loader.c bridges each standard vk* call to the
 * vulkan.hidd OOP method, which dispatches to whichever driver registered a GPU
 * (radeonsi.hidd registers the 9060 at boot). radv itself is otherwise lazy —
 * nothing exercises it on a normal boot, so without this probe its state on the
 * 9060 is simply unknown.
 *
 * v1 stops at device + compute-queue creation (the natural first milestone). A
 * full compute dispatch is a follow-up probe once this path is green.
 */
#include <proto/exec.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include <vulkan/vulkan.h>
#include <string.h>

/* Log only to serial via bug() — that's the channel we capture, and it avoids a
 * posixc/stdcio dependency the vulkan mmakefile's -static prog link doesn't pull. */
#define PLOG(...) do { bug("[VulkanProbe] " __VA_ARGS__); bug("\n"); } while (0)

#define DUT_VENDOR_AMD   0x1002u
#define DUT_DEVICE_9060  0x7590u

int main(void)
{
    VkResult r;
    uint32_t apiVersion = 0;
    VkInstance instance = VK_NULL_HANDLE;
    uint32_t devCount = 0;
    VkPhysicalDevice devs[8];
    int amdIndex = -1;

    PLOG("start — driving the Vulkan client surface to ground-truth radv on the 9060");

    /* Stage 0: load the driver so it registers its GPU + radv callbacks with
     * vulkan.hidd. radeonsi.hidd's ADD2INITLIB runs radeonsi_scan_all_gpus() ->
     * VulkanHidd_RegisterGPU(). Without a registered GPU, vulkan.hidd's
     * CreateInstance has nothing to dispatch to and returns -3 (the v1 result).
     * Mirrors how AmdgpuProbe opens amdgpu.hidd to fire its init ladder. */
    {
        struct Library *radeonsiBase = OpenLibrary((CONST_STRPTR)"radeonsi.hidd", 0);
        PLOG("OpenLibrary(radeonsi.hidd) -> %p %s", (void *)radeonsiBase,
             radeonsiBase ? "(GPU registration ran — see [Radeonsi]/RegisterGPU lines)"
                          : "(FAILED — driver not in image or its init refused)");
        /* keep it open for the life of the probe so the registration stays live */
    }

    /* Stage 1: instance API version (does the loader/HIDD answer at all). */
    r = vkEnumerateInstanceVersion(&apiVersion);
    PLOG("vkEnumerateInstanceVersion -> r=%d api=%u.%u.%u", (int)r,
         VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion),
         VK_VERSION_PATCH(apiVersion));

    /* Stage 2: create the instance (radv_CreateInstance behind the HIDD). */
    {
        VkApplicationInfo app;
        VkInstanceCreateInfo ci;
        memset(&app, 0, sizeof(app));
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "VulkanProbe";
        app.applicationVersion = 1;
        app.pEngineName = "none";
        app.apiVersion = VK_API_VERSION_1_2;
        memset(&ci, 0, sizeof(ci));
        ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        ci.pApplicationInfo = &app;

        r = vkCreateInstance(&ci, NULL, &instance);
        PLOG("vkCreateInstance -> r=%d instance=%p", (int)r, (void *)instance);
        if (r != VK_SUCCESS || instance == VK_NULL_HANDLE) {
            PLOG("FAIL: no instance — radv_CreateInstance did not return one. STOP.");
            return 20;
        }
    }

    /* Stage 3: enumerate physical devices — does radv find the 9060? This is the
     * call that fires aros_radv_winsys_create (its own MMIO/PSP/topology probe). */
    devCount = 8;
    memset(devs, 0, sizeof(devs));
    r = vkEnumeratePhysicalDevices(instance, &devCount, devs);
    PLOG("vkEnumeratePhysicalDevices -> r=%d count=%u", (int)r, devCount);
    if (r != VK_SUCCESS && r != VK_INCOMPLETE) {
        PLOG("FAIL: enumeration error — winsys/physdev path died. STOP.");
        vkDestroyInstance(instance, NULL);
        return 20;
    }
    if (devCount == 0) {
        PLOG("FAIL: 0 physical devices — radv reached no GPU (winsys_create likely "
             "failed opening the 9060). STOP.");
        vkDestroyInstance(instance, NULL);
        return 20;
    }

    /* Stage 4: identify each device; find the 9060. */
    {
        uint32_t i;
        for (i = 0; i < devCount && i < 8; i++) {
            VkPhysicalDeviceProperties p;
            memset(&p, 0, sizeof(p));
            vkGetPhysicalDeviceProperties(devs[i], &p);
            PLOG("  physdev[%u]: '%s' vendor=0x%04x device=0x%04x type=%d api=%u.%u.%u",
                 i, p.deviceName, p.vendorID, p.deviceID, (int)p.deviceType,
                 VK_VERSION_MAJOR(p.apiVersion), VK_VERSION_MINOR(p.apiVersion),
                 VK_VERSION_PATCH(p.apiVersion));
            if (p.vendorID == DUT_VENDOR_AMD && p.deviceID == DUT_DEVICE_9060)
                amdIndex = (int)i;
        }
    }
    if (amdIndex < 0) {
        PLOG("NOTE: the 9060 XT (1002:7590) was not among the enumerated devices; "
             "using physdev[0] for the device-creation attempt.");
        amdIndex = 0;
    } else {
        PLOG("*** radv ENUMERATED the RX 9060 XT as physdev[%d] ***", amdIndex);
    }

    /* Stage 5: find a compute-capable queue family. */
    {
        uint32_t qCount = 0, qi, computeFamily = 0xFFFFFFFFu;
        VkQueueFamilyProperties qprops[16];

        vkGetPhysicalDeviceQueueFamilyProperties(devs[amdIndex], &qCount, NULL);
        PLOG("queue families: %u", qCount);
        if (qCount > 16) qCount = 16;
        vkGetPhysicalDeviceQueueFamilyProperties(devs[amdIndex], &qCount, qprops);
        for (qi = 0; qi < qCount; qi++) {
            PLOG("  qfamily[%u]: flags=0x%x count=%u%s", qi,
                 qprops[qi].queueFlags, qprops[qi].queueCount,
                 (qprops[qi].queueFlags & VK_QUEUE_COMPUTE_BIT) ? " [COMPUTE]" : "");
            if ((computeFamily == 0xFFFFFFFFu) &&
                (qprops[qi].queueFlags & VK_QUEUE_COMPUTE_BIT))
                computeFamily = qi;
        }
        if (computeFamily == 0xFFFFFFFFu) {
            PLOG("FAIL: no compute-capable queue family. STOP.");
            vkDestroyInstance(instance, NULL);
            return 20;
        }
        PLOG("compute queue family = %u", computeFamily);

        /* Stage 6: create a logical device + fetch the compute queue. This is the
         * milestone: radv building a real device on the 9060. */
        {
            float prio = 1.0f;
            VkDeviceQueueCreateInfo qci;
            VkDeviceCreateInfo dci;
            VkDevice device = VK_NULL_HANDLE;
            VkQueue queue = VK_NULL_HANDLE;

            memset(&qci, 0, sizeof(qci));
            qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qci.queueFamilyIndex = computeFamily;
            qci.queueCount = 1;
            qci.pQueuePriorities = &prio;

            memset(&dci, 0, sizeof(dci));
            dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            dci.queueCreateInfoCount = 1;
            dci.pQueueCreateInfos = &qci;

            r = vkCreateDevice(devs[amdIndex], &dci, NULL, &device);
            PLOG("vkCreateDevice -> r=%d device=%p", (int)r, (void *)device);
            if (r != VK_SUCCESS || device == VK_NULL_HANDLE) {
                PLOG("FAIL: no logical device — radv device init on the 9060 failed. STOP.");
                vkDestroyInstance(instance, NULL);
                return 20;
            }

            vkGetDeviceQueue(device, computeFamily, 0, &queue);
            PLOG("vkGetDeviceQueue -> queue=%p", (void *)queue);
            if (queue == VK_NULL_HANDLE) {
                PLOG("FAIL: no compute queue handle. STOP.");
                vkDestroyDevice(device, NULL);
                vkDestroyInstance(instance, NULL);
                return 20;
            }

            PLOG("*** SUCCESS: radv created a logical DEVICE + COMPUTE QUEUE on the "
                 "9060 — the compute path is reachable (next: a dispatch) ***");
            vkDestroyDevice(device, NULL);
        }
    }

    vkDestroyInstance(instance, NULL);
    PLOG("done");
    return 0;
}
