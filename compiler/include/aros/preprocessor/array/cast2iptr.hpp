# ifndef AROS_PREPROCESSOR_ARRAY_CAST2IPTR_HPP
# define AROS_PREPROCESSOR_ARRAY_CAST2IPTR_HPP
# 
# include <boost/preprocessor/repetition/repeat.hpp>
# include <boost/preprocessor/array.hpp>
# include <boost/preprocessor/comparison/equal.hpp>
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/logical/compl.hpp>
# include <boost/preprocessor/punctuation/comma_if.hpp>
#
# define AROS_PP_ARRAY_CAST2IPTR_M(z, i, array)                                      \
      (IPTR)(BOOST_PP_ARRAY_ELEM(i,array))BOOST_PP_COMMA_IF(                         \
          BOOST_PP_COMPL(BOOST_PP_EQUAL(BOOST_PP_ARRAY_SIZE(array),BOOST_PP_INC(i))) \
      )                                                                              \
      /**/
#
# define AROS_PP_ARRAY_CAST2IPTR(array)                                             \
      BOOST_PP_REPEAT(BOOST_PP_ARRAY_SIZE(array), AROS_PP_ARRAY_CAST2IPTR_M, array) \
      /**/
#
#endif

    
