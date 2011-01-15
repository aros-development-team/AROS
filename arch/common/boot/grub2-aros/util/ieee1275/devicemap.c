#include <stdio.h>
#include <string.h>
#include <grub/types.h>
#include <grub/util/deviceiter.h>
#include <grub/util/ofpath.h>
#include <grub/util/misc.h>

/* Since OF path names can have "," characters in them, and GRUB
   internally uses "," to indicate partitions (unlike OF which uses
   ":" for this purpose) we escape such commas.  */

static char *
escape_of_path (const char *orig_path)
{
  char *new_path, *d, c;
  const char *p;

  if (!strchr (orig_path, ','))
    return (char *) orig_path;

  new_path = xmalloc (strlen (orig_path) * 2);

  p = orig_path;
  d = new_path;
  while ((c = *p++) != '\0')
    {
      if (c == ',')
	*d++ = '\\';
      *d++ = c;
    }

  free ((char *) orig_path);

  return new_path;
}

void
grub_util_emit_devicemap_entry (FILE *fp, char *name,
				int is_floppy __attribute__((unused)),
				int *num_fd __attribute__((unused)),
				int *num_hd __attribute__((unused)))
{
  const char *orig_path = grub_util_devname_to_ofpath (name);
  char *ofpath = escape_of_path (orig_path);

  fprintf(fp, "(%s)\t%s\n", ofpath, name);

  free (ofpath);
}
