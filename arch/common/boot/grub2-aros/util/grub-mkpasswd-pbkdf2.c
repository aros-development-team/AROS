/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1992-1999,2001,2003,2004,2005,2009 Free Software Foundation, Inc.
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

#include <config.h>

#include <grub/types.h>
#include <grub/crypto.h>
#include <grub/emu/misc.h>
#include <grub/util/misc.h>
#include <grub/i18n.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>

#include "progname.h"

static struct option options[] =
  {
    {"iteration_count", required_argument, 0, 'c'},
    {"buflen", required_argument, 0, 'l'},
    {"saltlen", required_argument, 0, 's'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
  };

static void
usage (int status)
{
  if (status)
    fprintf (stderr, "Try `%s --help' for more information.\n", program_name);
  else
    printf ("\
Usage: %s [OPTIONS]\n\
\nOptions:\n\
     -c number, --iteration-count=number  Number of PBKDF2 iterations\n\
     -l number, --buflen=number           Length of generated hash\n\
     -s number, --salt=number             Length of salt\n\
\n\
Report bugs to <%s>.\n", program_name, PACKAGE_BUGREPORT);

  exit (status);
}

static void
hexify (char *hex, grub_uint8_t *bin, grub_size_t n)
{
  while (n--)
    {
      if (((*bin & 0xf0) >> 4) < 10)
	*hex = ((*bin & 0xf0) >> 4) + '0';
      else
	*hex = ((*bin & 0xf0) >> 4) + 'A' - 10;
      hex++;

      if ((*bin & 0xf) < 10)
	*hex = (*bin & 0xf) + '0';
      else
	*hex = (*bin & 0xf) + 'A' - 10;
      hex++;
      bin++;
    }
  *hex = 0;
}

int
main (int argc, char *argv[])
{
  unsigned int count = 10000, buflen = 64, saltlen = 64;
  char *pass1, *pass2;
  char *bufhex, *salthex;
  gcry_err_code_t gcry_err;
  grub_uint8_t *buf, *salt;
  ssize_t nr;
  FILE *in, *out;
  struct termios s, t;
  int tty_changed;

  set_program_name (argv[0]);

  grub_util_init_nls ();

  /* Check for options.  */
  while (1)
    {
      int c = getopt_long (argc, argv, "c:l:s:hvV", options, 0);

      if (c == -1)
	break;

      switch (c)
	{
	case 'c':
	  count = strtoul (optarg, NULL, 0);
	  break;

	case 'l':
	  buflen = strtoul (optarg, NULL, 0);
	  break;

	case 's':
	  saltlen = strtoul (optarg, NULL, 0);
	  break;

	case 'h':
	  usage (0);
	  return 0;
	  
	case 'V':
	  printf ("%s (%s) %s\n", program_name,
		  PACKAGE_NAME, PACKAGE_VERSION);
	  return 0;
	    
	default:
	  usage (1);
	  return 1;
	}
    }

  bufhex = malloc (buflen * 2 + 1);
  if (!bufhex)
    grub_util_error ("out of memory");
  buf = malloc (buflen);
  if (!buf)
    {
      free (bufhex);
      grub_util_error ("out of memory");
    }

  salt = malloc (saltlen);
  if (!salt)
    {
      free (bufhex);
      free (buf);
      grub_util_error ("out of memory");
    }
  salthex = malloc (saltlen * 2 + 1);
  if (!salthex)
    {
      free (salt);
      free (bufhex);
      free (buf);
      grub_util_error ("out of memory");
    }

  /* Disable echoing. Based on glibc.  */
  in = fopen ("/dev/tty", "w+c");
  if (in == NULL)
    {
      in = stdin;
      out = stderr;
    }
  else
    out = in;

  if (tcgetattr (fileno (in), &t) == 0)
    {
      /* Save the old one. */
      s = t;
      /* Tricky, tricky. */
      t.c_lflag &= ~(ECHO|ISIG);
      tty_changed = (tcsetattr (fileno (in), TCSAFLUSH, &t) == 0);
    }
  else
    tty_changed = 0;
  
  printf ("Enter password: ");
  pass1 = NULL;
  {
    grub_size_t n;
    nr = getline (&pass1, &n, stdin);
  }
  if (nr < 0 || !pass1)
    {
      free (buf);
      free (bufhex);
      free (salthex);
      free (salt);
      /* Restore the original setting.  */
      if (tty_changed)
	(void) tcsetattr (fileno (in), TCSAFLUSH, &s);
      grub_util_error ("failure to read password");
    }
  if (nr >= 1 && pass1[nr-1] == '\n')
    pass1[nr-1] = 0;

  printf ("\nReenter password: ");
  pass2 = NULL;
  {
    grub_size_t n;
    nr = getline (&pass2, &n, stdin);
  }
  /* Restore the original setting.  */
  if (tty_changed)
    (void) tcsetattr (fileno (in), TCSAFLUSH, &s);
  printf ("\n");

  if (nr < 0 || !pass2)
    {
      memset (pass1, 0, strlen (pass1));
      free (pass1);
      free (buf);
      free (bufhex);
      free (salthex);
      free (salt);
      grub_util_error ("failure to read password");
    }
  if (nr >= 1 && pass2[nr-1] == '\n')
    pass2[nr-1] = 0;

  if (strcmp (pass1, pass2) != 0)
    {
      memset (pass1, 0, strlen (pass1));
      memset (pass2, 0, strlen (pass2));
      free (pass1);
      free (pass2);
      free (buf);
      free (bufhex);
      free (salthex);
      free (salt);
      grub_util_error ("passwords don't match");
    }
  memset (pass2, 0, strlen (pass2));
  free (pass2);

#if ! defined (__linux__) && ! defined (__FreeBSD__)
  printf ("WARNING: your random generator isn't known to be secure\n");
#endif

  {
    FILE *f;
    size_t rd;
    f = fopen ("/dev/random", "rb");
    if (!f)
      {
	memset (pass1, 0, strlen (pass1));
	free (pass1);
	free (buf);
	free (bufhex);
	free (salthex);
	free (salt);
	fclose (f);
	grub_util_error ("couldn't retrieve random data for salt");
      }
    rd = fread (salt, 1, saltlen, f);
    if (rd != saltlen)
      {
	fclose (f);
	memset (pass1, 0, strlen (pass1));
	free (pass1);
	free (buf);
	free (bufhex);
	free (salthex);
	free (salt);
	fclose (f);
	grub_util_error ("couldn't retrieve random data for salt");
      }
    fclose (f);
  }

  gcry_err = grub_crypto_pbkdf2 (GRUB_MD_SHA512,
				 (grub_uint8_t *) pass1, strlen (pass1),
				 salt, saltlen,
				 count, buf, buflen);
  memset (pass1, 0, strlen (pass1));
  free (pass1);

  if (gcry_err)
    {
      memset (buf, 0, buflen);
      memset (bufhex, 0, 2 * buflen);
      free (buf);
      free (bufhex);
      memset (salt, 0, saltlen);
      memset (salthex, 0, 2 * saltlen);
      free (salt);
      free (salthex);
      grub_util_error ("cryptographic error number %d", gcry_err);
    }

  hexify (bufhex, buf, buflen);
  hexify (salthex, salt, saltlen);

  printf ("Your PBKDF2 is grub.pbkdf2.sha512.%d.%s.%s\n",
	  count, salthex, bufhex);
  memset (buf, 0, buflen);
  memset (bufhex, 0, 2 * buflen);
  free (buf);
  free (bufhex);
  memset (salt, 0, saltlen);
  memset (salthex, 0, 2 * saltlen);
  free (salt);
  free (salthex);

  return 0;
}
