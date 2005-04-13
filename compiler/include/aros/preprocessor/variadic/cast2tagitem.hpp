# ifndef AROS_PREPROCESSOR_VARIADIC_CAST2TAGITEM_HPP
# define AROS_PREPROCESSOR_VARIADIC_CAST2TAGITEM_HPP
# 
# include <boost/preprocessor/repetition/for.hpp>
# include <boost/preprocessor/logical/compl.hpp>
# include <boost/preprocessor/control/expr_iif.hpp>
# include <boost/preprocessor/punctuation/comma_if.hpp>
# include <boost/preprocessor/punctuation/comma.hpp>
# include <boost/preprocessor/facilities/expand.hpp>
# include <aros/preprocessor/facilities/is_empty.hpp>
# include <aros/preprocessor/variadic/first.hpp>
# include <aros/preprocessor/variadic/rest.hpp>
#
# define AROS_PP_VARIADIC_CAST2TAGITEM_O(_, tuple) \
      (AROS_PP_VARIADIC_REST(AROS_PP_VARIADIC_REST tuple))
#
# define AROS_PP_VARIADIC_CAST2TAGITEM_M(_, tuple)                     \
      {(Tag)(AROS_PP_VARIADIC_FIRST tuple),                            \
      BOOST_PP_EXPR_IIF(                                               \
           AROS_PP_VARIADIC_CAST2TAGITEM_P(,                           \
	       (AROS_PP_VARIADIC_FIRST(AROS_PP_VARIADIC_REST tuple))   \
	   ),                                                          \
	   (IPTR)(AROS_PP_VARIADIC_FIRST(AROS_PP_VARIADIC_REST tuple)) \
      )                                                                \
      }BOOST_PP_COMMA_IF(                                              \
              AROS_PP_VARIADIC_CAST2TAGITEM_P(,                        \
                  (AROS_PP_VARIADIC_REST(AROS_PP_VARIADIC_REST tuple)  \
              )                                                        \
          )                                                            \
      )                                                                \
      /**/
#
# define AROS_PP_VARIADIC_CAST2TAGITEM_P(_, tuple) \
      BOOST_PP_COMPL(AROS_PP_IS_EMPTY(AROS_PP_VARIADIC_FIRST tuple))
#
# define AROS_PP_VARIADIC_CAST2TAGITEM(...)                                \
      BOOST_PP_FOR(                                                        \
          (__VA_ARGS__), AROS_PP_VARIADIC_CAST2TAGITEM_P,                  \
          AROS_PP_VARIADIC_CAST2TAGITEM_O, AROS_PP_VARIADIC_CAST2TAGITEM_M \
      )                                                                    \
      /**/
#
# endif

