/* editenv.c - tool to edit environment block.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009,2010,2013 Free Software Foundation, Inc.
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
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/util/install.h>
#include <grub/lib/envblk.h>
#include <grub/i18n.h>
#include <grub/emu/hostfile.h>

#include <errno.h>
#include <string.h>

#define DEFAULT_ENVBLK_SIZE	1024

void
grub_util_create_envblk_file (const char *name)
{
  FILE *fp;
  char *buf;
  char *namenew;

  buf = xmalloc (DEFAULT_ENVBLK_SIZE);

  namenew = xasprintf ("%s.new", name);
  fp = grub_util_fopen (namenew, "wb");
  if (! fp)
    grub_util_error (_("cannot open `%s': %s"), namenew,
		     strerror (errno));

  memcpy (buf, GRUB_ENVBLK_SIGNATURE, sizeof (GRUB_ENVBLK_SIGNATURE) - 1);
  memset (buf + sizeof (GRUB_ENVBLK_SIGNATURE) - 1, '#',
          DEFAULT_ENVBLK_SIZE - sizeof (GRUB_ENVBLK_SIGNATURE) + 1);

  if (fwrite (buf, 1, DEFAULT_ENVBLK_SIZE, fp) != DEFAULT_ENVBLK_SIZE)
    grub_util_error (_("cannot write to `%s': %s"), namenew,
		     strerror (errno));


  grub_util_file_sync (fp);
  free (buf);
  fclose (fp);

  if (grub_util_rename (namenew, name) < 0)
    grub_util_error (_("cannot rename the file %s to %s"), namenew, name);
  free (namenew);
}
