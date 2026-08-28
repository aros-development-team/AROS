/*
    Copyright (C) 2008-2014, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0

#include <inttypes.h>
#include <aros/kernel.h>
#include <aros/symbolsets.h>
#include <aros/debug.h>
#include <exec/nodes.h>
#include <exec/lists.h>
#include <utility/tagitem.h>
#include <proto/kernel.h>
#include <proto/utility.h>

#include "of_intern.h"
#include LC_LIBDEFS_FILE

int my_strcmp(const char *s1, const char *s2)
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

AROS_LH1(void *, OF_OpenKey,
         AROS_LHA(char *, Key, A0),
         struct OpenFirmwareBase *, OpenFirmwareBase, 1, Openfirmware)
{
    AROS_LIBFUNC_INIT

    char ptrbuf[64];
    int i;
    of_node_t *node, *root = NULL;

    D(bug("[OF] OpenKey('%s')\n", Key));

    if (*Key != '/')
    {
        D(bug("[OF] Key must have absolute path\n"));
    }
    else
    {
        root = LIBBASE->of_Root;

        while(*Key)
        {
                of_node_t *found = NULL;

                Key++;
                for (i=0; i < 63; i++)
                {
                        if (*Key == '/' || *Key == 0)
                                break;
                        ptrbuf[i] = *Key;
                        Key++;
                }

                /* A trailing slash, or "/" alone, names the node itself. */
                if (i == 0)
                        break;

                ptrbuf[i] = 0;

                D(bug("[OF] looking for child '%s'\n", ptrbuf));

                ForeachNode(&root->on_children, node)
                {
                        if (!my_strcmp(node->on_name, ptrbuf))
                        {
                                found = node;
                                break;
                        }
                }

                /*
                 * A path component that is not there means the caller asked
                 * for a node this machine does not have. Say so, rather than
                 * handing back the deepest ancestor that did match - which
                 * reads as success and leaves the caller inspecting the
                 * wrong node. dt_find_node() in the kernel answers the same
                 * way.
                 */
                if (!found)
                        return NULL;

                root = found;
        }
    }

    return root;

    AROS_LIBFUNC_EXIT
}

AROS_LH1I(void, OF_CloseKey,
         AROS_LHA(void *, Key, A0),
         struct OpenFirmwareBase *, OpenFirmwareBase, 2, Openfirmware)
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

AROS_LH2I(void *, OF_GetChild,
         AROS_LHA(void *, Key, A0),
         AROS_LHA(void *, Prev, A1),
         struct OpenFirmwareBase *, OpenFirmwareBase, 3, Openfirmware)
{
    AROS_LIBFUNC_INIT

    of_node_t *node = (of_node_t *)Key;
    of_node_t *last = (of_node_t *)Prev;

    if (last)
        return GetSucc(last);
    else
        return GetHead(&node->on_children);

    AROS_LIBFUNC_EXIT
}

AROS_LH2I(void *, OF_FindProperty,
         AROS_LHA(void *, Key, A0),
         AROS_LHA(char *, name, A1),
         struct OpenFirmwareBase *, OpenFirmwareBase, 4, Openfirmware)
{
    AROS_LIBFUNC_INIT

    of_node_t *node = (of_node_t *)Key;
    of_property_t *p, *prop = NULL;

    ForeachNode(&node->on_properties, p)
    {
        if (!my_strcmp(p->op_name, name))
        {
                prop = p;
                        break;
                }
    }

        return prop;

    AROS_LIBFUNC_EXIT
}

AROS_LH2I(void *, OF_GetProperty,
         AROS_LHA(void *, Key, A0),
         AROS_LHA(void *, Prev, A1),
         struct OpenFirmwareBase *, OpenFirmwareBase, 5, Openfirmware)
{
    AROS_LIBFUNC_INIT

    of_node_t *node = (of_node_t *)Key;
    of_property_t *last = (of_property_t *)Prev;

    if (last)
        return GetSucc(last);
    else
        return GetHead(&node->on_properties);

    AROS_LIBFUNC_EXIT
}

AROS_LH1I(uint32_t, OF_GetPropLen,
         AROS_LHA(void *, Key, A0),
         struct OpenFirmwareBase *, OpenFirmwareBase, 6, Openfirmware)
{
    AROS_LIBFUNC_INIT

    of_property_t *prop = (of_property_t *)Key;

    if (prop)
        return prop->op_length;
    else
        return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1I(char *, OF_GetPropName,
         AROS_LHA(void *, Key, A0),
         struct OpenFirmwareBase *, OpenFirmwareBase, 8, Openfirmware)
{
    AROS_LIBFUNC_INIT

    of_property_t *prop = (of_property_t *)Key;

    if (prop)
        return prop->op_name;
    else
        return "(null)";

    AROS_LIBFUNC_EXIT
}

AROS_LH1I(void *, OF_GetPropValue,
         AROS_LHA(void *, Key, A0),
         struct OpenFirmwareBase *, OpenFirmwareBase, 7, Openfirmware)
{
    AROS_LIBFUNC_INIT

    of_property_t *prop = (of_property_t *)Key;

    if (prop)
        return prop->op_value;
    else
        return NULL;

    AROS_LIBFUNC_EXIT
}


/* Does this node's "compatible" list - a run of NUL terminated strings -
   name the given binding? */
static BOOL of_is_compatible(of_node_t *node, const char *compatible)
{
    of_property_t *p;
    const char *v;
    uint32_t len, i;

    ForeachNode(&node->on_properties, p)
    {
        if (my_strcmp(p->op_name, "compatible"))
            continue;

        v = (const char *)p->op_value;
        len = p->op_length;
        if (!v)
            return FALSE;

        for (i = 0; i < len; i++)
        {
            if (!my_strcmp(&v[i], compatible))
                return TRUE;
            while (i < len && v[i] != '\0')
                i++;
        }
        return FALSE;
    }
    return FALSE;
}

static of_node_t *of_search_compatible(of_node_t *node, const char *compatible)
{
    of_node_t *child, *found;

    if (of_is_compatible(node, compatible))
        return node;

    ForeachNode(&node->on_children, child)
    {
        if ((found = of_search_compatible(child, compatible)) != NULL)
            return found;
    }
    return NULL;
}

/*
 * Find a node by what it is rather than where it sits. Unit addresses in
 * node names are bus addresses and move between SoC generations - the
 * Broadcom RNG is rng@7e104000 on a Pi 3 and rng@7d208000 on a Pi 5 - so a
 * driver that hardcodes a path silently stops matching, or worse, matches
 * nothing and carries on. The binding string is the stable identifier.
 */
AROS_LH2(void *, OF_FindNodeByCompatible,
         AROS_LHA(void *, Start, A0),
         AROS_LHA(char *, Compatible, A1),
         struct OpenFirmwareBase *, OpenFirmwareBase, 9, Openfirmware)
{
    AROS_LIBFUNC_INIT

    of_node_t *from = Start ? (of_node_t *)Start : LIBBASE->of_Root;

    if (!from || !Compatible)
        return NULL;

    return of_search_compatible(from, Compatible);

    AROS_LIBFUNC_EXIT
}

static int OF_Init(LIBBASETYPEPTR LIBBASE)
{
        void *KernelBase = OpenResource("kernel.resource");

        D(bug("[OF] OpenFirmware_Init\n"));

        if (KernelBase)
        {
                struct TagItem *tags = KrnGetBootInfo();

                if (tags)
                {
                        intptr_t oftree;
                        D(bug("[OF] BootInto @ %08x\n", tags));

                        oftree = GetTagData(KRN_OpenFirmwareTree, 0, tags);

                        if (oftree)
                        {
                                D(bug("[OF] OpenFirmware root at %08x\n", oftree));
                                LIBBASE->of_Root = (of_node_t *)oftree;

                                return TRUE;
                        }
                        D(else bug("[OF] No OpenFirmware tree passed from bootloader\n"));
                }
                D(else bug("[OF] No BootInfo found\n"));
        }

        return FALSE;
}

ADD2INITLIB(OF_Init, 0)
