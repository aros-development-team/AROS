typedef struct {
	struct MinNode	m_node;
	void *		segList;
	char *          m_str;
	void *          m_lowest;
	void *          m_highest;
	struct MinList  m_symbols;
	char		m_name[1];
} module_t;

typedef struct {
	struct MinNode  s_node;
	char *		s_name;
	void *          s_lowest;
	void *          s_highest;
} symbol_t;
