# ifndef AROS_PREPROCESSOR_VARIADIC_CAST2IPTR_HPP
# define AROS_PREPROCESSOR_VARIADIC_CAST2IPTR_HPP
# 
# include <boost/preprocessor/repetition/for.hpp>
# include <boost/preprocessor/logical/compl.hpp>
# include <boost/preprocessor/punctuation/comma_if.hpp>
# include <aros/preprocessor/facilities/is_empty.hpp>
# include <aros/preprocessor/variadic/first.hpp>
# include <aros/preprocessor/variadic/rest.hpp>
#
# define AROS_PP_VARIADIC_CAST2IPTR_O(_, tuple) \
      (AROS_PP_VARIADIC_REST tuple)
#
# define AROS_PP_VARIADIC_CAST2IPTR_M(_, tuple)              \
      (IPTR)(AROS_PP_VARIADIC_FIRST tuple)BOOST_PP_COMMA_IF( \
          AROS_PP_VARIADIC_CAST2IPTR_P(,                     \
              (AROS_PP_VARIADIC_REST tuple)                  \
          )                                                  \
      )                                                      \
      /**/
#
# define AROS_PP_VARIADIC_CAST2IPTR_P(_, tuple) \
      BOOST_PP_COMPL(AROS_PP_IS_EMPTY(AROS_PP_VARIADIC_FIRST tuple))
#
# define AROS_PP_VARIADIC_CAST2IPTR(...)                              \
      BOOST_PP_FOR(                                                   \
          (__VA_ARGS__), AROS_PP_VARIADIC_CAST2IPTR_P,                \
          AROS_PP_VARIADIC_CAST2IPTR_O, AROS_PP_VARIADIC_CAST2IPTR_M  \
      )                                                               \
      /**/
#
# endif

