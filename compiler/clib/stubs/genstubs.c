#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <aros/stubs.h>
#include <exec/libraries.h>

#define SYSTEM_CALL(name) "" #name "" ,

static char *names[]=
{
#include <sys/syscall.def>
NULL
};

void emitstub(int n)
{
    char *fname;
    FILE *f;

    fname = malloc(strlen(names[n])+3);
    if (!fname) exit(1);

    strcpy(fname, names[n]);
    strcat(fname, ".S");

    f = fopen(fname, "w");

    if (!f) exit(1);

    fprintf(f, STUBCODE,
	       names[n], names[n], "aroscbase",
	       -(n+LIB_RESERVED+1)*LIB_VECTSIZE);

    fclose(f);
    free(fname);
}

void emitstubslist(void)
{
    FILE *f;
    int n;

    f = fopen("stubslist", "w");
    if (!f) exit(1);

    for (n=0; names[n]; n++)
    	fprintf(f, "%s\n", names[n]);

    fclose(f);
}

void emitdeps(char *prog)
{
    FILE *f;
    int n;

    f = fopen("stubsdeps", "w");
    if (!f) exit(1);

    for (n=0; names[n]; n++)
    	fprintf(f, "%s.S :\n\t%s stubs\n\n", names[n], prog);

    fclose(f);
}

int main(int argc, char *argv[])
{
    int n;

    if (argc != 2)
    {
    	fprintf(stderr, "Argument required: either 'stubs' or 'list' or 'deps'\n");
	return 1;
    }

    if (!strcmp(argv[1], "stubs"))
    	for (n=0; names[n]; n++)
            emitstub(n);
    else
    if (!strcmp(argv[1], "list"))
	emitstubslist();
    else
    if (!strcmp(argv[1], "deps"))
	emitdeps(argv[0]);
    else
    {
    	fprintf(stderr, "Argument required: either 'stubs' or 'list' or 'deps'\n");
	return 1;
    }

    return 0;
}
