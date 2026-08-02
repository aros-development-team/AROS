/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Reading the flattened device tree the firmware handed over.

    On a device-tree machine this is the only description of the
    hardware there is, so drivers discover their registers, interrupts
    and clocks through here.

    The blob is read in place; nothing is allocated and nothing is
    modified, so it is safe to call from a driver's init before any
    memory system is available.
*/

#ifndef FDT_H
#define FDT_H

#include <exec/types.h>

/* An offset into the blob's structure block, or -1 for "not found" */
typedef LONG fdt_node_t;

#define FDT_NONE ((fdt_node_t)-1)

/*
 * Hand the library the blob to work on. Returns FALSE if it is not a
 * device tree. Drivers normally pass what KrnGetBootInfo() reports for
 * KRN_FlattenedDeviceTree.
 */
BOOL FDT_Open(const void *dtb);

/* Is a blob currently open? */
BOOL FDT_IsOpen(void);

/* The root node */
fdt_node_t FDT_Root(void);

/*
 * Walk the tree. FDT_NextNode() returns every node in depth-first
 * order, so a plain loop from FDT_Root() visits the whole tree.
 */
fdt_node_t FDT_NextNode(fdt_node_t node);

/* The node's name, without the unit address ("serial", not
   "serial@20300000"). NULL if the node does not exist. */
CONST_STRPTR FDT_NodeName(fdt_node_t node);

/*
 * Does this node's "compatible" list contain the given string? The
 * property is a list, and a node usually names both its specific part
 * and the generic one it behaves like, so matching the generic name
 * covers a whole family of parts.
 */
BOOL FDT_IsCompatible(fdt_node_t node, CONST_STRPTR compat);

/* The first node compatible with the given string, starting the search
   after `from` (pass FDT_NONE to start at the root) */
fdt_node_t FDT_FindCompatible(fdt_node_t from, CONST_STRPTR compat);

/* Raw property access. Returns NULL if absent; *len gets its size in
   bytes when len is non-NULL. */
CONST_APTR FDT_GetProp(fdt_node_t node, CONST_STRPTR name, ULONG *len);

/* A property holding a single cell, or `def` if absent */
ULONG FDT_GetPropU32(fdt_node_t node, CONST_STRPTR name, ULONG def);

/* A string property, or NULL */
CONST_STRPTR FDT_GetPropStr(fdt_node_t node, CONST_STRPTR name);

/* Is the property present at all (for the valueless flags such as
   "interrupt-controller" or "dma-coherent")? */
BOOL FDT_HasProp(fdt_node_t node, CONST_STRPTR name);

/*
 * One entry of a "reg" property, decoded with the cell counts the
 * parent bus declares. `index` selects the entry; when the node has a
 * "reg-names" property, FDT_GetRegNamed() picks the entry by name
 * instead, which is how a PCIe controller's "dbi" window is told apart
 * from its "config" window.
 *
 * Returns FALSE when there is no such entry.
 */
BOOL FDT_GetReg(fdt_node_t node, ULONG index, UQUAD *addr, UQUAD *size);
BOOL FDT_GetRegNamed(fdt_node_t node, CONST_STRPTR name, UQUAD *addr,
                     UQUAD *size);

/* #address-cells / #size-cells that apply to this node's own reg -
   i.e. the values its parent declares */
ULONG FDT_ParentAddressCells(fdt_node_t node);
ULONG FDT_ParentSizeCells(fdt_node_t node);

/* Read `count` cells starting at `cells` as one number */
UQUAD FDT_ReadCells(const ULONG *cells, ULONG count);

#endif /* FDT_H */
