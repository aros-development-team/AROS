typedef struct _Hash Hash;

typedef void (*DeleteNodeProc) (char * key, void * data);
typedef void (*TraverseProc) (const char * key, void * data, void * userdata);

Hash * createhash (void);
void storedata (Hash *, const char * key, const void * data);
void * retrievedata (Hash *, const char * key);
void traversehash (Hash *, TraverseProc, void * userdata);
void deletehash (Hash *, DeleteNodeProc);
