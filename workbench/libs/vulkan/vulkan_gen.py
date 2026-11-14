#!/usr/bin/env python3
#
# Copyright (C) 2026, The AROS Development Team. All rights reserved.
#
# vulkan_gen.py vk.xml outdir: generates, from the Vulkan registry,
#
#   vulkan.conf         the library's function table: the native vkA* calls,
#                       then the core API in registry order (1.0 .. 1.4),
#                       VK_KHR_surface and VK_KHR_swapchain. The order is the
#                       library's ABI: only ever append to it.
#   vulkan_dispatch.h   struct vkl_dispatch, one pointer per command that
#                       takes a dispatchable object
#   vulkan_tramp.c      the public entry of each such command (it jumps
#                       through the table in its first argument's loader
#                       word) and the name table vkGet*ProcAddr search
#
# The output is committed: the library builds without the Mesa sources.
# Run it again (against the vk.xml of the Mesa version in the tree) when the
# Vulkan headers are updated.

import sys
import xml.etree.ElementTree as ET

# the public symbol of these is written by hand (vulkan_instance.c ...)
HANDWRITTEN = {
    'vkCreateInstance', 'vkEnumerateInstanceVersion',
    'vkEnumerateInstanceLayerProperties',
    'vkEnumerateInstanceExtensionProperties',
    'vkDestroyInstance', 'vkEnumeratePhysicalDevices',
    'vkEnumeratePhysicalDeviceGroups', 'vkEnumeratePhysicalDeviceGroupsKHR',
    'vkGetInstanceProcAddr', 'vkGetDeviceProcAddr',
    'vkDestroySurfaceKHR',
}

GLOBALS = {
    'vkCreateInstance', 'vkEnumerateInstanceVersion',
    'vkEnumerateInstanceLayerProperties',
    'vkEnumerateInstanceExtensionProperties',
}

LEVELS = {
    'VkInstance': 'INSTANCE',
    'VkPhysicalDevice': 'PHYSDEV',
    'VkDevice': 'DEVICE',
    'VkQueue': 'DEVICE',
    'VkCommandBuffer': 'DEVICE',
}

EXPORT_EXTENSIONS = ['VK_KHR_surface', 'VK_KHR_swapchain']

NATIVE = '''VkResult vkACreateSurface(VkInstance instance, struct TagItem *tagList, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
IPTR vkAGetSurfaceAttr(VkInstance instance, VkSurfaceKHR surface, ULONG attr)
VkResult vkCreateAROSSurfaceAROS(VkInstance instance, const VkAROSSurfaceCreateInfoAROS *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface)
.skip 13
'''

CONF_HEAD = '''##begin config
version 1.0
basename Vulkan
libbase VulkanBase
libbasetype struct VulkanLibBase
copyright Copyright (C) 2026 The AROS Development Team
##end config
##begin cdef
#include <exec/types.h>
#include <utility/tagitem.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_aros.h>

__BEGIN_DECLS

VkResult vkACreateSurfaceTags(VkInstance instance, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface, Tag tag1, ...);

__END_DECLS

##end cdef
##begin cdefprivate
#include "vulkan_intern.h"
##end cdefprivate
##begin functionlist
'''


def api_ok(el):
    api = el.get('api')
    return api is None or 'vulkan' in api.split(',')


class Command:
    pass


def text_of(el):
    return ' '.join(''.join(el.itertext()).split())


def parse_commands(root):
    cmds = {}
    aliases = []
    for c in root.find('commands'):
        if c.tag != 'command' or not api_ok(c):
            continue
        if c.get('alias'):
            aliases.append((c.get('name'), c.get('alias')))
            continue
        proto = c.find('proto')
        cmd = Command()
        cmd.name = proto.find('name').text
        cmd.ret = text_of(proto)[:-len(cmd.name)].strip()
        cmd.params = []
        for p in c.findall('param'):
            if not api_ok(p):
                continue
            decl = text_of(p)
            pname = p.find('name').text
            ptype = p.find('type').text
            cmd.params.append((decl, pname, ptype))
        cmds[cmd.name] = cmd
    for name, target in aliases:
        if target in cmds:
            a = Command()
            a.name = name
            a.ret = cmds[target].ret
            a.params = cmds[target].params
            cmds[name] = a
    return cmds


def feature_commands(root):
    feats = []
    for f in root.findall('feature'):
        if not api_ok(f):
            continue
        names = []
        for req in f.findall('require'):
            if not api_ok(req):
                continue
            for c in req.findall('command'):
                if api_ok(c):
                    names.append(c.get('name'))
        feats.append((float(f.get('number')), names))
    out = []
    # stable: by version, then registry order
    for num, names in sorted(feats, key=lambda x: x[0]):
        for n in names:
            if n not in out:
                out.append(n)
    return out


def extension_commands(root):
    """commands of every extension an AROS build can declare: no platform
    (window system) types, not provisional, not disabled"""
    exts = {}
    for e in root.find('extensions'):
        sup = e.get('supported', '')
        if 'vulkan' not in sup.split(','):
            continue
        if e.get('platform') or e.get('provisional') == 'true':
            continue
        names = []
        for req in e.findall('require'):
            if not api_ok(req):
                continue
            for c in req.findall('command'):
                if api_ok(c):
                    names.append(c.get('name'))
        exts[e.get('name')] = names
    return exts


def conf_decl(decl, pname):
    # genmodule takes "type name": an array parameter becomes a pointer
    if '[' in decl:
        base = decl[:decl.index('[')].rstrip()
        assert base.endswith(pname)
        return base[:-len(pname)].rstrip() + ' *' + pname
    return decl


def main():
    xml, outdir = sys.argv[1], sys.argv[2]
    root = ET.parse(xml).getroot()
    cmds = parse_commands(root)
    core = [n for n in feature_commands(root) if n in cmds]
    exts = extension_commands(root)

    exported = list(core)
    for e in EXPORT_EXTENSIONS:
        for n in exts[e]:
            if n not in exported:
                exported.append(n)

    allnames = list(exported)
    for e in sorted(exts):
        for n in exts[e]:
            if n in cmds and n not in allnames:
                allnames.append(n)

    def level(n):
        if n in GLOBALS:
            return 'GLOBAL'
        c = cmds[n]
        if not c.params:
            return None
        return LEVELS.get(c.params[0][2])

    table = [n for n in allnames if level(n) in ('INSTANCE', 'PHYSDEV', 'DEVICE')]
    exported = [n for n in exported if level(n)]

    # ---- vulkan.conf ----
    with open(outdir + '/vulkan.conf', 'w') as f:
        f.write(CONF_HEAD)
        f.write(NATIVE)
        for n in exported:
            c = cmds[n]
            args = ', '.join(conf_decl(d, p) for d, p, t in c.params)
            f.write('%s %s(%s)\n' % (c.ret, n, args))
        f.write('##end functionlist\n')

    banner = '/* Generated by vulkan_gen.py from vk.xml. Do not edit. */\n\n'

    # ---- vulkan_dispatch.h ----
    with open(outdir + '/vulkan_dispatch.h', 'w') as f:
        f.write(banner)
        f.write('#ifndef VULKAN_DISPATCH_H\n#define VULKAN_DISPATCH_H\n\n')
        f.write('struct vkl_dispatch\n{\n')
        for n in table:
            f.write('    PFN_%s %s;\n' % (n, n[2:]))
        f.write('};\n\n')
        f.write('#define VKL_NUM_COMMANDS %d\n\n' % len(table))
        f.write('#endif /* VULKAN_DISPATCH_H */\n')

    # ---- vulkan_tramp.c ----
    with open(outdir + '/vulkan_tramp.c', 'w') as f:
        f.write(banner)
        f.write('#include "vulkan_intern.h"\n\n')
        for n in table:
            if n in HANDWRITTEN:
                continue
            c = cmds[n]
            decls = ', '.join(d for d, p, t in c.params)
            lv = level(n)
            ret = '' if c.ret == 'void' else 'return '
            f.write('VKAPI_ATTR %s VKAPI_CALL %s(%s)\n{\n' % (c.ret, n, decls))
            if lv == 'INSTANCE':
                # extension commands on the instance go to its first driver
                args = ', '.join(['VKL_ICD_INSTANCE(%s)' % c.params[0][1]] +
                                 [p for d, p, t in c.params[1:]])
                f.write('    %sVKL_INSTANCE_DISPATCH(%s)->%s(%s);\n' %
                        (ret, c.params[0][1], n[2:], args))
            else:
                args = ', '.join(p for d, p, t in c.params)
                f.write('    %sVKL_DISPATCH(%s)->%s(%s);\n' %
                        (ret, c.params[0][1], n[2:], args))
            f.write('}\n\n')
        f.write('const struct vkl_command vkl_commands[VKL_NUM_COMMANDS] =\n{\n')
        for n in sorted(table):
            f.write('    { "%s", (PFN_vkVoidFunction)%s, offsetof(struct vkl_dispatch, %s), VKL_LEVEL_%s },\n' %
                    (n, n, n[2:], level(n)))
        f.write('};\n')

    print('%d exported, %d dispatched commands' % (len(exported), len(table)))


if __name__ == '__main__':
    main()
