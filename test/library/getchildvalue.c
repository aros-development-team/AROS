/*
 * This is part of userel.library to check if it can access global variable
 * defined in pertask.library.
 */

#include "pertaskvalue.h"

int GetChildValue(void)
{
    return pertaskvalue;
}
