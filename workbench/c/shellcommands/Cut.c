/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id:$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Cut

    TEMPLATE

        STRING/A,C=CHAR/K,W=WORD/K,S=SEPARATOR/K

    LOCATION

        SYS:C

    FUNCTION

        Extracts some characters or words from a string.

    FORMAT

        CUT <string> [CHAR <range> | WORD <range> [SEPARATOR <string>] ]

    RESULT

        Standard DOS return codes.

    NOTES

        Both  CHAR  (character)  and WORD arguments allow to define a begin and an end
        position  in the original string. Words are strings separated by a SEPARATOR (a
        space character (" ") is the default), which can also be a string. 

        Positions  range  is specified using numbers with the form "P1-P2", where "P1"
        is  the position of the first character (resp. word) to extract in the original
        string, "-" is the hyphen-minus character, and "P2" is the position of the last
        character (resp. word) to extract.

        If  only  one  position  is  supplied, then only one character (resp. word) is
        extracted,  unless  the  hyphen-minus  character is supplied too: P- extracts a 
        string  begining  at  the  character (resp. word) at position P in the original
        string until the end, and -P extracts a string starting at the beginning of the
        original  string,  and  ending with the character (resp. word) at position P in
        the original string.

    EXAMPLES

        Example 1:
        > Cut "A dummy sentence" CHAR 7
        y
        extract one character.

        Example 2:
        > Cut "A dummy sentence" CHAR 6-12
        my sent
        extract from character 6 to 12.

        Example 3:
        > Cut "A dummy sentence" CHAR -7
        A dummy
        extract from character 1 to 7 without specifying the beginning position.

        Example 4:
        > Cut "A dummy sentence" CHAR 12-
        tence
        extract from character 12 of the string until the end.

        Example 5:
        > Cut "A dummy sentence" WORD 2 SEPARATOR "en"
        t
        extract the second word (using an user-defined separator).


******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>

//#define DEBUG 1
#include <aros/debug.h>

#include <aros/shcommands.h>

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

struct Bounds
{
    LONG Start;
    LONG End;
};


static struct Bounds *getBoundaries(STRPTR String, APTR DOSBase, struct ExecBase *SysBase);

AROS_SH4H(Cut,50.1, "extract some characters or words from a string\n",
AROS_SHAH(STRPTR,  ,STRING   ,/A,NULL,"Quoted string from which to extract CHARacters or WORDs"),
AROS_SHAH(STRPTR,C=,CHAR     ,/K,NULL,"Extract one or a range of characters, form is P1-P2"),
AROS_SHAH(STRPTR,W=,WORD     ,/K,NULL,"Extract one or a range of words separated by SEPARATOR"),
AROS_SHAH(STRPTR,S=,SEPARATOR,/K," " ,"Specify a string of any length to be used to split\n"
                                  "\t\tthe original STRING in WORDs (default: space character)"

                             "\n\nExample: extract from character 6 to 12:\n"
                                 "> Cut \"A dummy sentence\" CHAR 6-12\n"
                                 "my sent\n") )
{
    AROS_SHCOMMAND_INIT

    int  rc    = RETURN_FAIL;
    LONG ioerr, stringLen, sepLen, wordCnt = 0;
    struct Bounds *charBounds, *wordBounds;
    STRPTR stringBuf = SHArg(STRING), sepBuf;

    if (!SHArg(CHAR) && !SHArg(WORD))
        SetIoErr(ERROR_BAD_TEMPLATE);

    stringLen = strlen((char *)SHArg(STRING));
    sepLen = strlen((char *)SHArg(SEPARATOR));

    if (SHArg(WORD))
    {
        if ((wordBounds = getBoundaries(SHArg(WORD), DOSBase, SysBase)) != NULL)
        {
            if (0 != wordBounds->Start)
            {
                wordCnt = wordBounds->Start - 1;
                while (wordCnt && (sepBuf = (STRPTR)strstr((char *)stringBuf, (char *)SHArg(SEPARATOR))))
                {
                    stringBuf = sepBuf + sepLen;
                    wordCnt--;
                }
            }

            if (0 == wordCnt)
            {
                if (wordBounds->End)
                    wordCnt = wordBounds->End - (wordBounds->Start ? wordBounds->Start : 1) + 1;
                else
                    /* Make sure to reach the end of the string in any case */
                    wordCnt = strlen((char *)stringBuf);

                while (*stringBuf)
                {
                    sepBuf = (STRPTR)strstr((char *)stringBuf, (char *)SHArg(SEPARATOR));
                    if (sepBuf && (sepBuf == stringBuf))
                        wordCnt--;

                    if (0 < wordCnt)
                        FPutC(Output(), *stringBuf++);
                    else
                        break;
                }
            }

            FreeVec(wordBounds);
            rc = RETURN_OK;
        }
    }

    if (SHArg(CHAR))
    {
        if ((charBounds = getBoundaries(SHArg(CHAR), DOSBase, SysBase)) != NULL)
        {
            if (0 != charBounds->Start)
                stringBuf += charBounds->Start - 1;

            while((stringBuf - SHArg(STRING)) < ((0 != charBounds->End) ? min(stringLen, charBounds->End) : stringLen))
            {
                FPutC(Output(), *stringBuf++);
            }

            FreeVec(charBounds);
            rc = RETURN_OK;
        }
    }

    if (rc == RETURN_OK)
        FPutC(Output(), '\n');
    else 
    {
        ioerr = IoErr();
        PrintFault(ioerr, (CONST_STRPTR)"Cut");
        if (ioerr == ERROR_BAD_TEMPLATE)
            rc = RETURN_ERROR;
    }

    return rc;

    AROS_SHCOMMAND_EXIT
}


static struct Bounds *getBoundaries(STRPTR String, APTR DOSBase, struct ExecBase *SysBase)
{
    CONST_STRPTR buffer;
    struct Bounds *bounds;

    if ((buffer = String) == NULL)
    {
        SetIoErr(ERROR_BAD_TEMPLATE);
        return NULL;
    }

    D(bug("[Cut] getBoundaries() buffer = '%s'\n", buffer));

    if ((bounds = AllocVec(sizeof(struct Bounds), MEMF_ANY | MEMF_CLEAR)) != NULL)
    {
        if (*buffer != '-')
        {
            buffer += StrToLong(buffer, &bounds->Start);
        }

        /* It is mandatory that boundaries are separated by '-'! */
        if ((*buffer) && (*buffer++ == '-'))
        {
            if (*buffer)
            {
                StrToLong(buffer, &bounds->End);
            }
        }
        else
            bounds->End = bounds->Start;

        D(bug("[Cut] getBoundaries() bounds->Start = %ld, bounds->End = %ld\n",
            bounds->Start, bounds->End));
    }

    return bounds;
}


