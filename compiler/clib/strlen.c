int strlen (const char * ptr)
{
    int len=0;

    while (*ptr++) len++;

    return len;
}

