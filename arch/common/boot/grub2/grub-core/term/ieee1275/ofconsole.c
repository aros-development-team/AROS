/*  ofconsole.c -- Open Firmware console for GRUB.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2003,2004,2005,2007,2008,2009  Free Software Foundation, Inc.
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

#include <grub/term.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/time.h>
#include <grub/terminfo.h>
#include <grub/ieee1275/console.h>
#include <grub/ieee1275/ieee1275.h>

static grub_ieee1275_ihandle_t stdout_ihandle;
static grub_ieee1275_ihandle_t stdin_ihandle;

extern struct grub_terminfo_output_state grub_ofconsole_terminfo_output;

struct color
{
  int red;
  int green;
  int blue;
};

/* Use serial colors as they are default on most firmwares and some firmwares
   ignore set-color!. Additionally output may be redirected to serial.  */
static struct color colors[] =
  {
    // {R, G, B}
    {0x00, 0x00, 0x00}, // 0 = black
    {0xA8, 0x00, 0x00}, // 1 = red
    {0x00, 0xA8, 0x00}, // 2 = green
    {0xFE, 0xFE, 0x54}, // 3 = yellow
    {0x00, 0x00, 0xA8}, // 4 = blue
    {0xA8, 0x00, 0xA8}, // 5 = magenta
    {0x00, 0xA8, 0xA8}, // 6 = cyan
    {0xFE, 0xFE, 0xFE}  // 7 = white
  };

static void
put (struct grub_term_output *term __attribute__ ((unused)), const int c)
{
  char chr = c;

  grub_ieee1275_write (stdout_ihandle, &chr, 1, 0);
}

static int
readkey (struct grub_term_input *term __attribute__ ((unused)))
{
  grub_uint8_t c;
  grub_ssize_t actual = 0;

  grub_ieee1275_read (stdin_ihandle, &c, 1, &actual);
  if (actual > 0)
    return c;
  return -1;
}

static void
grub_ofconsole_dimensions (void)
{
  grub_ieee1275_ihandle_t options;
  grub_ssize_t lval;

  if (! grub_ieee1275_finddevice ("/options", &options)
      && options != (grub_ieee1275_ihandle_t) -1)
    {
      if (! grub_ieee1275_get_property_length (options, "screen-#columns",
					       &lval)
	  && lval >= 0 && lval < 1024)
	{
	  char val[lval];

	  if (! grub_ieee1275_get_property (options, "screen-#columns",
					    val, lval, 0))
	    grub_ofconsole_terminfo_output.width
	      = (grub_uint8_t) grub_strtoul (val, 0, 10);
	}
      if (! grub_ieee1275_get_property_length (options, "screen-#rows", &lval)
	  && lval >= 0 && lval < 1024)
	{
	  char val[lval];
	  if (! grub_ieee1275_get_property (options, "screen-#rows",
					    val, lval, 0))
	    grub_ofconsole_terminfo_output.height
	      = (grub_uint8_t) grub_strtoul (val, 0, 10);
	}
    }

  /* Use a small console by default.  */
  if (! grub_ofconsole_terminfo_output.width)
    grub_ofconsole_terminfo_output.width = 80;
  if (! grub_ofconsole_terminfo_output.height)
    grub_ofconsole_terminfo_output.height = 24;
}

static void
grub_ofconsole_setcursor (struct grub_term_output *term,
			  int on)
{
  grub_terminfo_setcursor (term, on);

  if (!grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_HAS_CURSORONOFF))
    return;

  /* Understood by the Open Firmware flavour in OLPC.  */
  if (on)
    grub_ieee1275_interpret ("cursor-on", 0);
  else
    grub_ieee1275_interpret ("cursor-off", 0);
}

static grub_err_t
grub_ofconsole_init_input (struct grub_term_input *term)
{
  grub_ssize_t actual;

  if (grub_ieee1275_get_integer_property (grub_ieee1275_chosen, "stdin", &stdin_ihandle,
					  sizeof stdin_ihandle, &actual)
      || actual != sizeof stdin_ihandle)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "cannot find stdin");

  return grub_terminfo_input_init (term);
}

static grub_err_t
grub_ofconsole_init_output (struct grub_term_output *term)
{
  grub_ssize_t actual;

  /* The latest PowerMacs don't actually initialize the screen for us, so we
   * use this trick to re-open the output device (but we avoid doing this on
   * platforms where it's known to be broken). */
  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_BROKEN_OUTPUT))
    grub_ieee1275_interpret ("output-device output", 0);

  if (grub_ieee1275_get_integer_property (grub_ieee1275_chosen, "stdout", &stdout_ihandle,
					  sizeof stdout_ihandle, &actual)
      || actual != sizeof stdout_ihandle)
    return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "cannot find stdout");

  /* Initialize colors.  */
  if (! grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_CANNOT_SET_COLORS))
    {
      unsigned col;
      for (col = 0; col < ARRAY_SIZE (colors); col++)
	grub_ieee1275_set_color (stdout_ihandle, col, colors[col].red,
				 colors[col].green, colors[col].blue);

      /* Set the right fg and bg colors.  */
      grub_terminfo_setcolorstate (term, GRUB_TERM_COLOR_NORMAL);
    }

  grub_ofconsole_dimensions ();

  grub_terminfo_output_init (term);

  return 0;
}



struct grub_terminfo_input_state grub_ofconsole_terminfo_input =
  {
    .readkey = readkey
  };

struct grub_terminfo_output_state grub_ofconsole_terminfo_output =
  {
    .put = put,
    .width = 80,
    .height = 24
  };

static struct grub_term_input grub_ofconsole_term_input =
  {
    .name = "ofconsole",
    .init = grub_ofconsole_init_input,
    .getkey = grub_terminfo_getkey,
    .data = &grub_ofconsole_terminfo_input
  };

static struct grub_term_output grub_ofconsole_term_output =
  {
    .name = "ofconsole",
    .init = grub_ofconsole_init_output,
    .putchar = grub_terminfo_putchar,
    .getxy = grub_terminfo_getxy,
    .getwh = grub_terminfo_getwh,
    .gotoxy = grub_terminfo_gotoxy,
    .cls = grub_terminfo_cls,
    .setcolorstate = grub_terminfo_setcolorstate,
    .setcursor = grub_ofconsole_setcursor,
    .flags = GRUB_TERM_CODE_TYPE_ASCII,
    .data = &grub_ofconsole_terminfo_output,
    .normal_color = GRUB_TERM_DEFAULT_NORMAL_COLOR,
    .highlight_color = GRUB_TERM_DEFAULT_HIGHLIGHT_COLOR,
  };

void grub_terminfo_fini (void);
void grub_terminfo_init (void);

void
grub_console_init_early (void)
{
  grub_term_register_input ("ofconsole", &grub_ofconsole_term_input);
  grub_term_register_output ("ofconsole", &grub_ofconsole_term_output);
}

void
grub_console_init_lately (void)
{
  const char *type;

  if (grub_ieee1275_test_flag (GRUB_IEEE1275_FLAG_NO_ANSI))
    type = "dumb";
  else
    type = "ieee1275";

  grub_terminfo_init ();
  grub_terminfo_output_register (&grub_ofconsole_term_output, type);
}

void
grub_console_fini (void)
{
  grub_term_unregister_input (&grub_ofconsole_term_input);
  grub_term_unregister_output (&grub_ofconsole_term_output);
  grub_terminfo_output_unregister (&grub_ofconsole_term_output);

  grub_terminfo_fini ();
}
