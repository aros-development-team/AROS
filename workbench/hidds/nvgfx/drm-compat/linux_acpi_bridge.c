/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    The acpica.library side of the Linux ACPI interface (linux_acpi.c).
    Built with ACPICA's headers only: the Linux side sees plain types.
*/

/* acpica.library's functions take C arguments, called through ACPICABase */
#define __INLINE_ACPICA_STACKCALL__

#include <aros/symbolsets.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/acpica.h>

#include <acpica/acnames.h>
#include <acpica/accommon.h>

#include "linux_acpi_bridge.h"

struct Library *ACPICABase;
static BOOL acpica_tried;

/* acpica.library, opened on first use; NULL without ACPI */
int acpi_bridge_open(void)
{
    if (!acpica_tried) {
        acpica_tried = TRUE;
        ACPICABase = OpenLibrary("acpica.library", 0);
    }
    return ACPICABase != NULL;
}

static int acpi_bridge_close(void)
{
    if (ACPICABase)
        CloseLibrary(ACPICABase);
    ACPICABase = NULL;
    acpica_tried = FALSE;
    return TRUE;
}
ADD2EXPUNGELIB(acpi_bridge_close, 0)

unsigned long acpi_bridge_object_size(void)
{
    return sizeof(ACPI_OBJECT);
}

unsigned int acpi_bridge_evaluate(void *handle, const char *path, unsigned int argc,
                                  void *argv, void **result)
{
    ACPI_OBJECT_LIST args = { argc, argv };
    ACPI_BUFFER out = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_STATUS status;

    *result = NULL;
    if (!acpi_bridge_open())
        return AE_NOT_FOUND;
    status = AcpiEvaluateObject(handle, (ACPI_STRING)path, argv ? &args : NULL, &out);
    if (ACPI_SUCCESS(status))
        *result = out.Pointer;
    return status;
}

void acpi_bridge_free(void *buffer)
{
    /* ACPICA allocates what it hands out with AllocVec() (acpios_aros.c) */
    FreeVec(buffer);
}

unsigned int acpi_bridge_get_handle(void *parent, const char *path, void **handle)
{
    if (!acpi_bridge_open())
        return AE_NOT_FOUND;
    return AcpiGetHandle(parent, path, (ACPI_HANDLE *)handle);
}

unsigned int acpi_bridge_get_name(void *handle, unsigned int type, char **name)
{
    ACPI_BUFFER out = { ACPI_ALLOCATE_BUFFER, NULL };
    ACPI_STATUS status;

    *name = NULL;
    if (!acpi_bridge_open())
        return AE_NOT_FOUND;
    status = AcpiGetName(handle, type, &out);
    if (ACPI_SUCCESS(status))
        *name = out.Pointer;
    return status;
}

unsigned int acpi_bridge_get_next_object(unsigned int type, void *parent, void *child, void **next)
{
    if (!acpi_bridge_open())
        return AE_NOT_FOUND;
    return AcpiGetNextObject(type, parent, child, (ACPI_HANDLE *)next);
}

const char *acpi_bridge_format_exception(unsigned int status)
{
    if (!acpi_bridge_open())
        return "AE_NOT_FOUND";
    return AcpiFormatException(status);
}

static BOOL get_integer(ACPI_HANDLE handle, const char *name, UINT64 *value)
{
    ACPI_OBJECT obj;
    ACPI_BUFFER out = { sizeof(obj), &obj };

    if (ACPI_FAILURE(AcpiEvaluateObjectTyped(handle, (ACPI_STRING)name, NULL, &out,
                                            ACPI_TYPE_INTEGER)))
        return FALSE;
    *value = obj.Integer.Value;
    return TRUE;
}

/*
 * The device node of PCI function bus:devfn: from each root bridge, follow
 * the children whose _ADR names a bridge on the path down to that bus.
 */
struct pci_walk {
    unsigned int bus, devfn;
    acpi_bridge_cfg8 cfg8;
    ACPI_HANDLE found;
};

static ACPI_HANDLE walk_bus(struct pci_walk *pw, ACPI_HANDLE node, unsigned int bus, int depth)
{
    ACPI_HANDLE child = NULL, found;
    UINT64 adr;

    if (depth > 8)
        return NULL;
    while (ACPI_SUCCESS(AcpiGetNextObject(ACPI_TYPE_DEVICE, node, child, &child)) && child) {
        unsigned int dev, fn, devfn, secondary, subordinate;

        if (!get_integer(child, METHOD_NAME__ADR, &adr))
            continue;
        dev = (adr >> 16) & 0x1f;
        fn = adr & 0xffff;
        if (fn == 0xffff)       /* all functions of the device */
            fn = 0;
        devfn = (dev << 3) | (fn & 7);
        if (bus == pw->bus && devfn == pw->devfn)
            return child;
        /* a PCI-to-PCI bridge whose bus range holds the target */
        if ((pw->cfg8(bus, devfn, 0x0e) & 0x7f) != 1)
            continue;
        secondary = pw->cfg8(bus, devfn, 0x19);
        subordinate = pw->cfg8(bus, devfn, 0x1a);
        if (secondary == 0 || secondary <= bus || pw->bus < secondary || pw->bus > subordinate)
            continue;
        if ((found = walk_bus(pw, child, secondary, depth + 1)))
            return found;
    }
    return NULL;
}

static ACPI_STATUS root_bridge(ACPI_HANDLE handle, UINT32 level, void *context, void **ret)
{
    struct pci_walk *pw = context;
    UINT64 seg = 0, bbn = 0;

    (void)level;
    (void)ret;
    if (pw->found)
        return AE_CTRL_TERMINATE;
    get_integer(handle, METHOD_NAME__SEG, &seg);
    get_integer(handle, METHOD_NAME__BBN, &bbn);
    if (seg != 0 || bbn > pw->bus)
        return AE_OK;
    pw->found = walk_bus(pw, handle, (unsigned int)bbn, 0);
    return pw->found ? AE_CTRL_TERMINATE : AE_OK;
}

void *acpi_bridge_find_pci(unsigned int bus, unsigned int devfn, acpi_bridge_cfg8 cfg8)
{
    struct pci_walk pw = { bus, devfn, cfg8, NULL };

    if (!acpi_bridge_open())
        return NULL;
    /* PCI Express root bridges, then conventional ones */
    AcpiGetDevices("PNP0A08", root_bridge, &pw, NULL);
    if (!pw.found)
        AcpiGetDevices("PNP0A03", root_bridge, &pw, NULL);
    return pw.found;
}
