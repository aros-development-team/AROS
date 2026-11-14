/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vkwsi - vulkan.library's window system integration: a surface on a
    window (VK_AROS_surface, then the tag based vkACreateSurface), a
    swapchain, and frames presented into the window. The frames are plain
    clears, so no shader compiler is involved; each of the first frames is
    read back out of the window's RastPort and compared with the colour
    that was cleared. The images are cleared as transfer destinations
    first, then as the colour attachment of a render pass.

    Then the window is resized: acquiring or presenting must report it
    (VK_ERROR_OUT_OF_DATE_KHR / VK_SUBOPTIMAL_KHR), and a swapchain made
    with the old one as oldSwapchain must present at the new size.

    FRAMES=n presents n more animated frames afterwards (default 30).

    When the swapchain presents by scan-out (its images are display buffers:
    VKSA_PresentPath is VKPP_Flip) the frame is not in the window's bitmap;
    the pixel is then read from the front buffer of the scan-out bitmap,
    the memory the display shows. SetEnv VK_PRESENT readback makes the
    library copy into the window instead.

    Result: 0 when every check passes, 20 otherwise.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/oop.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/gfx.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_aros.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIDTH   320
#define HEIGHT  240

static int failures;

OOP_AttrBase HiddBitMapAttrBase;

/* where the surface is in the window's inner area */
static LONG surf_left, surf_top;

#define CHECK(cond, ...) \
    do { if (!(cond)) { printf("[vkwsi] FAIL: " __VA_ARGS__); printf("\n"); failures++; } } while (0)
#define VK(call) \
    do { VkResult r_ = (call); if (r_ != VK_SUCCESS) { \
        printf("[vkwsi] FAIL: %s = %d\n", #call, (int)r_); failures++; goto out; } } while (0)

static VkInstance instance;
static VkPhysicalDevice phys;
static VkDevice device;
static VkQueue queue;
static uint32_t family;
static VkSurfaceKHR surface;
static VkSwapchainKHR swapchain;
static VkImage images[8];
static uint32_t imageCount;
static VkExtent2D extent;
static VkCommandPool pool;
static VkCommandBuffer cmd;
static VkSemaphore acquireSem, renderSem;
static VkFence fence;
static struct Window *win;
static VkRenderPass renderpass;
static VkImageView views[8];
static VkFramebuffer framebuffers[8];

static VkResult make_swapchain(VkSwapchainKHR old)
{
    VkSurfaceCapabilitiesKHR caps;
    VkSwapchainCreateInfoKHR ci =
    {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = old,
    };
    VkResult res;

    res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phys, surface, &caps);
    if (res != VK_SUCCESS)
        return res;
    extent = caps.currentExtent;
    ci.minImageCount = caps.minImageCount + 1;
    ci.imageExtent = extent;

    res = vkCreateSwapchainKHR(device, &ci, NULL, &swapchain);
    if (res != VK_SUCCESS)
        return res;
    if (old)
        vkDestroySwapchainKHR(device, old, NULL);

    imageCount = 8;
    res = vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images);
    printf("[vkwsi] swapchain %ux%u, %u images\n", (unsigned)extent.width, (unsigned)extent.height,
           (unsigned)imageCount);
    return res;
}

static void free_framebuffers(void)
{
    uint32_t i;

    for (i = 0; i < 8; i++)
    {
        if (framebuffers[i])
            vkDestroyFramebuffer(device, framebuffers[i], NULL);
        if (views[i])
            vkDestroyImageView(device, views[i], NULL);
        framebuffers[i] = VK_NULL_HANDLE;
        views[i] = VK_NULL_HANDLE;
    }
    if (renderpass)
        vkDestroyRenderPass(device, renderpass, NULL);
    renderpass = VK_NULL_HANDLE;
}

/* a render pass that clears a swapchain image and leaves it presentable,
   and a framebuffer for each image */
static VkResult make_framebuffers(void)
{
    VkAttachmentDescription att =
    {
        .format = VK_FORMAT_B8G8R8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference ref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub =
    {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ref,
    };
    VkRenderPassCreateInfo rpi =
    {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &att,
        .subpassCount = 1,
        .pSubpasses = &sub,
    };
    VkResult res;
    uint32_t i;

    res = vkCreateRenderPass(device, &rpi, NULL, &renderpass);
    for (i = 0; res == VK_SUCCESS && i < imageCount; i++)
    {
        VkImageViewCreateInfo vi =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_B8G8R8A8_UNORM,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        VkFramebufferCreateInfo fi =
        {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderpass,
            .attachmentCount = 1,
            .pAttachments = &views[i],
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
        };

        res = vkCreateImageView(device, &vi, NULL, &views[i]);
        if (res == VK_SUCCESS)
            res = vkCreateFramebuffer(device, &fi, NULL, &framebuffers[i]);
    }
    return res;
}

static BOOL use_renderpass;

/* clear the next image to the colour and present it; the acquire/present result */
static VkResult frame(float r, float g, float b)
{
    VkCommandBufferBeginInfo bi =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    VkImageMemoryBarrier barrier =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .subresourceRange = range,
    };
    VkClearColorValue color = { .float32 = { r, g, b, 1.0f } };
    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo si =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &acquireSem,
        .pWaitDstStageMask = &stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &renderSem,
    };
    VkPresentInfoKHR pi =
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderSem,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
    };
    uint32_t index = 0;
    VkResult res;

    res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, acquireSem, VK_NULL_HANDLE, &index);
    if (res != VK_SUCCESS)
        return res;

    vkResetCommandBuffer(cmd, 0);
    vkBeginCommandBuffer(cmd, &bi);
    if (use_renderpass)
    {
        VkClearValue clear = { .color = color };
        VkRenderPassBeginInfo rbi =
        {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = renderpass,
            .framebuffer = framebuffers[index],
            .renderArea = { { 0, 0 }, extent },
            .clearValueCount = 1,
            .pClearValues = &clear,
        };

        stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        vkCmdBeginRenderPass(cmd, &rbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdEndRenderPass(cmd);
        vkEndCommandBuffer(cmd);
        goto submit;
    }
    stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    barrier.image = images[index];
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier);
    vkCmdClearColorImage(cmd, images[index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, NULL, 0, NULL, 1, &barrier);
    vkEndCommandBuffer(cmd);

submit:
    res = vkQueueSubmit(queue, 1, &si, fence);
    if (res != VK_SUCCESS)
        return res;

    pi.pImageIndices = &index;
    res = vkQueuePresentKHR(queue, &pi);

    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &fence);
    return res;
}

static BOOL flipping(void)
{
    return surface && vkAGetSurfaceAttr(instance, surface, VKSA_PresentPath) == VKPP_Flip;
}

/* the pixel of the window's inner area at x,y, as 0x00RRGGBB */
static ULONG window_pixel(LONG x, LONG y)
{
    ULONG argb = 0;

    if (flipping())
    {
        /* what the display shows: the scan-out bitmap's front buffer */
        OOP_Object *bm = (OOP_Object *)vkAGetSurfaceAttr(instance, surface, VKSA_ScanoutBitMap);
        IPTR front = 0;
        UBYTE *addr = NULL;
        ULONG pitch = 0, rgb = 0xDEAD00;

        if (!bm || !HiddBitMapAttrBase)
            return rgb;
        OOP_GetAttr(bm, aHidd_BitMap_FrontBuffer, &front);
        if (HIDD_BM_LockBuffer(bm, front, &addr, &pitch, NULL))
        {
            struct Rectangle none = { 0, 0, -1, -1 };
            UBYTE *p = addr + (y - surf_top) * pitch + (x - surf_left) * 4;     /* B G R A */

            rgb = ((ULONG)p[2] << 16) | ((ULONG)p[1] << 8) | p[0];
            HIDD_BM_UnlockBuffer(bm, front, &none);
        }
        return rgb;
    }

    ReadPixelArray(&argb, 0, 0, 4, win->RPort, win->BorderLeft + x, win->BorderTop + y, 1, 1, RECTFMT_ARGB32);
    return AROS_BE2LONG(argb) & 0x00FFFFFF;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_cb(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT *data, void *user)
{
    return VK_FALSE;
}

static BOOL near(ULONG a, ULONG b)
{
    int i;

    /* a 15/16 bit screen keeps 5 or 6 bits of a channel */
    for (i = 0; i < 24; i += 8)
    {
        int d = (int)((a >> i) & 0xFF) - (int)((b >> i) & 0xFF);

        if (d < -8 || d > 8)
            return FALSE;
    }
    return TRUE;
}

int main(int argc, char **argv)
{
    static const char *instExts[4] = { VK_KHR_SURFACE_EXTENSION_NAME, VK_AROS_SURFACE_EXTENSION_NAME,
                                       VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME };
    BOOL haveDebug = FALSE;
    static const char *devExts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    static const struct { float r, g, b; ULONG rgb; } colours[] =
    {
        { 1, 0, 0, 0xFF0000 }, { 0, 1, 0, 0x00FF00 }, { 0, 0, 1, 0x0000FF },
        { 1, 1, 1, 0xFFFFFF }, { 0, 0, 0, 0x000000 },
    };
    VkApplicationInfo app =
    {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "vkwsi",
        .apiVersion = VK_API_VERSION_1_1,
    };
    VkInstanceCreateInfo ici =
    {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app,
        .enabledExtensionCount = 3,
        .ppEnabledExtensionNames = instExts,
    };
    VkExtensionProperties exts[256];
    VkPhysicalDevice devs[8];
    VkPhysicalDeviceProperties props;
    VkQueueFamilyProperties qf[16];
    VkSurfaceFormatKHR formats[16];
    VkPresentModeKHR modes[8];
    uint32_t count, i, apiVersion = 0;
    BOOL haveSurface = FALSE, haveAROS = FALSE, haveSwapchain = FALSE, found = FALSE;
    int frames = 30;
    VkResult res;

    HiddBitMapAttrBase = OOP_ObtainAttrBase((STRPTR)IID_Hidd_BitMap);

    for (i = 1; i < (uint32_t)argc; i++)
        if (!strncmp(argv[i], "FRAMES=", 7))
            frames = atoi(argv[i] + 7);

    vkEnumerateInstanceVersion(&apiVersion);
    printf("[vkwsi] instance version %u.%u.%u\n", (unsigned)VK_API_VERSION_MAJOR(apiVersion),
           (unsigned)VK_API_VERSION_MINOR(apiVersion), (unsigned)VK_API_VERSION_PATCH(apiVersion));

    count = 256;
    VK(vkEnumerateInstanceExtensionProperties(NULL, &count, exts));
    for (i = 0; i < count; i++)
    {
        if (!strcmp(exts[i].extensionName, VK_KHR_SURFACE_EXTENSION_NAME))
            haveSurface = TRUE;
        if (!strcmp(exts[i].extensionName, VK_AROS_SURFACE_EXTENSION_NAME))
            haveAROS = TRUE;
        if (!strcmp(exts[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
            haveDebug = TRUE;
        if (strstr(exts[i].extensionName, "headless"))
            CHECK(0, "the driver's own %s is listed", exts[i].extensionName);
    }
    printf("[vkwsi] %u instance extensions\n", (unsigned)count);
    CHECK(haveSurface, "no VK_KHR_surface");
    CHECK(haveAROS, "no VK_AROS_surface");

    /* a driver's extension, passed down to it */
    if (haveDebug)
        instExts[ici.enabledExtensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    VK(vkCreateInstance(&ici, NULL, &instance));

    /* an extension command on the instance: the library forwards it to the driver */
    if (haveDebug)
    {
        PFN_vkCreateDebugUtilsMessengerEXT create = (PFN_vkCreateDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        PFN_vkDestroyDebugUtilsMessengerEXT destroy = (PFN_vkDestroyDebugUtilsMessengerEXT)
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        VkDebugUtilsMessengerCreateInfoEXT mi =
        {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
            .pfnUserCallback = debug_cb,
        };
        VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;

        CHECK(create && destroy, "no debug utils messenger entry points");
        if (create && destroy)
        {
            VK(create(instance, &mi, NULL, &messenger));
            destroy(instance, messenger, NULL);
            printf("[vkwsi] debug utils messenger made and destroyed\n");
        }
    }
    else
        printf("[vkwsi] no VK_EXT_debug_utils: instance extension forwarding not tested\n");

    count = 8;
    res = vkEnumeratePhysicalDevices(instance, &count, devs);
    if (res < 0 || !count)
    {
        printf("[vkwsi] FAIL: no physical device (%d)\n", (int)res);
        failures++;
        goto out;
    }
    phys = devs[0];
    vkGetPhysicalDeviceProperties(phys, &props);
    printf("[vkwsi] %u device(s), using '%s'\n", (unsigned)count, props.deviceName);

    count = 256;
    VK(vkEnumerateDeviceExtensionProperties(phys, NULL, &count, exts));
    for (i = 0; i < count; i++)
        if (!strcmp(exts[i].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
            haveSwapchain = TRUE;
    CHECK(haveSwapchain, "no VK_KHR_swapchain");

    win = OpenWindowTags(NULL,
        WA_Title, (IPTR)"vkwsi",
        WA_InnerWidth, WIDTH, WA_InnerHeight, HEIGHT,
        WA_Left, 40, WA_Top, 40,
        WA_DragBar, TRUE, WA_DepthGadget, TRUE, WA_CloseGadget, TRUE, WA_SizeGadget, TRUE,
        WA_MinWidth, 64, WA_MinHeight, 64, WA_MaxWidth, ~0, WA_MaxHeight, ~0,
        WA_Activate, TRUE, WA_IDCMP, IDCMP_CLOSEWINDOW,
        TAG_DONE);
    if (!win)
    {
        printf("[vkwsi] FAIL: no window\n");
        failures++;
        goto out;
    }

    {
        VkAROSSurfaceCreateInfoAROS sci =
        {
            .sType = VK_STRUCTURE_TYPE_AROS_SURFACE_CREATE_INFO_AROS,
            .window = win,
        };
        PFN_vkCreateAROSSurfaceAROS create =
            (PFN_vkCreateAROSSurfaceAROS)vkGetInstanceProcAddr(instance, "vkCreateAROSSurfaceAROS");

        CHECK(create != NULL, "vkGetInstanceProcAddr(vkCreateAROSSurfaceAROS) is NULL");
        if (!create)
            goto out;
        VK(create(instance, &sci, NULL, &surface));
    }
    CHECK((struct Window *)vkAGetSurfaceAttr(instance, surface, VKSA_Window) == win, "VKSA_Window");
    CHECK(vkAGetSurfaceAttr(instance, surface, VKSA_Width) == WIDTH, "VKSA_Width");

    count = 16;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &count, qf);
    for (i = 0; i < count; i++)
    {
        VkBool32 ok = VK_FALSE;

        vkGetPhysicalDeviceSurfaceSupportKHR(phys, i, surface, &ok);
        if (ok && (qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            family = i;
            found = TRUE;
            break;
        }
    }
    CHECK(found, "no queue family presents");
    if (!found)
        goto out;

    count = 16;
    VK(vkGetPhysicalDeviceSurfaceFormatsKHR(phys, surface, &count, formats));
    printf("[vkwsi] %u surface format(s), first %d\n", (unsigned)count, (int)formats[0].format);
    CHECK(count > 0, "no surface format");
    {
        PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR caps2 = (PFN_vkGetPhysicalDeviceSurfaceCapabilities2KHR)
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceSurfaceCapabilities2KHR");
        VkPhysicalDeviceSurfaceInfo2KHR si2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR,
                                                .surface = surface };
        VkSurfaceCapabilities2KHR c2 = { .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR };

        CHECK(caps2 != NULL, "no vkGetPhysicalDeviceSurfaceCapabilities2KHR");
        if (caps2)
        {
            VK(caps2(phys, &si2, &c2));
            CHECK(c2.surfaceCapabilities.currentExtent.width == WIDTH, "capabilities2 extent");
        }
    }
    count = 8;
    VK(vkGetPhysicalDeviceSurfacePresentModesKHR(phys, surface, &count, modes));
    CHECK(count > 0 , "no present mode");

    {
        float prio = 1.0f;
        VkDeviceQueueCreateInfo qci =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = family,
            .queueCount = 1,
            .pQueuePriorities = &prio,
        };
        VkDeviceCreateInfo dci =
        {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &qci,
            .enabledExtensionCount = 1,
            .ppEnabledExtensionNames = devExts,
        };
        VkCommandPoolCreateInfo pci =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = family,
        };
        VkCommandBufferAllocateInfo cai =
        {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkSemaphoreCreateInfo sci = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
        VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };

        VK(vkCreateDevice(phys, &dci, NULL, &device));
        vkGetDeviceQueue(device, family, 0, &queue);
        CHECK(vkGetDeviceProcAddr(device, "vkQueuePresentKHR") != NULL, "vkGetDeviceProcAddr(vkQueuePresentKHR)");
        CHECK(vkGetDeviceProcAddr(device, "vkCmdDraw") != NULL, "vkGetDeviceProcAddr(vkCmdDraw)");
        VK(vkCreateCommandPool(device, &pci, NULL, &pool));
        cai.commandPool = pool;
        VK(vkAllocateCommandBuffers(device, &cai, &cmd));
        VK(vkCreateSemaphore(device, &sci, NULL, &acquireSem));
        VK(vkCreateSemaphore(device, &sci, NULL, &renderSem));
        VK(vkCreateFence(device, &fci, NULL, &fence));
    }

    VK(make_swapchain(VK_NULL_HANDLE));
    CHECK(extent.width == WIDTH && extent.height == HEIGHT, "extent %ux%u", (unsigned)extent.width,
          (unsigned)extent.height);

    /* frames whose colour is checked in the window */
    for (i = 0; i < sizeof(colours) / sizeof(colours[0]); i++)
    {
        ULONG got;

        VK(frame(colours[i].r, colours[i].g, colours[i].b));
        got = window_pixel(WIDTH / 2, HEIGHT / 2);
        CHECK(near(got, colours[i].rgb), "frame %u: window has %06lx, expected %06lx", (unsigned)i,
              (unsigned long)got, (unsigned long)colours[i].rgb);
        got = window_pixel(WIDTH - 1, HEIGHT - 1);
        CHECK(near(got, colours[i].rgb), "frame %u corner: window has %06lx, expected %06lx", (unsigned)i,
              (unsigned long)got, (unsigned long)colours[i].rgb);
    }
    printf("[vkwsi] colour frames done, %d failure(s) so far\n", failures);
    if (flipping())
        printf("[vkwsi] presenting by scan-out: %lu buffers, %s\n",
               (unsigned long)vkAGetSurfaceAttr(instance, surface, VKSA_Buffers),
               vkAGetSurfaceAttr(instance, surface, VKSA_OnPlane) ? "on a plane" : "blitted by the display");
    else
        printf("[vkwsi] presenting by read-back\n");

    /* the same, the images being the colour attachment of a render pass */
    VK(make_framebuffers());
    use_renderpass = TRUE;
    for (i = 0; i < sizeof(colours) / sizeof(colours[0]); i++)
    {
        ULONG got;

        VK(frame(colours[i].r, colours[i].g, colours[i].b));
        got = window_pixel(WIDTH / 2, HEIGHT / 2);
        CHECK(near(got, colours[i].rgb), "render pass frame %u: window has %06lx, expected %06lx", (unsigned)i,
              (unsigned long)got, (unsigned long)colours[i].rgb);
    }
    use_renderpass = FALSE;
    vkDeviceWaitIdle(device);
    free_framebuffers();
    printf("[vkwsi] render pass frames done, %d failure(s) so far\n", failures);
    CHECK(vkAGetSurfaceAttr(instance, surface, VKSA_PresentPath) != VKPP_None, "VKSA_PresentPath");

    /* resize: the swapchain no longer fits */
    ChangeWindowBox(win, win->LeftEdge, win->TopEdge, win->Width + 80, win->Height + 40);
    for (i = 0; i < 50 && win->Width - win->BorderLeft - win->BorderRight == WIDTH; i++)
        Delay(2);
    res = frame(0.5f, 0.5f, 0.5f);
    printf("[vkwsi] after resize: %d\n", (int)res);
    CHECK(res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR, "resize not reported (%d)", (int)res);

    vkDeviceWaitIdle(device);
    VK(make_swapchain(swapchain));
    CHECK(extent.width == WIDTH + 80 && extent.height == HEIGHT + 40, "new extent %ux%u",
          (unsigned)extent.width, (unsigned)extent.height);
    VK(frame(1, 1, 0));
    {
        ULONG got = window_pixel(WIDTH + 80 - 1, HEIGHT + 40 - 1);

        CHECK(near(got, 0xFFFF00), "resized frame: window has %06lx, expected ffff00", (unsigned long)got);
    }

    /* the tag based surface call, on a rectangle of the window */
    vkDeviceWaitIdle(device);
    vkDestroySwapchainKHR(device, swapchain, NULL);
    swapchain = VK_NULL_HANDLE;
    vkDestroySurfaceKHR(instance, surface, NULL);
    surface = VK_NULL_HANDLE;
    VK(vkACreateSurfaceTags(instance, NULL, &surface, VKA_Window, (IPTR)win,
                            VKA_Left, 16, VKA_Top, 16, VKA_Width, 128, VKA_Height, 96, TAG_DONE));
    surf_left = 16;
    surf_top = 16;
    VK(make_swapchain(VK_NULL_HANDLE));
    CHECK(extent.width == 128 && extent.height == 96, "rectangle extent %ux%u", (unsigned)extent.width,
          (unsigned)extent.height);
    VK(frame(1, 0, 1));
    {
        ULONG in = window_pixel(16 + 64, 16 + 48);

        CHECK(near(in, 0xFF00FF), "rectangle: inside has %06lx, expected ff00ff", (unsigned long)in);
        /* outside the rectangle the window keeps the frame before: only
           when frames are copied into the window at all */
        if (!flipping())
        {
            ULONG outside = window_pixel(4, 4);

            CHECK(near(outside, 0xFFFF00), "rectangle: outside has %06lx, expected ffff00",
                  (unsigned long)outside);
        }
    }

    /* and some frames to look at */
    for (i = 0; i < (uint32_t)frames; i++)
    {
        struct IntuiMessage *msg;
        float t = (float)i / (float)(frames > 1 ? frames - 1 : 1);

        res = frame(t, 1.0f - t, 0.25f);
        if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
        {
            printf("[vkwsi] frame %u: %d\n", (unsigned)i, (int)res);
            break;
        }
        if ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
        {
            ReplyMsg((struct Message *)msg);
            break;
        }
    }

    /* a window closed under its surface: no crash, and the surface is lost */
    if (failures == 0)
    {
        uint32_t index = 0;

        vkDeviceWaitIdle(device);
        CloseWindow(win);
        win = NULL;
        res = vkAcquireNextImageKHR(device, swapchain, 0, VK_NULL_HANDLE, VK_NULL_HANDLE, &index);
        printf("[vkwsi] acquire after the window closed: %d\n", (int)res);
        CHECK(res == VK_ERROR_SURFACE_LOST_KHR, "closed window not reported (%d)", (int)res);
    }

out:
    if (device)
    {
        vkDeviceWaitIdle(device);
        free_framebuffers();
        if (swapchain)
            vkDestroySwapchainKHR(device, swapchain, NULL);
        if (fence)
            vkDestroyFence(device, fence, NULL);
        if (renderSem)
            vkDestroySemaphore(device, renderSem, NULL);
        if (acquireSem)
            vkDestroySemaphore(device, acquireSem, NULL);
        if (pool)
            vkDestroyCommandPool(device, pool, NULL);
        vkDestroyDevice(device, NULL);
    }
    if (surface)
        vkDestroySurfaceKHR(instance, surface, NULL);
    if (instance)
        vkDestroyInstance(instance, NULL);
    if (win)
        CloseWindow(win);

    printf("[vkwsi] done: %s (%d failure(s))\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 20 : 0;
}
