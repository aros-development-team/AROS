/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/


#include <stdio.h>
#include <stdlib.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <exec/memory.h>
#include <exec/nodes.h>

#include "devices.h"
#include "error.h"
#include "gui.h"
#include "hdtoolbox_support.h"
#include "locale.h"
#include "partitions.h"
#include "partitiontables.h"
#include "platform.h"
#include "ptclass.h"
#include "prefs.h"

#define DEBUG 1
#include "debug.h"

struct IntuitionBase *IntuitionBase=NULL;
struct GfxBase *GfxBase=NULL;
struct PartitionBase *PartitionBase=NULL;

extern struct ListNode root;

LONG initEnv(char *device) {
LONG retval;

	if (!InitListNode(&root, NULL))
		return ERR_MEMORY;
	IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 37);
	if (!IntuitionBase)
		return ERR_INTUI;
	GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 37);
	if (!GfxBase)
		return ERR_GFX;
	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
	if (!PartitionBase)
		return ERR_PARTITION;

	retval = initGUI();
	if (retval != ERR_NONE)
		return retval;
	LoadPrefs("ENV:hdtoolbox.prefs");
	return ERR_NONE;
}

void uninitEnv() {
	deinitGUI();
	if (PartitionBase)
		CloseLibrary((struct Library *)PartitionBase);
	if (GfxBase)
		CloseLibrary((struct Library *)GfxBase);
	if (IntuitionBase)
		CloseLibrary((struct Library *)IntuitionBase);
	freeDeviceList();
}

void waitMessage() {
ULONG sigs = 0;

	while (!QuitGUI(&sigs))
	{
		if (sigs)
		{
			sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
			if (sigs & SIGBREAKF_CTRL_C)
				break;
			if (sigs & SIGBREAKF_CTRL_D)
				break;
		}
	}
}

int main(int argc, char **argv) {
ULONG error;
char *device;

	InitLocale("System/Tools/HDToolBox.catalog", 1);
	device = argc > 1 ? argv[1] : NULL;
	if ((error=initEnv(device))==ERR_NONE)
	{
		waitMessage();
	}
	else
		printf("Error %ld\n", error);
	uninitEnv();
	CleanupLocale();
	return 0;
}

