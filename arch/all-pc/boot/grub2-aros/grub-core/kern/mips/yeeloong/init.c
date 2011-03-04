/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009,2010  Free Software Foundation, Inc.
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

#include <grub/kernel.h>
#include <grub/misc.h>
#include <grub/env.h>
#include <grub/time.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/machine/kernel.h>
#include <grub/machine/memory.h>
#include <grub/memory.h>
#include <grub/mips/loongson.h>
#include <grub/cs5536.h>
#include <grub/term.h>
#include <grub/machine/ec.h>

extern void grub_video_sm712_init (void);
extern void grub_video_init (void);
extern void grub_bitmap_init (void);
extern void grub_font_init (void);
extern void grub_gfxterm_init (void);
extern void grub_at_keyboard_init (void);
extern void grub_serial_init (void);
extern void grub_terminfo_init (void);
extern void grub_keylayouts_init (void);

/* FIXME: use interrupt to count high.  */
grub_uint64_t
grub_get_rtc (void)
{
  static grub_uint32_t high = 0;
  static grub_uint32_t last = 0;
  grub_uint32_t low;

  asm volatile ("mfc0 %0, " GRUB_CPU_LOONGSON_COP0_TIMER_COUNT : "=r" (low));
  if (low < last)
    high++;
  last = low;

  return (((grub_uint64_t) high) << 32) | low;
}

grub_err_t
grub_machine_mmap_iterate (grub_memory_hook_t hook)
{
  hook (GRUB_ARCH_LOWMEMPSTART, grub_arch_memsize << 20,
	GRUB_MEMORY_AVAILABLE);
  hook (GRUB_ARCH_HIGHMEMPSTART, grub_arch_highmemsize << 20,
	GRUB_MEMORY_AVAILABLE);
  return GRUB_ERR_NONE;
}

static void
init_pci (void)
{
  auto int NESTED_FUNC_ATTR set_card (grub_pci_device_t dev, grub_pci_id_t pciid);
  int NESTED_FUNC_ATTR set_card (grub_pci_device_t dev, grub_pci_id_t pciid)
  {
    grub_pci_address_t addr;
    /* FIXME: autoscan for BARs and devices.  */
    switch (pciid)
      {
      case GRUB_YEELOONG_OHCI_PCIID:
	addr = grub_pci_make_address (dev, GRUB_PCI_REG_ADDRESS_REG0);
	grub_pci_write (addr, 0x5025000);
	addr = grub_pci_make_address (dev, GRUB_PCI_REG_COMMAND);
	grub_pci_write_word (addr, GRUB_PCI_COMMAND_SERR_ENABLE
			     | GRUB_PCI_COMMAND_PARITY_ERROR
			     | GRUB_PCI_COMMAND_BUS_MASTER
			     | GRUB_PCI_COMMAND_MEM_ENABLED);

	addr = grub_pci_make_address (dev, GRUB_PCI_REG_STATUS);
	grub_pci_write_word (addr, 0x0200 | GRUB_PCI_STATUS_CAPABILITIES);
	break;
      case GRUB_YEELOONG_EHCI_PCIID:
	addr = grub_pci_make_address (dev, GRUB_PCI_REG_ADDRESS_REG0);
	grub_pci_write (addr, 0x5026000);
	addr = grub_pci_make_address (dev, GRUB_PCI_REG_COMMAND);
	grub_pci_write_word (addr, GRUB_PCI_COMMAND_SERR_ENABLE
			     | GRUB_PCI_COMMAND_PARITY_ERROR
			     | GRUB_PCI_COMMAND_BUS_MASTER
			     | GRUB_PCI_COMMAND_MEM_ENABLED);

	addr = grub_pci_make_address (dev, GRUB_PCI_REG_STATUS);
	grub_pci_write_word (addr, (1 << GRUB_PCI_STATUS_DEVSEL_TIMING_SHIFT)
			     | GRUB_PCI_STATUS_CAPABILITIES);
	break;
      }
    return 0;
  }

  *((volatile grub_uint32_t *) GRUB_CPU_LOONGSON_PCI_HIT1_SEL_LO) = 0x8000000c;
  *((volatile grub_uint32_t *) GRUB_CPU_LOONGSON_PCI_HIT1_SEL_HI) = 0xffffffff;

  /* Setup PCI controller.  */
  *((volatile grub_uint16_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER
				+ GRUB_PCI_REG_COMMAND))
    = GRUB_PCI_COMMAND_PARITY_ERROR | GRUB_PCI_COMMAND_BUS_MASTER
    | GRUB_PCI_COMMAND_MEM_ENABLED;
  *((volatile grub_uint16_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER
				+ GRUB_PCI_REG_STATUS))
    = (1 << GRUB_PCI_STATUS_DEVSEL_TIMING_SHIFT)
    | GRUB_PCI_STATUS_FAST_B2B_CAPABLE | GRUB_PCI_STATUS_66MHZ_CAPABLE
    | GRUB_PCI_STATUS_CAPABILITIES;

  *((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER
				+ GRUB_PCI_REG_CACHELINE)) = 0xff;
  *((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER 
				+ GRUB_PCI_REG_ADDRESS_REG0))
    = 0x80000000 | GRUB_PCI_ADDR_MEM_TYPE_64 | GRUB_PCI_ADDR_MEM_PREFETCH;
  *((volatile grub_uint32_t *) (GRUB_MACHINE_PCI_CONTROLLER_HEADER 
				+ GRUB_PCI_REG_ADDRESS_REG1)) = 0;

  grub_pci_iterate (set_card);
}

void
grub_machine_init (void)
{
  grub_addr_t modend;

  /* FIXME: measure this.  */
  if (grub_arch_busclock == 0)
    {
      grub_arch_busclock = 66000000;
      grub_arch_cpuclock = 797000000;
    }

  grub_install_get_time_ms (grub_rtc_get_time_ms);

  if (grub_arch_memsize == 0)
    {
      grub_port_t smbbase;
      grub_err_t err;
      grub_pci_device_t dev;
      struct grub_smbus_spd spd;
      unsigned totalmem;
      int i;

      if (!grub_cs5536_find (&dev))
	grub_fatal ("No CS5536 found\n");

      err = grub_cs5536_init_smbus (dev, 0x7ff, &smbbase);
      if (err)
	grub_fatal ("Couldn't init SMBus: %s\n", grub_errmsg);

      /* Yeeloong has only one memory slot.  */
      err = grub_cs5536_read_spd (smbbase, GRUB_SMB_RAM_START_ADDR, &spd);
      if (err)
	grub_fatal ("Couldn't read SPD: %s\n", grub_errmsg);
      for (i = 5; i < 13; i++)
	if (spd.ddr2.rank_capacity & (1 << (i & 7)))
	  break;
      /* Something is wrong.  */
      if (i == 13)
	totalmem = 256;
      else
	totalmem = ((spd.ddr2.num_of_ranks
		     & GRUB_SMBUS_SPD_MEMORY_NUM_OF_RANKS_MASK) + 1) << (i + 2);
      
      if (totalmem >= 256)
	{
	  grub_arch_memsize = 256;
	  grub_arch_highmemsize = totalmem - 256;
	}
      else
	{
	  grub_arch_memsize = (totalmem >> 20);
	  grub_arch_highmemsize = 0;
	}

      grub_cs5536_init_geode (dev);

      init_pci ();
    }

  modend = grub_modules_get_end ();
  grub_mm_init_region ((void *) modend, (grub_arch_memsize << 20)
		       - (modend - GRUB_ARCH_LOWMEMVSTART));
  /* FIXME: use upper memory as well.  */

  /* Initialize output terminal (can't be done earlier, as gfxterm
     relies on a working heap.  */
  grub_video_init ();
  grub_video_sm712_init ();
  grub_bitmap_init ();
  grub_font_init ();
  grub_gfxterm_init ();

  grub_keylayouts_init ();
  grub_at_keyboard_init ();

  grub_terminfo_init ();
  grub_serial_init ();
}

void
grub_machine_fini (void)
{
}

void
grub_halt (void)
{
  grub_outb (grub_inb (GRUB_CPU_LOONGSON_GPIOCFG)
	     & ~GRUB_CPU_LOONGSON_SHUTDOWN_GPIO, GRUB_CPU_LOONGSON_GPIOCFG);

  grub_millisleep (1500);

  grub_printf ("Shutdown failed\n");
  grub_refresh ();
  while (1);
}

void
grub_exit (void)
{
  grub_halt ();
}

void
grub_reboot (void)
{
  grub_write_ec (GRUB_MACHINE_EC_COMMAND_REBOOT);

  grub_millisleep (1500);

  grub_printf ("Reboot failed\n");
  grub_refresh ();
  while (1);
}

