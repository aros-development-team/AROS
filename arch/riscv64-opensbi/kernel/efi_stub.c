/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.

    Desc: UEFI boot stub for the opensbi-riscv64 target.

    Real hardware generally boots through UEFI
    rather than handing over from OpenSBI directly. The image carries a
    PE/COFF header (see startup.S) so the firmware can load it as an EFI
    application and enter here.

    The stub does what a loader would otherwise have done for us:

      - finds the device tree in the EFI configuration table,
      - asks the firmware for the boot hart id,
      - loads the BSP package from the volume the image came from,
      - moves the image to its link address,
      - leaves boot services and enters the kernel with the ordinary
        RISC-V boot handoff (a0 = hart id, a1 = DTB).

    It is deliberately self-contained: no AROS headers, no libc, and no
    absolute data references, because it runs wherever the firmware
    decided to place the image.
*/

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long       u64;
typedef unsigned long       uintn;
typedef long                efi_status_t;
typedef void               *efi_handle_t;

#define EFI_SUCCESS             0
#define EFIERR(a)               (0x8000000000000000UL | (a))
#define EFI_BUFFER_TOO_SMALL    EFIERR(5)

#define EFI_ALLOCATE_ADDRESS    2
#define EFI_LOADER_DATA         2

#define EFI_FILE_MODE_READ      0x0000000000000001UL

struct efi_guid
{
    u32 a;
    u16 b, c;
    u8  d[8];
};

/* {b1b621d5-f19c-41a5-830b-d9152c69aae0} - the flattened device tree */
static const struct efi_guid FDT_GUID =
    { 0xb1b621d5, 0xf19c, 0x41a5,
      { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } };

/* {ccd15fec-6f73-4eec-8395-3e69e4b940bf} - RISCV_EFI_BOOT_PROTOCOL */
static const struct efi_guid RISCV_BOOT_GUID =
    { 0xccd15fec, 0x6f73, 0x4eec,
      { 0x83, 0x95, 0x3e, 0x69, 0xe4, 0xb9, 0x40, 0xbf } };

/* {5b1b31a1-9562-11d2-8e3f-00a0c969723b} - EFI_LOADED_IMAGE_PROTOCOL */
static const struct efi_guid LOADED_IMAGE_GUID =
    { 0x5b1b31a1, 0x9562, 0x11d2,
      { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

/* {964e5b22-6459-11d2-8e39-00a0c969723b} - SIMPLE_FILE_SYSTEM */
static const struct efi_guid SIMPLE_FS_GUID =
    { 0x964e5b22, 0x6459, 0x11d2,
      { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

/* {09576e92-6d3f-11d2-8e39-00a0c969723b} - EFI_FILE_INFO */
static const struct efi_guid FILE_INFO_GUID =
    { 0x09576e92, 0x6d3f, 0x11d2,
      { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } };

/* {8868e871-e4f1-11d3-bc22-0080c73c8881} - ACPI 2.0+ RSDP */
static const struct efi_guid ACPI20_GUID =
    { 0x8868e871, 0xe4f1, 0x11d3,
      { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } };

/* {eb9d2d30-2d88-11d3-9a16-0090273fc14d} - ACPI 1.0 RSDP */
static const struct efi_guid ACPI10_GUID =
    { 0xeb9d2d30, 0x2d88, 0x11d3,
      { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } };

/* {9042a9de-23dc-4a38-96fb-7aded080516a} - GRAPHICS_OUTPUT_PROTOCOL */
static const struct efi_guid GOP_GUID =
    { 0x9042a9de, 0x23dc, 0x4a38,
      { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } };

struct efi_simple_text_output
{
    void        *reset;
    efi_status_t (*output_string)(struct efi_simple_text_output *, u16 *);
};

struct efi_config_table
{
    struct efi_guid  guid;
    void            *table;
};

struct efi_boot_services
{
    char  pad0[24];                                     /* EFI_TABLE_HEADER */
    void *raise_tpl;
    void *restore_tpl;
    efi_status_t (*allocate_pages)(int, int, uintn, u64 *);
    efi_status_t (*free_pages)(u64, uintn);
    efi_status_t (*get_memory_map)(uintn *, void *, uintn *, uintn *, u32 *);
    efi_status_t (*allocate_pool)(int, uintn, void **);
    efi_status_t (*free_pool)(void *);
    void *create_event;
    void *set_timer;
    void *wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    efi_status_t (*handle_protocol)(efi_handle_t, const struct efi_guid *,
                                    void **);
    void *reserved;
    void *register_protocol_notify;
    void *locate_handle;
    void *locate_device_path;
    void *install_configuration_table;
    void *load_image;
    void *start_image;
    void *exit;
    void *unload_image;
    efi_status_t (*exit_boot_services)(efi_handle_t, uintn);
    void *get_next_monotonic_count;
    void *stall;
    void *set_watchdog_timer;
    void *connect_controller;
    void *disconnect_controller;
    void *open_protocol;
    void *close_protocol;
    void *open_protocol_information;
    void *protocols_per_handle;
    void *locate_handle_buffer;
    efi_status_t (*locate_protocol)(const struct efi_guid *, void *, void **);
};

struct efi_gop_pixel_bitmask
{
    u32 red;
    u32 green;
    u32 blue;
    u32 reserved;
};

/* Pixel byte order in memory, lowest address first */
#define EFI_GOP_PIXEL_RGBX      0
#define EFI_GOP_PIXEL_BGRX      1
#define EFI_GOP_PIXEL_BITMASK   2
#define EFI_GOP_PIXEL_BLT_ONLY  3

struct efi_gop_mode_info
{
    u32 version;
    u32 horizontal_resolution;
    u32 vertical_resolution;
    u32 pixel_format;
    struct efi_gop_pixel_bitmask pixel_bitmask;
    u32 pixels_per_scanline;
};

struct efi_gop_mode
{
    u32 max_mode;
    u32 mode;
    struct efi_gop_mode_info *info;
    uintn size_of_info;
    u64 framebuffer_base;
    uintn framebuffer_size;
};

struct efi_gop
{
    void *query_mode;
    void *set_mode;
    void *blt;
    struct efi_gop_mode *mode;
};

struct efi_system_table
{
    char  pad0[24];                                     /* EFI_TABLE_HEADER */
    u16  *firmware_vendor;
    u32   firmware_revision;
    void *console_in_handle;
    void *con_in;
    void *console_out_handle;
    struct efi_simple_text_output *con_out;
    void *standard_error_handle;
    void *std_err;
    void *runtime_services;
    struct efi_boot_services *boot_services;
    uintn nr_tables;
    struct efi_config_table *tables;
};

struct efi_loaded_image
{
    u32   revision;
    void *parent_handle;
    void *system_table;
    efi_handle_t device_handle;
    void *file_path;
    void *reserved;
    u32   load_options_size;
    void *load_options;
    void *image_base;
    u64   image_size;
};

struct efi_file_protocol
{
    u64 revision;
    efi_status_t (*open)(struct efi_file_protocol *,
                         struct efi_file_protocol **, u16 *, u64, u64);
    efi_status_t (*close)(struct efi_file_protocol *);
    void *del;
    efi_status_t (*read)(struct efi_file_protocol *, uintn *, void *);
    void *write;
    void *get_position;
    void *set_position;
    efi_status_t (*get_info)(struct efi_file_protocol *,
                             const struct efi_guid *, uintn *, void *);
};

struct efi_simple_file_system
{
    u64 revision;
    efi_status_t (*open_volume)(struct efi_simple_file_system *,
                                struct efi_file_protocol **);
};

struct efi_riscv_boot_protocol
{
    u64 revision;
    efi_status_t (*get_boot_hartid)(struct efi_riscv_boot_protocol *,
                                    uintn *);
};

/*
 * Set by the stub before the image is moved into place, so the values
 * travel with the copy. They must live in .data - .bss is cleared by
 * the kernel entry path.
 */
unsigned long __efi_initrd_start __attribute__((section(".data"))) = 0;
unsigned long __efi_initrd_end   __attribute__((section(".data"))) = 0;

/*
 * A UEFI system need not provide a device tree - this firmware (and
 * some machines) describes the machine with ACPI instead.
 * The stub then takes what the kernel needs from EFI itself.
 */
unsigned long __efi_rsdp __attribute__((section(".data"))) = 0;

/*
 * The EFI system table, passed on to efi.resource through the boot
 * tags. acpica.library asks it for the ACPI root pointer; without it
 * ACPICA has no way to find the tables on a machine with no PC ROM
 * area to scan.
 */
unsigned long __efi_systab __attribute__((section(".data"))) = 0;

/*
 * Every GUID in the EFI configuration table, so the kernel can report
 * what the firmware actually published. The stub's own output goes to
 * the EFI console, which is not necessarily the UART being watched.
 */
#define EFI_MAX_TABLES 48
unsigned long __efi_ntables __attribute__((section(".data"))) = 0;
unsigned char __efi_tableguids[EFI_MAX_TABLES][16]
    __attribute__((section(".data"))) = { { 0 } };

/*
 * Memory the firmware keeps: its runtime code and data (which the
 * system table and the configuration tables live in) and the ACPI
 * tables. It is deliberately absent from __efi_regions - AROS must not
 * allocate out of it - but it still has to be mapped, or the first
 * ACPI table read faults.
 */
#define EFI_MAX_FWREGIONS 24
unsigned long __efi_nfwregions __attribute__((section(".data"))) = 0;
unsigned long __efi_fwregions[EFI_MAX_FWREGIONS][2]
    __attribute__((section(".data"))) = { { 0, 0 } };
unsigned long __efi_mem_start __attribute__((section(".data"))) = 0;
unsigned long __efi_mem_size  __attribute__((section(".data"))) = 0;

/*
 * Every usable memory region EFI reports. The firmware's map is the
 * authoritative description of RAM on a UEFI machine - the device tree
 * it hands over can describe less than is actually fitted.
 */
#define EFI_MAX_REGIONS 16
unsigned long __efi_nregions __attribute__((section(".data"))) = 0;
unsigned long __efi_regions[EFI_MAX_REGIONS][2]
    __attribute__((section(".data"))) = { { 0, 0 } };

/*
 * The command line. UEFI passes one through the loaded image's
 * LoadOptions (as UTF-16), and aros.cmd on the boot volume overrides
 * that. Empty by default, so dosboot takes its normal path rather than
 * the emergency console.
 */
char __efi_cmdline[128] __attribute__((section(".data"))) = "";

/*
 * The framebuffer the firmware's Graphics Output Protocol scans out.
 * It has to be captured here: the protocol is gone after
 * ExitBootServices, but the display keeps showing the surface, so the
 * kernel can describe it to the graphics driver. Masks are in-memory
 * 32-bit pixel values. All zero when the firmware drives no display.
 */
unsigned long __efi_fb_base   __attribute__((section(".data"))) = 0;
unsigned long __efi_fb_size   __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_width  __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_height __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_pitch  __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_bpp    __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_redmask   __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_greenmask __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_bluemask  __attribute__((section(".data"))) = 0;
unsigned int  __efi_fb_resmask   __attribute__((section(".data"))) = 0;

/* The real kernel entry, past the PE header (see startup.S) */
extern void _start_kernel(unsigned long hartid, void *fdt);
extern char _start[];
extern char __kernel_end[];

static struct efi_simple_text_output *conout;

static void efi_puts(const char *s)
{
    u16 buf[128];
    int i = 0;

    if (!conout)
        return;

    while (*s && i < 120)
    {
        if (*s == '\n')
            buf[i++] = '\r';
        buf[i++] = (u16)*s++;
    }
    buf[i] = 0;
    conout->output_string(conout, buf);
}

static void efi_puthex(u64 v)
{
    static const char hex[] = "0123456789abcdef";
    char buf[19];
    int i;

    buf[0] = '0';
    buf[1] = 'x';
    for (i = 0; i < 16; i++)
        buf[2 + i] = hex[(v >> (60 - i * 4)) & 0xF];
    buf[18] = 0;
    efi_puts(buf);
}

static int guid_eq(const struct efi_guid *a, const struct efi_guid *b)
{
    const u8 *p = (const u8 *)a, *q = (const u8 *)b;
    int i;

    for (i = 0; i < (int)sizeof(struct efi_guid); i++)
        if (p[i] != q[i])
            return 0;
    return 1;
}

static void mem_copy(void *dst, const void *src, u64 len)
{
    u8 *d = dst;
    const u8 *s = src;

    while (len--)
        *d++ = *s++;
}

/*
 * Just enough SBI to reconcile the caches. The firmware above us runs in
 * S-mode like we do, so the SEE underneath is still reachable by ecall.
 * Spelled out here rather than included, as the stub carries no headers
 * of its own.
 */
#define SBI_EXT_BASE            0x10
#define SBI_BASE_PROBE_EXT      3
#define SBI_EXT_RFENCE          0x52464E43
#define SBI_EXT_LEGACY_FENCE_I  0x05

struct sbi_reply { long error; long value; };

static struct sbi_reply sbi_call(u64 eid, u64 fid, u64 arg0, u64 arg1)
{
    register u64 r_a0 asm("a0") = arg0;
    register u64 r_a1 asm("a1") = arg1;
    register u64 r_a6 asm("a6") = fid;
    register u64 r_a7 asm("a7") = eid;
    struct sbi_reply reply;

    asm volatile("ecall"
                 : "+r"(r_a0), "+r"(r_a1)
                 : "r"(r_a6), "r"(r_a7)
                 : "memory");

    reply.error = (long)r_a0;
    reply.value = (long)r_a1;
    return reply;
}

static long sbi_have(u64 eid)
{
    struct sbi_reply reply = sbi_call(SBI_EXT_BASE, SBI_BASE_PROBE_EXT,
                                      eid, 0);
    return (reply.error == 0) ? reply.value : 0;
}

/*
 * Make what was just written visible to instruction fetch, before
 * anything jumps into it.
 *
 * fence.i would do it locally, but it needs Zifencei, which the profile
 * this is built for does not include - naming it here would stop the
 * stub running on a machine without it. Ask the SEE instead: that needs
 * no extension of ours and covers every hart. A machine offering
 * neither has a fetcher that does not need telling.
 */
static void sync_icache(void)
{
    if (sbi_have(SBI_EXT_RFENCE))
        sbi_call(SBI_EXT_RFENCE, 0, 0, (u64)-1);        /* every hart */
    else if (sbi_have(SBI_EXT_LEGACY_FENCE_I))
        sbi_call(SBI_EXT_LEGACY_FENCE_I, 0, 0, 0);
}

struct efi_memory_desc
{
    u32 type;
    u32 pad;
    u64 phys;
    u64 virt;
    u64 pages;
    u64 attr;
};

#define EFI_CONVENTIONAL_MEMORY 7

/* Largest usable memory region, for when there is no device tree */
static void efi_find_memory(u8 *map, uintn mapsize, uintn descsize)
{
    uintn off;
    u64 best_base = 0, best_size = 0;

    for (off = 0; off + descsize <= mapsize; off += descsize)
    {
        struct efi_memory_desc *d = (struct efi_memory_desc *)(map + off);
        u64 size = d->pages << 12;

        if (d->type != EFI_CONVENTIONAL_MEMORY)
            continue;
        if (size > best_size)
        {
            best_size = size;
            best_base = d->phys;
        }
    }

    __efi_mem_start = best_base;
    __efi_mem_size  = best_size;
}

/* Record every usable region, coalescing the adjacent ones */
static void efi_collect_memory(u8 *map, uintn mapsize, uintn descsize)
{
    uintn off;
    unsigned long n = 0;

    for (off = 0; off + descsize <= mapsize && n < EFI_MAX_REGIONS;
         off += descsize)
    {
        struct efi_memory_desc *d = (struct efi_memory_desc *)(map + off);
        u64 base = d->phys, size = d->pages << 12;

        /*
         * Anything the OS may use once boot services are gone: the
         * conventional memory plus the areas the firmware only needed
         * while booting.
         */
        if (d->type != EFI_CONVENTIONAL_MEMORY &&
            d->type != 3 /* boot services code */ &&
            d->type != 4 /* boot services data */ &&
            d->type != 1 /* loader code */ &&
            d->type != 2 /* loader data */)
            continue;
        if (!size)
            continue;

        /* Coalesce with the previous entry when they touch */
        if (n && __efi_regions[n - 1][0] + __efi_regions[n - 1][1] == base)
        {
            __efi_regions[n - 1][1] += size;
            continue;
        }

        __efi_regions[n][0] = base;
        __efi_regions[n][1] = size;
        n++;
    }

    __efi_nregions = n;
}

/*
 * Record the regions that must stay mapped but must never be allocated
 * from: EFI runtime code/data and the ACPI tables.
 */
static void efi_collect_firmware(u8 *map, uintn mapsize, uintn descsize)
{
    uintn off;
    unsigned long n = 0;

    for (off = 0; off + descsize <= mapsize && n < EFI_MAX_FWREGIONS;
         off += descsize)
    {
        struct efi_memory_desc *d = (struct efi_memory_desc *)(map + off);
        u64 base = d->phys, size = d->pages << 12;

        if (d->type != 5  /* runtime services code */ &&
            d->type != 6  /* runtime services data */ &&
            d->type != 9  /* ACPI reclaim           */ &&
            d->type != 10 /* ACPI NVS               */)
            continue;
        if (!size)
            continue;

        if (n && __efi_fwregions[n - 1][0] + __efi_fwregions[n - 1][1] == base)
        {
            __efi_fwregions[n - 1][1] += size;
            continue;
        }

        __efi_fwregions[n][0] = base;
        __efi_fwregions[n][1] = size;
        n++;
    }

    __efi_nfwregions = n;
}

/* Load the BSP package from the volume this image was loaded from */
static void efi_load_package(struct efi_system_table *st, efi_handle_t image)
{
    struct efi_boot_services *bs = st->boot_services;
    struct efi_loaded_image *li = 0;
    struct efi_simple_file_system *fs = 0;
    struct efi_file_protocol *root = 0, *file = 0;
    /* L"aros-bsp.pkg" */
    static u16 name[] = { 'a','r','o','s','-','b','s','p','.','p','k','g',0 };
    /* L"aros.cmd" - optional command line, one line of plain text */
    static u16 cmdname[] = { 'a','r','o','s','.','c','m','d',0 };
    struct efi_file_protocol *cmd = 0;
    int have_options = 0;
    u8 info[512];
    uintn infosz = sizeof(info);
    u64 size, addr;
    uintn readsz;

    if (bs->handle_protocol(image, &LOADED_IMAGE_GUID, (void **)&li) !=
        EFI_SUCCESS || !li)
        return;

    /* Take the command line while we have the loaded image */
    if (li->load_options && li->load_options_size >= 2)
    {
        u16 *o = li->load_options;
        int n = li->load_options_size / 2, i, j = 0;

        for (i = 0; i < n && j < (int)sizeof(__efi_cmdline) - 1; i++)
        {
            if (!o[i])
                break;
            __efi_cmdline[j++] = (char)o[i];
        }
        if (j)
        {
            __efi_cmdline[j] = 0;
            have_options = 1;
        }
    }
    if (bs->handle_protocol(li->device_handle, &SIMPLE_FS_GUID,
                            (void **)&fs) != EFI_SUCCESS || !fs)
        return;
    if (fs->open_volume(fs, &root) != EFI_SUCCESS || !root)
        return;
    /*
     * An explicit command line always wins, but this image is normally
     * started by the firmware's default boot path, where there is no way
     * to supply one. Fall back to "aros.cmd" beside the package so the
     * command line can be changed by editing a file on the boot volume.
     */
    if (!have_options &&
        root->open(root, &cmd, cmdname, EFI_FILE_MODE_READ, 0) == EFI_SUCCESS
        && cmd)
    {
        uintn n = sizeof(__efi_cmdline) - 1;

        if (cmd->read(cmd, &n, __efi_cmdline) == EFI_SUCCESS && n)
        {
            uintn i;

            /* One line only, and drop anything unprintable (CR/LF/tabs) */
            for (i = 0; i < n; i++)
            {
                if (__efi_cmdline[i] < ' ')
                    break;
            }
            __efi_cmdline[i] = 0;

            efi_puts("AROS: command line from aros.cmd: ");
            efi_puts(__efi_cmdline);
            efi_puts("\n");
        }
        cmd->close(cmd);
    }

    if (root->open(root, &file, name, EFI_FILE_MODE_READ, 0) != EFI_SUCCESS
        || !file)
    {
        efi_puts("AROS: no aros-bsp.pkg on the boot volume\n");
        root->close(root);
        return;
    }

    if (file->get_info(file, &FILE_INFO_GUID, &infosz, info) != EFI_SUCCESS)
        goto out;

    /* EFI_FILE_INFO: Size, FileSize, ... - FileSize is at offset 8 */
    size = *(u64 *)(info + 8);
    if (!size)
        goto out;

    if (bs->allocate_pool(EFI_LOADER_DATA, size, (void **)&addr) !=
        EFI_SUCCESS)
        goto out;

    readsz = size;
    if (file->read(file, &readsz, (void *)addr) != EFI_SUCCESS)
        goto out;

    __efi_initrd_start = addr;
    __efi_initrd_end   = addr + readsz;

    efi_puts("AROS: package loaded at ");
    efi_puthex(addr);
    efi_puts("\n");

out:
    file->close(file);
    root->close(root);
}

/*
 * Capture the firmware framebuffer while the Graphics Output Protocol
 * can still be asked about it. Only the mode the firmware already set
 * is taken - picking a different one is a policy decision that can
 * wait until a real display driver exists.
 */
static void efi_collect_gfx(struct efi_boot_services *bs)
{
    struct efi_gop *gop = 0;
    struct efi_gop_mode_info *mi;
    u32 rmask, gmask, bmask, xmask, allmask;
    u32 bpp;

    if (bs->locate_protocol(&GOP_GUID, 0, (void **)&gop) != EFI_SUCCESS ||
        !gop || !gop->mode || !gop->mode->info)
        return;

    mi = gop->mode->info;

    switch (mi->pixel_format)
    {
    case EFI_GOP_PIXEL_RGBX:
        rmask = 0x000000FF; gmask = 0x0000FF00;
        bmask = 0x00FF0000; xmask = 0xFF000000;
        break;
    case EFI_GOP_PIXEL_BGRX:
        bmask = 0x000000FF; gmask = 0x0000FF00;
        rmask = 0x00FF0000; xmask = 0xFF000000;
        break;
    case EFI_GOP_PIXEL_BITMASK:
        rmask = mi->pixel_bitmask.red;
        gmask = mi->pixel_bitmask.green;
        bmask = mi->pixel_bitmask.blue;
        xmask = mi->pixel_bitmask.reserved;
        break;
    default:
        /* Blt-only: no linear framebuffer to draw into */
        return;
    }

    /* Pixel size: the smallest whole number of bytes holding every mask */
    allmask = rmask | gmask | bmask | xmask;
    if (!allmask)
        return;
    for (bpp = 32; bpp > 8; bpp -= 8)
    {
        if (allmask & (0xFFU << (bpp - 8)))
            break;
    }

    __efi_fb_base      = gop->mode->framebuffer_base;
    __efi_fb_size      = gop->mode->framebuffer_size;
    __efi_fb_width     = mi->horizontal_resolution;
    __efi_fb_height    = mi->vertical_resolution;
    __efi_fb_pitch     = mi->pixels_per_scanline * (bpp >> 3);
    __efi_fb_bpp       = bpp;
    __efi_fb_redmask   = rmask;
    __efi_fb_greenmask = gmask;
    __efi_fb_bluemask  = bmask;
    __efi_fb_resmask   = xmask;

    efi_puts("AROS: framebuffer at ");
    efi_puthex(__efi_fb_base);
    efi_puts("\n");
}

/*
 * EFI entry point. Named in the PE header (see startup.S).
 */
efi_status_t efi_stub_entry(efi_handle_t image, struct efi_system_table *st)
{
    struct efi_boot_services *bs = st->boot_services;
    struct efi_riscv_boot_protocol *rvboot = 0;
    void *fdt = 0;
    uintn hartid = 0;
    uintn i;
    u64 target, imgsize;
    uintn mapsize = 0, mapkey = 0, descsize = 0;
    u32 descver = 0;
    u8 *dummy = 0;
    void (*entry)(unsigned long, void *);

    conout = st->con_out;
    __efi_systab = (unsigned long)st;
    efi_puts("\nAROS/riscv64 UEFI stub\n");

    /* The device tree comes from the EFI configuration table */
    for (i = 0; i < st->nr_tables; i++)
    {
        if (guid_eq(&st->tables[i].guid, &FDT_GUID))
        {
            fdt = st->tables[i].table;
            break;
        }
    }
    /* Keep the whole configuration table list for the kernel to report */
    for (i = 0; i < st->nr_tables && i < EFI_MAX_TABLES; i++)
    {
        const unsigned char *g = (const unsigned char *)&st->tables[i].guid;
        unsigned int b;

        for (b = 0; b < 16; b++)
            __efi_tableguids[i][b] = g[b];
    }
    __efi_ntables = (st->nr_tables < EFI_MAX_TABLES) ? st->nr_tables
                                                     : EFI_MAX_TABLES;

    /*
     * The ACPI root pointer, likewise. Prefer 2.0+ (XSDT, 64bit
     * addresses); the 1.0 table is only a fallback.
     */
    for (i = 0; i < st->nr_tables; i++)
    {
        if (guid_eq(&st->tables[i].guid, &ACPI20_GUID))
        {
            __efi_rsdp = (unsigned long)st->tables[i].table;
            break;
        }
        if (guid_eq(&st->tables[i].guid, &ACPI10_GUID) && !__efi_rsdp)
            __efi_rsdp = (unsigned long)st->tables[i].table;
    }
    if (__efi_rsdp)
    {
        efi_puts("AROS: ACPI RSDP at ");
        efi_puthex(__efi_rsdp);
        efi_puts("\n");
    }

    if (fdt)
    {
        efi_puts("AROS: dtb at ");
        efi_puthex((u64)fdt);
        efi_puts("\n");
    }
    else
        efi_puts("AROS: no device tree - describing the machine from EFI\n");

    /* The boot hart id, if the firmware implements the protocol */
    if (bs->handle_protocol(image, &RISCV_BOOT_GUID, (void **)&rvboot) ==
        EFI_SUCCESS && rvboot)
        rvboot->get_boot_hartid(rvboot, &hartid);

    efi_load_package(st, image);

    efi_collect_gfx(bs);

    /*
     * Move the image to its link address. The kernel is linked to run
     * there and the firmware will have placed us somewhere else.
     */
    target  = (u64)0x80200000;
    imgsize = (u64)(__kernel_end - _start);

    if (bs->allocate_pages(EFI_ALLOCATE_ADDRESS, EFI_LOADER_DATA,
                           (imgsize + 0xFFF) >> 12, &target) != EFI_SUCCESS)
    {
        efi_puts("AROS: could not claim the kernel load address!\n");
        return EFIERR(9);
    }

    efi_puts("AROS: relocating to ");
    efi_puthex(target);
    efi_puts(" and leaving boot services\n");

    entry = (void (*)(unsigned long, void *))
            (target + ((u64)&_start_kernel - (u64)_start));

    /*
     * ExitBootServices needs the current memory map key. The first call
     * is expected to fail with the size, which is also how we learn how
     * big a buffer to pass.
     */
    bs->get_memory_map(&mapsize, dummy, &mapkey, &descsize, &descver);
    mapsize += descsize * 8;
    if (bs->allocate_pool(EFI_LOADER_DATA, mapsize, (void **)&dummy) !=
        EFI_SUCCESS)
        return EFIERR(9);
    if (bs->get_memory_map(&mapsize, dummy, &mapkey, &descsize, &descver) !=
        EFI_SUCCESS)
        return EFIERR(9);

    efi_collect_memory(dummy, mapsize, descsize);
    efi_collect_firmware(dummy, mapsize, descsize);
    if (!fdt)
    {
        efi_find_memory(dummy, mapsize, descsize);
        efi_puts("AROS: memory ");
        efi_puthex(__efi_mem_start);
        efi_puts(" size ");
        efi_puthex(__efi_mem_size);
        efi_puts("\n");
    }

    if (bs->exit_boot_services(image, mapkey) != EFI_SUCCESS)
    {
        /* The map changed underneath us - take it again and retry once */
        bs->get_memory_map(&mapsize, dummy, &mapkey, &descsize, &descver);
        if (bs->exit_boot_services(image, mapkey) != EFI_SUCCESS)
            return EFIERR(9);
    }

    /*
     * The boot protocol hands the kernel a machine with translation
     * off, and it is also how we escape the page protections the
     * firmware applied to its own mappings (the image would otherwise
     * be copied into no-execute pages). EDK2 identity-maps, so the code
     * running here stays valid across the switch.
     */
    asm volatile("csrw satp, zero\n"
                 "sfence.vma zero, zero" ::: "memory");

    /* Copy ourselves into place and enter the kernel */
    mem_copy((void *)target, _start, imgsize);

    sync_icache();

    entry(hartid, fdt);

    return EFI_SUCCESS;      /* not reached */
}
