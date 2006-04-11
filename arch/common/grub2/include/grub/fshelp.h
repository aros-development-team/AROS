/* fshelp.h -- Filesystem helper functions */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2004  Free Software Foundation, Inc.
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

#ifndef GRUB_FSHELP_HEADER
#define GRUB_FSHELP_HEADER	1

#include <grub/types.h>
#include <grub/symbol.h>
#include <grub/err.h>

typedef struct grub_fshelp_node *grub_fshelp_node_t;

enum grub_fshelp_filetype
  {
    GRUB_FSHELP_UNKNOWN,
    GRUB_FSHELP_REG,
    GRUB_FSHELP_DIR,
    GRUB_FSHELP_SYMLINK
  };

/* Lookup the node PATH.  The node ROOTNODE describes the root of the
   directory tree.  The node found is returned in FOUNDNODE, which is
   either a ROOTNODE or a new malloc'ed node.  ITERATE_DIR is used to
   iterate over all directory entries in the current node.
   READ_SYMLINK is used to read the symlink if a node is a symlink.
   EXPECTTYPE is the type node that is expected by the called, an
   error is generated if the node is not of the expected type.  Make
   sure you use the NESTED_FUNC_ATTR macro for HOOK, this is required
   because GCC has a nasty bug when using regparm=3.  */
grub_err_t
EXPORT_FUNC(grub_fshelp_find_file) (const char *path,
				    grub_fshelp_node_t rootnode,
				    grub_fshelp_node_t *foundnode,
				    int (*iterate_dir) (grub_fshelp_node_t dir,
							int NESTED_FUNC_ATTR
							(*hook) (const char *filename,
								 enum grub_fshelp_filetype filetype,
								 grub_fshelp_node_t node)),
				    char *(*read_symlink) (grub_fshelp_node_t node),
				    enum grub_fshelp_filetype expect);


/* Read LEN bytes from the file NODE on disk DISK into the buffer BUF,
   beginning with the block POS.  READ_HOOK should be set before
   reading a block from the file.  GET_BLOCK is used to translate file
   blocks to disk blocks.  The file is FILESIZE bytes big and the
   blocks have a size of LOG2BLOCKSIZE (in log2).  */
grub_ssize_t
EXPORT_FUNC(grub_fshelp_read_file) (grub_disk_t disk, grub_fshelp_node_t node,
				    void (*read_hook) (unsigned long sector,
						       unsigned offset,
						       unsigned length),
				    int pos, unsigned int len, char *buf,
				    int (*get_block) (grub_fshelp_node_t node,
						      int block),
				    unsigned int filesize, int log2blocksize);
     
#endif /* ! GRUB_FSHELP_HEADER */
