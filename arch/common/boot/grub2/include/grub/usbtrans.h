/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008  Free Software Foundation, Inc.
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

#ifndef	GRUB_USBTRANS_H
#define	GRUB_USBTRANS_H	1

typedef enum
  {
    GRUB_USB_TRANSFER_TYPE_IN,
    GRUB_USB_TRANSFER_TYPE_OUT,
    GRUB_USB_TRANSFER_TYPE_SETUP
  } grub_transfer_type_t;

typedef enum
  {
    GRUB_USB_TRANSACTION_TYPE_CONTROL,
    GRUB_USB_TRANSACTION_TYPE_BULK
  } grub_transaction_type_t;

struct grub_usb_transaction
{
  int size;
  int toggle;
  grub_transfer_type_t pid;
  char *data;
};
typedef struct grub_usb_transaction *grub_usb_transaction_t;

struct grub_usb_transfer
{
  int devaddr;

  int endpoint;

  int size;

  int transcnt;

  int max;

  grub_transaction_type_t type;

  struct grub_usb_device *dev;

  struct grub_usb_transaction *transactions;
};
typedef struct grub_usb_transfer *grub_usb_transfer_t;


#define GRUB_USB_REQTYPE_IN		(1 << 7)
#define GRUB_USB_REQTYPE_OUT		(0 << 7)
#define GRUB_USB_REQTYPE_STANDARD	(0 << 5)
#define GRUB_USB_REQTYPE_CLASS		(1 << 5)
#define GRUB_USB_REQTYPE_VENDOR		(2 << 5)
#define GRUB_USB_REQTYPE_TARGET_DEV	(0 << 0)
#define GRUB_USB_REQTYPE_TARGET_INTERF	(1 << 0)
#define GRUB_USB_REQTYPE_TARGET_ENDP	(2 << 0)
#define GRUB_USB_REQTYPE_TARGET_OTHER	(3 << 0)

#define GRUB_USB_REQ_GET_STATUS		0x00
#define GRUB_USB_REQ_CLEAR_FEATURE	0x01
#define GRUB_USB_REQ_SET_FEATURE	0x03
#define GRUB_USB_REQ_SET_ADDRESS	0x05
#define GRUB_USB_REQ_GET_DESCRIPTOR	0x06
#define GRUB_USB_REQ_SET_DESCRIPTOR	0x07
#define GRUB_USB_REQ_GET_CONFIGURATION	0x08
#define GRUB_USB_REQ_SET_CONFIGURATION	0x09
#define GRUB_USB_REQ_GET_INTERFACE	0x0A
#define GRUB_USB_REQ_SET_INTERFACE	0x0B
#define GRUB_USB_REQ_SYNC_FRAME		0x0C

#define GRUB_USB_REQ_HUB_GET_PORT_STATUS 0x00

#define GRUB_USB_FEATURE_ENDP_HALT	0x01
#define GRUB_USB_FEATURE_DEV_REMOTE_WU	0x02
#define GRUB_USB_FEATURE_TEST_MODE	0x04

#define GRUB_USB_HUB_STATUS_CONNECTED	(1 << 0)
#define GRUB_USB_HUB_STATUS_LOWSPEED	(1 << 9)
#define GRUB_USB_HUB_STATUS_HIGHSPEED	(1 << 10)

struct grub_usb_packet_setup
{
  grub_uint8_t reqtype;
  grub_uint8_t request;
  grub_uint16_t value;
  grub_uint16_t index;
  grub_uint16_t length;
} __attribute__((packed));


#endif /* GRUB_USBTRANS_H */
