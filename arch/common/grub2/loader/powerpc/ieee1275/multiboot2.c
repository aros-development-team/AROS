/* multiboot.c - boot a multiboot 2 OS image. */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007  Free Software Foundation, Inc.
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

#include <multiboot2.h>
#include <grub/loader.h>
#include <grub/ieee1275/ieee1275.h>
#include <grub/multiboot2.h>
#include <grub/err.h>
#include <grub/elf.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/machine/kernel.h>

typedef void (*kernel_entry_t) (unsigned long, void *, int (void *),
                                unsigned long, unsigned long);

/* Claim the memory occupied by the multiboot kernel.  */
grub_err_t
grub_mb2_arch_elf32_hook (Elf32_Phdr *phdr, UNUSED grub_addr_t *addr)
{
  int rc;

  rc = grub_claimmap (phdr->p_paddr, phdr->p_memsz);
  if (rc)
    return grub_error(GRUB_ERR_OUT_OF_MEMORY, "Couldn't claim %x - %x",
		      phdr->p_paddr, phdr->p_paddr + phdr->p_memsz);

  grub_dprintf ("loader", "Loading segment at 0x%x - 0x%x\n", phdr->p_paddr,
		phdr->p_paddr + phdr->p_memsz);

  return GRUB_ERR_NONE;
}

/* Claim the memory occupied by the multiboot kernel.  */
grub_err_t
grub_mb2_arch_elf64_hook (Elf64_Phdr *phdr, UNUSED grub_addr_t *addr)
{
  int rc;

  rc = grub_claimmap (phdr->p_paddr, phdr->p_memsz);
  if (rc)
    return grub_error(GRUB_ERR_OUT_OF_MEMORY, "Couldn't claim 0x%lx - 0x%lx",
		      phdr->p_paddr, phdr->p_paddr + phdr->p_memsz);

  grub_dprintf ("loader", "Loading segment at 0x%lx - 0x%lx\n",
		(unsigned long) phdr->p_paddr,
		(unsigned long) (phdr->p_paddr + phdr->p_memsz));

  return GRUB_ERR_NONE;
}

grub_err_t
grub_mb2_arch_module_alloc (grub_size_t size, grub_addr_t *addr)
{
  int rc;

  /* XXX Will need to map on some firmwares.  */
  rc = grub_ieee1275_claim (0, size, MULTIBOOT2_MOD_ALIGN, addr);
  if (rc)
    return grub_error (GRUB_ERR_OUT_OF_MEMORY,
		       "Firmware couldn't allocate memory (size 0x%lx)", size);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_mb2_arch_module_free (grub_addr_t addr, grub_size_t size)
{
  grub_ieee1275_release (addr, size);
  return GRUB_ERR_NONE;
}

grub_err_t
grub_mb2_tags_arch_create (void)
{
  /* Nothing special.  */
  return GRUB_ERR_NONE;
}

/* Release the memory we claimed from Open Firmware above.  */
void
grub_mb2_arch_unload (struct multiboot_tag_header *tags)
{
  struct multiboot_tag_header *tag;

  /* Free all module memory in the tag list.  */
  for_each_tag (tag, tags)
    {
      if (tag->key == MULTIBOOT2_TAG_MODULE)
	{
	  struct multiboot_tag_module *module =
	      (struct multiboot_tag_module *) tag;
	  grub_ieee1275_release (module->addr, module->size);
	}
    }
}

void
grub_mb2_arch_boot (grub_addr_t entry_addr, void *tags)
{
  kernel_entry_t entry = (kernel_entry_t) entry_addr;
#if defined(__powerpc__)
  entry (MULTIBOOT2_BOOTLOADER_MAGIC, tags, grub_ieee1275_entry_fn, 0, 0);
#elif defined(__i386__)
  grub_multiboot2_real_boot (entry, tags);
#else
#error
#endif
}
