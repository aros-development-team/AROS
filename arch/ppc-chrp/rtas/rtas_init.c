/*
    Copyright © 2009-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1

//#include <asm/mpc5200b.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <aros/libcall.h>
#include <proto/openfirmware.h>
#include <proto/kernel.h>
#include <proto/exec.h>
#include <proto/rtas.h>

#include "rtas_private.h"

static int rtas_init(struct RTASBase *RTASBase)
{
	int retval = TRUE;
	void *OpenFirmwareBase;
	void *key, *prop;

	D(bug("[RTAS] RTAS Init.\n"));

	OpenFirmwareBase = RTASBase->OpenFirmwareBase = OpenResource("openfirmware.resource");
	RTASBase->KernelBase = OpenResource("kernel.resource");

	/* Get some addresses from OF tree */
	if (OpenFirmwareBase)
	{
		key = OF_OpenKey("/rtas");
		if (key)
		{
			prop = OF_FindProperty(key, "load_base");
			if (prop)
			{
				void **reg = OF_GetPropValue(prop);
				D(bug("[RTAS] RTAS loaded at %08x\n", *reg));
				RTASBase->rtas_base = *reg;
			}

			prop = OF_FindProperty(key, "entry");
			if (prop)
			{
				void **reg = OF_GetPropValue(prop);
				D(bug("[RTAS] Entry at %08x\n", *reg));
				RTASBase->rtas_entry = *reg;
			}
		}
	}
	else
		retval = FALSE;


	return retval;
}

extern void return_entry();
extern void return_2();

AROS_LH5(int32_t, RTASCall,
		AROS_LHA(const char *,	name,	A0),
		AROS_LHA(uint32_t,		nargs,	D0),
		AROS_LHA(uint32_t,		nrets,	D1),
		AROS_LHA(uint32_t *,	output, A1),
		AROS_LHA(va_list,		args,	A2),
		struct RTASBase *, RTASBase, 1, Rtas)
{
	AROS_LIBFUNC_INIT

	void *OpenFirmwareBase = RTASBase->OpenFirmwareBase;
	void *KernelBase = RTASBase->KernelBase;

	int32_t token = 0;
	int32_t retval = -1;

	D(bug("[RTAS] Calling method '%s'\n", name));

	void *key = OF_OpenKey("/rtas");
	if (key)
	{
		void *prop = OF_FindProperty(key, name);

		if (prop)
		{
			token = *(int32_t *)OF_GetPropValue(prop);
		}
	}

	if (token)
	{
		D(bug("[RTAS] Method '%s', token %d\n", name, token));
		int i;

		RTASBase->rtas_args.token = token;
		RTASBase->rtas_args.nargs = nargs;
		RTASBase->rtas_args.nret = nrets;
		RTASBase->rtas_args.rets = KrnVirtualToPhysical(&RTASBase->rtas_args.args[nargs]);

		for (i=0; i < nargs; i++)
		{
			RTASBase->rtas_args.args[i] = va_arg(args, uint32_t);
		}

		Disable();

		{
			register intptr_t args asm ("r4") = KrnVirtualToPhysical(&RTASBase->rtas_args);
			register intptr_t base asm ("r5") = RTASBase->rtas_base;
			register intptr_t entry asm ("r6") = RTASBase->rtas_entry;

			asm volatile("li %%r3,%0; sc"::"i"(9 /*SC_RTAS*/),"r"(args),"r"(base),"r"(entry):"memory","r3");
		}

		if (nrets > 1 && output)
		{
			for (i=0; i < nrets-1; i++)
				output[i] = RTASBase->rtas_args.rets[i+1];
		}

		retval = nrets > 0 ? RTASBase->rtas_args.rets[0] : 0;

		D(bug("[RTAS] The call returned %d\n", retval));

		Enable();
	}
	else
		retval = -1;

	return retval;

	AROS_LIBFUNC_EXIT
}

ADD2INITLIB(rtas_init, 0)
