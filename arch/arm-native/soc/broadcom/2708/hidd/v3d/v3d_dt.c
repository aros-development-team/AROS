/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    VideoCore VI (V3D) - block addresses from the device tree.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/openfirmware.h>

#include <string.h>

#include "v3d_intern.h"

IPTR v3d_pm_base;
IPTR v3d_asb_base;

/* The 2711 node sits on a one-cell bus, the 2712 on a two-cell one. PM holds
 * the reset; the 2712 has its own pm block, not the legacy one. */
static const struct
{
    const char *compat;
    const char *parent;
    const char *pmpath;
    const char *pmparent;
    ULONG       asb_offset;     /* no DT node describes it; 0 = none */
} v3d_variants[] =
{
    { "brcm,2711-v3d", "/v3dbus", "/soc/watchdog@7e100000",
      "/soc", V3D_ASB_OFFSET },
    { "brcm,2712-v3d", "/axi",    "/soc@107c000000/watchdog@7d200000",
      "/soc@107c000000", 0 },
};

/* Named for the OF_ macros, which call through a base of this name. */
static uint32_t of_cells(APTR OpenFirmwareBase, const char *path,
                         const char *which, uint32_t dflt)
{
    void *key = OF_OpenKey((char *)path);
    void *prop = key ? OF_FindProperty(key, (char *)which) : NULL;

    return prop ? AROS_BE2LONG(*(const uint32_t *)OF_GetPropValue(prop)) : dflt;
}

/* /v3dbus maps the 2711 blocks into the peripheral window; /axi on the
 * 2712 is 1:1. No ranges = identity. */
static IPTR of_translate(APTR OpenFirmwareBase, const char *parent,
                         uint64_t child, uint32_t ac, uint32_t sc)
{
    void *key = OF_OpenKey((char *)parent);
    void *prop = key ? OF_FindProperty(key, "ranges") : NULL;
    const uint32_t *r;
    uint32_t root_ac, stride, cells, i;

    if (!prop || OF_GetPropLen(prop) == 0)
        return (IPTR)child;

    root_ac = of_cells(OpenFirmwareBase, "/", "#address-cells", 2);
    stride  = ac + root_ac + sc;
    r       = (const uint32_t *)OF_GetPropValue(prop);
    cells   = OF_GetPropLen(prop) / 4;

    for (; cells >= stride; cells -= stride, r += stride)
    {
        uint64_t cbase = 0, pbase = 0, len = 0;

        for (i = 0; i < ac; i++)
            cbase = (cbase << 32) | AROS_BE2LONG(r[i]);
        for (i = 0; i < root_ac; i++)
            pbase = (pbase << 32) | AROS_BE2LONG(r[ac + i]);
        for (i = 0; i < sc; i++)
            len = (len << 32) | AROS_BE2LONG(r[ac + root_ac + i]);

        if (child >= cbase && child - cbase < len)
            return (IPTR)(pbase + (child - cbase));
    }

    return (IPTR)child;
}

static IPTR of_first_reg(APTR OpenFirmwareBase, const char *path,
                         const char *parent, uint32_t ac, uint32_t sc)
{
    void *key = OF_OpenKey((char *)path);
    void *prop = key ? OF_FindProperty(key, "reg") : NULL;
    const uint32_t *r;
    uint64_t addr = 0;
    uint32_t i;

    if (!prop || OF_GetPropLen(prop) < (LONG)(ac * 4))
        return 0;

    r = (const uint32_t *)OF_GetPropValue(prop);
    for (i = 0; i < ac; i++)
        addr = (addr << 32) | AROS_BE2LONG(r[i]);

    return of_translate(OpenFirmwareBase, parent, addr, ac, sc);
}

BOOL v3d_probe_dt(struct V3DData *sd)
{
    APTR OpenFirmwareBase = OpenResource("openfirmware.resource");
    unsigned int v;

    if (!OpenFirmwareBase)
    {
        D(bug("[V3D] no openfirmware.resource\n"));
        return FALSE;
    }

    for (v = 0; v < sizeof(v3d_variants) / sizeof(v3d_variants[0]); v++)
    {
        void *key = OF_FindNodeByCompatible(NULL, (char *)v3d_variants[v].compat);
        void *prop;
        const uint32_t *r;
        uint32_t ac, sc, stride, entries;

        if (!key)
            continue;

        ac = of_cells(OpenFirmwareBase, v3d_variants[v].parent, "#address-cells", 2);
        sc = of_cells(OpenFirmwareBase, v3d_variants[v].parent, "#size-cells", 2);
        stride = ac + sc;

        prop = OF_FindProperty(key, "reg");
        if (!prop || stride == 0)
            continue;

        entries = (OF_GetPropLen(prop) / 4) / stride;
        if (entries < 2)
        {
            D(bug("[V3D] %s: reg has %u entries, need hub and core0\n",
                  v3d_variants[v].compat, entries));
            continue;
        }

        r = (const uint32_t *)OF_GetPropValue(prop);
        {
            uint32_t e, i;
            IPTR base[3] = { 0, 0, 0 };

            for (e = 0; e < entries && e < 3; e++)
            {
                uint64_t a = 0;

                for (i = 0; i < ac; i++)
                    a = (a << 32) | AROS_BE2LONG(r[e * stride + i]);
                base[e] = of_translate(OpenFirmwareBase,
                                       v3d_variants[v].parent, a, ac, sc);
            }

            sd->hub_base   = base[0];
            sd->core0_base = base[1];
            sd->sms_base   = base[2];
        }

        v3d_pm_base  = of_first_reg(OpenFirmwareBase, v3d_variants[v].pmpath,
                                    v3d_variants[v].pmparent,
                                    of_cells(OpenFirmwareBase, v3d_variants[v].pmparent,
                                             "#address-cells", 1),
                                    of_cells(OpenFirmwareBase, v3d_variants[v].pmparent,
                                             "#size-cells", 1));
        v3d_asb_base = v3d_variants[v].asb_offset
                     ? ARM_PERIIOBASE + v3d_variants[v].asb_offset : 0;

        bug("[V3D] %s: hub 0x%p core0 0x%p sms 0x%p pm 0x%p asb 0x%p\n",
            v3d_variants[v].compat, (APTR)sd->hub_base, (APTR)sd->core0_base,
            (APTR)sd->sms_base, (APTR)v3d_pm_base, (APTR)v3d_asb_base);

        return sd->hub_base && sd->core0_base && v3d_pm_base;
    }

    D(bug("[V3D] no v3d node in the device tree\n"));
    return FALSE;
}
