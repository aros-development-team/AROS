/* *****************************************************************************
 * NullAuthentication.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.auth;

import javax.servlet.http.*;
import java.util.*;

public class NullAuthentication implements Authentication
{
    public static final String DEFAULT_USERNAME = "user";
    public String mDefaultUserName = null;

    public void init(Properties prop) {
        mDefaultUserName = 
            prop.getProperty("connection.none-authenticator.username", DEFAULT_USERNAME);
    }

    public int login(HttpServletRequest req, HttpServletResponse res,
              HashMap param, StringBuffer xmlResponse) {

        String usr = (String) param.get("usr");
        if (usr == null)
            usr = mDefaultUserName;

        xmlResponse
            .append("<authentication>")
            .append("<response type=\"login\">")
            .append("<status code=\"0\" msg=\"ok\" />")
            .append("<username>").append(usr).append("</username>")
            .append("</response>")
            .append("</authentication>");
        return 0;
    }


    public int logout(HttpServletRequest req, HttpServletResponse res,
               HashMap param, StringBuffer xmlResponse) {
        xmlResponse
            .append("<authentication>")
            .append("<response type=\"logout\">")
            .append("<status code=\"0\" msg=\"ok\" />")
            .append("</response>")
            .append("</authentication>");
        return 0;
    }


    public String getUsername(HttpServletRequest req, HttpServletResponse res,
                              HashMap param)
    {
        String usr = (String) param.get("usr");
        return (usr!=null ? usr : mDefaultUserName);
    }
}
