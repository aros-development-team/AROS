/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "env.h"
#include "misc.h"
#include "docommand.h"
#include "backend.h"
#include "ldscript.h"
#include "gensets.h"

#define EXTRA_ARG_CNT 2

#define EI_OSABI        7
#define EI_ABIVERSION   8

#define ELFOSABI_AROS   15


static char *ldscriptname, *tempoutput, *ld_name, *strip_name;
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

static int set_os_and_abi(const char *file)
{
    int f;
    const unsigned char osabi = ELFOSABI_AROS;
    const unsigned char abiversion = 1;

    /* Modify OS and ABI fields */

    f = open(file, O_RDWR);
    if (f >= 0) {
        lseek(f, EI_OSABI, SEEK_SET);
        if (write(f, &osabi, 1) == 1) {
            lseek(f, EI_ABIVERSION, SEEK_SET);
            if (write(f, &abiversion, 1) == 1) {
                close(f);
                return 1;
            }
        }
    }

    perror(file);
    if (f >= 0)
    	    close(f);
    return 0;
}

int main(int argc, char *argv[])
{
    int cnt, i;
    char *output, **ldargs;
    /* incremental = 1 -> don't do final linking.
       incremental = 2 -> don't do final linking AND STILL produce symbol sets.  */
    int incremental = 0, ignore_undefined_symbols = 0;
    int strip_all   = 0;
    char *do_verbose = NULL;

    setnode *setlist = NULL, *liblist = NULL;

    program_name = argv[0];
    ld_name = LD_NAME;
    strip_name = STRIP_NAME;

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
	        incremental  = 1;
	    else
	    /* Incremental, but produce the symbol sets */
	    if (strncmp(&argv[cnt][1], "Ur", 3) == 0)
	    {
                incremental  = 2;
                
		argv[cnt][1] = 'r';  /* Just some non-harming option... */
		argv[cnt][2] = '\0';
	    }
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

    ldargs = xmalloc(sizeof(char *) * (argc + EXTRA_ARG_CNT
        + ((incremental == 1) ? 0 : 2) + 1));

    ldargs[0] = ld_name;
    ldargs[1] = OBJECT_FORMAT;
    ldargs[2] = "-r";

    for (i = 1; i < argc; i++)
        ldargs[i + EXTRA_ARG_CNT] = argv[i];
    cnt = argc + EXTRA_ARG_CNT;

    if (incremental != 1)
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

        ldargs[cnt++] = "-o";
        ldargs[cnt++] = tempoutput;
    }

    ldargs[cnt] = NULL;
              
    docommandvp(ld_name, ldargs);

    if (incremental == 1)
        return set_os_and_abi(output) ? EXIT_SUCCESS : EXIT_FAILURE;

    collect_libs(tempoutput, &liblist);
    collect_sets(tempoutput, &setlist);

    if (setlist) {
        struct setnode *n;
        for (n = setlist; n; n = n->next) {
            if (strncmp(n->secname,".aros.set.",10) == 0) {
               fprintf(ldscriptfile, "EXTERN(__%s__symbol_set_handler_missing)\n", &n->secname[10]);
            }
        }
    }

    fwrite(LDSCRIPT_PART1, sizeof(LDSCRIPT_PART1) - 1, 1, ldscriptfile);
    emit_sets(setlist, ldscriptfile);
    emit_libs(liblist, ldscriptfile);
    fwrite(LDSCRIPT_PART2, sizeof(LDSCRIPT_PART2) - 1, 1, ldscriptfile);
    /* Append .eh_frame terminator only on final stage */
    if (incremental == 0)
    	fputs("LONG(0)\n", ldscriptfile);
    fwrite(LDSCRIPT_PART3, sizeof(LDSCRIPT_PART3) - 1, 1, ldscriptfile);
    fwrite(LDSCRIPT_PART4, sizeof(LDSCRIPT_PART4) - 1, 1, ldscriptfile);

    fclose(ldscriptfile);
    ldscriptfile = NULL;

    docommandlp(ld_name, ld_name, OBJECT_FORMAT, "-r", "-o", output,
        tempoutput, "-T", ldscriptname, do_verbose, NULL);

    if (incremental != 0)
        return set_os_and_abi(output) ? EXIT_SUCCESS : EXIT_FAILURE;
        
    if (!ignore_undefined_symbols && check_and_print_undefined_symbols(output))
    {
        remove(output);
        return EXIT_FAILURE;
    }

    chmod(output, 0766);

    if (strip_all)
    {
        docommandlp(strip_name, strip_name, "--strip-unneeded", output, NULL);
    }

    if (!set_os_and_abi(output))
    {
        remove(output);
        return EXIT_FAILURE;
    }

    return 0;
}
