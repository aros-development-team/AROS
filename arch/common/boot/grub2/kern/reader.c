/* reader.c - reader support */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#include <grub/types.h>
#include <grub/mm.h>
#include <grub/reader.h>
#include <grub/parser.h>

struct grub_handler_class grub_reader_class =
  {
    .name = "reader"
  };

grub_err_t
grub_reader_loop (grub_reader_getline_t getline)
{
  while (1)
    {
      char *line;
      grub_reader_getline_t func;

      /* Print an error, if any.  */
      grub_print_error ();
      grub_errno = GRUB_ERR_NONE;

      func = (getline) ? : grub_reader_get_current ()->read_line;
      if ((func (&line, 0)) || (! line))
	return grub_errno;

      grub_parser_get_current ()->parse_line (line, func);
      grub_free (line);
    }
}
