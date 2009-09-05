/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/env.h>
#include <grub/xnu.h>
#include <grub/cpu/xnu.h>
#include <grub/efi/api.h>
#include <grub/efi/efi.h>
#include <grub/efi/uga_draw.h>
#include <grub/pci.h>
#include <grub/misc.h>

/* Setup video for xnu. Big parts are copied from linux.c. */

static grub_efi_guid_t uga_draw_guid = GRUB_EFI_UGA_DRAW_GUID;

#define RGB_MASK	0xffffff
#define RGB_MAGIC	0x121314
#define LINE_MIN	800
#define LINE_MAX	4096
#define FBTEST_STEP	(0x10000 >> 2)
#define FBTEST_COUNT	8

static int
find_line_len (grub_uint32_t *fb_base, grub_uint32_t *line_len)
{
  grub_uint32_t *base = (grub_uint32_t *) (grub_target_addr_t) *fb_base;
  int i;

  for (i = 0; i < FBTEST_COUNT; i++, base += FBTEST_STEP)
    {
      if ((*base & RGB_MASK) == RGB_MAGIC)
	{
	  int j;

	  for (j = LINE_MIN; j <= LINE_MAX; j++)
	    {
	      if ((base[j] & RGB_MASK) == RGB_MAGIC)
		{
		  *fb_base = (grub_uint32_t) (grub_target_addr_t) base;
		  *line_len = j << 2;

		  return 1;
		}
	    }

	  break;
	}
    }

  return 0;
}

static int
find_framebuf (grub_uint32_t *fb_base, grub_uint32_t *line_len)
{
  int found = 0;

  auto int NESTED_FUNC_ATTR find_card (int bus, int dev, int func,
				       grub_pci_id_t pciid);

  int NESTED_FUNC_ATTR find_card (int bus, int dev, int func,
				  grub_pci_id_t pciid)
    {
      grub_pci_address_t addr;

      addr = grub_pci_make_address (bus, dev, func, 2);
      if (grub_pci_read (addr) >> 24 == 0x3)
	{
	  int i;

	  grub_printf ("Display controller: %d:%d.%d\nDevice id: %x\n",
		       bus, dev, func, pciid);
	  addr += 8;
	  for (i = 0; i < 6; i++, addr += 4)
	    {
	      grub_uint32_t old_bar1, old_bar2, type;
	      grub_uint64_t base64;

	      old_bar1 = grub_pci_read (addr);
	      if ((! old_bar1) || (old_bar1 & GRUB_PCI_ADDR_SPACE_IO))
		continue;

	      type = old_bar1 & GRUB_PCI_ADDR_MEM_TYPE_MASK;
	      if (type == GRUB_PCI_ADDR_MEM_TYPE_64)
		{
		  if (i == 5)
		    break;

		  old_bar2 = grub_pci_read (addr + 4);
		}
	      else
		old_bar2 = 0;

	      base64 = old_bar2;
	      base64 <<= 32;
	      base64 |= (old_bar1 & GRUB_PCI_ADDR_MEM_MASK);

	      grub_printf ("%s(%d): 0x%llx\n",
			   ((old_bar1 & GRUB_PCI_ADDR_MEM_PREFETCH) ?
			    "VMEM" : "MMIO"), i,
			   (unsigned long long) base64);

	      if ((old_bar1 & GRUB_PCI_ADDR_MEM_PREFETCH) && (! found))
		{
		  *fb_base = base64;
		  if (find_line_len (fb_base, line_len))
		    found++;
		}

	      if (type == GRUB_PCI_ADDR_MEM_TYPE_64)
		{
		  i++;
		  addr += 4;
		}
	    }
	}

      return found;
    }

  grub_pci_iterate (find_card);
  return found;
}

grub_err_t
grub_xnu_set_video (struct grub_xnu_boot_params *params)
{
  grub_efi_uga_draw_protocol_t *c;
  grub_uint32_t width, height, depth, rate, pixel, fb_base, line_len;
  int ret;

  c = grub_efi_locate_protocol (&uga_draw_guid, 0);
  if (! c)
    return grub_error (GRUB_ERR_IO, "Couldn't find UGADraw");

  if (efi_call_5 (c->get_mode, c, &width, &height, &depth, &rate))
    return grub_error (GRUB_ERR_IO, "Couldn't retrieve video mode");

  grub_printf ("Video mode: %ux%u-%u@%u\n", width, height, depth, rate);

  grub_efi_set_text_mode (0);
  pixel = RGB_MAGIC;
  efi_call_10 (c->blt, c, (struct grub_efi_uga_pixel *) &pixel,
	       GRUB_EFI_UGA_VIDEO_FILL, 0, 0, 0, 0, 1, height, 0);
  ret = find_framebuf (&fb_base, &line_len);
  grub_efi_set_text_mode (1);

  if (! ret)
    return grub_error (GRUB_ERR_IO, "Can\'t find frame buffer address\n");

  grub_printf ("Frame buffer base: 0x%x\n", fb_base);
  grub_printf ("Video line length: %d\n", line_len);

  params->lfb_width = width;
  params->lfb_height = height;
  params->lfb_depth = depth;
  params->lfb_line_len = line_len;
  params->lfb_mode = GRUB_XNU_VIDEO_TEXT_IN_VIDEO;
  params->lfb_base = fb_base;
  return GRUB_ERR_NONE;
}
