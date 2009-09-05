/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008  Free Software Foundation, Inc.
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
#include <grub/i386/pc/console.h>
#include <grub/i386/at_keyboard.h>
#include <grub/i386/io.h>
#include <grub/misc.h>
#include <grub/term.h>

static short at_keyboard_status = 0;

#define KEYBOARD_STATUS_SHIFT_L		(1 << 0)
#define KEYBOARD_STATUS_SHIFT_R		(1 << 1)
#define KEYBOARD_STATUS_ALT_L		(1 << 2)
#define KEYBOARD_STATUS_ALT_R		(1 << 3)
#define KEYBOARD_STATUS_CTRL_L		(1 << 4)
#define KEYBOARD_STATUS_CTRL_R		(1 << 5)
#define KEYBOARD_STATUS_CAPS_LOCK	(1 << 6)

static char keyboard_map[128] =
{
  '\0', GRUB_TERM_ESC, '1', '2', '3', '4', '5', '6',
  '7', '8', '9', '0', '-', '=', GRUB_TERM_BACKSPACE, GRUB_TERM_TAB,
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
  'o', 'p', '[', ']', '\n', '\0', 'a', 's',
  'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',
  '\'', '`', '\0', '\\', 'z', 'x', 'c', 'v',
  'b', 'n', 'm', ',', '.', '/', '\0', '*',
  '\0', ' ', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', GRUB_TERM_HOME,
  GRUB_TERM_UP, GRUB_TERM_NPAGE, '-', GRUB_TERM_LEFT, '\0', GRUB_TERM_RIGHT, '+', GRUB_TERM_END,
  GRUB_TERM_DOWN, GRUB_TERM_PPAGE, '\0', GRUB_TERM_DC, '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', OLPC_UP, OLPC_DOWN, OLPC_LEFT,
  OLPC_RIGHT
};

static char keyboard_map_shift[128] =
{
  '\0', '\0', '!', '@', '#', '$', '%', '^',
  '&', '*', '(', ')', '_', '+', '\0', '\0',
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
  'O', 'P', '{', '}', '\n', '\0', 'A', 'S',
  'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',
  '\"', '~', '\0', '|', 'Z', 'X', 'C', 'V',
  'B', 'N', 'M', '<', '>', '?'
};

static grub_uint8_t grub_keyboard_controller_orig;

static void
grub_keyboard_controller_write (grub_uint8_t c)
{
  while (! KEYBOARD_COMMAND_ISREADY (grub_inb (KEYBOARD_REG_STATUS)));
  grub_outb (KEYBOARD_COMMAND_WRITE, KEYBOARD_REG_STATUS);
  grub_outb (c, KEYBOARD_REG_DATA);
}

static grub_uint8_t
grub_keyboard_controller_read (void)
{
  while (! KEYBOARD_COMMAND_ISREADY (grub_inb (KEYBOARD_REG_STATUS)));
  grub_outb (KEYBOARD_COMMAND_READ, KEYBOARD_REG_STATUS);
  return grub_inb (KEYBOARD_REG_DATA);
}

/* FIXME: This should become an interrupt service routine.  For now
   it's just used to catch events from control keys.  */
static void
grub_keyboard_isr (char key)
{
  char is_make = KEYBOARD_ISMAKE (key);
  key = KEYBOARD_SCANCODE (key);
  if (is_make)
    switch (key)
      {
	case SHIFT_L:
	  at_keyboard_status |= KEYBOARD_STATUS_SHIFT_L;
	  break;
	case SHIFT_R:
	  at_keyboard_status |= KEYBOARD_STATUS_SHIFT_R;
	  break;
	case CTRL:
	  at_keyboard_status |= KEYBOARD_STATUS_CTRL_L;
	  break;
	case ALT:
	  at_keyboard_status |= KEYBOARD_STATUS_ALT_L;
	  break;
	default:
	  /* Skip grub_dprintf.  */
	  return;
      }
  else
    switch (key)
      {
	case SHIFT_L:
	  at_keyboard_status &= ~KEYBOARD_STATUS_SHIFT_L;
	  break;
	case SHIFT_R:
	  at_keyboard_status &= ~KEYBOARD_STATUS_SHIFT_R;
	  break;
	case CTRL:
	  at_keyboard_status &= ~KEYBOARD_STATUS_CTRL_L;
	  break;
	case ALT:
	  at_keyboard_status &= ~KEYBOARD_STATUS_ALT_L;
	  break;
	default:
	  /* Skip grub_dprintf.  */
	  return;
      }
#ifdef DEBUG_AT_KEYBOARD
  grub_dprintf ("atkeyb", "Control key 0x%0x was %s\n", key, is_make ? "pressed" : "unpressed");
#endif
}

/* If there is a raw key pending, return it; otherwise return -1.  */
static int
grub_keyboard_getkey (void)
{
  grub_uint8_t key;
  if (KEYBOARD_ISREADY (grub_inb (KEYBOARD_REG_STATUS)))
    return -1;
  key = grub_inb (KEYBOARD_REG_DATA);
  /* FIXME */ grub_keyboard_isr (key);
  if (! KEYBOARD_ISMAKE (key))
    return -1;
  return (KEYBOARD_SCANCODE (key));
}

/* If there is a character pending, return it; otherwise return -1.  */
static int
grub_at_keyboard_checkkey (void)
{
  int code, key;
  code = grub_keyboard_getkey ();
  if (code == -1)
    return -1;
#ifdef DEBUG_AT_KEYBOARD
  grub_dprintf ("atkeyb", "Detected key 0x%x\n", key);
#endif
  switch (code)
    {
      case CAPS_LOCK:
	at_keyboard_status ^= KEYBOARD_STATUS_CAPS_LOCK;
	/* Caps lock sends scan code twice.  Get the second one and discard it.  */
	while (grub_keyboard_getkey () == -1);
#ifdef DEBUG_AT_KEYBOARD
	grub_dprintf ("atkeyb", "caps_lock = %d\n", !!(at_keyboard_status & KEYBOARD_STATUS_CAPS_LOCK));
#endif
	key = -1;
	break;
      default:
	if (at_keyboard_status & (KEYBOARD_STATUS_CTRL_L | KEYBOARD_STATUS_CTRL_R))
	  key = keyboard_map[code] - 'a' + 1;
	else if ((at_keyboard_status & (KEYBOARD_STATUS_SHIFT_L | KEYBOARD_STATUS_SHIFT_R))
	    && keyboard_map_shift[code])
	  key = keyboard_map_shift[code];
	else
	  key = keyboard_map[code];

	if (key == 0)
	  grub_dprintf ("atkeyb", "Unknown key 0x%x detected\n", code);

	if (at_keyboard_status & KEYBOARD_STATUS_CAPS_LOCK)
	  {
	    if ((key >= 'a') && (key <= 'z'))
	      key += 'A' - 'a';
	    else if ((key >= 'A') && (key <= 'Z'))
	      key += 'a' - 'A';
	  }
    }
  return (int) key;
}

static int
grub_at_keyboard_getkey (void)
{
  int key;
  do
    {
      key = grub_at_keyboard_checkkey ();
    } while (key == -1);
  return key;
}

static grub_err_t
grub_keyboard_controller_init (void)
{
  grub_keyboard_controller_orig = grub_keyboard_controller_read ();
  grub_keyboard_controller_write (grub_keyboard_controller_orig | KEYBOARD_SCANCODE_SET1);
  return GRUB_ERR_NONE;
}

static grub_err_t
grub_keyboard_controller_fini (void)
{
  grub_keyboard_controller_write (grub_keyboard_controller_orig);
  return GRUB_ERR_NONE;
}

static struct grub_term_input grub_at_keyboard_term =
  {
    .name = "at_keyboard",
    .init = grub_keyboard_controller_init,
    .fini = grub_keyboard_controller_fini,
    .checkkey = grub_at_keyboard_checkkey,
    .getkey = grub_at_keyboard_getkey,
  };

GRUB_MOD_INIT(at_keyboard)
{
  grub_term_register_input ("at_keyboard", &grub_at_keyboard_term);
}

GRUB_MOD_FINI(at_keyboard)
{
  grub_term_unregister_input (&grub_at_keyboard_term);
}
