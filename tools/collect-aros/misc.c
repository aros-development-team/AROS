#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <string.h>

#include "misc.h"

char *program_name;
void nonfatal(const char *msg, const char *errorstr)
{
    if (msg != NULL)
        fprintf(stderr, "%s: %s: %s\n" , program_name, msg, errorstr);
    else
        fprintf(stderr, "%s: %s\n" , program_name, errorstr);
}

void fatal(const char *msg, const char *errorstr)
{
    nonfatal(msg, errorstr);
    exit(EXIT_FAILURE);
}

void set_compiler_path(void)
{
    static int path_set = 0;

    if (!path_set)
    {
        char *compiler_path = getenv("COMPILER_PATH");
        char *path          = getenv("PATH");

        if (compiler_path && path)
	{
            char *new_path;

            new_path = malloc(5 + strlen(compiler_path) + 1 + strlen(path) + 1);
            if (new_path)
            {
                strcat(new_path, "PATH=");
                strcat(new_path, compiler_path);
                strcat(new_path, ":");
                strcat(new_path, path);

	        if (putenv(new_path) == 0)
		    path_set = 1;
            }
	}
    }
}
