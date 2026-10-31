/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Fixture for the sizePCIBaseReg() host test.  run_sizebar.py prepends
    an #include of the real rom/hidds/pci/include/hardware/pci.h (from the
    source root under test) to this file, then the production function
    extracted verbatim, then test_sizebar.c.  This file includes nothing
    of its own and never copies register constants: it supplies only
    scalar stand-ins, the opaque object types, the four HIDD_PCIDriver_*
    config-register macros routed to a scripted config space, and a
    __WORDSIZE fallback.
*/
#ifndef PCI_HOST_FIXTURE_H
#define PCI_HOST_FIXTURE_H

/* Host headers define no AROS scalars; these stand in at the widths
 * sizePCIBaseReg() documents: ULONG is always 32 bits (AROS), IPTR is
 * the pointer width of the compilation. */
typedef int BOOL;
typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned int ULONG;
typedef unsigned long IPTR;

#ifndef __WORDSIZE
#define __WORDSIZE (__SIZEOF_POINTER__ * 8) /* host headers lack it:
                                             * derive from pointer width */
#endif

typedef struct OOP_Object_S { int opaque; } OOP_Object;

struct pci_staticdata { int opaque; };

struct DeviceData { int isBridge; UBYTE Class; UBYTE SubClass; };

/* ---- scripted config space ---------------------------------------- *
 * BAR value + read-back size mask per BAR slot; a read after an
 * all-ones write yields the scripted mask, otherwise the scripted
 * value.  The command register lives in fake_cmd, decoded at first.
 */
static unsigned int fake_cmd = PCICMF_IODECODE | PCICMF_MEMDECODE | PCICMF_BUSMASTER;
static int decode_violations;
static unsigned int cfg_value[6];
static unsigned int cfg_last_write[6];
static unsigned int cfg_mask[6];
static int cfg_written[6];

static unsigned int fake_cfg_read(unsigned reg)
{
    unsigned i = (reg - PCICS_BAR0) >> 2;

    if (i < 6 && cfg_written[i])
        return cfg_mask[i];
    if (i < 6)
        return cfg_value[i];
    return 0;
}

static void fake_cfg_write(unsigned reg, unsigned int val)
{
    unsigned i = (reg - PCICS_BAR0) >> 2;

    if (i < 6)
    {
        if (fake_cmd & (PCICMF_IODECODE | PCICMF_MEMDECODE))
            decode_violations++;
        cfg_written[i] = (val == ~0u);
        cfg_last_write[i] = val;
    }
}

static void setup_bar(unsigned idx, unsigned int value, unsigned int mask)
{
    cfg_value[idx] = value;
    cfg_mask[idx] = mask;
    cfg_written[idx] = 0;
}

static unsigned short fake_cfg_read_w(unsigned reg)
{
    if (reg == PCICS_COMMAND)
        return (unsigned short)fake_cmd;
    return 0;
}

static void fake_cfg_write_w(unsigned reg, unsigned int val)
{
    if (reg == PCICS_COMMAND)
        fake_cmd = val;
}

#define HIDD_PCIDriver_ReadConfigLong(driver, o, bus, dev, sub, reg) \
    fake_cfg_read(reg)
#define HIDD_PCIDriver_WriteConfigLong(driver, o, bus, dev, sub, reg, val) \
    fake_cfg_write(reg, val)
#define HIDD_PCIDriver_ReadConfigWord(driver, o, bus, dev, sub, reg) \
    fake_cfg_read_w(reg)
#define HIDD_PCIDriver_WriteConfigWord(driver, o, bus, dev, sub, reg, val) \
    fake_cfg_write_w(reg, val)

#endif /* PCI_HOST_FIXTURE_H */
