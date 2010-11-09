#include <stdarg.h>

#include "appdelegate.h"
#include "ui.h"

void DisplayError(char *fmt, ...)
{
    va_list ap;
    NSString *format = [NSString stringWithCString:fmt encoding:NSISOLatin1StringEncoding];
    BootstrapDelegate *ad = (BootstrapDelegate *)[[UIApplication sharedApplication] delegate];

    va_start(ap, fmt);

    NSString *text = [[NSString alloc] initWithFormat:format arguments:ap];

    [ad ShowAlert:text withTitle:@"AROS bootstrap error"];
    [text release];

    va_end(ap);
}
