/* Prototypes for mandatory functions */
void cleanup( void );

/* Return values */
#define OK   0	/* All tests succeded */
#define FAIL 5  /* Some of the tests failed */

/* Testing macro */
#define TEST(x) if( !(x) )                                                           \
    	    	{                                                                  \
                    printf( "Test FAILED in %s, line %d.\n", __FILE__, __LINE__ ); \
		    cleanup();                                                     \
		    return FAIL;                                                   \
		}
