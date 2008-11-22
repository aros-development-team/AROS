/* bufio.c - buffered io access */
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

#include <grub/err.h>
#include <grub/types.h>
#include <grub/mm.h>
#include <grub/misc.h>
#include <grub/fs.h>
#include <grub/bufio.h>

#define GRUB_BUFIO_DEF_SIZE	8192
#define GRUB_BUFIO_MAX_SIZE	1048576

struct grub_bufio
{
  grub_file_t file;
  grub_size_t block_size;
  grub_size_t buffer_len;
  char buffer[0];
};
typedef struct grub_bufio *grub_bufio_t;

static struct grub_fs grub_bufio_fs;

grub_file_t
grub_bufio_open (grub_file_t io, int size)
{
  grub_file_t file;
  grub_bufio_t bufio = 0;

  file = (grub_file_t) grub_malloc (sizeof (*file));
  if (! file)
    return 0;

  if (size == 0)
    size = GRUB_BUFIO_DEF_SIZE;
  else if (size > GRUB_BUFIO_MAX_SIZE)
    size = GRUB_BUFIO_MAX_SIZE;

  if ((size < 0) || ((unsigned) size > io->size))
    size = ((io->size > GRUB_BUFIO_MAX_SIZE) ? GRUB_BUFIO_MAX_SIZE :
            io->size);

  bufio = grub_malloc (sizeof (struct grub_bufio) + size);
  if (! bufio)
    {
      grub_free (file);
      return 0;
    }

  bufio->file = io;
  bufio->block_size = size;
  bufio->buffer_len = 0;

  file->device = io->device;
  file->offset = 0;
  file->size = io->size;
  file->data = bufio;
  file->read_hook = 0;
  file->fs = &grub_bufio_fs;

  return file;
}

grub_file_t
grub_buffile_open (const char *name, int size)
{
  grub_file_t io, file;

  io = grub_file_open (name);
  if (! io)
    return 0;

  file = grub_bufio_open (io, size);
  if (! file)
    {
      grub_file_close (io);
      return 0;
    }

  return file;
}

static grub_ssize_t
grub_bufio_read (grub_file_t file, char *buf, grub_size_t len)
{
  grub_size_t res = len;
  grub_bufio_t bufio = file->data;
  grub_uint32_t pos;

  if ((file->offset >= bufio->file->offset) &&
      (file->offset < bufio->file->offset + bufio->buffer_len))
    {
      grub_size_t n;

      pos = file->offset - bufio->file->offset;
      n = bufio->buffer_len - pos;
      if (n > len)
        n = len;

      grub_memcpy (buf, &bufio->buffer[pos], n);
      len -= n;
      if (! len)
        return res;

      buf += n;
      bufio->file->offset += bufio->buffer_len;
      pos = 0;
    }
  else
    {
      bufio->file->offset = grub_divmod64 (file->offset, bufio->block_size,
                                           &pos);
      bufio->file->offset *= bufio->block_size;
    }

  if (pos + len >= bufio->block_size)
    {
      if (pos)
        {
          grub_size_t n;

          bufio->file->fs->read (bufio->file, bufio->buffer,
                                 bufio->block_size);
          if (grub_errno)
            return -1;

          n = bufio->block_size - pos;
          grub_memcpy (buf, &bufio->buffer[pos], n);
          len -= n;
          buf += n;
          bufio->file->offset += bufio->block_size;
          pos = 0;
        }

      while (len >= bufio->block_size)
        {
          bufio->file->fs->read (bufio->file, buf, bufio->block_size);
          if (grub_errno)
            return -1;

          len -= bufio->block_size;
          buf += bufio->block_size;
          bufio->file->offset += bufio->block_size;
        }

      if (! len)
        {
          bufio->buffer_len = 0;
          return res;
        }
    }

  bufio->buffer_len = bufio->file->size - bufio->file->offset;
  if (bufio->buffer_len > bufio->block_size)
    bufio->buffer_len = bufio->block_size;

  bufio->file->fs->read (bufio->file, bufio->buffer, bufio->buffer_len);
  if (grub_errno)
    return -1;

  grub_memcpy (buf, &bufio->buffer[pos], len);

  return res;
}

static grub_err_t
grub_bufio_close (grub_file_t file)
{
  grub_bufio_t bufio = file->data;

  grub_file_close (bufio->file);
  grub_free (bufio);

  file->device = 0;

  return grub_errno;
}

static struct grub_fs grub_bufio_fs =
  {
    .name = "bufio",
    .dir = 0,
    .open = 0,
    .read = grub_bufio_read,
    .close = grub_bufio_close,
    .label = 0,
    .next = 0
  };
