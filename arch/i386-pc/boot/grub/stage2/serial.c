/* serial.c - serial device interface */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000, 2001  Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef SUPPORT_SERIAL

#include <shared.h>
#include <serial.h>

/* The structure for speed vs. divisor.  */
struct divisor
{
  int speed;
  unsigned short div;
};

/* Store the port number of a serial unit.  */
static unsigned short serial_port = -1;

/* The table which lists common configurations.  */
static struct divisor divisor_tab[] =
{
  { 2400,   0x0030 },
  { 4800,   0x0018 },
  { 9600,   0x000C },
  { 19200,  0x0006 },
  { 38400,  0x0003 },
  { 57600,  0x0002 },
  { 115200, 0x0001 }
};


/* Read a byte from a port.  */
static inline unsigned char
inb (unsigned short port)
{
  unsigned char value;
  
  asm volatile ("inb	%w1, %0" : "=a" (value) : "Nd" (port));
  return value;
}

/* Write a byte to a port.  */
static inline void
outb (unsigned short port, unsigned char value)
{
  asm volatile ("outb	%b0, %w1" : : "a" (value), "Nd" (port));
}

/* The serial version of getkey.  */
int
serial_getkey (void)
{
  /* Wait until data is ready.  */
  while ((inb (serial_port + UART_LSR) & UART_DATA_READY) == 0)
    ;

  /* Read and return the data.  */
  return inb (serial_port + UART_RX);
}

/* The serial version of checkkey. This doesn't return a character code,
   but that doesn't matter actually.  */
int
serial_checkkey (void)
{
  unsigned char status;

  status = inb (serial_port + UART_LSR);
  return status & UART_DATA_READY ? : -1;
}

/* The serial version of grub_putchar.  */
void
serial_putchar (int c)
{
  /* Perhaps a timeout is necessary.  */
  int timeout = 10000;

  /* Wait until the transmitter holding register is empty.  */
  while ((inb (serial_port + UART_LSR) & UART_EMPTY_TRANSMITTER) == 0)
    if (--timeout == 0)
      /* There is something wrong. But what can I do?  */
      return;

  outb (serial_port + UART_TX, c);
}

/* Check if a serial port is set up.  */
int
serial_exists (void)
{
  return serial_port != -1;
}

/* Return the port number for the UNITth serial device.  */
unsigned short
serial_get_port (int unit)
{
  /* The BIOS data area.  */
  const unsigned short *addr = (const unsigned short *) 0x0400;
  
  return addr[unit];
}

/* Initialize a serial device. PORT is the port number for a serial device.
   SPEED is a DTE-DTE speed which must be one of these: 2400, 4800, 9600,
   19200, 38400, 57600 and 115200. WORD_LEN is the word length to be used
   for the device. Likewise, PARITY is the type of the parity and
   STOP_BIT_LEN is the length of the stop bit. The possible values for
   WORD_LEN, PARITY and STOP_BIT_LEN are defined in the header file as
   macros.  */
int
serial_init (unsigned short port, unsigned int speed,
	     int word_len, int parity, int stop_bit_len)
{
  int i;
  unsigned short div = 0;
  unsigned char status = 0;
  
  /* Turn off the interrupt.  */
  outb (port + UART_IER, 0);

  /* Set DLAB.  */
  outb (port + UART_LCR, UART_DLAB);
  
  /* Set the baud rate.  */
  for (i = 0; i < sizeof (divisor_tab) / sizeof (divisor_tab[0]); i++)
    if (divisor_tab[i].speed == speed)
      {
	div = divisor_tab[i].div;
	break;
      }
  
  if (div == 0)
    return 0;
  
  outb (port + UART_DLL, div & 0xFF);
  outb (port + UART_DLH, div >> 8);
  
  /* Set the line status.  */
  status |= parity | word_len | stop_bit_len;
  outb (port + UART_LCR, status);

  /* Enable the FIFO.  */
  outb (port + UART_FCR, UART_ENABLE_FIFO);

  /* Turn on DTR, RTS, and OUT2.  */
  outb (port + UART_MCR, UART_ENABLE_MODEM);

  /* Store the port number.  */
  serial_port = port;
  
  /* Drain the input buffer.  */
  while (serial_checkkey () != -1)
    (void) serial_getkey ();
  
  return 1;
}

#endif /* SUPPORT_SERIAL */
