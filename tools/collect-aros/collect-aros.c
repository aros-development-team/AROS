/*
    Copyright (C) 1995-2023, The AROS Development Team. All rights reserved.
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

int have_gnunm = 0;

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

struct libentry
{
    int idx;
    char *str;
};

int main(int argc, char *argv[])
{
    struct libentry llibs[argc];
    struct libentry rellibs[argc];
    char *output, **ldargs;
    char *do_verbose = NULL;
    /* incremental = 1 -> don't do final linking.
       incremental = 2 -> don't do final linking AND STILL produce symbol sets.  */
    int incremental = 0, ignore_undefined_symbols = 0;
    int strip_all   = 0;
    int cnt, i, libcnt = 0, relcnt = 0;

    setnode *setlist = NULL, *liblist = NULL, *extralist = NULL;

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
            /* linklib */
            if (strncmp(&argv[cnt][1], "l", 1) == 0)
            {
                int lnlen = strlen(&argv[cnt][2]);
                if ((lnlen > 4) && (strcmp("_rel", &argv[cnt][lnlen - 2]) == 0))
                {
                    rellibs[relcnt].idx = cnt;
                    rellibs[relcnt++].str = &argv[cnt][2];
                }
                else
                {
                    llibs[libcnt].idx = cnt;
                    llibs[libcnt++].str = &argv[cnt][2];
                }
            }
            else
            /* Complete stripping is requested, but we do it our own way */
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

    // Fixup relbase lib linking 
    if (relcnt > 0)
    {
        int relid, libid;
        for (relid = 0; relid < relcnt; relid++)
            for (libid = 0; libid < libcnt; libid++)
            {
                if (!strncmp(llibs[libid].str, rellibs[relid].str, strlen(llibs[libid].str)))
                {
                    ldargs[llibs[libid].idx + EXTRA_ARG_CNT] = ldargs[rellibs[relid].idx + EXTRA_ARG_CNT];
                }
            }
    }
  
    
    ldargs[cnt] = NULL;
              
    docommandvp(ld_name, ldargs);

    if (incremental == 1)
        return set_os_and_abi(output) ? EXIT_SUCCESS : EXIT_FAILURE;

    /*
       * initalize the backend and process the 
       * remaining work
       */
    backend_init(ldargs[0]);
    collect_sets(tempoutput, &setlist);
    collect_extra(tempoutput, &extralist);
    collect_libs(tempoutput, &liblist);

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
    
    char **cmdargv;
    int argallocs = 0;
    if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            argallocs += 1;
    }

#ifdef TARGET_FORMAT_EXE
    if (incremental == 0)
    {
#ifdef OBJECT_FORMAT_EXTRA_FINAL
        argallocs += 10;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = OBJECT_FORMAT_EXTRA_FINAL;
        cmdargv[3] = "-o";
        cmdargv[4] = output;
        cmdargv[5] = tempoutput;

        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[6 + cnt] = "-T";
        cmdargv[7 + cnt] = ldscriptname;
        cmdargv[8 + cnt] = do_verbose;
        cmdargv[9 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
#else
        argallocs += 9;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = "-o";
        cmdargv[3] = output;
        cmdargv[4] = tempoutput;
        
        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[5 + cnt] = "-T";
        cmdargv[6 + cnt] = ldscriptname;
        cmdargv[7 + cnt] = do_verbose;
        cmdargv[8 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
#endif
    }
    else
    {
        argallocs += 10;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = "-r";
        cmdargv[3] = "-o";
        cmdargv[4] = output;
        cmdargv[5] = tempoutput;

        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[6 + cnt] = "-T";
        cmdargv[7 + cnt] = ldscriptname;
        cmdargv[8 + cnt] = do_verbose;
        cmdargv[9 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
    }
#else
#ifdef OBJECT_FORMAT_EXTRA_FINAL
    if (incremental == 0)
    {
        argallocs += 11;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = OBJECT_FORMAT_EXTRA_FINAL;
        cmdargv[3] = "-r";
        cmdargv[4] = "-o";
        cmdargv[5] = output;
        cmdargv[6] = tempoutput;
        
        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[7 + cnt] = "-T";
        cmdargv[8 + cnt] = ldscriptname;
        cmdargv[9 + cnt] = do_verbose;
        cmdargv[10 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
    }
    else
#endif
    {
        argallocs += 10;
        cmdargv = xmalloc(sizeof(char *) * (argallocs + 1));
        cmdargv[0] = ld_name;
        cmdargv[1] = OBJECT_FORMAT;
        cmdargv[2] = "-r";
        cmdargv[3] = "-o";
        cmdargv[4] = output;
        cmdargv[5] = tempoutput;

        cnt = 0;
        if (extralist) {
        struct setnode *n;
        for (n = extralist; n; n = n->next)
            cmdargv[6 + cnt++] = n->secname;
        }
    
        cmdargv[6 + cnt] = "-T";
        cmdargv[7 + cnt] = ldscriptname;
        cmdargv[8 + cnt] = do_verbose;
        cmdargv[9 + cnt] = NULL;

        docommandvp(ld_name, cmdargv);
    }
#endif

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
