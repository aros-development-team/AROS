/* lspci.c - List PCI devices.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#include <grub/pci.h>
#include <grub/dl.h>
#include <grub/normal.h>
#include <grub/misc.h>

struct grub_pci_classname
{
  int class;
  int subclass;
  char *desc;
};

static const struct grub_pci_classname grub_pci_classes[] =
  {
    { 0, 0, "" },
    { 1, 0, "SCSCI Controller" },
    { 1, 1, "IDE Controller" },
    { 1, 2, "Floppy Controller" },
    { 1, 3, "IPI Controller" },
    { 1, 4, "RAID Controller" },
    { 1, 0x80, "Mass storage Controller" },
    { 2, 0, "Ethernet Controller" },
    { 2, 1, "Token Ring Controller" },
    { 2, 2, "FDDI Controller" },
    { 2, 3, "ATM Controller" },
    { 2, 4, "ISDN Controller" },
    { 2, 0x80, "Network controller" },
    { 3, 0, "VGA Controller" },
    { 3, 1, "XGA Controller" },
    { 3, 2, "3D Controller" },
    { 3, 0x80, "Display Controller" },
    { 4, 0, "Multimedia Video Device" },
    { 4, 1, "Multimedia Audio Device" },
    { 4, 2, "Multimedia Telephony Device" },
    { 4, 0x80, "Multimedia device" },
    { 5, 0, "RAM Controller" },
    { 5, 1, "Flash Memory Controller" },
    { 5, 0x80, "Memory Controller" },
    { 6, 0, "Host Bridge" },
    { 6, 1, "ISA Bridge" },
    { 6, 2, "EISA Bride" },
    { 6, 3, "MCA Bridge" },
    { 6, 4, "PCI-PCI Bridge" },
    { 6, 5, "PCMCIA Bridge" },
    { 6, 6, "NuBus Bridge" },
    { 6, 7, "CardBus Bridge" },
    { 6, 8, "Raceway Bridge" },
    { 6, 0x80, "Unknown Bridge" },
    { 7, 0x80, "Communication controller" },
    { 8, 0x80, "System hardware" },
    { 9, 0, "Keyboard Controller" },
    { 9, 1, "Digitizer" },
    { 9, 2, "Mouse Controller" },
    { 9, 3, "Scanner Controller" },
    { 9, 4, "Gameport Controller" },
    { 9, 0x80, "Unknown Input Device" },
    { 10, 0, "Generic Docking Station" },
    { 10, 0x80, "Unkown Docking Station" },
    { 11, 0, "80386 Processor" },
    { 11, 1, "80486 Processor" },
    { 11, 2, "Pentium Processor" },
    { 11, 0x10, "Alpha Processor" },
    { 11, 0x20, "PowerPC Processor" },
    { 11, 0x30, "MIPS Processor" },
    { 11, 0x40, "Co-Processor" },
    { 11, 0x80, "Unkown Processor" },
    { 12, 0x80, "Serial Bus Controller" },
    { 13, 0x80, "Wireless Controller" },
    { 14, 0, "I2O" },
    { 15, 0, "iRDA Controller" },
    { 15, 1, "Consumer IR" },
    { 15, 0x10, "RF-Controller" },
    { 15, 0x80, "Satellite Communication Controller" },
    { 16, 0, "Network Decryption" },
    { 16, 1, "Entertainment Decryption" },
    { 16, 0x80, "Unkown Decryption Controller" },
    { 17, 0, "Digital IO Module" },
    { 17, 0x80, "Unkown Data Input System" },
    { 0, 0, 0 },
  };

static const char *
grub_pci_get_class (int class, int subclass)
{
  const struct grub_pci_classname *curr = grub_pci_classes;

  while (curr->desc)
    {
      if (curr->class == class && curr->subclass == subclass)
	return curr->desc;
      curr++;
    }

  return 0;
}

static int
grub_lspci_iter (int bus, int dev, int func, grub_pci_id_t pciid)
{
  grub_uint32_t class;
  const char *sclass;
  grub_pci_address_t addr;

  grub_printf ("%02x:%02x.%x %04x:%04x.%d", 0, dev, func, pciid >> 16, pciid & 0xFFFF, func);
  addr = grub_pci_make_address (bus, dev, func, 2);
  class = grub_pci_read (addr);
	  
  /* Lookup the class name, if there isn't a specific one,
     retry with 0x80 to get the generic class name.  */
  sclass = grub_pci_get_class (class >> 24, (class >> 16) & 0xFF);
  if (! sclass)
    sclass = grub_pci_get_class (class >> 24, 0x80);
  if (! sclass)
    sclass = "";

  grub_printf (" %s\n", sclass);

  return 0;
}

static grub_err_t
grub_cmd_lspci (struct grub_arg_list *state __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  grub_pci_iterate (grub_lspci_iter);
  return GRUB_ERR_NONE;
}




GRUB_MOD_INIT(pci)
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("lspci", grub_cmd_lspci, GRUB_COMMAND_FLAG_BOTH,
			 "lspci", "List PCI devices", 0);
}


GRUB_MOD_FINI(pci)
{
  grub_unregister_command ("lspci");
}
