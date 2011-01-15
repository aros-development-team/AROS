/* grub-pe2elf.c - tool to convert pe image to elf.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009 Free Software Foundation, Inc.
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
#include <grub/types.h>
#include <grub/util/misc.h>
#include <grub/elf.h>
#include <grub/efi/pe32.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include "progname.h"

static struct option options[] = {
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"verbose", no_argument, 0, 'v'},
  {0, 0, 0, 0}
};

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try `%s --help' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [OPTIONS] input [output]\n\
\n\
Tool to convert pe image to elf.\n\
\nOptions:\n\
  -h, --help                display this message and exit\n\
  -V, --version             print version information and exit\n\
  -v, --verbose             print verbose messages\n\
\n\
Report bugs to <%s>.\n", program_name, PACKAGE_BUGREPORT);

  exit (status);
}

/*
 *  Section layout
 *
 *    null
 *    .text
 *    .rdata
 *    .data
 *    .bss
 *    .modname
 *    .moddeps
 *    .symtab
 *    .strtab
 *    relocation sections
 */

#define TEXT_SECTION	1
#define RDATA_SECTION	2
#define DATA_SECTION	3
#define BSS_SECTION	4
#define MODNAME_SECTION	5
#define MODDEPS_SECTION	6
#define SYMTAB_SECTION	7
#define STRTAB_SECTION	8

#define REL_SECTION	9
#define MAX_SECTIONS	12

#define STRTAB_BLOCK	256

static char *strtab;
static int strtab_max, strtab_len;

Elf32_Ehdr ehdr;
Elf32_Shdr shdr[MAX_SECTIONS];
int num_sections;
grub_uint32_t offset;

static int
insert_string (char *name)
{
  int len, result;

  if (*name == '_')
    name++;

  len = strlen (name);
  if (strtab_len + len >= strtab_max)
    {
      strtab_max += STRTAB_BLOCK;
      strtab = xrealloc (strtab, strtab_max);
    }

  strcpy (strtab + strtab_len, name);
  result = strtab_len;
  strtab_len += len + 1;

  return result;
}

static int *
write_section_data (FILE* fp, char *image,
                    struct grub_pe32_coff_header *pe_chdr,
                    struct grub_pe32_section_table *pe_shdr)
{
  int *section_map;
  int i;

  section_map = xmalloc ((pe_chdr->num_sections + 1) * sizeof (int));
  section_map[0] = 0;

  for (i = 0; i < pe_chdr->num_sections; i++, pe_shdr++)
    {
      grub_uint32_t idx;

      if (! strcmp (pe_shdr->name, ".text"))
        {
          idx = TEXT_SECTION;
          shdr[idx].sh_flags = SHF_ALLOC | SHF_EXECINSTR;
        }
      else if (! strcmp (pe_shdr->name, ".rdata"))
        {
          idx = RDATA_SECTION;
          shdr[idx].sh_flags = SHF_ALLOC;
        }
      else if (! strcmp (pe_shdr->name, ".data"))
        {
          idx = DATA_SECTION;
          shdr[idx].sh_flags = SHF_ALLOC | SHF_WRITE;
        }
      else if (! strcmp (pe_shdr->name, ".bss"))
        {
          idx = BSS_SECTION;
          shdr[idx].sh_flags = SHF_ALLOC | SHF_WRITE;
        }
      else if (! strcmp (pe_shdr->name, ".modname"))
        idx = MODNAME_SECTION;
      else if (! strcmp (pe_shdr->name, ".moddeps"))
        idx = MODDEPS_SECTION;
      else
        {
          section_map[i + 1] = -1;
          continue;
        }

      section_map[i + 1] = idx;

      shdr[idx].sh_type = (idx == BSS_SECTION) ? SHT_NOBITS : SHT_PROGBITS;
      shdr[idx].sh_size = pe_shdr->raw_data_size;
      shdr[idx].sh_addralign = 1 << (((pe_shdr->characteristics >>
                                       GRUB_PE32_SCN_ALIGN_SHIFT) &
                                      GRUB_PE32_SCN_ALIGN_MASK) - 1);

      if (idx != BSS_SECTION)
        {
          shdr[idx].sh_offset = offset;
          grub_util_write_image_at (image + pe_shdr->raw_data_offset,
                                    pe_shdr->raw_data_size, offset, fp);

          offset += pe_shdr->raw_data_size;
        }

      if (pe_shdr->relocations_offset)
        {
          char name[5 + strlen (pe_shdr->name)];

          if (num_sections >= MAX_SECTIONS)
            grub_util_error ("too many sections");

          sprintf (name, ".rel%s", pe_shdr->name);

          shdr[num_sections].sh_name = insert_string (name);
          shdr[num_sections].sh_link = i;
          shdr[num_sections].sh_info = idx;

          shdr[idx].sh_name = shdr[num_sections].sh_name + 4;

          num_sections++;
        }
      else
        shdr[idx].sh_name = insert_string (pe_shdr->name);
    }

  return section_map;
}

static void
write_reloc_section (FILE* fp, char *image,
                     struct grub_pe32_coff_header *pe_chdr,
                     struct grub_pe32_section_table *pe_shdr,
                     Elf32_Sym *symtab,
                     int *symtab_map)
{
  int i;

  for (i = REL_SECTION; i < num_sections; i++)
    {
      struct grub_pe32_section_table *pe_sec;
      struct grub_pe32_reloc *pe_rel;
      Elf32_Rel *rel;
      int num_rels, j, modified;

      pe_sec = pe_shdr + shdr[i].sh_link;
      pe_rel = (struct grub_pe32_reloc *) (image + pe_sec->relocations_offset);
      rel = (Elf32_Rel *) xmalloc (pe_sec->num_relocations * sizeof (Elf32_Rel));
      num_rels = 0;
      modified = 0;

      for (j = 0; j < pe_sec->num_relocations; j++, pe_rel++)
        {
          int type;
          grub_uint32_t ofs, *addr;

          if ((pe_rel->symtab_index >= pe_chdr->num_symbols) ||
              (symtab_map[pe_rel->symtab_index] == -1))
            grub_util_error ("invalid symbol");

          if (pe_rel->type == GRUB_PE32_REL_I386_DIR32)
            type = R_386_32;
          else if (pe_rel->type == GRUB_PE32_REL_I386_REL32)
            type = R_386_PC32;
          else
            grub_util_error ("unknown pe relocation type %d\n", pe_rel->type);

          ofs = pe_rel->offset - pe_sec->virtual_address;
          addr = (grub_uint32_t *)(image + pe_sec->raw_data_offset + ofs);
          if (type == R_386_PC32)
            {
              unsigned char code;

              code = image[pe_sec->raw_data_offset + ofs - 1];

              if (((code != 0xe8) && (code != 0xe9)) || (*addr))
                grub_util_error ("invalid relocation (%x %x)", code, *addr);

              modified = 1;
              if (symtab[symtab_map[pe_rel->symtab_index]].st_shndx)
                {
                  if (symtab[symtab_map[pe_rel->symtab_index]].st_shndx
                      != shdr[i].sh_info)
                    grub_util_error ("cross section call is not allowed");

                  *addr = (symtab[symtab_map[pe_rel->symtab_index]].st_value
                           - ofs - 4);

                  continue;
                }
              else
                *addr = -4;
            }

          rel[num_rels].r_offset = ofs;
          rel[num_rels].r_info = ELF32_R_INFO (symtab_map[pe_rel->symtab_index],
                                               type);
          num_rels++;
        }

      if (modified)
        grub_util_write_image_at (image + pe_sec->raw_data_offset,
                                  shdr[shdr[i].sh_info].sh_size,
                                  shdr[shdr[i].sh_info].sh_offset,
                                  fp);

      shdr[i].sh_type = SHT_REL;
      shdr[i].sh_offset = offset;
      shdr[i].sh_link = SYMTAB_SECTION;
      shdr[i].sh_addralign = 4;
      shdr[i].sh_entsize = sizeof (Elf32_Rel);
      shdr[i].sh_size = num_rels * sizeof (Elf32_Rel);

      grub_util_write_image_at (rel, shdr[i].sh_size, offset, fp);
      offset += shdr[i].sh_size;
      free (rel);
    }
}

static void
write_symbol_table (FILE* fp, char *image,
                    struct grub_pe32_coff_header *pe_chdr,
                    struct grub_pe32_section_table *pe_shdr,
                    int *section_map)
{
  struct grub_pe32_symbol *pe_symtab;
  char *pe_strtab;
  Elf32_Sym *symtab;
  int *symtab_map, num_syms;
  int i;

  pe_symtab = (struct grub_pe32_symbol *) (image + pe_chdr->symtab_offset);
  pe_strtab = (char *) (pe_symtab + pe_chdr->num_symbols);

  symtab = (Elf32_Sym *) xmalloc ((pe_chdr->num_symbols + 1) *
                                  sizeof (Elf32_Sym));
  memset (symtab, 0, (pe_chdr->num_symbols + 1) * sizeof (Elf32_Sym));
  num_syms = 1;

  symtab_map = (int *) xmalloc (pe_chdr->num_symbols * sizeof (int));

  for (i = 0; i < (int) pe_chdr->num_symbols;
       i += pe_symtab->num_aux + 1, pe_symtab += pe_symtab->num_aux + 1)
    {
      int bind, type;

      symtab_map[i] = -1;
      if ((pe_symtab->section > pe_chdr->num_sections) ||
          (section_map[pe_symtab->section] == -1))
        continue;

      if (! pe_symtab->section)
        type = STT_NOTYPE;
      else if (pe_symtab->type == GRUB_PE32_DT_FUNCTION)
        type = STT_FUNC;
      else
        type = STT_OBJECT;

      if (pe_symtab->storage_class == GRUB_PE32_SYM_CLASS_EXTERNAL)
        bind = STB_GLOBAL;
      else
        bind = STB_LOCAL;

      if ((type != STT_FUNC) && (pe_symtab->num_aux))
        {
          if (! pe_symtab->value)
            type = STT_SECTION;

          symtab[num_syms].st_name = shdr[section_map[pe_symtab->section]].sh_name;
        }
      else
        {
	  char short_name[9];
          char *name;

	  if (pe_symtab->long_name[0])
	    {
	      strncpy (short_name, pe_symtab->short_name, 8);
	      short_name[8] = 0;
	      name = short_name;
	    }
	  else
	    name = pe_strtab + pe_symtab->long_name[1];

          if ((strcmp (name, "_grub_mod_init")) &&
              (strcmp (name, "_grub_mod_fini")) &&
              (bind == STB_LOCAL))
              continue;

          symtab[num_syms].st_name = insert_string (name);
        }

      symtab[num_syms].st_shndx = section_map[pe_symtab->section];
      symtab[num_syms].st_value = pe_symtab->value;
      symtab[num_syms].st_info = ELF32_ST_INFO (bind, type);

      symtab_map[i] = num_syms;
      num_syms++;
    }

  write_reloc_section (fp, image, pe_chdr, pe_shdr, symtab, symtab_map);

  shdr[SYMTAB_SECTION].sh_name = insert_string (".symtab");
  shdr[SYMTAB_SECTION].sh_type = SHT_SYMTAB;
  shdr[SYMTAB_SECTION].sh_offset = offset;
  shdr[SYMTAB_SECTION].sh_size = num_syms * sizeof (Elf32_Sym);
  shdr[SYMTAB_SECTION].sh_entsize = sizeof (Elf32_Sym);
  shdr[SYMTAB_SECTION].sh_link = STRTAB_SECTION;
  shdr[SYMTAB_SECTION].sh_addralign = 4;

  grub_util_write_image_at (symtab, shdr[SYMTAB_SECTION].sh_size,
                            offset, fp);
  offset += shdr[SYMTAB_SECTION].sh_size;

  free (symtab);
  free (symtab_map);
}

static void
write_string_table (FILE* fp)
{
  shdr[STRTAB_SECTION].sh_name = insert_string (".strtab");
  shdr[STRTAB_SECTION].sh_type = SHT_STRTAB;
  shdr[STRTAB_SECTION].sh_offset = offset;
  shdr[STRTAB_SECTION].sh_size = strtab_len;
  shdr[STRTAB_SECTION].sh_addralign = 1;
  grub_util_write_image_at (strtab, strtab_len, offset, fp);
  offset += strtab_len;

  free (strtab);
}

static void
write_section_header (FILE* fp)
{
  ehdr.e_ident[EI_MAG0] = ELFMAG0;
  ehdr.e_ident[EI_MAG1] = ELFMAG1;
  ehdr.e_ident[EI_MAG2] = ELFMAG2;
  ehdr.e_ident[EI_MAG3] = ELFMAG3;
  ehdr.e_ident[EI_VERSION] = EV_CURRENT;
  ehdr.e_version = EV_CURRENT;
  ehdr.e_type = ET_REL;

  ehdr.e_ident[EI_CLASS] = ELFCLASS32;
  ehdr.e_ident[EI_DATA] = ELFDATA2LSB;
  ehdr.e_machine = EM_386;

  ehdr.e_ehsize = sizeof (ehdr);
  ehdr.e_shentsize = sizeof (Elf32_Shdr);
  ehdr.e_shstrndx = STRTAB_SECTION;

  ehdr.e_shoff = offset;
  ehdr.e_shnum = num_sections;
  grub_util_write_image_at (&shdr, sizeof (Elf32_Shdr) * num_sections,
                            offset, fp);

  grub_util_write_image_at (&ehdr, sizeof (Elf32_Ehdr), 0, fp);
}

static void
convert_pe (FILE* fp, char *image)
{
  struct grub_pe32_coff_header *pe_chdr;
  struct grub_pe32_section_table *pe_shdr;
  int *section_map;

  pe_chdr = (struct grub_pe32_coff_header *) image;
  if (grub_le_to_cpu16 (pe_chdr->machine) != GRUB_PE32_MACHINE_I386)
    grub_util_error ("invalid coff image");

  strtab = xmalloc (STRTAB_BLOCK);
  strtab_max = STRTAB_BLOCK;
  strtab[0] = 0;
  strtab_len = 1;

  offset = sizeof (ehdr);
  pe_shdr = (struct grub_pe32_section_table *) (pe_chdr + 1);
  num_sections = REL_SECTION;

  section_map = write_section_data (fp, image, pe_chdr, pe_shdr);

  write_symbol_table (fp, image, pe_chdr, pe_shdr, section_map);
  free (section_map);

  write_string_table (fp);

  write_section_header (fp);
}

int
main (int argc, char *argv[])
{
  char *image;
  FILE* fp;

  set_program_name (argv[0]);

    /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "hVv", options, 0);

      if (c == -1)
	break;
      else
	switch (c)
	  {
	  case 'h':
	    usage (0);
	    break;

	  case 'V':
	    printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
	    return 0;

	  case 'v':
	    verbosity++;
	    break;

	  default:
	    usage (1);
	    break;
	  }
    }

  /* Obtain PATH.  */
  if (optind >= argc)
    {
      fprintf (stderr, "Filename not specified.\n");
      usage (1);
    }

  image = grub_util_read_image (argv[optind]);

  if (optind + 1 < argc)
    optind++;

  fp = fopen (argv[optind], "wb");
  if (! fp)
    grub_util_error ("cannot open %s", argv[optind]);

  convert_pe (fp, image);

  fclose (fp);

  return 0;
}
