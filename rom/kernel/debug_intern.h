typedef struct
{
	struct MinList  m_symbols; /* Symbols list		   */
	char *		m_str;	   /* String table		   */
	unsigned int	m_segcnt;  /* Count of segments		   */
	char		m_name[1]; /* Module name, variable length */
} module_t;

struct segment
{
    struct MinNode s_node;	/* For linking into the list 	 */
    void *	   s_lowest;	/* Start address	     	 */
    void *	   s_highest;	/* End address		     	 */
    BPTR	   s_seg;	/* DOS segment pointer		 */
    module_t *	   s_mod;	/* Module descriptor	     	 */
    char *	   s_name;	/* Segment name			 */
    unsigned int   s_num;	/* Segment number	     	 */
};

typedef struct
{
	struct MinNode  s_node;
	char *		s_name;
	void *          s_lowest;
	void *          s_highest;
} symbol_t;
