/*
    (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Copy CLI command
    Lang: english
*/

/*****************************************************************************

    NAME

        Copy

    SYNOPSIS

        SOURCE/M/A,DEST/A

    LOCATION

        Workbench:c

    FUNCTION

        Creates identical copies of one or more files.

    INPUTS

        SOURCE - The files that are to be copied. You can specify a pattern
                 here.

        DEST   - The directory, where the copies of the files are to be
                 stored. If only one file was specified as SOURCE, this
                 can be a different filename. The new file will named DEST.

    RESULT

        Standard DOS return codes.

    NOTES

    EXAMPLE

    BUGS

        Pattern-Matching does not work, yet.

        Error messages could be more comprehensive.

    SEE ALSO

        Delete(), Rename(), MakeDir()

    INTERNALS

    HISTORY

******************************************************************************/

#include <aros/rt.h>

#include <proto/exec.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <utility/tagitem.h>


static const char version[] = "$VER: Copy 41.0 (29.08.1998)\n";


#define ARG_TEMPLATE "SOURCE/M/A,DEST/A"
#define ARG_SOURCE 0
#define ARG_DEST   1
#define ARG_NUM    2


#define ERROR_HEADER "Copy"



int copy_file(STRPTR source, STRPTR dest)
{
    int result = RETURN_OK;
    BPTR sf, df;

    sf = Open(source, MODE_OLDFILE);
    if(sf) {
        df = Open(dest, MODE_NEWFILE);
        if(df) {
            char buffer[1024];
            LONG read;
            while((read = Read(sf, buffer, sizeof(buffer))) > 0)
                Write(df, buffer, read);
            if(read == -1) {
                PrintFault(IoErr(), ERROR_HEADER);
                result = RETURN_ERROR;
            }
            Close(df);
        } else {
            PrintFault(IoErr(), ERROR_HEADER);
            result = RETURN_FAIL;
        }
        Close(sf);
    } else {
        PrintFault(IoErr(), ERROR_HEADER);
        result = RETURN_FAIL;
    }

    return result;
}


STRPTR make_dest_name(STRPTR source, STRPTR destdir)
{
    STRPTR dest;
    ULONG slen = (ULONG)strlen(source);
    ULONG dlen = (ULONG)strlen(destdir);

    dest = AllocVec(slen + dlen + 2, MEMF_ANY);
    if(!dest) {
        PrintFault(ERROR_NO_FREE_STORE, ERROR_HEADER);
        return NULL;
    }

    strcpy(dest, destdir);
    AddPart(dest, source, slen + dlen + 2);

    return dest;
}


int copy_files(STRPTR *source, STRPTR dest)
{
    int result = RETURN_OK;
    BOOL dest_is_dir = FALSE;
    BOOL multiple_sources = FALSE;

    BPTR lock;

    /* find out, if dest is a directory */
    lock = Lock(dest, SHARED_LOCK);
    if(!lock) {
        /* if IoErr() == ERROR_OBJECT_NOT_FOUND, dest is a plain file */
        if(IoErr() != ERROR_OBJECT_NOT_FOUND) {
            PrintFault(IoErr(), ERROR_HEADER);
            result = RETURN_FAIL;
        }
    } else {
        struct FileInfoBlock *fib;
        fib = AllocDosObject(DOS_FIB, NULL);
        if(fib) {
            if(Examine(lock, fib) == DOSTRUE) {
                if(fib->fib_DirEntryType >= 0)
                    dest_is_dir = TRUE;
            } else {
                PrintFault(IoErr(), ERROR_HEADER);
                result = RETURN_FAIL;
            }
            FreeDosObject(DOS_FIB, fib);
        } else {
            PrintFault(IoErr(), ERROR_HEADER);
            result = RETURN_FAIL;
        }
        UnLock(lock);
    }
    if(result)
        return result;

    /* find out, if we have multiple source files */
    if(dest[1])
        multiple_sources = TRUE;
    else {
#warning FIXME: pattern matching
    }

    if(multiple_sources && !dest_is_dir) {
        PrintFault(ERROR_OBJECT_WRONG_TYPE, ERROR_HEADER);
        result = RETURN_FAIL;
    }

    if(multiple_sources) {
        unsigned int pos = 0;
        STRPTR dest_name = NULL;
        while(source[pos] && result == RETURN_OK) {
#warning FIXME: pattern matching
            dest_name = make_dest_name(source[pos], dest);
            if(dest_name) {
                result = copy_file(source[pos], dest_name);
                FreeVec(dest_name);
            } else
                result = RETURN_FAIL;
            pos++;
        }
          
    } else { /* !multiple_sources */
        if(dest_is_dir) {
            STRPTR dest_name = make_dest_name(*source, dest);
            if(dest_name) {
                result = copy_file(*source, dest_name);
                FreeVec(dest_name);
            } else
                result = RETURN_FAIL;
        } else
            result = copy_file(*source, dest);
    }

    return result;
}


int main(int argc, char *argv[])
{
    int result = RETURN_OK;
    struct RDArgs *rda;
    IPTR argarray[ARG_NUM] = { NULL, NULL };

    RT_Init();

    rda = ReadArgs(ARG_TEMPLATE, argarray, NULL);
    if(rda) {
        result = copy_files((STRPTR *)argarray[ARG_SOURCE], (STRPTR)argarray[ARG_DEST]);
        FreeArgs(rda);
    } else {
        PrintFault(IoErr(), ERROR_HEADER);
        result = RETURN_FAIL;
    }

    RT_Exit();

    return result;
}
