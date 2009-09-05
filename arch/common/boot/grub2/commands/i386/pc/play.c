/* play.c - command to play a tune  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2005,2007  Free Software Foundation, Inc.
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

/* Lots of this file is borrowed from GNU/Hurd generic-speaker driver.  */

#include <grub/dl.h>
#include <grub/file.h>
#include <grub/disk.h>
#include <grub/term.h>
#include <grub/misc.h>
#include <grub/machine/time.h>
#include <grub/cpu/io.h>
#include <grub/command.h>

#define BASE_TEMPO 120

/* The speaker port.  */
#define SPEAKER			0x61

/* If 0, follow state of SPEAKER_DATA bit, otherwise enable output
   from timer 2.  */
#define SPEAKER_TMR2		0x01

/* If SPEAKER_TMR2 is not set, this provides direct input into the
   speaker.  Otherwise, this enables or disables the output from the
   timer.  */
#define SPEAKER_DATA		0x02

/* The PIT channel value ports.  You can write to and read from them.
   Do not mess with timer 0 or 1.  */
#define PIT_COUNTER_0		0x40
#define PIT_COUNTER_1		0x41
#define PIT_COUNTER_2		0x42

/* The frequency of the PIT clock.  */
#define PIT_FREQUENCY		0x1234dd

/* The PIT control port.  You can only write to it.  Do not mess with
   timer 0 or 1.  */
#define PIT_CTRL		0x43
#define PIT_CTRL_SELECT_MASK	0xc0
#define PIT_CTRL_SELECT_0	0x00
#define PIT_CTRL_SELECT_1	0x40
#define PIT_CTRL_SELECT_2	0x80

/* Read and load control.  */
#define PIT_CTRL_READLOAD_MASK	0x30
#define PIT_CTRL_COUNTER_LATCH	0x00	/* Hold timer value until read.  */
#define PIT_CTRL_READLOAD_LSB	0x10	/* Read/load the LSB.  */
#define PIT_CTRL_READLOAD_MSB	0x20	/* Read/load the MSB.  */
#define PIT_CTRL_READLOAD_WORD	0x30	/* Read/load the LSB then the MSB.  */

/* Mode control.  */
#define PIT_CTRL_MODE_MASK	0x0e

/* Interrupt on terminal count.  Setting the mode sets output to low.
   When counter is set and terminated, output is set to high.  */
#define PIT_CTRL_INTR_ON_TERM	0x00

/* Programmable one-shot.  When loading counter, output is set to
   high.  When counter terminated, output is set to low.  Can be
   triggered again from that point on by setting the gate pin to
   high.  */
#define PIT_CTRL_PROGR_ONE_SHOT	0x02

/* Rate generator.  Output is low for one period of the counter, and
   high for the other.  */
#define PIT_CTRL_RATE_GEN	0x04

/* Square wave generator.  Output is low for one half of the period,
   and high for the other half.  */
#define PIT_CTRL_SQUAREWAVE_GEN	0x06

/* Software triggered strobe.  Setting the mode sets output to high.
   When counter is set and terminated, output is set to low.  */
#define PIT_CTRL_SOFTSTROBE	0x08

/* Hardware triggered strobe.  Like software triggered strobe, but
   only starts the counter when the gate pin is set to high.  */
#define PIT_CTRL_HARDSTROBE	0x0a

/* Count mode.  */
#define PIT_CTRL_COUNT_MASK	0x01
#define PIT_CTRL_COUNT_BINARY	0x00	/* 16-bit binary counter.  */
#define PIT_CTRL_COUNT_BCD	0x01	/* 4-decade BCD counter.  */

#define T_REST			((short) 0)
#define T_FINE			((short) -1)

struct note
{
  short pitch;
  short duration;
};

static void
beep_off (void)
{
  unsigned char status;

  status = grub_inb (SPEAKER);
  grub_outb (status & ~(SPEAKER_TMR2 | SPEAKER_DATA), SPEAKER);
}

static void
beep_on (short pitch)
{
  unsigned char status;
  unsigned int counter;

  if (pitch < 20)
    pitch = 20;
  else if (pitch > 20000)
    pitch = 20000;

  counter = PIT_FREQUENCY / pitch;

  /* Program timer 2.  */
  grub_outb (PIT_CTRL_SELECT_2 | PIT_CTRL_READLOAD_WORD
	| PIT_CTRL_SQUAREWAVE_GEN | PIT_CTRL_COUNT_BINARY, PIT_CTRL);
  grub_outb (counter & 0xff, PIT_COUNTER_2);		/* LSB */
  grub_outb ((counter >> 8) & 0xff, PIT_COUNTER_2);	/* MSB */

  /* Start speaker.  */
  status = grub_inb (SPEAKER);
  grub_outb (status | SPEAKER_TMR2 | SPEAKER_DATA, SPEAKER);
}

static grub_err_t
grub_cmd_play (grub_command_t cmd __attribute__ ((unused)),
	       int argc, char **args)
{
  grub_file_t file;
  struct note buf;
  int tempo;
  unsigned int to;

  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "file name required");

  file = grub_file_open (args[0]);
  if (! file)
    return grub_error (GRUB_ERR_FILE_NOT_FOUND, "file not found");

  if (grub_file_read (file, &tempo, sizeof(tempo)) != sizeof(tempo))
    {
      grub_file_close (file);
      return grub_error (GRUB_ERR_FILE_READ_ERROR,
                         "file doesn't even contains a full tempo record");
    }

  grub_dprintf ("play","tempo = %d\n", tempo);

  while (grub_file_read (file, &buf,
                         sizeof (struct note)) == sizeof (struct note)
         && buf.pitch != T_FINE && grub_checkkey () < 0)
    {

      grub_dprintf ("play", "pitch = %d, duration = %d\n", buf.pitch,
                    buf.duration);

      switch (buf.pitch)
        {
          case T_REST:
            beep_off ();
            break;

          default:
            beep_on (buf.pitch);
            break;
        }

      to = grub_get_rtc () + BASE_TEMPO * buf.duration / tempo;
      while (((unsigned int) grub_get_rtc () <= to) && (grub_checkkey () < 0))
        ;

    }

  beep_off ();

  grub_file_close (file);

  while (grub_checkkey () > 0)
    grub_getkey ();

  return 0;
}

static grub_command_t cmd;

GRUB_MOD_INIT(play)
{
  cmd = grub_register_command ("play", grub_cmd_play,
			       "play FILE", "Play a tune");
}

GRUB_MOD_FINI(play)
{
  grub_unregister_command (cmd);
}
