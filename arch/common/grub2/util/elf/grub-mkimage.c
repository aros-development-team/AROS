/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004,2005,2006,2007,2008  Free Software Foundation, Inc.
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

#include <config.h>

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <grub/elf.h>
#include <grub/misc.h>
#include <grub/util/misc.h>
#include <grub/util/resolve.h>
#include <grub/kernel.h>
#include <grub/machine/kernel.h>

#define GRUB_IEEE1275_NOTE_NAME "PowerPC"
#define GRUB_IEEE1275_NOTE_TYPE 0x1275

/* These structures are defined according to the CHRP binding to IEEE1275,
   "Client Program Format" section.  */

struct grub_ieee1275_note_hdr
{
  grub_uint32_t namesz;
  grub_uint32_t descsz;
  grub_uint32_t type;
  char name[sizeof (GRUB_IEEE1275_NOTE_NAME)];
};

struct grub_ieee1275_note_desc
{
  grub_uint32_t real_mode;
  grub_uint32_t real_base;
  grub_uint32_t real_size;
  grub_uint32_t virt_base;
  grub_uint32_t virt_size;
  grub_uint32_t load_base;
};

struct grub_ieee1275_note
{
  struct grub_ieee1275_note_hdr header;
  struct grub_ieee1275_note_desc descriptor;
};

void
load_note (Elf32_Phdr *phdr, FILE *out)
{
  struct grub_ieee1275_note note;
  int note_size = sizeof (struct grub_ieee1275_note);

  grub_util_info ("adding CHRP NOTE segment");

  note.header.namesz = grub_host_to_target32 (sizeof (GRUB_IEEE1275_NOTE_NAME));
  note.header.descsz = grub_host_to_target32 (note_size);
  note.header.type = grub_host_to_target32 (GRUB_IEEE1275_NOTE_TYPE);
  strcpy (note.header.name, GRUB_IEEE1275_NOTE_NAME);
  note.descriptor.real_mode = grub_host_to_target32 (0xffffffff);
  note.descriptor.real_base = grub_host_to_target32 (0x00c00000);
  note.descriptor.real_size = grub_host_to_target32 (0xffffffff);
  note.descriptor.virt_base = grub_host_to_target32 (0xffffffff);
  note.descriptor.virt_size = grub_host_to_target32 (0xffffffff);
  note.descriptor.load_base = grub_host_to_target32 (0x00004000);

  /* Write the note data to the new segment.  */
  grub_util_write_image_at (&note, note_size,
			    grub_target_to_host32 (phdr->p_offset), out);

  /* Fill in the rest of the segment header.  */
  phdr->p_type = grub_host_to_target32 (PT_NOTE);
  phdr->p_flags = grub_host_to_target32 (PF_R);
  phdr->p_align = grub_host_to_target32 (sizeof (long));
  phdr->p_vaddr = 0;
  phdr->p_paddr = 0;
  phdr->p_filesz = grub_host_to_target32 (note_size);
  phdr->p_memsz = 0;
}

void
load_modules (grub_addr_t modbase, Elf32_Phdr *phdr, const char *dir,
	      char *mods[], FILE *out)
{
  char *module_img;
  struct grub_util_path_list *path_list;
  struct grub_util_path_list *p;
  struct grub_module_info *modinfo;
  size_t offset;
  size_t total_module_size;

  path_list = grub_util_resolve_dependencies (dir, "moddep.lst", mods);

  offset = sizeof (struct grub_module_info);
  total_module_size = sizeof (struct grub_module_info);
  for (p = path_list; p; p = p->next)
    {
      total_module_size += (grub_util_get_image_size (p->name)
	  + sizeof (struct grub_module_header));
    }

  grub_util_info ("the total module size is 0x%x", total_module_size);

  module_img = xmalloc (total_module_size);
  modinfo = (struct grub_module_info *) module_img;
  modinfo->magic = grub_host_to_target32 (GRUB_MODULE_MAGIC);
  modinfo->offset = grub_host_to_target32 (sizeof (struct grub_module_info));
  modinfo->size = grub_host_to_target32 (total_module_size);

  /* Load all the modules, with headers, into module_img.  */
  for (p = path_list; p; p = p->next)
    {
      struct grub_module_header *header;
      size_t mod_size;

      grub_util_info ("adding module %s", p->name);

      mod_size = grub_util_get_image_size (p->name);

      header = (struct grub_module_header *) (module_img + offset);
      header->offset = grub_host_to_target32 (sizeof (*header));
      header->size = grub_host_to_target32 (mod_size + sizeof (*header));

      grub_util_load_image (p->name, module_img + offset + sizeof (*header));

      offset += sizeof (*header) + mod_size;
    }

  /* Write the module data to the new segment.  */
  grub_util_write_image_at (module_img, total_module_size,
			    grub_host_to_target32 (phdr->p_offset), out);

  /* Fill in the rest of the segment header.  */
  phdr->p_type = grub_host_to_target32 (PT_LOAD);
  phdr->p_flags = grub_host_to_target32 (PF_R | PF_W | PF_X);
  phdr->p_align = grub_host_to_target32 (sizeof (long));
  phdr->p_vaddr = grub_host_to_target32 (modbase);
  phdr->p_paddr = grub_host_to_target32 (modbase);
  phdr->p_filesz = grub_host_to_target32 (total_module_size);
  phdr->p_memsz = grub_host_to_target32 (total_module_size);
}

void
add_segments (char *dir, FILE *out, int chrp, char *mods[])
{
  Elf32_Ehdr ehdr;
  Elf32_Phdr *phdrs = NULL;
  Elf32_Phdr *phdr;
  FILE *in;
  char *kernel_path;
  grub_addr_t grub_end = 0;
  off_t phdroff;
  int i;

  /* Read ELF header.  */
  kernel_path = grub_util_get_path (dir, "kernel.elf");
  in = fopen (kernel_path, "rb");
  if (! in)
    grub_util_error ("cannot open %s", kernel_path);

  grub_util_read_at (&ehdr, sizeof (ehdr), 0, in);
  
  phdrs = xmalloc (grub_target_to_host16 (ehdr.e_phentsize)
		   * (grub_target_to_host16 (ehdr.e_phnum) + 2));
  /* Copy all existing segments.  */
  for (i = 0; i < grub_target_to_host16 (ehdr.e_phnum); i++)
    {
      char *segment_img;
      grub_size_t segment_end;

      phdr = phdrs + i;

      /* Read segment header.  */
      grub_util_read_at (phdr, sizeof (Elf32_Phdr),
			 (grub_target_to_host32 (ehdr.e_phoff)
			  + (i * grub_target_to_host16 (ehdr.e_phentsize))),
			 in);
      grub_util_info ("copying segment %d, type %d", i,
		      grub_target_to_host32 (phdr->p_type));

      /* Locate _end.  */
      segment_end = grub_target_to_host32 (phdr->p_paddr)
		    + grub_target_to_host32 (phdr->p_memsz);
      grub_util_info ("segment %u end 0x%lx", i, segment_end);
      if (segment_end > grub_end)
	grub_end = segment_end;

      /* Read segment data and write it to new file.  */
      segment_img = xmalloc (grub_target_to_host32 (phdr->p_filesz));
  
      grub_util_read_at (segment_img, grub_target_to_host32 (phdr->p_filesz),
			 grub_target_to_host32 (phdr->p_offset), in);
      grub_util_write_image_at (segment_img, grub_target_to_host32 (phdr->p_filesz),
				grub_target_to_host32 (phdr->p_offset), out);

      free (segment_img);
    }

  if (mods[0] != NULL)
    {
      grub_addr_t modbase;

      /* Place modules just after grub segment.  */
      modbase = ALIGN_UP(grub_end + GRUB_MOD_GAP, GRUB_MOD_ALIGN);

      /* Construct new segment header for modules.  */
      phdr = phdrs + grub_target_to_host16 (ehdr.e_phnum);
      ehdr.e_phnum = grub_host_to_target16 (grub_target_to_host16 (ehdr.e_phnum) + 1);

      /* Fill in p_offset so the callees know where to write.  */
      phdr->p_offset = grub_host_to_target32 (ALIGN_UP (grub_util_get_fp_size (out),
						   sizeof (long)));

      load_modules (modbase, phdr, dir, mods, out);
    }

  if (chrp)
    {
      /* Construct new segment header for the CHRP note.  */
      phdr = phdrs + grub_target_to_host16 (ehdr.e_phnum);
      ehdr.e_phnum = grub_host_to_target16 (grub_target_to_host16 (ehdr.e_phnum) + 1);

      /* Fill in p_offset so the callees know where to write.  */
      phdr->p_offset = grub_host_to_target32 (ALIGN_UP (grub_util_get_fp_size (out),
						   sizeof (long)));

      load_note (phdr, out);
    }

  /* Don't bother preserving the section headers.  */
  ehdr.e_shoff = 0;
  ehdr.e_shnum = 0;
  ehdr.e_shstrndx = 0;

  /* Append entire segment table to the file.  */
  phdroff = ALIGN_UP (grub_util_get_fp_size (out), sizeof (long));
  grub_util_write_image_at (phdrs, grub_target_to_host16 (ehdr.e_phentsize)
			    * grub_target_to_host16 (ehdr.e_phnum), phdroff,
			    out);

  /* Write ELF header.  */
  ehdr.e_phoff = grub_host_to_target32 (phdroff);
  grub_util_write_image_at (&ehdr, sizeof (ehdr), 0, out);

  free (phdrs);
  free (kernel_path);
}

static struct option options[] =
  {
    {"directory", required_argument, 0, 'd'},
    {"output", required_argument, 0, 'o'},
    {"help", no_argument, 0, 'h'},
    {"note", no_argument, 0, 'n'},
    {"version", no_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    { 0, 0, 0, 0 },
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try ``grub-mkimage --help'' for more information.\n");
  else
    printf ("\
Usage: grub-mkimage -o FILE [OPTION]... [MODULES]\n\
\n\
Make a bootable image of GRUB.\n\
\n\
-d, --directory=DIR     use images and modules under DIR [default=%s]\n\
-o, --output=FILE       output a generated image to FILE\n\
-h, --help              display this message and exit\n\
-n, --note              add NOTE segment for CHRP Open Firmware\n\
-V, --version           print version information and exit\n\
-v, --verbose           print verbose messages\n\
\n\
Report bugs to <%s>.\n\
", GRUB_LIBDIR, PACKAGE_BUGREPORT);

  exit (status);
}

int
main (int argc, char *argv[])
{
  FILE *fp;
  char *output = NULL;
  char *dir = NULL;
  int chrp = 0;

  progname = "grub-mkimage";

  while (1)
    {
      int c = getopt_long (argc, argv, "d:o:hVvn", options, 0);
      if (c == -1)
	break;

      switch (c)
	{
	  case 'd':
	    if (dir)
	      free (dir);
	    dir = xstrdup (optarg);
	    break;
	  case 'h':
	    usage (0);
	    break;
	  case 'n':
	    chrp = 1;
	    break;
	  case 'o':
	    if (output)
	      free (output);
	    output = xstrdup (optarg);
	    break;
	  case 'V':
	    printf ("grub-mkimage (%s) %s\n", PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;
	  case 'v':
	    verbosity++;
	    break;
	  default:
	    usage (1);
	    break;
	}
  }

  if (!output)
    usage (1);

  fp = fopen (output, "wb");
  if (! fp)
    grub_util_error ("cannot open %s", output);

  add_segments (dir ? : GRUB_LIBDIR, fp, chrp, argv + optind);

  fclose (fp);

  return 0;
}
