/******************************************************************************
 * LZHttpUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.net.URL;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpUtils;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.commons.httpclient.methods.*;
import org.apache.commons.httpclient.HttpMethodBase;
import org.apache.commons.httpclient.Header;
import org.apache.commons.httpclient.URI;
import org.apache.commons.httpclient.URIException;

import java.text.SimpleDateFormat;
import java.util.TimeZone;
import java.util.Date;
import java.util.Enumeration;
import java.security.*;

import org.apache.log4j.*;

import org.openlaszlo.utils.ChainedException;

/**
 * Utility class for http servlets
 */
public class LZHttpUtils {

    private static Logger mLogger  = Logger.getLogger(LZHttpUtils.class);

    public static final String CONTENT_ENCODING      = "Content-Encoding";
    public static final String CONTENT_LENGTH        = "Content-Length";
    public static final String CONTENT_TYPE          = "Content-Type";
    public static final String IF_MODIFIED_SINCE     = "If-Modified-Since";
    public static final String LAST_MODIFIED         = "Last-Modified";
    public static final String IF_NONE_MATCH         = "If-None-Match";
    public static final String TRANSFER_ENCODING     = "Transfer-Encoding";
    public static final String HOST                  = "Host";
    public static final String CONNECTION            = "Connection";
    public static final String AUTHORIZATION         = "Authorization";
    public static final String COOKIE                = "Cookie";
    public static final String CACHE_CONTROL         = "Cache-Control";
    public static final String USER_AGENT            = "User-Agent";
    public static final String ACCEPT_ENCODING       = "Accept-Encoding";
    public static final String RANGE                 = "Range";
    public static final String ACCEPT_RANGES         = "Accept-Ranges";
    public static final String IF_RANGE              = "If-Range";

    public static final String NO_STORE              = "no-store";
    public static final String NO_CACHE              = "no-cache";


    /**
     * @return the URL for the request // with the query string?
     * @param req the request
     */
    public static URL getRequestURL(HttpServletRequest req) {
        StringBuffer surl = HttpUtils.getRequestURL(req);
        String sturl = surl.toString(); // I love java!
        if (sturl.indexOf("https") == 0) {
            try {
                System.setProperty("java.protocol.handler.pkgs",
                                   "com.sun.net.ssl.internal.www.protocol");
                Class provClass = Class.forName("com.sun.net.ssl.internal.ssl.Provider");
                Provider provider = (Provider)provClass.newInstance(); 
                Security.addProvider(provider);
            } catch (InstantiationException e) {
                throw new ChainedException(e);
            } catch (IllegalAccessException e) {
                throw new ChainedException(e);
            } catch (ClassNotFoundException e) {
                throw new ChainedException(e);
            }
        }
        // surl.append("?");
        // surl.append(req.getQueryString());
        try {
            return new URL(surl.toString());
        } catch (Exception e) {
            throw new ChainedException(e);
        }
    }

    /**
     * For formatting HTTP dates
     */

    /**
     * Return a formatter for HTTP date headers 
     */
    private static SimpleDateFormat getGMTFormatter() {
        SimpleDateFormat dateFormatter =  
            new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss z");
        TimeZone tz = TimeZone.getTimeZone("GMT");
        dateFormatter.setTimeZone(tz);
        return dateFormatter;
    }

    /**
     * Convert a long to an HTTP Date String (TZ must be GMT)
     */
    public static String getDateString(long d) {
        SimpleDateFormat dateFormatter = getGMTFormatter();  
        return dateFormatter.format(new Date(d));
    }

    /**
     * Convert an HTTP Date String to a long 
     */
    public static long getDate(String s) {
        try {
            SimpleDateFormat dateFormatter = getGMTFormatter();
            return dateFormatter.parse(s).getTime();
        } catch (java.text.ParseException e) {
            throw new ChainedException(e);
        }
    }

    /** From RFC2616, 14.10:        
     *
     * HTTP/1.1 proxies MUST parse the Connection header field before a message
     * is forwarded and, for each connection-token in this field, remove any
     * header field(s) from the message with the same name as the
     * connection-token. Connection options are signaled by the presence of a
     * connection-token in the Connection header field, not by any corresponding
     * additional header field(s), since the additional header field may not be
     * sent if there are no parameters associated with that connection
     * option. */
    static public boolean allowForward(String header, Enumeration connEnum)
    {
        if (header.toLowerCase().startsWith("content-"))
            return false;

        if (header.equalsIgnoreCase(CONNECTION))
            return false;

        if (header.equalsIgnoreCase(HOST))
            return false;

        if (header.equalsIgnoreCase(TRANSFER_ENCODING))
            return false;

        if (header.equalsIgnoreCase(IF_MODIFIED_SINCE))
            return false;

        if (header.equalsIgnoreCase(LAST_MODIFIED))
            return false;

        if (header.equalsIgnoreCase(IF_NONE_MATCH))
            return false;

        if (header.equalsIgnoreCase(ACCEPT_ENCODING))
            return false;

        // Someday we may allow these only when the proxy is non-transcoding
        if (header.equalsIgnoreCase(RANGE))
            return false;

        // Someday we may allow these only when the proxy is non-transcoding
        if (header.equalsIgnoreCase(ACCEPT_RANGES))
            return false;

        // Someday we may allow these only when the proxy is non-transcoding
        if (header.equalsIgnoreCase(IF_RANGE))
            return false;

        // Don't forward any headers that have the same name as a connection
        // token.
        if (connEnum != null) {
            while (connEnum.hasMoreElements()) {
                String token = (String)connEnum.nextElement();
                if (header.equalsIgnoreCase(token))
                    return false;
            }
        }

        return true;
    }

    /** Add request headers into method. 
     *
     * @param req http servlet request object
     * @param method method to insert request headers into 
     */
    static public void proxyRequestHeaders(HttpServletRequest req,
                                           HttpMethodBase method)
    {
        mLogger.debug("proxyRequestHeaders");
        
        // Copy all headers, if the servlet container allows, otherwise just
        // copy the cookie header and log a message.
        Enumeration enum = req.getHeaderNames();
        if (enum!=null) {

            // Connection header tokens not to forward
            Enumeration connEnum = req.getHeaders(CONNECTION);

            while (enum.hasMoreElements()) {
                String key = (String)enum.nextElement();
                if (allowForward(key, connEnum)) {
                    String val = (String)req.getHeader(key);
                    method.addRequestHeader(key,val);
                    mLogger.debug("  " + key + "=" + val);
                }
            }

        } else {
            mLogger.warn("Can't get header names to proxy request headers");

            Enumeration cookieEnum = req.getHeaders(COOKIE);
            if (cookieEnum != null) {
                while (cookieEnum.hasMoreElements()) {
                    String val = (String)cookieEnum.nextElement();
                    method.addRequestHeader(COOKIE, val);
                    mLogger.debug("  Cookie=" + val);
                }
            }

            Enumeration authEnum = req.getHeaders(AUTHORIZATION);
            if (authEnum != null) {
                while (authEnum.hasMoreElements()) {
                    String val = (String)authEnum.nextElement();
                    method.addRequestHeader(AUTHORIZATION, val);
                    mLogger.debug("  Authorization=" + val);
                }
            }
        }
    }

    /** Pull response headers from method and put into 
     * servlet response object.
     *
     * @param req http servlet response object to proxy to
     * @param method method to proxy from
     * @param isSecure true if get method is secure
     */
    static public void proxyResponseHeaders(GetMethod meth, 
                                            HttpServletResponse res,
                                            boolean isSecure)
    {
        mLogger.debug("proxyResponseHeaders");
        
        Header[] hedz = meth.getResponseHeaders();

        for (int i = 0; i < hedz.length; i++) {
            String name = hedz[i].getName();
            String value = hedz[i].getValue();
            // Content length passed back to swf app will be different
            if (allowForward(name, null)) {
                // Don't send no-cache headers request is SSL; IE 6 has
                // problems.
                if (isSecure) {
                    if (name.equals("Pragma") && value.equals("no-cache"))
                        continue;
                    if (name.equals("Cache-Control") && value.equals("no-cache"))
                        continue;
                }

                mLogger.debug("  " + name + "=" + value);

                try {
                    if (name.equals("Date") || name.equals("Server")) {
                        res.setHeader(name, value);
                    } else {
                        res.addHeader(name, value);
                    }
                } catch (Exception e) {
                    mLogger.error("Exception when proxying a response header: " + e.getMessage());
                }
            }
        }
    }

    /**
     * Fetch cookie value for a particular cookie name.
     *
     * @param req servlet request.
     * @param name name of cookie key to fetch.
     */
    static public String getCookie(HttpServletRequest req, String name)
    {
        javax.servlet.http.Cookie[] cookies = req.getCookies();
        if (cookies != null) {
            for (int i=0; i < cookies.length; i++) {
                javax.servlet.http.Cookie cookie = cookies[i];
                if (cookie.getName().equals(name)) {
                    return cookie.getValue();
                }
            }
        }
        return null;
    }

    /** 
     * Replace real path forward slash characters to back-slash for Windoze.
     * This is to get around a WebSphere problem (see bug 988). Note that if the
     * web application content is being served directly from a .war file, this
     * method will return null. See ServletContext.getRealPath() for more
     * details.
     *
     * @param ctx servlet context
     * @param path virtual webapp path to resolve into a real path
     * @return the real path, or null if the translation cannot be performed
     */
    static public String getRealPath(ServletContext ctxt, String path)
    {
        String realPath = ctxt.getRealPath(path);
        if ( realPath != null && File.separatorChar == '\\' )
            realPath = realPath.replace('/', '\\');
        try {
            return new File(realPath).getCanonicalPath();
        } catch (java.io.IOException e) {
            throw new org.openlaszlo.utils.ChainedException(e);
        }
    }

    /**
     * Replace real path forward slash characters to back-slash for Windoze.
     *
     * @param ctx servlet context
     * @param generating request
     * @return the real path, or null if the translation cannot be performed
     */
    static public String getRealPath(ServletContext ctxt, HttpServletRequest req)
    {
        String uriwithoutcontext = req.getRequestURI().substring(req.getContextPath().length());
        if (uriwithoutcontext != null && File.separatorChar == '\\' ) {
            uriwithoutcontext = uriwithoutcontext.replace('/', '\\');
        }
        return getRealPath(ctxt, "/") + uriwithoutcontext;
    }

    private static String WEBAPP = "/@WEBAPP@/";

    /** 
     * If a URL contains <code>/@WEBAPP@</code>, replaces that string with
     * context path. If context path is <code>/</code>, the function just
     * removes the <code>/@WEBAPP@</code> string.
     *
     * @param contextPath current context path.
     * @param url URL to check if <code>/@WEBAPP</code> token exists.
     * @return if <code>/@WEBAPP@</code> exists, new modified URL else old URL.
     */
    public static String modifyWEBAPP(HttpServletRequest req, String url)
    {
        mLogger.debug("modifyWEBAPP");
        if (url.startsWith(WEBAPP)) {
            mLogger.debug("    Old URL: " + url);
            String protocol = (req.isSecure()?"https":"http");
            String host = req.getServerName();
            int port = req.getServerPort();
            String cp = req.getContextPath();
            url = protocol + "://" + host + ":" + port + cp 
                + url.substring(WEBAPP.length()-1);
            mLogger.debug("    New URL: " + url);
        }
        return url;
    }

    /**
     * Mark response with no-store cache control
     */
    static public void noStore(HttpServletResponse res) {
        if (res.containsHeader(LZHttpUtils.CACHE_CONTROL)) {
            mLogger.warn("over-riding back-end cache-control header to: no-store");
        } 
        res.setHeader(CACHE_CONTROL, NO_STORE);
    }


    /**
     * Return a URI object, escaping input only if needed
     */
    static public URI newURI(String s) throws URIException {
        try {
            return new URI(s.toCharArray());
        } catch (URIException urie) {
            // Try escaping
            try {
                return new URI(s);
            } catch (Exception e) {
                // Escaping failed, throw the original error
                throw urie;
            }
        }
    }

    /**
     * Decodes a  urlencoded string using a specific charset encoding.
     */
    public static String urldecode(String s, String enc) 
      throws UnsupportedEncodingException {
        boolean needToChange = false;
        StringBuffer sb = new StringBuffer();
        int numChars = s.length();
        int i = 0;

        if (enc.length() == 0) {
            throw new UnsupportedEncodingException ("LzHTTPUtils.urldecode: empty string enc parameter");
        }

        while (i < numChars) {
            char c = s.charAt(i);
            switch (c) {
              case '+':
                sb.append(' ');
                i++;
                needToChange = true;
                break;
              case '%':
                /*
                 * Starting with this instance of %, process all
                 * consecutive substrings of the form %xy. Each
                 * substring %xy will yield a byte. Convert all
                 * consecutive  bytes obtained this way to whatever
                 * character(s) they represent in the provided
                 * encoding.
                 */

                try {

                    // (numChars-i)/3 is an upper bound for the number
                    // of remaining bytes
                    byte[] bytes = new byte[(numChars-i)/3];
                    int pos = 0;
            
                    while ( ((i+2) < numChars) && 
                            (c=='%')) {
                        bytes[pos++] = 
                            (byte)Integer.parseInt(s.substring(i+1,i+3),16);
                        i+= 3;
                        if (i < numChars)
                            c = s.charAt(i);
                    }

                    // A trailing, incomplete byte encoding such as
                    // "%x" will cause an exception to be thrown

                    if ((i < numChars) && (c=='%'))
                        throw new IllegalArgumentException(
                            "URLDecoder: Incomplete trailing escape (%) pattern");
            
                    sb.append(new String(bytes, 0, pos, enc));
                } catch (NumberFormatException e) {
                    throw new IllegalArgumentException(
                        "URLDecoder: Illegal hex characters in escape (%) pattern - " 
                        + e.getMessage());
                }
                needToChange = true;
                break;
              default: 
                sb.append(c); 
                i++;
                break; 
            }
        }
        return (needToChange? sb.toString() : s);
    }

}
