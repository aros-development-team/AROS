#include <grub/loader.h>
#include <grub/cpu/bsd.h>
#include <grub/mm.h>
#include <grub/elf.h>
#include <grub/misc.h>
#include <grub/i386/loader.h>

#define ALIGN_PAGE(a)	ALIGN_UP (a, 4096)

static inline grub_err_t
load (grub_file_t file, void *where, grub_off_t off, grub_size_t size)
{
  if (PTR_TO_UINT32 (where) + size > grub_os_area_addr + grub_os_area_size)
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
		       "Not enough memory for the module");
  if (grub_file_seek (file, off) == (grub_off_t) -1)
    return grub_errno;
  if (grub_file_read (file, where, size)
      != (grub_ssize_t) size)
    {
      if (grub_errno)
	return grub_errno;
      else
	return grub_error (GRUB_ERR_BAD_OS, "file is truncated");
    }
  return GRUB_ERR_NONE;
}

static inline grub_err_t
read_headers (grub_file_t file, Elf_Ehdr *e, char **shdr)
{
 if (grub_file_seek (file, 0) == (grub_off_t) -1)
    return grub_errno;

  if (grub_file_read (file, (char *) e, sizeof (*e)) != sizeof (*e))
    {
      if (grub_errno)
	return grub_errno;
      else
	return grub_error (GRUB_ERR_BAD_OS, "file is too short");
    }

  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT
      || e->e_version != EV_CURRENT)
    return grub_error (GRUB_ERR_BAD_OS, "invalid arch independent ELF magic");

  if (e->e_ident[EI_CLASS] != SUFFIX (ELFCLASS))
    return grub_error (GRUB_ERR_BAD_OS, "invalid arch dependent ELF magic");

  *shdr = grub_malloc (e->e_shnum * e->e_shentsize);
  if (! *shdr)
    return grub_errno;

  if (grub_file_seek (file, e->e_shoff) == (grub_off_t) -1)
    return grub_errno;

  if (grub_file_read (file, *shdr, e->e_shnum * e->e_shentsize)
      != e->e_shnum * e->e_shentsize)
    {
      if (grub_errno)
	return grub_errno;
      else
	return grub_error (GRUB_ERR_BAD_OS, "file is truncated");
    }

  return GRUB_ERR_NONE;
}

/* On i386 FreeBSD uses "elf module" approarch for 32-bit variant
   and "elf obj module" for 64-bit variant. However it may differ on other
   platforms. So I keep both versions.  */
#if OBJSYM
grub_err_t
SUFFIX (grub_freebsd_load_elfmodule_obj) (grub_file_t file, int argc,
					  char *argv[], grub_addr_t *kern_end)
{
  Elf_Ehdr e;
  Elf_Shdr *s;
  char *shdr;
  grub_addr_t curload, module;
  grub_err_t err;

  err = read_headers (file, &e, &shdr);
  if (err)
    return err;

  curload = module = ALIGN_PAGE (*kern_end);

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) ((char *) shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
    {
      if (s->sh_size == 0)
	continue;

      if (s->sh_addralign)
	curload = ALIGN_UP (curload, s->sh_addralign);
      s->sh_addr = curload;

      grub_dprintf ("bsd", "loading section to %x, size %d, align %d\n",
		    (unsigned) curload, (int) s->sh_size,
		    (int) s->sh_addralign);

      switch (s->sh_type)
	{
	default:
	case SHT_PROGBITS:
	  err = load (file, UINT_TO_PTR (curload), s->sh_offset, s->sh_size);
	  if (err)
	    return err;
	  break;
	case SHT_NOBITS:
	  if (curload + s->sh_size > grub_os_area_addr + grub_os_area_size)
	    return grub_error (GRUB_ERR_OUT_OF_RANGE,
			       "Not enough memory for the module");
	  grub_memset (UINT_TO_PTR (curload), 0, s->sh_size);
	  break;
	}
      curload += s->sh_size;
    }

  *kern_end = ALIGN_PAGE (curload);

  err = grub_freebsd_add_meta_module (argv[0], FREEBSD_MODTYPE_ELF_MODULE_OBJ,
				      argc - 1, argv + 1, module,
				      curload - module);
  if (! err)
    err = grub_freebsd_add_meta (FREEBSD_MODINFO_METADATA
				 | FREEBSD_MODINFOMD_ELFHDR,
				 &e, sizeof (e));
  if (! err)
    err = grub_freebsd_add_meta (FREEBSD_MODINFO_METADATA
				 | FREEBSD_MODINFOMD_SHDR,
				 shdr, e.e_shnum * e.e_shentsize);

  return err;
}

#else

grub_err_t
SUFFIX (grub_freebsd_load_elfmodule) (grub_file_t file, int argc, char *argv[],
				      grub_addr_t *kern_end)
{
  Elf_Ehdr e;
  Elf_Shdr *s;
  char *shdr;
  grub_addr_t curload, module;
  grub_err_t err;

  err = read_headers (file, &e, &shdr);
  if (err)
    return err;

  curload = module = ALIGN_PAGE (*kern_end);

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) ((char *) shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
    {
      if (s->sh_size == 0)
	continue;

      if (! (s->sh_flags & SHF_ALLOC))
	continue;

      grub_dprintf ("bsd", "loading section to %x, size %d, align %d\n",
		    (unsigned) curload, (int) s->sh_size,
		    (int) s->sh_addralign);

      switch (s->sh_type)
	{
	default:
	case SHT_PROGBITS:
	  err = load (file, UINT_TO_PTR (module + s->sh_addr),
		      s->sh_offset, s->sh_size);
	  if (err)
	    return err;
	  break;
	case SHT_NOBITS:
	  if (module + s->sh_addr + s->sh_size
	      > grub_os_area_addr + grub_os_area_size)
	    return grub_error (GRUB_ERR_OUT_OF_RANGE,
			       "Not enough memory for the module");
	  grub_memset (UINT_TO_PTR (module + s->sh_addr), 0, s->sh_size);
	  break;
	}
      if (curload < module + s->sh_addr + s->sh_size)
	curload = module + s->sh_addr + s->sh_size;
    }

  load (file, UINT_TO_PTR (module), 0, sizeof (e));
  if (curload < module + sizeof (e))
    curload = module + sizeof (e);

  load (file, UINT_TO_PTR (curload), e.e_shoff,
	e.e_shnum * e.e_shentsize);
  e.e_shoff = curload - module;
  curload +=  e.e_shnum * e.e_shentsize;

  load (file, UINT_TO_PTR (curload), e.e_phoff,
	e.e_phnum * e.e_phentsize);
  e.e_phoff = curload - module;
  curload +=  e.e_phnum * e.e_phentsize;

  *kern_end = curload;

  grub_freebsd_add_meta_module (argv[0], FREEBSD_MODTYPE_ELF_MODULE,
				argc - 1, argv + 1, module,
				curload - module);
  return SUFFIX (grub_freebsd_load_elf_meta) (file, kern_end);
}

#endif

grub_err_t
SUFFIX (grub_freebsd_load_elf_meta) (grub_file_t file, grub_addr_t *kern_end)
{
  grub_err_t err;
  Elf_Ehdr e;
  Elf_Shdr *s;
  char *shdr;
  unsigned symoff, stroff, symsize, strsize;
  grub_addr_t curload;
  grub_freebsd_addr_t symstart, symend, symentsize, dynamic;
  Elf_Sym *sym;
  const char *str;
  unsigned i;

  err = read_headers (file, &e, &shdr);
  if (err)
    return err;

  err = grub_freebsd_add_meta (FREEBSD_MODINFO_METADATA |
			       FREEBSD_MODINFOMD_ELFHDR, &e,
			       sizeof (e));
  if (err)
    return err;

  for (s = (Elf_Shdr *) shdr; s < (Elf_Shdr *) (shdr
						+ e.e_shnum * e.e_shentsize);
       s = (Elf_Shdr *) ((char *) s + e.e_shentsize))
      if (s->sh_type == SHT_SYMTAB)
	break;
  if (s >= (Elf_Shdr *) ((char *) shdr
			+ e.e_shnum * e.e_shentsize))
    return grub_error (GRUB_ERR_BAD_OS, "no symbol table");
  symoff = s->sh_offset;
  symsize = s->sh_size;
  symentsize = s->sh_entsize;
  s = (Elf_Shdr *) (shdr + e.e_shentsize * s->sh_link);
  stroff = s->sh_offset;
  strsize = s->sh_size;

  if (*kern_end + 4 * sizeof (grub_freebsd_addr_t) + symsize + strsize
      > grub_os_area_addr + grub_os_area_size)
    return grub_error (GRUB_ERR_OUT_OF_RANGE,
		       "Not enough memory for kernel symbols");

  symstart = curload = ALIGN_UP (*kern_end, sizeof (grub_freebsd_addr_t));
  *((grub_freebsd_addr_t *) UINT_TO_PTR (curload)) = symsize;
  curload += sizeof (grub_freebsd_addr_t);
  if (grub_file_seek (file, symoff) == (grub_off_t) -1)
    return grub_errno;
  sym = (Elf_Sym *) UINT_TO_PTR (curload);
  if (grub_file_read (file, UINT_TO_PTR (curload), symsize) !=
      (grub_ssize_t) symsize)
    {
      if (! grub_errno)
	return grub_error (GRUB_ERR_BAD_OS, "invalid elf");
      return grub_errno;
    }
  curload += symsize;

  *((grub_freebsd_addr_t *) UINT_TO_PTR (curload)) = strsize;
  curload += sizeof (grub_freebsd_addr_t);
  if (grub_file_seek (file, stroff) == (grub_off_t) -1)
    return grub_errno;
  str = (char *) UINT_TO_PTR (curload);
  if (grub_file_read (file, UINT_TO_PTR (curload), strsize)
      != (grub_ssize_t) strsize)
    {
      if (! grub_errno)
	return grub_error (GRUB_ERR_BAD_OS, "invalid elf");
      return grub_errno;
    }
  curload += strsize;
  curload = ALIGN_UP (curload, sizeof (grub_freebsd_addr_t));
  symend = curload;

  for (i = 0;
       i * symentsize < symsize;
       i++, sym = (Elf_Sym *) ((char *) sym + symentsize))
    {
      const char *name = str + sym->st_name;
      if (grub_strcmp (name, "_DYNAMIC") == 0)
	break;
    }

  if (i * symentsize < symsize)
    {
      dynamic = sym->st_value;
      grub_dprintf ("bsd", "dynamic = %llx\n", (unsigned long long) dynamic);
      err = grub_freebsd_add_meta (FREEBSD_MODINFO_METADATA |
				   FREEBSD_MODINFOMD_DYNAMIC, &dynamic,
				   sizeof (dynamic));
      if (err)
	return err;
    }

  err = grub_freebsd_add_meta (FREEBSD_MODINFO_METADATA |
			       FREEBSD_MODINFOMD_SSYM, &symstart,
			       sizeof (symstart));
  if (err)
    return err;

  err = grub_freebsd_add_meta (FREEBSD_MODINFO_METADATA |
			       FREEBSD_MODINFOMD_ESYM, &symend,
			       sizeof (symend));
  if (err)
    return err;
  *kern_end = ALIGN_PAGE (curload);

  return GRUB_ERR_NONE;
}
