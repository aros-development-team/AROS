/* Support for the HID Boot Protocol.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008, 2009  Free Software Foundation, Inc.
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
#include <grub/machine/machine.h>
#include <grub/machine/console.h>
#include <grub/time.h>
#include <grub/cpu/io.h>
#include <grub/misc.h>
#include <grub/term.h>
#include <grub/usb.h>
#include <grub/dl.h>
#include <grub/time.h>


static char keyboard_map[128] =
  {
    '\0', '\0', '\0', '\0', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z', '1', '2',
    '3', '4', '5', '6', '7', '8', '9', '0',
    '\n', GRUB_TERM_ESC, GRUB_TERM_BACKSPACE, GRUB_TERM_TAB, ' ', '-', '=', '[',
    ']', '\\', '#', ';', '\'', '`', ',', '.',
    '/', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
    '\0', '\0', GRUB_TERM_HOME, GRUB_TERM_PPAGE, GRUB_TERM_DC, GRUB_TERM_END, GRUB_TERM_NPAGE, GRUB_TERM_RIGHT,
    GRUB_TERM_LEFT, GRUB_TERM_DOWN, GRUB_TERM_UP
  };

static char keyboard_map_shift[128] =
  {
    '\0', '\0', '\0', '\0', 'A', 'B', 'C', 'D',
    'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
    'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', '!', '@',
    '#', '$', '%', '^', '&', '*', '(', ')',
    '\n', '\0', '\0', '\0', ' ', '_', '+', '{',
    '}', '|', '#', ':', '"', '`', '<', '>',
    '?'
  };

static grub_usb_device_t usbdev;

/* Valid values for bmRequestType.  See HID definition version 1.11 section
   7.2.  */
#define USB_HID_HOST_TO_DEVICE	0x21
#define USB_HID_DEVICE_TO_HOST	0x61

/* Valid values for bRequest.  See HID definition version 1.11 section 7.2. */
#define USB_HID_GET_REPORT	0x01
#define USB_HID_GET_IDLE	0x02
#define USB_HID_GET_PROTOCOL	0x03
#define USB_HID_SET_REPORT	0x09
#define USB_HID_SET_IDLE	0x0A
#define USB_HID_SET_PROTOCOL	0x0B

static void
grub_usb_hid (void)
{
  struct grub_usb_desc_device *descdev;

  auto int usb_iterate (grub_usb_device_t dev);
  int usb_iterate (grub_usb_device_t dev)
    {
      descdev = &dev->descdev;

      grub_dprintf ("usb_keyboard", "%x %x %x\n",
		   descdev->class, descdev->subclass, descdev->protocol);

#if 0
      if (descdev->class != 0x09
	  || descdev->subclass == 0x01
	  || descdev->protocol != 0x02)
	return 0;
#endif

      if (descdev->class != 0 || descdev->subclass != 0 || descdev->protocol != 0)
	return 0;

      grub_printf ("HID found!\n");

      usbdev = dev;

      return 1;
    }
  grub_usb_iterate (usb_iterate);

  /* Place the device in boot mode.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_PROTOCOL,
			0, 0, 0, 0);

  /* Reports every time an event occurs and not more often than that.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_IDLE,
			0<<8, 0, 0, 0);
}

static grub_err_t
grub_usb_keyboard_getreport (grub_usb_device_t dev, grub_uint8_t *report)
{
  return grub_usb_control_msg (dev, USB_HID_DEVICE_TO_HOST, USB_HID_GET_REPORT,
			       0, 0, 8, (char *) report);
}



static int
grub_usb_keyboard_checkkey (void)
{
  grub_uint8_t data[8];
  int key;
  grub_err_t err;
  grub_uint64_t currtime;
  int timeout = 50;

  data[2] = 0;
  currtime = grub_get_time_ms ();
  do
    {
      /* Get_Report.  */
      err = grub_usb_keyboard_getreport (usbdev, data);

      /* Implement a timeout.  */
      if (grub_get_time_ms () > currtime + timeout)
	break;
    }
  while (err || !data[2]);

  if (err || !data[2])
    return -1;

  grub_dprintf ("usb_keyboard",
		"report: 0x%02x 0x%02x 0x%02x 0x%02x"
		" 0x%02x 0x%02x 0x%02x 0x%02x\n",
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);

  /* Check if the Control or Shift key was pressed.  */
  if (data[0] & 0x01 || data[0] & 0x10)
    key = keyboard_map[data[2]] - 'a' + 1;
  else if (data[0] & 0x02 || data[0] & 0x20)
    key = keyboard_map_shift[data[2]];
  else
    key = keyboard_map[data[2]];

  if (key == 0)
    grub_printf ("Unknown key 0x%x detected\n", data[2]);

#if 0
  /* Wait until the key is released.  */
  while (!err && data[2])
    {
      err = grub_usb_control_msg (usbdev, USB_HID_DEVICE_TO_HOST,
				  USB_HID_GET_REPORT, 0, 0,
				  sizeof (data), (char *) data);
      grub_dprintf ("usb_keyboard",
		    "report2: 0x%02x 0x%02x 0x%02x 0x%02x"
		    " 0x%02x 0x%02x 0x%02x 0x%02x\n",
		    data[0], data[1], data[2], data[3],
		    data[4], data[5], data[6], data[7]);
    }
#endif

  grub_errno = GRUB_ERR_NONE;

  return key;
}

typedef enum
{
  GRUB_HIDBOOT_REPEAT_NONE,
  GRUB_HIDBOOT_REPEAT_FIRST,
  GRUB_HIDBOOT_REPEAT
} grub_usb_keyboard_repeat_t;

static int
grub_usb_keyboard_getkey (void)
{
  int key;
  grub_err_t err;
  grub_uint8_t data[8];
  grub_uint64_t currtime;
  int timeout;
  static grub_usb_keyboard_repeat_t repeat = GRUB_HIDBOOT_REPEAT_NONE;

 again:

  do
    {
      key = grub_usb_keyboard_checkkey ();
    } while (key == -1);

  data[2] = !0; /* Or whatever.  */
  err = 0;

  switch (repeat)
    {
    case GRUB_HIDBOOT_REPEAT_FIRST:
      timeout = 500;
      break;
    case GRUB_HIDBOOT_REPEAT:
      timeout = 50;
      break;
    default:
      timeout = 100;
      break;
    }

  /* Wait until the key is released.  */
  currtime = grub_get_time_ms ();
  while (!err && data[2])
    {
      /* Implement a timeout.  */
      if (grub_get_time_ms () > currtime + timeout)
	{
	  if (repeat == 0)
	    repeat = 1;
	  else
	    repeat = 2;

	  grub_errno = GRUB_ERR_NONE;
	  return key;
	}

      err = grub_usb_keyboard_getreport (usbdev, data);
    }

  if (repeat)
    {
      repeat = 0;
      goto again;
    }

  repeat = 0;

  grub_errno = GRUB_ERR_NONE;

  return key;
}

static int
grub_usb_keyboard_getkeystatus (void)
{
  grub_uint8_t data[8];
  int mods = 0;
  grub_err_t err;
  grub_uint64_t currtime;
  int timeout = 50;

  /* Set idle time to the minimum offered by the spec (4 milliseconds) so
     that we can find out the current state.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_IDLE,
			0<<8, 0, 0, 0);

  currtime = grub_get_time_ms ();
  do
    {
      /* Get_Report.  */
      err = grub_usb_keyboard_getreport (usbdev, data);

      /* Implement a timeout.  */
      if (grub_get_time_ms () > currtime + timeout)
	break;
    }
  while (err || !data[0]);

  /* Go back to reporting every time an event occurs and not more often than
     that.  */
  grub_usb_control_msg (usbdev, USB_HID_HOST_TO_DEVICE, USB_HID_SET_IDLE,
			0<<8, 0, 0, 0);

  /* We allowed a while for modifiers to show up in the report, but it is
     not an error if they never did.  */
  if (err)
    return -1;

  grub_dprintf ("usb_keyboard",
		"report: 0x%02x 0x%02x 0x%02x 0x%02x"
		" 0x%02x 0x%02x 0x%02x 0x%02x\n",
		data[0], data[1], data[2], data[3],
		data[4], data[5], data[6], data[7]);

  /* Check Shift, Control, and Alt status.  */
  if (data[0] & 0x02 || data[0] & 0x20)
    mods |= GRUB_TERM_STATUS_SHIFT;
  if (data[0] & 0x01 || data[0] & 0x10)
    mods |= GRUB_TERM_STATUS_CTRL;
  if (data[0] & 0x04 || data[0] & 0x40)
    mods |= GRUB_TERM_STATUS_ALT;

  grub_errno = GRUB_ERR_NONE;

  return mods;
}

static struct grub_term_input grub_usb_keyboard_term =
  {
    .name = "usb_keyboard",
    .checkkey = grub_usb_keyboard_checkkey,
    .getkey = grub_usb_keyboard_getkey,
    .getkeystatus = grub_usb_keyboard_getkeystatus,
    .next = 0
  };

GRUB_MOD_INIT(usb_keyboard)
{
  grub_usb_hid ();
  grub_term_register_input ("usb_keyboard", &grub_usb_keyboard_term);
}

GRUB_MOD_FINI(usb_keyboard)
{
  grub_term_unregister_input (&grub_usb_keyboard_term);
}
