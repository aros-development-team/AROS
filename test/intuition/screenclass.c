#include <intuition/diattr.h>
#include <intuition/iobsolete.h>
#include <proto/intuition.h>

#include <stdio.h>

int main(void)
{
    struct Screen *scr = LockPubScreen(NULL);
    IPTR val;

    if (!scr)
    {
        printf("Failed to lock default public screen!\n");
        return 20;
    }

    GetAttr(SA_ScreenbarTextPen, (Object *)scr, &val);
    printf("ScreenbarTextPen: 0x%08lX\n", val);

    UnlockPubScreen(NULL, scr);

    return 0;
}
