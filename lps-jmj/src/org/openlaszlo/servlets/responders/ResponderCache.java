/******************************************************************************
 * ResponderCache.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.net.UnknownHostException;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.Properties;
import java.util.HashMap;
import java.util.Iterator;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.ServletOutputStream;
import org.openlaszlo.cache.RequestCache;
import org.openlaszlo.data.*;
import org.openlaszlo.media.MimeType;
import org.openlaszlo.server.LPS;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.xml.internal.XMLUtils;
import org.apache.commons.httpclient.URI;
import org.apache.commons.httpclient.URIException;
import org.apache.log4j.Logger;

public abstract class ResponderCache extends Responder
{
    private static boolean mIsInitialized = false;

    private static HashMap mDataSourceMap = new HashMap();
    private static DataSource mHTTPDataSource = null;

    private static Logger mLogger = Logger.getLogger(ResponderCache.class);

    protected RequestCache mCache = null;

    /**
     * Keeps track of url stats.
     */
    public class URLStat
    {
        String mName;

        final static public int ERRTYPE_NONE             = -1;
        final static public int ERRTYPE_CONVERSION       = 0;
        final static public int ERRTYPE_DATA_SOURCE      = 1;
        final static public int ERRTYPE_UNKNOWN_HOST     = 2;
        final static public int ERRTYPE_MALFORMED_URL    = 3;
        final static public int ERRTYPE_IO               = 4;
        final static public int ERRTYPE_ILLEGAL_ARGUMENT = 5;
        final static public int ERRTYPE_TIMEOUT          = 6;
        final static public int ERRTYPE_FORBIDDEN        = 7;
        final static public int ERRTYPE_OTHER            = 8;
        final static public int NUM_ERRTYPES             = 9;


        HashMap mURLs = new HashMap();
        HashMap mErrorURLs = new HashMap();

        int mSuccessCount;
        /**
         * 0: mConversionException, 1: mDataSourceException, 2: mUnknownHostException,
         * 3: mMalformedURLException, 4: mIOException, 5: mIllegalArgumentException,
         * 6: mInterrupedIOException, 7 mException
         */
        int[] mErrorCount = new int[NUM_ERRTYPES];

        
        boolean mDoURLCollection = false;
        

        /**
         * Create an URLStat with a name.
         */
        public URLStat(String name)
        {
            mName = name;
            clear();
        }
        
        /**
         * @param doCollection whether the object should collect unique url
         * info.
         */
        void doURLCollection(boolean doCollection)
        {
            if (mDoURLCollection != doCollection) {
                mDoURLCollection = doCollection;
                clear();
            }
        }

        /**
         * Increment stat on url with successful status.
         * @param url successful url.
         */
        void success(String url)
        {
            int x = url.indexOf('?');
            if (x != -1) {
                url = url.substring(0, x);
            }
            synchronized (mURLs) {
                if (mDoURLCollection) {
                    // Add unique urls
                    int[] s = (int[]) mURLs.get(url);
                    if (s == null) {
                        s = new int[1];
                        mURLStat.mURLs.put(url, s);
                    }
                    ++s[0];
                }
                ++mSuccessCount;
            }
        }


        /**
         * Increment stat on url with error status.
         *
         * @param errType see ERRTYPEs.
         * @param url error url.
         */
        void error(int errType, String url)
        {
            int x = url.indexOf('?');
            if (x != -1) {
                url = url.substring(0, x);
            }
            synchronized (mErrorURLs) {
                if (mDoURLCollection) {
                    int[] e = (int[]) mErrorURLs.get(url);
                    if (e == null) {
                        e = new int[NUM_ERRTYPES];
                        mErrorURLs.put(url, e);
                    }
                    ++e[errType];
                }
                ++mErrorCount[errType];
            }

        }

        /**
         * Clear URL stats.
         */
        void clear()
        {
            synchronized (mURLs) {
                mURLs.clear();
                mSuccessCount = 0;
            }

            synchronized (mErrorURLs) {
                mErrorURLs.clear();
                for (int i=0; i < mErrorCount.length; i++)
                    mErrorCount[i] = 0;
            }
        }

        /**
         * Return url statistics in XML string.
         * @return xml info.
         */
        public String toXML()
        {
            StringBuffer buf = new StringBuffer();
            synchronized (mURLs) {
                buf.append("<").append(mName).append("-urls ")
                    .append(" unique=\"").append(mURLs.size()).append("\"")
                    .append(">\n");

                buf.append("<success")
                    .append(" total-requests=\"").append(mSuccessCount).append("\"")
                    .append(">\n");
                if (mDoURLCollection)
                {
                    Iterator iter = mURLs.keySet().iterator();
                    while (iter.hasNext()) {
                        String k = (String)iter.next();
                        int[] success = (int[])mURLs.get(k);
                        buf.append("<url")
                            .append(" requests=\"").append(success[0]).append("\"")
                            .append(" href=\"").append(XMLUtils.escapeXml(k)).append("\" />");
                    }
                }
                buf.append("</success>\n");
            }

            synchronized (mErrorURLs) {
                int errTotal = 0;
                for (int i=0; i < mErrorCount.length; i++)
                    errTotal += mErrorCount[i];
                buf.append("<errors")
                    .append(" total-errors=\"").append(errTotal).append("\"")
                    .append(" conversion=\"").append(mErrorCount[ERRTYPE_CONVERSION]).append("\"")
                    .append(" datasource=\"").append(mErrorCount[ERRTYPE_DATA_SOURCE]).append("\"")
                    .append(" unknownhost=\"").append(mErrorCount[ERRTYPE_UNKNOWN_HOST]).append("\"")
                    .append(" malformedurl=\"").append(mErrorCount[ERRTYPE_MALFORMED_URL]).append("\"")
                    .append(" ioexception=\"").append(mErrorCount[ERRTYPE_IO]).append("\"")
                    .append(" illegalargument=\"").append(mErrorCount[ERRTYPE_ILLEGAL_ARGUMENT]).append("\"")
                    .append(" timeout=\"").append(mErrorCount[ERRTYPE_TIMEOUT]).append("\"")
                    .append(" forbidden=\"").append(mErrorCount[ERRTYPE_FORBIDDEN]).append("\"")
                    .append(" uncaught-exception=\"").append(mErrorCount[ERRTYPE_OTHER]).append("\"")
                    .append(">\n");
                if (mDoURLCollection)
                {
                    Iterator iter = mErrorURLs.keySet().iterator();
                    while (iter.hasNext()) {
                        String k = (String)iter.next();
                        int[] e = (int[])mErrorURLs.get(k);
                        buf.append("<url")
                            .append(" conversion=\"").append(e[ERRTYPE_CONVERSION]).append("\"")
                            .append(" datasource=\"").append(e[ERRTYPE_DATA_SOURCE]).append("\"")
                            .append(" unknownhost=\"").append(e[ERRTYPE_UNKNOWN_HOST]).append("\"")
                            .append(" malformedurl=\"").append(e[ERRTYPE_MALFORMED_URL]).append("\"")
                            .append(" ioexception=\"").append(e[ERRTYPE_IO]).append("\"")
                            .append(" illegalargument=\"").append(e[ERRTYPE_ILLEGAL_ARGUMENT]).append("\"")
                            .append(" timeout=\"").append(e[ERRTYPE_TIMEOUT]).append("\"")
                            .append(" forbidden=\"").append(e[ERRTYPE_FORBIDDEN]).append("\"")
                            .append(" uncaught-exception=\"").append(e[ERRTYPE_OTHER]).append("\"")
                            .append(" href=\"").append(XMLUtils.escapeXml(k)).append("\"")
                            .append(" />\n");
                    }
                }
                buf.append("</errors>\n");
            }

            buf.append("</").append(mName).append("-urls>\n");

            return buf.toString();
        }
    }

    public URLStat mURLStat = null;

    synchronized public void init(String reqName, ServletConfig config, 
            RequestCache cache, Properties prop)
        throws ServletException, IOException
    {
        super.init(reqName, config, prop);

        String reqProp = reqName.toLowerCase() + "Request.collectURL";
        boolean doURLCollection =
            prop.getProperty(reqProp, "false").intern() == "true";

        mURLStat = new URLStat(reqName);
        mURLStat.doURLCollection(doURLCollection);

        if (! mIsInitialized) {
            //------------------------------------------------------------
            // Well-known data sources
            //------------------------------------------------------------
            mHTTPDataSource   = new HTTPDataSource();

            mDataSourceMap.put("http",   mHTTPDataSource);
            try {
                mDataSourceMap.put("file",   new FileDataSource());
            } catch (Throwable e) {
                mLogger.warn("can't load file datasource", e);
            }

            try {
                mDataSourceMap.put("java",   new JavaDataSource());
            } catch (Throwable e) {
                mLogger.warn("can't load java datasource", e);
            }

            try {
                mDataSourceMap.put("soap",   new SOAPDataSource());
            } catch (Throwable e) {
                mLogger.warn("can't load soap datasource", e);
            }

            try {
                mDataSourceMap.put("xmlrpc", new XMLRPCDataSource());
            } catch (Throwable e) {
                mLogger.warn("can't load xmlrpc datasource", e);
            }

            mIsInitialized = true;
        }

        mCache = cache; 
    }

    protected void respondImpl(HttpServletRequest req, HttpServletResponse res) {

        String path = req.getServletPath();
        String url;
        try {
            url  = DataSource.getURL(req);
        } catch (java.net.MalformedURLException e) {
            respondWithErrorSWF(res, "bad url: " + e.getMessage());
            if (mCollectStat) {
                mURLStat.error(URLStat.ERRTYPE_MALFORMED_URL, "bad-url");
            }
            return;
        }

        if (path.endsWith(".lzo")) {
            path = path.substring(0, path.length() - 1) + "x";
        }

        if (req.getMethod().intern() == "POST") {
            float fpv = getFlashPlayerVersion(req);
            String ua = req.getHeader(LZHttpUtils.USER_AGENT);
            mLogger.debug("POST request, flash player version: " + fpv);
            if (fpv < 6.47 && 
                LPS.configuration.optionAllows("disable-post-keep-alive", ua)) {
                // Prevent browser keep-alive to get around bug 4048.
                mLogger.debug("Disabling keep-alive for " + ua);
                res.setHeader("Connection", "close");
                res.setHeader("Keep-Alive", "close");
            }
        }

        if ( ! LPS.configuration.optionAllows(path, "proxy-security-urls", url) ) {
            String err = "Forbidden url: " +  url;
            respondWithError(res, err, HttpServletResponse.SC_FORBIDDEN);
            mLogger.error(err);
            if (mCollectStat) {
                mURLStat.error(URLStat.ERRTYPE_FORBIDDEN, url);
            }
            return;
        }

        int errType = URLStat.ERRTYPE_NONE;

        try { 

            DataSource source = getDataSource(req, res);
            if (source == null) {
                return;
            }

            res.setContentType(MimeType.SWF);

            String app = LZHttpUtils.getRealPath(mContext, req);
            boolean isClientCacheable = DataSource.isClientCacheable(req);
            if (mCache.isCacheable(req)) {
                if (isClientCacheable) {
                    mLogger.info("proxying " + url + ", cacheable on server and client");
                } else {
                    mLogger.info("proxying " + url + ", cacheable on server and not client");
                }
                mCache.getAsSWF(app, req, res, source);
            } else {
                if (isClientCacheable) {
                    mLogger.info("proxying " + url + ", not cacheable on server and cacheable on the client");
                } else {
                    mLogger.info("proxying " + url + ", not cacheable on server or client");
                }
                source.getAsSWF(app, req, res, getConverter());
            }
        } catch (ConversionException e) {
            respondWithErrorSWF(res, "data conversion error for " + url + 
                                     ": " + e.getMessage());
            errType = URLStat.ERRTYPE_CONVERSION;
        } catch (DataSourceException e) {
                respondWithErrorSWF(res, "data source error for " + url +
                                         ": " + e.getMessage());
            errType = URLStat.ERRTYPE_DATA_SOURCE;
        } catch (UnknownHostException e) {
            respondWithErrorSWF(res, "unknown host for " + url + 
                                     ": " +  e.getMessage());
            errType = URLStat.ERRTYPE_UNKNOWN_HOST;
        } catch (URIException e) {
            respondWithErrorSWF(res, "bad url: " + e.getMessage());
            errType = URLStat.ERRTYPE_MALFORMED_URL;
        } catch (MalformedURLException e) {
            respondWithErrorSWF(res, "bad url: " + e.getMessage());
            errType = URLStat.ERRTYPE_MALFORMED_URL;
        } catch (InterruptedIOException e) {
            respondWithErrorSWF(res, "backend timeout for " + url +
                                     ": " + e.getMessage());
            errType = URLStat.ERRTYPE_TIMEOUT;
        } catch (IOException e) {
            // Handle SocketTimeoutExceptions as timeouts instead of IO issues
            Class stec = null;
            try {
                stec = Class.forName("java.net.SocketTimeoutException");
            } catch (ClassNotFoundException cfne) {
            }
            if (stec != null && stec.isAssignableFrom(e.getClass())) {
                errType = URLStat.ERRTYPE_TIMEOUT;
                respondWithErrorSWF(res, "backend timeout for " + url +
                                         ": " + e.getMessage());
            } else {
                respondWithExceptionSWF(res, e);
                errType = URLStat.ERRTYPE_IO;
            }
        } catch (IllegalArgumentException e) {
            respondWithExceptionSWF(res, e);
            errType = URLStat.ERRTYPE_ILLEGAL_ARGUMENT;
        } catch (Throwable e) { 
            // Makes much easier to debug runtime exceptions
            // but perhaps not strictly correct.
            respondWithExceptionSWF(res, e);
            errType = URLStat.ERRTYPE_OTHER;
        } 

        if (mCollectStat) {
            if (errType == URLStat.ERRTYPE_NONE)
                mURLStat.success(url);
            else 
                mURLStat.error(errType, url);
        }
    }

    /**
     * @return the datasource for this request
     */
    protected DataSource getDataSource(HttpServletRequest req, 
                                       HttpServletResponse res) 
        throws MalformedURLException, URIException
    {
        String ds = "http";
        String urlstr = DataSource.getURL(req);
        if (urlstr != null) {
            mLogger.debug("urlstr " + urlstr);
            URI uri = LZHttpUtils.newURI(urlstr);
            String protocol = uri.getScheme();
            if (protocol != null && protocol.equals("https")) {
                    protocol = "http";
            }
            ds = protocol;
        }

        mLogger.debug("ds is " + ds);

        DataSource source = null;
        if (ds == null) {
            source = mHTTPDataSource;
        } else {
            source = (DataSource) mDataSourceMap.get(ds);
            if (source == null) 
                respondWithErrorSWF(res, "Can't find a data source for " + urlstr);
        }
        return source;
    }

    public int getMimeType()
    {
        return MIME_TYPE_SWF;
    }

    public float getFlashPlayerVersion(HttpServletRequest req) {
        float fpv = (float)-1.0;
        try {
            String _fpv = req.getParameter("fpv");
            if (_fpv != null)
                fpv = Float.parseFloat(_fpv);
        } catch (NumberFormatException e) {
            mLogger.debug(e.getMessage());
        }
        return fpv;
    }

    /**
     * @return the converter to be used by this cache
     */
    public Converter getConverter() {
        return mCache.getConverter();
    }
}
