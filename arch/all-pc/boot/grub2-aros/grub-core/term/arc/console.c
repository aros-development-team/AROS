/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2011  Free Software Foundation, Inc.
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

#include <grub/arc/arc.h>
#include <grub/arc/console.h>
#include <grub/term.h>
#include <grub/terminfo.h>

/* FIXME: use unicode.  */

static int
readkey (struct grub_term_input *term __attribute__ ((unused)))
{
  unsigned long count;
  char chr;

  if (GRUB_ARC_FIRMWARE_VECTOR->get_read_status (GRUB_ARC_STDIN))
    return -1;

  if (GRUB_ARC_FIRMWARE_VECTOR->read (GRUB_ARC_STDIN, &chr, 1, &count))
    return -1;
  if (!count)
    return -1;
  return chr;
}

static void
put (struct grub_term_output *term __attribute__ ((unused)), const int c)
{
  unsigned long count;
  char chr = c;

  GRUB_ARC_FIRMWARE_VECTOR->write (GRUB_ARC_STDOUT, &chr, 1, &count);
}

static struct grub_terminfo_output_state grub_console_terminfo_output;

static grub_err_t
grub_console_init_output (struct grub_term_output *term)
{
  struct grub_arc_display_status *info = NULL;
  if (GRUB_ARC_SYSTEM_PARAMETER_BLOCK->firmware_vector_length
      >= ((char *) (&GRUB_ARC_FIRMWARE_VECTOR->getdisplaystatus + 1)
	  - (char *) GRUB_ARC_FIRMWARE_VECTOR)
      && GRUB_ARC_FIRMWARE_VECTOR->getdisplaystatus)
    info = GRUB_ARC_FIRMWARE_VECTOR->getdisplaystatus (GRUB_ARC_STDOUT);
  if (info)
    {
      grub_console_terminfo_output.width = info->w + 1;
      grub_console_terminfo_output.height = info->h + 1;
    }
  grub_terminfo_output_init (term);

  return 0;
}

static struct grub_terminfo_input_state grub_console_terminfo_input =
  {
    .readkey = readkey
  };

static struct grub_terminfo_output_state grub_console_terminfo_output =
  {
    .put = put,
    .width = 80,
    .height = 20
  };

static struct grub_term_input grub_console_term_input =
  {
    .name = "console",
    .init = grub_terminfo_input_init,
    .getkey = grub_terminfo_getkey,
    .data = &grub_console_terminfo_input
  };

static struct grub_term_output grub_console_term_output =
  {
    .name = "console",
    .init = grub_console_init_output,
    .putchar = grub_terminfo_putchar,
    .getxy = grub_terminfo_getxy,
    .getwh = grub_terminfo_getwh,
    .gotoxy = grub_terminfo_gotoxy,
    .cls = grub_terminfo_cls,
    .setcolorstate = grub_terminfo_setcolorstate,
    .setcursor = grub_terminfo_setcursor,
    .flags = GRUB_TERM_CODE_TYPE_ASCII,
    .data = &grub_console_terminfo_output,
    .normal_color = GRUB_TERM_DEFAULT_NORMAL_COLOR,
    .highlight_color = GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR,
  };

void
grub_console_init_early (void)
{
  grub_term_register_input ("console", &grub_console_term_input);
  grub_term_register_output ("console", &grub_console_term_output);
}

void
grub_console_init_lately (void)
{
  grub_terminfo_init ();
  grub_terminfo_output_register (&grub_console_term_output, "arc");
}
