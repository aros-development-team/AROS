#include <stringset.h>

struct ZStringSet {
    GSList *list;
    GStringChunk *chunk;
};


ZStringSet *
z_string_set_new(void)
{
    ZStringSet *set = g_malloc0(sizeof(ZStringSet));

    set->chunk = g_string_chunk_new(64);
    return set;
}

void 
z_string_set_destroy(ZStringSet *set)
{
    g_return_if_fail (set != NULL);

    g_slist_free (set->list);
    g_string_chunk_free(set->chunk);
}

void 
z_string_set_add(ZStringSet *set, const char *str)
{
    set->list = g_slist_append(set->list,
			       g_string_chunk_insert_const(set->chunk, str));
}

static void
write_string (gpointer data, gpointer udata)
{
    fprintf((FILE *)udata, "%s\n", (gchar *)data);
}

void 
z_string_set_dump(ZStringSet *set, FILE *out)
{
    g_slist_foreach(set->list, write_string, out);
}

