/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <uproperties.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

struct UProperties {
    GTree        *collection;
    GStringChunk *strings;
    ZStringSet   *comments;
};

#define KEYVALUE 1
/* when a ! comment is read and must be kept */
#define COMMENT 2

/* The properties parser is ugly, but can you make it simpler ?
 * Anyway, excepted the parser, it seems correct.
 */

static void
write_escaped_string (char *s, FILE *outfile)
{
    char c;

    while ((c = *s++))
    {
	switch(c)
	{
	case ' ':
	case ':':
	case '!':
	case '#':
	case '=':
	    fprintf(outfile, "\\%c", c);
	    break;
	case '\n':
	    fputs("\\n", outfile);
	    break;
	case '\\':
	    fputs("\\\\", outfile);
	    break;
	case '\r':
	    fputs("\\r", outfile);
	    break;
	case '\t':
	    fputs("\\t", outfile);
	    break;
	default :
	    if (isprint(c))
		fputc(c, outfile);
	    else
		fprintf(outfile, "\\%03ho", c);
	}
    }
}

/* 
 * The value field doesn't need to as escaped as the key,
 * because we will read from the first separator up to end of line
 * without bothering with subsequent separators.
 * To get a value as escaped as a key, replace call to this function
 * with call to write_escaped_string
 */
static void
write_escaped_value (char *s, FILE *outfile)
{
    char c;

    while ((c = *s++))
    {
	switch(c)
	{
	case '\n':
	    fputs("\\n", outfile);
	    break;
	case '\\':
	    fputs("\\\\", outfile);
	    break;
	case '\r':
	    fputs("\\r", outfile);
	    break;
	case '\t':
	    fputs("\\t", outfile);
	    break;
	default :
	    if (isprint(c))
		fputc(c, outfile);
	    else
		fprintf(outfile, "\\%03ho", c);
	}
    }
}

static long
output_key_value (gpointer keyp,
		  gpointer valuep,
		  gpointer user_data)
{
    FILE *outfile = (FILE *)user_data;
    char *key = (char *)keyp;
    char *value = (char *)valuep;
    write_escaped_string(key, outfile);
    fputc('=', outfile);
    write_escaped_value(value, outfile);
    fputc('\n', outfile);
    return 0;
}

/*
 * parser : skip white spaces
 */
static void
skip_white_space(FILE *infile)
{
    int c;

    while ((c = fgetc(infile)) != EOF)
    {
	if (c == ' ' || c == '\t')
	    continue;
	else
	{
	    ungetc(c, infile);
	    break;
	}
    }
}

/*
 * parser : go to beginning of next line
 */
static void
next_line(FILE *infile)
{
    int c;
    int reached = 0;

    while ((c = fgetc(infile)) != EOF)
    {
	if (c != '\n' && c != '\r')
	{
	    if (reached)
	    {
		ungetc(c, infile);
		break;
	    }
	    else
		continue;
	}
	else
	{
	    reached = 1;
	}
    }
}

/*
 * parser : remove escape sequences from string
 */
static char *
cleanup_string (GString *mystring)
{
    gchar *s = mystring->str;
    GString *new_string = g_string_sized_new(mystring->len);
    int c;

    while ((c = *s++))
    {
	if (c != '\\')
	{
	    g_string_append_c(new_string, c);
	    continue;
	}

	switch((c = *s++))
	{
	case 0:
	    goto end;
	case ' ':
	case ':':
	case '!':
	case '#':
	case '=':
	case '\\':
	    g_string_append_c(new_string, c);
	    break;
	case 'n':
	    g_string_append_c(new_string, '\n');
	    break;
	case 'r':
	    g_string_append_c(new_string, '\r');
	    break;
	case 't':
	    g_string_append_c(new_string, '\t');
	    break;
	case '\n':
	    while ((c = *s++))
	    {
		if (!isspace(c))
		{
		    --s;
		    break;
		}   
	    }
	    if (c == 0)
		goto end;
	    break;
	default :
	    if (isdigit(c))
	    {
		static char num[4];
		gulong res;

		num[0] = c;
		num[1] = *s++;
		if (!num[1])
		    goto end;
		num[2] = *s++;
		if (!num[2])
		    goto end;
		num[3] = 0;
		res = strtoul(num, NULL, 8);
		if (res != ULONG_MAX)
		    g_string_append_c(new_string, res);
	    }
	    else
		g_string_append_c(new_string, c);
	}
    }
 end:
    g_string_free(mystring, 1);
    s = new_string->str;
    g_string_free(new_string, 0);
    return s;
}


/*
 * parser : break a line into key/value
 */
static int
parse_line (FILE *infile, char **kp, char **vp)
{
    char comment[1000];
    int c;
    GString *key;
    GString *value;

    g_return_val_if_fail(infile && kp && vp, 0);

    skip_white_space(infile);
    c = fgetc(infile);
    switch (c)
    {
    case EOF:
    case '\n':
    case '\r':
    case '#':
	return 0;
    case '!':
	ungetc(c, infile);
	*kp = g_strdup(fgets(comment, 1000, infile));
	if ((*kp)[strlen(*kp) - 1] == '\n')
	{
	    (*kp)[strlen(*kp) - 1] = 0;
	    ungetc('\n', infile);
	}
	return COMMENT;
    default:
	ungetc(c, infile);
    }

    key = g_string_sized_new(16);
    value = g_string_sized_new(16);

    /* read key */
    while ((c = fgetc(infile)) != EOF)
    {
	if (c == '\\')
	{
	    switch (c = fgetc(infile))
	    {
	    case EOF:
		break;
	    default:
		g_string_append_c(key, '\\');
		g_string_append_c(key, c);
		break;
	    }
	}
	else if (c == ':' || c == '=' || isspace(c))
	{
	    ungetc(c, infile);
	    break;
	}
	else
	{
	    g_string_append_c(key, c);
	}
    }

    switch (c)
    {
    case EOF:
	g_string_free(key, 1);
	g_string_free(value, 1);
	return 0;
    case '\n':
    case '\r':
	goto success_exit;
    case ':':
    case '=':
	fgetc(infile);
	break;
    }

    /* skip white space before value */
    skip_white_space(infile);
    c = fgetc(infile);
    if (c == '=' || c == ':')
	skip_white_space(infile);
    else
	ungetc(c, infile);

    /* read value */
    while ((c = fgetc(infile)) != EOF)
    {
	if (c == '\\')
	{
	    switch (c = fgetc(infile))
	    {
	    case EOF:
		break;
	    default:
		g_string_append_c(value, '\\');
		g_string_append_c(value, c);
		break;
	    }
	}
	else if (c == '\n' || c == '\r')
	{
	    ungetc(c, infile);
	    break;
	}
	else
	{
	    g_string_append_c(value, c);
	}
    }

 success_exit:
    *kp = cleanup_string(key);
    *vp = cleanup_string(value);
    return KEYVALUE;
}


/* creation */
/* comments - optional storage for comments */
UProperties *
uprop_new (ZStringSet *comments)
{
    UProperties *prop;
    
    prop = g_new0(UProperties, 1);
    prop->collection = g_tree_new((GCompareFunc)strcmp);
    prop->strings = g_string_chunk_new(64);
    prop->comments = comments;
/*  g_print("comments = %p\n", comments); */
    return prop;
}

/* destroy obj, and free all keys and values */
void 
uprop_destroy (UProperties *prop)
{
    g_tree_destroy(prop->collection);
    g_string_chunk_free(prop->strings);
}

static void
free_parsed_line (char *key, char *value)
{
    if (key)
	g_free(key);
    if (value)
	g_free(value);
}

/* file I/O */
int 
uprop_load (UProperties *prop, FILE *infile)
{
    char *key;
    char *value;

    g_return_val_if_fail(prop && infile, 0);

    while (!feof(infile))
    {
	key = NULL;
	value = NULL;
	switch (parse_line(infile, &key, &value))
	{
	    case KEYVALUE:
		uprop_set_property(prop, key, value);
		free_parsed_line(key, value);
		break;
	    case COMMENT:
		if (prop->comments)
		{
/*  		    g_print("adding %s\n", key); */
		    z_string_set_add(prop->comments, key);
		}
		free_parsed_line(key, value);
		break;
	}
	next_line(infile);
    }

    return 1;
}

int 
uprop_save (UProperties *prop, FILE *outfile, const char *header)
{
    g_return_val_if_fail(prop && outfile, 0);

    if (header)
	fprintf(outfile, "# %s\n\n", header);

    g_tree_traverse (prop->collection, output_key_value, G_IN_ORDER, outfile);
    if (prop->comments)
	z_string_set_dump(prop->comments, outfile);
    return 1;
}


/* access */
const char *
uprop_get_property (UProperties *prop, const char *key)
{
    return g_tree_lookup(prop->collection, key);
}

const char *
uprop_get_property_with_defaults (UProperties *prop, const char *key,
				  const char *def_val)
{
    const char *val = uprop_get_property(prop, key);
    if (!val)
	return def_val;
    return val;
}

/* note: both key and value are copied */
void 
uprop_set_property (UProperties *prop, const char *key, const char *value)
{
    g_tree_insert(prop->collection,
		  g_string_chunk_insert(prop->strings, key),
		  g_string_chunk_insert(prop->strings, value));
}


/* support (return ptr to static buffers, OK with uprop_set_property) */
static char *
ulong_to_string (gulong val)
{
    static char buf[60];

    g_snprintf(buf, 60, "%lu", val);
    return buf;
}

static char *
long_to_string (long val)
{
    static char buf[60];
    
    g_snprintf(buf, 60, "%ld", val);
    return buf;
}

static char *
double_to_string (double val)
{
    static char buf[60];

    g_snprintf(buf, 60, "%g", val);
    return buf;
}

static char *
float_to_string (float val)
{
    static char buf[60];
    
    g_snprintf(buf, 60, "%g", val);
    return buf;
}

/****/

void 
uprop_set_property_uint (UProperties *prop, const char *key, unsigned long value)
{
    uprop_set_property(prop, key, ulong_to_string(value));
}

void 
uprop_set_property_int (UProperties *prop, const char *key, long value)
{
    uprop_set_property(prop, key, long_to_string(value));
}

void 
uprop_set_property_double (UProperties *prop, const char *key, double value)
{
    uprop_set_property(prop, key, double_to_string(value));
}

void 
uprop_set_property_float (UProperties *prop, const char *key, float value)
{
    uprop_set_property(prop, key, float_to_string(value));
}


/****/


int
uprop_get_property_uint (UProperties *prop, const char *key, unsigned long *res)
{
    const char *s = uprop_get_property(prop, key);
    if (s)
    {
	gulong tmp = (gulong)strtoul(s, NULL, 10);
	*res = tmp;
	return 1;
    }
    return 0;
}

int
uprop_get_property_int (UProperties *prop, const char *key, long *res)
{
    const char *s = uprop_get_property(prop, key);
    if (s)
    {
	long tmp = strtol(s, NULL, 10);
	*res = tmp;
	return 1;
    }
    return 0;
}

int
uprop_get_property_double (UProperties *prop, const char *key, double *res)
{
    const char *s = uprop_get_property(prop, key);
    if (s)
    {
	*res = strtod(s, NULL);
	return 1;
    }
    return 0;
}

int
uprop_get_property_float (UProperties *prop, const char *key, float *res)
{
    const char *s = uprop_get_property(prop, key);
    if (s)
    {
	*res = strtod(s, NULL);
	return 1;
    }
    return 0;
}
