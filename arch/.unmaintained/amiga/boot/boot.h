
struct ilsMemList
{
	struct MinList iml_List;
	ULONG iml_Num;
};

struct ilsMemNode
{
	struct MinNode imn_Node;
	APTR imn_Addr;				/* address of memory region */
	ULONG imn_Size;				/* size of memory region */
};
