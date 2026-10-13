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

/*****************************************************************************

    NAME */
        AROS_LH1(void *, OF_OpenKey,

/*  SYNOPSIS */
                 AROS_LHA(char *, Key, A0),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 1, Openfirmware)
/*  FUNCTION
        Find a node by an absolute path in the boot-supplied device tree.
        Each component is matched exactly and case-sensitively against a
        child node name. Unit-address suffixes are part of the name.
        The path "/" returns the root; a trailing slash names the same node.

    INPUTS
        Key - Non-NULL NUL-terminated absolute path. Use components of
              at most 63 bytes and a single slash between components.

    RESULT
        A borrowed node handle, or NULL for a relative path or a component
        that cannot be found.

    NOTES
        Returned pointers refer to the boot-supplied tree; no copy is made
        and ownership is not transferred. Do not modify or free this data.
        The tree must remain valid while its handles or values are used.
        This function performs no allocation or wait.

        The string is not modified. No alias expansion, wildcard matching,
        unit-address abbreviation or dot/dot-dot resolution is performed.
        OF_CloseKey() currently has no effect on the returned handle.

    BUGS
        An empty component terminates traversal, so repeated slashes can
        return an ancestor without examining the rest of the path. Components
        longer than 63 bytes are not handled as complete names and are not
        reliably rejected. Use the input form described above.

    SEE ALSO
        OF_CloseKey(), OF_GetChild(), OF_FindProperty(), OF_FindNodeByCompatible()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH1I(void, OF_CloseKey,

/*  SYNOPSIS */
                 AROS_LHA(void *, Key, A0),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 2, Openfirmware)
/*  FUNCTION
        Release a node handle obtained from openfirmware.resource.

    INPUTS
        Key - Node handle to close. The current implementation ignores it.

    RESULT
        None.

    NOTES
        This is currently a no-op: no memory is freed and no reference count
        is changed. The tree remains owned by the boot environment. Closing
        a handle does not invalidate borrowed child or property pointers.
        This function performs no allocation or wait.

    SEE ALSO
        OF_OpenKey(), OF_GetChild()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH2I(void *, OF_GetChild,

/*  SYNOPSIS */
                 AROS_LHA(void *, Key, A0),
                 AROS_LHA(void *, Prev, A1),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 3, Openfirmware)
/*  FUNCTION
        Enumerate the immediate children of a node in list order.
        Pass NULL as Prev to start, then pass the previous result to advance.

    INPUTS
        Key - Non-NULL node handle.
        Prev - NULL, or the last child node handle returned for this same Key.
               It must still belong to that node's list.

    RESULT
        A borrowed child node handle, or NULL for an empty list or after
        its last element.

    NOTES
        Returned pointers refer to the boot-supplied tree; no copy is made
        and ownership is not transferred. Do not modify or free this data.
        The tree must remain valid while its handles or values are used.
        This function performs no allocation or wait.

        There is no handle-type or parent-membership validation. A non-NULL
        Prev is followed directly; Key is not used to check its parent.

    SEE ALSO
        OF_OpenKey(), OF_CloseKey()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH2I(void *, OF_FindProperty,

/*  SYNOPSIS */
                 AROS_LHA(void *, Key, A0),
                 AROS_LHA(char *, name, A1),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 4, Openfirmware)
/*  FUNCTION
        Find a named property among the immediate properties of a node.

    INPUTS
        Key - Non-NULL node handle.
        name - NUL-terminated property name, matched exactly and
               case-sensitively. NULL matches no property.

    RESULT
        A borrowed property handle, or NULL if no name matches.

    NOTES
        Returned pointers refer to the boot-supplied tree; no copy is made
        and ownership is not transferred. Do not modify or free this data.
        The tree must remain valid while its handles or values are used.
        This function performs no allocation or wait.

    SEE ALSO
        OF_GetProperty(), OF_GetPropName(), OF_GetPropLen(), OF_GetPropValue()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH2I(void *, OF_GetProperty,

/*  SYNOPSIS */
                 AROS_LHA(void *, Key, A0),
                 AROS_LHA(void *, Prev, A1),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 5, Openfirmware)
/*  FUNCTION
        Enumerate the immediate properties of a node in list order.
        Pass NULL as Prev to start, then pass the previous result to advance.

    INPUTS
        Key - Non-NULL node handle.
        Prev - NULL, or the last property handle returned for this same Key.
               It must still belong to that node's list.

    RESULT
        A borrowed property handle, or NULL for an empty list or after
        its last element.

    NOTES
        Returned pointers refer to the boot-supplied tree; no copy is made
        and ownership is not transferred. Do not modify or free this data.
        The tree must remain valid while its handles or values are used.
        This function performs no allocation or wait.

        There is no handle-type or parent-membership validation. A non-NULL
        Prev is followed directly; Key is not used to check its parent.

    SEE ALSO
        OF_FindProperty(), OF_GetPropName(), OF_GetPropLen(), OF_GetPropValue()

*****************************************************************************/
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

/*****************************************************************************

    NAME */
        AROS_LH1I(uint32_t, OF_GetPropLen,

/*  SYNOPSIS */
                 AROS_LHA(void *, Key, A0),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 6, Openfirmware)
/*  FUNCTION
        Return the recorded byte length of a property value.

    INPUTS
        Key - Property handle, not a node handle; NULL is accepted.

    RESULT
        The property's stored byte length, or zero for NULL. Zero may also
        be a valid property length and does not by itself prove absence.

    NOTES
        No data is copied or validated. The length describes raw property
        bytes, not a count of integer cells or characters. The property
        handle must remain valid. This function performs no allocation or
        wait.

    SEE ALSO
        OF_FindProperty(), OF_GetProperty(), OF_GetPropValue()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    of_property_t *prop = (of_property_t *)Key;

    if (prop)
        return prop->op_length;
    else
        return 0;

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1I(char *, OF_GetPropName,

/*  SYNOPSIS */
                 AROS_LHA(void *, Key, A0),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 8, Openfirmware)
/*  FUNCTION
        Return the name attached to a property.

    INPUTS
        Key - Property handle, not a node handle; NULL is accepted.

    RESULT
        The borrowed name pointer for a property. For NULL, the literal
        string "(null)" is returned rather than a NULL pointer.

    NOTES
        Returned pointers refer to the boot-supplied tree; no copy is made
        and ownership is not transferred. Do not modify or free this data.
        The tree must remain valid while its handles or values are used.
        This function performs no allocation or wait.

    SEE ALSO
        OF_FindProperty(), OF_GetProperty()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    of_property_t *prop = (of_property_t *)Key;

    if (prop)
        return prop->op_name;
    else
        return "(null)";

    AROS_LIBFUNC_EXIT
}

/*****************************************************************************

    NAME */
        AROS_LH1I(void *, OF_GetPropValue,

/*  SYNOPSIS */
                 AROS_LHA(void *, Key, A0),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 7, Openfirmware)
/*  FUNCTION
        Return direct access to the bytes of a property value.

    INPUTS
        Key - Property handle, not a node handle; NULL is accepted.

    RESULT
        The stored value pointer, or NULL when Key is NULL. The pointer is
        not a copy and does not imply that any particular length or type
        is available.

    NOTES
        Returned pointers refer to the boot-supplied tree; no copy is made
        and ownership is not transferred. Do not modify or free this data.
        The tree must remain valid while its handles or values are used.
        This function performs no allocation or wait.

        Use OF_GetPropLen() before accessing bytes. Values may be binary,
        zero-length, strings or lists of strings; no NUL termination is
        added. Numeric byte order and cell layout are those of the supplied
        device tree: this function does not convert them to host types.

    SEE ALSO
        OF_GetPropLen(), OF_FindProperty(), OF_GetProperty()

*****************************************************************************/
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
/*****************************************************************************

    NAME */
        AROS_LH2(void *, OF_FindNodeByCompatible,

/*  SYNOPSIS */
                 AROS_LHA(void *, Start, A0),
                 AROS_LHA(char *, Compatible, A1),

/*  LOCATION */
                 struct OpenFirmwareBase *, OpenFirmwareBase, 9, Openfirmware)
/*  FUNCTION
        Find the first node whose "compatible" property contains an exact,
        case-sensitive match for the requested string. Search the start
        node first, then recursively search each child subtree in list
        order. The whole NUL-separated compatible list is examined.

    INPUTS
        Start - Node handle at which to start, or NULL for the tree root.
        Compatible - NUL-terminated binding string. NULL is accepted and
                     returns no match.

    RESULT
        The first matching borrowed node handle, or NULL for no match,
        a missing root or a NULL Compatible argument.

    NOTES
        Returned pointers refer to the boot-supplied tree; no copy is made
        and ownership is not transferred. Do not modify or free this data.
        The tree must remain valid while its handles or values are used.
        This function performs no allocation or wait.

        Start is included in the search; it is not an iteration cursor.
        There is no status-property filtering: a matching disabled node
        can be returned. Property strings must be well-formed and terminated
        within their stored values. The search uses recursive stack space
        proportional to tree depth.

    SEE ALSO
        OF_OpenKey(), OF_GetChild(), OF_FindProperty()

*****************************************************************************/
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
