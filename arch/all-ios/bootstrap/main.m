#include <stdlib.h>
#include <string.h>

#include "appdelegate.h"

int _argc;
char **_argv;

int main(int argc, char **argv)
{
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int i;

    /* The easiest way to pass arguments to the bootstrap. 
       TODO: in real life we have no way to specify them. Need
       to teach bootstrap to store them in the configuration file. */
    _argc = argc;
    _argv = argv;

    i = UIApplicationMain(argc, argv, NULL, @"BootstrapDelegate");

    [pool release];
    return i;
}
