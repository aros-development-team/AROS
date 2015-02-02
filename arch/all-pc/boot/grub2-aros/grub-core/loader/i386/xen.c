/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2013  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <grub/loader.h>
#include <grub/memory.h>
#include <grub/normal.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/err.h>
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/term.h>
#include <grub/cpu/linux.h>
#include <grub/video.h>
#include <grub/video_fb.h>
#include <grub/command.h>
#include <grub/xen/relocator.h>
#include <grub/i18n.h>
#include <grub/elf.h>
#include <grub/elfload.h>
#include <grub/lib/cmdline.h>
#include <grub/xen.h>
#include <grub/xen_file.h>
#include <grub/linux.h>

GRUB_MOD_LICENSE ("GPLv3+");

static struct grub_relocator *relocator = NULL;
static grub_uint64_t max_addr;
static grub_dl_t my_mod;
static int loaded = 0;
static struct start_info next_start;
static void *kern_chunk_src;
static struct grub_xen_file_info xen_inf;
static struct xen_multiboot_mod_list *xen_module_info_page;
static grub_uint64_t modules_target_start;
static grub_size_t n_modules;

#define PAGE_SIZE 4096
#define MAX_MODULES (PAGE_SIZE / sizeof (struct xen_multiboot_mod_list))
#define PAGE_SHIFT 12
#define STACK_SIZE 1048576
#define ADDITIONAL_SIZE (1 << 19)
#define ALIGN_SIZE (1 << 22)
#define LOG_POINTERS_PER_PAGE 9
#define POINTERS_PER_PAGE (1 << LOG_POINTERS_PER_PAGE)

static grub_uint64_t
page2offset (grub_uint64_t page)
{
  return page << PAGE_SHIFT;
}

#ifdef __x86_64__
#define NUMBER_OF_LEVELS 4
#define INTERMEDIATE_OR 7
#else
#define NUMBER_OF_LEVELS 3
#define INTERMEDIATE_OR 3
#endif

static grub_uint64_t
get_pgtable_size (grub_uint64_t total_pages, grub_uint64_t virt_base)
{
  if (!virt_base)
    total_pages++;
  grub_uint64_t ret = 0;
  grub_uint64_t ll = total_pages;
  int i;
  for (i = 0; i < NUMBER_OF_LEVELS; i++)
    {
      ll = (ll + POINTERS_PER_PAGE - 1) >> LOG_POINTERS_PER_PAGE;
      /* PAE wants all 4 root directories present.  */
#ifdef __i386__
      if (i == 1)
	ll = 4;
#endif
      ret += ll;
    }
  for (i = 1; i < NUMBER_OF_LEVELS; i++)
    if (virt_base >> (PAGE_SHIFT + i * LOG_POINTERS_PER_PAGE))
      ret++;
  return ret;
}

static void
generate_page_table (grub_uint64_t *where, grub_uint64_t paging_start,
		     grub_uint64_t total_pages, grub_uint64_t virt_base,
		     grub_xen_mfn_t *mfn_list)
{
  if (!virt_base)
    total_pages++;

  grub_uint64_t lx[NUMBER_OF_LEVELS], lxs[NUMBER_OF_LEVELS];
  grub_uint64_t nlx, nls, sz = 0;
  int l;

  nlx = total_pages;
  nls = virt_base >> PAGE_SHIFT;
  for (l = 0; l < NUMBER_OF_LEVELS; l++)
    {
      nlx = (nlx + POINTERS_PER_PAGE - 1) >> LOG_POINTERS_PER_PAGE;
      /* PAE wants all 4 root directories present.  */
#ifdef __i386__
      if (l == 1)
	nlx = 4;
#endif
      lx[l] = nlx;
      sz += lx[l];
      lxs[l] = nls & (POINTERS_PER_PAGE - 1);
      if (nls && l != 0)
	sz++;
      nls >>= LOG_POINTERS_PER_PAGE;
    }

  grub_uint64_t lp;
  grub_uint64_t j;
  grub_uint64_t *pg = (grub_uint64_t *) where;
  int pr = 0;

  grub_memset (pg, 0, sz * PAGE_SIZE);

  lp = paging_start + lx[NUMBER_OF_LEVELS - 1];
  for (l = NUMBER_OF_LEVELS - 1; l >= 1; l--)
    {
      if (lxs[l] || pr)
	pg[0] = page2offset (mfn_list[lp++]) | INTERMEDIATE_OR;
      if (pr)
	pg += POINTERS_PER_PAGE;
      for (j = 0; j < lx[l - 1]; j++)
	pg[j + lxs[l]] = page2offset (mfn_list[lp++]) | INTERMEDIATE_OR;
      pg += lx[l] * POINTERS_PER_PAGE;
      if (lxs[l])
	pr = 1;
    }

  if (lxs[0] || pr)
    pg[0] = page2offset (mfn_list[total_pages]) | 5;
  if (pr)
    pg += POINTERS_PER_PAGE;

  for (j = 0; j < total_pages; j++)
    {
      if (j >= paging_start && j < lp)
	pg[j + lxs[0]] = page2offset (mfn_list[j]) | 5;
      else
	pg[j + lxs[0]] = page2offset (mfn_list[j]) | 7;
    }
}

static grub_err_t
set_mfns (grub_xen_mfn_t * new_mfn_list, grub_xen_mfn_t pfn)
{
  grub_xen_mfn_t i, t;
  grub_xen_mfn_t cn_pfn = -1, st_pfn = -1;
  struct mmu_update m2p_updates[4];


  for (i = 0; i < grub_xen_start_page_addr->nr_pages; i++)
    {
      if (new_mfn_list[i] == grub_xen_start_page_addr->console.domU.mfn)
	cn_pfn = i;
      if (new_mfn_list[i] == grub_xen_start_page_addr->store_mfn)
	st_pfn = i;
    }
  if (cn_pfn == (grub_xen_mfn_t)-1)
    return grub_error (GRUB_ERR_BUG, "no console");
  if (st_pfn == (grub_xen_mfn_t)-1)
    return grub_error (GRUB_ERR_BUG, "no store");
  t = new_mfn_list[pfn];
  new_mfn_list[pfn] = new_mfn_list[cn_pfn];
  new_mfn_list[cn_pfn] = t;
  t = new_mfn_list[pfn + 1];
  new_mfn_list[pfn + 1] = new_mfn_list[st_pfn];
  new_mfn_list[st_pfn] = t;

  m2p_updates[0].ptr = page2offset (new_mfn_list[pfn]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[0].val = pfn;
  m2p_updates[1].ptr =
    page2offset (new_mfn_list[pfn + 1]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[1].val = pfn + 1;
  m2p_updates[2].ptr =
    page2offset (new_mfn_list[cn_pfn]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[2].val = cn_pfn;
  m2p_updates[3].ptr =
    page2offset (new_mfn_list[st_pfn]) | MMU_MACHPHYS_UPDATE;
  m2p_updates[3].val = st_pfn;

  grub_xen_mmu_update (m2p_updates, 4, NULL, DOMID_SELF);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_xen_boot (void)
{
  struct grub_relocator_xen_state state;
  grub_relocator_chunk_t ch;
  grub_err_t err;
  grub_size_t pgtsize;
  struct start_info *nst;
  grub_uint64_t nr_info_pages;
  grub_uint64_t nr_pages, nr_pt_pages, nr_need_pages;
  struct gnttab_set_version gnttab_setver;
  grub_xen_mfn_t *new_mfn_list;
  grub_size_t i;

  if (grub_xen_n_allocated_shared_pages)
    return grub_error (GRUB_ERR_BUG, "active grants");

  state.mfn_list = max_addr;
  next_start.mfn_list = max_addr + xen_inf.virt_base;
  next_start.first_p2m_pfn = max_addr >> PAGE_SHIFT;	/* Is this right? */
  pgtsize = sizeof (grub_xen_mfn_t) * grub_xen_start_page_addr->nr_pages;
  err = grub_relocator_alloc_chunk_addr (relocator, &ch, max_addr, pgtsize);
  next_start.nr_p2m_frames = (pgtsize + PAGE_SIZE - 1) >> PAGE_SHIFT;
  if (err)
    return err;
  new_mfn_list = get_virtual_current_address (ch);
  grub_memcpy (new_mfn_list,
	       (void *) grub_xen_start_page_addr->mfn_list, pgtsize);
  max_addr = ALIGN_UP (max_addr + pgtsize, PAGE_SIZE);

  err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					 max_addr, sizeof (next_start));
  if (err)
    return err;
  state.start_info = max_addr + xen_inf.virt_base;
  nst = get_virtual_current_address (ch);
  max_addr = ALIGN_UP (max_addr + sizeof (next_start), PAGE_SIZE);

  next_start.nr_pages = grub_xen_start_page_addr->nr_pages;
  grub_memcpy (next_start.magic, grub_xen_start_page_addr->magic,
	       sizeof (next_start.magic));
  next_start.store_mfn = grub_xen_start_page_addr->store_mfn;
  next_start.store_evtchn = grub_xen_start_page_addr->store_evtchn;
  next_start.console.domU = grub_xen_start_page_addr->console.domU;
  next_start.shared_info = grub_xen_start_page_addr->shared_info;

  err = set_mfns (new_mfn_list, max_addr >> PAGE_SHIFT);
  if (err)
    return err;
  max_addr += 2 * PAGE_SIZE;

  next_start.pt_base = max_addr + xen_inf.virt_base;
  state.paging_start = max_addr >> PAGE_SHIFT;

  nr_info_pages = max_addr >> PAGE_SHIFT;
  nr_pages = nr_info_pages;

  while (1)
    {
      nr_pages = ALIGN_UP (nr_pages, (ALIGN_SIZE >> PAGE_SHIFT));
      nr_pt_pages = get_pgtable_size (nr_pages, xen_inf.virt_base);
      nr_need_pages =
	nr_info_pages + nr_pt_pages +
	((ADDITIONAL_SIZE + STACK_SIZE) >> PAGE_SHIFT);
      if (nr_pages >= nr_need_pages)
	break;
      nr_pages = nr_need_pages;
    }

  grub_dprintf ("xen", "bootstrap domain %llx+%llx\n",
		(unsigned long long) xen_inf.virt_base,
		(unsigned long long) page2offset (nr_pages));

  err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					 max_addr, page2offset (nr_pt_pages));
  if (err)
    return err;

  generate_page_table (get_virtual_current_address (ch),
		       max_addr >> PAGE_SHIFT, nr_pages,
		       xen_inf.virt_base, new_mfn_list);

  max_addr += page2offset (nr_pt_pages);
  state.stack = max_addr + STACK_SIZE + xen_inf.virt_base;
  state.entry_point = xen_inf.entry_point;

  next_start.nr_p2m_frames += nr_pt_pages;
  next_start.nr_pt_frames = nr_pt_pages;
  state.paging_size = nr_pt_pages;

  *nst = next_start;

  grub_memset (&gnttab_setver, 0, sizeof (gnttab_setver));

  gnttab_setver.version = 1;
  grub_xen_grant_table_op (GNTTABOP_set_version, &gnttab_setver, 1);

  for (i = 0; i < ARRAY_SIZE (grub_xen_shared_info->evtchn_pending); i++)
    grub_xen_shared_info->evtchn_pending[i] = 0;

  return grub_relocator_xen_boot (relocator, state, nr_pages,
				  xen_inf.virt_base <
				  PAGE_SIZE ? page2offset (nr_pages) : 0,
				  nr_pages - 1,
				  page2offset (nr_pages - 1) +
				  xen_inf.virt_base);
}

static grub_err_t
grub_xen_unload (void)
{
  grub_dl_unref (my_mod);
  loaded = 0;
  return GRUB_ERR_NONE;
}

#define HYPERCALL_INTERFACE_SIZE 32

#ifdef __x86_64__
static grub_uint8_t template[] =
  {
    0x51, /* push %rcx */
    0x41, 0x53, /* push %r11 */
    0x48, 0xc7, 0xc0, 0xbb, 0xaa, 0x00, 0x00, 	/* mov    $0xaabb,%rax */
    0x0f, 0x05,  /* syscall  */
    0x41, 0x5b, /* pop %r11 */
    0x59, /* pop %rcx  */
    0xc3 /* ret */
  };

static grub_uint8_t template_iret[] =
  {
    0x51, /* push   %rcx */
    0x41, 0x53, /* push   %r11 */
    0x50, /* push   %rax */
    0x48, 0xc7, 0xc0, 0x17, 0x00, 0x00, 0x00, /* mov    $0x17,%rax */
    0x0f, 0x05 /* syscall */
  };
#define CALLNO_OFFSET 6
#else

static grub_uint8_t template[] =
  {
    0xb8, 0xbb, 0xaa, 0x00, 0x00, /* mov imm32, %eax */
    0xcd, 0x82,  /* int $0x82  */
    0xc3 /* ret */
  };

static grub_uint8_t template_iret[] =
  {
    0x50, /* push   %eax */
    0xb8, 0x17, 0x00, 0x00, 0x00, /* mov    $0x17,%eax */
    0xcd, 0x82,  /* int $0x82  */
  };
#define CALLNO_OFFSET 1

#endif


static void
set_hypercall_interface (grub_uint8_t *tgt, unsigned callno)
{
  if (callno == 0x17)
    {
      grub_memcpy (tgt, template_iret, ARRAY_SIZE (template_iret));
      grub_memset (tgt + ARRAY_SIZE (template_iret), 0xcc,
		   HYPERCALL_INTERFACE_SIZE - ARRAY_SIZE (template_iret));
      return;
    }
  grub_memcpy (tgt, template, ARRAY_SIZE (template));
  grub_memset (tgt + ARRAY_SIZE (template), 0xcc,
	       HYPERCALL_INTERFACE_SIZE - ARRAY_SIZE (template));
  tgt[CALLNO_OFFSET] = callno & 0xff;
  tgt[CALLNO_OFFSET + 1] = callno >> 8;
}

#ifdef __x86_64__
#define grub_elfXX_load grub_elf64_load
#else
#define grub_elfXX_load grub_elf32_load
#endif

static grub_err_t
grub_cmd_xen (grub_command_t cmd __attribute__ ((unused)),
	      int argc, char *argv[])
{
  grub_file_t file;
  grub_elf_t elf;
  grub_err_t err;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  grub_loader_unset ();

  grub_memset (&next_start, 0, sizeof (next_start));

  xen_module_info_page = NULL;
  n_modules = 0;

  grub_create_loader_cmdline (argc - 1, argv + 1,
			      (char *) next_start.cmd_line,
			      sizeof (next_start.cmd_line) - 1);

  file = grub_file_open (argv[0]);
  if (!file)
    return grub_errno;

  elf = grub_xen_file (file);
  if (!elf)
    goto fail;

  err = grub_xen_get_info (elf, &xen_inf);
  if (err)
    goto fail;
#ifdef __x86_64__
  if (xen_inf.arch != GRUB_XEN_FILE_X86_64)
#else
  if (xen_inf.arch != GRUB_XEN_FILE_I386_PAE
      && xen_inf.arch != GRUB_XEN_FILE_I386_PAE_BIMODE)
#endif
    {
      grub_error (GRUB_ERR_BAD_OS, "incompatible architecture: %d",
		  xen_inf.arch);
      goto fail;
    }

  if (xen_inf.virt_base & (PAGE_SIZE - 1))
    {
      grub_error (GRUB_ERR_BAD_OS, "unaligned virt_base");
      goto fail;
    }
  grub_dprintf ("xen", "virt_base = %llx, entry = %llx\n",
		(unsigned long long) xen_inf.virt_base,
		(unsigned long long) xen_inf.entry_point);

  relocator = grub_relocator_new ();
  if (!relocator)
    goto fail;

  grub_relocator_chunk_t ch;
  grub_addr_t kern_start = xen_inf.kern_start - xen_inf.paddr_offset;
  grub_addr_t kern_end = xen_inf.kern_end - xen_inf.paddr_offset;

  if (xen_inf.has_hypercall_page)
    {
      grub_dprintf ("xen", "hypercall page at 0x%llx\n",
		    (unsigned long long) xen_inf.hypercall_page);
      if (xen_inf.hypercall_page - xen_inf.virt_base < kern_start)
	kern_start = xen_inf.hypercall_page - xen_inf.virt_base;

      if (xen_inf.hypercall_page - xen_inf.virt_base + PAGE_SIZE > kern_end)
	kern_end = xen_inf.hypercall_page - xen_inf.virt_base + PAGE_SIZE;
    }

  max_addr = ALIGN_UP (kern_end, PAGE_SIZE);

  err = grub_relocator_alloc_chunk_addr (relocator, &ch, kern_start,
					 kern_end - kern_start);
  if (err)
    goto fail;
  kern_chunk_src = get_virtual_current_address (ch);

  grub_dprintf ("xen", "paddr_offset = 0x%llx\n",
		(unsigned long long) xen_inf.paddr_offset);
  grub_dprintf ("xen", "kern_start = 0x%llx, kern_end = 0x%llx\n",
		(unsigned long long) xen_inf.kern_start,
		(unsigned long long) xen_inf.kern_end);

  err = grub_elfXX_load (elf, argv[0],
			 (grub_uint8_t *) kern_chunk_src - kern_start
			 - xen_inf.paddr_offset, 0, 0, 0);

  if (xen_inf.has_hypercall_page)
    {
      unsigned i;
      for (i = 0; i < PAGE_SIZE / HYPERCALL_INTERFACE_SIZE; i++)
	set_hypercall_interface ((grub_uint8_t *) kern_chunk_src +
				 i * HYPERCALL_INTERFACE_SIZE +
				 xen_inf.hypercall_page - xen_inf.virt_base -
				 kern_start, i);
    }

  if (err)
    goto fail;

  grub_dl_ref (my_mod);
  loaded = 1;

  grub_loader_set (grub_xen_boot, grub_xen_unload, 0);
  loaded = 1;

  goto fail;

fail:

  if (elf)
    grub_elf_close (elf);
  else if (file)
    grub_file_close (file);

  if (grub_errno != GRUB_ERR_NONE)
    loaded = 0;

  return grub_errno;
}

static grub_err_t
grub_cmd_initrd (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_size_t size = 0;
  grub_err_t err;
  struct grub_linux_initrd_context initrd_ctx;
  grub_relocator_chunk_t ch;

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!loaded)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT,
		  N_("you need to load the kernel first"));
      goto fail;
    }

  if (next_start.mod_start || next_start.mod_len)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, N_("initrd already loaded"));
      goto fail;
    }

  if (grub_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  size = grub_get_initrd_size (&initrd_ctx);

  if (size)
    {
      err = grub_relocator_alloc_chunk_addr (relocator, &ch, max_addr, size);
      if (err)
	return err;

      if (grub_initrd_load (&initrd_ctx, argv,
			    get_virtual_current_address (ch)))
	goto fail;
    }

  next_start.mod_start = max_addr + xen_inf.virt_base;
  next_start.mod_len = size;

  max_addr = ALIGN_UP (max_addr + size, PAGE_SIZE);

  grub_dprintf ("xen", "Initrd, addr=0x%x, size=0x%x\n",
		(unsigned) next_start.mod_start, (unsigned) size);

fail:
  grub_initrd_close (&initrd_ctx);

  return grub_errno;
}

static grub_err_t
grub_cmd_module (grub_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  grub_size_t size = 0;
  grub_err_t err;
  grub_relocator_chunk_t ch;
  grub_size_t cmdline_len;
  int nounzip = 0;
  grub_file_t file;

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (grub_strcmp (argv[0], "--nounzip") == 0)
    {
      argv++;
      argc--;
      nounzip = 1;
    }

  if (argc == 0)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (!loaded)
    {
      return grub_error (GRUB_ERR_BAD_ARGUMENT,
			 N_("you need to load the kernel first"));
    }

  if ((next_start.mod_start || next_start.mod_len) && !xen_module_info_page)
    {
      return grub_error (GRUB_ERR_BAD_ARGUMENT, N_("initrd already loaded"));
    }

  /* Leave one space for terminator.  */
  if (n_modules >= MAX_MODULES - 1)
    {
      return grub_error (GRUB_ERR_BAD_ARGUMENT, "too many modules");
    }

  if (!xen_module_info_page)
    {
      n_modules = 0;
      max_addr = ALIGN_UP (max_addr, PAGE_SIZE);
      modules_target_start = max_addr;
      next_start.mod_start = max_addr + xen_inf.virt_base;
      next_start.flags |= SIF_MULTIBOOT_MOD;

      err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					     max_addr, MAX_MODULES
					     *
					     sizeof (xen_module_info_page
						     [0]));
      if (err)
	return err;
      xen_module_info_page = get_virtual_current_address (ch);
      grub_memset (xen_module_info_page, 0, MAX_MODULES
		   * sizeof (xen_module_info_page[0]));
      max_addr += MAX_MODULES * sizeof (xen_module_info_page[0]);
    }

  max_addr = ALIGN_UP (max_addr, PAGE_SIZE);

  if (nounzip)
    grub_file_filter_disable_compression ();
  file = grub_file_open (argv[0]);
  if (!file)
    return grub_errno;
  size = grub_file_size (file);

  cmdline_len = grub_loader_cmdline_size (argc - 1, argv + 1);

  err = grub_relocator_alloc_chunk_addr (relocator, &ch,
					 max_addr, cmdline_len);
  if (err)
    goto fail;

  grub_create_loader_cmdline (argc - 1, argv + 1,
			      get_virtual_current_address (ch), cmdline_len);

  xen_module_info_page[n_modules].cmdline = max_addr - modules_target_start;
  max_addr = ALIGN_UP (max_addr + cmdline_len, PAGE_SIZE);

  if (size)
    {
      err = grub_relocator_alloc_chunk_addr (relocator, &ch, max_addr, size);
      if (err)
	goto fail;
      if (grub_file_read (file, get_virtual_current_address (ch), size)
	  != (grub_ssize_t) size)
	{
	  if (!grub_errno)
	    grub_error (GRUB_ERR_FILE_READ_ERROR,
			N_("premature end of file %s"), argv[0]);
	  goto fail;
	}
    }
  next_start.mod_len = max_addr + size - modules_target_start;
  xen_module_info_page[n_modules].mod_start = max_addr - modules_target_start;
  xen_module_info_page[n_modules].mod_end =
    max_addr + size - modules_target_start;

  n_modules++;
  grub_dprintf ("xen", "module, addr=0x%x, size=0x%x\n",
		(unsigned) max_addr, (unsigned) size);
  max_addr = ALIGN_UP (max_addr + size, PAGE_SIZE);


fail:
  grub_file_close (file);

  return grub_errno;
}

static grub_command_t cmd_xen, cmd_initrd, cmd_module, cmd_multiboot;

GRUB_MOD_INIT (xen)
{
  cmd_xen = grub_register_command ("linux", grub_cmd_xen,
				   0, N_("Load Linux."));
  cmd_multiboot = grub_register_command ("multiboot", grub_cmd_xen,
					 0, N_("Load Linux."));
  cmd_initrd = grub_register_command ("initrd", grub_cmd_initrd,
				      0, N_("Load initrd."));
  cmd_module = grub_register_command ("module", grub_cmd_module,
				      0, N_("Load module."));
  my_mod = mod;
}

GRUB_MOD_FINI (xen)
{
  grub_unregister_command (cmd_xen);
  grub_unregister_command (cmd_initrd);
  grub_unregister_command (cmd_multiboot);
  grub_unregister_command (cmd_module);
}
