/* reader.h - prototypes for command line reader.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009  Free Software Foundation, Inc.
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

#ifndef GRUB_READER_HEADER
#define GRUB_READER_HEADER	1

#include <grub/types.h>
#include <grub/err.h>
#include <grub/handler.h>

typedef grub_err_t (*grub_reader_getline_t) (char **, int);

struct grub_reader
{
  /* The next reader.  */
  struct grub_parser *next;

  /* The reader name.  */
  const char *name;

  /* Initialize the reader.  */
  grub_err_t (*init) (void);

  /* Clean up the reader.  */
  grub_err_t (*fini) (void);

  grub_reader_getline_t read_line;
};
typedef struct grub_reader *grub_reader_t;

extern struct grub_handler_class EXPORT_VAR(grub_reader_class);

grub_err_t EXPORT_FUNC(grub_reader_loop) (grub_reader_getline_t getline);

static inline void
grub_reader_register (const char *name __attribute__ ((unused)),
		      grub_reader_t reader)
{
  grub_handler_register (&grub_reader_class, GRUB_AS_HANDLER (reader));
}

static inline void
grub_reader_unregister (grub_reader_t reader)
{
  grub_handler_unregister (&grub_reader_class, GRUB_AS_HANDLER (reader));
}

static inline grub_reader_t
grub_reader_get_current (void)
{
  return (grub_reader_t) grub_reader_class.cur_handler;
}

static inline grub_err_t
grub_reader_set_current (grub_reader_t reader)
{
  return grub_handler_set_current (&grub_reader_class,
				   GRUB_AS_HANDLER (reader));
}

void grub_register_rescue_reader (void);

#endif /* ! GRUB_READER_HEADER */
