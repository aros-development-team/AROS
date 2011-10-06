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

        Extract some characters or words from a string.

    FORMAT

        CUT <string> [CHAR <range> | WORD <range> [SEPARATOR <string>] ]

    RESULT

        Standard DOS return codes.

    NOTES

Quoted from http://www.titan.co.nz/amigaak/AA020844e2C.htm :

The extracted string is defined by a begin and an end position. Those values
will be characters or words positions in the original string, i.e you may want
to extract a string beginning with a character at position P1 and ending with a
character at position P2. Behaviour is the same with words instead of
characters.

Use the CHAR argument if you want to use begin/end values defined in characters.
Use the WORD argument if you want to extract any number of words. Words are
strings separated by the "space" character (default). Using the SEPARATOR
argument, you can specify a string of any length to be used to split the
original string in words.

The length of the string to extract will depend on the begin (P1) and the end
(P2) position in the original string. This P1-P2 range to give after the CHAR
(or WORD) argument follows the template:

P1[-P2] | [P1-]P2 | [P1]-P2 | P1-[P2]

The begin (P1) and end (P2) values are optional. This allows to extract only one
character (or word) if you omit the end value. i.e with the argument like
"CHAR P1" In order to extract several characters (or words), you need to specify
a range with the "-" character like "CHAR P1-P2"

You can omit P1 if you want a string starting at the beginning of <string> with
"CHAR -P2". And you do not need to know the string length because P2 can be
omitted like "CHAR P1-". This will extract the string beginning with character
at position P1 and ending at the end of the original <string>.

    EXAMPLES

Example 1:
3.OS4:> CUT "Hello world" char 2
e
extract one character.

Example 2:
3.OS4:> CUT "Hello world" char 1-5
Hello
extract from character 1 to 5.

Example 3:
3.OS4:> CUT "Hello world" char -5
Hello
extract from character 1 to 5 without specifying the beginning position.

Example 4:
3.OS4:> CUT "Hello world" char 7-
world
extract from character 7 of the string till the end.

Example 5:
3.OS4:> CUT "Hello world" word 1 separator "ll"
He
extract one word (with another separator).


******************************************************************************/

#include <proto/exec.h>
#include <proto/dos.h>

//#define DEBUG 1
#include <aros/debug.h>

#include <aros/shcommands.h>

#ifdef AROS_SHAH
#undef AROS_SHAH
#endif
#define AROS_SHAH(type, abbr, name, modf, def, help) type,abbr,name,modf,def, __SHA_OPT(type,abbr,name,modf,def,help) "\t" help "\n"

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))

struct Bounds
{
    LONG Start;
    LONG End;
};


static struct Bounds *getBoundaries(STRPTR String, APTR DOSBase);

AROS_SH4H(Cut,50.1,        "extract some characters or words from a string\n",
AROS_SHAH(STRPTR,  ,STRING   ,/A,NULL,"Quoted string from which to extract characters or words"),
AROS_SHAH(STRPTR,C=,CHAR     ,/K,NULL,"Use begin/end values defined in characters"),
AROS_SHAH(STRPTR,W=,WORD     ,/K,NULL,"Extract any number of words separated by SEPARATOR"),
AROS_SHAH(STRPTR,S=,SEPARATOR,/K, " ", "Specify a string of any length to be used to split\n"
                                   "\t\tthe original string in words (default: space character)\n") )
{
    AROS_SHCOMMAND_INIT

    LONG ioerr, stringLen, sepLen, wordCnt = 0;
    int  rc    = RETURN_FAIL;
    struct Bounds *charBounds, *wordBounds;
    STRPTR stringBuf = SHArg(STRING), sepBuf/* = SHArg(SEPARATOR)*/;

    if (!SHArg(CHAR) && !SHArg(WORD))
        SetIoErr(ERROR_BAD_TEMPLATE);

    stringLen = strlen((char *)SHArg(STRING));
    sepLen = strlen((char *)SHArg(SEPARATOR));

    if (SHArg(WORD))
    {
        if ((wordBounds = getBoundaries(SHArg(WORD), DOSBase)) != NULL)
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
        if ((charBounds = getBoundaries(SHArg(CHAR), DOSBase)) != NULL)
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


static struct Bounds *getBoundaries(STRPTR String, APTR DOSBase)
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
    else
        SetIoErr(ERROR_NO_FREE_STORE);

    return bounds;
}


