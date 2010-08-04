#include <windows.h>

void Host_Alert(char *text)
{
    MessageBox(NULL, text, "AROS Guru meditation", MB_ICONERROR|MB_SETFOREGROUND);
}
