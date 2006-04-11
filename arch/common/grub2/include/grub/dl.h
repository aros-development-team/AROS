/* dl.h - types and prototypes for loadable module support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002, 2004, 2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_DL_H
#define GRUB_DL_H	1

#include <grub/symbol.h>
#include <grub/err.h>
#include <grub/types.h>

#define GRUB_MOD_INIT	\
static void grub_mod_init (grub_dl_t mod) __attribute__ ((used)); \
static void \
grub_mod_init (grub_dl_t mod)

#define GRUB_MOD_FINI	\
static void grub_mod_fini (void) __attribute__ ((used)); \
static void \
grub_mod_fini (void)

#define GRUB_MOD_NAME(name)	\
__asm__ (".section .modname,\"S\"\n.string \"" #name "\"\n.previous")

#define GRUB_MOD_DEP(name)	\
__asm__ (".section .moddeps,\"S\"\n.string \"" #name "\"\n.previous")

struct grub_dl_segment
{
  struct grub_dl_segment *next;
  void *addr;
  grub_size_t size;
  unsigned section;
};
typedef struct grub_dl_segment *grub_dl_segment_t;

struct grub_dl;

struct grub_dl_dep
{
  struct grub_dl_dep *next;
  struct grub_dl *mod;
};
typedef struct grub_dl_dep *grub_dl_dep_t;

struct grub_dl
{
  char *name;
  int ref_count;
  grub_dl_dep_t dep;
  grub_dl_segment_t segment;
  void (*init) (struct grub_dl *mod);
  void (*fini) (void);
};
typedef struct grub_dl *grub_dl_t;

grub_err_t EXPORT_FUNC(grub_dl_check_header) (void *ehdr, grub_size_t size);
grub_dl_t EXPORT_FUNC(grub_dl_load_file) (const char *filename);
grub_dl_t EXPORT_FUNC(grub_dl_load) (const char *name);
grub_dl_t grub_dl_load_core (void *addr, grub_size_t size);
int EXPORT_FUNC(grub_dl_unload) (grub_dl_t mod);
void grub_dl_unload_unneeded (void);
void grub_dl_unload_all (void);
int EXPORT_FUNC(grub_dl_ref) (grub_dl_t mod);
int EXPORT_FUNC(grub_dl_unref) (grub_dl_t mod);
void EXPORT_FUNC(grub_dl_iterate) (int (*hook) (grub_dl_t mod));
grub_dl_t EXPORT_FUNC(grub_dl_get) (const char *name);
grub_err_t EXPORT_FUNC(grub_dl_register_symbol) (const char *name, void *addr,
					    grub_dl_t mod);
void *EXPORT_FUNC(grub_dl_resolve_symbol) (const char *name);

grub_err_t grub_arch_dl_check_header (void *ehdr);
grub_err_t grub_arch_dl_relocate_symbols (grub_dl_t mod, void *ehdr);

#endif /* ! GRUB_DL_H */
