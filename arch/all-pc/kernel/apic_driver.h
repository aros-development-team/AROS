/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Generic AROS APIC definitions.
    Lang: english
*/

struct GenericAPIC
{
    const char *name;
    IPTR      (*probe)(void);
    IPTR      (*getbase)(void);
    IPTR      (*getid)(IPTR base);
    IPTR      (*wake)(APTR startrip, UBYTE apicid, IPTR base);
    IPTR      (*init)(IPTR base);
    void      (*ack)(UBYTE intnum);
};

const struct GenericAPIC *core_APIC_Probe(void);
