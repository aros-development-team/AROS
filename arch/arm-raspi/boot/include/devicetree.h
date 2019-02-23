#ifndef _DEVICETREE_H
#define _DEVICETREE_H

#include <aros/macros.h>
#include <exec/lists.h>
#include <stdint.h>
#include "of_intern.h"

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

struct fdt_reserve_entry {
    uint64_t address;
    uint64_t size;
};

struct fdt_prop_entry {
    uint32_t len;
    uint32_t nameoffset;
};

#define FDT_END         0x00000009
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004

#define FDT_MAGIC       0xd00dfeed

void dt_dump_tree();
of_node_t *dt_parse(void *ptr);
long dt_total_size();
of_node_t *dt_find_node_by_phandle(uint32_t phandle);
of_node_t *dt_find_node(char *key);
of_property_t *dt_find_property(void *key, char *propname);

#endif /* _DEVICETREE_H */
