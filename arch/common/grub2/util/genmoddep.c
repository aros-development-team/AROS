/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002  Free Software Foundation, Inc.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define BUF_SIZE	1024
#define SYMTAB_SIZE	509

struct symbol
{
  const char *name;
  const char *mod;
  struct symbol *next;
};

struct module
{
  const char *name;
  struct module *next;
};

static char buf[BUF_SIZE];
static struct symbol *symtab[SYMTAB_SIZE];

static void
err (const char *fmt, ...)
{
  va_list ap;

  fprintf (stderr, "genmoddep: error: ");
  
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);

  fputc ('\n', stderr);
  exit (1);
}

static void *
xmalloc (size_t size)
{
  void *p;

  p = malloc (size);
  if (! p)
    err ("out of memory");

  return p;
}

static char *
xstrdup (const char *str)
{
  char *s;
  size_t len;

  len = strlen (str);
  s = (char *) xmalloc (len + 1);
  memcpy (s, str, len + 1);

  return s;
}

static void
chomp (char *str)
{
  int end;
  
  end = strlen (str) - 1;
  if (end < 0)
    err ("empty string");

  if (str[end] == '\n')
    str[end] = '\0';
}

static unsigned
symbol_hash (const char *s)
{
  unsigned key = 0;

  while (*s)
    key = key * 65599 + *s++;

  return (key + (key >> 5)) % SYMTAB_SIZE;
}

static struct symbol *
get_symbol (const char *name)
{
  unsigned k;
  struct symbol *sym;
  
  k = symbol_hash (name);
  for (sym = symtab[k]; sym; sym = sym->next)
    if (strcmp (sym->name, name) == 0)
      return sym;

  return 0;
}

static void
add_symbol (const char *name, const char *mod)
{
  unsigned k;
  struct symbol *sym;

  if (get_symbol (name))
    err ("duplicated symbol: %s", name);
  
  sym = (struct symbol *) xmalloc (sizeof (*sym));
  sym->name = xstrdup (name);
  sym->mod = xstrdup (mod);

  k = symbol_hash (name);
  sym->next = symtab[k];
  symtab[k] = sym;
}

static void
free_symbols (void)
{
  int i;

  for (i = 0; i < SYMTAB_SIZE; i++)
    {
      struct symbol *p, *q;

      p = symtab[i];
      while (p)
	{
	  q = p->next;
	  free ((void *) p->name);
	  free ((void *) p->mod);
	  free (p);
	  p = q;
	}
    }
}

static void
read_defined_symbols (FILE *fp)
{
  while (fgets (buf, sizeof (buf), fp))
    {
      char *p;

      if (! *buf)
	err ("empty symbol name: %s", buf);
      
      p = strchr (buf, ' ');
      if (! p)
	err ("invalid line format: %s", buf);

      p++;
      
      if (! *p)
	err ("empty module name: %s", buf);

      *(p - 1) = '\0';
      chomp (p);
      
      add_symbol (buf, p);
    }
}

static void
add_module (struct module **head, const char *name)
{
  struct module *mod;

  for (mod = *head; mod; mod = mod->next)
    if (strcmp (mod->name, name) == 0)
      return;

  mod = (struct module *) xmalloc (sizeof (*mod));
  mod->name = xstrdup (name);

  mod->next = *head;
  *head = mod;
}

static void
free_modules (struct module *head)
{
  struct module *next;

  while (head)
    {
      next = head->next;
      free ((void *) head->name);
      free (head);
      head = next;
    }
}

static void
find_dependencies (FILE *fp)
{
  char *mod_name;
  struct module *mod_list = 0;
  struct module *mod;
  
  if (! fgets (buf, sizeof (buf), fp) || buf[0] == '\n' || buf[0] == '\0')
    err ("no module name");

  chomp (buf);
  mod_name = xstrdup (buf);

  while (fgets (buf, sizeof (buf), fp))
    {
      struct symbol *sym;
      
      chomp (buf);
      sym = get_symbol (buf);
      if (! sym)
	err ("%s in %s is not defined", buf, mod_name);

      add_module (&mod_list, sym->mod);
    }

  printf ("%s:", mod_name);
  
  for (mod = mod_list; mod; mod = mod->next)
    if (strcmp (mod->name, "kernel") != 0)
      printf (" %s", mod->name);
  
  putchar ('\n');

  free_modules (mod_list);
}

int
main (int argc, char *argv[])
{
  int i;
  
  /* First, get defined symbols.  */
  read_defined_symbols (stdin);

  /* Second, find the dependecies.  */
  for (i = 1; i < argc; i++)
    {
      FILE *fp;

      fp = fopen (argv[i], "r");
      if (! fp)
	err ("cannot open %s", argv[i]);

      find_dependencies (fp);

      fclose (fp);
    }

  /* Last, free memory.  */
  free_symbols ();
  
  return 0;
}
