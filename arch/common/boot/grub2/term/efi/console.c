/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007  Free Software Foundation, Inc.
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
#include <grub/misc.h>
#include <grub/types.h>
#include <grub/err.h>
#include <grub/efi/efi.h>
#include <grub/efi/api.h>
#include <grub/efi/console.h>

static grub_uint8_t
grub_console_standard_color = GRUB_EFI_TEXT_ATTR (GRUB_EFI_YELLOW,
						  GRUB_EFI_BACKGROUND_BLACK);
static grub_uint8_t
grub_console_normal_color = GRUB_EFI_TEXT_ATTR (GRUB_EFI_LIGHTGRAY,
						GRUB_EFI_BACKGROUND_BLACK);
static grub_uint8_t
grub_console_highlight_color = GRUB_EFI_TEXT_ATTR (GRUB_EFI_BLACK,
						   GRUB_EFI_BACKGROUND_LIGHTGRAY);

static int read_key = -1;

static void
grub_console_putchar (grub_uint32_t c)
{
  grub_efi_char16_t str[2];
  grub_efi_simple_text_output_interface_t *o;
  
  o = grub_efi_system_table->con_out;
  
  /* For now, do not try to use a surrogate pair.  */
  if (c > 0xffff)
    c = '?';

  str[0] = (grub_efi_char16_t)  (c & 0xffff);
  str[1] = 0;

  /* Should this test be cached?  */
  if (c > 0x7f && o->test_string (o, str) != GRUB_EFI_SUCCESS)
    return;
  
  o->output_string (o, str);
}

static grub_ssize_t
grub_console_getcharwidth (grub_uint32_t c __attribute__ ((unused)))
{
  /* For now, every printable character has the width 1.  */
  return 1;
}

static int
grub_console_checkkey (void)
{
  grub_efi_simple_input_interface_t *i;
  grub_efi_input_key_t key;
  grub_efi_status_t status;
  
  if (read_key >= 0)
    return 1;

  i = grub_efi_system_table->con_in;
  status = i->read_key_stroke (i, &key);
#if 1
  switch (status)
    {
    case GRUB_EFI_SUCCESS:
      {
	grub_uint16_t xy;
	
	xy = grub_getxy ();
	grub_gotoxy (0, 0);
	grub_printf ("scan_code=%x,unicode_char=%x  ",
		     (unsigned) key.scan_code,
		     (unsigned) key.unicode_char);
	grub_gotoxy (xy >> 8, xy & 0xff);
      }
      break;

    case GRUB_EFI_NOT_READY:
      //grub_printf ("not ready   ");
      break;

    default:
      //grub_printf ("device error   ");
      break;
    }
#endif
  
  if (status == GRUB_EFI_SUCCESS)
    {
      switch (key.scan_code)
	{
	case 0x00:
	  read_key = key.unicode_char;
	  break;
	case 0x01:
	  read_key = 16;
	  break;
	case 0x02:
	  read_key = 14;
	  break;
	case 0x03:
	  read_key = 6;
	  break;
	case 0x04:
	  read_key = 2;
	  break;
	case 0x05:
	  read_key = 1;
	  break;
	case 0x06:
	  read_key = 5;
	  break;
	case 0x07:
	  break;
	case 0x08:
	  read_key = 4;
	  break;
	case 0x09:
	  break;
	case 0x0a:
	  break;
	case 0x17:
	  read_key = '\e';
	  break;
	default:
	  break;
	}
    }

  return read_key;
}

static int
grub_console_getkey (void)
{
  grub_efi_simple_input_interface_t *i;
  grub_efi_boot_services_t *b;
  grub_efi_uintn_t index;
  grub_efi_status_t status;
  int key;

  if (read_key >= 0)
    {
      key = read_key;
      read_key = -1;
      return key;
    }

  i = grub_efi_system_table->con_in;
  b = grub_efi_system_table->boot_services;

  do
    {
      status = b->wait_for_event (1, &(i->wait_for_key), &index);
      if (status != GRUB_EFI_SUCCESS)
      	return -1;
      
      grub_console_checkkey ();
    }
  while (read_key < 0);
  
  key = read_key;
  read_key = -1;
  return key;
}

static grub_uint16_t
grub_console_getwh (void)
{
  grub_efi_simple_text_output_interface_t *o;
  grub_efi_uintn_t columns, rows;
  
  o = grub_efi_system_table->con_out;
  if (o->query_mode (o, o->mode->mode, &columns, &rows) != GRUB_EFI_SUCCESS)
    {
      /* Why does this fail?  */
      columns = 80;
      rows = 25;
    }

  return ((columns << 8) | rows);
}

static grub_uint16_t
grub_console_getxy (void)
{
  grub_efi_simple_text_output_interface_t *o;
  
  o = grub_efi_system_table->con_out;
  return ((o->mode->cursor_column << 8) | o->mode->cursor_row);
}

static void
grub_console_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  grub_efi_simple_text_output_interface_t *o;
  
  o = grub_efi_system_table->con_out;
  o->set_cursor_position (o, x, y);
}

static void
grub_console_cls (void)
{
  grub_efi_simple_text_output_interface_t *o;
  grub_efi_int32_t orig_attr;
  
  o = grub_efi_system_table->con_out;
  orig_attr = o->mode->attribute;
  o->set_attributes (o, GRUB_EFI_BACKGROUND_BLACK);
  o->clear_screen (o);
  o->set_attributes (o, orig_attr);
}

static void
grub_console_setcolorstate (grub_term_color_state state)
{
  grub_efi_simple_text_output_interface_t *o;

  o = grub_efi_system_table->con_out;

  switch (state) {
    case GRUB_TERM_COLOR_STANDARD:
      o->set_attributes (o, grub_console_standard_color);
      break;
    case GRUB_TERM_COLOR_NORMAL:
      o->set_attributes (o, grub_console_normal_color);
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      o->set_attributes (o, grub_console_highlight_color);
      break;
    default:
      break;
  }
}

static void
grub_console_setcolor (grub_uint8_t normal_color, grub_uint8_t highlight_color)
{
  grub_console_normal_color = normal_color;
  grub_console_highlight_color = highlight_color;
}

static void
grub_console_getcolor (grub_uint8_t *normal_color, grub_uint8_t *highlight_color)
{
  *normal_color = grub_console_normal_color;
  *highlight_color = grub_console_highlight_color;
}

static void
grub_console_setcursor (int on)
{
  grub_efi_simple_text_output_interface_t *o;

  o = grub_efi_system_table->con_out;
  o->enable_cursor (o, on);
}

static struct grub_term grub_console_term =
  {
    .name = "console",
    .init = 0,
    .fini = 0,
    .putchar = grub_console_putchar,
    .getcharwidth = grub_console_getcharwidth,
    .checkkey = grub_console_checkkey,
    .getkey = grub_console_getkey,
    .getwh = grub_console_getwh,
    .getxy = grub_console_getxy,
    .gotoxy = grub_console_gotoxy,
    .cls = grub_console_cls,
    .setcolorstate = grub_console_setcolorstate,
    .setcolor = grub_console_setcolor,
    .getcolor = grub_console_getcolor,
    .setcursor = grub_console_setcursor,
    .flags = 0,
    .next = 0
  };

void
grub_console_init (void)
{
  /* FIXME: it is necessary to consider the case where no console control
     is present but the default is already in text mode.  */
  if (! grub_efi_set_text_mode (1))
    {
      grub_error (GRUB_ERR_BAD_DEVICE, "cannot set text mode");
      return;
    }

  grub_term_register (&grub_console_term);
  grub_term_set_current (&grub_console_term);
}

void
grub_console_fini (void)
{
  grub_term_unregister (&grub_console_term);
}
