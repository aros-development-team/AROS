#ifndef DB_H
#define DB_H

#ifndef TOOLLIB_TOOLLIB_H
#   include <toollib/toollib.h>
#endif

typedef struct _DB DB;

extern void DB_Init PARAMS ((void));
extern void DB_Exit PARAMS ((void));

extern void DB_Free PARAMS ((DB * db));
extern int DB_Add PARAMS ((const char * dbname, const char * filename));
extern DB * DB_Find PARAMS ((const char * dbname));
extern void * DB_FindData PARAMS ((const char * dbname, const char * key));

#endif /* DB_H */
