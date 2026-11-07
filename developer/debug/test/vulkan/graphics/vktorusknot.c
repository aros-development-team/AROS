/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    vktorusknot - draws through the Vulkan graphics pipeline directly, with no
    GL in between: a lit, rainbow-coloured (2,3) torus knot of about 25000
    triangles, with a depth buffer, per-vertex diffuse and specular lighting
    and the transforms in push constants, turning in a window.

    Each frame is rendered into an offscreen image, copied to a host-visible
    buffer and written into the window with WritePixelArray(), so only the
    Vulkan driver is under test. The first frame is checked: the background
    must be the clear colour, the knot must cover a plausible part of the
    image with lit and shaded parts, and drawing the same frame again must
    give the same pixels.

    Like glsimplerendering it runs until its window is closed (or Ctrl-C),
    with the frame rate in the window title. FRAMES=n stops after n frames
    instead, for scripted runs; NOWINDOW renders without a window: the
    check alone, or FRAMES frames of rendering.

    Result: 0 when the check passes, 20 when it fails or Vulkan is missing.
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <intuition/intuition.h>
#include <cybergraphx/cybergraphics.h>
#include <aros/debug.h>

#include <vulkan/vulkan.h>

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vktorusknot_shaders.h"

#define WIDTH           384
#define HEIGHT          384
#define SEGMENTS        512     /* along the knot */
#define SIDES           24      /* around the tube */
#define FENCE_TIMEOUT   (5ULL * 1000000000ULL)

/* The shader's push constants (std430 offsets 0, 64, 128, 144) */
struct pushc
{
    float mvp[16];
    float model[16];
    float light[4];
    float halfdir[4];
};

struct vertex
{
    float pos[3];
    float nrm[3];
    float col[3];
};

static const float clearcol[4] = { 0.08f, 0.08f, 0.12f, 1.0f };

/* ---- Vulkan entry points, all through vkGetInstanceProcAddr ---- */

#define VKFUNCS \
    F(vkDestroyInstance) F(vkEnumeratePhysicalDevices) F(vkGetPhysicalDeviceProperties) \
    F(vkGetPhysicalDeviceQueueFamilyProperties) F(vkGetPhysicalDeviceMemoryProperties) \
    F(vkCreateDevice) F(vkDestroyDevice) F(vkGetDeviceQueue) F(vkDeviceWaitIdle) \
    F(vkCreateImage) F(vkDestroyImage) F(vkGetImageMemoryRequirements) F(vkBindImageMemory) \
    F(vkCreateImageView) F(vkDestroyImageView) \
    F(vkCreateBuffer) F(vkDestroyBuffer) F(vkGetBufferMemoryRequirements) F(vkBindBufferMemory) \
    F(vkAllocateMemory) F(vkFreeMemory) F(vkMapMemory) F(vkUnmapMemory) \
    F(vkCreateRenderPass) F(vkDestroyRenderPass) F(vkCreateFramebuffer) F(vkDestroyFramebuffer) \
    F(vkCreateShaderModule) F(vkDestroyShaderModule) \
    F(vkCreatePipelineLayout) F(vkDestroyPipelineLayout) \
    F(vkCreateGraphicsPipelines) F(vkDestroyPipeline) \
    F(vkCreateCommandPool) F(vkDestroyCommandPool) F(vkAllocateCommandBuffers) \
    F(vkBeginCommandBuffer) F(vkEndCommandBuffer) F(vkResetCommandBuffer) \
    F(vkCmdBeginRenderPass) F(vkCmdEndRenderPass) F(vkCmdBindPipeline) \
    F(vkCmdBindVertexBuffers) F(vkCmdBindIndexBuffer) F(vkCmdPushConstants) \
    F(vkCmdDrawIndexed) F(vkCmdCopyImageToBuffer) F(vkCmdPipelineBarrier) \
    F(vkInvalidateMappedMemoryRanges) F(vkCreateFence) F(vkDestroyFence) F(vkWaitForFences) F(vkResetFences) F(vkQueueSubmit)

#define F(n) static PFN_##n p##n;
VKFUNCS
#undef F

static VkInstance instance;
static VkPhysicalDevice physical;
static VkDevice device;
static VkQueue queue;
static VkPhysicalDeviceMemoryProperties memprops;

static VkImage colorimg, depthimg;
static VkDeviceMemory colormem, depthmem, vbmem, ibmem, rbmem;
static VkImageView colorview, depthview;
static VkBuffer vb, ib, rb;
static VkRenderPass renderpass;
static VkFramebuffer framebuffer;
static VkShaderModule vsmod, fsmod;
static VkPipelineLayout layout;
static VkPipeline pipeline;
static VkCommandPool cmdpool;
static VkCommandBuffer cmd;
static VkFence fence;
static UBYTE *pixels;
static BOOL rbinvalidate;
static VkMemoryPropertyFlags rbflags;
static ULONG nindices;

static void tlog(const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    printf("[vktorusknot] %s\n", buf);
    bug("[vktorusknot] %s\n", buf);
}

#define CHECK(call) do { VkResult _r = (call); if (_r != VK_SUCCESS) { \
    tlog("%s failed: %d", #call, (int)_r); return FALSE; } } while (0)

/* ---- matrices (column-major, Vulkan clip space: y down, z 0..1) ---- */

static void mat_mul(float *r, const float *a, const float *b)
{
    float t[16];
    int i, j, k;

    for (i = 0; i < 4; i++)
        for (j = 0; j < 4; j++)
        {
            t[j * 4 + i] = 0;
            for (k = 0; k < 4; k++)
                t[j * 4 + i] += a[k * 4 + i] * b[j * 4 + k];
        }
    memcpy(r, t, sizeof(t));
}

static void mat_rot(float *m, float angle, int axis)
{
    float c = cosf(angle), s = sinf(angle);
    int a = (axis + 1) % 3, b = (axis + 2) % 3;

    memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1;
    m[a * 4 + a] = c;
    m[b * 4 + a] = -s;
    m[a * 4 + b] = s;
    m[b * 4 + b] = c;
}

static void frame_matrices(struct pushc *pc, float t)
{
    float rx[16], ry[16], view[16], proj[16], vp[16];
    float f = 1.0f / tanf(0.5f * 0.75f), n = 0.1f, fa = 10.0f;
    float l[3] = { -0.5f, 0.7f, 0.6f }, h[3];
    float ll, hl;
    int i;

    mat_rot(rx, 0.6f + 0.37f * t, 0);
    mat_rot(ry, t, 1);
    mat_mul(pc->model, ry, rx);

    memset(view, 0, sizeof(view));
    view[0] = view[5] = view[10] = view[15] = 1;
    view[14] = -3.2f;

    memset(proj, 0, sizeof(proj));
    proj[0] = f;
    proj[5] = -f;                       /* Vulkan's y points down */
    proj[10] = fa / (n - fa);
    proj[11] = -1;
    proj[14] = n * fa / (n - fa);

    mat_mul(vp, proj, view);
    mat_mul(pc->mvp, vp, pc->model);

    /* light and the half vector to a viewer on +z, in world space */
    ll = sqrtf(l[0] * l[0] + l[1] * l[1] + l[2] * l[2]);
    for (i = 0; i < 3; i++)
        l[i] /= ll;
    h[0] = l[0]; h[1] = l[1]; h[2] = l[2] + 1;
    hl = sqrtf(h[0] * h[0] + h[1] * h[1] + h[2] * h[2]);
    for (i = 0; i < 3; i++)
    {
        pc->light[i] = l[i];
        pc->halfdir[i] = h[i] / hl;
    }
    pc->light[3] = pc->halfdir[3] = 0;
}

/* ---- the mesh: a tube around a (2,3) torus knot ---- */

static void knot(float t, float *p)
{
    float r = 2.0f + cosf(3 * t);

    p[0] = 0.28f * r * cosf(2 * t);
    p[1] = 0.28f * r * sinf(2 * t);
    p[2] = 0.28f * sinf(3 * t);
}

static void normalize3(float *v)
{
    float l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

    if (l > 0)
    {
        v[0] /= l; v[1] /= l; v[2] /= l;
    }
}

static void cross3(float *r, const float *a, const float *b)
{
    r[0] = a[1] * b[2] - a[2] * b[1];
    r[1] = a[2] * b[0] - a[0] * b[2];
    r[2] = a[0] * b[1] - a[1] * b[0];
}

static void hue(float h, float *c)
{
    int i;

    for (i = 0; i < 3; i++)
    {
        float x = fabsf(fmodf(h * 6 + 4 - 2 * i + 6, 6) - 3) - 1;
        c[i] = x < 0 ? 0 : (x > 1 ? 1 : x);
    }
}

static void build_mesh(struct vertex *v, UWORD *idx)
{
    const float tube = 0.11f, e = 0.001f;
    int i, j, k = 0;

    for (i = 0; i < SEGMENTS; i++)
    {
        float t = 2 * (float)M_PI * i / SEGMENTS;
        float c[3], a[3], b[3], tan[3], acc[3], nrm[3], bin[3], col[3];

        knot(t, c);
        knot(t - e, a);
        knot(t + e, b);
        for (j = 0; j < 3; j++)
        {
            tan[j] = b[j] - a[j];
            acc[j] = b[j] + a[j] - 2 * c[j];
        }
        normalize3(tan);
        cross3(bin, tan, acc);
        normalize3(bin);
        cross3(nrm, bin, tan);
        hue((float)i / SEGMENTS, col);

        for (j = 0; j < SIDES; j++)
        {
            float u = 2 * (float)M_PI * j / SIDES, cu = cosf(u), su = sinf(u);
            struct vertex *o = &v[i * SIDES + j];
            int m;

            for (m = 0; m < 3; m++)
            {
                o->nrm[m] = cu * nrm[m] + su * bin[m];
                o->pos[m] = c[m] + tube * o->nrm[m];
                o->col[m] = col[m];
            }
        }
    }
    for (i = 0; i < SEGMENTS; i++)
    {
        int i1 = (i + 1) % SEGMENTS;

        for (j = 0; j < SIDES; j++)
        {
            int j1 = (j + 1) % SIDES;

            idx[k++] = i * SIDES + j;
            idx[k++] = i1 * SIDES + j;
            idx[k++] = i * SIDES + j1;
            idx[k++] = i * SIDES + j1;
            idx[k++] = i1 * SIDES + j;
            idx[k++] = i1 * SIDES + j1;
        }
    }
}

/* ---- setup ---- */

static int memtype(uint32_t bits, VkMemoryPropertyFlags want)
{
    uint32_t i;

    for (i = 0; i < memprops.memoryTypeCount; i++)
        if ((bits & (1u << i)) && (memprops.memoryTypes[i].propertyFlags & want) == want)
            return (int)i;
    return -1;
}

static BOOL bind_memory(VkMemoryRequirements *req, VkMemoryPropertyFlags want, VkDeviceMemory *mem)
{
    VkMemoryAllocateInfo ai = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    int type = memtype(req->memoryTypeBits, want);

    if (type < 0)
    {
        tlog("no memory type with flags 0x%x", (unsigned)want);
        return FALSE;
    }
    ai.allocationSize = req->size;
    ai.memoryTypeIndex = type;
    CHECK(pvkAllocateMemory(device, &ai, NULL, mem));
    return TRUE;
}

static BOOL make_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer *buf, VkDeviceMemory *mem)
{
    VkBufferCreateInfo bi = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    VkMemoryRequirements req;
    VkMemoryPropertyFlags want = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    bi.size = size;
    bi.usage = usage;
    bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    CHECK(pvkCreateBuffer(device, &bi, NULL, buf));
    pvkGetBufferMemoryRequirements(device, *buf, &req);
    /* The CPU reads the frame back: from uncached memory that costs more
     * than drawing it, so take cached memory where there is some */
    if (usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT)
    {
        if (memtype(req.memoryTypeBits, want | VK_MEMORY_PROPERTY_HOST_CACHED_BIT) >= 0)
            want |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        else if (memtype(req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT) >= 0)
        {
            /* cached but not coherent: each frame is invalidated before it is read */
            want = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
            rbinvalidate = TRUE;
        }
        rbflags = memprops.memoryTypes[memtype(req.memoryTypeBits, want)].propertyFlags;
    }
    if (!bind_memory(&req, want, mem))
        return FALSE;
    CHECK(pvkBindBufferMemory(device, *buf, *mem, 0));
    return TRUE;
}

static BOOL make_image(VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect,
    VkImage *img, VkDeviceMemory *mem, VkImageView *view)
{
    VkImageCreateInfo ii = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    VkImageViewCreateInfo vi = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    VkMemoryRequirements req;

    ii.imageType = VK_IMAGE_TYPE_2D;
    ii.format = format;
    ii.extent.width = WIDTH;
    ii.extent.height = HEIGHT;
    ii.extent.depth = 1;
    ii.mipLevels = 1;
    ii.arrayLayers = 1;
    ii.samples = VK_SAMPLE_COUNT_1_BIT;
    ii.tiling = VK_IMAGE_TILING_OPTIMAL;
    ii.usage = usage;
    ii.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    CHECK(pvkCreateImage(device, &ii, NULL, img));
    pvkGetImageMemoryRequirements(device, *img, &req);
    if (!bind_memory(&req, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mem))
        return FALSE;
    CHECK(pvkBindImageMemory(device, *img, *mem, 0));

    vi.image = *img;
    vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    vi.format = format;
    vi.subresourceRange.aspectMask = aspect;
    vi.subresourceRange.levelCount = 1;
    vi.subresourceRange.layerCount = 1;
    CHECK(pvkCreateImageView(device, &vi, NULL, view));
    return TRUE;
}

static BOOL make_pipeline(void)
{
    VkShaderModuleCreateInfo si = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    VkPushConstantRange range = { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(struct pushc) };
    VkPipelineLayoutCreateInfo li = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    VkPipelineShaderStageCreateInfo stages[2] = {
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO },
        { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO } };
    VkVertexInputBindingDescription binding = { 0, sizeof(struct vertex), VK_VERTEX_INPUT_RATE_VERTEX };
    VkVertexInputAttributeDescription attrs[3] = {
        { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct vertex, pos) },
        { 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct vertex, nrm) },
        { 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(struct vertex, col) } };
    VkPipelineVertexInputStateCreateInfo vin = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    VkPipelineInputAssemblyStateCreateInfo ia = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    VkViewport vp = { 0, 0, WIDTH, HEIGHT, 0, 1 };
    VkRect2D sc = { { 0, 0 }, { WIDTH, HEIGHT } };
    VkPipelineViewportStateCreateInfo vps = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    VkPipelineRasterizationStateCreateInfo rs = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    VkPipelineMultisampleStateCreateInfo ms = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    VkPipelineDepthStencilStateCreateInfo ds = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    VkPipelineColorBlendAttachmentState cba = { 0 };
    VkPipelineColorBlendStateCreateInfo cb = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    VkGraphicsPipelineCreateInfo pi = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

    si.codeSize = sizeof(vktk_vert);
    si.pCode = vktk_vert;
    CHECK(pvkCreateShaderModule(device, &si, NULL, &vsmod));
    si.codeSize = sizeof(vktk_frag);
    si.pCode = vktk_frag;
    CHECK(pvkCreateShaderModule(device, &si, NULL, &fsmod));

    li.pushConstantRangeCount = 1;
    li.pPushConstantRanges = &range;
    CHECK(pvkCreatePipelineLayout(device, &li, NULL, &layout));

    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vsmod;
    stages[0].pName = "main";
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fsmod;
    stages[1].pName = "main";

    vin.vertexBindingDescriptionCount = 1;
    vin.pVertexBindingDescriptions = &binding;
    vin.vertexAttributeDescriptionCount = 3;
    vin.pVertexAttributeDescriptions = attrs;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    vps.viewportCount = 1;
    vps.pViewports = &vp;
    vps.scissorCount = 1;
    vps.pScissors = &sc;
    rs.polygonMode = VK_POLYGON_MODE_FILL;
    rs.cullMode = VK_CULL_MODE_NONE;
    rs.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rs.lineWidth = 1;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    pi.stageCount = 2;
    pi.pStages = stages;
    pi.pVertexInputState = &vin;
    pi.pInputAssemblyState = &ia;
    pi.pViewportState = &vps;
    pi.pRasterizationState = &rs;
    pi.pMultisampleState = &ms;
    pi.pDepthStencilState = &ds;
    pi.pColorBlendState = &cb;
    pi.layout = layout;
    pi.renderPass = renderpass;
    CHECK(pvkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, NULL, &pipeline));
    return TRUE;
}

static BOOL setup(void)
{
    VkApplicationInfo app = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    VkInstanceCreateInfo ici = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    VkPhysicalDevice devs[16];
    uint32_t ndev = 16, i, family = ~0u;
    VkPhysicalDeviceProperties props;
    float prio = 1.0f;
    VkDeviceQueueCreateInfo qci = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    VkDeviceCreateInfo dci = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    VkAttachmentDescription att[2] = { { 0 }, { 0 } };
    VkAttachmentReference cref = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference dref = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
    VkSubpassDescription sub = { 0 };
    VkRenderPassCreateInfo rpi = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    VkImageView views[2];
    VkFramebufferCreateInfo fbi = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    VkCommandPoolCreateInfo cpi = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    VkCommandBufferAllocateInfo cai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    VkFenceCreateInfo fi = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    struct vertex *verts;
    UWORD *idx;
    ULONG nverts = SEGMENTS * SIDES;

    app.pApplicationName = "vktorusknot";
    app.apiVersion = VK_API_VERSION_1_0;
    ici.pApplicationInfo = &app;
    CHECK(vkCreateInstance(&ici, NULL, &instance));

#define F(n) if (!(p##n = (PFN_##n)vkGetInstanceProcAddr(instance, #n))) { tlog("no %s", #n); return FALSE; }
    VKFUNCS
#undef F

    CHECK(pvkEnumeratePhysicalDevices(instance, &ndev, devs));
    for (i = 0; i < ndev && family == ~0u; i++)
    {
        VkQueueFamilyProperties fam[16];
        uint32_t nfam = 16, f;

        pvkGetPhysicalDeviceQueueFamilyProperties(devs[i], &nfam, fam);
        for (f = 0; f < nfam; f++)
            if (fam[f].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                physical = devs[i];
                family = f;
                break;
            }
    }
    if (family == ~0u)
    {
        tlog("no device with a graphics queue (%u device(s))", (unsigned)ndev);
        return FALSE;
    }
    pvkGetPhysicalDeviceProperties(physical, &props);
    pvkGetPhysicalDeviceMemoryProperties(physical, &memprops);
    tlog("device: %s (Vulkan %u.%u.%u), queue family %u", props.deviceName,
        VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
        VK_VERSION_PATCH(props.apiVersion), (unsigned)family);

    qci.queueFamilyIndex = family;
    qci.queueCount = 1;
    qci.pQueuePriorities = &prio;
    dci.queueCreateInfoCount = 1;
    dci.pQueueCreateInfos = &qci;
    CHECK(pvkCreateDevice(physical, &dci, NULL, &device));
    pvkGetDeviceQueue(device, family, 0, &queue);

    if (!make_image(VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT, &colorimg, &colormem, &colorview) ||
        !make_image(VK_FORMAT_D16_UNORM, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT, &depthimg, &depthmem, &depthview))
        return FALSE;

    att[0].format = VK_FORMAT_R8G8B8A8_UNORM;
    att[0].samples = VK_SAMPLE_COUNT_1_BIT;
    att[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    att[1].format = VK_FORMAT_D16_UNORM;
    att[1].samples = VK_SAMPLE_COUNT_1_BIT;
    att[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    att[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sub.colorAttachmentCount = 1;
    sub.pColorAttachments = &cref;
    sub.pDepthStencilAttachment = &dref;
    rpi.attachmentCount = 2;
    rpi.pAttachments = att;
    rpi.subpassCount = 1;
    rpi.pSubpasses = &sub;
    CHECK(pvkCreateRenderPass(device, &rpi, NULL, &renderpass));

    views[0] = colorview;
    views[1] = depthview;
    fbi.renderPass = renderpass;
    fbi.attachmentCount = 2;
    fbi.pAttachments = views;
    fbi.width = WIDTH;
    fbi.height = HEIGHT;
    fbi.layers = 1;
    CHECK(pvkCreateFramebuffer(device, &fbi, NULL, &framebuffer));

    if (!make_pipeline())
        return FALSE;

    /* the mesh goes straight into host-visible buffers */
    nindices = SEGMENTS * SIDES * 6;
    if (!make_buffer(nverts * sizeof(struct vertex), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, &vb, &vbmem) ||
        !make_buffer(nindices * sizeof(UWORD), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, &ib, &ibmem) ||
        !make_buffer(WIDTH * HEIGHT * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, &rb, &rbmem))
        return FALSE;
    CHECK(pvkMapMemory(device, vbmem, 0, VK_WHOLE_SIZE, 0, (void **)&verts));
    CHECK(pvkMapMemory(device, ibmem, 0, VK_WHOLE_SIZE, 0, (void **)&idx));
    build_mesh(verts, idx);
    pvkUnmapMemory(device, vbmem);
    pvkUnmapMemory(device, ibmem);
    CHECK(pvkMapMemory(device, rbmem, 0, VK_WHOLE_SIZE, 0, (void **)&pixels));

    cpi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cpi.queueFamilyIndex = family;
    CHECK(pvkCreateCommandPool(device, &cpi, NULL, &cmdpool));
    cai.commandPool = cmdpool;
    cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cai.commandBufferCount = 1;
    CHECK(pvkAllocateCommandBuffers(device, &cai, &cmd));
    CHECK(pvkCreateFence(device, &fi, NULL, &fence));
    for (i = 0; i < memprops.memoryTypeCount; i++)
        tlog("memory type %u: heap %u flags 0x%x", (unsigned)i, (unsigned)memprops.memoryTypes[i].heapIndex,
            (unsigned)memprops.memoryTypes[i].propertyFlags);
    tlog("%u triangles, %ux%u, readback memory flags 0x%x%s", (unsigned)(nindices / 3), WIDTH, HEIGHT,
        (unsigned)rbflags, rbinvalidate ? " (invalidated per frame)" : "");
    return TRUE;
}

static void teardown(void)
{
    if (!device)
    {
        if (instance)
            pvkDestroyInstance ? pvkDestroyInstance(instance, NULL) : (void)0;
        return;
    }
    pvkDeviceWaitIdle(device);
    if (fence) pvkDestroyFence(device, fence, NULL);
    if (cmdpool) pvkDestroyCommandPool(device, cmdpool, NULL);
    if (pipeline) pvkDestroyPipeline(device, pipeline, NULL);
    if (layout) pvkDestroyPipelineLayout(device, layout, NULL);
    if (vsmod) pvkDestroyShaderModule(device, vsmod, NULL);
    if (fsmod) pvkDestroyShaderModule(device, fsmod, NULL);
    if (framebuffer) pvkDestroyFramebuffer(device, framebuffer, NULL);
    if (renderpass) pvkDestroyRenderPass(device, renderpass, NULL);
    if (pixels) pvkUnmapMemory(device, rbmem);
    if (vb) pvkDestroyBuffer(device, vb, NULL);
    if (ib) pvkDestroyBuffer(device, ib, NULL);
    if (rb) pvkDestroyBuffer(device, rb, NULL);
    if (colorview) pvkDestroyImageView(device, colorview, NULL);
    if (depthview) pvkDestroyImageView(device, depthview, NULL);
    if (colorimg) pvkDestroyImage(device, colorimg, NULL);
    if (depthimg) pvkDestroyImage(device, depthimg, NULL);
    if (vbmem) pvkFreeMemory(device, vbmem, NULL);
    if (ibmem) pvkFreeMemory(device, ibmem, NULL);
    if (rbmem) pvkFreeMemory(device, rbmem, NULL);
    if (colormem) pvkFreeMemory(device, colormem, NULL);
    if (depthmem) pvkFreeMemory(device, depthmem, NULL);
    pvkDestroyDevice(device, NULL);
    pvkDestroyInstance(instance, NULL);
}

/* ---- one frame: draw, copy to the readback buffer, wait ---- */

static BOOL render(float t)
{
    VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    VkClearValue clears[2];
    VkRenderPassBeginInfo rbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    VkDeviceSize zero = 0;
    VkBufferImageCopy region = { 0 };
    VkBufferMemoryBarrier hostbar = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
    VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    struct pushc pc;

    frame_matrices(&pc, t);

    CHECK(pvkResetCommandBuffer(cmd, 0));
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    CHECK(pvkBeginCommandBuffer(cmd, &bi));

    memcpy(clears[0].color.float32, clearcol, sizeof(clearcol));
    clears[1].depthStencil.depth = 1.0f;
    clears[1].depthStencil.stencil = 0;
    rbi.renderPass = renderpass;
    rbi.framebuffer = framebuffer;
    rbi.renderArea.extent.width = WIDTH;
    rbi.renderArea.extent.height = HEIGHT;
    rbi.clearValueCount = 2;
    rbi.pClearValues = clears;
    pvkCmdBeginRenderPass(cmd, &rbi, VK_SUBPASS_CONTENTS_INLINE);
    pvkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    pvkCmdBindVertexBuffers(cmd, 0, 1, &vb, &zero);
    pvkCmdBindIndexBuffer(cmd, ib, 0, VK_INDEX_TYPE_UINT16);
    pvkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
    pvkCmdDrawIndexed(cmd, nindices, 1, 0, 0, 0);
    pvkCmdEndRenderPass(cmd);

    /* the render pass left the image in TRANSFER_SRC_OPTIMAL */
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = WIDTH;
    region.imageExtent.height = HEIGHT;
    region.imageExtent.depth = 1;
    pvkCmdCopyImageToBuffer(cmd, colorimg, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, rb, 1, &region);

    hostbar.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    hostbar.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    hostbar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hostbar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    hostbar.buffer = rb;
    hostbar.size = VK_WHOLE_SIZE;
    pvkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT,
        0, 0, NULL, 1, &hostbar, 0, NULL);
    CHECK(pvkEndCommandBuffer(cmd));

    si.commandBufferCount = 1;
    si.pCommandBuffers = &cmd;
    CHECK(pvkResetFences(device, 1, &fence));
    CHECK(pvkQueueSubmit(queue, 1, &si, fence));
    CHECK(pvkWaitForFences(device, 1, &fence, VK_TRUE, FENCE_TIMEOUT));
    if (rbinvalidate)
    {
        VkMappedMemoryRange range = { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };

        range.memory = rbmem;
        range.size = VK_WHOLE_SIZE;
        CHECK(pvkInvalidateMappedMemoryRanges(device, 1, &range));
    }
    return TRUE;
}

/* ---- the first frame's check ---- */

static BOOL check_frame(void)
{
    static UBYTE first[WIDTH * HEIGHT * 4];
    UBYTE bg[3];
    ULONG covered = 0, x, y, minl = 255 * 3, maxl = 0, diff = 0;
    BOOL ok = TRUE;
    int i;

    for (i = 0; i < 3; i++)
        bg[i] = (UBYTE)(clearcol[i] * 255.0f + 0.5f);

    if (!render(0.0f))
        return FALSE;
    memcpy(first, pixels, sizeof(first));

    for (y = 0; y < HEIGHT; y++)
        for (x = 0; x < WIDTH; x++)
        {
            UBYTE *p = &first[(y * WIDTH + x) * 4];
            BOOL isbg = abs(p[0] - bg[0]) <= 1 && abs(p[1] - bg[1]) <= 1 && abs(p[2] - bg[2]) <= 1;

            if (!isbg)
            {
                ULONG l = p[0] + p[1] + p[2];

                covered++;
                if (l < minl) minl = l;
                if (l > maxl) maxl = l;
            }
            if ((x < 4 || x >= WIDTH - 4) && (y < 4 || y >= HEIGHT - 4) && !isbg)
            {
                if (ok)
                    tlog("FAIL: corner pixel %lu,%lu is %02x%02x%02x, not the clear colour %02x%02x%02x",
                        x, y, p[0], p[1], p[2], bg[0], bg[1], bg[2]);
                ok = FALSE;
            }
        }
    tlog("first frame: %lu of %lu pixels covered (%lu%%), brightness %lu..%lu",
        covered, (ULONG)(WIDTH * HEIGHT), covered * 100 / (WIDTH * HEIGHT), minl, maxl);
    if (covered < WIDTH * HEIGHT / 20 || covered > WIDTH * HEIGHT / 2)
    {
        tlog("FAIL: the knot covers %lu pixels, expected 5%%..50%% of the image", covered);
        ok = FALSE;
    }
    if (maxl - minl < 150)
    {
        tlog("FAIL: brightness range %lu..%lu, the lighting is missing", minl, maxl);
        ok = FALSE;
    }

    /* the same frame again must be the same pixels */
    if (!render(0.0f))
        return FALSE;
    for (i = 0; i < WIDTH * HEIGHT * 4; i++)
        if (pixels[i] != first[i])
            diff++;
    if (diff)
    {
        tlog("FAIL: rendering the same frame twice gave %lu different bytes", diff);
        ok = FALSE;
    }
    return ok;
}

int main(void)
{
    IPTR args[2] = { 0, 0 };
    struct RDArgs *rda = ReadArgs("FRAMES/K/N,NOWINDOW/S", args, NULL);
    ULONG frames = (rda && args[0]) ? *(ULONG *)args[0] : 0;
    BOOL nowindow = args[1] ? TRUE : FALSE;
    struct Window *win = NULL;
    BOOL ok, quit = FALSE;
    ULONG n = 0, second = 0;
    struct DateStamp ds0, ds, start;
    char title[64];

    if (rda)
        FreeArgs(rda);

    ok = setup() && check_frame();
    tlog("check: %s", ok ? "PASS" : "FAIL");

    if (ok && !nowindow)
    {
        win = OpenWindowTags(NULL,
            WA_Title, (IPTR)"VKTorusKnot",
            WA_InnerWidth, WIDTH,
            WA_InnerHeight, HEIGHT,
            WA_Left, 80,
            WA_Top, 80,
            WA_DragBar, TRUE,
            WA_DepthGadget, TRUE,
            WA_CloseGadget, TRUE,
            WA_Activate, TRUE,
            WA_GimmeZeroZero, TRUE,
            WA_IDCMP, IDCMP_CLOSEWINDOW,
            TAG_DONE);
        if (!win && !frames)
        {
            tlog("FAIL: no window, and no FRAMES to stop after");
            ok = FALSE;
        }
        else if (!win)
            tlog("no window: rendering without one");
    }

    /* NOWINDOW without FRAMES: the check alone */
    if (ok && (win || frames))
    {
        DateStamp(&ds0);
        start = ds0;
        while (!quit && (!frames || n < frames))
        {
            struct IntuiMessage *msg;

            if (!render(n * 0.02f))
            {
                ok = FALSE;
                break;
            }
            if (win)
            {
                WritePixelArray(pixels, 0, 0, WIDTH * 4, win->RPort, 0, 0, WIDTH, HEIGHT, RECTFMT_RGBA);
                while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort)))
                {
                    if (msg->Class == IDCMP_CLOSEWINDOW)
                        quit = TRUE;
                    ReplyMsg((struct Message *)msg);
                }
            }
            if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
                quit = TRUE;
            n++;
            second++;
            DateStamp(&ds);
            if ((ds.ds_Minute - ds0.ds_Minute) * 60 * TICKS_PER_SECOND + ds.ds_Tick - ds0.ds_Tick >= TICKS_PER_SECOND)
            {
                if (win)
                {
                    snprintf(title, sizeof(title), "VKTorusKnot, FPS: %lu", second);
                    SetWindowTitles(win, title, (CONST_STRPTR)~0);
                }
                else
                    tlog("%lu frames/s", second);
                second = 0;
                ds0 = ds;
            }
        }
        {
            LONG ticks;

            DateStamp(&ds);
            ticks = ((ds.ds_Days - start.ds_Days) * 1440 + ds.ds_Minute - start.ds_Minute) * 60 * TICKS_PER_SECOND
                    + ds.ds_Tick - start.ds_Tick;
            tlog("%lu frames in %ld.%02ld s: %ld frames/s", n, ticks / TICKS_PER_SECOND,
                (ticks % TICKS_PER_SECOND) * 100 / TICKS_PER_SECOND,
                ticks > 0 ? (LONG)(n * TICKS_PER_SECOND / ticks) : 0L);
        }
    }

    if (win)
        CloseWindow(win);
    teardown();
    tlog("%s", ok ? "PASS" : "FAIL");
    return ok ? RETURN_OK : RETURN_FAIL;
}
