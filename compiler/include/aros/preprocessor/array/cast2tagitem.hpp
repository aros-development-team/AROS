# ifndef AROS_PREPROCESSOR_ARRAY_CAST2TAGITEM_HPP
# define AROS_PREPROCESSOR_ARRAY_CAST2TAGITEM_HPP
# 
# include <boost/preprocessor/repetition/repeat.hpp>
# include <boost/preprocessor/array.hpp>
# include <boost/preprocessor/comparison/equal.hpp>
# include <boost/preprocessor/arithmetic/inc.hpp>
# include <boost/preprocessor/arithmetic/div.hpp>
# include <boost/preprocessor/arithmetic/mul.hpp>
# include <boost/preprocessor/logical/compl.hpp>
# include <boost/preprocessor/punctuation/comma_if.hpp>
# include <boost/preprocessor/control/expr_if.hpp>
#
# define AROS_PP_TAGITEM_TAG(i, array)                    \
      (Tag)(BOOST_PP_ARRAY_ELEM(BOOST_PP_MUL(i,2),array)) \
      /**/
      
# define AROS_PP_TAGITEM_ITEM(i, array)                                      \
      BOOST_PP_EXPR_IF(                                                      \
          BOOST_PP_NOT_EQUAL(i,BOOST_PP_DIV(BOOST_PP_ARRAY_SIZE(array),2)),  \
          (IPTR)(BOOST_PP_ARRAY_ELEM(BOOST_PP_INC(BOOST_PP_MUL(i,2)),array)) \
      )                                                                      \
      /**/
#
# define AROS_PP_ARRAY_CAST2TAGITEM_M(z, i, array)                                                                 \
      { AROS_PP_TAGITEM_TAG(i, array), AROS_PP_TAGITEM_ITEM(i, array) }BOOST_PP_COMMA_IF                           \
      (                                                                                                            \
          BOOST_PP_COMPL(BOOST_PP_EQUAL(BOOST_PP_DIV(BOOST_PP_INC(BOOST_PP_ARRAY_SIZE(array)),2),BOOST_PP_INC(i))) \
      )                                                                                                            \
      /**/
#
# define AROS_PP_ARRAY_CAST2TAGITEM(array)                                                                           \
      BOOST_PP_REPEAT(BOOST_PP_DIV(BOOST_PP_INC(BOOST_PP_ARRAY_SIZE(array)),2), AROS_PP_ARRAY_CAST2TAGITEM_M, array) \
      /**/
#
#endif

    
