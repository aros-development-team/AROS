/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

