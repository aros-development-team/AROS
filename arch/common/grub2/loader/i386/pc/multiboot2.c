/* multiboot2.c - boot a multiboot 2 OS image. */
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
#include <grub/multiboot2.h>
#include <grub/elf.h>
#include <grub/err.h>
#include <grub/machine/loader.h>
#include <grub/mm.h>

grub_err_t
grub_mb2_arch_elf32_hook (Elf32_Phdr *phdr, UNUSED grub_addr_t *addr)
{
  Elf32_Addr paddr = phdr->p_paddr;

  if ((paddr < grub_os_area_addr)
      || (paddr + phdr->p_memsz > grub_os_area_addr + grub_os_area_size))
    return grub_error(GRUB_ERR_OUT_OF_RANGE,"Address 0x%x is out of range", 
                      paddr);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_mb2_arch_elf64_hook (Elf64_Phdr *phdr, UNUSED grub_addr_t *addr)
{
  Elf64_Addr paddr = phdr->p_paddr;

  if ((paddr < grub_os_area_addr)
      || (paddr + phdr->p_memsz > grub_os_area_addr + grub_os_area_size))
    return (GRUB_ERR_OUT_OF_RANGE,"Address 0x%x is out of range",
            paddr);

  return GRUB_ERR_NONE;
}

grub_err_t
grub_mb2_arch_module_alloc (grub_size_t size, grub_addr_t *addr)
{
  grub_addr_t modaddr;

  modaddr = grub_memalign (MULTIBOOT2_MOD_ALIGN, size);
  if (! modaddr)
    return grub_errno;

  *addr = modaddr;
  return GRUB_ERR_NONE;
}

grub_err_t
grub_mb2_arch_module_free (grub_addr_t addr, UNUSED grub_size_t size)
{
  grub_free((void *) addr);
  return GRUB_ERR_NONE;
}

void
grub_mb2_arch_boot (grub_addr_t entry, void *tags)
{
  grub_multiboot2_real_boot (entry, tags);
}

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
           grub_free((void *) module->addr);
         }
     }
}

grub_err_t
grub_mb2_tags_arch_create (void)
{
  /* XXX Create boot device et al. */
  return GRUB_ERR_NONE;
}
