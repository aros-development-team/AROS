/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000,2001,2002,2003,2004,2005  Free Software Foundation, Inc.
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

#include <grub/machine/serial.h>
#include <grub/machine/console.h>
#include <grub/term.h>
#include <grub/types.h>
#include <grub/dl.h>
#include <grub/misc.h>
#include <grub/normal.h>
#include <grub/arg.h>
#include <grub/terminfo.h>

#define TEXT_WIDTH	80
#define TEXT_HEIGHT	25

static unsigned int xpos, ypos;
static unsigned int keep_track = 1;
static unsigned int registered = 0;

/* An input buffer.  */
static char input_buf[8];
static unsigned int npending = 0;

/* Argument options.  */
static const struct grub_arg_option options[] =
{
  {"unit",   'u', 0, "Set the serial unit",             0, ARG_TYPE_INT},
  {"port",   'p', 0, "Set the serial port address",     0, ARG_TYPE_STRING},
  {"speed",  's', 0, "Set the serial port speed",       0, ARG_TYPE_INT},
  {"word",   'w', 0, "Set the serial port word length", 0, ARG_TYPE_INT},
  {"parity", 'r', 0, "Set the serial port parity",      0, ARG_TYPE_STRING},
  {"stop",   't', 0, "Set the serial port stop bits",   0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

/* Serial port settings.  */
struct serial_port
{
  unsigned short port;
  unsigned short divisor;
  unsigned short word_len;
  unsigned int   parity;
  unsigned short stop_bits;
};

/* Serial port settings.  */
static struct serial_port serial_settings;

/* Read a byte from a port.  */
static inline unsigned char
inb (const unsigned short port)
{
  unsigned char value;

  asm volatile ("inb    %w1, %0" : "=a" (value) : "Nd" (port));
  asm volatile ("outb   %%al, $0x80" : : );

  return value;
}

/* Write a byte to a port.  */
static inline void
outb (const unsigned short port, const unsigned char value)
{
  asm volatile ("outb   %b0, %w1" : : "a" (value), "Nd" (port));
  asm volatile ("outb   %%al, $0x80" : : );
}

/* Return the port number for the UNITth serial device.  */
static inline unsigned short
serial_hw_get_port (const unsigned short unit)
{
  /* The BIOS data area.  */
  const unsigned short *addr = (const unsigned short *) 0x0400;
  return addr[unit];
}

/* Fetch a key.  */
static int
serial_hw_fetch (void)
{
  if (inb (serial_settings.port + UART_LSR) & UART_DATA_READY)
    return inb (serial_settings.port + UART_RX);

  return -1;
}

/* Put a chararacter.  */
static void
serial_hw_put (const int c)
{
  unsigned int timeout = 100000;

  /* Wait until the transmitter holding register is empty.  */
  while ((inb (serial_settings.port + UART_LSR) & UART_EMPTY_TRANSMITTER) == 0)
    {
      if (--timeout == 0)
        /* There is something wrong. But what can I do?  */
        return;
    }

  outb (serial_settings.port + UART_TX, c);
}

static void
serial_translate_key_sequence (void)
{
  static struct
  {
    char key;
    char ascii;
  }
  three_code_table[] =
    {
      {'A', 16},
      {'B', 14},
      {'C', 6},
      {'D', 2},
      {'F', 5},
      {'H', 1},
      {'4', 4}
    };
  
  static struct
  {
      short key;
      char ascii;
  }
  four_code_table[] =
    {
      {('1' | ('~' << 8)), 1},
      {('3' | ('~' << 8)), 4},
      {('5' | ('~' << 8)), 7},
      {('6' | ('~' << 8)), 3}
    };

  /* The buffer must start with "ESC [".  */
  if (*((unsigned short *) input_buf) != ('\e' | ('[' << 8)))
    return;

  if (npending >= 3)
    {
      unsigned int i;
      
      for (i = 0;
	   i < sizeof (three_code_table) / sizeof (three_code_table[0]);
	   i++)
	if (three_code_table[i].key == input_buf[2])
	  {
	    input_buf[0] = three_code_table[i].ascii;
	    npending -= 2;
	    grub_memmove (input_buf + 1, input_buf + 3, npending - 1);
	    return;
	  }
    }

  if (npending >= 4)
    {
      unsigned int i;
      short key = *((short *) (input_buf + 2));
      
      for (i = 0;
	   i < sizeof (four_code_table) / sizeof (four_code_table[0]);
	   i++)
	if (four_code_table[i].key == key)
	  {
	    input_buf[0] = four_code_table[i].ascii;
	    npending -= 3;
	    grub_memmove (input_buf + 1, input_buf + 4, npending - 1);
	    return;
	  }
    }
}

static int
fill_input_buf (const int nowait)
{
  int i;

  for (i = 0; i < 10000 && npending < sizeof (input_buf); i++)
    {
      int c;
      
      c = serial_hw_fetch ();
      if (c >= 0)
	{
	  input_buf[npending++] = c;
	  
	  /* Reset the counter to zero, to wait for the same interval.  */
	  i = 0;
	}
      
      if (nowait)
	break;
    }

  /* Translate some key sequences.  */
  serial_translate_key_sequence ();

  return npending;
}

/* Convert speed to divisor.  */
static unsigned short
serial_get_divisor (unsigned int speed)
{
  unsigned int i;

  /* The structure for speed vs. divisor.  */
  struct divisor
  {
    unsigned int speed;
    unsigned short div;
  };

  /* The table which lists common configurations.  */
  /* 1843200 / (speed * 16)  */
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

  /* Set the baud rate.  */
  for (i = 0; i < sizeof (divisor_tab) / sizeof (divisor_tab[0]); i++)
    if (divisor_tab[i].speed == speed)
      return divisor_tab[i].div;
  return 0;
}

/* The serial version of checkkey.  */
static int
grub_serial_checkkey (void)
{
  if (fill_input_buf (1))
    return input_buf[0];
  else
    return -1;
}

/* The serial version of getkey.  */
static int
grub_serial_getkey (void)
{
  int c;

  while (! fill_input_buf (0))
    ;

  c = input_buf[0];
  grub_memmove (input_buf, input_buf + 1, --npending);

  return c;
}

/* Initialize a serial device. PORT is the port number for a serial device.
   SPEED is a DTE-DTE speed which must be one of these: 2400, 4800, 9600,
   19200, 38400, 57600 and 115200. WORD_LEN is the word length to be used
   for the device. Likewise, PARITY is the type of the parity and
   STOP_BIT_LEN is the length of the stop bit. The possible values for
   WORD_LEN, PARITY and STOP_BIT_LEN are defined in the header file as
   macros.  */
static grub_err_t
serial_hw_init (void)
{
  unsigned char status = 0;

  /* Turn off the interupt.  */
  outb (serial_settings.port + UART_IER, 0);

  /* Set DLAB.  */
  outb (serial_settings.port + UART_LCR, UART_DLAB);

  /* Set the baud rate.  */
  outb (serial_settings.port + UART_DLL, serial_settings.divisor & 0xFF);
  outb (serial_settings.port + UART_DLH, serial_settings.divisor >> 8 );

  /* Set the line status.  */
  status |= (serial_settings.parity
	     | serial_settings.word_len
	     | serial_settings.stop_bits);
  outb (serial_settings.port + UART_LCR, status);

  /* Enable the FIFO.  */
  outb (serial_settings.port + UART_FCR, UART_ENABLE_FIFO);

  /* Turn on DTR, RTS, and OUT2.  */
  outb (serial_settings.port + UART_MCR, UART_ENABLE_MODEM);

  /* Drain the input buffer.  */
  while (grub_serial_checkkey () != -1)
    (void) grub_serial_getkey ();

  /*  FIXME: should check if the serial terminal was found.  */

  return GRUB_ERR_NONE;
}

/* The serial version of putchar.  */
static void
grub_serial_putchar (grub_uint32_t c)
{
  /* Keep track of the cursor.  */
  if (keep_track)
    {
      /* The serial terminal does not have VGA fonts.  */
      if (c > 0x7F)
	{
	  /* Better than nothing.  */
	  switch (c)
	    {
	    case GRUB_TERM_DISP_LEFT:
	      c = '<';
	      break;
	      
	    case GRUB_TERM_DISP_UP:
	      c = '^';
	      break;
	      
	    case GRUB_TERM_DISP_RIGHT:
	      c = '>';
	      break;
	      
	    case GRUB_TERM_DISP_DOWN:
	      c = 'v';
	      break;
	      
	    case GRUB_TERM_DISP_HLINE:
	      c = '-';
	      break;
	      
	    case GRUB_TERM_DISP_VLINE:
	      c = '|';
	      break;
	      
	    case GRUB_TERM_DISP_UL:
	    case GRUB_TERM_DISP_UR:
	    case GRUB_TERM_DISP_LL:
	    case GRUB_TERM_DISP_LR:
	      c = '+';
	      break;
	      
	    default:
	      c = '?';
	      break;
	    }
	}
      
      switch (c)
	{
	case '\a':
	  break;
	  
	case '\b':
	case 127:
	  if (xpos > 0)
	    xpos--;
	  break;
	  
	case '\n':
	  if (ypos < TEXT_HEIGHT)
	    ypos++;
	  break;
	  
	case '\r':
	  xpos = 0;
	  break;
	  
	default:
	  if (xpos >= TEXT_WIDTH)
	    {
	      grub_putchar ('\r');
	      grub_putchar ('\n');
	    }
	  xpos++;
	  break;
	}
    }
  
  serial_hw_put (c);
}

static grub_ssize_t
grub_serial_getcharwidth (grub_uint32_t c __attribute__ ((unused)))
{
  return 1;
}

static grub_uint16_t
grub_serial_getwh (void)
{
  return (TEXT_WIDTH << 8) | TEXT_HEIGHT;
}

static grub_uint16_t
grub_serial_getxy (void)
{
  return ((xpos << 8) | ypos);
}

static void
grub_serial_gotoxy (grub_uint8_t x, grub_uint8_t y)
{
  if (x > TEXT_WIDTH || y > TEXT_HEIGHT)
    {
      grub_error (GRUB_ERR_OUT_OF_RANGE, "invalid point (%u,%u)", x, y);
    }
  else
    {
      keep_track = 0;
      grub_terminfo_gotoxy (x, y);
      keep_track = 1;
      
      xpos = x;
      ypos = y;
    }
}

static void
grub_serial_cls (void)
{
  keep_track = 0;
  grub_terminfo_cls ();
  keep_track = 1;

  xpos = ypos = 0;
}

static void
grub_serial_setcolorstate (const grub_term_color_state state)
{
  keep_track = 0;
  switch (state)
    {
    case GRUB_TERM_COLOR_STANDARD:
    case GRUB_TERM_COLOR_NORMAL:
      grub_terminfo_reverse_video_off ();
      break;
    case GRUB_TERM_COLOR_HIGHLIGHT:
      grub_terminfo_reverse_video_on ();
      break;
    default:
      break;
    }
  keep_track = 1;
}

static void
grub_serial_setcolor (grub_uint8_t normal_color __attribute__ ((unused)),
                      grub_uint8_t highlight_color __attribute__ ((unused)))
{
  /* FIXME */
}

static void
grub_serial_setcursor (const int on)
{
  if (on)
    grub_terminfo_cursor_on ();
  else
    grub_terminfo_cursor_off ();
}

static struct grub_term grub_serial_term =
{
  .name = "serial",
  .init = 0,
  .fini = 0,
  .putchar = grub_serial_putchar,
  .getcharwidth = grub_serial_getcharwidth,
  .checkkey = grub_serial_checkkey,
  .getkey = grub_serial_getkey,
  .getwh = grub_serial_getwh,
  .getxy = grub_serial_getxy,
  .gotoxy = grub_serial_gotoxy,
  .cls = grub_serial_cls,
  .setcolorstate = grub_serial_setcolorstate,
  .setcolor = grub_serial_setcolor,
  .setcursor = grub_serial_setcursor,
  .flags = 0,
  .next = 0
};



static grub_err_t
grub_cmd_serial (struct grub_arg_list *state,
                 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))
{
  struct serial_port backup_settings = serial_settings;
  grub_err_t hwiniterr;
  int arg;

  if (state[0].set)
    {
      arg = grub_strtoul (state[0].arg, 0, 0);
      if (arg >= 0 && arg < 4)
	serial_settings.port = serial_hw_get_port ((int) arg);
      else
	return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad unit number.");
    }
  
  if (state[1].set)
    serial_settings.port = (unsigned short) grub_strtoul (state[1].arg, 0, 0);
  
  if (state[2].set)
    {
      unsigned long speed;

      speed = grub_strtoul (state[2].arg, 0, 0);
      serial_settings.divisor = serial_get_divisor ((unsigned int) speed);
      if (serial_settings.divisor == 0)
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad speed");
	}
    }
  
  if (state[3].set)
    {
      if (! grub_strcmp (state[3].arg, "5"))
	serial_settings.word_len = UART_5BITS_WORD;
      else if (! grub_strcmp (state[3].arg, "6"))
	serial_settings.word_len = UART_6BITS_WORD;
      else if (! grub_strcmp (state[3].arg, "7"))
	serial_settings.word_len = UART_7BITS_WORD;
      else if (! grub_strcmp (state[3].arg, "8"))
	serial_settings.word_len = UART_8BITS_WORD;
      else
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad word length");
	}
    }
  
  if (state[4].set)
    {
      if (! grub_strcmp (state[4].arg, "no"))
	serial_settings.parity = UART_NO_PARITY;
      else if (! grub_strcmp (state[4].arg, "odd"))
	serial_settings.parity = UART_ODD_PARITY;
      else if (! grub_strcmp (state[4].arg, "even"))
	serial_settings.parity = UART_EVEN_PARITY;
      else
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad parity");
	}
    }
  
  if (state[5].set)
    {
      if (! grub_strcmp (state[5].arg, "1"))
	serial_settings.stop_bits = UART_1_STOP_BIT;
      else if (! grub_strcmp (state[5].arg, "2"))
	serial_settings.stop_bits = UART_2_STOP_BITS;
      else
	{
	  serial_settings = backup_settings;
	  return grub_error (GRUB_ERR_BAD_ARGUMENT, "bad number of stop bits");
	}
    }

  /* Initialize with new settings.  */
  hwiniterr = serial_hw_init ();
  
  if (hwiniterr == GRUB_ERR_NONE)
    {
      /* Register terminal if not yet registered.  */
      if (registered == 0)
	{
	  grub_term_register (&grub_serial_term);
	  registered = 1;
	}
    }
  else
    {
      /* Initialization with new settings failed.  */
      if (registered == 1)
	{
	  /* If the terminal is registered, attempt to restore previous
	     settings.  */
	  serial_settings = backup_settings;
	  if (serial_hw_init () != GRUB_ERR_NONE)
	    {
	      /* If unable to restore settings, unregister terminal.  */
	      grub_term_unregister (&grub_serial_term);
	      registered = 0;
	    }
	}
    }
  
  return hwiniterr;
}

GRUB_MOD_INIT
{
  (void) mod;			/* To stop warning. */
  grub_register_command ("serial", grub_cmd_serial, GRUB_COMMAND_FLAG_BOTH,
                         "serial [OPTIONS...]", "Configure serial port.", options);
  /* Set default settings.  */
  serial_settings.port      = serial_hw_get_port (0);
  serial_settings.divisor   = serial_get_divisor (9600);
  serial_settings.word_len  = UART_8BITS_WORD;
  serial_settings.parity    = UART_NO_PARITY;
  serial_settings.stop_bits = UART_1_STOP_BIT;
}

GRUB_MOD_FINI
{
  grub_unregister_command ("serial");
  if (registered == 1)		/* Unregister terminal only if registered. */
    grub_term_unregister (&grub_serial_term);
}
