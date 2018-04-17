/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <dos/dos.h>

#include <stdio.h>
#include <strings.h>

/*
    Test output using various float printing formats.
*/

int main(void)
{
    int retval = RETURN_OK;

    char buffer[50];
    float xf = 3.14;
    double xd = 6.28;
    
    sprintf(buffer, "%6.3f %6.3f", xf, xd);

    if (strcmp(buffer, " 3.140  6.280"))
    {
        bug("float/double string is: %s\n", buffer);
        bug("expected result was: %s\n", " 3.140  6.280");
        retval = RETURN_ERROR;
    }

	xd = 2;
	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x1p+1"))
    {
        bug("2 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x1p+1");
        retval = RETURN_ERROR;
    }

	xd = 256;
	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x1p+8"))
    {
        bug("2^8 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x1p+8");
        retval = RETURN_ERROR;
    }

	xd = 0.015625; //= 2^-6 
 	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x1p-6"))
    {
        bug("2^-6 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x1p-6");
        retval = RETURN_ERROR;
    }

	xd = 0.857421875;
	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x1.b7p-1"))
    {
        bug("0.857421875 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x1.b7p-1");
        retval = RETURN_ERROR;
    }

	xd = 0x1p-1074; //Smallest double (unnormalized)
	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x0.0000000000001p-1022"))
    {
        bug("0x1p-1074 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x0.0000000000001p-1022");
        retval = RETURN_ERROR;
    }

	xd = 3.1415926;
	sprintf(buffer, "%A", xd);
    if (strcmp(buffer, "0X1.921FB4D12D84AP+1"))
    {
        bug("3.1415926 in upper case hex: %s\n", buffer);
        bug("expected result was: %s\n", "0X1.921FB4D12D84AP+1");
        retval = RETURN_ERROR;
    }

	xd = 0.1;
	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x1.999999999999ap-4"))
    {
        bug("0.1 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x1.999999999999ap-4");
        retval = RETURN_ERROR;
    }

	xd = 0x3.3333333333334p-5;
	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x1.999999999999ap-4"))
    {
        bug("0x3.3333333333334p-5 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x1.999999999999ap-4");
        retval = RETURN_ERROR;
    }

	xd = 0xcc.ccccccccccdp-11;
	sprintf(buffer, "%a", xd);
    if (strcmp(buffer, "0x1.999999999999ap-4"))
    {
        bug("0xcc.ccccccccccdp-11 in hex: %s\n", buffer);
        bug("expected result was: %s\n", "0x1.999999999999ap-4");
        retval = RETURN_ERROR;
    }

    return retval;
}
