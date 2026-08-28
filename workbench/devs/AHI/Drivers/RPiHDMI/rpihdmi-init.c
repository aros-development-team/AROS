#define DEBUG 0
#include <aros/debug.h>
#include <config.h>

#include <proto/openfirmware.h>
#include <proto/exec.h>
#include <proto/kernel.h>
#include <proto/dma.h>
#include <aros/macros.h>
#include <string.h>

#include "hardware/bcm2708.h"
#include "library.h"
#include "DriverData.h"
#include "rpihdmi-soc.h"

APTR KernelBase = NULL;
APTR DMABase = NULL;
APTR OpenFirmwareBase = NULL;

static void *find_hdmi_node(struct DriverBase *AHIsubBase, char *compatible)
{
    if (OpenFirmwareBase == NULL)
        return NULL;

    return OF_FindNodeByCompatible(NULL, compatible);
}

static ULONG get_device_tree_reg_value(struct DriverBase *AHIsubBase, void *node, LONG index)
{
    void *prop = OF_FindProperty(node, "reg");

    if (index < 0 || prop == NULL) {
       return 0;
    }

    const ULONG *cells = OF_GetPropValue(prop);
    ULONG len = OF_GetPropLen(prop) / sizeof(ULONG);

    if ((ULONG)(index * 2) >= len)
        return 0;

    return AROS_BE2LONG(cells[(size_t)(index * 2)]);
}

static ULONG find_dreq(struct DriverBase *AHIsubBase, void *node)
{
    void *prop = OF_FindProperty(node, "dmas");
    const ULONG *cells;
    ULONG len;

    if (prop != NULL) {
        cells = OF_GetPropValue(prop);
        len = OF_GetPropLen(prop) / sizeof(ULONG);

        if (len > 1) {
            ULONG dma_spec = AROS_BE2LONG(cells[1]);
            return dma_spec & 0xff;
        }
    }

    return 0;
}

/* "hdmi\0dvp\0phy\0rm\0packet\0..." */
static LONG find_element_index(struct DriverBase *AHIsubBase, void *node, char *name, char *element)
{
    void *prop = OF_FindProperty(node, name);
    if (prop == NULL) {
        bug("[RPiHDMI] Failed to find device tree property: %s\n", name);
        return -1;
    }

    const char *value = OF_GetPropValue(prop);
    ULONG len = OF_GetPropLen(prop);
    ULONG element_index = 0;

    for (ULONG i = 0; i < len; ) {
        if (value[i] != '\0') {
            if (strcmp(&value[i], element) == 0)
                return (int)element_index;

            element_index++;

            while (i < len && value[i] != '\0')
                i++;
        }

        while (i < len && value[i] == '\0')
            i++;
    }

    return -1;
}

static BOOL fill_reg_values(struct DriverBase *AHIsubBase, struct RPiHDMISoc *soc, void *node, char *compatible) {
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *) AHIsubBase;
    ULONG bus_peribase = (RPiHDMIBase->periiobase == BCM2712_PERIIOBASE) ? BCM2712_BUS_PERIIOBASE : BCM2711_BUS_PERIIOBASE;
    LONG hdmi_index =
        find_element_index(AHIsubBase, node, "reg-names", "hdmi");
    LONG hd_index =
        find_element_index(AHIsubBase, node, "reg-names", "hd");
    LONG packet_index =
        find_element_index(AHIsubBase, node, "reg-names", "packet");

    if (hdmi_index < 0 || hd_index < 0 || packet_index < 0) {
        bug("[RPiHDMI] Missing HDMI register block, using fallback\n");
        return FALSE;
    }

    ULONG packet_base =
        get_device_tree_reg_value(AHIsubBase, node, packet_index);
    ULONG hdmi_base =
        get_device_tree_reg_value(AHIsubBase, node, hdmi_index);
    ULONG mai_base =
        get_device_tree_reg_value(AHIsubBase, node, hd_index);

    if (hdmi_base == 0 || packet_base == 0 || mai_base == 0) {
        bug("[RPiHDMI] Missing HDMI register values, using fallback\n");
        return FALSE;
    }

    soc->hdmi_base = hdmi_base - bus_peribase;
    soc->mai_base = mai_base - bus_peribase;
    soc->packet_base = packet_base - bus_peribase;

    D(bug("[RPiHDMI] hdmi_base=%08x mai_base=%08x packet_base=%08x\n",
        (ULONG)soc->hdmi_base,
        (ULONG)soc->mai_base,
        (ULONG)soc->packet_base));

    soc->mai_data_bus = mai_base + soc->regs.mai_data;
    return TRUE;
}

static BOOL fill_dreq_value(struct DriverBase *AHIsubBase, struct RPiHDMISoc *soc, void *node, char *compatible) {
    ULONG dreq_found = find_dreq(AHIsubBase, node);
    if (dreq_found > 0) {
        soc->dma_dreq = dreq_found;
    } else {
        bug("[RPiHDMI] Couldn't find DREQ device tree value, using fallback\n");
        return FALSE;
    }
    return TRUE;
}

static BOOL travers_device_tree_and_fill_soc(struct DriverBase *AHIsubBase, struct RPiHDMISoc *soc, char *compatible)
{
    void *node = find_hdmi_node(AHIsubBase, compatible);

    if (node != NULL) {
        if (!fill_reg_values(AHIsubBase, soc, node, compatible)) {
            return FALSE;
        }
        if (!fill_dreq_value(AHIsubBase, soc, node, compatible)) {
            return FALSE;
        }
        return TRUE;
    } else {
        bug("[RPiHDMI] Failed to find hdmi node in device tree for %s, using fallback\n", compatible);
        return FALSE;
    }
}

/******************************************************************************
** Custom driver init *********************************************************
******************************************************************************/

BOOL DriverInit(struct DriverBase *AHIsubBase)
{
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *) AHIsubBase;

    RPiHDMIBase->dosbase = (struct DosLibrary *) OpenLibrary(DOSNAME, 37);

    if (RPiHDMIBase->dosbase == NULL)
    {
        Req("Unable to open 'dos.library' version 37.\n");
        return FALSE;
    }

    KernelBase = OpenResource("kernel.resource");

    if (KernelBase == NULL) {
        Req("Unable to open 'kernel.resource'.\n");
        return FALSE;
    }

    DMABase = OpenResource("dma.resource");

    if (DMABase == NULL)
    {
        Req("Unable to open 'dma.resource'.\n");
        return FALSE;
    }

    RPiHDMIBase->periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);

    if (RPiHDMIBase->periiobase == 0)
    {
        Req("No BCM283x peripheral base found.\n");
        return FALSE;
    }

    if (RPiHDMIBase->periiobase == BCM2712_PERIIOBASE)
    {
        OpenFirmwareBase = OpenResource("openfirmware.resource");

        if (OpenFirmwareBase == NULL) {
            Req("Unable to open 'openfirmware.resource'.\n");
            return FALSE;
        }

        UWORD num = 0;

        if (travers_device_tree_and_fill_soc(AHIsubBase, &rpihdmi_bcm2712_hdmi0_soc, "brcm,bcm2712-hdmi0"))
            RPiHDMIBase->soc[num++] = &rpihdmi_bcm2712_hdmi0_soc;

        if (travers_device_tree_and_fill_soc(AHIsubBase, &rpihdmi_bcm2712_hdmi1_soc, "brcm,bcm2712-hdmi1"))
            RPiHDMIBase->soc[num++] = &rpihdmi_bcm2712_hdmi1_soc;

        RPiHDMIBase->num_outputs = num;

        if (num == 0) {
            Req("Unsupported Raspberry Pi HDMI audio hardware.\n");
            return FALSE;
        }
    }
    else if (RPiHDMIBase->periiobase == BCM2711_PERIIOBASE)
    {
        OpenFirmwareBase = OpenResource("openfirmware.resource");

        if (OpenFirmwareBase == NULL) {
            Req("Unable to open 'openfirmware.resource'.\n");
            return FALSE;
        }

        UWORD num = 0;

        if (travers_device_tree_and_fill_soc(AHIsubBase, &rpihdmi_bcm2711_hdmi0_soc, "brcm,bcm2711-hdmi0"))
            RPiHDMIBase->soc[num++] = &rpihdmi_bcm2711_hdmi0_soc;

        if (travers_device_tree_and_fill_soc(AHIsubBase, &rpihdmi_bcm2711_hdmi1_soc, "brcm,bcm2711-hdmi1"))
            RPiHDMIBase->soc[num++] = &rpihdmi_bcm2711_hdmi1_soc;

        RPiHDMIBase->num_outputs = num;

        if (num == 0) {
            Req("Unsupported Raspberry Pi HDMI audio hardware.\n");
            return FALSE;
        }
    }
    else if (RPiHDMIBase->periiobase == BCM2836_PERIPHYSBASE || RPiHDMIBase->periiobase == BCM2835_PERIPHYSBASE)
    {
        RPiHDMIBase->soc[0] = &rpihdmi_bcm283x_soc;
        RPiHDMIBase->num_outputs = 1;
    }
    else
    {
        Req("Unsupported Raspberry Pi HDMI audio hardware.\n");
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
** Custom driver clean-up *****************************************************
******************************************************************************/

VOID DriverCleanup(struct DriverBase *AHIsubBase)
{
    struct RPiHDMIBase *RPiHDMIBase = (struct RPiHDMIBase *) AHIsubBase;

    CloseLibrary((struct Library *) DOSBase);
}
