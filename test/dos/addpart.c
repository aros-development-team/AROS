// test for AddPart(). Test cases are from Guru Book.

#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

int test(STRPTR p1, CONST_STRPTR p2, CONST_STRPTR expected)
{
    BOOL res;
    char buffer[100];
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, p1);

    res = AddPart(buffer, p2, sizeof(buffer));

    if (res == DOSFALSE)
    {
        printf("AddPart() returned DOSFALSE for %s %s\n", p1, p2);
        return 1;
    }

    if (strcmp(buffer, expected) != 0)
    {
        printf("AddPart() created string %s; expected was %s\n", buffer, expected);
        return 1;        
    }
    return 0;
}

int main(void)
{
    int error = 0;

    error += test("foo",         "bar",          "foo/bar");
    error += test("foo/",        "bar",          "foo/bar");
    error += test("foo/baz",     "bar",          "foo/baz/bar");
    error += test("foo",         "bar/baz",      "foo/bar/baz");
    error += test("foo:",        "bar",          "foo:bar");
    error += test("foo:",        "bar:",         "bar:");
    error += test("foo:baz",     ":bar",         "foo:bar");
    error += test("foo",         "/bar",         "foo//bar");
    error += test("foo/",        "/bar",         "foo//bar");

    return error ? RETURN_ERROR : RETURN_OK;
}
