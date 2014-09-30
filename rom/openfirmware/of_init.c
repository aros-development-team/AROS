/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

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
    		Key++;
    		for (i=0; i < 63; i++)
    		{
    			if (*Key == '/' || *Key == 0)
    				break;
    			ptrbuf[i] = *Key;
    			Key++;
    		}

    		ptrbuf[i] = 0;

    		D(bug("[OF] looking for child '%s'\n", ptrbuf));

    		ForeachNode(&root->on_children, node)
    		{
    			if (!strcmp(node->on_name, ptrbuf))
    			{
    				root = node;
    				break;
    			}
    		}
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
    	if (!strcmp(p->op_name, name))
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
