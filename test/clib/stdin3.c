#include <errno.h>
#include <stdio.h>

int main(void)
{
    puts("Type a line and it should be repeated");

    char c;
    for (c = getc(stdin);
         c != '\n' && c != EOF && errno == 0;
         c = getc(stdin)
    )
        putc(c, stdout);
    putc('\n', stdout);

    return 0;
}
