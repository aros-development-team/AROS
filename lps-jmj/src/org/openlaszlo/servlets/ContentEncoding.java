/* *****************************************************************************
 * ContentEncoding.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets;

import javax.servlet.*;
import javax.servlet.http.*;

import java.util.StringTokenizer;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;

/**
 * ContentEncoding is representation of an HTTP 1.1 Content Encoding
 * according to RFC 2616:
 * <a href="http://www.w3.org/Protocols/rfc2616/rfc2616.html"
 *  >http://www.w3.org/Protocols/rfc2616/rfc2616.html</a>
 *  This spec is less than perfect.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">bloch@laszlosystems.com</a>
 */
public class ContentEncoding {

    public String name = null;
    public float  q    = 0;

    /**
     * @param req an HTTP request 
     * @return ContentEncoding [] an array of encodings acceptable by
     * client making this request
     */
    private static List parseEncodings(HttpServletRequest req) {    

        String acceptHeader = req.getHeader("Accept-Encoding");
        if (acceptHeader == null) {
            return null;
        }

        StringTokenizer toker = new StringTokenizer(acceptHeader, ",");
        
        int             numEncodings = toker.countTokens();

        ArrayList encs = new ArrayList(numEncodings);
        int             i = 0;

        while (toker.hasMoreElements()) {
            String token = toker.nextToken();
            StringTokenizer t = new StringTokenizer(token, ";");
            ContentEncoding enc = new ContentEncoding();
            enc.name = t.nextToken().trim();
            if (t.countTokens() > 1) {
                enc.q = Float.parseFloat(t.nextToken().trim());
            } else {
                enc.q = 1;
            }
            encs.add(enc);
        }

        return encs;
    }

    /**
     * @return the encoding that is "best" among the array for 
     * a response.  We support gzip and deflate
     * @param req HttpServlet request
     */
    public static String chooseEncoding(HttpServletRequest req) {

        List encs = parseEncodings(req);
        if (encs == null) {
            return null;
        }

        // First try gzip(transduce x-gzip to gzip) and then 
        // try deflate.  Otherwise try null.
        //
        // FIXME: [2002-12-17] The spec says we should use a more complicated 
        // algorithm but this will probably work in general.  Time will tell.
        
        Iterator iter;

        iter = encs.iterator();
        while (iter.hasNext()) {
            ContentEncoding e = (ContentEncoding)iter.next();
            if (e.name.equals("gzip") || e.name.equals("x-gzip")) {
                return "gzip";
            }
        }

        // FIXME: [2002-12-17 bloch]deflate as used in CompilationManager
        // doesn't seem to produce bits that mozilla or ie can cope with.  Hmmm...
        /*
        iter = encs.iterator();
        while (iter.hasNext()) {
            ContentEncoding e = (ContentEncoding)iter.next();
            if (e.name.equals("deflate")) {
                return "deflate";
            }
        }
        */
        
        return null;
   }
}
