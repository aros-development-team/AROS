#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#include "misc.h"
#include "docommand.h"
#include "backend.h"
#include "ldscript.h"
#include "gensets.h"

static char *ldscriptname, *tempoutput;
static FILE *ldscriptfile;

static void exitfunc(void)
{
    if (ldscriptfile != NULL)
        fclose(ldscriptfile);

    if (ldscriptname != NULL)
        remove(ldscriptname);

    if (tempoutput != NULL)
        remove(tempoutput);
}

int main(int argc, char *argv[])
{
    int cnt;
    char *output, **ldargs;
    int incremental = 0, ignore_undefined_symbols = 0;
    int strip_all   = 0;
    char *do_verbose = NULL;

    setnode *setlist = NULL;

    program_name = argv[0];

    /* Do some stuff with the arguments */
    output = "a.out";
    for (cnt = 1; argv[cnt]; cnt++)
    {
    	/* We've encountered an option */
	if (argv[cnt][0]=='-')
	{
            /* Get the output file name */
	    if (argv[cnt][1]=='o')
     	        output = argv[cnt][2]?&argv[cnt][2]:argv[++cnt];
            else
	    /* Incremental linking is requested */
            if ((argv[cnt][1]=='r' || argv[cnt][1]=='i') && argv[cnt][2]=='\0')
	        incremental = 1;
	    else
	    /* Ignoring of missing symbols is requested */
	    if (strncmp(&argv[cnt][1], "ius", 4) == 0)
	    {
	        ignore_undefined_symbols = 1;
		argv[cnt][1] = 'r';  /* Just some non-harming option... */
		argv[cnt][2] = '\0';
	    }
	    else
	    /* Complete stripping is requested, but we do it our own way */
	    if (argv[cnt][1]=='s' && argv[cnt][2]=='\0')
	    {
                strip_all = 1;
		argv[cnt][1] = 'r'; /* Just some non-harming option... */
	    }
	    else
	    /* The user just requested help info, don't do anything else */
	    if (strncmp(&argv[cnt][1], "-help", 6) == 0)
	    {
	        /* I know, it's not incremental linking we're after, but the end result
		   is the same */
	        incremental = 1;
	        break;
	    }
	    else
	    /* verbose output */
	    if (strncmp(&argv[cnt][1], "-verbose", 9) == 0)
	    {
	        do_verbose = argv[cnt];
	        break;
	    }
	}
    }

    ldargs = xmalloc(sizeof(char *) * (argc+2 + 2*(incremental == 0)));

    ldargs[0] = "ld";
    ldargs[1] = "-r";

    for (cnt = 1; cnt < argc; cnt++)
    	ldargs[cnt+1] = argv[cnt];

    if (!incremental)
    {
        atexit(exitfunc);

	if
	(
	    !(tempoutput   = make_temp_file(NULL))     ||
	    !(ldscriptname = make_temp_file(NULL))     ||
	    !(ldscriptfile = fopen(ldscriptname, "w"))
	)
	{
	    fatal(ldscriptname ? ldscriptname : "make_temp_file()", strerror(errno));
	}

        ldargs[cnt + 1] = "-o";
        ldargs[cnt + 2] = tempoutput;
	cnt += 2;
    }

    ldargs[cnt+1] = NULL;

    docommandvp("ld", ldargs);

    if (incremental)
        return EXIT_SUCCESS;

    collect_sets(tempoutput, &setlist);

    if (setlist != NULL)
        fprintf(ldscriptfile, "EXTERN(__this_program_requires_symbol_sets_handling)\n");

    fwrite(LDSCRIPT_PART1, sizeof(LDSCRIPT_PART1) - 1, 1, ldscriptfile);
    emit_sets(setlist, ldscriptfile);
    fwrite(LDSCRIPT_PART2, sizeof(LDSCRIPT_PART2) - 1, 1, ldscriptfile);

    fclose(ldscriptfile);
    ldscriptfile = NULL;

    docommandlp("ld", "ld", "-r", "-o", output, tempoutput, "-T", ldscriptname, do_verbose, NULL);

    if (!ignore_undefined_symbols && check_and_print_undefined_symbols(output))
    {
        remove(output);
        return EXIT_FAILURE;
    }

    chmod(output, 0766);

    if (strip_all)
    {
        docommandlp("strip", "strip", "--strip-unneeded", output, NULL);
    }

    return 0;
}
