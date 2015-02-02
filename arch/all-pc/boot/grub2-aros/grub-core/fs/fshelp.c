/* fshelp.c -- Filesystem helper functions */
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

#include <grub/err.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/disk.h>
#include <grub/fshelp.h>
#include <grub/dl.h>
#include <grub/i18n.h>

GRUB_MOD_LICENSE ("GPLv3+");

typedef int (*iterate_dir_func) (grub_fshelp_node_t dir,
				 grub_fshelp_iterate_dir_hook_t hook,
				 void *data);
typedef char *(*read_symlink_func) (grub_fshelp_node_t node);

/* Context for grub_fshelp_find_file.  */
struct grub_fshelp_find_file_ctx
{
  const char *path;
  grub_fshelp_node_t rootnode, currroot, currnode, oldnode;
  enum grub_fshelp_filetype foundtype;
  int symlinknest;
  const char *name;
  const char *next;
  enum grub_fshelp_filetype type;
};

/* Helper for find_file_iter.  */
static void
free_node (grub_fshelp_node_t node, struct grub_fshelp_find_file_ctx *ctx)
{
  if (node != ctx->rootnode && node != ctx->currroot)
    grub_free (node);
}

/* Helper for grub_fshelp_find_file.  */
static int
find_file_iter (const char *filename, enum grub_fshelp_filetype filetype,
		grub_fshelp_node_t node, void *data)
{
  struct grub_fshelp_find_file_ctx *ctx = data;

  if (filetype == GRUB_FSHELP_UNKNOWN ||
      ((filetype & GRUB_FSHELP_CASE_INSENSITIVE)
       ? grub_strncasecmp (ctx->name, filename, ctx->next - ctx->name)
       : grub_strncmp (ctx->name, filename, ctx->next - ctx->name))
      || filename[ctx->next - ctx->name])
    {
      grub_free (node);
      return 0;
    }

  /* The node is found, stop iterating over the nodes.  */
  ctx->type = filetype & ~GRUB_FSHELP_CASE_INSENSITIVE;
  ctx->oldnode = ctx->currnode;
  ctx->currnode = node;

  return 1;
}

static grub_err_t
find_file (const char *currpath, grub_fshelp_node_t currroot,
	   grub_fshelp_node_t *currfound,
	   iterate_dir_func iterate_dir, read_symlink_func read_symlink,
	   struct grub_fshelp_find_file_ctx *ctx)
{
  ctx->currroot = currroot;
  ctx->name = currpath;
  ctx->type = GRUB_FSHELP_DIR;
  ctx->currnode = currroot;
  ctx->oldnode = currroot;

  for (;;)
    {
      int found;

      /* Remove all leading slashes.  */
      while (*ctx->name == '/')
	ctx->name++;

      /* Found the node!  */
      if (! *ctx->name)
	{
	  *currfound = ctx->currnode;
	  ctx->foundtype = ctx->type;
	  return 0;
	}

      /* Extract the actual part from the pathname.  */
      for (ctx->next = ctx->name; *ctx->next && *ctx->next != '/'; ctx->next++);

      /* At this point it is expected that the current node is a
	 directory, check if this is true.  */
      if (ctx->type != GRUB_FSHELP_DIR)
	{
	  free_node (ctx->currnode, ctx);
	  ctx->currnode = 0;
	  return grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("not a directory"));
	}

      /* Iterate over the directory.  */
      found = iterate_dir (ctx->currnode, find_file_iter, ctx);
      if (! found)
	{
	  free_node (ctx->currnode, ctx);
	  ctx->currnode = 0;
	  if (grub_errno)
	    return grub_errno;

	  break;
	}

      /* Read in the symlink and follow it.  */
      if (ctx->type == GRUB_FSHELP_SYMLINK)
	{
	  char *symlink;
	  const char *next;

	  /* Test if the symlink does not loop.  */
	  if (++ctx->symlinknest == 8)
	    {
	      free_node (ctx->currnode, ctx);
	      free_node (ctx->oldnode, ctx);
	      ctx->currnode = 0;
	      ctx->oldnode = 0;
	      return grub_error (GRUB_ERR_SYMLINK_LOOP,
				 N_("too deep nesting of symlinks"));
	    }

	  symlink = read_symlink (ctx->currnode);
	  free_node (ctx->currnode, ctx);
	  ctx->currnode = 0;

	  if (!symlink)
	    {
	      free_node (ctx->oldnode, ctx);
	      ctx->oldnode = 0;
	      return grub_errno;
	    }

	  /* The symlink is an absolute path, go back to the root inode.  */
	  if (symlink[0] == '/')
	    {
	      free_node (ctx->oldnode, ctx);
	      ctx->oldnode = ctx->rootnode;
	    }

	  /* Lookup the node the symlink points to.  */
	  next = ctx->next;
	  find_file (symlink, ctx->oldnode, &ctx->currnode,
		     iterate_dir, read_symlink, ctx);
	  ctx->next = next;
	  ctx->type = ctx->foundtype;
	  grub_free (symlink);

	  if (grub_errno)
	    {
	      free_node (ctx->oldnode, ctx);
	      ctx->oldnode = 0;
	      return grub_errno;
	    }
	}

      if (ctx->oldnode != ctx->currnode)
	{
	  free_node (ctx->oldnode, ctx);
	  ctx->oldnode = 0;
	}

      ctx->name = ctx->next;
    }

  return grub_error (GRUB_ERR_FILE_NOT_FOUND, N_("file `%s' not found"),
		     ctx->path);
}

/* Lookup the node PATH.  The node ROOTNODE describes the root of the
   directory tree.  The node found is returned in FOUNDNODE, which is
   either a ROOTNODE or a new malloc'ed node.  ITERATE_DIR is used to
   iterate over all directory entries in the current node.
   READ_SYMLINK is used to read the symlink if a node is a symlink.
   EXPECTTYPE is the type node that is expected by the called, an
   error is generated if the node is not of the expected type.  */
grub_err_t
grub_fshelp_find_file (const char *path, grub_fshelp_node_t rootnode,
		       grub_fshelp_node_t *foundnode,
		       iterate_dir_func iterate_dir,
		       read_symlink_func read_symlink,
		       enum grub_fshelp_filetype expecttype)
{
  struct grub_fshelp_find_file_ctx ctx = {
    .path = path,
    .rootnode = rootnode,
    .foundtype = GRUB_FSHELP_DIR,
    .symlinknest = 0
  };
  grub_err_t err;

  if (!path || path[0] != '/')
    {
      grub_error (GRUB_ERR_BAD_FILENAME, N_("invalid file name `%s'"), path);
      return grub_errno;
    }

  err = find_file (path, rootnode, foundnode, iterate_dir, read_symlink, &ctx);
  if (err)
    return err;

  /* Check if the node that was found was of the expected type.  */
  if (expecttype == GRUB_FSHELP_REG && ctx.foundtype != expecttype)
    return grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("not a regular file"));
  else if (expecttype == GRUB_FSHELP_DIR && ctx.foundtype != expecttype)
    return grub_error (GRUB_ERR_BAD_FILE_TYPE, N_("not a directory"));

  return 0;
}

/* Read LEN bytes from the file NODE on disk DISK into the buffer BUF,
   beginning with the block POS.  READ_HOOK should be set before
   reading a block from the file.  READ_HOOK_DATA is passed through as
   the DATA argument to READ_HOOK.  GET_BLOCK is used to translate
   file blocks to disk blocks.  The file is FILESIZE bytes big and the
   blocks have a size of LOG2BLOCKSIZE (in log2).  */
grub_ssize_t
grub_fshelp_read_file (grub_disk_t disk, grub_fshelp_node_t node,
		       grub_disk_read_hook_t read_hook, void *read_hook_data,
		       grub_off_t pos, grub_size_t len, char *buf,
		       grub_disk_addr_t (*get_block) (grub_fshelp_node_t node,
                                                      grub_disk_addr_t block),
		       grub_off_t filesize, int log2blocksize,
		       grub_disk_addr_t blocks_start)
{
  grub_disk_addr_t i, blockcnt;
  int blocksize = 1 << (log2blocksize + GRUB_DISK_SECTOR_BITS);

  if (pos > filesize)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE,
		  N_("attempt to read past the end of file"));
      return -1;
    }

  /* Adjust LEN so it we can't read past the end of the file.  */
  if (pos + len > filesize)
    len = filesize - pos;

  blockcnt = ((len + pos) + blocksize - 1) >> (log2blocksize + GRUB_DISK_SECTOR_BITS);

  for (i = pos >> (log2blocksize + GRUB_DISK_SECTOR_BITS); i < blockcnt; i++)
    {
      grub_disk_addr_t blknr;
      int blockoff = pos & (blocksize - 1);
      int blockend = blocksize;

      int skipfirst = 0;

      blknr = get_block (node, i);
      if (grub_errno)
	return -1;

      blknr = blknr << log2blocksize;

      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) & (blocksize - 1);

	  /* The last portion is exactly blocksize.  */
	  if (! blockend)
	    blockend = blocksize;
	}

      /* First block.  */
      if (i == (pos >> (log2blocksize + GRUB_DISK_SECTOR_BITS)))
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      /* If the block number is 0 this block is not stored on disk but
	 is zero filled instead.  */
      if (blknr)
	{
	  disk->read_hook = read_hook;
	  disk->read_hook_data = read_hook_data;

	  grub_disk_read (disk, blknr + blocks_start, skipfirst,
			  blockend, buf);
	  disk->read_hook = 0;
	  if (grub_errno)
	    return -1;
	}
      else
	grub_memset (buf, 0, blockend);

      buf += blocksize - skipfirst;
    }

  return len;
}
