#include <proto/exec.h>
#include <proto/muimaster.h>
#include <muimaster_private.h>

#include <zunepriv.h>
#include <string.h>

#ifdef TEST
#include <proto/dos.h>

#define AROS_LH1(t,n,a1,      x,y,z,w) t n(a1)
#define AROS_LH2(t,n,a1,a2,   x,y,z,w) t n(a1,a2)
#define AROS_LH3(t,n,a1,a2,a3,x,y,z,w) t n(a1,a2,a3)
#define AROS_LHA(t,n,r)             t n
#define AROS_LIBFUNC_INIT
#define AROS_LIBFUNC_EXIT
#define AROS_LIBBASE_EXT_DECL(x,y)
#endif

/*
 * General notes:
 * - structure is allocated with AllocMem() as we always free it here
 * - string is allocated with AllocVec(): if user calls g_string_free(s,0)
 *    he has to be able to free it with g_free() which maps to FreeVec()
 */

static GString *doAlloc(int len)
{
  GString *str;

  str = AllocMem(sizeof(GString), MEMF_ANY);
  if (str != NULL)
  {
    str->max = (len + 16) & ~15;
    str->str = AllocVec(str->max, MEMF_ANY);
    if (str->str == NULL)
    {
      FreeMem(str, sizeof(GString));
      str = NULL;
    }
  }

  return str;
}

static char *reAlloc(GString *str, int len)
{
    int max = (len + 16) & ~15;
    char *s = AllocVec(max, MEMF_ANY);
    if (s != NULL)
    {
      memcpy(s, str->str, str->len+1);
      FreeVec(str->str);
      str->max = max;
      str->str = s;
    }

    return s;
}

AROS_LH1(GString *, g_string_new,
    AROS_LHA(const char *, s, A0),
    struct Library *, MUIMasterBase, 35, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    int len = strlen(s);
    GString *str= doAlloc(len);

    if (str != NULL)
    {
      memcpy(str->str, s, len);
      str->str[len] = 0;
      str->len      = len;
    }

    return str;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(GString *, g_string_sized_new,
    AROS_LHA(int, len, D0),
    struct Library *, MUIMasterBase, 36, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    GString *str = doAlloc(len);

    if (str != NULL)
    {
      str->str[0] = 0;
      str->len    = 0;
    }

    return str;

    AROS_LIBFUNC_EXIT
}

/*
 * flag - true: free GString and character data
 *        false: free only GString
 */
AROS_LH2(void, g_string_free,
    AROS_LHA(GString *, str,  A0),
    AROS_LHA(int      , flag, D0),
    struct Library *, MUIMasterBase, 37, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    if (str != NULL)
    {
      if (flag)
        FreeVec(str->str);

      FreeMem(str, sizeof(GString));
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GString *, g_string_append,
    AROS_LHA(GString    *, str, A0),
    AROS_LHA(const char *, s,   A1),
    struct Library *, MUIMasterBase, 38, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    int len = strlen(s);

    if (str->len + len >= str->max)
    {
      if (!reAlloc(str, str->len + len))
        return NULL;
    }

    strcpy(str->str + str->len, s);
    str->len += len;
    return str;

    AROS_LIBFUNC_EXIT
}

AROS_LH2(GString *, g_string_append_c,
    AROS_LHA(GString *, str, A0),
    AROS_LHA(char     , c,   D0),
    struct Library *, MUIMasterBase, 39, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    char *s;

    if (str->len + 1 >= str->max)
    {
      if (!reAlloc(str, str->len + 1))
        return NULL;
    }

    s = str->str + str->len;
    *s++ = c;
    *s   = 0;
    str->len += 1;
    return str;

    AROS_LIBFUNC_EXIT
}

/*
 * Split 's' on char 'c', return NULL-terminated array.
 */
AROS_LH3(STRPTR *, g_strsplit,
    AROS_LHA(const char *, string,     A0),
    AROS_LHA(const char *, delimiter,  A1),
    AROS_LHA(      int   , max_tokens, D0),
    struct Library *, MUIMasterBase, 50, MUIMaster)
{
kprintf(">>> g_strsplit()\n");
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    STRPTR *str_array;
    const char *s;
          char *t;
    int i = 1, n = 0;

    /* count tokens */
kprintf(">>> g_strsplit(): count tokens\n");
    for (s = string; *s; )
    {
      if (*s == *delimiter)
      {
        /* one more token, except if start of string */
        if (s > string) i++;

        /* skip all delimiters */
        do { s++; } while (*s && (*s == *delimiter));

        /* one less token if string ends with delimiter */
        if (*s == 0) i--;

        if ((max_tokens > 0) && (i == max_tokens))
        {
          while (*s) { s++; n++; }
          break;
        }
      }
      else
      {
        /* one more real character */
        do { n++; s++; } while (*s && (*s != *delimiter));
      }
    }

kprintf(">>> g_strsplit(): AllocVec() i=%d n=%d\n", i, n);
    str_array = AllocVec((i+1) * sizeof(char *) + n + i, MEMF_ANY);
    if (str_array == NULL)
      return NULL;

    t = (char *)(str_array + i + 1);
    i = 0;

    /* extract tokens */
kprintf(">>> g_strsplit(): extract tokens\n");
    for (s = string; *s; )
    {
      if (*s == *delimiter)
      {
        /* skip all delimiters */
        do { s++; } while (*s && (*s == *delimiter));
      }
      else
      {
        str_array[i++] = t;

        if ((max_tokens > 0) && (i == max_tokens))
        {
          strcpy(t, s);
          break;
        }

        do { *t++ = *s++; } while (*s && (*s != *delimiter));
        *t++ = 0;
      }
    }

    str_array[i] = 0;
kprintf(">>> g_strsplit(): done\n");
    return str_array;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(void, g_strfreev,
    AROS_LHA(STRPTR *, str_array, A0),
    struct Library *, MUIMasterBase, 51, MUIMaster)
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *, MUIMasterBase)

    FreeVec(str_array);

    AROS_LIBFUNC_EXIT
}


#ifdef TEST
#include <stdio.h>

int main(int argc, char *argv[])
{
  GString *str = g_string_new(argv[0]);
  char **zz;
  int i;

  Printf("start: >%s<\n", str->str);

  for (i = 1; i < argc; i++)
  {
    g_string_append_c(str, '.');
    g_string_append(str, argv[i]);
    Printf("%5ld: >%s<\n", i, str->str);
  }

  zz = g_strsplit(str->str, argv[1], 10);
  if (zz && zz[0])
  {
    for (i = 0; zz[i]; i++)
      Printf("zz[%ld] = >%s<\n", i, zz[i]);
  }
  g_strfreev(zz);

  g_string_free(str, 1);

  return 0;
}

#endif

/*** EOF ***/
