/* ieee1275.h - Access the Open Firmware client interface.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003, 2004, 2005  Free Software Foundation, Inc.
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

#ifndef GRUB_IEEE1275_HEADER
#define GRUB_IEEE1275_HEADER	1

#include <stdint.h>
#include <grub/err.h>
#include <grub/types.h>
#include <grub/machine/ieee1275.h>

/* Maps a device alias to a pathname.  */
struct grub_ieee1275_devalias
{
  char *name;
  char *path;
  char *type;
};

struct grub_ieee1275_mem_region 
{
  unsigned int start;
  unsigned int size;
};

#ifndef IEEE1275_CALL_ENTRY_FN
#define IEEE1275_CALL_ENTRY_FN(args) (*grub_ieee1275_entry_fn) (args)
#endif

/* All backcalls to the firmware is done by calling an entry function 
   which was passed to us from the bootloader.  When doing the backcall, 
   a structure is passed which specifies what the firmware should do.  
   NAME is the requested service.  NR_INS and NR_OUTS is the number of
   passed arguments and the expected number of return values, resp. */
struct grub_ieee1275_common_hdr
{
  grub_ieee1275_cell_t name;
  grub_ieee1275_cell_t nr_ins;
  grub_ieee1275_cell_t nr_outs;
};

#define INIT_IEEE1275_COMMON(p, xname, xins, xouts) \
  (p)->name = (grub_ieee1275_cell_t) xname; \
  (p)->nr_ins = (grub_ieee1275_cell_t) xins; \
  (p)->nr_outs = (grub_ieee1275_cell_t) xouts

typedef grub_ieee1275_cell_t grub_ieee1275_ihandle_t;
typedef grub_ieee1275_cell_t grub_ieee1275_phandle_t;

extern grub_ieee1275_phandle_t EXPORT_VAR(grub_ieee1275_chosen);
extern int (* EXPORT_VAR(grub_ieee1275_entry_fn)) (void *);

enum grub_ieee1275_flag
{
  /* Old World Macintosh firmware fails seek when "dev:0" is opened.  */
  GRUB_IEEE1275_FLAG_NO_PARTITION_0,

  /* Apple firmware runs in translated mode and requires use of the "map"
     method.  Other firmware runs in untranslated mode and doesn't like "map"
     calls.  */
  GRUB_IEEE1275_FLAG_REAL_MODE,

  /* CHRP specifies partitions are numbered from 1 (partition 0 refers to the
     whole disk). However, CodeGen firmware numbers partitions from 0.  */
  GRUB_IEEE1275_FLAG_0_BASED_PARTITIONS,
};

extern int EXPORT_FUNC(grub_ieee1275_test_flag) (enum grub_ieee1275_flag flag);
extern void EXPORT_FUNC(grub_ieee1275_set_flag) (enum grub_ieee1275_flag flag);




grub_uint32_t EXPORT_FUNC(grub_ieee1275_decode_int_4) (unsigned char *p);
int EXPORT_FUNC(grub_ieee1275_finddevice) (char *name,
					   grub_ieee1275_phandle_t *phandlep);
int EXPORT_FUNC(grub_ieee1275_get_property) (grub_ieee1275_phandle_t phandle,
					     const char *property, void *buf,
					     grub_size_t size,
					     grub_ssize_t *actual);
int EXPORT_FUNC(grub_ieee1275_next_property) (grub_ieee1275_phandle_t phandle,
					      char *prev_prop, char *prop,
					      grub_ieee1275_cell_t *flags);
int EXPORT_FUNC(grub_ieee1275_get_property_length) 
     (grub_ieee1275_phandle_t phandle, const char *prop, grub_ssize_t *length);
int EXPORT_FUNC(grub_ieee1275_instance_to_package) 
     (grub_ieee1275_ihandle_t ihandle, grub_ieee1275_phandle_t *phandlep);
int EXPORT_FUNC(grub_ieee1275_package_to_path) (grub_ieee1275_phandle_t phandle,
						char *path, grub_size_t len,
						grub_ssize_t *actual);
int EXPORT_FUNC(grub_ieee1275_instance_to_path) 
     (grub_ieee1275_ihandle_t ihandle, char *path, grub_size_t len,
      grub_ssize_t *actual);
int EXPORT_FUNC(grub_ieee1275_write) (grub_ieee1275_ihandle_t ihandle,
				      void *buffer, grub_size_t len,
				      grub_ssize_t *actualp);
int EXPORT_FUNC(grub_ieee1275_read) (grub_ieee1275_ihandle_t ihandle,
				     void *buffer, grub_size_t len,
				     grub_ssize_t *actualp);
int EXPORT_FUNC(grub_ieee1275_seek) (grub_ieee1275_ihandle_t ihandle,
				     int pos_hi, int pos_lo,
				     grub_ssize_t *result);
int EXPORT_FUNC(grub_ieee1275_peer) (grub_ieee1275_phandle_t node,
				     grub_ieee1275_phandle_t *result);
int EXPORT_FUNC(grub_ieee1275_child) (grub_ieee1275_phandle_t node,
				      grub_ieee1275_phandle_t *result);
int EXPORT_FUNC(grub_ieee1275_parent) (grub_ieee1275_phandle_t node,
				       grub_ieee1275_phandle_t *result);
int EXPORT_FUNC(grub_ieee1275_interpret) (const char *command,
					  grub_ieee1275_cell_t *catch);
int EXPORT_FUNC(grub_ieee1275_enter) (void);
void EXPORT_FUNC(grub_ieee1275_exit) (void) __attribute__ ((noreturn));
int EXPORT_FUNC(grub_ieee1275_open) (const char *node,
				     grub_ieee1275_ihandle_t *result);
int EXPORT_FUNC(grub_ieee1275_close) (grub_ieee1275_ihandle_t ihandle);
int EXPORT_FUNC(grub_ieee1275_claim) (grub_addr_t addr, grub_size_t size,
				      unsigned int align, grub_addr_t *result);
int EXPORT_FUNC(grub_ieee1275_release) (grub_addr_t addr, grub_size_t size);
int EXPORT_FUNC(grub_ieee1275_set_property) (grub_ieee1275_phandle_t phandle,
					     const char *propname, void *buf,
					     grub_size_t size,
					     grub_ssize_t *actual);
int EXPORT_FUNC(grub_ieee1275_set_color) (grub_ieee1275_ihandle_t ihandle,
					  int index, int r, int g, int b);
int EXPORT_FUNC(grub_ieee1275_milliseconds) (grub_uint32_t *msecs);


grub_err_t EXPORT_FUNC(grub_devalias_iterate)
     (int (*hook) (struct grub_ieee1275_devalias *alias));
grub_err_t EXPORT_FUNC(grub_children_iterate) (char *devpath,
     int (*hook) (struct grub_ieee1275_devalias *alias));
int EXPORT_FUNC(grub_claimmap) (grub_addr_t addr, grub_size_t size);

char *EXPORT_FUNC(grub_ieee1275_encode_devname) (const char *path);
char *EXPORT_FUNC(grub_ieee1275_get_filename) (const char *path);

#endif /* ! GRUB_IEEE1275_HEADER */
