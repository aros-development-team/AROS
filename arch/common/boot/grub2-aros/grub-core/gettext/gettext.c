/* gettext.c - gettext module */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2009 Free Software Foundation, Inc.
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

#include <grub/list.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/mm.h>
#include <grub/err.h>
#include <grub/dl.h>
#include <grub/normal.h>
#include <grub/file.h>
#include <grub/kernel.h>
#include <grub/i18n.h>

/*
   .mo file information from:
   http://www.gnu.org/software/autoconf/manual/gettext/MO-Files.html .
*/


static grub_file_t fd_mo;

static int grub_gettext_offsetoriginal;
static int grub_gettext_max;

static const char *(*grub_gettext_original) (const char *s);

struct grub_gettext_msg
{
  struct grub_gettext_msg *next;
  const char *name;

  const char *translated;
};

struct grub_gettext_msg *grub_gettext_msg_list = NULL;

#define GETTEXT_MAGIC_NUMBER 		0
#define GETTEXT_FILE_FORMAT		4
#define GETTEXT_NUMBER_OF_STRINGS 	8
#define GETTEXT_OFFSET_ORIGINAL 	12
#define GETTEXT_OFFSET_TRANSLATION 	16

#define MO_MAGIC_NUMBER 		0x950412de

static grub_ssize_t
grub_gettext_pread (grub_file_t file, void *buf, grub_size_t len,
		    grub_off_t offset)
{
  if (grub_file_seek (file, offset) == (grub_off_t) - 1)
    {
      return -1;
    }
  return grub_file_read (file, buf, len);
}

static grub_uint32_t
grub_gettext_get_info (int offset)
{
  grub_uint32_t value;

  grub_gettext_pread (fd_mo, (char *) &value, 4, offset);

  value = grub_cpu_to_le32 (value);
  return value;
}

static void
grub_gettext_getstring_from_offset (grub_uint32_t offset,
				    grub_uint32_t length, char *translation)
{
  grub_gettext_pread (fd_mo, translation, length, offset);
  translation[length] = '\0';
}

static const char *
grub_gettext_gettranslation_from_position (int position)
{
  int offsettranslation;
  int internal_position;
  grub_uint32_t length, offset;
  char *translation;

  offsettranslation = grub_gettext_get_info (GETTEXT_OFFSET_TRANSLATION);

  internal_position = offsettranslation + position * 8;

  grub_gettext_pread (fd_mo, (char *) &length, 4, internal_position);
  length = grub_cpu_to_le32 (length);

  grub_gettext_pread (fd_mo, (char *) &offset, 4, internal_position + 4);
  offset = grub_cpu_to_le32 (offset);

  translation = grub_malloc (length + 1);
  grub_gettext_getstring_from_offset (offset, length, translation);

  return translation;
}

static char *
grub_gettext_getstring_from_position (int position)
{
  int internal_position;
  int length, offset;
  char *original;

  /* Get position for string i.  */
  internal_position = grub_gettext_offsetoriginal + (position * 8);

  /* Get the length of the string i.  */
  grub_gettext_pread (fd_mo, (char *) &length, 4, internal_position);

  /* Get the offset of the string i.  */
  grub_gettext_pread (fd_mo, (char *) &offset, 4, internal_position + 4);

  /* Get the string i.  */
  original = grub_malloc (length + 1);
  grub_gettext_getstring_from_offset (offset, length, original);

  return original;
}

static const char *
grub_gettext_translate (const char *orig)
{
  char *current_string;
  const char *ret;

  int min, max, current;
  int found = 0;

  struct grub_gettext_msg *cur;

  /* Make sure we can use grub_gettext_translate for error messages.  Push
     active error message to error stack and reset error message.  */
  grub_error_push ();

  cur = grub_named_list_find (GRUB_AS_NAMED_LIST (grub_gettext_msg_list),
			      orig);

  if (cur)
    {
      grub_error_pop ();
      return cur->translated;
    }

  if (fd_mo == 0)
    {
      grub_error_pop ();
      return orig;
    }

  min = 0;
  max = grub_gettext_max;

  current = (max + min) / 2;

  while (current != min && current != max && found == 0)
    {
      current_string = grub_gettext_getstring_from_position (current);

      /* Search by bisection.  */
      if (grub_strcmp (current_string, orig) < 0)
	{
	  grub_free (current_string);
	  min = current;
	}
      else if (grub_strcmp (current_string, orig) > 0)
	{
	  grub_free (current_string);
	  max = current;
	}
      else if (grub_strcmp (current_string, orig) == 0)
	{
	  grub_free (current_string);
	  found = 1;
	}
      current = (max + min) / 2;
    }

  ret = found ? grub_gettext_gettranslation_from_position (current) : orig;

  if (found)
    {
      cur = grub_zalloc (sizeof (*cur));

      if (cur)
	{
	  cur->name = grub_strdup (orig);
	  if (cur->name)
	    {
	      cur->translated = ret;
	      grub_list_push (GRUB_AS_LIST_P (&grub_gettext_msg_list),
			      GRUB_AS_LIST (cur));
	    }
	}
      else
	grub_errno = GRUB_ERR_NONE;
    }

  grub_error_pop ();
  return ret;
}

/* This is similar to grub_file_open. */
static grub_file_t
grub_mofile_open (const char *filename)
{
  int unsigned magic;
  int version;

  /* Using fd_mo and not another variable because
     it's needed for grub_gettext_get_info.  */

  fd_mo = grub_file_open (filename);
  grub_errno = GRUB_ERR_NONE;

  if (!fd_mo)
    {
      grub_dprintf ("gettext", "Cannot read %s\n", filename);
      return 0;
    }

  magic = grub_gettext_get_info (GETTEXT_MAGIC_NUMBER);

  if (magic != MO_MAGIC_NUMBER)
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE, "mo: invalid mo file: %s",
		  filename);
      grub_file_close (fd_mo);
      fd_mo = 0;
      return 0;
    }

  version = grub_gettext_get_info (GETTEXT_FILE_FORMAT);

  if (version != 0)
    {
      grub_error (GRUB_ERR_BAD_FILE_TYPE,
		  "mo: invalid mo version in file: %s", filename);
      fd_mo = 0;
      return 0;
    }

  return fd_mo;
}

/* Returning grub_file_t would be more natural, but grub_mofile_open assigns
   to fd_mo anyway ...  */
static void
grub_mofile_open_lang (const char *locale_dir, const char *locale)
{
  char *mo_file;

  /* mo_file e.g.: /boot/grub/locale/ca.mo   */

  mo_file = grub_xasprintf ("%s/%s.mo", locale_dir, locale);
  if (!mo_file)
    return;

  fd_mo = grub_mofile_open (mo_file);

  /* Will try adding .gz as well.  */
  if (fd_mo == NULL)
    {
      char *mo_file_old;
      mo_file_old = mo_file;
      mo_file = grub_xasprintf ("%s.gz", mo_file);
      grub_free (mo_file_old);
      if (!mo_file)
	return;
      fd_mo = grub_mofile_open (mo_file);
    }
}

static void
grub_gettext_init_ext (const char *locale)
{
  char *locale_dir;

  locale_dir = grub_env_get ("locale_dir");
  if (locale_dir == NULL)
    {
      grub_dprintf ("gettext", "locale_dir variable is not set up.\n");
      return;
    }

  fd_mo = NULL;

  grub_mofile_open_lang (locale_dir, locale);

  /* ll_CC didn't work, so try ll.  */
  if (fd_mo == NULL)
    {
      char *lang = grub_strdup (locale);
      char *underscore = grub_strchr (lang, '_');

      if (underscore)
	{
	  *underscore = '\0';
	  grub_mofile_open_lang (locale_dir, lang);
	}

      grub_free (lang);
    }

  if (fd_mo)
    {
      grub_gettext_offsetoriginal =
	grub_gettext_get_info (GETTEXT_OFFSET_ORIGINAL);
      grub_gettext_max = grub_gettext_get_info (GETTEXT_NUMBER_OF_STRINGS);

      grub_gettext_original = grub_gettext;
      grub_gettext = grub_gettext_translate;
    }
}

static void
grub_gettext_delete_list (void)
{
  while (grub_gettext_msg_list)
    {
      grub_free ((char *) grub_gettext_msg_list->name);
      grub_gettext_msg_list = grub_gettext_msg_list->next;
      /* Don't delete the translated message because could be in use.  */
    }
}

static char *
grub_gettext_env_write_lang (struct grub_env_var *var
			     __attribute__ ((unused)), const char *val)
{
  grub_gettext_init_ext (val);

  grub_gettext_delete_list ();

  return grub_strdup (val);
}

static grub_err_t
grub_cmd_translate (grub_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  if (argc != 1)
    return grub_error (GRUB_ERR_BAD_ARGUMENT, "text to translate required");

  const char *translation;
  translation = grub_gettext_translate (args[0]);
  grub_printf ("%s\n", translation);
  return 0;
}

GRUB_MOD_INIT (gettext)
{
  (void) mod;			/* To stop warning.  */

  const char *lang;

  lang = grub_env_get ("lang");

  grub_gettext_init_ext (lang);

  grub_register_command_p1 ("gettext", grub_cmd_translate,
			    N_("STRING"),
			    N_("Translates the string with the current settings."));

  /* Reload .mo file information if lang changes.  */
  grub_register_variable_hook ("lang", NULL, grub_gettext_env_write_lang);

  /* Preserve hooks after context changes.  */
  grub_env_export ("lang");
}

GRUB_MOD_FINI (gettext)
{
  if (fd_mo != 0)
    grub_file_close (fd_mo);

  grub_gettext_delete_list ();

  grub_gettext = grub_gettext_original;
}
