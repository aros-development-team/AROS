/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdlib.h>
#include <stdio.h>

const char * strings [] = 
{
    "1e1",
    "1.2e2",
    ".1e3",
    ".e4",
    "exp",
    "e",
    "12e ",
    "12.1ef",
    "12.e1",
    "12.e",
    "10.",
    "10e+2",
    "1000e-2",
    "20e+ ",
    "20e- ",
    "-exp",
    "+dummy",
    "+10",
    "-12.1e",
    "-.e+4",
    "-.1e+3",
    NULL
};

const double results [] =
{
    10.0,
    120.0,
    100.0,
    0.0,
    0.0,
    0.0,
    12.0,
    12.1,
    120,
    12,
    10,
    1000,
    10,
    20,
    20,
    0,
    0,
    10.0,
    -12.1,
    0,
    -100.0,
};

const int ptroffset [] =
{
    3,
    5,
    4,
    0,
    0,
    0,
    2,
    4,
    5,
    3,
    3,
    5,
    7,
    2,
    2,
    0,
    0,
    3,
    5,
    0,
    6
};

const char * newlinetest_rn = "\t-2.700264 -1.792122 -2.037897\r\n\t-0.084267 0.081827 0.584534\r\n";
const char * newlinetest_n = "\t-2.700264 -1.792122 -2.037897\n\t-0.084267 0.081827 0.584534\n";

const double newlinetestresults [] =
{
    -2.700264,
    -1.792122,
    -2.037897,
    -0.084267,
    0.081827,
    0.584534,
    0.000000
};

const int newlinetestptroffset_rn [] =
{
    10,
    10,
    10,
    12,
    9,
    9,
    0,
    -1,
    -1
};

const int newlinetestptroffset_n [] =
{
    10,
    10,
    10,
    11,
    9,
    9,
    0,
    -1,
    -1
};

#define TESTLINELINE_I  6

void testnewline(const char * buffer, const int * ptroffset)
{
    int i = 0; char * src = NULL; char *next;
    for(src = (char *)buffer; i < (TESTLINELINE_I + 2); i++, src = next)
    {
        double f = strtod(src, &next);
        if ((float)newlinetestresults[i] != (float)f)
            printf("RESULT FAILURE @ %s, should be %f was %f\n", src, newlinetestresults[i], f);
        if (ptroffset[i] != (next - src))
            printf("OFFSET FAILURE @ %s, should be %d was %d\n", src, ptroffset[i], (int)(next - src));
        if(next <= src) break;
    }
    
    if (TESTLINELINE_I != i)
        printf("ITER FAILURE @ %s, should be %d was %d\n", buffer, TESTLINELINE_I, i);
}

int main()
{
    char * float_end = NULL;
    double f = 0;
    int i = 0;
    const char * str = NULL;

    while((str = strings[i]) != NULL)
    {
        f = strtod(str, &float_end);
        if (f != results[i])
            printf("RESULT FAILURE @ %s, should be %f was %f\n", str, results[i], f);
        if ((float_end - str) != ptroffset[i])
            printf("OFFSET FAILURE @ %s, should be %d was %d\n", str, ptroffset[i], (int)(float_end - str));
        i++;
    }
    
    /* Check bahavior with new-lined strings */
    testnewline(newlinetest_rn, newlinetestptroffset_rn);
    testnewline(newlinetest_n, newlinetestptroffset_n);

    return 0;
}
