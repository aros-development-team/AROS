/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005  Free Software Foundation, Inc.
 *
 *  GRUB is free software; you can redistribute it and/or modify
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
 *  along with GRUB; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GRUB_TERM_HEADER
#define GRUB_TERM_HEADER	1

#include <grub/err.h>
#include <grub/symbol.h>
#include <grub/types.h>

/* These are used to represent the various color states we use.  */
typedef enum
  {
    /* The color used to display all text that does not use the
       user defined colors below.  */
    GRUB_TERM_COLOR_STANDARD,
    /* The user defined colors for normal text.  */
    GRUB_TERM_COLOR_NORMAL,
    /* The user defined colors for highlighted text.  */
    GRUB_TERM_COLOR_HIGHLIGHT
  }
grub_term_color_state;

/* Flags for representing the capabilities of a terminal.  */
/* Some notes about the flags:
   - These flags are used by higher-level functions but not terminals
   themselves.
   - If a terminal is dumb, you may assume that only putchar, getkey and
   checkkey are called.
   - Some fancy features (setcolorstate, setcolor and setcursor) can be set
   to NULL.  */

/* Set when input characters shouldn't be echoed back.  */
#define GRUB_TERM_NO_ECHO	(1 << 0)
/* Set when the editing feature should be disabled.  */
#define GRUB_TERM_NO_EDIT	(1 << 1)
/* Set when the terminal cannot do fancy things.  */
#define GRUB_TERM_DUMB		(1 << 2)
/* Set when the terminal needs to be initialized.  */
#define GRUB_TERM_NEED_INIT	(1 << 16)


/* Unicode characters for fancy graphics.  */
#define GRUB_TERM_DISP_LEFT	0x2190
#define GRUB_TERM_DISP_UP	0x2191
#define GRUB_TERM_DISP_RIGHT	0x2192
#define GRUB_TERM_DISP_DOWN	0x2193
#define GRUB_TERM_DISP_HLINE	0x2501
#define GRUB_TERM_DISP_VLINE	0x2503
#define GRUB_TERM_DISP_UL	0x250F
#define GRUB_TERM_DISP_UR	0x2513
#define GRUB_TERM_DISP_LL	0x2517
#define GRUB_TERM_DISP_LR	0x251B


/* Menu-related geometrical constants.  */

/* FIXME: Ugly way to get them form terminal.  */
#define GRUB_TERM_WIDTH         ((grub_getwh()&0xFF00)>>8)
#define GRUB_TERM_HEIGHT        (grub_getwh()&0xFF)

/* The number of lines of "GRUB version..." at the top.  */
#define GRUB_TERM_INFO_HEIGHT	1

/* The number of columns/lines between messages/borders/etc.  */
#define GRUB_TERM_MARGIN	1

/* The number of columns of scroll information.  */
#define GRUB_TERM_SCROLL_WIDTH	1

/* The Y position of the top border.  */
#define GRUB_TERM_TOP_BORDER_Y	(GRUB_TERM_MARGIN + GRUB_TERM_INFO_HEIGHT \
                                 + GRUB_TERM_MARGIN)

/* The X position of the left border.  */
#define GRUB_TERM_LEFT_BORDER_X	GRUB_TERM_MARGIN

/* The width of the border.  */
#define GRUB_TERM_BORDER_WIDTH	(GRUB_TERM_WIDTH \
                                 - GRUB_TERM_MARGIN * 3 \
				 - GRUB_TERM_SCROLL_WIDTH)

/* The number of lines of messages at the bottom.  */
#define GRUB_TERM_MESSAGE_HEIGHT	8

/* The height of the border.  */
#define GRUB_TERM_BORDER_HEIGHT	(GRUB_TERM_HEIGHT \
                                 - GRUB_TERM_TOP_BORDER_Y \
                                 - GRUB_TERM_MESSAGE_HEIGHT)

/* The number of entries shown at a time.  */
#define GRUB_TERM_NUM_ENTRIES	(GRUB_TERM_BORDER_HEIGHT - 2)

/* The Y position of the first entry.  */
#define GRUB_TERM_FIRST_ENTRY_Y	(GRUB_TERM_TOP_BORDER_Y + 1)

/* The max column number of an entry. The last "-1" is for a
   continuation marker.  */
#define GRUB_TERM_ENTRY_WIDTH	(GRUB_TERM_BORDER_WIDTH - 2 \
                                 - GRUB_TERM_MARGIN * 2 - 1)

/* The standard X position of the cursor.  */
#define GRUB_TERM_CURSOR_X	(GRUB_TERM_LEFT_BORDER_X \
                                 + GRUB_TERM_BORDER_WIDTH \
                                 - GRUB_TERM_MARGIN \
                                 - 1)


struct grub_term
{
  /* The terminal name.  */
  const char *name;

  /* Initialize the terminal.  */
  grub_err_t (*init) (void);

  /* Clean up the terminal.  */
  grub_err_t (*fini) (void);
  
  /* Put a character. C is encoded in Unicode.  */
  void (*putchar) (grub_uint32_t c);

  /* Get the number of columns occupied by a given character C. C is
     encoded in Unicode.  */
  grub_ssize_t (*getcharwidth) (grub_uint32_t c);
  
  /* Check if any input character is available.  */
  int (*checkkey) (void);
  
  /* Get a character.  */
  int (*getkey) (void);
  
  /* Get the screen size. The return value is ((Width << 8) | Height).  */
  grub_uint16_t (*getwh) (void);

  /* Get the cursor position. The return value is ((X << 8) | Y).  */
  grub_uint16_t (*getxy) (void);
  
  /* Go to the position (X, Y).  */
  void (*gotoxy) (grub_uint8_t x, grub_uint8_t y);
  
  /* Clear the screen.  */
  void (*cls) (void);
  
  /* Set the current color to be used */
  void (*setcolorstate) (grub_term_color_state state);
  
  /* Set the normal color and the highlight color. The format of each
     color is VGA's.  */
  void (*setcolor) (grub_uint8_t normal_color, grub_uint8_t highlight_color);
  
  /* Turn on/off the cursor.  */
  void (*setcursor) (int on);

  /* Update the screen.  */
  void (*refresh) (void);

  /* The feature flags defined above.  */
  grub_uint32_t flags;
  
  /* The next terminal.  */
  struct grub_term *next;
};
typedef struct grub_term *grub_term_t;

void EXPORT_FUNC(grub_term_register) (grub_term_t term);
void EXPORT_FUNC(grub_term_unregister) (grub_term_t term);
void EXPORT_FUNC(grub_term_iterate) (int (*hook) (grub_term_t term));

grub_err_t EXPORT_FUNC(grub_term_set_current) (grub_term_t term);
grub_term_t EXPORT_FUNC(grub_term_get_current) (void);

void EXPORT_FUNC(grub_putchar) (int c);
void EXPORT_FUNC(grub_putcode) (grub_uint32_t code);
grub_ssize_t EXPORT_FUNC(grub_getcharwidth) (grub_uint32_t code);
int EXPORT_FUNC(grub_getkey) (void);
int EXPORT_FUNC(grub_checkkey) (void);
grub_uint16_t EXPORT_FUNC(grub_getwh) (void);
grub_uint16_t EXPORT_FUNC(grub_getxy) (void);
void EXPORT_FUNC(grub_gotoxy) (grub_uint8_t x, grub_uint8_t y);
void EXPORT_FUNC(grub_cls) (void);
void EXPORT_FUNC(grub_setcolorstate) (grub_term_color_state state);
void EXPORT_FUNC(grub_setcolor) (grub_uint8_t normal_color,
				 grub_uint8_t highlight_color);
int EXPORT_FUNC(grub_setcursor) (int on);
int EXPORT_FUNC(grub_getcursor) (void);
void EXPORT_FUNC(grub_refresh) (void);
void EXPORT_FUNC(grub_set_more) (int onoff);

/* For convenience.  */
#define GRUB_TERM_ASCII_CHAR(c)	((c) & 0xff)

#endif /* ! GRUB_TERM_HEADER */
