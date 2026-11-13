/* Copyright (C) 2026, The AROS Development Team. All rights reserved. */
#include <proto/dos.h>

int main(int argc, char **argv)
{
    char data[64];
    BPTR result;
    LONG count;

    if (argc != 2)
        return RETURN_FAIL;
    count = Read(Input(), data, sizeof data);
    if (count < 0 || Write(Output(), data, count) != count)
        return RETURN_FAIL;
    result = Open((CONST_STRPTR)argv[1], MODE_NEWFILE);
    if (!result)
        return RETURN_FAIL;
    if (!count)
    {
        data[0] = 'E'; data[1] = 'M'; data[2] = 'P';
        data[3] = 'T'; data[4] = 'Y'; data[5] = '\n';
        count = 6;
    }
    if (Write(result, data, count) != count)
    {
        Close(result);
        return RETURN_FAIL;
    }
    return Close(result) ? RETURN_OK : RETURN_FAIL;
}
