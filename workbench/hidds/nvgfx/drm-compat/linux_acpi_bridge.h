/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Between linux_acpi.c (Linux types) and linux_acpi_bridge.c (ACPICA
    types): plain C types only. Handles and objects are ACPICA's; status
    values are ACPICA's AE_ codes.
*/
#ifndef LINUX_ACPI_BRIDGE_H
#define LINUX_ACPI_BRIDGE_H

/* reads byte `where` of PCI function bus:devfn's configuration space */
typedef unsigned char (*acpi_bridge_cfg8)(unsigned int bus, unsigned int devfn, int where);

int acpi_bridge_open(void);
unsigned long acpi_bridge_object_size(void);
/* *result is an ACPICA-allocated object tree: acpi_bridge_free() it */
unsigned int acpi_bridge_evaluate(void *handle, const char *path, unsigned int argc,
                                  void *argv, void **result);
void acpi_bridge_free(void *buffer);
unsigned int acpi_bridge_get_handle(void *parent, const char *path, void **handle);
/* *name is ACPICA-allocated: acpi_bridge_free() it */
unsigned int acpi_bridge_get_name(void *handle, unsigned int type, char **name);
unsigned int acpi_bridge_get_next_object(unsigned int type, void *parent, void *child, void **next);
const char *acpi_bridge_format_exception(unsigned int status);
void *acpi_bridge_find_pci(unsigned int bus, unsigned int devfn, acpi_bridge_cfg8 cfg8);

#endif
