#include <aros/macros.h>
#include "devicetree.h"
#include "boot.h"
#include <stdlib.h>
#include <string.h>

#define D(x)

struct dt_entry *root = NULL;
uint32_t *data;
char *strings;

struct dt_entry * dt_build_node(struct dt_entry *parent)
{
    struct dt_entry *e = malloc(sizeof(struct dt_entry));

    if (e != NULL)
    {
        e->dte_parent = parent;
        e->dte_children = NULL;
        e->dte_properties = NULL;
        e->dte_next = NULL;
        e->dte_name = (char *)data;
        data += (strlen((char *)data) + 4) / 4;

        D(kprintf("[BOOT] new node %s\n", e->dte_name));

        while(1)
        {
            switch (*data++)
            {
                case FDT_BEGIN_NODE:
                {
                    struct dt_entry *e1 = dt_build_node(e);
                    e1->dte_next = e->dte_children;
                    e->dte_children = e1;
                    break;
                }

                case FDT_PROP:
                {
                    struct dt_prop *p = malloc(sizeof (struct dt_prop));
                    p->dtp_next = e->dte_properties;
                    e->dte_properties = p;
                    p->dtp_length = *data++;
                    p->dtp_name = &strings[*data++];
                    if (p->dtp_length)
                        p->dtp_value = data;
                    else
                        p->dtp_value = NULL;
                    data += (p->dtp_length + 3)/4;
                    D(kprintf("[BOOT] prop %s with length %d\n", p->dtp_name, p->dtp_length));
                    break;
                }

                case FDT_NOP:
                    break;

                case FDT_END_NODE:
                    return e;
            }
        }
    }
    return e;
}

static struct fdt_header *hdr;

long dt_total_size()
{
    if (hdr != NULL)
        return AROS_BE2LONG(hdr->totalsize);
    else
        return 0;
}

struct dt_entry * dt_parse(void *dt)
{
    uint32_t token = 0;

    hdr = dt;

    D(kprintf("[BOOT] Checking device tree at %p\n", hdr));
    D(kprintf("[BOOT] magic=%08x\n", hdr->magic));

    if (hdr->magic == AROS_LONG2BE(FDT_MAGIC))
    {
        D(kprintf("[BOOT] size=%d\n", hdr->totalsize));
        D(kprintf("[BOOT] off_dt_struct=%d\n", hdr->off_dt_struct));
        D(kprintf("[BOOT] off_dt_strings=%d\n", hdr->off_dt_strings));
        D(kprintf("[BOOT] off_mem_rsvmap=%d\n", hdr->off_mem_rsvmap));

        strings = dt + hdr->off_dt_strings;
        data = dt + hdr->off_dt_struct;

        if (hdr->off_mem_rsvmap)
        {
            struct fdt_reserve_entry *rsrvd = dt + hdr->off_mem_rsvmap;

            while (rsrvd->address != 0 || rsrvd->size != 0) {
                D(kprintf("[BOOT]   reserved: %08x-%08x\n", (uint32_t)rsrvd->address, (uint32_t)(rsrvd->address + rsrvd->size - 1)));
                rsrvd++;
            }
        }

        do
        {
            token = *data++;

            switch (token)
            {
                case FDT_BEGIN_NODE:
                    root = dt_build_node(NULL);
                    break;
                case FDT_PROP:
                {
                    kprintf("[BOOT] Property outside root node?");
                    break;
                }
            }
        } while (token != FDT_END);
    }
    else
    {
        hdr = NULL;
    }

    return root;
}

static struct dt_entry * dt_find_by_phandle(uint32_t phandle, struct dt_entry *root)
{
    struct dt_prop *p = dt_find_property(root, "phandle");

    if (p && *((uint32_t *)p->dtp_value) == phandle)
        return root;
    else {
        for (struct dt_entry *c = root->dte_children; c; c = c->dte_next) {
            struct dt_entry *found = dt_find_by_phandle(phandle, c);
            if (found)
                return found;
        }
    }
    return NULL;
}

struct dt_entry * dt_find_node_by_phandle(uint32_t phandle)
{
    return dt_find_by_phandle(phandle, root);
}

char ptrbuf[64];

struct dt_entry * dt_find_node(char *key)
{
    int i;
    struct dt_entry *node, *ret = NULL;

    if (*key == '/')
    {
        ret = root;

        while(*key)
        {
            key++;
            for (i=0; i < 63; i++)
            {
                if (*key == '/' || *key == 0)
                    break;
                ptrbuf[i] = *key;
                key++;
            }

            ptrbuf[i] = 0;

            for (node = ret->dte_children; node; node = node->dte_next)
            {
                if (!strcmp(node->dte_name, ptrbuf))
                {
                    ret = node;
                    break;
                }
            }
        }
    }

    return ret;
}

struct dt_prop *dt_find_property(void *key, char *propname)
{
    struct dt_entry *node = (struct dt_entry *)key;
    struct dt_prop *p, *prop = NULL;

    if (node)
    {
        for (p = node->dte_properties; p; p=p->dtp_next)
        {
            if (!strcmp(p->dtp_name, propname))
            {
                prop = p;
                break;
            }
        }
    }
    return prop;
}

char fill[] = "                         ";

void dt_dump_node(struct dt_entry *n, int level)
{
    kprintf("[BOOT] %s%s\n", &fill[25-2*level], n->dte_name);
    for (struct dt_prop *p = n->dte_properties; p; p = p->dtp_next)
    {
        kprintf("[BOOT] %s  %s=", &fill[25-2*level], p->dtp_name);
        for (int i=0; i < p->dtp_length; i++) {
            char *pchar = p->dtp_value + i;
            if (*pchar >= ' ' && *pchar <= 'z')
                kprintf("%c", *pchar);
            else
                kprintf(".");
        }

        if (p->dtp_length) {
            kprintf(" (");
            int max = 16;
            if (max > p->dtp_length)
                max = p->dtp_length;

            for (int i=0; i < p->dtp_length; i++) {
                char *pchar = p->dtp_value + i;
                kprintf("%02x", *pchar);
            }
            kprintf(")");
        }
        kprintf("\n");
    }
    for (struct dt_entry *c = n->dte_children; c; c = c->dte_next)
    {
        dt_dump_node(c, level+1);
    }
}

void dt_dump_tree()
{
    kprintf("[BOOT] Device Tree dump:\n");

    dt_dump_node(root, 0);

}
