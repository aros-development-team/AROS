/*
    Copyright © 2020-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

/* When libheif is built against boost::optional (pre-C++17 toolchains)
 * on a target compiled with -fno-exceptions, boost defines
 * BOOST_NO_EXCEPTIONS and expects the application to provide the
 * boost::throw_exception() handler it calls instead of throwing.
 */
#if defined(LIBHEIF_USE_BOOST_OPTIONAL) && !defined(__EXCEPTIONS)
#include <exception>
#include <cstdlib>

namespace boost
{
    void throw_exception(std::exception const &e)
    {
        std::abort();
    }
}
#endif
