/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
*/

#define PACKAGE
#define PACKAGE_VERSION
#include <bfd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/param.h>

#include "misc.h"
#include "backend.h"

static void bfd_fatal(const char *msg)
{
    fatal(msg, bfd_errmsg(bfd_get_error()));
}

static void setfunc(bfd *exe, asection *sect, void *setlist_ptr)
{
    parse_secname(sect->name, (setnode **)setlist_ptr);
}

void collect_sets(const char *file, setnode **setlist_ptr)
{
    bfd *abfd;

    if
    (
        (abfd = bfd_openr(file, "default")) == NULL ||
        !bfd_check_format(abfd, bfd_object)
    )
    {
        bfd_fatal(file);
    }

    parse_format(abfd->xvec->name);
    bfd_map_over_sections(abfd, setfunc, setlist_ptr);

    bfd_close(abfd);
}

static void collect_lib(asymbol *sym, setnode **liblist_ptr)
{
    setnode *node;
    char *cp, *name;
    int pri;

    if (strncmp(sym->name, "__aros_libreq_", 14) != 0)
            return;

    node = xmalloc(sizeof(*node)+strlen(sym->name)+1);
    name = (char *)(&node[1]);
    strcpy(name, sym->name);

    cp = strchr(name + 14, '.');
    if (cp != NULL) {
        char *tmp;
        pri = strtoul(cp+1, &tmp, 0);

        if ((cp+1) == tmp) {
            free(node);
            return;
        }

        *(cp++) = 0;

    } else {
        pri = 0;
    }

    node->secname = name;
    node->off_setname = 14;
    node->pri = pri;
    node->next = *liblist_ptr;
    *liblist_ptr = node;
}

void collect_libs(const char *file, setnode **liblist_ptr)
{
    long symtab_size;
    bfd *abfd;
    if
    (
        (abfd = bfd_openr(file, "default")) == NULL ||
        !bfd_check_format(abfd, bfd_object)
    )
    {
        bfd_fatal(file);
    }

    symtab_size = bfd_get_symtab_upper_bound(abfd);
    if (symtab_size > 0) {
        asymbol **symtab;
        long symbols;

        symtab = (asymbol **)xmalloc(symtab_size);
        symbols = bfd_canonicalize_symtab(abfd, symtab);
        if (symbols > 0) {
            long i;

            for (i = 0; i < symbols; i++)
                collect_lib(symtab[i], liblist_ptr);
        }

        free(symtab);
    }

    bfd_close(abfd);
}

/*
 * Find undefined symbols that require extra object linkage
 * Objects are expected to be available in the lib/ dir
 */
static void collect_extrasymbol(asymbol *sym, setnode **liblist_ptr)
{
    setnode *node;
    char *cp, *name, *extraobj;
    int pri;

    if (strncmp(sym->name, "__cxa_pure_virtual", 18) == 0)
    {
        /*
             * If the selected libraries didnt provide the symbol,
             * link in the aros static-cxx-cxa-pure-virtual implementation
             */
        extraobj = xmalloc(strlen(OBJLIBDIR)+strlen(AROSOBJ_CXXPUREVIRT)+2);
        sprintf(extraobj, "%s/%s", OBJLIBDIR, AROSOBJ_CXXPUREVIRT);
    }
    else
        return;

    node = xmalloc(sizeof(*node)+strlen(extraobj)+1);
    name = (char *)(&node[1]);
    strcpy(name, extraobj);

    node->secname = name;
    node->next = *liblist_ptr;
    *liblist_ptr = node;
}

void collect_extra(const char *file, setnode **liblist_ptr)
{
    long symtab_size;
    bfd *abfd;

    if
    (
        (abfd = bfd_openr(file, "default")) == NULL ||
        !bfd_check_format(abfd, bfd_object)
    )
    {
        bfd_fatal(file);
    }

    symtab_size = bfd_get_symtab_upper_bound(abfd);
    if (symtab_size > 0) {
        asymbol **symtab;
        long symbols;

        symtab = (asymbol **)xmalloc(symtab_size);
        symbols = bfd_canonicalize_symtab(abfd, symtab);
        if (symbols > 0) {
            long i;

            for (i = 0; i < symbols; i++)
                collect_extrasymbol(symtab[i], liblist_ptr);
        }

        free(symtab);
    }

    bfd_close(abfd);
}

int check_and_print_undefined_symbols(const char *file)
{
    bfd_byte *cur, *minisyms;
    asymbol *store;
    long symcount;
    unsigned int size;
    int undefined_syms = 0;
    bfd *abfd;

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
            if (!undefined_syms)
            {
                undefined_syms = 1;

                fprintf(stderr, "There are undefined symbols in '%s':\n", bfd_get_filename(abfd));
            }

            fprintf(stderr, "%s\n", sym->name);
        }
    }

    bfd_close(abfd);
    /* We should free() minisyms, but since we're called only once, we let the system
       do it for us.  */

    return undefined_syms;
}

void backend_init(char *ldname)
{
    bfd_init();
}
