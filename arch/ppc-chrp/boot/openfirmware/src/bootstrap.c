/*
    Copyright © 2008-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0

#include <debug.h>
#include <support.h>
#include <inttypes.h>
#include <of1275.h>
#include <menu.h>
#include <elf.h>
#include <kernel.h>

typedef uint32_t ti_Tag;
typedef intptr_t ti_Data;

typedef struct {uintptr_t ti_Tag; intptr_t ti_Data; } tagitem_t;
extern struct bss_tracker tracker[MAX_BSS_SECTIONS];
list_t *debug_info;

char *bootpath = NULL;
char *bootargs;

static void flush_cache(char *start, char *end)
{
    start = (char*)((unsigned long)start & 0xffffffe0);
    end = (char*)((unsigned long)end & 0xffffffe0);
    char *ptr;

    for (ptr = start; ptr < end; ptr +=32)
    {
        asm volatile("dcbst 0,%0"::"r"(ptr));
    }
    asm volatile("sync");

    for (ptr = start; ptr < end; ptr +=32)
    {
        asm volatile("icbi 0,%0"::"r"(ptr));
    }

    asm volatile("sync; isync; ");
}

char *cmdline;

/* Loads either a single ELF file, or the package of elf files */
static int load_elf_or_package(char *name, uint8_t *base, unsigned long virt)
{
    if (base[0] == 0x7f && base[1] == 'E' && base[2] == 'L' && base[3] == 'F')
    {
	return load_elf_file(name, base, virt);
    }
    else if (base[0] == 'P' && base[1] == 'K' && base[2] == 'G' && base[3] == 0x01)
    {
	uint8_t *file = base+4;
	uint32_t total_length = *(uint32_t*)file; /* Total length of the module */
	const uint8_t *file_end = base+total_length;
	uint32_t len;

	file = base + 8;

	while(file < file_end)
	{
	    const char *filename = remove_path(file+4);

	    printf("\r\n");

	    /* get text length */
	    len = *(uint32_t*)file;
	    /* display the file name */
	    printf("    %s  ", filename);

	    file += len + 5;

	    /* get the ELF size */
	    len = *(uint32_t*)file;
	    file += 4;

	    /* load it */
	    if (!load_elf_file(filename, file, virt))
		return 0;

	    /* go to the next file */
	    file += len;
	}
	return 1;
    }
    else
    {
	printf("UNSUPPORTED FILE");
	/* Unknown files are just ignored, it's not a critical error */
	return 1;
    }
}

int bootstrap(uint32_t r3, uint32_t r4, void *r5)
{
	void *rtas, *handle;
	uint32_t rtas_size;
	uint8_t *rtas_base=NULL, *load_base=NULL;
	void *rtas_entry;
	int i;
	ofw_node_t *rootnode;
	tagitem_t *tags;
	int tagcnt = 0;

	int (*kernel_phys_entry)(tagitem_t *, uint32_t) = (int (*)())KERNEL_PHYS_BASE;
	unsigned long virt;

	struct of_region memory_region = {NULL, 0};

	ofw_init(r5);

	printf("\r\nSecond Level Bootloader for OpenFirmware\r\n");
	printf("Build %s from %s\r\n", VERSION, __DATE__);

//	void *ih1 = ofw_open("scsi0");
//	D(bug("%p\n", ih1));
//	if (ih1 != (void*)0xffffffff && ih1 != (void*)0)
//	{
//	    char buf[2048];
//	    long size;
//	    D(bug("%p\n", ofw_seek(ih1, 0, 2048)));
//	    ofw_read(ih1, buf, 2048);
//	    ofw_close(ih1);
//
//
//		int j;
//	    for (j=0; j <128; j++)
//		D(bug("%c", buf[j]));
//	}
//return -1;

	handle = ofw_find_device("/memory");
	if (handle)
	{
	    if (ofw_get_prop_len(handle, "reg") == 8)
	    {
		ofw_get_prop(handle, "reg", &memory_region, sizeof(memory_region));

		printf("Available memory %dMB at 0x%p\r\n", memory_region.size >> 20, memory_region.base);
	    }
	}

	if (!memory_region.size)
	{
	    printf("Failed to obtain memory information\n");
	    return -1;
	}

	handle = ofw_find_device("/options");

	debug_info = __claim(sizeof(list_t));
	new_list(debug_info);

	if (handle)
	{
		char str[20];
		char *c = str;
		int i = 0;
		intptr_t temp = 0;
		int base = 10;

		D(printf("load_base proplen = %d\r\n", ofw_get_prop_len(handle, "load-base")));

		ofw_get_prop(handle, "load-base", str, 20);
		str[ofw_get_prop_len(handle, "load-base")] = 0;

		if (str[0] == '0' && str[1] == 'x')
		{
			base = 16;
			c += 2;
			i += 2;
		}

		while (i < 20 && *c != 0)
		{
			temp = temp * base;
			if (*c >= '0' && *c <= '9')
				temp += *c - '0';
			else if (*c >= 'A' && *c <= 'F')
				temp += *c - 'A' + 10;
			else if (*c >= 'a' && *c <= 'f')
				temp += *c - 'a' + 10;

			c++;
		}

		load_base = (uint8_t *)temp;
		D(printf("Load base @ %p\r\n", load_base));
	}

	D(bug("Locking load-base (%p-%p) and kernel area (%p-%p)\r\n",
			load_base, load_base + LOAD_SIZE - 1,
			0x07000000, 0x07ffffff));

	ofw_claim(load_base, LOAD_SIZE, 0);
	ofw_claim((void*)0x07000000, 0x01000000, 0);

	tags = ofw_claim(NULL, sizeof(tagitem_t) * 20, 4);

	cmdline = ofw_claim(NULL, 10240, 4);

	rootnode = ofw_scan_tree();

	tags[tagcnt].ti_Tag = KRN_OpenFirmwareTree;
	tags[tagcnt].ti_Data = (intptr_t)rootnode;
	tagcnt++;

	handle = ofw_find_device("/chosen");
	bootargs = ofw_GetString(handle, "bootargs");

	D(bug("[BOOT] bootargs='%s'\n", bootargs));

	/* Parse owr own command line options */
	for(;;)
	{
	    /* Skip leading spaces */
	    while (isspace(*bootargs))
	        bootargs++;

	    if (!strncasecmp(bootargs, "--root", 6))
	    {
	    	/*
 	     	 * --root command line option overrides boot path.
	     	 * Useful on Pegasos where /chosen/bootpath doesn't include anything
	     	 * beyond physical device.
	     	 */
	    	bootpath = &bootargs[6];

	    	/* Skip all spaces between keyword and value */
	    	while (isspace(*bootpath))
		    bootpath++;

	        bootargs = bootpath + 1;
	    	/* Now skip non-spaces. This will skip the value. */
	        while (!isspace(*bootargs))
		    bootargs++;

	    	/* Separate bootpath from the rest of command line */
	    	*bootargs++ = 0;
	    }
	    else
		break;
	}

	if (!bootpath)
	    bootpath = ofw_GetString(handle, "bootpath");

	D(bug("[BOOT] bootpath='%s' bootargs='%s'\n", bootpath, bootargs));

	if (!load_menu(load_base))
	{
		int res = 0;
		menu_entry_t *selection;

		parse_menu((unsigned long)kernel_phys_entry);
		selection = execute_menu();

		if (selection)
		{
		    char buff[1024];

		    printf("\r\nLoading \"%s\"\r\n", selection->m_title);

		    virt = selection->m_virtual;

		    if (selection->m_kernel)
		    {
			/* "kernel" line is optional and obsolete */
			sprintf(buff, "load %s %s", bootpath, selection->m_kernel);
			printf("  %s  ", selection->m_kernel);
			ofw_interpret(buff);
			res = load_elf_file(remove_path(selection->m_kernel), load_base, virt);
			printf("\r\n");
		    }

		    for (i=0; i < selection->m_modules_cnt; i++)
		    {
			printf("  %s  ", selection->m_modules[i]);
			sprintf(buff, "load %s %s", bootpath, selection->m_modules[i]);
			if (!ofw_interpret(buff))
			{
			    res = load_elf_or_package(remove_path(selection->m_modules[i]), load_base, virt);
			}
			else
			{
			    printf(" !!LOAD ERROR!!");
			    res = 0;
			}
			printf("\r\n");

			if (!res)
			    break;
		    }

		    if (!res)
		    {
			printf("Failed to load kickstart!\r\n");

			ofw_release(load_base, LOAD_SIZE);
			ofw_release(rtas_base, rtas_size);
			ofw_release((void*)0x07000000, 0x01000000 - rtas_size);

			return -1;
		    }

		    if (selection->m_cmdline || strlen(bootargs))
		    {
			if (selection->m_cmdline && strlen(selection->m_cmdline))
			    sprintf(cmdline, "%s %s", selection->m_cmdline, bootargs);
			else
			    sprintf(cmdline, "%s", bootargs);

			tags[tagcnt].ti_Tag = KRN_CmdLine;
			tags[tagcnt].ti_Data = (intptr_t)cmdline;
			tagcnt++;
		    }
		}
	}

	rtas = ofw_find_device("/rtas");
	if (rtas)
	{
		ofw_get_prop(rtas, "rtas-size", &rtas_size, sizeof(rtas_size));
		D(bug("RTAS services node @ %p\r\n", rtas));
		D(bug("The size of RTAS handler is %d bytes\r\n", rtas_size));

		if (rtas_size)
		{
			void *ihandle;

			rtas_size = (rtas_size + 4095) & ~4095;

			/* adjust the ro pointer. It should be aligned at 4K boundary, just for God's sake... */

			rtas_base = get_ptr_ro();
			rtas_base = (uint8_t *)(((intptr_t)rtas_base + 4095) &~4095);
			ptr_ro_add(rtas_size + (intptr_t)rtas_base - (intptr_t)get_ptr_ro());

			rtas_base[0] = 0;

			D(printf("RTAS services will be located at %p - %p\r\n",
					rtas_base, rtas_base + rtas_size - 1));

			ihandle = ofw_open("/rtas");

			D(printf("RTAS ihandle = %p\r\n", ihandle));

			printf("RTAS instantiate returns %d\r\n", ofw_instantiate_rtas(ihandle, rtas_base, &rtas_entry));

			D(printf("RTAS entry point @ %p\r\n", rtas_entry));

			ofw_close(ihandle);

			D(printf("%d%d%d%d\r\n", rtas_base[0], rtas_base[1], rtas_base[2], rtas_base[3]));
		}
	}


	ofw_node_t *tmp = (ofw_node_t *)rootnode->on_children.l_head;

	while(tmp->on_node.n_succ)
	{
		if (!strcasecmp(tmp->on_name, "rtas"))
		{
			ofw_property_t *prop = ofw_claim(NULL, sizeof(ofw_property_t), 4);
			prop->op_length = 4;
			prop->op_name = "load_base";
			prop->op_value = &rtas_base;

			add_tail(&tmp->on_properties, &prop->op_node);

			prop = ofw_claim(NULL, sizeof(ofw_property_t), 4);
			prop->op_length = 4;
			prop->op_name = "entry";
			prop->op_value = &rtas_entry;

			add_tail(&tmp->on_properties, &prop->op_node);

			break;
		}

		tmp = (ofw_node_t *)tmp->on_node.n_succ;
	}

	tags[tagcnt].ti_Tag = KRN_KernelBss;
	tags[tagcnt].ti_Data = (intptr_t)tracker;
	tagcnt++;

	tags[tagcnt].ti_Tag = KRN_KernelBase;
	tags[tagcnt].ti_Data = (intptr_t)KERNEL_PHYS_BASE;
	tagcnt++;

	tags[tagcnt].ti_Tag = KRN_KernelLowest;
	tags[tagcnt].ti_Data = (intptr_t)get_ptr_rw();
	tagcnt++;

	tags[tagcnt].ti_Tag = KRN_KernelHighest;
	tags[tagcnt].ti_Data = (intptr_t)get_ptr_ro();
	tagcnt++;

	tags[tagcnt].ti_Tag = KRN_DebugInfo;
	tags[tagcnt].ti_Data = (intptr_t)debug_info;
	tagcnt++;

	tags[tagcnt].ti_Tag = TAG_DONE;
	tags[tagcnt].ti_Data = 0UL;

	flush_cache(get_ptr_rw(), get_ptr_ro());

	/* Jump into the JUNGLE! */
	printf("Entering kickstart at 0x%p...\r\n", kernel_phys_entry);
	kernel_phys_entry(tags, AROS_BOOT_MAGIC);

	free_menu();

	printf("!!!!!FAIL!!!!!\r\n");
	ofw_release(load_base, LOAD_SIZE);
	ofw_release(rtas_base, rtas_size);
	ofw_release((void*)0x07000000, 0x01000000 - rtas_size);

	return -1;
}
