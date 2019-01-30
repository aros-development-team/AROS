#include <aros/macros.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include "kernel_intern.h"
#include "kernel_debug.h"
#include "of_intern.h"

#undef KernelBase
#define KernelBase (NULL)

static of_node_t * root = NULL;

void dt_set_root(void * r)
{
    root = (of_node_t *)r;
}

static int my_strcmp(const char *s1, const char *s2)
{
    if (s1 && s2)
    {
        while (*s1 && *s2) {
            if (*s1 != *s2)
                break;
            s1++;
            s2++;
        }
        return *s1 - *s2;
    }
    else return -1;
}

static void * dt_find_by_phandle(uint32_t phandle, of_node_t *root)
{
    of_property_t *p = dt_find_property(root, "phandle");

    if (p && *((uint32_t *)p->op_value) == AROS_LONG2BE(phandle))
        return root;
    else {
        of_node_t *c;
        struct List *l = (struct List *)&root->on_children;
        ForeachNode(l, c)
        {
            of_node_t *found = dt_find_by_phandle(phandle, c);
            if (found)
                return found;
        }
    }
    return NULL;
}

void * dt_find_node_by_phandle(uint32_t phandle)
{
    return dt_find_by_phandle(phandle, root);
}

#define MAX_KEY_SIZE    64
char ptrbuf[64];

void * dt_find_node(char *key)
{
    int i;
    of_node_t *node, *ret = NULL;

    bug("[Kernel] dt_find_node('%s')\n", key);

    if (*key == '/')
    {
        ret = root;

        bug("[Kernel] root=%p\n", root);

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

            // If key is empty return current node.
            if (i == 0)
            {
                return ret;
            }

            ptrbuf[i] = 0;
            struct List *c = (struct List *)&ret->on_children;
            bug("[Kernel] ptrbuf='%s'\n", ptrbuf);
            ForeachNode(c, node)
            {
                bug("[Kernel] node=%p\n", node);
                if (!my_strcmp(node->on_name, ptrbuf))
                {
                    ret = node;
                    break;
                }
            }
        }
    }

    return ret;
}

void *dt_find_property(void *key, char *propname)
{
    of_node_t *node = (of_node_t *)key;
    of_property_t *p, *prop = NULL;

    if (node)
    {
        struct List *l = (struct List *)&node->on_properties;
        ForeachNode(l, p)
        {
            if (!my_strcmp(p->op_name, propname))
            {
                prop = p;
                break;
            }
        }
    }
    return prop;
}

void *dt_get_prop_value(void *prop)
{
    of_property_t *p = (of_property_t *)prop;

    if (p)
    {
        return p->op_value;
    }
    else
        return NULL;
}

int dt_get_prop_len(void *prop)
{
    of_property_t *p = (of_property_t *)prop;

    if (p)
    {
        return p->op_length;
    }
    else
        return -1;
}
