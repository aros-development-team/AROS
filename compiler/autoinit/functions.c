/*
    Copyright (C) 1995-2000 AROS - The Amiga Research OS
    $Id$

    Desc: autoinit library - functions sets handling
    Lang: english
*/

typedef int (*fptr)(void);

int set_call_funcs(fptr list[], int order)
{
    int n;

    if (order>=0)
    {
	int ret;

	n = 1;
	while(list[n])
	    if ((ret = list[n++]())) return ret;
    }
    else
    {
	n = ((int *)list)[0];

	while (n)
	    (void)list[n--]();
    }

    return 0;
}

