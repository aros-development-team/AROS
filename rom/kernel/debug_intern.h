typedef struct
{
    char	 *m_shstr;   /* Section headers table */
    char	 *m_str;     /* Symbol names table    */
    unsigned int  m_segcnt;  /* Count of segments     */
    dbg_mod_t	  mod;
} module_t;

struct segment
{
    struct MinNode s_node;	/* For linking into the list 	 */
    BPTR	   s_seg;	/* DOS segment pointer		 */
    void *	   s_lowest;	/* Start address	     	 */
    void *	   s_highest;	/* End address		     	 */
    module_t *	   s_mod;	/* Module descriptor	     	 */
    char *	   s_name;	/* Segment name			 */
    unsigned int   s_num;	/* Segment number	     	 */
};
