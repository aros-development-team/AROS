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

    return 0;
}
