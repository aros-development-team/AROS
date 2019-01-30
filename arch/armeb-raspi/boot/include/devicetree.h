#ifndef _DEVICETREE_H
#define _DEVICETREE_H

#include <aros/macros.h>
#include <stdint.h>

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

struct dt_prop {
    struct dt_prop *    dtp_next;
    char *              dtp_name;
    uint32_t            dtp_length;
    void *              dtp_value;
};

struct dt_entry {
    struct dt_entry *   dte_next;
    struct dt_entry *   dte_parent;
    char *              dte_name;
    struct dt_entry *   dte_children;
    struct dt_prop *    dte_properties;
};

#define FDT_END         0x00000009
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004

#define FDT_MAGIC       0xd00dfeed

void dt_dump_tree();
struct dt_entry *dt_parse(void *ptr);
struct dt_entry *dt_find_node_by_phandle(uint32_t phandle);
struct dt_entry *dt_find_node(char *key);
struct dt_prop *dt_find_property(void *key, char *propname);

#endif /* _DEVICETREE_H */
