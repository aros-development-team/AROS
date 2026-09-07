/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include "misc.h"
#include "backend.h"
#include <sys/param.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "env.h"

static FILE *my_popen(const char *command, const char *file)
{
    static char command_buf[MAXPATHLEN];

    size_t command_len = strlen(command);
    size_t file_len    = strlen(file);

    FILE *pipe;

    if (command_len >= sizeof(command_buf) - 1)
        fatal("collect_sets()", strerror(ENAMETOOLONG));
    memcpy(command_buf, command, command_len);
    if (command[command_len - 1] != ' ')
    {
        command_buf[command_len++] = ' ';
    }
    if (file_len + command_len >= sizeof(command_buf))
        fatal("collect_sets()", strerror(ENAMETOOLONG));
    memcpy(command_buf + command_len, file, file_len + 1);

    set_compiler_path();

    pipe = popen(command_buf, "r");
    if (pipe == NULL)
        fatal(command_buf, strerror(errno));

    return pipe;
}

/*
    The following routines are slow, but do the work and are the simplest to write down.
    All this will get integrated into the linker anyway, so there's no point
    in doing optimizations
*/
void collect_sets(const char *file, setnode **setlist_ptr)
{
    char secname[201];

    FILE *pipe = my_popen(OBJDUMP_NAME " -h ", file);

    /* split the stream into separate words */
    while (fscanf(pipe, " %200s ", secname) > 0)
    {
        parse_format(secname);
        parse_secname(secname, setlist_ptr);
    }

    pclose(pipe);
}

void collect_libs(const char *file, setnode **liblist_ptr)
{
    char secname[201];
    char buff[256];
    unsigned long offset;
    char type;

    FILE *pipe = my_popen("nm ", file);

    while (fgets(buff, sizeof(buff), pipe)) {
        struct setnode *node;
        int pri;

        offset = 0;

        if (sscanf(buff, "%lx %c %200s ", &offset, &type, secname) != 3 &&
            sscanf(buff, " %c %200s", &type, secname) != 2)
            continue;

        if (strncmp(secname, "__aros_libreq_", 14) != 0)
            continue;

        /* AROS_LIBREQ emits the version marker as a weak absolute symbol, so
           nm reports it as 'W'/'V' rather than 'A' - accept both spellings. */
        if (type == 'A' || type == 'W' || type == 'V') {
            char *cp, *tmp;

            cp = strchr(secname + 14, '.');
            if (cp == NULL)
                continue;

            pri = strtoul(cp+1, &tmp, 0);
            if ((cp+1) == tmp)
                continue;

            *(cp++) = 0;
        } else if (type == 'w') {
            pri = 0;
        } else {
            continue;
        }

        node = calloc(sizeof(*node),1);
        node->secname = strdup(secname);
        node->off_setname = 14;
        node->pri = pri;
        node->next = *liblist_ptr;
        *liblist_ptr = node;
    }

    pclose(pipe);
}

void collect_extra(const char *file, setnode **liblist_ptr)
{
    char *objname, secname[201];
    char buff[256];
    unsigned long offset;
    char type;
    int pthread_added = 0;

    FILE *pipe = my_popen("nm ", file);

    while (fgets(buff, sizeof(buff), pipe)) {
        struct setnode *node;

        offset = 0;

        if (sscanf(buff, "%lx %c %200s ", &offset, &type, secname) != 3 &&
            sscanf(buff, " %c %200s", &type, secname) != 2)
            continue;

        if ((strncmp(secname, "__cxa_pure_virtual", 18) == 0) &&
            (type == 'w'))
        {
            objname = calloc(strlen(OBJLIBDIR)+strlen(AROSOBJ_CXXPUREVIRT)+2, 1);
            sprintf(objname, "%s/%s", OBJLIBDIR, AROSOBJ_CXXPUREVIRT);
        }
        else if ((strncmp(secname, "pthread_", 8) == 0) &&
            (type == 'U') && !pthread_added)
        {
            /*
             * Emulated TLS (libgcc's emutls.o, pulled in for __thread /
             * _Thread_local data) references the pthread_* functions. These
             * live in the standard AROS libpthread linklib but are not part of
             * the auto-linked set, so they never appear on the command line and
             * are left undefined by the (LTO plugin) relocatable stage. Supply
             * the libpthread archive from the lib/ dir so ld can pull the
             * required members on demand.
             */
            objname = calloc(strlen(OBJLIBDIR)+strlen(AROSLIB_PTHREAD)+2, 1);
            sprintf(objname, "%s/%s", OBJLIBDIR, AROSLIB_PTHREAD);
            pthread_added = 1;

            if (getenv("COLLECT_AROS_DEBUG") != NULL)
                fprintf(stderr, "[collect-aros] undefined pthread symbol '%s'"
                                " (emulated-TLS dependency); adding %s\n",
                                secname, objname);
        }
        else
            continue;

        node = calloc(sizeof(*node),1);
        node->secname = strdup(objname);
        node->next = *liblist_ptr;
        *liblist_ptr = node;

        free(objname);
    }

    pclose(pipe);
}

/* nm marks an undefined symbol 'U', and one that is only weakly referenced
   'w' ('v' when it is an object). A weak reference is allowed to stay
   unresolved - it evaluates to zero, and the code using it tests for NULL
   before calling it - so it must not be reported as a link failure. */
static int is_weak_undefined(const char *line)
{
    while (*line == ' ' || *line == '\t')
        line++;

    return (line[0] == 'w' || line[0] == 'v')
            && (line[1] == ' ' || line[1] == '\t');
}

int check_and_print_undefined_symbols(const char *file)
{
    char cmd[200], line[4096];
    int undefined_syms = 0;
    int skipping = 0;

    strcpy(cmd, NM_NAME);
    if (!strstr(cmd, "--demangle"))
        strcat(cmd, " --demangle");
    if (!strstr(cmd, "--undefined-only"))
        strcat(cmd, " --undefined-only");
    if ((have_gnunm) && (!strstr(cmd, "--line-numbers")))
        strcat(cmd, " --line-numbers");

    FILE *pipe = my_popen(cmd, file);

    while (fgets(line, sizeof(line), pipe) != NULL)
    {
        int complete = (strchr(line, '\n') != NULL);

        /* A line longer than the buffer arrives in pieces; the pieces
           after the first are not symbols of their own. */
        if (!skipping)
        {
            if (is_weak_undefined(line))
            {
                skipping = !complete;
                continue;
            }

            if (!undefined_syms)
            {
                undefined_syms = 1;
                fprintf(stderr, "There are undefined symbols in '%s':\n", file);
            }

            fputs(line, stderr);
        }

        if (complete)
            skipping = 0;
    }

    pclose(pipe);

    return undefined_syms;
}

/* Quiet variant of the above: returns non-zero if there is at least one
   undefined symbol, without printing anything. Used to decide whether the
   library re-supply fixup is needed for the final link.

   Unlike the error report above, weak references DO count here. They are
   what is normally left after the relocatable pass (__aros_libreq_SysBase,
   the DECLARESET'd __*_LIST__ symbols), and the final link also adds strong
   EXTERN(__*__symbol_set_handler_missing) references from the generated
   ldscript for every symbol set the objects carry - and those can only be
   satisfied from the libraries (libautoinit's initexitsets.o). Skipping the
   weak ones here left UserShell-Seg's INIT guard unresolvable. */
int has_undefined_symbols(const char *file)
{
    char buf[200], line[4096];
    int result = 0;

    strcpy(buf, NM_NAME);
    if (!strstr(buf, "--undefined-only"))
        strcat(buf, " --undefined-only");

    FILE *pipe = my_popen(buf, file);

    while (fgets(line, sizeof(line), pipe) != NULL)
    {
        result = 1;
        break;
    }

    pclose(pipe);

    return result;
}

void backend_init(char *ldname)
{
    // nothing to do
    return;
}
