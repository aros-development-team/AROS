/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <math.h>

static int failures;
#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); failures++; } } while (0)

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
            failures++, printf("RESULT FAILURE @ %s, should be %f was %f\n", src, newlinetestresults[i], f);
        if (ptroffset[i] != (next - src))
            failures++, printf("OFFSET FAILURE @ %s, should be %d was %d\n", src, ptroffset[i], (int)(next - src));
        if(next <= src) break;
    }
    
    if (TESTLINELINE_I != i)
        failures++, printf("ITER FAILURE @ %s, should be %d was %d\n", buffer, TESTLINELINE_I, i);
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
            failures++, printf("RESULT FAILURE @ %s, should be %f was %f\n", str, results[i], f);
        if ((float_end - str) != ptroffset[i])
            failures++, printf("OFFSET FAILURE @ %s, should be %d was %d\n", str, ptroffset[i], (int)(float_end - str));
        i++;
    }
    
    /* Check bahavior with new-lined strings */
    testnewline(newlinetest_rn, newlinetestptroffset_rn);
    testnewline(newlinetest_n, newlinetestptroffset_n);


    {
        struct { const char *s; uint64_t bits; int offset, range; } cases[] = {
            {"0.002", 0x3f60624dd2f1a9fcULL, 5, 0},
            {"0.05", 0x3fa999999999999aULL, 4, 0},
            {"-0", 0x8000000000000000ULL, 2, 0},
            {"0x1.8p3!", 0x4028000000000000ULL, 7, 0},
            {"0x.8p-1", 0x3fd0000000000000ULL, 7, 0},
            {"0x1p-1074", 1, 9, 0},
            {"0x1p-1075", 0, 9, 1},
            {"0x3p-1075", 2, 9, 1},
            {"1e99999", 0x7ff0000000000000ULL, 7, 1},
            {"-1e-99999", 0x8000000000000000ULL, 9, 1},
            {"1.00000000000000011102230246251565404236316680908203125", 0x3ff0000000000000ULL, 55, 0},
            {"1.00000000000000011102230246251565404236316680908203126", 0x3ff0000000000001ULL, 55, 0},
            {"3.14", 0x40091eb851eb851fULL, 4, 0},
            {"0.1", 0x3fb999999999999aULL, 3, 0},
            {"1e22", 0x4480f0cf064dd592ULL, 4, 0},
            {"1e23", 0x44b52d02c7e14af6ULL, 4, 0},
            {"123456789012345e22", 0x476db89cafccd3a9ULL, 18, 0},
            {"1234567890123456", 0x43118b54f22aeb00ULL, 16, 0},
            {"9007199254740993", 0x4340000000000000ULL, 16, 0},
            {"4503599627370497e-15", 0x401203af9ee75617ULL, 20, 0},
            {"-7.9036812831662", 0xc01f9d5ea054d317ULL, 16, 0},
            {"64312.0169300046", 0x40ef67008ab0cb03ULL, 16, 0},
            {"0.0042894744920585", 0x3f7191d70a9111e9ULL, 18, 0},
        };
        size_t k;
        for (k=0;k<sizeof(cases)/sizeof(cases[0]);k++)
        {
            uint64_t bits;
            char *end;
            errno=0;
            f=strtod(cases[k].s,&end);
            memcpy(&bits,&f,sizeof(bits));
            CHECK(bits==cases[k].bits);
            CHECK(end-cases[k].s==cases[k].offset);
            CHECK(errno==(cases[k].range?ERANGE:0));
        }
    }
    {
        double d=99;
        float small=0;
        long double large=0;
        char c=0;
        int n=-1;
        CHECK(sscanf("0.002!", "%lf%n%c", &d,&n,&c)==2);
        CHECK(d==0.002 && n==5 && c=='!');
        CHECK(sscanf("12.5", "%2lf%c", &d,&c)==2 && d==12 && c=='.');
        CHECK(sscanf("  0x1.8p3 tail", "%la%n", &d,&n)==1 && d==12 && n==9);
        CHECK(sscanf("0.05", "%f", &small)==1 && small==0.05f);
        CHECK(sscanf("1.25", "%Lf", &large)==1 && large==1.25L);
        CHECK(sscanf("infinity", "%lf", &d)==1 && isinf(d));
        CHECK(sscanf("nan", "%lf", &d)==1 && isnan(d));
        CHECK(sscanf("word", "%lf", &d)==0);
        CHECK(sscanf("", "%lf", &d)==EOF);
        CHECK(sscanf("12.5 0.05", "%*lf%lf", &d)==1 && d==0.05);
    }
    {
        char *huge=malloc(20064), *end;
        CHECK(huge != NULL);
        if (huge)
        {
            huge[0]='1'; memset(huge+1,'0',20000); strcpy(huge+20001,"e-20000");
            f=strtod(huge,&end); CHECK(f==1 && *end==0);
            CHECK(sscanf(huge,"%lf",&f)==1 && f==1);
            strcpy(huge,"1.00000000000000011102230246251565404236316680908203125");
            memset(huge+55,'0',20000-55); huge[20000]='1';huge[20001]=0;
            f=strtod(huge,&end); CHECK(f==0x1.0000000000001p0 && *end==0);
            free(huge);
        }
    }
    printf("strtod regression: %s (%d failures)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 20 : 0;
}
