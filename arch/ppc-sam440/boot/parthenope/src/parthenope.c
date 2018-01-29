/* parthenope.c */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "context.h"
#include "device.h"
#include "tftp.h"
#include "ext2.h"
#include "sfs.h"
#include "dos.h"
#include "menu.h"
#include "elf.h"
#include "image.h"
#include "cdrom.h"

char __attribute__ ((__used__)) * version =
    "\0$VER: Parthenope 0." VERSION " (" DATE ") "
    "Copyright (C) 2008 Giuseppe Coviello. "
    "This is free software.  You may redistribute copies of it under the "
    "terms of the GNU General Public License "
    "<http://www.gnu.org/licenses/gpl.html>. There is NO WARRANTY, "
    "to the extent permitted by law.";

extern unsigned long __bss_start;
extern unsigned long _end;

list_t *debug_info;

#define __startup __attribute__((section(".aros.startup")))

static void clear_bss()
{
	unsigned long *ptr = &__bss_start;
	while (ptr < &_end)
		*ptr++ = 0;
}

static void flush_cache(char *start, char *end)
{
	start = (char *)((unsigned long)start & 0xffffffe0);
	end = (char *)((unsigned long)end & 0xffffffe0);
	char *ptr;

	for (ptr = start; ptr < end; ptr += 32)
		asm volatile ("dcbst 0,%0"::"r" (ptr));

	asm volatile ("sync");

	for (ptr = start; ptr < end; ptr += 32)
		asm volatile ("icbi 0,%0"::"r" (ptr));
	asm volatile ("sync; isync; ");
}

int max_entries;
static void set_progress(int progress)
{
	int p = progress * 66 / max_entries;
	video_repeat_char(7, 7, p, 219, 0);
	video_repeat_char(7 + p, 7, 66 - p, 177, 0);
}

static boot_dev_t *get_booting_device(void)
{
	SCAN_HANDLE hnd;
	hnd = get_curr_device();

	switch (hnd->ush_device.type) {
	case DEV_TYPE_HARDDISK:
		return NULL;
		break;
	case DEV_TYPE_NETBOOT:
		return tftp_create();
		break;
	case DEV_TYPE_CDROM:
		return cdrom_create();
		break;
	}

	return NULL;
}

void testboot_linux(menu_t * entry, void *kernel, boot_dev_t * dev)
{
	image_header_t *header;
	char *argv[3];
	int argc;
	void *initrd;

	header = kernel;

	if (header->ih_magic != IH_MAGIC
	    || header->ih_type != IH_TYPE_KERNEL
	    || header->ih_os != IH_OS_LINUX)
		return;

	setenv("stdout", "vga");
	printf("We should boot %s:\n\t%s %s\n\t%s\n", entry->title,
	       entry->kernel, entry->append, entry->initrd);

	if (entry->append != NULL)
		setenv("bootargs", entry->append);
	else
		setenv("bootargs", "");

	argc = 2;
	argv[0] = "bootm";
	argv[1] = malloc(32);
	sprintf(argv[1], "%p", kernel);
	argv[2] = NULL;

	if (entry->initrd != NULL) {
		initrd = malloc(18 * 1024 * 1024);
		dev->load_file(dev, entry->initrd, initrd);
		argc = 3;
		argv[2] = malloc(32);
		sprintf(argv[2], "%p", initrd);
	}

	bootm(NULL, 0, argc, argv);
}

void testboot_standalone(menu_t * entry, void *kernel, boot_dev_t * dev)
{
	image_header_t *header;
	char *argv[2];
	int argc;

	header = kernel;

	if (header->ih_magic != IH_MAGIC
	    || header->ih_type != IH_TYPE_STANDALONE)
		return;

	setenv("autostart", "yes");
	setenv("stdout", "vga");

	argc = 2;
	argv[0] = "bootm";
	argv[1] = malloc(32);
	sprintf(argv[1], "%p", kernel);

	bootm(NULL, 0, argc, argv);
}

void testboot_aros(menu_t * menu, void *kernel, boot_dev_t * boot)
{
	const uint32_t magic = ('A' << 24) | ('R' << 16) | ('O' << 8) | 'S';
	int i;
	char tmpbuf[100];
	void *file_buff = malloc(10 * 1024 * 1024);
	tagitem_t items[50];
	tagitem_t *tags = &items[0];

	if (!load_elf_file(menu->kernel, kernel))
		return;

	max_entries = menu->modules_cnt + 1;
	sprintf(tmpbuf, "Booting %s", menu->title);
	video_clear();
	video_set_partial_scroll_limits(12, 25);

	video_draw_box(1, 0, tmpbuf, 1, 5, 4, 70, 7);
	set_progress(1);
	video_draw_text(7, 9, 0, menu->kernel, 66);

	for (i = 0; i < menu->modules_cnt; i++) {
		printf("[BOOT] Loading file '%s'\n", menu->modules[i]->name);
		if (boot->load_file(boot, menu->modules[i]->name, file_buff) < 0) {
			return;
		}
		if (!load_elf_file(menu->modules[i]->name, file_buff)) {
			printf("[BOOT] Load ERROR\n");
			return;
		}
		set_progress(i + 2);
		video_draw_text(7, 9, 0, menu->modules[i]->name, 66);
	}

	void (*entry) (void *, uint32_t);
	flush_cache(get_ptr_rw(), get_ptr_ro());

	entry = (void *)KERNEL_PHYS_BASE;

	tags->ti_tag = KRN_KernelBss;
	tags->ti_data = (unsigned long)&tracker[0];
	tags++;

	tags->ti_tag = KRN_KernelLowest;
	tags->ti_data = (unsigned long)get_ptr_rw();
	tags++;

	tags->ti_tag = KRN_KernelHighest;
	tags->ti_data = (unsigned long)get_ptr_ro();
	tags++;

	tags->ti_tag = KRN_KernelBase;
	tags->ti_data = (unsigned long)KERNEL_PHYS_BASE;
	tags++;

	tags->ti_tag = KRN_ProtAreaStart;
	tags->ti_data = (unsigned long)KERNEL_VIRT_BASE;
	tags++;

	tags->ti_tag = KRN_ProtAreaEnd;
	tags->ti_data = (unsigned long)get_ptr_ro();
	tags++;

	tags->ti_tag = KRN_CmdLine;
	tags->ti_data = (unsigned long)menu->append;
	tags++;

	tags->ti_tag = KRN_BootLoader;
	tags->ti_data = (unsigned long)"Parthenope 0." VERSION " (" DATE ")";
	tags++;

	tags->ti_tag = KRN_DebugInfo;
	tags->ti_data = (unsigned long)debug_info;
	tags++;

	tags->ti_tag = 0;

	struct bss_tracker *bss = &tracker[0];
	while (bss->addr) {
		printf("[BOOT] Bss: %p-%p, %08x\n",
		       bss->addr, (char *)bss->addr + bss->length - 1,
		       bss->length);
		bss++;
	}

        printf("[BOOT] Jumping into kernel @ %p\n", entry);
	entry(&items[0], magic);

	printf("[BOOT] Shouldn't be back...\n");

	while (1) ;
}

struct module {
	node_t node;
	char *name;
	char *options;
	uint16_t id;
	uint16_t flags;
	void *alloc;
	void *dealloc;
	void *address;
	unsigned size;
};

void testboot_aos(menu_t * menu, void *kernel, boot_dev_t * boot)
{
	int i;
	char tmpbuf[100];
	int length;
	struct module *module;
	list_t *list;

	list = list_new();
	max_entries = menu->modules_cnt + 1;
	sprintf(tmpbuf, "Booting %s", menu->title);
	video_clear();
	video_set_partial_scroll_limits(12, 25);

	video_draw_box(1, 0, tmpbuf, 1, 5, 4, 70, 7);
	set_progress(1);
	video_draw_text(7, 9, 0, menu->kernel, 66);

	module = malloc(sizeof(struct module));
	void *buffer = malloc(16 * 1024 * 1024);
	for (i = 0; i < menu->modules_cnt; i++) {
		video_draw_text(7, 9, 0, menu->modules[i]->name, 66);
		if ((length = boot->load_file(boot, menu->modules[i]->name,
					      buffer)) < 0)
			return;
		module = malloc(sizeof(struct module));
		module->name = menu->modules[i]->name;
		module->options = NULL;
		module->id = 0x2;
		module->flags = 0x3;
		module->alloc = NULL;
		module->dealloc = NULL;
		module->address = malloc(length);
		memmove((char *)module->address, (char *)buffer, length);
		module->size = length;
		list_append(list, (node_t *) module);
		set_progress(i + 2);
	}
	((void (*)(unsigned char *, void *, void *, void *))
	 kernel) (NULL, list, context_get()->c_get_board_info(),
		  getenv("os4_commandline"));
}

int __startup bootstrap(context_t * ctx)
{
	boot_dev_t *boot;
	menu_t *menu, *entry;

	clear_bss();

	context_init(ctx);

	setenv("stdout", "serial");

	debug_info = list_new();

	video_clear();
	video_draw_text(5, 4, 0, " Parthenope (ub2lb) version 0." VERSION, 80);

	RdbPartitionTable_init();
	boot = get_booting_device();

	menu = menu_load(boot);
	if (menu == NULL) {
		setenv("stdout", "vga");
		printf("No menu.lst or Kicklayout found!\n");
		goto exit;
	}

	entry = menu_display(menu);

	if (entry == NULL)
		goto exit;

	if (entry->other != NULL) 
		return bootu(entry->other);
	
	switch (entry->device_type) {
	case IDE_TYPE:
	{
		struct RdbPartition *partition;
		boot = NULL;
		partition = RdbPartitionTable_get(entry->device_num,
						  entry->partition);
		if(partition == NULL)
			break;
		boot = dos_create(partition);
		if (boot == NULL)
		    boot = sfs_create(partition);
		if (boot == NULL)
		    boot = ext2_create(partition);
	}
		break;
	case TFTP_TYPE:
		boot = tftp_create();
		break;
	case CD_TYPE:
		boot = cdrom_create();
		break;
	}

	if (boot == NULL)
		goto exit;

	void *kernel = malloc(5 * 1024 * 1024);
	if (boot->load_file(boot, entry->kernel, kernel) < 0)
		return 1;

	testboot_standalone(entry, kernel, boot);
	testboot_linux(entry, kernel, boot);
	testboot_aros(entry, kernel, boot);
	testboot_aos(entry, kernel, boot);

	free(kernel);

	boot->destroy(boot);
exit:
	setenv("stdout", "vga");
	printf("Press a key to open U-Boot prompt!\n");
	video_get_key();
	return 0;
}
