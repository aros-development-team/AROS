/* get_disk_name.c */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007 Free Software Foundation, Inc.
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
#include <grub/util/misc.h>

char *
grub_util_get_disk_name (int disk __attribute__ ((unused)), char *name)
{
  int p[2];

  char *line = NULL;
  size_t zero = 0;
  int len;

  pipe (p);

  switch (fork ())
    {
    case -1:
      perror ("fork");
      exit (1);
    case 0:
      close (1);
      dup (p[1]);
      close (p[0]);
      close (p[1]);
      execlp ("ofpathname", "ofpathname", "-a", name, NULL);
      perror ("execlp");
    default:
      close (0);
      dup (p[0]);
      close (p[0]);
      close (p[1]);
    }

  len = getline (&line, &zero, stdin);
  if (len < 2)
    grub_util_error ("ofpathname didn't print a meaningful alias name");

  line[len - 1] = '\0';
  
  return line;
}
