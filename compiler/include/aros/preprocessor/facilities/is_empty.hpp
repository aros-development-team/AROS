# ifndef AROS_PREPROCESSOR_FACILITIES_IS_EMPTY_HPP
# define AROS_PREPROCESSOR_FACILITIES_IS_EMPTY_HPP
#
# include <aros/preprocessor/variadic/first.hpp>
# include <aros/preprocessor/variadic/rest.hpp>
#
# define AROS_PP_IS_EMPTY(a)    AROS_PP_IS_EMPTY_I(a)
# define AROS_PP_IS_EMPTY_I(a)  AROS_PP_IS_EMPTY_II(a)
# define AROS_PP_IS_EMPTY_II(...)         \
      AROS_PP_VARIADIC_FIRST(             \
          AROS_PP_VARIADIC_REST(          \
              AROS_PP_VARIADIC_REST(      \
                  , ## __VA_ARGS__, 0, 1  \
              )                           \
          )                               \
      )                                   \
      /**/
#
# endif
