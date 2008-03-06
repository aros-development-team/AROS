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

#include <grub/loader.h>
#include <grub/cpu/loader.h>
#include <grub/cpu/bsd.h>
#include <grub/machine/init.h>
#include <grub/machine/memory.h>
#include <grub/file.h>
#include <grub/err.h>
#include <grub/rescue.h>
#include <grub/dl.h>
#include <grub/mm.h>
#include <grub/elfload.h>
#include <grub/env.h>
#include <grub/misc.h>
#include <grub/gzio.h>
#include <grub/aout.h>

#define ALIGN_DWORD(a)	ALIGN_UP (a, 4)
#define ALIGN_PAGE(a)	ALIGN_UP (a, 4096)

#define MOD_BUF_ALLOC_UNIT	4096

static int kernel_type;
static grub_dl_t my_mod;
static grub_addr_t entry, kern_start, kern_end;
static grub_uint32_t bootflags;
static char *mod_buf;
static grub_uint32_t mod_buf_len, mod_buf_max;
static int is_elf_kernel;

static const char freebsd_opts[] = "DhaCcdgmnpqrsv";
static const grub_uint32_t freebsd_flags[] =
{
  FREEBSD_RB_DUAL, FREEBSD_RB_SERIAL, FREEBSD_RB_ASKNAME,
  FREEBSD_RB_CDROM, FREEBSD_RB_CONFIG, FREEBSD_RB_KDB,
  FREEBSD_RB_GDB, FREEBSD_RB_MUTE, FREEBSD_RB_NOINTR,
  FREEBSD_RB_PAUSE, FREEBSD_RB_QUIET, FREEBSD_RB_DFLTROOT,
  FREEBSD_RB_SINGLE, FREEBSD_RB_VERBOSE
};

static const char openbsd_opts[] = "abcsd";
static const grub_uint32_t openbsd_flags[] =
{
  OPENBSD_RB_ASKNAME, OPENBSD_RB_HALT, OPENBSD_RB_CONFIG,
  OPENBSD_RB_SINGLE, OPENBSD_RB_KDB
};

static const char netbsd_opts[] = "abcdmqsvxz";
static const grub_uint32_t netbsd_flags[] =
{
  NETBSD_RB_ASKNAME, NETBSD_RB_HALT, NETBSD_RB_USERCONFIG,
  NETBSD_RB_KDB, NETBSD_RB_MINIROOT, NETBSD_AB_QUIET,
  NETBSD_RB_SINGLE, NETBSD_AB_VERBOSE, NETBSD_AB_DEBUG,
  NETBSD_AB_SILENT
};

static void
grub_bsd_get_device (grub_uint32_t * biosdev,
		     grub_uint32_t * unit,
		     grub_uint32_t * slice, grub_uint32_t * part)
{
  char *p;

  *biosdev = *unit = *slice = *part = 0;
  p = grub_env_get ("root");
  if ((p) && ((p[0] == 'h') || (p[0] == 'f')) && (p[1] == 'd') &&
      (p[2] >= '0') && (p[2] <= '9'))
    {
      if (p[0] == 'h')
	*biosdev = 0x80;

      *unit = grub_strtoul (p + 2, &p, 0);
      *biosdev += *unit;

      if ((p) && (p[0] == ','))
	{
	  if ((p[1] >= '0') && (p[1] <= '9'))
	    {
	      *slice = grub_strtoul (p + 1, &p, 0);

	      if ((p) && (p[0] == ','))
		p++;
	    }

	  if ((p[0] >= 'a') && (p[0] <= 'z'))
	    *part = p[0] - 'a';
	}
    }
}

static grub_err_t
grub_freebsd_add_meta (grub_uint32_t type, void *data, grub_uint32_t len)
{
  if (mod_buf_max < mod_buf_len + len + 8)
    {
      char *new_buf;

      do
	{
	  mod_buf_max += MOD_BUF_ALLOC_UNIT;
	}
      while (mod_buf_max < mod_buf_len + len + 8);

      new_buf = grub_malloc (mod_buf_max);
      if (!new_buf)
	return grub_errno;

      grub_memcpy (new_buf, mod_buf, mod_buf_len);
      grub_free (mod_buf);

      mod_buf = new_buf;
    }

  *((grub_uint32_t *) (mod_buf + mod_buf_len)) = type;
  *((grub_uint32_t *) (mod_buf + mod_buf_len + 4)) = len;
  mod_buf_len += 8;

  if (len)
    grub_memcpy (mod_buf + mod_buf_len, data, len);

  mod_buf_len = ALIGN_DWORD (mod_buf_len + len);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_freebsd_add_meta_module (int is_kern, int argc, char **argv,
			      grub_addr_t addr, grub_uint32_t size)
{
  char *name, *type;

  name = grub_strrchr (argv[0], '/');
  if (name)
    name++;
  else
    name = argv[0];

  if (grub_freebsd_add_meta (FREEBSD_MODINFO_NAME, name,
			     grub_strlen (name) + 1))
    return grub_errno;

  argc--;
  argv++;

  if ((argc) && (!grub_memcmp (argv[0], "type=", 5)))
    {
      type = &argv[0][5];
      argc--;
      argv++;
    }
  else
    type = (is_kern) ? FREEBSD_MODTYPE_KERNEL : FREEBSD_MODTYPE_RAW;

  if ((grub_freebsd_add_meta (FREEBSD_MODINFO_TYPE, type,
			      grub_strlen (type) + 1)) ||
      (grub_freebsd_add_meta (FREEBSD_MODINFO_ADDR, &addr, sizeof (addr))) ||
      (grub_freebsd_add_meta (FREEBSD_MODINFO_SIZE, &size, sizeof (size))))
    return grub_errno;

  if (argc)
    {
      int i, n;

      n = 0;
      for (i = 0; i < argc; i++)
	{
	  n += grub_strlen (argv[i]) + 1;
	}

      if (n)
	{
	  char cmdline[n], *p;

	  p = cmdline;
	  for (i = 0; i < argc; i++)
	    {
	      grub_strcpy (p, argv[i]);
	      p += grub_strlen (argv[i]);
	      *(p++) = ' ';
	    }
	  *p = 0;

	  if (grub_freebsd_add_meta (FREEBSD_MODINFO_ARGS, cmdline, n))
	    return grub_errno;
	}
    }

  return GRUB_ERR_NONE;
}

static void
grub_freebsd_list_modules (void)
{
  grub_uint32_t pos = 0;

  grub_printf ("  %-18s  %-18s%14s%14s\n", "name", "type", "addr", "size");
  while (pos < mod_buf_len)
    {
      grub_uint32_t type, size;

      type = *((grub_uint32_t *) (mod_buf + pos));
      size = *((grub_uint32_t *) (mod_buf + pos + 4));
      pos += 8;
      switch (type)
	{
	case FREEBSD_MODINFO_NAME:
	case FREEBSD_MODINFO_TYPE:
	  grub_printf ("  %-18s", mod_buf + pos);
	  break;
	case FREEBSD_MODINFO_ADDR:
	  {
	    grub_addr_t addr;

	    addr = *((grub_addr_t *) (mod_buf + pos));
	    grub_printf ("    0x%08x", addr);
	    break;
	  }
	case FREEBSD_MODINFO_SIZE:
	  {
	    grub_uint32_t len;

	    len = *((grub_uint32_t *) (mod_buf + pos));
	    grub_printf ("    0x%08x\n", len);
	  }
	}

      pos = ALIGN_DWORD (pos + size);
    }
}

static grub_err_t
grub_freebsd_boot (void)
{
  struct grub_freebsd_bootinfo bi;
  char *p;
  grub_uint32_t bootdev, biosdev, unit, slice, part;

  auto int iterate_env (struct grub_env_var *var);
  int iterate_env (struct grub_env_var *var)
  {
    if ((!grub_memcmp (var->name, "FreeBSD.", 8)) && (var->name[8]))
      {
	grub_strcpy (p, &var->name[8]);
	p += grub_strlen (p);
	*(p++) = '=';
	grub_strcpy (p, var->value);
	p += grub_strlen (p) + 1;
      }

    return 0;
  }

  grub_memset (&bi, 0, sizeof (bi));
  bi.bi_version = FREEBSD_BOOTINFO_VERSION;
  bi.bi_size = sizeof (bi);

  grub_bsd_get_device (&biosdev, &unit, &slice, &part);
  bootdev = (FREEBSD_B_DEVMAGIC + ((slice + 1) << FREEBSD_B_SLICESHIFT) +
	     (unit << FREEBSD_B_UNITSHIFT) + (part << FREEBSD_B_PARTSHIFT));

  bi.bi_bios_dev = biosdev;

  p = (char *) kern_end;

  grub_env_iterate (iterate_env);

  if (p != (char *) kern_end)
    {
      *(p++) = 0;

      bi.bi_envp = kern_end;
      kern_end = ALIGN_PAGE ((grub_uint32_t) p);
    }

  if (is_elf_kernel)
    {
      if (grub_freebsd_add_meta (FREEBSD_MODINFO_END, 0, 0))
	return grub_errno;

      grub_memcpy ((char *) kern_end, mod_buf, mod_buf_len);
      bi.bi_modulep = kern_end;

      kern_end = ALIGN_PAGE (kern_end + mod_buf_len);
    }

  bi.bi_kernend = kern_end;

  grub_unix_real_boot (entry, bootflags | FREEBSD_RB_BOOTINFO, bootdev,
		       0, 0, 0, &bi, bi.bi_modulep, kern_end);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_openbsd_boot (void)
{
  char *buf = (char *) GRUB_BSD_TEMP_BUFFER;
  struct grub_machine_mmap_entry mmap;
  struct grub_openbsd_bios_mmap *pm;
  struct grub_openbsd_bootargs *pa;
  grub_uint32_t bootdev, biosdev, unit, slice, part, cont;

  pa = (struct grub_openbsd_bootargs *) buf;

  pa->ba_type = OPENBSD_BOOTARG_MMAP;
  pm = (struct grub_openbsd_bios_mmap *) (pa + 1);
  cont = grub_get_mmap_entry (&mmap, 0);
  if (mmap.size)
    do
      {
	pm->addr = mmap.addr;
	pm->len = mmap.len;
	pm->type = mmap.type;
	pm++;

	if (!cont)
	  break;

	cont = grub_get_mmap_entry (&mmap, cont);
      }
    while (mmap.size);

  pa->ba_size = (char *) pm - (char *) pa;
  pa->ba_next = (struct grub_openbsd_bootargs *) pm;
  pa = pa->ba_next;
  pa->ba_type = OPENBSD_BOOTARG_END;
  pa++;

  grub_bsd_get_device (&biosdev, &unit, &slice, &part);
  bootdev = (OPENBSD_B_DEVMAGIC + (unit << OPENBSD_B_UNITSHIFT) +
	     (part << OPENBSD_B_PARTSHIFT));

  grub_unix_real_boot (entry, bootflags, bootdev, OPENBSD_BOOTARG_APIVER,
		       0, grub_upper_mem >> 10, grub_lower_mem >> 10,
		       (char *) pa - buf, buf);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_netbsd_boot (void)
{
  struct grub_netbsd_btinfo_rootdevice *rootdev;
  struct grub_netbsd_bootinfo *bootinfo;
  grub_uint32_t biosdev, unit, slice, part;

  grub_bsd_get_device (&biosdev, &unit, &slice, &part);

  rootdev = (struct grub_netbsd_btinfo_rootdevice *) GRUB_BSD_TEMP_BUFFER;

  rootdev->common.len = sizeof (struct grub_netbsd_btinfo_rootdevice);
  rootdev->common.type = NETBSD_BTINFO_ROOTDEVICE;
  grub_sprintf (rootdev->devname, "%cd%d%c", (biosdev & 0x80) ? 'w' : 'f',
		unit, 'a' + part);

  bootinfo = (struct grub_netbsd_bootinfo *) (rootdev + 1);
  bootinfo->bi_count = 1;
  bootinfo->bi_data[0] = rootdev;

  grub_unix_real_boot (entry, bootflags, 0, bootinfo,
		       0, grub_upper_mem >> 10, grub_lower_mem >> 10);

  /* Not reached.  */
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_bsd_unload (void)
{
  if (mod_buf)
    {
      grub_free (mod_buf);
      mod_buf = 0;
      mod_buf_max = 0;
    }

  kernel_type = KERNEL_TYPE_NONE;
  grub_dl_unref (my_mod);

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_bsd_load_aout (grub_file_t file)
{
  grub_addr_t load_addr, bss_end_addr;
  int ofs, align_page;
  union grub_aout_header ah;

  if ((grub_file_seek (file, 0)) == (grub_off_t) - 1)
    return grub_errno;

  if (grub_file_read (file, (char *) &ah, sizeof (ah)) != sizeof (ah))
    return grub_error (GRUB_ERR_READ_ERROR, "cannot read the a.out header");

  if (grub_aout_get_type (&ah) != AOUT_TYPE_AOUT32)
    return grub_error (GRUB_ERR_BAD_OS, "invalid a.out header");

  entry = ah.aout32.a_entry & 0xFFFFFF;

  if (AOUT_GETMAGIC (ah.aout32) == AOUT32_ZMAGIC)
    {
      load_addr = entry;
      ofs = 0x1000;
      align_page = 0;
    }
  else
    {
      load_addr = entry & 0xF00000;
      ofs = sizeof (struct grub_aout32_header);
      align_page = 1;
    }

  if (load_addr < 0x100000)
    return grub_error (GRUB_ERR_BAD_OS, "load address below 1M");

  kern_start = load_addr;
  kern_end = load_addr + ah.aout32.a_text + ah.aout32.a_data;
  if (align_page)
    kern_end = ALIGN_PAGE (kern_end);

  if (ah.aout32.a_bss)
    {
      kern_end += ah.aout32.a_bss;
      if (align_page)
	kern_end = ALIGN_PAGE (kern_end);

      bss_end_addr = kern_end;
    }
  else
    bss_end_addr = 0;

  return grub_aout_load (file, ofs, load_addr,
			 ah.aout32.a_text + ah.aout32.a_data, bss_end_addr);
}

static grub_err_t
grub_bsd_elf32_hook (Elf32_Phdr * phdr, UNUSED grub_addr_t * addr)
{
  Elf32_Addr paddr;

  phdr->p_paddr &= 0xFFFFFF;
  paddr = phdr->p_paddr;

  if ((paddr < grub_os_area_addr)
      || (paddr + phdr->p_memsz > grub_os_area_addr + grub_os_area_size))
    return grub_error (GRUB_ERR_OUT_OF_RANGE, "Address 0x%x is out of range",
		       paddr);

  if ((!kern_start) || (paddr < kern_start))
    kern_start = paddr;

  if (paddr + phdr->p_memsz > kern_end)
    kern_end = paddr + phdr->p_memsz;

  return GRUB_ERR_NONE;
}

static grub_err_t
grub_bsd_load_elf (grub_elf_t elf)
{
  kern_start = kern_end = 0;

  if (grub_elf_is_elf32 (elf))
    {
      entry = elf->ehdr.ehdr32.e_entry & 0xFFFFFF;
      return grub_elf32_load (elf, grub_bsd_elf32_hook, 0, 0);
    }
  else
    return grub_error (GRUB_ERR_BAD_OS, "invalid elf");
}

static grub_err_t
grub_bsd_load (int argc, char *argv[])
{
  grub_file_t file;
  grub_elf_t elf;

  grub_dl_ref (my_mod);

  grub_loader_unset ();

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no kernel specified");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if (!file)
    goto fail;

  elf = grub_elf_file (file);
  if (elf)
    {
      is_elf_kernel = 1;
      grub_bsd_load_elf (elf);
      grub_elf_close (elf);
    }
  else
    {
      is_elf_kernel = 0;
      grub_errno = 0;
      grub_bsd_load_aout (file);
      grub_file_close (file);
    }

fail:

  if (grub_errno != GRUB_ERR_NONE)
    grub_dl_unref (my_mod);

  return grub_errno;
}

static grub_uint32_t
grub_bsd_parse_flags (char *str, const char *opts,
		      const grub_uint32_t * flags)
{
  grub_uint32_t result = 0;

  while (*str)
    {
      const char *po;
      const grub_uint32_t *pf;

      po = opts;
      pf = flags;
      while (*po)
	{
	  if (*str == *po)
	    {
	      result |= *pf;
	      break;
	    }
	  po++;
	  pf++;
	}
      str++;
    }

  return result;
}

void
grub_rescue_cmd_freebsd (int argc, char *argv[])
{
  kernel_type = KERNEL_TYPE_FREEBSD;
  bootflags = ((argc <= 1) ? 0 :
	       grub_bsd_parse_flags (argv[1], freebsd_opts, freebsd_flags));

  if (grub_bsd_load (argc, argv) == GRUB_ERR_NONE)
    {
      kern_end = ALIGN_PAGE (kern_end);
      if ((is_elf_kernel) &&
	  (grub_freebsd_add_meta_module (1, argc, argv, kern_start,
					 kern_end - kern_start)))
	return;
      grub_loader_set (grub_freebsd_boot, grub_bsd_unload, 1);
    }
}

void
grub_rescue_cmd_openbsd (int argc, char *argv[])
{
  kernel_type = KERNEL_TYPE_OPENBSD;
  bootflags = ((argc <= 1) ? 0 :
	       grub_bsd_parse_flags (argv[1], openbsd_opts, openbsd_flags));

  if (grub_bsd_load (argc, argv) == GRUB_ERR_NONE)
    grub_loader_set (grub_openbsd_boot, grub_bsd_unload, 1);
}

void
grub_rescue_cmd_netbsd (int argc, char *argv[])
{
  kernel_type = KERNEL_TYPE_NETBSD;
  bootflags = ((argc <= 1) ? 0 :
	       grub_bsd_parse_flags (argv[1], netbsd_opts, netbsd_flags));

  if (grub_bsd_load (argc, argv) == GRUB_ERR_NONE)
    grub_loader_set (grub_netbsd_boot, grub_bsd_unload, 1);
}

void
grub_rescue_cmd_freebsd_loadenv (int argc, char *argv[])
{
  grub_file_t file = 0;
  char *buf = 0, *curr, *next;
  int len;

  if (kernel_type != KERNEL_TYPE_FREEBSD)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "only freebsd support environment");
      return;
    }

  if (argc == 0)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "no filename");
      goto fail;
    }

  file = grub_gzfile_open (argv[0], 1);
  if ((!file) || (!file->size))
    goto fail;

  len = file->size;
  buf = grub_malloc (len + 1);
  if (!buf)
    goto fail;

  if (grub_file_read (file, buf, len) != len)
    goto fail;

  buf[len] = 0;

  next = buf;
  while (next)
    {
      char *p;

      curr = next;
      next = grub_strchr (curr, '\n');
      if (next)
	{

	  p = next - 1;
	  while (p > curr)
	    {
	      if ((*p != '\r') && (*p != ' ') && (*p != '\t'))
		break;
	      p--;
	    }

	  if ((p > curr) && (*p == '"'))
	    p--;

	  *(p + 1) = 0;
	  next++;
	}

      if (*curr == '#')
	continue;

      p = grub_strchr (curr, '=');
      if (!p)
	continue;

      *(p++) = 0;

      if (*curr)
	{
	  char name[grub_strlen (curr) + 8 + 1];

	  if (*p == '"')
	    p++;

	  grub_sprintf (name, "FreeBSD.%s", curr);
	  if (grub_env_set (name, p))
	    goto fail;
	}
    }

fail:
  grub_free (buf);

  if (file)
    grub_file_close (file);
}

void
grub_rescue_cmd_freebsd_module (int argc, char *argv[])
{
  grub_file_t file = 0;

  if (kernel_type != KERNEL_TYPE_FREEBSD)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "only freebsd support module");
      return;
    }

  if (!is_elf_kernel)
    {
      grub_error (GRUB_ERR_BAD_ARGUMENT, "only elf kernel support module");
      return;
    }

  /* List the current modules if no parameter.  */
  if (!argc)
    {
      grub_freebsd_list_modules ();
      return;
    }

  file = grub_gzfile_open (argv[0], 1);
  if ((!file) || (!file->size))
    goto fail;

  if (kern_end + file->size > grub_os_area_addr + grub_os_area_size)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "Not enough memory for the module");
      goto fail;
    }

  grub_file_read (file, (char *) kern_end, file->size);
  if ((!grub_errno) &&
      (!grub_freebsd_add_meta_module (0, argc, argv, kern_end, file->size)))
    kern_end = ALIGN_PAGE (kern_end + file->size);

fail:
  if (file)
    grub_file_close (file);
}

GRUB_MOD_INIT (bsd)
{
  grub_rescue_register_command ("freebsd",
				grub_rescue_cmd_freebsd,
				"load freebsd kernel");
  grub_rescue_register_command ("openbsd",
				grub_rescue_cmd_openbsd,
				"load openbsd kernel");
  grub_rescue_register_command ("netbsd",
				grub_rescue_cmd_netbsd, "load netbsd kernel");

  grub_rescue_register_command ("freebsd_loadenv",
				grub_rescue_cmd_freebsd_loadenv,
				"load freebsd env");
  grub_rescue_register_command ("freebsd_module",
				grub_rescue_cmd_freebsd_module,
				"load freebsd module");

  my_mod = mod;
}

GRUB_MOD_FINI (bsd)
{
  grub_rescue_unregister_command ("freebsd");
  grub_rescue_unregister_command ("openbsd");
  grub_rescue_unregister_command ("netbsd");

  grub_rescue_unregister_command ("freebsd_loadenv");
  grub_rescue_unregister_command ("freebsd_module");

  if (mod_buf)
    {
      grub_free (mod_buf);
      mod_buf = 0;
      mod_buf_max = 0;
    }
}
