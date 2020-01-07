/*
 * $Id$
 */

#include <string.h>
#include <stdio.h>

/* convert ISO-8859-2 strings to AmigaPL */
void iso88592toamigapl(char *str)
{
    int i, len = strlen(str);
    printf("[FlexCat] Translating '%s' ->", str);
    for (i = 0; i < len; i ++)
    {
        if (str[i] == 161)
            str[i] = 194;
        else if (str[i] == 163)
            str[i] = 206;
        else if (str[i] == 166)
            str[i] = 212;
        else if (str[i] == 198)
            str[i] = 202;
        else if (str[i] == 202)
            str[i] = 203;
        else if (str[i] == 209)
            str[i] = 207;
        else if (str[i] == 172)
            str[i] = 218;
        else if (str[i] == 175)
            str[i] = 219;
        else if (str[i] == 177)
            str[i] = 226;
        else if (str[i] == 179)
            str[i] = 238;
        else if (str[i] == 182)
            str[i] = 244;
        else if (str[i] == 188)
            str[i] = 250;
        else if (str[i] == 191)
            str[i] = 251;
        else if (str[i] == 230)
            str[i] = 234;
        else if (str[i] == 234)
            str[i] = 235;
        else if (str[i] == 241)
            str[i] = 239;
    }
    printf(" '%s'\n", str);
}

