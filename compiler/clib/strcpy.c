
char * strcpy (char * dest, const char * src)
{
    char * ptr = dest;

    while ((*ptr++ = *src ++));

    return dest;
}

