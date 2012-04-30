/* Copyright 2007-2012 Fredrik Wikstrom. All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
*/

#include "diskimage_device.h"
#include "device_locale.h"
#include <dos/exall.h>

#ifndef EAD_IS_FILE
#define EAD_IS_FILE(ead) ((ead)->ed_Type <  0)
#endif

#ifndef FIB_IS_FILE
#define FIB_IS_FILE(fib) ((fib)->fib_DirEntryType <  0)
#endif

extern struct DiskImagePlugin generic_plugin;
extern struct DiskImagePlugin adf_plugin;
extern struct DiskImagePlugin d64_plugin;
extern struct DiskImagePlugin iso_plugin;

static struct DiskImagePlugin * const builtin_plugin_array[] = {
	&generic_plugin,
	&adf_plugin,
	&d64_plugin,
	&iso_plugin,
	NULL
};

static struct DiskImagePluginTable builtin_plugin_table = {
	PLUGIN_MAGIC,
	USED_PLUGIN_API_VERSION,
	-1UL,
	builtin_plugin_array
};

static ULONG InitPlugins (struct DiskImageBase *libBase, struct DiskImagePluginTable *plugin_table,
	BPTR seglist, const struct PluginData *plugin_data);

void LoadPlugins (struct DiskImageBase *libBase) {
	struct Library *DOSBase = libBase->DOSBase;
	struct PluginData plugin_data;

	IPluginIFace.Data.LibBase = &libBase->LibNode;
	plugin_data.SysBase = libBase->SysBase;
	plugin_data.DOSBase = libBase->DOSBase;
	plugin_data.UtilityBase = libBase->UtilityBase;
	plugin_data.IPlugin = &IPluginIFace;
#ifdef __AROS__
	plugin_data.aroscbase = libBase->aroscbase;
#endif

	libBase->HeaderTestSize = 0;
	libBase->FooterTestSize = 0;

	/* Built-in plugins */
	InitPlugins(libBase, &builtin_plugin_table, libBase->SegList, &plugin_data);

	/* Dynamically loaded plugins */
	{
#ifdef __AROS__
		#define EA_BUFFER_SIZE 4096
		BPTR dir, curr_dir;
		struct ExAllControl *eac;
		struct ExAllData *eabuffer;
		struct ExAllData *ead;
		BOOL more;
		BPTR seglist;
		struct DiskImagePluginTable *plugin_table;
		
		dir = Lock("DEVS:DiskImage", ACCESS_READ);
		eac = AllocDosObject(DOS_EXALLCONTROL, NULL);
		eabuffer = AllocVec(EA_BUFFER_SIZE, MEMF_ANY);
		
		if (dir && eac && eabuffer) {
			do {
				more = ExAll(dir, eabuffer, EA_BUFFER_SIZE, ED_TYPE, eac);
				if (eac->eac_Entries == 0) {
					continue;
				}
				ead = eabuffer;
				do {
					curr_dir = CurrentDir(dir);
					if (EAD_IS_FILE(ead) && (seglist = LoadSeg(ead->ed_Name))) {
						APTR (*entry)(void) = (APTR)((BPTR *)BADDR(seglist) + 1);
						plugin_table = (struct DiskImagePluginTable *)entry();
						if (InitPlugins(libBase, plugin_table, seglist, &plugin_data) == 0) {
							UnLoadSeg(seglist);
						}
					}
					CurrentDir(curr_dir);
				} while ((ead = ead->ed_Next));
			} while (more);
		}
		
		FreeDosObject(DOS_EXALLCONTROL, eac);
		FreeVec(eabuffer);
		UnLock(dir);
#else
		BPTR dir, curr_dir;
		struct FileInfoBlock *fib;
		BPTR seglist;
		struct DiskImagePluginTable *plugin_table;

		dir = Lock("DEVS:DiskImage", ACCESS_READ);
		fib = AllocDosObject(DOS_FIB, NULL);

		if (dir && fib && Examine(dir, fib)) {
			while (ExNext(dir, fib)) {
				curr_dir = CurrentDir(dir);
				if (FIB_IS_FILE(fib) && (seglist = LoadSeg(fib->fib_FileName))) {
					APTR (*entry)(void) = (APTR)((BPTR *)BADDR(seglist) + 1);
					plugin_table = (struct DiskImagePluginTable *)entry();
					if (InitPlugins(libBase, plugin_table, seglist, &plugin_data) == 0) {
						UnLoadSeg(seglist);
					}
				}
				CurrentDir(curr_dir);
			}
		}

		FreeDosObject(DOS_FIB, fib);
		UnLock(dir);
#endif
	}
}

static ULONG InitPlugins (struct DiskImageBase *libBase, struct DiskImagePluginTable *plugin_table,
	BPTR seglist, const struct PluginData *plugin_data)
{
	struct Library *SysBase = libBase->SysBase;

	if (plugin_table && TypeOfMem(plugin_table) &&
		plugin_table->Magic == PLUGIN_MAGIC &&
		plugin_table->API_Version >= MIN_PLUGIN_API_VERSION &&
		plugin_table->API_Version <= MAX_PLUGIN_API_VERSION)
	{
		struct DiskImagePlugin *plugin;
		ULONG i;

		for (i = 0; (plugin = plugin_table->Plugins[i]); i++) {
			if ((plugin->Flags & PLUGIN_UNUSED_FLAGS) ||
				!(plugin->Flags & PLUGIN_FLAG_M68K))
			{
				continue;
			}
			plugin->SegList = seglist;
			plugin->RefCount = &plugin_table->RefCount;
			if (!plugin->plugin_Init || Plugin_Init(plugin, plugin_data)) {
				if (plugin_table->RefCount == -1UL) {
					plugin->Flags |= PLUGIN_FLAG_BUILTIN;
				} else {
					plugin_table->RefCount++;
				}
				if (!(plugin->Flags & PLUGIN_FLAG_FOOTER)) {
					if (plugin->TestSize > libBase->HeaderTestSize) {
						libBase->HeaderTestSize = plugin->TestSize;
					}
				} else {
					if (plugin->TestSize > libBase->FooterTestSize) {
						libBase->FooterTestSize = plugin->TestSize;
					}
				}
				Enqueue(libBase->Plugins, &plugin->Node);
			}
		}
		return plugin_table->RefCount;
	}
	return 0;
}

void FreePlugins (struct DiskImageBase *libBase) {
	struct Library *SysBase = libBase->SysBase;
	struct Library *DOSBase = libBase->DOSBase;
	struct DiskImagePlugin *plugin;
	
	if (libBase->Plugins) {
		while ((plugin = (struct DiskImagePlugin *)RemHead(libBase->Plugins))) {
			if (plugin->plugin_Exit) Plugin_Exit(plugin);
			if (!(plugin->Flags & PLUGIN_FLAG_BUILTIN) && --plugin->RefCount[0] == 0) {
				UnLoadSeg(plugin->SegList);
			}
		}
	}
	
	libBase->HeaderTestSize = 0;
	libBase->FooterTestSize = 0;
}

struct DiskImagePlugin *FindPlugin (struct DiskImageUnit *unit, BPTR file, CONST_STRPTR name) {
	struct DiskImageBase *libBase = unit->LibBase;
	struct Library *DOSBase = libBase->DOSBase;
	struct List *list = libBase->Plugins;
	QUAD file_size;
	UBYTE *header_test = NULL;
	LONG header_test_size = libBase->HeaderTestSize;
	LONG header_test_len = 0;
	UBYTE *footer_test = NULL;
	LONG footer_test_size = libBase->FooterTestSize;
	LONG footer_test_len = 0;
	struct DiskImagePlugin *ptr;
	UBYTE *test;
	LONG test_len;

	file_size = GetFileSize(file);
	switch (file_size) {
		case 0:  SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		case -1: goto error;
	}
	
	if (header_test_size > 0 || footer_test_size > 0) {
		if (header_test_size > 0) {
			if ((QUAD)header_test_size > file_size) header_test_size = file_size;
			header_test = AllocVec(header_test_size, MEMF_ANY);
			if (!header_test) {
				SetIoErr(ERROR_NO_FREE_STORE);
				goto error;
			}
			header_test_len = Read(file, header_test, header_test_size);
			if (header_test_len == -1) {
				goto error;
			}
		}
		if (footer_test_size > 0) {
			if ((QUAD)footer_test_size > file_size) footer_test_size = file_size;
			if (!ChangeFilePosition(file, -footer_test_size, OFFSET_END)) {
				goto error;
			}
			footer_test = AllocVec(footer_test_size, MEMF_ANY);
			if (!footer_test) {
				SetIoErr(ERROR_NO_FREE_STORE);
				goto error;
			}
			footer_test_len = Read(file, footer_test, footer_test_size);
			if (footer_test_len == -1) {
				goto error;
			}
		}
		if (!ChangeFilePosition(file, 0, OFFSET_BEGINNING)) {
			goto error;
		}
	}

	ptr = (struct DiskImagePlugin *)list->lh_Head;
	while (ptr->Node.ln_Succ) {
		if (!(ptr->Flags & PLUGIN_FLAG_FOOTER)) {
			test = header_test;
			test_len = header_test_len;
		} else {
			test = footer_test;
			test_len = footer_test_len;
		}
		if (ptr->plugin_CheckImage && Plugin_CheckImage(ptr, file, name, file_size, test, test_len)) {
			FreeVec(header_test);
			FreeVec(footer_test);
			return ptr;
		}
		ptr = (struct DiskImagePlugin *)ptr->Node.ln_Succ;
	}

error:
	FreeVec(header_test);
	FreeVec(footer_test);
	SetDiskImageError(NULL, unit, IoErr(), 0);
	return NULL;
}

APTR OpenImage (APTR Self, struct DiskImageUnit *unit, BPTR file, CONST_STRPTR name) {
	struct DiskImagePlugin *ptr;

	ptr = FindPlugin(unit, file, name);
	if (ptr) {
		unit->Plugin = ptr;
		return Plugin_OpenImage(ptr, unit, file, name);
	}
	return NULL;
}
