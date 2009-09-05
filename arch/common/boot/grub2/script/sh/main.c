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

#include <grub/dl.h>
#include <grub/parser.h>
#include <grub/script_sh.h>

static grub_err_t
grub_normal_parse_line (char *line, grub_reader_getline_t getline)
{
  struct grub_script *parsed_script;

  /* Parse the script.  */
  parsed_script = grub_script_parse (line, getline);

  if (parsed_script)
    {
      /* Execute the command(s).  */
      grub_script_execute (parsed_script);

      /* The parsed script was executed, throw it away.  */
      grub_script_free (parsed_script);
    }

  return grub_errno;
}

static struct grub_parser grub_sh_parser =
  {
    .name = "sh",
    .parse_line = grub_normal_parse_line
  };

GRUB_MOD_INIT(sh)
{
  grub_parser_register ("sh", &grub_sh_parser);
}

GRUB_MOD_FINI(sh)
{
  grub_parser_unregister (&grub_sh_parser);
}
