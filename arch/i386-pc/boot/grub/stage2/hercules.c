/* hercules.c - hercules console interface */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2001  Free Software Foundation, Inc.
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

#ifdef SUPPORT_HERCULES

#include <shared.h>
#include <hercules.h>

/* The position of the cursor.  */
static unsigned hercx, hercy;

/* Write a byte to a port.  */
static inline void
outb (unsigned short port, unsigned char value)
{
  asm volatile ("outb	%b0, %w1" : : "a" (value), "Nd" (port));
}

static void
herc_set_cursor (void)
{
  unsigned offset = hercy * HERCULES_WIDTH + hercx;
  
  outb (HERCULES_INDEX_REG, 0x0f);
  outb (0x80, 0x0f);
  outb (HERCULES_DATA_REG, offset & 0xFF);
  outb (0x80, offset & 0xFF);
  
  outb (HERCULES_INDEX_REG, 0x0e);
  outb (0x80, 0x0e);
  outb (HERCULES_DATA_REG, offset >> 8);
  outb (0x80, offset >> 8);
}

void
herc_putchar (int c)
{
  
  switch (c)
    {
    case '\b':
      if (hercx)
	hercx--;
      break;
      
    case '\n':
      hercy++;
      break;
      
    case '\r':
      hercx = 0;
      break;

    default:
      {
	volatile unsigned short *video
	  = (unsigned short *) HERCULES_VIDEO_ADDR;
	
	video[hercy * HERCULES_WIDTH + hercx] = 0x0700 | c;
	hercx++;
	if (hercx >= HERCULES_WIDTH)
	  {
	    hercx = 0;
	    hercy++;
	  }
      }
      break;
    }

  if (hercy >= HERCULES_HEIGHT)
    {
      int i;
      volatile unsigned long *video = (unsigned long *) HERCULES_VIDEO_ADDR;
      
      hercy = HERCULES_HEIGHT - 1;
      grub_memmove ((char *) HERCULES_VIDEO_ADDR,
		    (char *) HERCULES_VIDEO_ADDR + HERCULES_WIDTH * 2,
		    HERCULES_WIDTH * (HERCULES_HEIGHT - 1) * 2);
      for (i = HERCULES_WIDTH * (HERCULES_HEIGHT - 1) / 2;
	   i < HERCULES_WIDTH * HERCULES_HEIGHT / 2;
	   i++)
	video[i] = 0x07200720;
    }
}

void
herc_cls (void)
{
  int i;
  volatile unsigned long *video = (unsigned long *) HERCULES_VIDEO_ADDR;
  
  for (i = 0; i < HERCULES_WIDTH * HERCULES_HEIGHT / 2; i++)
    video[i] = 0x07200720;

  hercx = hercy = 0;
  herc_set_cursor ();
}

int
herc_getxy (void)
{
  return (hercx << 8) | hercy;
}

void
herc_gotoxy (int x, int y)
{
  hercx = x;
  hercy = y;
  herc_set_cursor ();
}

void
herc_set_attrib (int attr)
{
  volatile unsigned char *video = (unsigned char *) HERCULES_VIDEO_ADDR;
  
  if (attr & 0xF0)
    attr = 0x70;
  else
    attr = 0x07;

  video[((hercy * HERCULES_WIDTH + hercx) << 1) + 1] = attr;
}

#endif /* SUPPORT_HERCULES */
