#include "misc.h"
#include "backend.h"
#include <sys/param.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "env.h"

static FILE *my_popen(const char *command, const char *file)
{
    static char command_buf[MAXPATHLEN];

    size_t command_len = strlen(command);
    size_t file_len    = strlen(file);

    FILE *pipe;

    if (file_len + command_len >= sizeof(command_buf))
        fatal("collect_sets()", strerror(ENAMETOOLONG));

    memcpy(command_buf, command, command_len);
    memcpy(command_buf + command_len, file, file_len + 1);

    set_compiler_path();

    pipe = popen(command_buf, "r");
    if (pipe == NULL)
        fatal(command_buf, strerror(errno));

    return pipe;
}


/*
    This routine is slow, but does the work and it's the simplest to write down.
    All this will get integrated into the linker anyway, so there's no point
    in doing optimizations
*/
void collect_sets(const char *file, setnode **setlist_ptr)
{
    char secname[201];

    FILE *pipe = my_popen(OBJDUMP_NAME " -h ", file);
    /* This fscanf() simply splits the whole stream into separate words */
    while (fscanf(pipe, " %200s ", secname) > 0)
    {
	parse_format(secname);
        parse_secname(secname, setlist_ptr);
    }

    pclose(pipe);
}

int check_and_print_undefined_symbols(const char *file)
{
    int there_are_undefined_syms = 0;
    char buf[200];

    FILE *pipe = my_popen(NM_NAME " -ulC ", file);

    while (fgets(buf, sizeof(buf), pipe) != NULL)
    {
	/* Ignore auto symbols used for autoinit, which
	 * are validly undefined for non-autoinit
	 * programs.
	 */
	const char isauto[] = "U __aros_libreq_";
	const char *s = buf;
	while (isspace(*s)) s++;
	if (strncmp(s,isauto,sizeof(isauto)-1)==0)
	    continue;

	if (!there_are_undefined_syms)
	{
	    there_are_undefined_syms = 1;
	    fprintf(stderr, "There are undefined symbols in '%s':\n", file);
        }

	fputs(buf, stderr);
    }

    pclose(pipe);

    return there_are_undefined_syms;
}
