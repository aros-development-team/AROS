#include <stdio.h>
#include <toollib/error.h>
#include <toollib/hash.h>
#include "db.h"
#include "var.h"

struct _DB
{
    Node   node;
    Hash * db;
};

static List dbs;

static String CheckDBCB (const char ** args, int dummy, CBD data);
static String QueryDBCB (const char ** args, int dummy, CBD data);

static String CheckDBCB (const char ** args, int dummy, CBD data)
{
    if (!args[0] || !args[1])
    {
	PushError ("$checkdb(): Expecting two args");
	return NULL;
    }

    return VS_New ((DB_FindData (args[0], args[1])) ? "1" : "0");
}

static String QueryDBCB (const char ** args, int dummy, CBD dummy2)
{
    char * data;

    if (!args[0] || !args[1])
    {
	PushError ("$querydb(): Expecting two args");
	return NULL;
    }

    data = DB_FindData (args[0], args[1]);

    if (!data)
    {
	PushError ("$querydb(%s,%s): Nothing found", args[0], args[1]);
	return NULL;
    }

    return VS_New (data);
}

void
DB_Init (void)
{
    NewList (&dbs);

    Func_Add ("checkdb", (CB) CheckDBCB, NULL);
    Func_Add ("querydb", (CB) QueryDBCB, NULL);
}

void
DB_Exit (void)
{
    DB * db, * next;

    ForeachNodeSafe (&dbs, db, next)
    {
	Remove (db);
	DB_Free (db);
    }
}

int
DB_Add (const char * dbname, const char * filename)
{
    DB	 * db = new (DB);
    FILE * fh;
    char   key[256], data[256];

    fh = fopen (filename, "r");

    if (!fh)
    {
	xfree (db);
	PushStdError ("Can't open %s for reading", filename);
	return 0;
    }

    db->node.name = xstrdup (dbname);
    db->db = Hash_New ();

    AddTail (&dbs, db);

    while (fgets (key, sizeof (key), fh))
    {
	if (!fgets (data, sizeof (data), fh))
	{
	    DB_Free (db);
	    PushStdError ("Error reading data for key %s in %s", key, filename);
	    return 0;
	}

	key[strlen (key) - 1] = 0;
	data[strlen (data) - 1] = 0;

#if 0
    printf ("Adding \"%s\":\"%s\"\n", key, data);
#endif

	Hash_StoreNC (db->db, xstrdup (key), xstrdup (data));
    }

    return 1;
}

static int
DB_FreeEntryCB (char * key, char * data, void * dummy)
{
    xfree (key);
    xfree (data);
    return 1;
}

void
DB_Free (DB * db)
{
    Hash_Delete (db->db, (CB) DB_FreeEntryCB, NULL);
    xfree (db->node.name);
    xfree (db);
}

DB *
DB_Find (const char * name)
{
    return (DB *) FindNodeNC (&dbs, name);
}

void *
DB_FindData (const char * name, const char * key)
{
    DB	 * db = DB_Find (name);
    void * data;

    if (!db)
    {
#if 0
	printf ("No DB %s\n", name);
#endif
	return NULL;
    }

    data = Hash_FindNC (db->db, key);

#if 0
    printf ("Key \"%s\" -> %p\n", key, data);
#endif

    return data;
}
