/* boot.c - load and bootstrap a kernel */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1996  Erich Boleyn  <erich@uruk.org>
 *  Copyright (C) 1999, 2000, 2001  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "shared.h"

#include "freebsd.h"
#include "imgact_aout.h"
#include "i386-elf.h"

static int cur_addr;
entry_func entry_addr;
static struct mod_list mll[99];

unsigned long mb_header_flags;

/*
 *  The next two functions, 'load_image' and 'load_module', are the building
 *  blocks of the multiboot loader component.  They handle essentially all
 *  of the gory details of loading in a bootable image and the modules.
 */

kernel_t
load_image (char *kernel, char *arg, kernel_t suggested_type,
	    unsigned long load_flags)
{
  int len, i, exec_type = 0, align_4k = 1;
  entry_func real_entry_addr = 0;
  kernel_t type = KERNEL_TYPE_NONE;
  unsigned long text_len = 0, data_len = 0, bss_len = 0;
  char *str = 0, *str2 = 0;
  struct linux_kernel_header *lh;
  union
    {
      struct multiboot_header *mb;
      struct exec *aout;
      Elf32_Ehdr *elf;
    }
  pu;
  /* presuming that MULTIBOOT_SEARCH is large enough to encompass an
     executable header */
  unsigned char buffer[MULTIBOOT_SEARCH];

  if (!grub_open (kernel))
    return KERNEL_TYPE_NONE;

  if (!(len = grub_read (buffer, MULTIBOOT_SEARCH)) || len < 32)
    {
      grub_close ();
      
      if (!errnum)
	errnum = ERR_EXEC_FORMAT;

      return KERNEL_TYPE_NONE;
    }

  for (i = 0; i < len; i++)
    {
      if (MULTIBOOT_FOUND ((int) (buffer + i), len - i))
	{
         pu.mb = (struct multiboot_header *) (buffer + i);
         mb_header_flags = pu.mb->flags;
         if (mb_header_flags & MULTIBOOT_UNSUPPORTED)
	    {
	      grub_close ();
	      errnum = ERR_BOOT_FEATURES;
	      return KERNEL_TYPE_NONE;
	    }
	  type = KERNEL_TYPE_MULTIBOOT;
	  str2 = "Multiboot";
	  break;
	}
    }
  /* Handle graphics request for multiboot kernels */
  if (type == KERNEL_TYPE_MULTIBOOT &&
      mb_header_flags & MULTIBOOT_VIDEO_MODE &&
      mbi.flags & MB_INFO_VIDEO_INFO)
    {
      mbi.vbe_mode = 0x03;

      if (pu.mb->mode_type == 0)
       {
         unsigned short fallback = 0xFFFF;

         for (mode_list
                = (unsigned short *) VBE_FAR_PTR (vbe_info_block.video_mode);
              *mode_list != 0xFFFF;
              mode_list++)
           {
             if (get_vbe_mode_info (*mode_list, &mode_info_block) != 0x004F
                 || (mode_info_block.mode_attributes & 0x0091) != 0x0091)
               continue;

             if (fallback == 0xFFFF) fallback = *mode_list;

             if (pu.mb->width == mode_info_block.x_resolution &&
                 pu.mb->height == mode_info_block.y_resolution &&
                 pu.mb->depth == mode_info_block.bits_per_pixel )
               {
                 mbi.vbe_mode = *mode_list;
                 break;
               }
           }

         if (*mode_list == 0xFFFF && fallback != 0xFFFF)
           mbi.vbe_mode = fallback;

       }

      if (debug)
       {
         grub_printf ("%s mode requested: %dx%dx%d\n",
                      (pu.mb->mode_type == 0 ? "VBE graphics" : "Text"),
                      pu.mb->width, pu.mb->height, pu.mb->depth);
         grub_printf ("Mode selected: 0x%x\n", mbi.vbe_mode);
       }

    }

  /* sets the header pointer to point to the beginning of the
     buffer by default */
  pu.aout = (struct exec *) buffer;


  /* Use BUFFER as a linux kernel header, if the image is Linux zImage
     or bzImage.  */
  lh = (struct linux_kernel_header *) buffer;
  
  /* ELF loading supported if multiboot, FreeBSD and NetBSD.  */
  if ((type == KERNEL_TYPE_MULTIBOOT
       || grub_strcmp (pu.elf->e_ident + EI_BRAND, "FreeBSD") == 0
       || suggested_type == KERNEL_TYPE_NETBSD)
      && len > sizeof (Elf32_Ehdr)
      && BOOTABLE_I386_ELF ((*((Elf32_Ehdr *) buffer))))
    {
      if (type == KERNEL_TYPE_MULTIBOOT)
	entry_addr = (entry_func) pu.elf->e_entry;
      else
	entry_addr = (entry_func) (pu.elf->e_entry & 0xFFFFFF);

      if (entry_addr < (entry_func) 0x100000)
	errnum = ERR_BELOW_1MB;

      /* don't want to deal with ELF program header at some random
         place in the file -- this generally won't happen */
      if (pu.elf->e_phoff == 0 || pu.elf->e_phnum == 0
	  || ((pu.elf->e_phoff + (pu.elf->e_phentsize * pu.elf->e_phnum))
	      >= len))
	errnum = ERR_EXEC_FORMAT;
      str = "elf";

      if (type == KERNEL_TYPE_NONE)
	{
	  /* At the moment, there is no way to identify a NetBSD ELF
	     kernel, so rely on the suggested type by the user.  */
	  if (suggested_type == KERNEL_TYPE_NETBSD)
	    {
	      str2 = "NetBSD";
	      type = suggested_type;
	    }
	  else
	    {
	      str2 = "FreeBSD";
	      type = KERNEL_TYPE_FREEBSD;
	    }
	}
    }
  else if (mb_header_flags & MULTIBOOT_AOUT_KLUDGE)
    {
      pu.mb = (struct multiboot_header *) (buffer + i);
      entry_addr = (entry_func) pu.mb->entry_addr;
      cur_addr = pu.mb->load_addr;
      /* first offset into file */
      grub_seek (i - (pu.mb->header_addr - cur_addr));
      text_len = pu.mb->load_end_addr - cur_addr;
      data_len = 0;
      bss_len = pu.mb->bss_end_addr - pu.mb->load_end_addr;

      if (pu.mb->header_addr < pu.mb->load_addr
	  || pu.mb->load_end_addr <= pu.mb->load_addr
	  || pu.mb->bss_end_addr < pu.mb->load_end_addr
	  || (pu.mb->header_addr - pu.mb->load_addr) > i)
	errnum = ERR_EXEC_FORMAT;

      if (cur_addr < 0x100000)
	errnum = ERR_BELOW_1MB;

      pu.aout = (struct exec *) buffer;
      exec_type = 2;
      str = "kludge";
    }
  else if (len > sizeof (struct exec) && !N_BADMAG ((*(pu.aout))))
    {
      entry_addr = (entry_func) pu.aout->a_entry;

      if (type == KERNEL_TYPE_NONE)
	{
	  /*
	   *  If it doesn't have a Multiboot header, then presume
	   *  it is either a FreeBSD or NetBSD executable.  If so,
	   *  then use a magic number of normal ordering, ZMAGIC to
	   *  determine if it is FreeBSD.
	   *
	   *  This is all because freebsd and netbsd seem to require
	   *  masking out some address bits...  differently for each
	   *  one...  plus of course we need to know which booting
	   *  method to use.
	   */
	  entry_addr = (entry_func) ((int) entry_addr & 0xFFFFFF);
	  
	  if (buffer[0] == 0xb && buffer[1] == 1)
	    {
	      type = KERNEL_TYPE_FREEBSD;
	      cur_addr = (int) entry_addr;
	      str2 = "FreeBSD";
	    }
	  else
	    {
	      type = KERNEL_TYPE_NETBSD;
	      cur_addr = (int) entry_addr & 0xF00000;
	      if (N_GETMAGIC ((*(pu.aout))) != NMAGIC)
		align_4k = 0;
	      str2 = "NetBSD";
	    }
	}

      /* first offset into file */
      grub_seek (N_TXTOFF (*(pu.aout)));
      text_len = pu.aout->a_text;
      data_len = pu.aout->a_data;
      bss_len = pu.aout->a_bss;

      if (cur_addr < 0x100000)
	errnum = ERR_BELOW_1MB;

      exec_type = 1;
      str = "a.out";
    }
  else if (lh->boot_flag == BOOTSEC_SIGNATURE
	   && lh->setup_sects <= LINUX_MAX_SETUP_SECTS)
    {
      int big_linux = 0;
      int setup_sects = lh->setup_sects;

      if (lh->header == LINUX_MAGIC_SIGNATURE && lh->version >= 0x0200)
	{
	  big_linux = (lh->loadflags & LINUX_FLAG_BIG_KERNEL);
	  lh->type_of_loader = LINUX_BOOT_LOADER_TYPE;

	  if (lh->version >= 0x0201)
	    {
	      lh->heap_end_ptr = LINUX_HEAP_END_OFFSET;
	      lh->loadflags |= LINUX_FLAG_CAN_USE_HEAP;
	    }

	  if (lh->version >= 0x0202)
	    lh->cmd_line_ptr = CL_MY_LOCATION;
	  else
	    {
	      lh->cl_magic = CL_MAGIC;
	      lh->cl_offset = CL_MY_LOCATION - CL_BASE_ADDR;
	      lh->setup_move_size
		= (unsigned short) (CL_MY_END_ADDR - CL_BASE_ADDR + 1);
	    }
	}
      else
	{
	  /* Your kernel is quite old...  */
	  lh->cl_magic = CL_MAGIC;
	  lh->cl_offset = CL_MY_LOCATION - CL_BASE_ADDR;
	  
	  setup_sects = LINUX_DEFAULT_SETUP_SECTS;
	}
      
      /* If SETUP_SECTS is not set, set it to the default (4).  */
      if (! setup_sects)
	setup_sects = LINUX_DEFAULT_SETUP_SECTS;

      data_len = setup_sects << 9;
      text_len = filemax - data_len - SECTOR_SIZE;
      
      if (! big_linux && text_len > LINUX_KERNEL_MAXLEN)
	{
	  grub_printf (" linux 'zImage' kernel too big, try 'make bzImage'\n");
	  grub_close ();
	  errnum = ERR_WONT_FIT;
	  return KERNEL_TYPE_NONE;
	}

      grub_printf ("   [Linux-%s, setup=0x%x, size=0x%x]\n",
		   (big_linux ? "bzImage" : "zImage"), data_len, text_len);

      /* FIXME: SETUP_SECTS should be supported up to 63.
	 But do you know there are >640KB conventional memory machines?  */
      if (mbi.mem_lower >= 608 && setup_sects < 60)
	{
	  /* Video mode selection support. What a mess!  */
	  /* NOTE: Even the word "mess" is not still enough to
	     represent how wrong and bad the Linux video support is,
	     but I don't want to hear complaints from Linux fanatics
	     any more. -okuji  */
	  {
	    char *vga;

	    /* Find the substring "vga=".  */
	    vga = grub_strstr (arg, "vga=");
	    if (vga)
	      {
		char *value = vga + 4;
		int vid_mode;

		/* Handle special strings.  */
		if (substring ("normal", value) < 1)
		  vid_mode = LINUX_VID_MODE_NORMAL;
		else if (substring ("ext", value) < 1)
		  vid_mode = LINUX_VID_MODE_EXTENDED;
		else if (substring ("ask", value) < 1)
		  vid_mode = LINUX_VID_MODE_ASK;
		else if (safe_parse_maxint (&value, &vid_mode))
		  ;
		else
		  {
		    /* ERRNUM is already set inside the function
		       safe_parse_maxint.  */
		    grub_close ();
		    return KERNEL_TYPE_NONE;
		  }

		lh->vid_mode = vid_mode;
	      }
	  }
		
	  memmove ((char *) LINUX_SETUP, buffer, data_len + SECTOR_SIZE);

	  if (lh->header != LINUX_MAGIC_SIGNATURE ||
	      lh->version < 0x0200)
	    /* Clear the heap space.  */
	    grub_memset ((char *) LINUX_SETUP + ((setup_sects - 1) << 9),
			 0,
			 (64 - setup_sects - 1) << 9);

	  /* Copy command-line plus memory hack to staging area.
	     NOTE: Linux has a bug that it doesn't handle multiple spaces
	     between two options and a space after a "mem=" option isn't
	     removed correctly so the arguments to init could be like
	     {"init", "", "", NULL}. This affects some not-very-clever
	     shells. Thus, the code below does a trick to avoid the bug.
	     That is, copy "mem=XXX" to the end of the command-line, and
	     avoid to copy spaces unnecessarily. Hell.  */
	  {
	    char *src = skip_to (0, arg);
	    char *dest = (char *) CL_MY_LOCATION;

	    while (((int) dest) < CL_MY_END_ADDR && *src)
	      *(dest++) = *(src++);
	    
	    /* Add a mem option automatically only if the user doesn't
	       specify it explicitly.  */
	    if (! grub_strstr (arg, "mem=")
		&& ! (load_flags & KERNEL_LOAD_NO_MEM_OPTION))
	      {
		if (dest != (char *) CL_MY_LOCATION)
		  *(dest++) = ' ';
		
		grub_memmove (dest, "mem=", 4);
		dest += 4;
		
		dest = convert_to_ascii (dest, 'u', (extended_memory + 0x400));
		*(dest++) = 'K';
	      }

	    *dest = 0;
	  }

	  /* offset into file */
	  grub_seek (data_len + SECTOR_SIZE);

	  cur_addr = LINUX_STAGING_AREA + text_len;
	  if (grub_read ((char *) LINUX_STAGING_AREA, text_len)
	      >= (text_len - 16))
	    {
	      grub_close ();

	      /* Sanity check.  */
	      if (suggested_type != KERNEL_TYPE_NONE
		  && ((big_linux && suggested_type != KERNEL_TYPE_BIG_LINUX)
		      || (! big_linux && suggested_type != KERNEL_TYPE_LINUX)))
		{
		  errnum = ERR_EXEC_FORMAT;
		  return KERNEL_TYPE_NONE;
		}

	      /* Ugly hack.  */
	      linux_text_len = text_len;
	      
	      return big_linux ? KERNEL_TYPE_BIG_LINUX : KERNEL_TYPE_LINUX;
	    }
	  else if (! errnum)
	    errnum = ERR_EXEC_FORMAT;
	}
      else
	errnum = ERR_WONT_FIT;
    }
  else				/* no recognizable format */
    errnum = ERR_EXEC_FORMAT;

  /* return if error */
  if (errnum)
    {
      grub_close ();
      return KERNEL_TYPE_NONE;
    }

  /* fill the multiboot info structure */
  mbi.cmdline = (int) arg;
  mbi.mods_count = 0;
  mbi.mods_addr = 0;
  mbi.boot_device = (current_drive << 24) | current_partition;
  mbi.flags &= ~(MB_INFO_MODS | MB_INFO_AOUT_SYMS | MB_INFO_ELF_SHDR);
  mbi.syms.a.tabsize = 0;
  mbi.syms.a.strsize = 0;
  mbi.syms.a.addr = 0;
  mbi.syms.a.pad = 0;

  printf ("   [%s-%s", str2, str);

  str = "";

  if (exec_type)		/* can be loaded like a.out */
    {
      if (mb_header_flags & MULTIBOOT_AOUT_KLUDGE)
	str = "-and-data";

      printf (", loadaddr=0x%x, text%s=0x%x", cur_addr, str, text_len);

      /* read text, then read data */
      if (grub_read ((char *) RAW_ADDR (cur_addr), text_len) == text_len)
	{
	  cur_addr += text_len;

	  if (!(mb_header_flags & MULTIBOOT_AOUT_KLUDGE))
	    {
	      /* we have to align to a 4K boundary */
	      if (align_4k)
		cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;
	      else
		printf (", C");

	      printf (", data=0x%x", data_len);

	      if ((grub_read ((char *) RAW_ADDR (cur_addr), data_len)
		   != data_len)
		  && !errnum)
		errnum = ERR_EXEC_FORMAT;
	      cur_addr += data_len;
	    }

	  if (!errnum)
	    {
	      memset ((char *) RAW_ADDR (cur_addr), 0, bss_len);
	      cur_addr += bss_len;

	      printf (", bss=0x%x", bss_len);
	    }
	}
      else if (!errnum)
	errnum = ERR_EXEC_FORMAT;

      if (!errnum && pu.aout->a_syms
	  && pu.aout->a_syms < (filemax - filepos))
	{
	  int symtab_err, orig_addr = cur_addr;

	  /* we should align to a 4K boundary here for good measure */
	  if (align_4k)
	    cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

	  mbi.syms.a.addr = cur_addr;

	  *((int *) RAW_ADDR (cur_addr)) = pu.aout->a_syms;
	  cur_addr += sizeof (int);
	  
	  printf (", symtab=0x%x", pu.aout->a_syms);

	  if (grub_read ((char *) RAW_ADDR (cur_addr), pu.aout->a_syms)
	      == pu.aout->a_syms)
	    {
	      cur_addr += pu.aout->a_syms;
	      mbi.syms.a.tabsize = pu.aout->a_syms;

	      if (grub_read ((char *) &i, sizeof (int)) == sizeof (int))
		{
		  *((int *) RAW_ADDR (cur_addr)) = i;
		  cur_addr += sizeof (int);

		  mbi.syms.a.strsize = i;

		  i -= sizeof (int);

		  printf (", strtab=0x%x", i);

		  symtab_err = (grub_read ((char *) RAW_ADDR (cur_addr), i)
				!= i);
		  cur_addr += i;
		}
	      else
		symtab_err = 1;
	    }
	  else
	    symtab_err = 1;

	  if (symtab_err)
	    {
	      printf ("(bad)");
	      cur_addr = orig_addr;
	      mbi.syms.a.tabsize = 0;
	      mbi.syms.a.strsize = 0;
	      mbi.syms.a.addr = 0;
	    }
	  else
	    mbi.flags |= MB_INFO_AOUT_SYMS;
	}
    }
  else
    /* ELF executable */
    {
      unsigned loaded = 0, memaddr, memsiz, filesiz;
      Elf32_Phdr *phdr;

      /* reset this to zero for now */
      cur_addr = 0;

      /* scan for program segments */
      for (i = 0; i < pu.elf->e_phnum; i++)
	{
	  phdr = (Elf32_Phdr *)
	    (pu.elf->e_phoff + ((int) buffer)
	     + (pu.elf->e_phentsize * i));
	  if (phdr->p_type == PT_LOAD)
	    {
	      /* offset into file */
	      grub_seek (phdr->p_offset);
	      filesiz = phdr->p_filesz;
	      
	      if (type == KERNEL_TYPE_FREEBSD || type == KERNEL_TYPE_NETBSD)
		memaddr = RAW_ADDR (phdr->p_paddr & 0xFFFFFF);
	      else
		memaddr = RAW_ADDR (phdr->p_paddr);
	      
	      memsiz = phdr->p_memsz;
	      if (memaddr < RAW_ADDR (0x100000))
		errnum = ERR_BELOW_1MB;

	      /* If the memory range contains the entry address, get the
		 physical address here.  */
	      if (type == KERNEL_TYPE_MULTIBOOT
		  && (unsigned) entry_addr >= phdr->p_vaddr
		  && (unsigned) entry_addr < phdr->p_vaddr + memsiz)
		real_entry_addr = (entry_func) ((unsigned) entry_addr
						+ memaddr - phdr->p_vaddr);
		
	      /* make sure we only load what we're supposed to! */
	      if (filesiz > memsiz)
		filesiz = memsiz;
	      /* mark memory as used */
	      if (cur_addr < memaddr + memsiz)
		cur_addr = memaddr + memsiz;
	      printf (", <0x%x:0x%x:0x%x>", memaddr, filesiz,
		      memsiz - filesiz);
	      /* increment number of segments */
	      loaded++;

	      /* load the segment */
	      if (memcheck (memaddr, memsiz)
		  && grub_read ((char *) memaddr, filesiz) == filesiz)
		{
		  if (memsiz > filesiz)
		    memset ((char *) (memaddr + filesiz), 0, memsiz - filesiz);
		}
	      else
		break;
	    }
	}

      if (! errnum)
	{
	  if (! loaded)
	    errnum = ERR_EXEC_FORMAT;
	  else
	    {
	      /* Load ELF symbols.  */
	      Elf32_Shdr *shdr = NULL;
	      int tab_size, sec_size;
	      int symtab_err = 0;

	      mbi.syms.e.num = pu.elf->e_shnum;
	      mbi.syms.e.size = pu.elf->e_shentsize;
	      mbi.syms.e.shndx = pu.elf->e_shstrndx;
	      
	      /* We should align to a 4K boundary here for good measure.  */
	      if (align_4k)
		cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;
	      
	      tab_size = pu.elf->e_shentsize * pu.elf->e_shnum;
	      
	      grub_seek (pu.elf->e_shoff);
	      if (grub_read ((char *) RAW_ADDR (cur_addr), tab_size)
		  == tab_size)
		{
		  mbi.syms.e.addr = cur_addr;
		  shdr = (Elf32_Shdr *) mbi.syms.e.addr;
		  cur_addr += tab_size;
		  
		  printf (", shtab=0x%x", cur_addr);
  		  
		  for (i = 0; i < mbi.syms.e.num; i++)
		    {
		      /* This section is a loaded section,
			 so we don't care.  */
		      if (shdr[i].sh_addr != 0)
			continue;
		      
		      /* This section is empty, so we don't care.  */
		      if (shdr[i].sh_size == 0)
			continue;
		      
		      /* Align the section to a sh_addralign bits boundary.  */
		      cur_addr = ((cur_addr + shdr[i].sh_addralign) & 
				  - (int) shdr[i].sh_addralign);
		      
		      grub_seek (shdr[i].sh_offset);
		      
		      sec_size = shdr[i].sh_size;

		      if (! (memcheck (cur_addr, sec_size)
			     && (grub_read ((char *) RAW_ADDR (cur_addr),
					    sec_size)
				 == sec_size)))
			{
			  symtab_err = 1;
			  break;
			}
		      
		      shdr[i].sh_addr = cur_addr;
		      cur_addr += sec_size;
		    }
		}
	      else 
		symtab_err = 1;
	      
	      if (mbi.syms.e.addr < RAW_ADDR(0x10000))
		symtab_err = 1;
	      
	      if (symtab_err) 
		{
		  printf ("(bad)");
		  mbi.syms.e.num = 0;
		  mbi.syms.e.size = 0;
		  mbi.syms.e.addr = 0;
		  mbi.syms.e.shndx = 0;
		  cur_addr = 0;
		}
	      else
		mbi.flags |= MB_INFO_ELF_SHDR;
	    }
	}
    }

  if (! errnum)
    {
      grub_printf (", entry=0x%x]\n", (unsigned) entry_addr);
      
      /* If the entry address is physically different from that of the ELF
	 header, correct it here.  */
      if (real_entry_addr)
	entry_addr = real_entry_addr;
    }
  else
    {
      putchar ('\n');
      type = KERNEL_TYPE_NONE;
    }

  grub_close ();

  /* Sanity check.  */
  if (suggested_type != KERNEL_TYPE_NONE && suggested_type != type)
    {
      errnum = ERR_EXEC_FORMAT;
      return KERNEL_TYPE_NONE;
    }
  
  return type;
}

int
load_module (char *module, char *arg)
{
  int len;

  /* if we are supposed to load on 4K boundaries */
  cur_addr = (cur_addr + 0xFFF) & 0xFFFFF000;

  if (!grub_open (module))
    return 0;

  len = grub_read ((char *) cur_addr, -1);
  if (! len)
    {
      grub_close ();
      return 0;
    }

  printf ("   [Multiboot-module @ 0x%x, 0x%x bytes]\n", cur_addr, len);

  /* these two simply need to be set if any modules are loaded at all */
  mbi.flags |= MB_INFO_MODS;
  mbi.mods_addr = (int) mll;

  mll[mbi.mods_count].cmdline = (int) arg;
  mll[mbi.mods_count].mod_start = cur_addr;
  cur_addr += len;
  mll[mbi.mods_count].mod_end = cur_addr;
  mll[mbi.mods_count].pad = 0;

  /* increment number of modules included */
  mbi.mods_count++;

  grub_close ();
  return 1;
}

int
load_initrd (char *initrd)
{
  int len;
  unsigned long moveto;
  struct linux_kernel_header *lh;
  
#ifndef NO_DECOMPRESSION
  no_decompression = 1;
#endif
  
  if (! grub_open (initrd))
    goto fail;

  len = grub_read ((char *) cur_addr, -1);
  if (! len)
    {
      grub_close ();
      goto fail;
    }

  moveto = ((mbi.mem_upper + 0x400) * 0x400 - len) & 0xfffff000;
  if (moveto + len >= LINUX_INITRD_MAX_ADDRESS)
    moveto = (LINUX_INITRD_MAX_ADDRESS - len) & 0xfffff000;
  
  /* XXX: Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     XXX: Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb. *sigh*  */
  moveto -= 0x10000;
  memmove ((void *) RAW_ADDR (moveto), (void *) cur_addr, len);

  printf ("   [Linux-initrd @ 0x%x, 0x%x bytes]\n", moveto, len);

  /* FIXME: Should check if the kernel supports INITRD.  */
  lh = (struct linux_kernel_header *) LINUX_SETUP;
  lh->ramdisk_image = RAW_ADDR (moveto);
  lh->ramdisk_size = len;

  grub_close ();

 fail:
  
#ifndef NO_DECOMPRESSION
  no_decompression = 0;
#endif

  return ! errnum;
}


#ifdef GRUB_UTIL
/* Dummy function to fake the *BSD boot.  */
static void
bsd_boot_entry (int flags, int bootdev, int sym_start, int sym_end,
		int mem_upper, int mem_lower)
{
  stop ();
}
#endif


/*
 *  All "*_boot" commands depend on the images being loaded into memory
 *  correctly, the variables in this file being set up correctly, and
 *  the root partition being set in the 'saved_drive' and 'saved_partition'
 *  variables.
 */


void
bsd_boot (kernel_t type, int bootdev, char *arg)
{
  char *str;
  int clval = 0, i;
  struct bootinfo bi;

#ifdef GRUB_UTIL
  entry_addr = (entry_func) bsd_boot_entry;
#else
  stop_floppy ();
#endif

  while (*(++arg) && *arg != ' ');
  str = arg;
  while (*str)
    {
      if (*str == '-')
	{
	  while (*str && *str != ' ')
	    {
	      if (*str == 'C')
		clval |= RB_CDROM;
	      if (*str == 'a')
		clval |= RB_ASKNAME;
	      if (*str == 'b')
		clval |= RB_HALT;
	      if (*str == 'c')
		clval |= RB_CONFIG;
	      if (*str == 'd')
		clval |= RB_KDB;
	      if (*str == 'h')
		clval |= RB_SERIAL;
	      if (*str == 'r')
		clval |= RB_DFLTROOT;
	      if (*str == 's')
		clval |= RB_SINGLE;
	      if (*str == 'v')
		clval |= RB_VERBOSE;
	      str++;
	    }
	  continue;
	}
      str++;
    }

  if (type == KERNEL_TYPE_FREEBSD)
    {
      clval |= RB_BOOTINFO;

      bi.bi_version = BOOTINFO_VERSION;

      *arg = 0;
      while ((--arg) > (char *) MB_CMDLINE_BUF && *arg != '/');
      if (*arg == '/')
	bi.bi_kernelname = arg + 1;
      else
	bi.bi_kernelname = 0;

      bi.bi_nfs_diskless = 0;
      bi.bi_n_bios_used = 0;	/* this field is apparently unused */

      for (i = 0; i < N_BIOS_GEOM; i++)
	{
	  struct geometry geom;

	  /* XXX Should check the return value.  */
	  get_diskinfo (i + 0x80, &geom);
	  /* FIXME: If HEADS or SECTORS is greater than 255, then this will
	     break the geometry information. That is a drawback of BSD
	     but not of GRUB.  */
	  bi.bi_bios_geom[i] = (((geom.cylinders - 1) << 16)
				+ (((geom.heads - 1) & 0xff) << 8)
				+ (geom.sectors & 0xff));
	}

      bi.bi_size = sizeof (struct bootinfo);
      bi.bi_memsizes_valid = 1;
      bi.bi_bios_dev = saved_drive;
      bi.bi_basemem = mbi.mem_lower;
      bi.bi_extmem = extended_memory;

      if (mbi.flags & MB_INFO_AOUT_SYMS)
	{
	  bi.bi_symtab = mbi.syms.a.addr;
	  bi.bi_esymtab = mbi.syms.a.addr + 4
	    + mbi.syms.a.tabsize + mbi.syms.a.strsize;
	}
#if 0
      else if (mbi.flags & MB_INFO_ELF_SHDR)
	{
	  /* FIXME: Should check if a symbol table exists and, if exists,
	     pass the table to BI.  */
	}
#endif
      else
	{
	  bi.bi_symtab = 0;
	  bi.bi_esymtab = 0;
	}

      /* call entry point */
      (*entry_addr) (clval, bootdev, 0, 0, 0, ((int) (&bi)));
    }
  else
    {
      /*
       *  We now pass the various bootstrap parameters to the loaded
       *  image via the argument list.
       *
       *  This is the official list:
       *
       *  arg0 = 8 (magic)
       *  arg1 = boot flags
       *  arg2 = boot device
       *  arg3 = start of symbol table (0 if not loaded)
       *  arg4 = end of symbol table (0 if not loaded)
       *  arg5 = transfer address from image
       *  arg6 = transfer address for next image pointer
       *  arg7 = conventional memory size (640)
       *  arg8 = extended memory size (8196)
       *
       *  ...in actuality, we just pass the parameters used by the kernel.
       */

      /* call entry point */
      unsigned long end_mark;

      if (mbi.flags & MB_INFO_AOUT_SYMS)
	end_mark = (mbi.syms.a.addr + 4
		    + mbi.syms.a.tabsize + mbi.syms.a.strsize);
      else
	/* FIXME: it should be mbi.syms.e.size.  */
	end_mark = 0;
      
      (*entry_addr) (clval, bootdev, 0, end_mark,
		     extended_memory, mbi.mem_lower);
    }
}
