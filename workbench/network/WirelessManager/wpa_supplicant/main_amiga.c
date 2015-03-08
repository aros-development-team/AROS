/*
 * WPA Supplicant / main() function for Amiga-like OSes
 * Copyright (c) 2003-2007, Jouni Malinen <j@w1.fi>
 * Copyright (c) 2010-2015, Neil Cafferkey
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#include "includes.h"

#include "common.h"
#include "wpa_supplicant_i.h"

#include <exec/types.h>
#include <dos/dos.h>

#include <proto/dos.h>

#ifdef MUI_GUI
#include <proto/exec.h>
int start_gui(struct wpa_global *global);
void stop_gui(void);
VOID MUIGUI(VOID);

struct Process *gui_proc;
extern struct wpa_global *gui_global;
extern struct Task *main_task;
#endif

#ifndef UPINT
#ifdef __AROS__
typedef IPTR UPINT;
typedef SIPTR PINT;
#else
typedef ULONG UPINT;
typedef LONG PINT;
#endif
#endif

static const TEXT template[] = "DEVICE/A,UNIT/K/N,CONFIG/K,VERBOSE/S,NOGUI/S";
const TEXT version_string[] = "$VER: WirelessManager 1.6 (8.3.2015)";
static const TEXT config_file_name[] = "ENV:Sys/Wireless.prefs";


int main(int argc, char *argv[])
{
	struct RDArgs *read_args;
	LONG error = 0, unit_no = 0;
	struct
	{
		const TEXT *device;
		LONG *unit;
		const TEXT *config;
		PINT verbose;
		PINT no_gui;
	}
	args = {NULL, &unit_no, config_file_name, FALSE, FALSE};
	int i;
	struct wpa_interface *ifaces, *iface;
	int iface_count, exitcode = RETURN_FAIL;
	struct wpa_params params;
	struct wpa_global *global;
	char *ifname;

	if (os_program_init())
		return -1;

	os_memset(&params, 0, sizeof(params));
	params.wpa_debug_level = MSG_INFO;

	iface = ifaces = os_zalloc(sizeof(struct wpa_interface));
	if (ifaces == NULL)
		return RETURN_FAIL;
	iface_count = 1;

	/* Parse arguments */
	read_args = ReadArgs(template, (PINT *)&args, NULL);
	if (read_args == NULL)
	{
		error = IoErr();
		goto out;
	}

	iface->ifname = ifname = os_malloc(strlen((const char *)args.device)
		+ 1 + 10 + 1);
	if (ifname == NULL)
	{
		error = ERROR_NO_FREE_STORE;
		goto out;
	}

	sprintf(ifname, "%s:%ld", args.device, (long int)*args.unit);

	iface->confname = (char *) args.config;

	if (args.verbose)
		params.wpa_debug_level -= 2;

	exitcode = 0;
	global = wpa_supplicant_init(&params);
	if (global == NULL) {
		wpa_printf(MSG_ERROR, "Failed to initialize WirelessManager");
		exitcode = RETURN_FAIL;
		goto out;
	}

	for (i = 0; exitcode == 0 && i < iface_count; i++) {
		if ((ifaces[i].confname == NULL &&
		     ifaces[i].ctrl_interface == NULL) ||
		    ifaces[i].ifname == NULL) {
			if (iface_count == 1 && (params.ctrl_interface ||
						 params.dbus_ctrl_interface))
				break;
			error = ERROR_REQUIRED_ARG_MISSING;
			exitcode = RETURN_FAIL;
			break;
		}
		if (wpa_supplicant_add_iface(global, &ifaces[i]) == NULL)
			exitcode = RETURN_FAIL;
	}

#ifdef MUI_GUI
	if (exitcode == 0 && !args.no_gui)
		exitcode = start_gui(global);
#endif

	if (exitcode == 0)
		exitcode = wpa_supplicant_run(global);

	wpa_supplicant_deinit(global);

out:
	os_free(ifaces);

//	os_program_deinit();
#ifdef MUI_GUI
	stop_gui();
#endif

	FreeArgs(read_args);

	/* Print error message */
	SetIoErr(error);
	PrintFault(error, NULL);

	if (error != 0)
		exitcode = RETURN_FAIL;

	return exitcode;
}

#ifdef MUI_GUI
int start_gui(struct wpa_global *global)
{
	gui_global = global;
	main_task = FindTask(NULL);
	gui_proc = CreateNewProcTags(NP_Entry, MUIGUI,
		NP_Name, "WirelessManager GUI", TAG_END);

	return 0;
}

void stop_gui(void)
{
	if (gui_proc != NULL)
	{
		Signal((struct Task *) gui_proc, SIGBREAKF_CTRL_C);
		Wait(SIGF_SINGLE);
	}
}
#endif
