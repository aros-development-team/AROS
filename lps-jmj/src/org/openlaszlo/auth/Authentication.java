/* *****************************************************************************
 * Authentication.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.auth;

import java.util.*;
import javax.servlet.http.*;

public interface Authentication
{
    /**
     * Initializer.
     * @param prop properties passed down from lps.properties
     */
    void init(Properties prop)
        throws AuthenticationException;

    /** 
     * @return username or null if session is invalid
     */
    String getUsername(HttpServletRequest req, HttpServletResponse res,
                       HashMap param) throws AuthenticationException;

    /**
     * @param req use to get request headers
     * @param res use to get response headers
     * @param param authparams
     * @param xmlResponse xml response string to send back to client
     * @return success, failure, error status code
     */
    int login(HttpServletRequest req, HttpServletResponse res,
              HashMap param, StringBuffer xmlResponse)
        throws AuthenticationException;

    /**
     * @param req use to get request headers
     * @param res use to get response headers
     * @param param authparams
     * @param xmlResponse xml response string to send back to client
     * @return success, failure, error status code
     */
    int logout(HttpServletRequest req, HttpServletResponse res,
               HashMap param, StringBuffer xmlResponse)
        throws AuthenticationException;
}
