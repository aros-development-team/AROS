/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    A link library member with entries in the INIT and EXIT sets, at a
    priority that nothing else in the program uses.
*/

#include <proto/dos.h>
#include <aros/symbolsets.h>

static int setmember_state;

static int setmember_init(void)
{
    setmember_state = 1;
    return 1;
}

static void setmember_exit(void)
{
    PutStr("exit set function called\n");
}

int setmember_value(void)
{
    return setmember_state;
}

ADD2INIT(setmember_init, 42);
ADD2EXIT(setmember_exit, 42);
