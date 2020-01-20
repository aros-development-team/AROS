#if (defined(__cplusplus) || defined(__STDC__) || defined(c_plusplus))
#define ARGS(parenthesized_list) parenthesized_list
#define STDC		1
#define VOID_ARG	void
#else
#define ARGS(parenthesized_list) ()
#define STDC		0
#define VOID_ARG
#define const
#endif

#if !defined(EXIT_SUCCESS)
#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
#endif
