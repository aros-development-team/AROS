struct EXCEPTION_REGISTRATION
{
    struct EXCEPTION_REGISTRATION *prev;
    void *handler;
};

#define BEGIN_EXCEPTION(x)		    \
{					    \
    struct EXCEPTION_REGISTRATION _ex_frame;\
					    \
    _ex_frame.handler = x;		    \
    ADD_EXCEPTION_FRAME(_ex_frame)

#define END_EXCEPTION()		      \
    REMOVE_EXCEPTION_FRAME(_ex_frame);\
}
