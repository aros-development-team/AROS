#include <bfd.h>

#include "misc.h"
#include "backend.h"

static void bfd_fatal(const char *msg)
{
    fatal(msg, bfd_errmsg(bfd_get_error()));
}

int check_and_print_undefined_symbols(const char *file)
{
    bfd_byte *cur, *minisyms;
    asymbol *store;
    long symcount;
    unsigned int size;
    int there_are_undefined_syms = 0;
    bfd *abfd;

    bfd_init();

    if
    (
        (abfd = bfd_openr(file, "default")) == NULL ||
        !bfd_check_format(abfd, bfd_object)
    )
    {
        bfd_fatal(file);
    }

    symcount = bfd_read_minisymbols(abfd, FALSE, (void **)&minisyms, &size);
    if (symcount < 0)
        bfd_fatal (bfd_get_filename (abfd));

    if (symcount == 0)
        return 0;

    store = bfd_make_empty_symbol(abfd);
    if (store == NULL)
        bfd_fatal(bfd_get_filename(abfd));

    for (cur = minisyms; cur < (minisyms + (symcount * size)); cur += size)
    {
        asymbol *sym;

        sym = bfd_minisymbol_to_symbol(abfd, FALSE, (const void *)cur, store);
        if (sym == NULL)
            bfd_fatal(bfd_get_filename (abfd));

	if (bfd_is_und_section (sym->section))
	{
	    if (!there_are_undefined_syms)
	    {
	        there_are_undefined_syms = 1;

		fprintf(stderr, "There are undefined symbols in '%s':\n", bfd_get_filename(abfd));
	    }

	    fprintf(stderr, "%s\n", sym->name);
	}
    }

    bfd_close(abfd);
    /* We should free() minisyms, but since we're called only once, we let the system
       do it for us.  */

    return there_are_undefined_syms;
}

static void setfunc(bfd *exe, asection *sect, PTR setlist_ptr)
{
    parse_secname(sect->name, (setnode **)setlist_ptr);
}

void collect_sets(const char *file, setnode **setlist_ptr)
{
    bfd_init();

    bfd *abfd;

    if
    (
        (abfd = bfd_openr(file, "default")) == NULL ||
        !bfd_check_format(abfd, bfd_object)
    )
    {
        bfd_fatal(file);
    }

    bfd_map_over_sections(abfd, setfunc, setlist_ptr);

    bfd_close(abfd);
}
