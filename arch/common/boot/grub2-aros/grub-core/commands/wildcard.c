/* wildcard.c - Wildcard character expansion for GRUB script.  */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#include <grub/mm.h>
#include <grub/fs.h>
#include <grub/env.h>
#include <grub/file.h>
#include <grub/device.h>
#include <grub/script_sh.h>

#include <regex.h>

static inline int isregexop (char ch);
static char ** merge (char **lhs, char **rhs);
static char *make_dir (const char *prefix, const char *start, const char *end);
static int make_regex (const char *regex_start, const char *regex_end,
		       regex_t *regexp);
static void split_path (const char *path, const char **suffix_end, const char **regex_end);
static char ** match_devices (const regex_t *regexp, int noparts);
static char ** match_files (const char *prefix, const char *suffix_start,
			    const char *suffix_end, const regex_t *regexp);

static char* wildcard_escape (const char *s);
static char* wildcard_unescape (const char *s);
static grub_err_t wildcard_expand (const char *s, char ***strs);

struct grub_script_wildcard_translator grub_filename_translator = {
  .expand = wildcard_expand,
  .escape = wildcard_escape,
  .unescape = wildcard_unescape
};

static char **
merge (char **dest, char **ps)
{
  int i;
  int j;
  char **p;

  if (! dest)
    return ps;

  if (! ps)
    return dest;

  for (i = 0; dest[i]; i++)
    ;
  for (j = 0; ps[j]; j++)
    ;

  p = grub_realloc (dest, sizeof (char*) * (i + j + 1));
  if (! p)
    {
      grub_free (dest);
      grub_free (ps);
      return 0;
    }

  dest = p;
  for (j = 0; ps[j]; j++)
    dest[i++] = ps[j];
  dest[i] = 0;

  grub_free (ps);
  return dest;
}

static inline int
isregexop (char ch)
{
  return grub_strchr ("*.\\", ch) ? 1 : 0;
}

static char *
make_dir (const char *prefix, const char *start, const char *end)
{
  char ch;
  unsigned i;
  unsigned n;
  char *result;

  i = grub_strlen (prefix);
  n = i + end - start;

  result = grub_malloc (n + 1);
  if (! result)
    return 0;

  grub_strcpy (result, prefix);
  while (start < end && (ch = *start++))
    if (ch == '\\' && isregexop (*start))
      result[i++] = *start++;
    else
      result[i++] = ch;

  result[i] = '\0';
  return result;
}

static int
make_regex (const char *start, const char *end, regex_t *regexp)
{
  char ch;
  int i = 0;
  unsigned len = end - start;
  char *buffer = grub_malloc (len * 2 + 2 + 1); /* worst case size. */

  if (! buffer)
    return 1;

  buffer[i++] = '^';
  while (start < end)
    {
      /* XXX Only * expansion for now.  */
      switch ((ch = *start++))
	{
	case '\\':
	  buffer[i++] = ch;
	  if (*start != '\0')
	    buffer[i++] = *start++;
	  break;

	case '.':
	case '(':
	case ')':
	  buffer[i++] = '\\';
	  buffer[i++] = ch;
	  break;

	case '*':
	  buffer[i++] = '.';
	  buffer[i++] = '*';
	  break;

	default:
	  buffer[i++] = ch;
	}
    }
  buffer[i++] = '$';
  buffer[i] = '\0';
  grub_dprintf ("expand", "Regexp is %s\n", buffer);

  if (regcomp (regexp, buffer, RE_SYNTAX_GNU_AWK))
    {
      grub_free (buffer);
      return 1;
    }

  grub_free (buffer);
  return 0;
}

/* Split `str' into two parts: (1) dirname that is regexop free (2)
   dirname that has a regexop.  */
static void
split_path (const char *str, const char **noregexop, const char **regexop)
{
  char ch = 0;
  int regex = 0;

  const char *end;
  const char *split;  /* points till the end of dirnaname that doesn't
			 need expansion.  */

  split = end = str;
  while ((ch = *end))
    {
      if (ch == '\\' && end[1])
	end++;

      else if (isregexop (ch))
	regex = 1;

      else if (ch == '/' && ! regex)
	split = end + 1;  /* forward to next regexop-free dirname */

      else if (ch == '/' && regex)
	break;  /* stop at the first dirname with a regexop */

      end++;
    }

  *regexop = end;
  if (! regex)
    *noregexop = end;
  else
    *noregexop = split;
}

static char **
match_devices (const regex_t *regexp, int noparts)
{
  int i;
  int ndev;
  char **devs;

  auto int match (const char *name);
  int match (const char *name)
  {
    char **t;
    char *buffer;

    /* skip partitions if asked to. */
    if (noparts && grub_strchr(name, ','))
      return 0;

    buffer = grub_xasprintf ("(%s)", name);
    if (! buffer)
      return 1;

    grub_dprintf ("expand", "matching: %s\n", buffer);
    if (regexec (regexp, buffer, 0, 0, 0))
      {
	grub_dprintf ("expand", "not matched\n");
	grub_free (buffer);
	return 0;
      }

    t = grub_realloc (devs, sizeof (char*) * (ndev + 2));
    if (! t)
      return 1;

    devs = t;
    devs[ndev++] = buffer;
    devs[ndev] = 0;
    return 0;
  }

  ndev = 0;
  devs = 0;

  if (grub_device_iterate (match))
    goto fail;

  return devs;

 fail:

  for (i = 0; devs && devs[i]; i++)
    grub_free (devs[i]);

  if (devs)
    grub_free (devs);

  return 0;
}

static char **
match_files (const char *prefix, const char *suffix, const char *end,
	     const regex_t *regexp)
{
  int i;
  char **files;
  unsigned nfile;
  char *dir;
  const char *path;
  char *device_name;
  grub_fs_t fs;
  grub_device_t dev;

  auto int match (const char *name, const struct grub_dirhook_info *info);
  int match (const char *name, const struct grub_dirhook_info *info)
  {
    char **t;
    char *buffer;

    /* skip . and .. names */
    if (grub_strcmp(".", name) == 0 || grub_strcmp("..", name) == 0)
      return 0;

    grub_dprintf ("expand", "matching: %s in %s\n", name, dir);
    if (regexec (regexp, name, 0, 0, 0))
      return 0;

    buffer = grub_xasprintf ("%s%s", dir, name);
    if (! buffer)
      return 1;

    t = grub_realloc (files, sizeof (char*) * (nfile + 2));
    if (! t)
      {
	grub_free (buffer);
	return 1;
      }

    files = t;
    files[nfile++] = buffer;
    files[nfile] = 0;
    return 0;
  }

  nfile = 0;
  files = 0;
  dev = 0;
  device_name = 0;
  grub_error_push ();

  dir = make_dir (prefix, suffix, end);
  if (! dir)
    goto fail;

  device_name = grub_file_get_device_name (dir);
  dev = grub_device_open (device_name);
  if (! dev)
    goto fail;

  fs = grub_fs_probe (dev);
  if (! fs)
    goto fail;

  path = grub_strchr (dir, ')');
  if (! path)
    goto fail;
  path++;

  if (fs->dir (dev, path, match))
    goto fail;

  grub_free (dir);
  grub_device_close (dev);
  grub_free (device_name);
  grub_error_pop ();
  return files;

 fail:

  if (dir)
    grub_free (dir);

  for (i = 0; files && files[i]; i++)
    grub_free (files[i]);

  if (files)
    grub_free (files);

  if (dev)
    grub_device_close (dev);

  if (device_name)
    grub_free (device_name);

  grub_error_pop ();
  return 0;
}

static char*
wildcard_escape (const char *s)
{
  int i;
  int len;
  char ch;
  char *p;

  len = grub_strlen (s);
  p = grub_malloc (len * 2 + 1);
  if (! p)
    return NULL;

  i = 0;
  while ((ch = *s++))
    {
      if (isregexop (ch))
	p[i++] = '\\';
      p[i++] = ch;
    }
  p[i] = '\0';
  return p;
}

static char*
wildcard_unescape (const char *s)
{
  int i;
  int len;
  char ch;
  char *p;

  len = grub_strlen (s);
  p = grub_malloc (len + 1);
  if (! p)
    return NULL;

  i = 0;
  while ((ch = *s++))
    {
      if (ch == '\\' && isregexop (*s))
	p[i++] = *s++;
      else
	p[i++] = ch;
    }
  p[i] = '\0';
  return p;
}

static grub_err_t
wildcard_expand (const char *s, char ***strs)
{
  const char *start;
  const char *regexop;
  const char *noregexop;
  char **paths = 0;

  unsigned i;
  regex_t regexp;

  start = s;
  while (*start)
    {
      split_path (start, &noregexop, &regexop);
      if (noregexop >= regexop) /* no more wildcards */
	break;

      if (make_regex (noregexop, regexop, &regexp))
	goto fail;

      if (paths == 0)
	{
	  if (start == noregexop) /* device part has regexop */
	    paths = match_devices (&regexp, *start != '(');

	  else if (*start == '(') /* device part explicit wo regexop */
	    paths = match_files ("", start, noregexop, &regexp);

	  else if (*start == '/') /* no device part */
	    {
	      char *root;
	      char *prefix;

	      root = grub_env_get ("root");
	      if (! root)
		goto fail;

	      prefix = grub_xasprintf ("(%s)", root);
	      if (! prefix)
		goto fail;

	      paths = match_files (prefix, start, noregexop, &regexp);
	      grub_free (prefix);
	    }
	}
      else
	{
	  char **r = 0;

	  for (i = 0; paths[i]; i++)
	    {
	      char **p;

	      p = match_files (paths[i], start, noregexop, &regexp);
	      if (! p)
		continue;

	      r = merge (r, p);
	      if (! r)
		goto fail;
	    }
	  paths = r;
	}

      regfree (&regexp);
      if (! paths)
	goto done;

      start = regexop;
    }

 done:

  *strs = paths;
  return 0;

 fail:

  for (i = 0; paths && paths[i]; i++)
    grub_free (paths[i]);
  grub_free (paths);
  regfree (&regexp);
  return grub_errno;
}
