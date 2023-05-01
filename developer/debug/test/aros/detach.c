#include <stdio.h>
#include <aros/detach.h>

int main(void)
{
    printf("vor detach\n");
    Detach();
    printf("nach detach\n");
    return 0;
}
