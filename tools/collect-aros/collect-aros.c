#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/wait.h>

extern int gensets(FILE *in, FILE *out);

char *joinargs(char *argv[])
{
    char *ret;
    int size, cnt;

    for
    (
    	cnt=1, size = 0;
	argv[cnt];
	cnt++
    )
    size += strlen(argv[cnt])+1;

    ret = malloc(size);
    if (!ret)
    {
    	perror("Internal error");
	exit(1);
    }

    ret[0]='\0';

    for
    (
    	cnt=1, size = 0;
	argv[cnt];
	cnt++
    )
    {
   	strcat(ret, " " );
    	strcat(ret, argv[cnt]);
    }

    return ret;
}

char *joinstrings(char *first, ...)
{
    va_list strings;
    char *str, *s;
    int size = 0;

    if (!first)
       	return NULL;

    size += strlen(first);

    va_start (strings, first);

    while ((s=va_arg(strings, char *)))
    	size += strlen(s);

    va_end(strings);

    str = malloc(size+1);
    if (!str)
    {
    	perror("Internal error");
	exit(1);
    }

    str[0]='\0';

    strcat(str, first);

    va_start(strings, first);

    while ((s=va_arg(strings, char *)))
    	strcat(str, s);

    va_end (strings);

    return str;
}

#define ERROR(num, action) \
do \
{  \
    ret = num;  \
    action;     \
    goto err;   \
} while(0)

int main(int argc, char *argv[])
{
    int cnt, ret;
    char *tempoutname;
    char *setsfilename;
    char *output;
    char *command;
    char *arguments;
    FILE *pipe;
    FILE *setsfile = NULL;

    /* Get the output file name */

    output = "a.out";
    for (cnt = 1; argv[cnt]; cnt++)
    {
    	if (argv[cnt][0]=='-' && argv[cnt][1]=='o')
     	    output = argv[cnt][2]?&argv[cnt][2]:argv[++cnt];
    }

    arguments = joinargs(argv);

    if
    (
    	!(tempoutname = tempnam("/tmp", NULL)) ||
	!(setsfilename  = joinstrings(tempoutname, "-set.c", NULL))
    )
    ERROR(1, perror("Internal Error"));

    command = joinstrings("ld -r ", arguments, NULL);
    printf(">>>1<<< %s\n", command);

    if ((ret=WEXITSTATUS(system(command))))
    	ERROR(ret, );

    free(command);
    command=joinstrings(" nm ", output, NULL);

    if
    (
    	!(pipe   = popen(command, "r")) ||
	!(setsfile = fopen(setsfilename, "w"))
    )
    ERROR(1, perror("Internal Error"));

    ret = gensets(pipe, setsfile);
    fclose(setsfile);

   if (ret)
    {
	free(command);
	command = joinstrings("gcc -nostartfiles -nostdlib -Wl,-r -o ", tempoutname, " ", output, " ", setsfilename, NULL);
	printf(">>>2<<< %s\n", command);
	if ((ret=WEXITSTATUS(system(command))))
	    ERROR(ret, );
	if ((ret=WEXITSTATUS(system(joinstrings("mv -f ", tempoutname, " ", output, NULL)))))
	    ERROR(ret, );
    }


err:
    if (setsfile)
    	remove(setsfilename);

    return ret;
}
