/*
    (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: english
*/

struct HistEntry
{
 unsigned int Count;
 unsigned long Red;
 unsigned long Green;
 unsigned long Blue;
};

unsigned int *MakeARGB(unsigned long *ColRegs, unsigned int Count);
unsigned int CountColors(unsigned int *ARGB, unsigned int Count);
int HistSort(const void *HistEntry1, const void *HistEntry2);


