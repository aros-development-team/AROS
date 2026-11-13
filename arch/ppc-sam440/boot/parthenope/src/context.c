/* context.c */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "context.h"

static context_t *context = NULL;

void context_init(context_t * ctx)
{
	context = ctx;
}

inline context_t *context_get(void)
{
	return context;
}

/* terminal IO functions */
inline int getc(void)
{
	return context->c_getc();
}

/* bogus uboot */
static char *scan_list[7] = {
	"s4sii",
	"s4siicdrom",
	"net",
	"ssii",
	"ssiicdrom",
	NULL,
	NULL
};

/* devices functions */
inline void *get_scan_list(void)
{
	if (strncmp(((char**)context->c_scan_list)[0], "boot2", 5) == 0)
		return scan_list;
 	return context->c_scan_list;
}

inline list_t *get_devices_list(void)
{
	return context->c_devices_list;
}

inline SCAN_HANDLE get_curr_device(void)
{
	return context->c_curr_device;
}

inline SCAN_HANDLE start_unit_scan(const void *scan_list,
				   uint32_t * const blocksize)
{
	return context->c_start_unit_scan(scan_list, blocksize);
}

inline SCAN_HANDLE next_unit_scan(SCAN_HANDLE h, unsigned int *const blocksize)
{
	return context->c_next_unit_scan(h, blocksize);
}

inline int open_specific_unit(const SCAN_HANDLE h)
{
	return context->c_open_specific_unit(h);
}

inline void end_unit_scan(SCAN_HANDLE h)
{
	return context->c_end_unit_scan(h);
}

inline void end_global_scan(void)
{
	return context->c_end_global_scan();
}

inline int loadsector(const unsigned int sectn, const unsigned int sect_size,
		      const unsigned int numb_sects, void *const dest_buf)
{
	return context->c_loadsector(sectn, sect_size, numb_sects, dest_buf);
}

inline int netloop(char *filename, void *dump_here)
{
	return context->c_my_netloop(filename, dump_here);
}

/* memory functions */
inline void *malloc(int size)
{
	return context->c_alloc_mem_for_anythingelse(size);
}

inline void free(void *ptr)
{
	return context->c_free_mem(ptr);
}

inline void *memmove(void *dest, const void *src, int count)
{
	return context->c_memmove(dest, src, count);
}

/* ENV functions */
inline char *getenv(char *var)
{
	return context->c_getenv(var);
}

inline void setenv(char *var, char *value)
{
	return context->c_setenv(var, value);
}

/* misc functions */
inline int tstc(void)
{
	return context->c_tstc();
}

inline void udelay(unsigned long t)
{
	return context->c_udelay(t);
}

/* video functions */
inline void video_clear(void)
{
	return context->c_video_clear();
}

inline void video_draw_box(int style, int attr, char *title, int separate,
			   int x, int y, int w, int h)
{
	return context->c_video_draw_box(style, attr, title, separate, x, y,
					 w, h);
}

inline void video_draw_text(int x, int y, int attr, char *text, int field)
{
	return context->c_video_draw_text(x, y, attr, text, field);
}

inline void video_repeat_char(int x, int y, int repcnt, int repchar, int attr)
{
	return context->c_video_repeat_char(x, y, repcnt, repchar, attr);
}

inline unsigned short video_set_partial_scroll_limits(const short start,
						      const short end)
{
	return context->c_set_partial_scroll_limits(start, end);
}

inline void video_get_partial_scroll_limits(short *const start,
					    short *const end)
{
	return context->c_get_partial_scroll_limits(start, end);
}

inline int video_get_key(void)
{
	return context->c_video_get_key();
}

/* ext2fs functions */
inline int ext2fs_set_blk_dev_full(block_dev_desc_t * const rbdd,
				   disk_partition_t * const p)
{
	return context->c_ext2fs_set_blk_dev_full(rbdd, p);
}

inline int ext2fs_open(char *filename)
{
	return context->c_ext2fs_open(filename);
}

inline int ext2fs_read(char *buf, unsigned len)
{
	return context->c_ext2fs_read(buf, len);
}

inline int ext2fs_mount(unsigned part_length)
{
	return context->c_ext2fs_mount(part_length);
}

inline int ext2fs_close(void)
{
	return context->c_ext2fs_close();
}

/* booting functions */
inline int bootm(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[])
{
	return context->c_do_bootm(cmdtp, flag, argc, argv);
}

inline void set_load_addr(void *const la)
{
	return context->c_set_load_addr(la);
}

inline int bootu(char *device)
{
	setenv("stdout", "vga");
	setenv("boot1", device);
	return context->c_bootu(device);
}
