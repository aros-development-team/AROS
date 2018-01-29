/*
 * $Id$
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#include "uboot.h"

/* The version of context_t we hope to got from U-Boot */
#define CALLBACK_VERSION 4

typedef struct context {
	unsigned int c_version;
	void (*c_printf) (const char *fmt, ...);
	int (*c_getc) (void);

	void *c_scan_list;
	list_t *c_devices_list;
	SCAN_HANDLE c_curr_device;

	 SCAN_HANDLE(*c_start_unit_scan) (const void *scan_list,
					  uint32_t * const blocksize);
	 SCAN_HANDLE(*c_next_unit_scan) (SCAN_HANDLE h,
					 unsigned int *const blocksize);
	int (*c_open_specific_unit) (const SCAN_HANDLE h);
	void (*c_end_unit_scan) (SCAN_HANDLE h);
	void (*c_end_global_scan) (void);
	int (*c_loadsector) (const unsigned int sectn,
			     const unsigned int sect_size,
			     const unsigned int numb_sects,
			     void *const dest_buf);

	int (*c_my_netloop) (char *filename, void *dump_here);

	char *(*c_getenv) (unsigned char *);
	void (*c_setenv) (char *, char *);

	void *(*c_alloc_mem_for_iobuffers) (const int size);
	void *(*c_alloc_mem_for_kickmodule) (const int size);
	void *(*c_alloc_mem_for_execNG) (const int size);
	void *(*c_alloc_mem_for_anythingelse) (const int size);
	void *(*c_alloc_mem_for_bootloader) (const int size);
	void (*c_free_mem) (void *const loc);

	void *(*c_get_board_info) (void);
	int (*c_BZ2_bzBuffToBuffDecompress) (char *dest,
					     unsigned int *destLen,
					     char *source,
					     unsigned int sourceLen,
					     int small, int verbosity);

	void (*c_video_clear) (void);
	void (*c_video_draw_box) (int style, int attr, char *title,
				  int separate, int x, int y, int w, int h);
	void (*c_video_draw_text) (int x, int y, int attr, char *text,
				   int field);
	void (*c_video_repeat_char) (int x, int y, int repcnt, int repchar,
				     int attr);

	unsigned short (*c_set_partial_scroll_limits) (const short start,
						       const short end);
	void (*c_get_partial_scroll_limits) (short *const start,
					     short *const end);
	int (*c_video_get_key) (void);

	int (*c_do_bootm) (cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
	void *(*c_memmove) (void *dest, const void *src, int count);
	void (*c_set_load_addr) (void *const la);

	int (*c_tstc) (void);
	void (*c_udelay) (unsigned long);
	int (*c_sprintf) (char *buf, const char *fmt, ...);

	int (*c_ext2fs_set_blk_dev_full) (block_dev_desc_t * const rbdd,
					  disk_partition_t * const p);
	int (*c_ext2fs_open) (char *filename);
	int (*c_ext2fs_read) (char *buf, unsigned len);
	int (*c_ext2fs_mount) (unsigned part_length);
	int (*c_ext2fs_close) (void);
	int (*c_bootu) (char *device_str);
} context_t;

void context_init(context_t * ctx);
inline context_t *context_get(void);

/* terminal IO functions */
#define printf(FMT, ARGS...) context_get()->c_printf((FMT), ##ARGS)
inline int getc(void);

/* devices functions */
inline void *get_scan_list(void);
inline list_t *get_devices_list(void);
inline SCAN_HANDLE get_curr_device(void);
inline SCAN_HANDLE start_unit_scan(const void *scan_list,
				   uint32_t * const blocksize);
inline SCAN_HANDLE next_unit_scan(SCAN_HANDLE h, unsigned int *const blocksize);
inline int open_specific_unit(const SCAN_HANDLE h);
inline void end_unit_scan(SCAN_HANDLE h);
inline void end_global_scan(void);
inline int loadsector(const unsigned int sectn,
		      const unsigned int sect_size,
		      const unsigned int numb_sects, void *const dest_buf);
inline int netloop(char *filename, void *dump_here);

/* memory functions */
inline void *malloc(int size);
inline void free(void *ptr);
inline void *memmove(void *dest, const void *src, int count);

/* ENV functions */
inline char *getenv(unsigned char *var);
inline void setenv(char *var, char *value);

/* misc functions */
inline int tstc(void);
inline void udelay(unsigned long t);
#define sprintf(BUF, FMT, ARGS...) context_get()->c_sprintf((BUF), (FMT), ##ARGS)

/* video functions */
inline void video_clear(void);
inline void video_draw_box(int style, int attr, char *title, int separate,
			   int x, int y, int w, int h);
inline void video_draw_text(int x, int y, int attr, char *text, int field);
inline void video_repeat_char(int x, int y, int repcnt, int repchar, int attr);
inline unsigned short video_set_partial_scroll_limits(const short start,
						      const short end);
inline void video_get_partial_scroll_limits(short *const start,
					    short *const end);
inline int video_get_key(void);
inline int video_display_bitmap(unsigned long, int, int);

/* ext2fs functions */
inline int ext2fs_set_blk_dev_full(block_dev_desc_t * const rbdd,
				   disk_partition_t * const p);
inline int ext2fs_open(char *filename);
inline int ext2fs_read(char *buf, unsigned len);
inline int ext2fs_mount(unsigned part_length);
inline int ext2fs_close(void);

/* booting functions */
inline int bootm(cmd_tbl_t * cmdtp, int flag, int argc, char *argv[]);
inline void set_load_addr(void *const la);
inline int bootu(char *device);

#endif /*CONTEXT_H_ */
