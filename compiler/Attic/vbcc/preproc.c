/* TODO:

   ---- HIGHEST PRIORITY ----

   * #ifndef

   * #if/#elif - Bearbeitung

   * leere argument-/tokenlists erlauben

   * Anzahl der Argumente checken (auch zuviele ergeben einen Error)

   * bei REDEFINING und allowredefinition=0 tokenlist genau ueberpruefen

   * Aufteilen in mehrere Sourcen main.c pplists.c etc.
   * Einige Variablen umbenennen (x,y,i,j,l,n etc =8)

   -----------------------------------------------------------------

   * Verbesserungen:
   * - Nach malloc()/vor free() ueberpruefen ob NULL-Pointer
   * - Statt per dlen per Ptr2EOL abfragen..

   * Fragen an Volker:
   * - _ alleine ein Identifier ? Ja

   ---- LOWEST PRIORITY ----

   * - Token loeschen, wenn nur noch TokenList benutzt wird.

   * - ParseIdent.. optimieren

   * - Rest optimieren =8)

   * - PreParsing umschreiben.. __LINE__ muss mit der echten
   *    Zeilennummer uebereinstimmen (die LFs drinlassen)
 */

/*
 * PreProcessor
 * $VER: VBPP V0.00 (19 Sep 1995)
 * (w) 1995 by Thorsten Schaaps
 */

#include <time.h>

#include "vbc.h"
#include "vbpp.h"

/*vb:   */
static char FILE_[]=__FILE__;

/*vb:   */
char pp_version[] = "vbpp V0.00 (w) 1995 by Thorsten Schaaps";

                                /* #define MAXIFNESTING 1024 *//*vb: ifnesting==incnesting */

FILE *in[MAXINCNESTING];        /*  Sourcefiles     */
int zn[MAXINCNESTING];          /*  Zeilennummern   */
char *filename[MAXINCNESTING];  /*  Filenamen   */
int incnesting;                 /*  aktuelle Verschachtelungstiefe  */
unsigned long linenr;           /*  Zeilennummer */

char *incpath[MAXINCPATHS] =
{"vinclude:"};                  /*  Includepfade    */

int incpathc = 1;               /*  Anzahl der Includepfade     */

char ppstring[MAXPPINPUT];

int cmtnesting = 0;             /*  aktuelle Kommentar-Versch.-Tiefe */

                                          /*int ifnesting; *//*vb: *//*  aktuelle IF-Tiefe */
/*vb: ifnesting==incnesting */
short ifstatus[MAXINCNESTING];  /* Array fuer Status-Verschachtelung */
int if_cnt;                     /* Zaehler fuer #if's waehrend do_output=0 */
int abs_if_cnt;                 /* zaehlt auch waehrend do_output */

int do_output = 1;              /* Flag zur Erzeugung eines Outputs */

struct strnode *strlist;

struct mnode *mlist;

int did_expand;

/* Temporaere Debugging-Routinen */
void PrintSN(struct strnode *tl)
{
    switch (tl->type) {
    case NORMAL:
        printf(" Normal........:");
        break;
    case PP_IDENT:
        printf(" Identifier....:");
        break;
    case ARGUMENT:
        printf(" Argument Nr.%2d:", tl->number);
        break;
    case NUMBER:
        printf(" Number........:");
        break;
    case PP_STR:
        printf(" String........:");
        break;
    case SPACE:
        printf(" Space.........:");
        break;
    case SPECIAL:
        printf(" Special..Nr.%2d:", tl->flags);
        break;
    default:
        printf(" unknown..Nr.%2d:", tl->type);
        break;
    }
    printf(" %s\n", tl->str);
}
void PrintTL(struct strnode *tl)
{

#ifdef bla

    if (tl) {
        printf("TokenList:\n");
        while (tl) {
            PrintSN(tl);
            tl = tl->next;
        }
    }
#endif

}


/* ******* Listen-Funktionen ******* */

/* AddMakroNode - Fuegt eine Node an den Anfang einer Liste ein */
void AddMakroNode(struct mnode **list, struct mnode *node)
{

    if (DEBUG & 32)
        puts("AddMakroNode");

    if (node) {
        node->prev = NULL;
        node->next = *list;
        *list = node;
        if (node->next)
            node->next->prev = node;
    }
}


/* AddStrNode - Fuegt eine Node in die Liste ein */
void AddStrNode(struct strnode **list, struct strnode *node, char *str)
{

    if (DEBUG & 32)
        puts("AddStrNode");


    if (node || str) {
        if (!node)
            node = (struct strnode *) malloc(sizeof(struct strnode));
        /* HIER: Rueckgabewert wegen Fehler ! */
        if (!node) {
            error(196);
        } else {
            node->prev = NULL;
            node->next = *list;
            if (str)
                node->str = str;
            *list = node;
            if (node->next)
                node->next->prev = node;
        }
    }
}


/* AddStrNodeBehind - Fuegt eine Node ans Ende der Liste ein */
void AddStrNodeBehind(struct strnode **list, struct strnode *node, char *str)
{
    struct strnode *listnode;

    if (DEBUG & 32)
        puts("AddStrNodeBehind");


    if (node || str) {
        if (!node)
            node = (struct strnode *) malloc(sizeof(struct strnode));
        if (!node) {
            error(196);
        } else {
            if (!*list) {
                node->prev = NULL;
                node->next = NULL;
                *list = node;
            } else {
                listnode = *list;
                while (listnode->next) {
                    listnode = listnode->next;
                }
                node->prev = listnode;
                node->next = NULL;
                listnode->next = node;
            }
            if (str)
                node->str = str;
        }
    }
}


/* RemMakroNode - Entfernt eine Node aus der Liste ohne sie zu loeschen */
void RemMakroNode(struct mnode **list, struct mnode *node)
{

    if (DEBUG & 32)
        puts("RemMakroNode");


    if (node->prev) {
        if (node->next)
            node->next->prev = node->prev;
        node->prev->next = node->next;
    } else {
        if (node->next)
            node->next->prev = NULL;
        *list = node->next;
    }
    node->next = node->prev = NULL;
}

/* InsertMakroNode - Setzt eine Node hinter einer anderen ein */
void InsertMakroNode(struct mnode **list, struct mnode *node, struct mnode *behind)
{

    if (DEBUG & 32)
        puts("InsertMakroNode");


    if (behind) {
        node->prev = behind;
        node->next = behind->next;
        behind->next = node;
        if (node->next)
            node->next->prev = node;
    } else
        AddMakroNode(list, node);
}

/* RemStrNode - Entfernt eine Node aus der Liste ohne sie zu loeschen */
void RemStrNode(struct strnode **list, struct strnode *node)
{

    if (DEBUG & 32)
        puts("RemStrNode");


    if (node->prev) {
        if (node->next)
            node->next->prev = node->prev;
        node->prev->next = node->next;
    } else {
        if (node->next)
            node->next->prev = NULL;
        *list = node->next;
    }
    node->next = node->prev = NULL;
}


/* FindMakroNode - sucht den passenden Eintrag in der Liste
 *                 len=0 - der gesamte String muss uebereinstimmen
 *                 len>0 - die ersten len Zeichen muessen stimmen
 */
struct mnode *FindMakroNode(struct mnode *list, char *str, int len)
{

    if (DEBUG & 32)
        puts("FindMakroNode");

    while (list) {
        if (len) {
            if ((strlen(list->name) == len) && (!strncmp(list->name, str, len)))
                return (list);
        } else {
            if (!strcmp(list->name, str))
                return (list);
        }
        list = list->next;
    }
    return (0);
}


/* DelMakroNode - gibt den gesamten Speicher einer Node frei, ist list#NULL
 *                wird die Node vorher aus der Liste entfernt
 */
void DelMakroNode(struct mnode **list, struct mnode *node)
{

    if (DEBUG & 32)
        puts("DelMakroNode");


    if (node) {
        if (list && *list)
            RemMakroNode(list, node);
        if (node->name)
            free(node->name);
        if (node->args)
            free(node->args);
        if (node->token)
            free(node->token);
        if (node->tokenlist)
            DelStrList(&node->tokenlist);
        free(node);
    }
}


/* DelStrNode - gibt den gesamten Speicher einer Node frei, ist list#NULL
 *              wird die Node vorher aus der Liste entfernt
 */
void DelStrNode(struct strnode **list, struct strnode *node)
{

    if (DEBUG & 32)
        puts("DelStrNode");


    if (node) {
        if (list && *list)
            RemStrNode(list, node);
        if (node->str)
            free(node->str);
        free(node);
    }
}

void MergeStrNodes(struct strnode *prev, struct strnode *next)
{
    /* next will now be deleted and prev->str will be */
    /* the joined strings from prev&next */
    char *newstr;
/*vb:   */
    char c, *s, *d;

    if (DEBUG & 32)
        puts("MergeStrNodes");


    newstr = (char *) malloc(prev->len + next->len + 1);
/*vb: string-Routinen ersetzt (hoffentlich schneller)
   strcpy(newstr,prev->str);
   strcat(newstr,next->str);
 */
    d = newstr;
    s = prev->str;
    do {
        c = *s++;
        *d++ = c;
    } while (c);
    d--;
    s = next->str;
    do {
        c = *s++;
        *d++ = c;
    } while (c);

    if (prev->str)
        free(prev->str);        /*vb: wenn prev->str==0 knallts doch schon oben? */
    prev->str = newstr;
    prev->len += next->len;
    DelStrNode(NULL, next);
}

/* DelMakroList - loescht eine gesamte Liste */
void DelMakroList(struct mnode **list)
{

    if (DEBUG & 32)
        puts("DelMakroList");


    while (*list)
        DelMakroNode(list, *list);
    /*  *list=NULL; *//*vb: *list==0 */
}


/* DelStrList - loescht eine gesamte Liste samt der Nodes/Strings */
void DelStrList(struct strnode **list)
{

    if (DEBUG & 32)
        puts("DelStrList");


    while (*list)
        DelStrNode(list, *list);
    /*  *list=NULL; *//*vb: *list==0 */
}

/* AllocSpace - erzeugt eine StrNode vom Typ Space */
struct strnode *AllocSpace()
{
    struct strnode *newnode;
    char *newstr;


    if (DEBUG & 32)
        puts("AllocSpace");


    newstr = (char *) malloc(2);
    if (newstr) {
        newstr[0] = ' ';
        newstr[1] = 0;
        newnode = (struct strnode *) malloc(sizeof(struct strnode));
        if (newnode) {
            newnode->str = newstr;
            newnode->type = SPACE;
            newnode->len = 1;
            newnode->next = newnode->prev = NULL;
            newnode->flags = newnode->number = 0;
            return (newnode);
        }
    }
    return (NULL);
}


/* CloneStrList - erstellt eine exakte Kopie der Liste oder eines Ausschnittes */
/*                wenn listend#NULL */
struct strnode *CloneStrList(struct strnode *list, struct strnode *listend)
{
    struct strnode *prevnode = NULL, *newnode, *newlist = NULL;
    char *newstr;

    if (DEBUG & 32)
        puts("CloneStrList");


    while (list) {
        newnode = (struct strnode *) malloc(sizeof(struct strnode));
        if (!newnode) {
            DelStrList(&newlist);
            return (NULL);
        }
        newstr = (char *) malloc(list->len + 1);
        if (!newstr) {
            free(newnode);
            DelStrList(&newlist);
            return (NULL);
        }
        strcpy(newstr, list->str);
        newnode->str = newstr;
        newnode->len = list->len;
        newnode->flags = list->flags;
        newnode->type = list->type;
        newnode->number = list->number;
        newnode->next = NULL;
        newnode->prev = prevnode;

        if (prevnode)
            prevnode->next = newnode;
        else
            newlist = newnode;
        prevnode = newnode;

        if (listend && list == listend)
            list = NULL;
        else
            list = list->next;
    }
    return (newlist);
}


/* DoMakroFunction - erstellt eine StrList entsprechend des Function-Makros */
struct strnode *DoMakroFunction(struct mnode *makro)
{
    struct strnode *result = NULL;
    char *newstr = NULL, *timestr = NULL;
    int len, type;
    time_t timevalue;

    if (DEBUG & 32)
        puts("DoMakroFunction");


    if (makro->flags & FUNCTION) {
        switch (makro->funcnum) {
        case FUNCLINE:
            newstr = (char *) malloc(11);
            if (!newstr)
                return (NULL);
            sprintf(newstr, "%lu", linenr);
            len = strlen(newstr);
            type = NUMBER;
            break;
        case FUNCFILE:
            len = strlen(filename[incnesting]);
            newstr = (char *) malloc(len + 1 + 2);
            if (!newstr)
                return (NULL);
            *newstr = '\"';
            strcpy((newstr + 1), filename[incnesting]);
            strcat(newstr, "\"");
            type = PP_STR;
            break;
        case FUNCDATE:
            type = PP_STR;
            newstr = (char *) malloc(14);
            if (!newstr)
                return (NULL);
            timevalue = time(0);
            timestr = ctime(&timevalue);
            newstr[0] = '\"';
            strncpy(newstr + 1, timestr + 4, 7);        /* copy 'Mmm dd ' */
            strcpy(newstr + 8, timestr + 20);   /* copy 'yyyy' */
            newstr[12] = '\"';
            newstr[13] = 0;
            len = 13;
            break;
        case FUNCTIME:
            type = PP_STR;
            newstr = (char *) malloc(11);
            if (!newstr)
                return (NULL);
            timevalue = time(0);
            timestr = ctime(&timevalue);
            newstr[0] = '\"';
            strncpy(newstr + 1, timestr + 11, 8);       /* copy 'hh:mm:ss' */
            newstr[9] = '\"';
            newstr[10] = 0;
            len = 10;
            break;
        default:
            return (NULL);
            break;
        }
        result = (struct strnode *) malloc(sizeof(struct strnode));
        if (!result) {
            if (newstr)
                free(newstr);
            return (NULL);
        }
        result->prev = result->next = NULL;
        result->str = newstr;
        result->len = len;
        result->number = result->flags = 0;
        result->type = type;
    }
    return (result);
}


/* *******  String-Funktionen ****** */

/* Str2List - parsed einen String in eine StrList */
struct strnode *Str2List(char *str)
{
    struct strnode *newlist = NULL, *newnode = NULL;
    char *temp, *string;
    int length, type, flags;

    if (DEBUG & 32)
        puts("Str2List");

    while (*str) {
        flags = 0;
/*vb: casts eingefuegt   */
        if (isspace((unsigned char) *str)) {    /* Spaces parsen */
            temp = str + 1;
            length = 1;
            while (*temp && isspace((unsigned char) *temp)) {
                temp++;
                length++;
            }
            type = SPACE;
        } else if (*str == '\"') {      /* String parsen */
            temp = str + 1;
            length = 2;
            while (*temp && *temp != '\"')
                if (*temp == '\\') {
                    temp += 2;
                    length += 2;
                } else {
                    temp++;
                    length++;
                }
            type = PP_STR;
        } else if (*str == '\'') {      /* String parsen */
            temp = str + 1;
            length = 2;
            while (*temp && *temp != '\'')
                if (*temp == '\\') {
                    temp += 2;
                    length += 2;
                } else {
                    temp++;
                    length++;
                }
            type = PP_STR;
        } else if (isdigit((unsigned char) *str) || (*str == '.' && isdigit((unsigned char) *(str + 1)))) {
            /* Zahlen parsen */
            temp = str;
            length = 0;
            while (*temp && (isalnum((unsigned char) *temp) || *temp == '_')) {
                temp++;
                length++;
            }
            if (*temp == '.') {
                temp++;
                length++;
                if (isdigit((unsigned char) *temp) || *temp == 'e' || *temp == 'E')
                    while (*temp && (isalnum((unsigned char) *temp) || *temp == '_')) {
                        temp++;
                        length++;
                    }
            }
            if (*(temp - 1) == 'e' || *(temp - 1) == 'E')
                if (isdigit((unsigned char) *temp) || *temp == '+' || *temp == '-') {
                    temp++;
                    length++;
                    while (*temp && (isalnum((unsigned char) *temp) || *temp == '_')) {
                        temp++;
                        length++;
                    }
                }
            type = NUMBER;
        } else if (isalpha((unsigned char) *str) || *str == '_') {      /* moegl. Identifier parsen */
            temp = str + 1;
            length = 1;
            while (*temp && (isalnum((unsigned char) *temp) || *temp == '_')) {
                temp++;
                length++;
            }
            type = PP_IDENT;
        } else if (*str) {      /* alles andere als einzelne Zeichen */
            length = 1;
            type = NORMAL;
        }
        string = (char *) malloc(length + 1);
        if (!string) {
            error(196);
            DelStrList(&newlist);
            return (NULL);
        }
        strncpy(string, str, length);
        string[length] = 0;
        str += length;

        newnode = (struct strnode *) malloc(sizeof(struct strnode));
        if (!newnode) {
            error(196);
            if (string)
                free(string);
            return (NULL);
        }
        newnode->str = string;
        newnode->len = length;
        newnode->flags = flags;
        newnode->type = type;

        AddStrNodeBehind(&newlist, newnode, NULL);
    }
    return (newlist);
}

/* List2Str - schreibt eine Liste als String zurueck */
int List2Str(struct strnode *list, char *str, int maxchars)
{
    int len = 0;
    char c, *p;

    if (DEBUG & 32)
        puts("List2Str");


    *str = 0;
    if (list) {
        while (list) {
            if ((len + (list->len)) > (maxchars - 1)) {
                error(177);
                return (0);
            } else {
                /*vb:   */
                p = list->str;
                do {
                    c = *p++;
                    *str++ = c;
                } while (c);
                str--;
/*        strcat(str,list->str); */
                len += list->len;
            }
            list = list->next;
        }
        return (len + 1);
    } else {
        *str = 0;
        return (1);
    }
}

/* ListLen - ermittelt die Laenge einer Str-Liste (in Zeichen) */
int ListLen(struct strnode *list)
{
    int len = 0;
    while (list) {
        len += list->len;
        list = list->next;
    }
    return (len);
}

/* ListStrLen - ermittelt die Laenge einer Str-List als String   */
/*              d.h. mit " am Anfang und Ende, sowie einem \ vor */
/*              jedem \ und "                                    */
int ListStrLen(struct strnode *list)
{
    int len = 2;
    while (list) {
        if (list->type == SPACE)
            len++;
        else {
            len += list->len;
            if (list->type == PP_STR && list->str[0] == '\"')
                len += 2;
            else if (list->type == NORMAL && list->str[0] == '\\')
                len++;
        }
        list = list->next;
    }
    return (len);
}

/* CopyList2StrStr - kopiert eine Str-List als String um         */
/*              d.h. mit " am Anfang und Ende, sowie einem \ vor */
/*              jedem \ und "                                    */
CopyList2StrStr(struct strnode * list, char *str)
{
    int len;

    if (DEBUG & 32)
        puts("CopyList2StrStr");


    str[0] = 0;
    if (list) {
        strcpy(str, "\"");
        len = 1;
        while (list) {
            if (list->type == SPACE) {
                len++;
                strcat(str, " ");
            } else {
                len += list->len;
                if (list->type == PP_STR && list->str[0] == '\"') {
                    strcat(str, "\\");
                    strncat(str, list->str, list->len - 1);
                    strcat(str, "\\\"");
                } else if (list->type == NORMAL && list->str[0] == '\\') {
                    strcat(str, "\\");
                    strcat(str, list->str);
                } else
                    strcat(str, list->str);
            }
            list = list->next;
        }
        strcat(str, "\"");
        len++;
    }
    return (len);
}

/* NextToSpecial - checked, ob die StrNode neben einem # oder ## steht */
int NextToSpecial(struct strnode *node)
{
    struct strnode *search;

    if (DEBUG & 32)
        puts("NextToSpecial");


    if (node->prev) {
        search = node->prev;
        while (search && search->type == SPACE)
            search = search->prev;
        if (search && search->type == SPECIAL)
            return (search->flags);
    }
    if (node->next) {
        search = node->next;
        while (search && search->type == SPACE)
            search = search->next;
        /* Hinter der Node ist ein # (ToString) ohne Bedeutung */
        if (search && search->type == SPECIAL
            && search->flags == KILLSPACES)
            return (KILLSPACES);
    }
    return (NONE);
}

/* FindBracket - sucht das passende Gegenstueck zu der (, auf die Node zeigt */
/*               dabei werden verschachtelte Klammern beachtet               */
struct strnode *FindBracket(struct strnode *node)
{

    if (DEBUG & 32)
        puts("FindBracket");


    if (node && node->next && node->type == NORMAL && node->str[0] == '(') {
        do {
            node = node->next;
            if (node && node->type == NORMAL) {
                if (node->str[0] == ')')
                    return (node);
                if (node->str[0] == '(')
                    node = FindBracket(node);
            }
        } while (node);
        return (NULL);
    } else
        return (NULL);
}

/* CloneArg - gibt eine Liste zurueck, die das n. Argument des Makros enthaelt */
struct strnode *CloneArg(struct strnode *list, int n, int *error)
{
    int argnum = 0;
    struct strnode *start;

    if (error)
        *error = OK;

    if (list) {
        if (DEBUG & 32)
            printf("CloneArg\n");
        if (list->type == PP_IDENT)
            list = list->next;
        while (list && list->type == SPACE)
            list = list->next;

        if (list && list->type == NORMAL && list->str[0] == '(') {
            list = list->next;  /* Skip ( */

            if (DEBUG & 32)
                printf("- IDENT skipped, ( found\n");

            while (list && (argnum < n)) {
                if (list->type == NORMAL) {
                    if (list->str[0] == ')')
                        list = NULL;
                    if (list && list->str[0] == '(')
                        list = (FindBracket(list))->next;
                    if (list && list->str[0] == ',')
                        argnum++;
                }
                if (list)
                    list = list->next;
            }

            while (list && list->type == SPACE)
                list = list->next;

            if (list) {
                if (list->type == NORMAL && (list->str[0] == ')' || list->str[0] == ',')) {     /* HIER evtl. ein Space erzeugen */
                    return (NULL);
                }
                start = list;
            } else {
                if (error)
                    *error = ARG_EXPECTED;
                return (NULL);
            }

            if (DEBUG & 32)
                printf("- Okay, Arg-Start found: %s\n", start->str);

            while (list && (argnum == n)) {
                if (list->type == NORMAL) {
                    switch (list->str[0]) {
                    case ')':
                    case ',':
                        argnum++;
                        list = list->prev;
                        break;
                    case '(':
                        list = FindBracket(list);
                    default:
                        if (list)
                            list = list->next;
                        break;
                    }
                } else if (list)
                    list = list->next;
            }

            while (list && list->type == SPACE)
                list = list->prev;

            if (DEBUG & 32)
                printf("- Okay, Arg-End found: %s\n", list->str);
            return (CloneStrList(start, list));

        } else {
            if (error)
                *error = ARG_EXPECTED;
            return (NULL);
        }
    } else
        return (NULL);
}

/* ExpandArgMakro - ersetzt ein Makro mit Argumenten */
int ExpandArgMakro(struct mnode *makro, struct strnode **list,
                   struct strnode **pos)
{
    struct strnode *clone, *temp, *temp1, *arg, *new;
    struct strnode *prev, *next;
    struct mnode *prevmakro;
    char *newstr;
    int spec, len, x;

    clone = CloneStrList(makro->tokenlist, NULL);
    if (!clone)
        return (OUT_OF_MEM);

    if (DEBUG & 32)
        printf("ExpandArgMakro - Clone build.\n");

    temp = clone;
    while (temp) {
        if (temp->type == ARGUMENT) {
            arg = CloneArg(*pos, temp->number, &x);
            if (!arg) {
                DelStrList(&clone);
                return (x);
            }
            if (DEBUG & 32) {
                printf("ARGUMENT:\n");
                PrintTL(arg);
                printf("Argument found - ");
            }
            if (spec = NextToSpecial(temp)) {
                /* nicht expandieren, nur einsetzen */
                if (spec == TOSTRING) {

                    if (DEBUG & 32)
                        printf("next to #\n");
                    /* als String einsetzen */
                    new = (struct strnode *) malloc(sizeof(struct strnode));
                    if (!new) {
                        DelStrList(&clone);
                        DelStrList(&arg);
                        return (OUT_OF_MEM);
                    }
                    len = ListStrLen(arg);
                    newstr = (char *) malloc(len + 1);
                    if (!newstr) {
                        DelStrList(&clone);
                        DelStrList(&arg);
                        if (new)
                            free(new);
                        return (OUT_OF_MEM);
                    }
                    CopyList2StrStr(arg, newstr);
                    DelStrList(&arg);
                    new->len = len + 2;
                    new->flags = new->number = 0;
                    new->type = PP_STR;
                    new->str = newstr;

                    prev = temp->prev;
                    next = temp->next;
                    DelStrNode(NULL, temp);     /* Argument */

                    while (prev && prev->type == SPACE) {
                        temp = prev->prev;
                        DelStrNode(&clone, prev);
                        prev = temp;
                    }

                    if (prev && prev->type == SPECIAL && prev->flags == TOSTRING) {
                        temp = prev->prev;
                        DelStrNode(&clone, prev);
                        prev = temp;
                    } else
                        ierror(0);

                    new->prev = prev;
                    if (prev)
                        prev->next = new;
                    else
                        clone = new;
                    new->next = next;
                    if (next)
                        next->prev = new;
                    temp = new;
                } else {
                    /* Einfach nur einsetzen */
                    if (DEBUG & 32)
                        printf("next to ##\n");
                    prev = temp->prev;
                    next = temp->next;
                    DelStrNode(NULL, temp);
                    arg->prev = prev;
                    if (prev)
                        prev->next = arg;
                    else
                        clone = arg;
                    while (arg->next)
                        arg = arg->next;
                    arg->next = next;
                    if (next)
                        next->prev = arg;
                    temp = arg;
                }
            } else {
                /* expandieren und einsetzen */
                if (DEBUG & 32)
                    printf("normal arg\n");
                if (x = ExpandList(&arg)) {
                    DelStrList(&clone);
                    DelStrList(&arg);
                    return (x);
                }
                if (DEBUG & 32) {
                    printf("expanded arg:\n");
                    PrintTL(arg);
                }
                prev = temp->prev;
                next = temp->next;
                DelStrNode(NULL, temp);
                arg->prev = prev;
                if (prev)
                    prev->next = arg;
                else
                    clone = arg;
                while (arg->next)
                    arg = arg->next;
                arg->next = next;
                if (next)
                    next->prev = arg;
                temp = arg;
            }                   /* of N2Special */
        }
        if (temp)
            temp = temp->next;
    }

    if (DEBUG & 32)
        printf("Arguments expanded.\n");

    temp = clone;
    while (temp) {
        if (temp->type == SPECIAL && temp->flags == KILLSPACES) {
            prev = temp->prev;
            next = temp->next;
            if ((prev->type == PP_IDENT || prev->type == NUMBER)) {
                switch (next->type) {
                case NUMBER:    /* merge -> ident */
                case PP_IDENT:
                    RemStrNode(&clone, next);
                    MergeStrNodes(prev, next);
                    next = temp->next;
                    /* next will now be deleted and prev->str will be */
                    /* the joined strings from prev&next */
                default:        /* no merge / del ## */
                    DelStrNode(&clone, temp);
                    temp = next;
                    break;
                }               /* of switch */
            } else {
                DelStrNode(&clone, temp);
                temp = next;
            }
        }
        if (temp)
            temp = temp->next;
    }

    /* 'makro' aus mlist ausklinken */
    prevmakro = makro->prev;
    RemMakroNode(&mlist, makro);

    /* alles expandieren */
    if (!(x = ExpandList(&clone))) {
        /* akt. StrNode durch clone-Liste ersetzen */

        if (DEBUG & 32) {
            printf("Complete Makro expanded\n");
            PrintTL(clone);
        }
        /* clone anstelle von pos in list einsetzen */
        prev = (*pos)->prev;
        next = (*pos)->next;

        DelStrNode(NULL, *pos);

#ifdef bla
        while (next && next->type == SPACE) {
            temp = next->next;
            DelStrNode(NULL, next);     /* SPACE zwischen Makro und Args loeschen */
            next = temp;
        }
#endif
        while (next && next->type == SPACE)
            next = next->next;
        /* SPACE hinter Makro ueberspringen */

        if (next && next->type == NORMAL && next->str[0] == '(') {
            temp = next;
            next = FindBracket(temp);
            if (next)
                next = next->next;
            do {
                temp1 = temp->next;
                DelStrNode(NULL, temp);         /* ARG-List hinter Makro loeschen */
                temp = temp1;
            } while (temp != next);
        } else {
            ierror(0);
        }


        /* vor clone ein SPACE einsetzen, wenn nicht PREV==SPACE */
/*vb:   hier !prev|| wegen Enforcerhit eingesetzt; waere prev&& besser? */
        if (!prev || prev->type != SPACE) {
            if (temp = AllocSpace()) {
                temp->next = clone;
                if (clone)
                    clone->prev = temp;
                clone = temp;
            }
        }
        clone->prev = prev;
        if (prev)
            prev->next = clone;
        else
            *list = clone;

        while (clone->next)
            clone = clone->next;
        /* nach clone ein SPACE einsetzen, wenn nicht NEXT==SPACE */
/*vb:   hier !next|| wegen Enforcerhit eingesetzt; waere next&& besser? */
        if (!next || next->type != SPACE) {
            if (temp = AllocSpace()) {
                temp->prev = clone;
                if (clone)
                    clone->next = temp;
                clone = temp;
            }
        }
        *pos = clone;
        clone->next = next;
        if (next)
            next->prev = clone;

        /* 'makro' wieder einsetzen */
        InsertMakroNode(&mlist, makro, prevmakro);
        return (OK);
    } else {
        InsertMakroNode(&mlist, makro, prevmakro);
        return (x);
    }

}

/* ExpandList - ersetzt alle Makros in der Liste */
int ExpandList(struct strnode **list)
{
    struct mnode *found, *before;
    struct strnode *clone, *beforestr, *afterstr, *listtemp, *temp2,
    *temp3;
    int result = OK;

    if (DEBUG & 32)
        puts("ExpandList");


    listtemp = *list;
    while (listtemp) {

        if (listtemp->type == PP_IDENT) {

            found = FindMakroNode(mlist, listtemp->str, 0);
            if (found) {
/*vb: merken, ob mind. ein Makro expandiert wurde   */
                did_expand = 1;
                if (found->flags & PARAMETER) {
                    /* Makro mit Argument(en) */
                    if (DEBUG & 32)
                        printf("Makro with args\n");

                    temp2 = listtemp->next;
                    while (temp2 && temp2->type == SPACE)
                        temp2 = temp2->next;

                    if (temp2 && temp2->type == NORMAL && temp2->str[0] == '(')
                        if (result = ExpandArgMakro(found, list, &listtemp))
                            return (result);

                } else {

                    /* Makro ohne Argument */

                    /* ExpandNormMakro - expandiert ein Makro ohne Argumente */
                    /* Parameter: s.o. */

                    if (found->flags & FUNCTION) {
                        clone = DoMakroFunction(found);
                    } else {
                        clone = CloneStrList(found->tokenlist, NULL);
                    }
                    if (!clone)
                        return (OUT_OF_MEM);
                    /* akt. MakroNode ausklinken um rekursive Exp. zu verhindern */
                    before = found->prev;
                    RemMakroNode(&mlist, found);
                    if (!(result = ExpandList(&clone))) {
                        /* akt. StrNode durch clone-Liste ersetzen */
                        beforestr = listtemp->prev;
                        afterstr = listtemp->next;
                        DelStrNode(NULL, listtemp);
                        listtemp = afterstr;
                        clone->prev = beforestr;
                        if (beforestr)
                            beforestr->next = clone;
                        else
                            *list = clone;
                        while (clone->next)
                            clone = clone->next;
                        clone->next = afterstr;
                        if (afterstr)
                            afterstr->prev = clone;
                        /* akt. Makronode wieder einsetzen */
                        InsertMakroNode(&mlist, found, before);
                    } else {
                        /* akt. Makronode wieder einsetzen */
                        InsertMakroNode(&mlist, found, before);
                        return (result);
                    }
                }
            }
        }
        if (listtemp)
            listtemp = listtemp->next;
    }

    return (OK);
}


/* ParseIdentifier - parsed den Input-Define-String und haengt das Makro */
/*                   in die Liste */

struct mnode *ParseIdentifier(char *str)
{
    int x, numargs = 0, flags = 0, len = 0;
    char *name = NULL, *args = NULL, *token = NULL, *temp, *temp2, *argtemp;
    struct mnode *newmakro, *found;
    struct strnode *tokenlist = NULL, *templist, *arglist = NULL, *templist2;
    struct strnode *next, *prev;

    if (DEBUG & 32)
        puts("ParseIdentifier");

/*vb: casts eingefuegt  */
    while (isspace((unsigned char) *str)) {
        str++;
    }
    if (!(isalpha((unsigned char) *str) || *str == '_')) {
        error(178);
        return (0);
    }
    temp = str;
    while (isalnum((unsigned char) *temp) || (*temp == '_')) {
        temp++;
        len++;
    }

    /* auf schon vorhandene (evtl. FESTE) Definition suchen */
    found = FindMakroNode(mlist, str, len);
    if (found && (found->flags & NOREDEF)) {
        error(179);
        return (0);
    }
/*vb:   */
    if (1) {
        /* Okay, ist egal, wie das alte Makro aussah */

/*vb:   */
        if (found && !(c_flags[15] & USEDFLAG)) {
            error(197);
        }
        temp = name = (char *) malloc(len + 1);         /* Speicher fuer Name-String belegen */
        if (!name) {
            error(196);
            return (0);
        }
        for (x = 0; x < len; x++) {
            *temp++ = *str++;
        }                       /* Namen kopieren */
        *temp = 0;

        if (*str == '(') {
            flags |= PARAMETER;
            str++;              /* jump over '(' */
            temp = str;
            len = 0;
/*vb: casts eingefuegt  */
            while (*temp && (*temp != ')'))
                if (!(isspace((unsigned char) *temp))) {
                    len++;
                    temp++;
                } else {
                    temp++;
                }
            if (*temp == 0) {
                error(180);
                if (name)
                    free(name);
                return (0);
            }
            args = temp = (char *) malloc(len + 1);
            if (!args) {
                error(196);
                if (name)
                    free(name);
                return (0);
            }
            for (x = 0; x < len;)
                if (!(isspace((unsigned char) *str))) {
                    x++;
                    *temp++ = *str++;
                } else {
                    str++;
                }
            *temp = 0;
            str++;              /* jump over ')' */

            /* Argumentliste parsen und auf Fehler pruefen */
            temp = args;
            while (*temp && (isalpha((unsigned char) *temp) || *temp == '_')) {
                temp++;
                while (*temp && (isalnum((unsigned char) *temp) || *temp == '_')) {
                    temp++;
                }
                if (!(*temp) || *temp == ',') {
                    numargs++;
                    if ((*temp == ',') && (isalpha((unsigned char) *(temp + 1)) || *(temp + 1) == '_'))
                        temp++;
                }
            }
            if (*temp) {
                if (args)
                    free(args);
                if (name)
                    free(name);
                if (*temp == ',') {
                    error(181);
                } else {
                    error(182);
                }
                return (0);
            }
        }
        while (isspace((unsigned char) *str))
            str++;              /* Skip Spaces */
        /* HIER: pruefen, ob tokenlist erstellt werden konnte (leere tokenlist->ok) */
        tokenlist = Str2List(str);
        if (DEBUG & 32)
            printf(" - build TokenList\n");
        templist = tokenlist;
        while (templist) {
            if ((templist->type == NORMAL) && (!strcmp(templist->str, "#"))) {

                if ((templist->next) && (templist->next->type == NORMAL)
                    && (!strcmp(templist->next->str, "#"))) {
                    /* ## KILLSPACES */
                    if ((templist->prev) && (templist->next->next)) {
                        templist->type = SPECIAL;
                        templist->flags = KILLSPACES;
                        DelStrNode(&tokenlist, templist->next);
                    } else {
                        error(183);
                        if (name)
                            free(name);
                        if (args)
                            free(args);
                        if (tokenlist)
                            DelStrList(&tokenlist);
                        return (0);
                    }
                }
            }
            templist = templist->next;
        }

        /* Token parsen/kopieren */
        if ((flags & PARAMETER)) {      /* Argumente parsen */
            arglist = NULL;
            temp = temp2 = args;
            while (*temp) {
                len = 0;
                while (*temp && *temp != ',') {
                    temp++;
                    len++;
                }
                if (*temp == ',')
                    temp++;
                argtemp = (char *) malloc(len + 1);
                if (!argtemp) {
                    error(196);
                    if (name)
                        free(name);
                    if (args)
                        free(args);
                    if (arglist)
                        DelStrList(&arglist);
                    if (tokenlist)
                        DelStrList(&tokenlist);
                    return (0);
                }
                strncpy(argtemp, temp2, len);
                temp2 = temp;
                *(argtemp + len) = 0;
                AddStrNodeBehind(&arglist, NULL, argtemp);
            }
            templist = tokenlist;
            while (templist) {
                if (templist->type == PP_IDENT) {
                    x = 0;
                    templist2 = arglist;
                    while (templist2) {
                        if (!strcmp(templist->str, templist2->str)) {
                            templist->type = ARGUMENT;
                            templist->number = x;
                        }
                        x++;
                        templist2 = templist2->next;
                    }
                }
                templist = templist->next;
            }
            DelStrList(&arglist);
            if (DEBUG & 32)
                printf(" - Arguments found.\n");
            /* #-Specials korrigieren, nachdem die Argumente erkannt wurden */
            templist = tokenlist;
            while (templist) {
                if ((templist->type == NORMAL) && (!strcmp(templist->str, "#"))) {
                    if ((templist->next) && (templist->next->type == ARGUMENT)) {
                        templist->type = SPECIAL;
                        templist->flags = TOSTRING;
                    } else {
                        error(184);
                        if (name)
                            free(name);
                        if (args)
                            free(args);
                        if (arglist)
                            DelStrList(&arglist);
                        if (tokenlist)
                            DelStrList(&tokenlist);
                        return (0);
                    }
                }
                templist = templist->next;
            }
            if (DEBUG & 32)
                printf(" - Special-# corrected.\n");
        } else {                /* Token kopieren */
            temp = str;
            len = 0;
            while ((*temp) && (*temp != '\n')) {
                temp++;
                len++;
            }
            temp = token = (char *) malloc(len + 1);
            if (!token) {
                error(196);
                if (name)
                    free(name);
                if (args)
                    free(args);
                if (tokenlist)
                    DelStrList(&tokenlist);
                return (0);
            }
            for (x = 0; x < len; x++) {
                *temp++ = *str++;
            }
            *temp = 0;
        }


        templist = tokenlist;
        while (templist) {
            if (templist->type == SPECIAL && templist->flags == KILLSPACES) {
                next = templist->next;
                prev = templist->prev;

                /* Kill Spaces before/after ## */
                while (prev && prev->type == SPACE) {
                    templist2 = prev->prev;
                    DelStrNode(&tokenlist, prev);
                    prev = templist2;
                }
                while (next && next->type == SPACE) {
                    templist2 = next->next;
                    DelStrNode(&tokenlist, next);
                    next = templist2;
                }
                if ((!next) || (!prev))
                    ierror(0);

                if (next->type != ARGUMENT && prev->type != ARGUMENT) {
                    if ((prev->type == PP_IDENT || prev->type == NUMBER)) {
                        switch (next->type) {
                        case NUMBER:    /* merge -> ident */
                        case PP_IDENT:
                            RemStrNode(&tokenlist, next);
                            MergeStrNodes(prev, next);
                            next = templist->next;
                            /* next will now be deleted and prev->str will be */
                            /* the joined strings from prev&next */
                        default:        /* no merge / del ## */
                            DelStrNode(&tokenlist, templist);
                            templist = next;
                            break;
                        }       /* of switch */
                    } else {
                        DelStrNode(&tokenlist, templist);
                        templist = next;
                    }
                }
            }
            if (templist)
                templist = templist->next;
        }


/* erst HIER bei allowredefinition=0 abfragen ?? */

        if (newmakro = (struct mnode *) malloc(sizeof(struct mnode))) {
            newmakro->name = name;
            newmakro->args = args;
            newmakro->token = token;
            newmakro->tokenlist = tokenlist;
            newmakro->flags = flags;
            newmakro->funcnum = 0;
            AddMakroNode(&mlist, newmakro);
        } else {
            error(196);
            if (name)
                free(name);
            if (token)
                free(token);
            if (args)
                free(args);
            if (tokenlist)
                DelStrList(&tokenlist);
        }
        return (newmakro);

    } else {
/*vb:   das ist irgendwie Unsinn hier, glaube ich   */
        ierror(0);
        /* HIER: ueberpruefen, ob Macrodefinitionen uebereinstimmen, sonst Error */
        if (*temp == '(') {
            if (found->flags & PARAMETER) {
                temp++;         /* skip ( */



            } else {
                error(185);
                return (NULL);
            }
        }
        /* HIER nur noch tokenlisten vergleichen */

    }
    if (DEBUG & 32)
        printf("ParseIdent falls of\n");
}


/* PreParse - 3-Zeichen-Folgen ersetzen, \+CR Zeilen anhaengen, usw.. */

int PreParse()
{
    char *src, *dest, *dest2;
    int slen, dlen = 0;
    int cat = 0;                /*vb: um zu merken, ob Zeilen mit \ verbunden werden */
    dest2 = string;

    if (DEBUG & 32)
        puts("PreParse");


    if (!fgets(ppstring, MAXPPINPUT, in[incnesting])) {
        if (cmtnesting)
            error(186);
/*vb:   */
/*      if(ifstatus[incnesting]) error(186); */
/*      ifnesting--; */
        if (DEBUG & 32)
            printf("-- end of file  ifnesting-1:%d\n\n", incnesting);
        if (incnesting) {
            fclose(in[incnesting]);
            incnesting--;
            return (pp_nextline());
        } else
            return (0);
    } else if (DEBUG & 32)
        printf("gets1:%s\n", ppstring);         /*vb: */

    if ((strlen(ppstring) == (MAXPPINPUT - 1)) && (ppstring[MAXPPINPUT - 2] != '\n')) {
        error(177);
        return (0);
    }
    zn[incnesting]++;
    linenr = zn[incnesting];

    do {
        /* Zeilen einlesen, bis kein Kommentar mehr */

        /* Zeilen einlesen bis kein \ mehr am Ende */
        do {
            /* Source-String an Dest-String anhaengen und ??x-Folgen ersetzen */
/*vb: das killt sonst das angehaengte wieder   */
            if (cat == 0) {
                dest = src = ppstring;
                slen = 0;
            }
            do {
                if (*src != '\n') {

                    if ((*src == '?') && (*(src + 1) == '?') && !(c_flags[16] & USEDFLAG)) {
                        switch (*(src + 2)) {
                        case '=':
                            *dest++ = '#';
                            src += 2;
                            break;
                        case '/':
                            *dest++ = '\\';
                            src += 2;
                            break;
                        case '\'':
                            *dest++ = '^';
                            src += 2;
                            break;
                        case '(':
                            *dest++ = '[';
                            src += 2;
                            break;
                        case ')':
                            *dest++ = ']';
                            src += 2;
                            break;
                        case '!':
                            *dest++ = '|';
                            src += 2;
                            break;
                        case '<':
                            *dest++ = '{';
                            src += 2;
                            break;
                        case '>':
                            *dest++ = '}';
                            src += 2;
                            break;
                        case '-':
                            *dest++ = '~';
                            src += 2;
                            break;
                        default:
                            *dest++ = *src;
                            break;
                        }       /* of switch */
                    } else {
                        *dest++ = *src;
                    }

                    if (slen < MAXPPINPUT) {
                        slen++;
                    } else {
                        error(177);
                        return (0);
                    }
                }
            } while (*src++);

            src = dest;
            /* dest->lastchar+2/NULL+1 (dest-1)-> 0 (dest-2)->lastchar */
            if (*(dest - 2) == '\\') {
                /*vb: sollte das slen statt dlen sein?  */
                if (fgets(src, MAXPPINPUT - dlen, in[incnesting])) {
                    if (DEBUG & 32)
                        printf("gets2:%s\n", src);      /*vb:  */
/* Hier auf LF+0 abfragen */
                    dest -= 2;
                    zn[incnesting]++;
                    cat = 1;    /*vb: merken, dass Zeilen verbunden wurden */
                }
            }
        } while (*dest == '\\');

        src = ppstring;
        while (*src) {
            /* ' Strings ueberlesen */
            if ((*src == '\'') && (!cmtnesting)) {
                if (dlen < MAXINPUT) {
                    *dest2++ = *src++;
                    dlen++;
                } else {
                    error(177);
                    return (0);
                }
                while (*src && *src != '\'') {
                    if (*src == '\\') {
                        *dest2++ = *src++;
                        dlen++;
                    };
                    if (dlen < MAXINPUT) {
                        *dest2++ = *src++;
                        dlen++;
                    } else {
                        error(177);
                        return (0);
                    }
                }
                if (dlen < MAXINPUT) {
                    *dest2++ = *src++;
                    dlen++;
                } else {
                    error(177);
                    return (0);
                }
            }
            /* " Strings ueberlesen */
            if ((*src == '\"') && (!cmtnesting)) {
                *dest2++ = *src++;
                dlen++;
                while (*src && *src != '\"') {
                    if (*src == '\\') {
                        *dest2++ = *src++;
                        dlen++;
                    };
                    if (dlen < MAXINPUT) {
                        *dest2++ = *src++;
                        dlen++;
                    } else {
                        error(177);
                        return (0);
                    }
                }
                if (dlen < MAXINPUT) {
                    *dest2++ = *src++;
                    dlen++;
                } else {
                    error(177);
                    return (0);
                }
            }
            /* Kommentare weglassen */
            if ((*src == '/') && (*(src + 1) == '*')) {
                src += 2;
                if (dlen < MAXINPUT) {
                    *dest2++ = ' ';
                    dlen++;
                } else {
                    error(177);
                    return (0);
                }
                if ((cmtnesting && (c_flags[13] & USEDFLAG)) || (!cmtnesting))
                    cmtnesting++;
                else
                    error(198);
            } else {
                if ((*src == '*') && (*(src + 1) == '/') && cmtnesting) {
                    src += 2;
                    cmtnesting--;
                }
            }

            /* C++-Comment weglassen */
            if ((!cmtnesting) && (*src == '/') && (*(src + 1) == '/') && (c_flags[14] & USEDFLAG))
                *src = 0;

            if (!cmtnesting) {
                if (*src) {
                    *dest2++ = *src++;
                }
                if (dlen < MAXINPUT) {
                    dlen++;
                } else {
                    error(177);
                    return (0);
                }
            } else {
                if (*src)
                    src++;
            }
        }

        if (cmtnesting) {
            if (!fgets(ppstring, MAXPPINPUT, in[incnesting])) {
                if (cmtnesting)
                    error(186);
                if (incnesting) {
                    fclose(in[incnesting]);
                    incnesting--;
                    return (pp_nextline());
                } else
                    return (0);
            } else if (DEBUG & 32)
                printf("gets2:%s\n", ppstring);         /*vb:   */

            if ((strlen(ppstring) == (MAXPPINPUT - 1)) && (ppstring[MAXPPINPUT - 2] != '\n')) {
                error(177);
                return (0);
            }
            zn[incnesting]++;
        }
    } while (cmtnesting);

/*vb: das ueberschreibt den Speicher */
#if 0
    *dest2-- = 0;
    while (isspace(*dest2))
        *dest2-- = 0;           /* Spaces killen */
#endif

/*vb: hoffe, das ist besser */
    *dest2 = 0;
    while (dest2 > string && isspace((unsigned char) *--dest2))
        *dest2 = 0;


    return (1);
}

/* **************** PreProcessor ***************** */

int pp_init(void)
{
    char *macroname;
    struct mnode *macronode;

    if (!(c_flags[6] & USEDFLAG))
        printf("%s\n", pp_version);

    incnesting = /*ifnesting= */ -1;    /*vb:  */
    cmtnesting = if_cnt = abs_if_cnt = 0;
    mlist = NULL;
    strlist = NULL;

    macroname = (char *) malloc(9);
    if (!macroname)
        return (0);
    strcpy(macroname, "__LINE__");
    macronode = (struct mnode *) malloc(sizeof(struct mnode));
    if (!macronode) {
        if (macroname)
            free(macroname);
        return (0);
    }
    macronode->name = macroname;
    macronode->args = macronode->token = NULL;
    macronode->tokenlist = NULL;
    macronode->flags = FUNCTION | NODELETE | NOREDEF;
    macronode->funcnum = FUNCLINE;
    AddMakroNode(&mlist, macronode);

    macroname = (char *) malloc(9);
    if (!macroname)
        return (0);
    strcpy(macroname, "__FILE__");
    macronode = (struct mnode *) malloc(sizeof(struct mnode));
    if (!macronode) {
        if (macroname)
            free(macroname);
        DelMakroList(&mlist);
        return (0);
    }
    macronode->name = macroname;
    macronode->args = macronode->token = NULL;
    macronode->tokenlist = NULL;
    macronode->flags = FUNCTION | NODELETE | NOREDEF;
    macronode->funcnum = FUNCFILE;
    AddMakroNode(&mlist, macronode);

    macroname = (char *) malloc(9);
    if (!macroname)
        return (0);
    strcpy(macroname, "__DATE__");
    macronode = (struct mnode *) malloc(sizeof(struct mnode));
    if (!macronode) {
        if (macroname)
            free(macroname);
        DelMakroList(&mlist);
        return (0);
    }
    macronode->name = macroname;
    macronode->args = macronode->token = NULL;
    macronode->tokenlist = NULL;
    macronode->flags = FUNCTION | NODELETE | NOREDEF;
    macronode->funcnum = FUNCDATE;
    AddMakroNode(&mlist, macronode);

    macroname = (char *) malloc(9);
    if (!macroname)
        return (0);
    strcpy(macroname, "__TIME__");
    macronode = (struct mnode *) malloc(sizeof(struct mnode));
    if (!macronode) {
        if (macroname)
            free(macroname);
        DelMakroList(&mlist);
        return (0);
    }
    macronode->name = macroname;
    macronode->args = macronode->token = NULL;
    macronode->tokenlist = NULL;
    macronode->flags = FUNCTION | NODELETE | NOREDEF;
    macronode->funcnum = FUNCTIME;
    AddMakroNode(&mlist, macronode);

    macroname = (char *) malloc(9);
    if (!macroname)
        return (0);
    strcpy(macroname, "__STDC__");
    macronode = (struct mnode *) malloc(sizeof(struct mnode));
    if (!macronode) {
        if (macroname)
            free(macroname);
        DelMakroList(&mlist);
        return (0);
    }
    macronode->name = macroname;
    macronode->args = NULL;
    macroname = (char *) malloc(2);
    if (!macroname) {
        if (macronode->name)
            free(macronode->name);
        if (macronode)
            free(macronode);
        DelMakroList(&mlist);
        return (0);
    }
    strcpy(macroname, "1");
    macronode->token = macroname;
    macronode->tokenlist = Str2List(macroname);
    if (!macronode->tokenlist) {
        if (macronode->name)
            free(macronode->name);
        if (macronode->token)
            free(macronode->token);
        if (macronode)
            free(macronode);
        DelMakroList(&mlist);
        return (0);
    }
    macronode->flags = NODELETE;
    macronode->funcnum = 0;
    AddMakroNode(&mlist, macronode);

    return (1);
}


void pp_free(void)
{
    int i;

    if (DEBUG & 32)
        puts("pp_free");

/*vb: Schleifenindex korrekt gesetzt    */
    for (i = incnesting; i >= 0; i--)
        if (in[i])
            fclose(in[i]);
    DelMakroList(&mlist);
    DelStrList(&strlist);
}

int pp_include(char *f)
{
    if (DEBUG & 32)
        printf("trying to include %s\n", f);    /*vb:  */

    if (incnesting >= MAXINCNESTING - 1) {
        error(187);
        return (0);
    }
/*vb:   */
/*    if(ifnesting>=MAXIFNESTING-1)   {error("Too many nested #ifs or #includes",0);return(0);} */
    incnesting++;
    /*    ifnesting++; *//*vb:   */
    ifstatus[incnesting] = 0;   /*vb:   */

    if (DEBUG & 32)
        printf("-- include: ifnesting:%d ifstatus:0 \n\n", incnesting);

    in[incnesting] = fopen(f, "r");
    if (!in[incnesting]) {
        incnesting--;
        return (0);
    }
    filename[incnesting] = f;
    zn[incnesting] = linenr = 0;
    return (1);
}



/* ********************** Main-Function ******************* */
int pp_nextline(void)
{
    char *src, *dest, *temp;
    int complete_len, len, i, result;
    struct mnode *makro;
    struct strnode *linelist, *inclinelist;

    if (DEBUG & 32)
        puts("pp_nextline");

    dest = string;
    dest[0] = 0;

    /* String vorbereiten, 3ZF ersetzen, \-Zeilen anhaengen, Kommentare loeschen */
    if (!PreParse())
        return (0);

    /* Ueberpruefung auf PreProcessor-Commands */
    src = string;
/*vb: casts eingefuegt  */
    while (isspace((unsigned char) *src)) {
        src++;
    }                           /* SkipSpaces */

    if (*src == '#') {          /* #-Direktive gefunden */
        src++;                  /* # ueberlesen */

        while (isspace((unsigned char) *src)) {
            src++;
        }                       /* SkipSpaces */

/*vb:   Direktiven, die nichts mit #if etc. zu tun haben, nach hinten   */


/* IFDEF */
        if (!strncmp(src, "ifdef", 5)) {
            src += 5;
            abs_if_cnt++;       /*vb: */
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }
                /* identifier suchen */
                if (do_output) {
/*vb:   */
/*            if(ifnesting>=MAXIFNESTING-1) {error("Too many nested #if's",0); return(0);} */
/*            ifnesting++; */
                    temp = src;
                    len = 0;
                    while ((isalnum((unsigned char) *temp)) || (*temp == '_')) {
                        temp++;
                        len++;
                    }
                    makro = FindMakroNode(mlist, src, len);
                    if (DEBUG & 32)
                        printf("-- #ifdef found, ");
                    if (makro) {
/*vb:   */
                        ifstatus[incnesting] = 1;       /* Bedingung == TRUE */
                        if (DEBUG & 32)
                            printf(" condition '%s' found     (TRUE)\n", makro->name);
                    } else {
/*vb:   */
                        ifstatus[incnesting] = 2;       /* Bedingung == FALSE */
                        do_output = 0;
                        if (DEBUG & 32)
                            printf(" condition not found (FALSE)\n\n");
                    }
                } else {
                    if_cnt++;
                    if (DEBUG & 32)
                        printf("-- #ifdef found and if_cnt increased\n\n");
                }
                string[0] = 0;  /*vb: yo   */
                return (1);
            }
        }                       /* EO IFDEF */
        /* IFNDEF */
        if (!strncmp(src, "ifndef", 6)) {
            src += 6;
            abs_if_cnt++;       /*vb: */
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }

                /* identifier suchen */
                if (do_output) {
/*vb:   */
/*            if(ifnesting>=MAXIFNESTING-1) {error("Too many nested #if's",0); return(0);} */
/*            ifnesting++; */
                    temp = src;
                    len = 0;
                    while ((isalnum((unsigned char) *temp)) || (*temp == '_')) {
                        temp++;
                        len++;
                    }
                    if (DEBUG & 32)
                        printf("-- FindMakroNode: len=%d temp=%p\n", len, (void*)temp);
                    makro = FindMakroNode(mlist, src, len);
                    if (DEBUG & 32)
                        printf("-- #ifndef found, ");
                    if (!makro) {
/*vb:   */
                        ifstatus[incnesting] = 1;       /* Bedingung == TRUE (Makro not existing) */
                        if (DEBUG & 32)
                            printf(" condition not found  (TRUE)\n\n");
                    } else {
/*vb:   */
                        ifstatus[incnesting] = 2;       /* Bedingung == FALSE */
                        do_output = 0;
                        if (DEBUG & 32)
                            printf(" condition '%s'     found (FALSE)\n\n", makro->name);
                    }
                    /* HIER: string[0]=0; */
                } else {
                    if_cnt++;
                    if (DEBUG & 32)
                        printf("-- #ifndef found and if_cnt increased\n\n");
                }
                string[0] = 0;  /*vb:   */
                return (1);
            }
        }                       /* EO IFNDEF */
        /* IF */
        if (!strncmp(src, "if", 2)) {
            src += 2;
            abs_if_cnt++;       /*vb:   */
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }
                /* Bedingung auswerten */



                printf("****** WARNING ******* #if is not yet implemented\n");


                string[0] = 0;  /*vb:   */
                return (1);
            }
        }                       /* EO IF */
        /* ELIF */
        if (!strncmp(src, "elif", 4)) {
            src += 4;
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }
                /* Bedingung auswerten */





                printf("****** WARNING ******* #elif is not yet implemented\n");




                string[0] = 0;  /*vb:   */
                return (1);
            }
        }                       /* EO ELIF */
        /* ELSE */
        if (!strncmp(src, "else", 4)) {
            src += 4;
            if (isspace((unsigned char) *src) || *src == 0) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }

                if (!if_cnt) {
                    switch (ifstatus[incnesting]) {     /*vb:   */
                    case 0:
                        error(188);
                        return (0);
                        break;
                    case 1:
                        do_output = 0;
                        /* HIER: auf #endif suchen SearchENDIF(); */
                        /* HIER: ifnesting--; do_output=1; */
                        break;
                    case 2:
                        ifstatus[incnesting] = 3;       /*vb:   */
                        do_output = 1;
                        break;
                    case 3:
                        error(189);
                        return (0);
                        break;
                    }
                }
                string[0] = 0;  /*vb:    */
                return (1);
            }
        }                       /* EO ELSE */
        /* ENDIF */
        if (!strncmp(src, "endif", 5)) {
            src += 5;
            abs_if_cnt--;       /*vb:   */
            if (isspace((unsigned char) *src) || *src == 0) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }
                /* HIER: Auf Zeilenende testen */

                if (DEBUG & 32)
                    printf("-- #endif found, ");

                if (if_cnt) {
                    if_cnt--;
                    if (DEBUG & 32)
                        printf("if_cnt decreased (to %d)\n", if_cnt);
                } else {
                    if (DEBUG & 32)
                        printf("ifnesting: %d  ifstatus: %d\n", incnesting, ifstatus[incnesting]);
                    if (abs_if_cnt < 0 /*(incnesting==-1)||ifstatus[incnesting]==0 */ ) {       /*vb:   */
                        error(190);
                        return (0);
                    }
                    /*            ifnesting--; *//*vb:   */
                    /* HIER: evtl. do_output entsprechend ifstatus[] setzen */
                    /*vb: natuerlich    */
                    do_output = 1;
                    ifstatus[incnesting] = abs_if_cnt > 0;      /*vb: 1 oder 0   */
                }
                string[0] = 0;  /*vb: ja */
                return (1);
            }
        }                       /* EO ENDIF */
        /*vb: andere Direktiven gegebenenfalls ueberspringen    */
        if (!do_output) {
            if (DEBUG & 32)
                printf("do_output==0 => skipping: %s\n", src);
            string[0] = 0;
            return (1);
        }
/* DEFINE */
        if ( /*(do_output)&& */ (!strncmp(src, "define", 6))) {         /*vb:   */
            if (DEBUG & 32)
                printf("#define found\n");
            src += 6;
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }
                if (ParseIdentifier(src)) {
                    string[0] = 0;
                    return (1);
                } else {
                    if (DEBUG & 32)
                        printf("ParseIdent returned 0\n");
                    return (0);
                }
            }
        }                       /* EO DEFINE */
        /* UNDEF */
        if ( /*(do_output)&& */ (!strncmp(src, "undef", 5))) {  /*vb:   */
            src += 5;
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }
                temp = src;
                len = 0;
                while ((isalnum((unsigned char) *temp)) || (*temp == '_')) {
                    temp++;
                    len++;
                }
                makro = FindMakroNode(mlist, src, len);
                if (makro->flags & NODELETE) {
                    error(199);
                } else {
                    DelMakroNode(&mlist, makro);
                }
                while (isspace((unsigned char) *temp)) {
                    temp++;
                }
                if (*temp != 0 && *temp != '\n') {
                    error(200);
                }
                string[0] = 0;
                return (1);
            }
        }                       /* EO UNDEF */
        /* INCLUDE */
        if ( /*(do_output)&& */ (!strncmp(src, "include", 7))) {        /*vb:   */
            src += 7;
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }
                if (*src != '<' && *src != '\"') {
                    /* Versuchen den Rest zu expandieren */
                    inclinelist = Str2List(src);
                    if (!ExpandList(&inclinelist)) {
                        if (List2Str(inclinelist, src, MAXINPUT - (src - string))) {
                            /* HIER exit ? */
                        }
                    } else {
                        error(191);     /* HIER exit ? */
                    }
                }
/*vb: geaendert, so dass #include "..." auch noch im Standardpfad sucht */
                if (DEBUG & 32)
                    printf("includename=%s\n", src);
                if (*src == '<' || *src == '\"') {
                    char *m = src, c;
                    if (*src == '<')
                        c = '>';
                    else
                        c = '\"';
                    if (*src == '\"') {
                        /* im aktuellen Verzeichnis suchen und includen */
                        src++;
                        temp = src;
                        len = 0;
                        while (*temp != '\"') {
                            temp++;
                            len++;
                        }
                        temp++;
                        while (isspace((unsigned char) *temp))
                            temp++;
                        if (*temp) {
                            error(200);
                        }
                        temp = (char *) malloc(len + 1);
                        if (temp) {
                            strncpy(temp, src, len);
                            *(temp + len) = 0;
                            if (pp_include(temp)) {
                                AddStrNode(&strlist, NULL, temp);
                                string[0] = 0;
                                return (1);
                            } else {
                                if (temp)
                                    free(temp);
/*vb:
   error("pp: cannot open file to include",0);
   return(0); */
                            }
                        } else {
                            error(196);
                        }       /* HIER: Exit? */
                    }
                    /* in den Standard-Verzeichnissen suchen und includen */
                    src = m;
                    src++;
                    temp = src;
                    len = 0;
                    while (*temp != c && *temp != 0) {
                        temp++;
                        len++;
                    }           /*vb: sicherer */
                    temp++;
                    while (isspace((unsigned char) *temp))
                        temp++;
                    if (*temp) {
                        error(200);
                    }
                    temp = NULL;
                    for (i = 0; i < incpathc; i++) {
                        complete_len = strlen(incpath[i]) + len;
                        temp = (char *) malloc(complete_len + 1);
                        if (temp) {
                            strcpy(temp, incpath[i]);
                            strncat(temp, src, len);
                            *(temp + complete_len) = 0;
                            if (pp_include(temp)) {
                                AddStrNode(&strlist, NULL, temp);
                                if (DEBUG & 32)
                                    printf("include <%s> found\n", temp);
                                string[0] = 0;
                                return (1);
                            } else {
                                if (temp)
                                    free(temp);
                            }
                        } else {
                            error(196);
                        }       /* HIER: Exit ? */
                    }           /* of FOR i */
                    error(191);
                    return (0);
                } else {
                    error(192);
                    return (0);
                }
            } else {
                error(193);
                return (0);
            }
        }                       /* EO INCLUDE */
        /* LINE */
        if (!strncmp(src, "line", 4)) {
            src += 4;
            if (isspace((unsigned char) *src)) {
                return (1);     /* Ignorieren und an Compiler weiterreichen */
            }
        }                       /* EO LINE */
        /* ERROR */
        if (!strncmp(src, "error", 5)) {
            src += 5;
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }

                return (1);
            }
        }                       /* EO ERROR */
        /* PRAGMA */
        if (!strncmp(src, "pragma", 6)) {
            src += 6;
            if (isspace((unsigned char) *src)) {
                while (isspace((unsigned char) *src)) {
                    src++;
                }

                return (1);
            }
        }                       /* EO PRAGMA */
        /* Unknown */
        /*    if(do_output){ */
        /*vb:   */
        error(193);
        return (0);
        /*    } *//*vb:   */
    } else if (do_output) {
        /* Normale Anweisung. Komplette Zeile expandieren */

/*vb:   */

        linelist = Str2List(string);

        if (DEBUG & 32)
            PrintTL(linelist);

/*vb: */
        did_expand = 0;

        if (result = ExpandList(&linelist)) {
            switch (result) {
            case OUT_OF_MEM:
                error(196);
                break;
            case NUM_OF_ARGS:
                error(194);
                break;
            case ARG_EXPECTED:
                error(195);
                break;
            default:
                ierror(0);
                break;
            }
            DelStrList(&linelist);
            return (0);
        }
        if (DEBUG & 32)
            PrintTL(linelist);

/*vb: List2Str nur aufrufen, falls etwas expandiert wurde   */
        if (did_expand && !List2Str(linelist, string, MAXINPUT)) {
            DelStrList(&linelist);
            return (0);
        }
        DelStrList(&linelist);

/*vb:   */
    } else
        string[0] = 0;
    return (1);
}
