/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
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


