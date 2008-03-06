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

#include <grub/machine/console.h>
#include <grub/cpu/io.h>
#include <grub/misc.h>
#include <grub/term.h>

#define SHIFT_L		0x2a
#define SHIFT_R		0x36
#define CTRL		0x1d
#define ALT		0x38
#define CAPS_LOCK	0x3a

#define KEYBOARD_STATUS_SHIFT_L		(1 << 0)
#define KEYBOARD_STATUS_SHIFT_R		(1 << 1)
#define KEYBOARD_STATUS_ALT_L		(1 << 2)
#define KEYBOARD_STATUS_ALT_R		(1 << 3)
#define KEYBOARD_STATUS_CTRL_L		(1 << 4)
#define KEYBOARD_STATUS_CTRL_R		(1 << 5)
#define KEYBOARD_STATUS_CAPS_LOCK	(1 << 6)

#define KEYBOARD_REG_DATA	0x60
#define KEYBOARD_REG_STATUS	0x64

/* Used for sending commands to the controller.  */
#define KEYBOARD_COMMAND_ISREADY(x)	!((x) & 0x02)
#define KEYBOARD_COMMAND_READ		0x20
#define KEYBOARD_COMMAND_WRITE		0x60

#define KEYBOARD_SCANCODE_SET1		0x40

#define KEYBOARD_ISMAKE(x)	!((x) & 0x80)
#define KEYBOARD_ISREADY(x)	(((x) & 0x01) == 0)
#define KEYBOARD_SCANCODE(x)	((x) & 0x7f)

static short at_keyboard_status = 0;

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
  GRUB_TERM_DOWN, GRUB_TERM_PPAGE, '\0', GRUB_TERM_DC
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

void
grub_keyboard_controller_init (void)
{
  grub_keyboard_controller_write (grub_keyboard_controller_read () | KEYBOARD_SCANCODE_SET1);
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
int
grub_console_checkkey (void)
{
  int key;
  key = grub_keyboard_getkey ();
  if (key == -1)
    return -1;
#ifdef DEBUG_AT_KEYBOARD
  grub_dprintf ("atkeyb", "Detected key 0x%x\n", key);
#endif
  switch (key)
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
	if ((at_keyboard_status & (KEYBOARD_STATUS_SHIFT_L | KEYBOARD_STATUS_SHIFT_R))
	    && keyboard_map_shift[key])
	  key = keyboard_map_shift[key];
	else
	  key = keyboard_map[key];
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

int
grub_console_getkey (void)
{
  int key;
  do
    {
      key = grub_console_checkkey ();
    } while (key == -1);
  return key;
}
