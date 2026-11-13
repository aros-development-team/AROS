#define __BSD_VISIBLE 1

#include <stdio.h>
#include <netinet/tcp.h>

int main(void)
{
    struct tcphdr tcp;
    printf("%p\n", &tcp);
    return 0;
}
