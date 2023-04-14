/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
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

        if (type == 'A') {
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
        else
            continue;

        node = calloc(sizeof(*node),1);
        node->secname = strdup(objname);
        node->next = *liblist_ptr;
        *liblist_ptr = node;
    }

    pclose(pipe);
}

int check_and_print_undefined_symbols(const char *file)
{
    char buf[200];
    int undefined_syms = 0;
    size_t cnt;

    strcpy(buf, NM_NAME);
    if (!strstr(buf, "--demangle"))
        strcat(buf, " --demangle");
    if (!strstr(buf, "--undefined-only"))
        strcat(buf, " --undefined-only");
    if ((have_gnunm) && (!strstr(buf, "--line-numbers")))
        strcat(buf, " --line-numbers");

    FILE *pipe = my_popen(buf, file);

    while ((cnt = fread(buf, 1, sizeof(buf), pipe)) != 0)
    {
        if (!undefined_syms)
        {
            undefined_syms = 1;
            fprintf(stderr, "There are undefined symbols in '%s':\n", file);
        }

        fwrite(buf, cnt, 1, stderr);
    }

    pclose(pipe);

    return undefined_syms;
}

void backend_init(char *ldname)
{
    // nothing to do
    return;
}
