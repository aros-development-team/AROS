# ifndef AROS_PREPROCESSOR_VARIADIC_CAST2TAGITEM_HPP
# define AROS_PREPROCESSOR_VARIADIC_CAST2TAGITEM_HPP
# 
# undef BOOST_PP_VARIADICS
# define  BOOST_PP_VARIADICS 1
#
# define AROS_PP_VARIADIC_SIZE_OVERRIDES_BOOST 1
# include <aros/preprocessor/variadic/size.hpp>
#
# define AROS_PP_VARIADIC_ELEM_OVERRIDES_BOOST 1
# include <aros/preprocessor/variadic/elem.hpp>
#
# include <aros/preprocessor/array/cast2tagitem.hpp>
# include <boost/preprocessor/tuple/enum.hpp>
# include <boost/preprocessor/if.hpp>
# include <boost/preprocessor/facilities/is_empty.hpp>
#
# include <boost/preprocessor/variadic/to_array.hpp>
#
# define AROS_PP_VARIADIC_CAST2TAGITEM(...)                                          \
      BOOST_PP_TUPLE_ENUM(                                                           \
          BOOST_PP_IF(                                                               \
              BOOST_PP_COMPL(BOOST_PP_IS_EMPTY(__VA_ARGS__)),                        \
              (AROS_PP_ARRAY_CAST2TAGITEM(BOOST_PP_VARIADIC_TO_ARRAY(__VA_ARGS__))), \
              ()                                                                     \
          )                                                                          \
      )                                                                              \
      /**/
#     
#endif

    
