/* *****************************************************************************
 * HTTPDataSource.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.net.URL;
import java.net.URLDecoder;
import java.net.MalformedURLException;
import java.net.UnknownHostException;
import java.io.*;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.commons.httpclient.*;
import org.apache.commons.httpclient.methods.*;
import org.apache.commons.httpclient.util.*;
import org.apache.log4j.*;

import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.LZGetMethod;
import org.openlaszlo.utils.LZPostMethod;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.server.LPS;
import org.apache.oro.text.regex.*;


/**
 * HTTP Transport
 */
public class HTTPDataSource extends DataSource {
    
    private static Logger mLogger  = Logger.getLogger(HTTPDataSource.class);

    /** Connection Manager */
    private static MultiThreadedHttpConnectionManager 
        mConnectionMgr = null;

    /** max number of http retries */
    private static int mMaxRetries = 1;

    /** Whether or not to use the http11 . */
    private static boolean mUseHttp11 = true;

    /** Connection timeout millis (0 means default) */
    private static int mConnectionTimeout = 0;

    /** Timeout millis (0 means infinity) */
    private static int mTimeout = 0;

    /** Connection pool timeout in millis (0 means infinity) */
    private static int mConnectionPoolTimeout = 0;

    /** Max total connections. */
    private static int mMaxTotalConnections = 1000;

    /** Max connections per host. */
    private static int mMaxConnectionsPerHost = mMaxTotalConnections;

    /** Number of backend redirects we allow (potential security hole) */
    private static int mFollowRedirects = 0;

    {
        String useMultiThreadedConnectionMgr = LPS.getProperty("http.useConnectionPool", "true");

        if (Boolean.valueOf(useMultiThreadedConnectionMgr).booleanValue()) {
            mLogger.info("using connection pool");
            mConnectionMgr = new MultiThreadedHttpConnectionManager();
        } else {
            mLogger.info("not using connection pool");
        }
    
        // Parse multi connection properties anyway. May be used by AXIS. See
        // ResponderCache for details.
        {
            String maxConns = LPS.getProperty("http.maxConns", "1000");
            mMaxTotalConnections = Integer.parseInt(maxConns);
            if (mConnectionMgr != null) {
                mConnectionMgr.setMaxTotalConnections(mMaxTotalConnections);
            }
    
            maxConns = LPS.getProperty("http.maxConnsPerHost", maxConns);
            mMaxConnectionsPerHost = Integer.parseInt(maxConns);
            if (mConnectionMgr != null) {
                mConnectionMgr.setMaxConnectionsPerHost(mMaxConnectionsPerHost);
            }
        }

        String maxRetries = LPS.getProperty("http.maxBackendRetries", "1");
        mMaxRetries = Integer.parseInt(maxRetries);

        String followRedirects = LPS.getProperty("http.followRedirects", "0");
        mFollowRedirects = Integer.parseInt(followRedirects);

        String timeout = LPS.getProperty("http.backendTimeout", "10000");
        mTimeout = Integer.parseInt(timeout);

        timeout = LPS.getProperty("http.backendConnectionTimeout", timeout);
        mConnectionTimeout = Integer.parseInt(timeout);

        timeout = LPS.getProperty("http.connectionPoolTimeout", "0");
        mConnectionPoolTimeout = Integer.parseInt(timeout);

        String useHttp11 = LPS.getProperty("http.useHttp11", "true");
        mUseHttp11 = Boolean.valueOf(useHttp11).booleanValue();
        if (mUseHttp11) {
            mLogger.info("using HTTP 1.1");
        } else {
            mLogger.info("not using HTTP 1.1");
        }
    }


    /**
     * @return name of this datasource
     */
    public String name() {
        return "http";
    }

    /**
     * Do an HTTP Get/Post based on this request
     * 
     * @return the data from this request
     * @param app absolute pathnane to app file
     * @param req request in progress (possibly null)
     * @param since this is the timestamp on the
     * currently cached item; this time can be used as the datasource
     * sees fit (or ignored) in constructing the results.  If 
     * the value is -1, assume there is no currently cached item.
     */
    public Data getData(String app, HttpServletRequest req, HttpServletResponse res, long since) 
        throws DataSourceException, IOException {
        return getHTTPData(req, res, getURL(req), since);
    }

    public static Data getHTTPData(HttpServletRequest req, HttpServletResponse res, 
                                   String surl, long since) 
        throws DataSourceException, IOException {

        int tries = 1;

        // timeout msecs of time we're allowed in this routine
        // we must return or throw an exception.  0 means infinite.
        int timeout = mTimeout;
        if (req != null) {
            String timeoutParm = req.getParameter("timeout");
            if (timeoutParm != null) {
                timeout = Integer.parseInt(timeoutParm);
            }
        }

        long t1 = System.currentTimeMillis();
        long elapsed = 0;
        if (surl == null) {
            surl = getURL(req);
        }

        while(true) {
            String m = null;

            long tout;
            if (timeout > 0) {
                tout =  timeout - elapsed;
                if (tout <= 0) {
                    throw new InterruptedIOException(surl + " timed out");
                }
            } else {
                tout = 0;
            }

            try {
                HttpData data = getDataOnce(req, res, since, surl, 0, (int)tout);
                if (data.code >= 400) {
                    data.release();
                    throw new DataSourceException(errorMessage(data.code));
                }
                return data;
            } catch (HttpRecoverableException e) {
                // This type of exception should be retried.
                if (tries++ > mMaxRetries) {
                    throw new InterruptedIOException("too many retries, exception: " + e.getMessage());
                }
                mLogger.warn("retrying a recoverable exception: " + e.getMessage());
            } catch (HttpException e) {
                String msg = "HttpException: " + e.getMessage();
                throw new IOException("HttpException: " + e.getMessage());
            } catch (IOException e) {

                try {
                    Class ssle = Class.forName("javax.net.ssl.SSLException");
                    if (ssle.isAssignableFrom(e.getClass())) {
                        throw new DataSourceException("SSL exception: " + 
                                e.getMessage());
                    }
                } catch (ClassNotFoundException cfne) {
                }

                throw e;
            }

            long t2 = System.currentTimeMillis();
            elapsed = (t2 - t1);
        }
    }

    /**
     * convenience routine missing from http library
     */
    static boolean isRedirect(int rc) {
        return (rc == HttpStatus.SC_MOVED_PERMANENTLY || 
                rc == HttpStatus.SC_MOVED_TEMPORARILY || 
                rc == HttpStatus.SC_SEE_OTHER || 
                rc == HttpStatus.SC_TEMPORARY_REDIRECT);
    }
    /**
     * @param since last modified time to use
     * @param req
     * @param url if null, ignored
     * @param redirCount number of redirs we've done
     */
    public static HttpData getDataOnce(HttpServletRequest req,
         HttpServletResponse res, long since, String surl,
         int redirCount, int timeout)
         throws IOException, HttpException, DataSourceException, MalformedURLException {

        GetMethod request = null;
        HostConfiguration hcfg = new HostConfiguration();

        try {

            // TODO: [2002-01-09 bloch] cope with cache-control
            // response headers (no-store, no-cache, must-revalidate, 
            // proxy-revalidate).
            
            if (surl == null) {
                surl = getURL(req);
            }
            if (surl == null || surl.equals("")) {
                throw new MalformedURLException("url is empty or null");
            }
    
            String reqType   = "";
            String headers   = "";

            if (req != null) {
                reqType   = req.getParameter("reqtype");
                headers   = req.getParameter("headers");
            }
    
            boolean isPost = false;
    
            if (reqType != null && reqType.equals("POST")) {
                request = new LZPostMethod();
                isPost = true;
            } else {
                request = new LZGetMethod();
            }

            request.setHttp11(mUseHttp11);

            // Proxy the request headers
            if (req != null) {
                LZHttpUtils.proxyRequestHeaders(req, request);
            }
    
            // Set headers from query string
            if (headers != null && headers.length() > 0) {
                StringTokenizer st = new StringTokenizer(headers, "\n");
                while (st.hasMoreTokens()) {
                    String h = st.nextToken();
                    int i = h.indexOf(":");
                    if (i > -1) {
                        String n = h.substring(0, i);
                        String v = h.substring(i + 2, h.length());
                        request.setRequestHeader( n , v );
                        mLogger.debug("  setting header " + n + "=" + v);
                    }
                }
            }
    
            mLogger.debug("Parsing url");
            URI uri = LZHttpUtils.newURI(surl);
            try {
                hcfg.setHost(uri);
            } catch (Exception e) {
                throw new MalformedURLException("can't form uri from " + surl); 
            }
    
            // This gets us the url-encoded (escaped) path and query string
            String path = uri.getEscapedPath();
            String query = uri.getEscapedQuery();
            mLogger.debug("encoded path:  " + path);
            mLogger.debug("encoded query: " + query);
    
            // This call takes a decoded (unescaped) path
            request.setPath(path);
    
            boolean hasQuery = (query != null && query.length() > 0);
    
            if (isPost) {
                if (hasQuery) {
                    final String postbodyparam = "lzpostbody=";
                    if (query.startsWith(postbodyparam)) {
                        // Get the unescaped query string
                        String v = uri.getQuery().substring(postbodyparam.length());
                        ((LZPostMethod)request).setRequestBody(v);
                    } else {
                        StringTokenizer st = new StringTokenizer(query, "&");
                        while (st.hasMoreTokens()) {
                            String it = st.nextToken();
                            int i = it.indexOf("=");
                            if (i > 0) {
                                String n = it.substring(0, i);
                                String v = it.substring(i + 1, it.length());
                                // POST encodes values during request
                                ((PostMethod)request).addParameter(n, URLDecoder.decode(v));
                            } else {
                                mLogger.warn("ignoring bad token (missing '=' char) in query string: " + it);
                            }
                        }
                    }
                }
            } else {   
                // This call takes an encoded (escaped) query string
                request.setQueryString(query);
            }
    
            // Put in the If-Modified-Since headers
            if (since != -1) {
                String lms = LZHttpUtils.getDateString(since);
                request.setRequestHeader(LZHttpUtils.IF_MODIFIED_SINCE, lms); 
                mLogger.debug("proxying lms: " + lms);
            }
                
            mLogger.debug("setting up http client");
            HttpClient htc = null;
            if (mConnectionMgr != null) {
                htc = new HttpClient(mConnectionMgr);
            } else {
                htc = new HttpClient();
            }

            htc.setHostConfiguration(hcfg);
    
            // This is the data timeout
            mLogger.debug("timeout set to " + timeout);
            htc.setTimeout(timeout);
    
            // Set connection timeout the same
            htc.setConnectionTimeout(mConnectionTimeout);
    
            // Set timeout for getting a connection
            htc.setHttpConnectionFactoryTimeout(mConnectionPoolTimeout);
    
            // TODO: [2003-03-05 bloch] this should be more configurable (per app?)
            request.setFollowRedirects(mFollowRedirects > 0);
    
            long t1 = System.currentTimeMillis();
            mLogger.debug("starting remote request");
            int rc = htc.executeMethod(hcfg, request);
            String status = HttpStatus.getStatusText(rc);
            if (status == null) {
                status = "" + rc;
            }
            mLogger.debug("remote response status: " + status);
    
            HttpData data = null;
            if ( isRedirect(rc) && mFollowRedirects > redirCount ) {
                String loc = request.getResponseHeader("Location").toString();
                String hostURI = loc.substring(loc.indexOf(": ") + 2, loc.length() ) ;
                mLogger.info("Following URL from redirect: " + hostURI);
                long t2 = System.currentTimeMillis();
                if (timeout > 0) {
                    timeout -= (t2 - t1);
                    if (timeout < 0) {
                        throw new InterruptedIOException(surl + " timed out after redirecting to " + loc);
                    }
                }

                data = getDataOnce(req, res, since, hostURI, redirCount++, timeout);
            } else {
                data = new HttpData(request, rc);
            }

            if (req != null && res != null) {
                // proxy response headers
                LZHttpUtils.proxyResponseHeaders(request, res, req.isSecure());
            }

            return data;

        } catch (HttpConnection.ConnectionTimeoutException ce) {
            // Transduce to an InterrupedIOException, since lps takes these to be timeouts.
            if (request != null) {
                request.releaseConnection();
            }
            throw new InterruptedIOException("connecting to " + hcfg.getHost() + ":" + hcfg.getPort() + 
                        " timed out beyond " + mConnectionTimeout + " msecs.");
        } catch (HttpRecoverableException hre) {
            if (request != null) {
                request.releaseConnection();
            }
            throw hre;
        } catch (HttpException e) {
            if (request != null) {
                request.releaseConnection();
            }
            throw e;
        } catch (IOException ie) {
            if (request != null) {
                request.releaseConnection();
            }
            throw ie;
        } catch (RuntimeException e) {
           if (request != null) {
               request.releaseConnection();
           }
           throw e;
        }
    }
    
    /**
     * utility
     */
    private static String errorMessage(int code) {
        return "HTTP Status code: " + code + ":" + 
                HttpStatus.getStatusText(code);
    }

    /**
     * A class for holding on to results of an Http fetch.
     *
     * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
     */

    public static class HttpData extends Data {

        /** response code */
        public final int code;
    
        /** Http request */
        public final GetMethod  request;
    
        private PatternMatcher pMatcher = new Perl5Matcher();
        private static final Pattern charsetPattern;
        private static final Pattern declEncodingPattern;
        static {
            try {
                Perl5Compiler compiler = new Perl5Compiler();
                charsetPattern = compiler.compile(";charset=([^ ]*)");
                declEncodingPattern =
                    compiler.compile("[ \t\r\n]*<[?]xml .*encoding=[\"']([^ \"']*)[\"'] .*[?]>");
            } catch (MalformedPatternException e) {
                throw new RuntimeException(e.getMessage());
            }
        }

        /** 
         * @param r filled request
         * @param c response code
         */
        public HttpData(GetMethod r, int c) {
            code = c;
            request = r;
        }
    
        /**
         * @return true if the data was "not modified"
         */
        public boolean notModified() {
            return code == HttpServletResponse.SC_NOT_MODIFIED;
        }
    
        /**
         * @return the lastModified time of the data
         */
        public long lastModified() {
    
            Header lastModifiedHdr = request.getResponseHeader(
                LZHttpUtils.LAST_MODIFIED);
                        
            if (lastModifiedHdr != null) {
                String lm = lastModifiedHdr.getValue();
                mLogger.debug("data with last modified at " + lm);
                long l = LZHttpUtils.getDate(lm);
                // Truncate to nearest second
                return ((l)/1000L) * 1000L;
            } else {
                mLogger.debug("data has no mod time");
                return -1;
            }
        }
    
        /**
         * append response headers
         */
        public void appendResponseHeadersAsXML(StringBuffer xmlResponse) {
    
            Header[] hedz = request.getResponseHeaders();
            for (int i = 0; i < hedz.length; i++) {
                String name = hedz[i].getName();
                if (LZHttpUtils.allowForward(name, null)) {
                    xmlResponse.append("<header name=\""+ XMLUtils.escapeXml( name ) + "\" " 
                                     + "value=\""       + XMLUtils.escapeXml( hedz[i].getValue() ) + "\" />");
                }
            }
        }
    
        /**
         * release any resources associated with this data
         */
        public void release() {
            request.releaseConnection();
        }

        /**
         * @return mime type 
         */
        public String getMimeType() {
            Header hdr = request.getResponseHeader(LZHttpUtils.CONTENT_TYPE);
            String contentType = "";
            if (hdr != null) {
                contentType = hdr.getValue();
            }
            mLogger.debug("content type: " + contentType);
            return contentType;
        }

        /**
         * @return string
         */
        public String getAsString() throws IOException {
            byte rawbytes[] = request.getResponseBody();
            if (rawbytes == null || rawbytes.length == 0) {
                throw new InterruptedIOException("null http response body");
            }
            String encoding = "UTF-8";
            String content = getMimeType();
            // search for ;charset=XXXX in Content-Type header
            if (pMatcher.matches(content, charsetPattern)) {
                encoding = pMatcher.getMatch().group(1);
            }
            // search for 'encoding' attribute in xml declaration, e.g.,
            // <?xml version="1.0" encoding="ISO-8859-1" standalone="no"?>
            
            String decl = getXMLDeclaration(rawbytes);
            if (pMatcher.matches(decl, declEncodingPattern)) {
                encoding = pMatcher.getMatch().group(1);
                //mLogger.debug("parsed data encoding: " + encoding);
            }
            
            return new String(rawbytes, encoding);
        }

        /** Returns the first non-whitespace line.
         *
         */
        String getXMLDeclaration(byte buf[]) {
            String str = new String(buf);
            BufferedReader br = new BufferedReader(new StringReader(str));
            String line;
            while (true) {
                try { line = br.readLine(); } catch (IOException e) { return ""; }
                if (line == null) {
                    return "";
                }
                if (line.length() == 0) continue;
                if (line.startsWith("<?xml ")) {
                    return line;
                } else {
                    return "";
                }
            }
        }

        /**
         * @return input stream
         */
        public InputStream getInputStream() throws IOException {
            InputStream str = request.getResponseBodyAsStream();
            if (str == null) {
                throw new IOException("http response body is null");
            }
            return str;
        }

        /**
         * @return size, if known
         */
        public long size() {
            Header hdr = request.getResponseHeader(LZHttpUtils.CONTENT_LENGTH);
            if (hdr != null) {
                String contentLength = hdr.getValue();
                if (contentLength != null) {
                    mLogger.debug("content length: " + contentLength);
                    int cl = Integer.parseInt(contentLength);
                    return cl;
                }
            }
            return -1;
        }
    }

    public static int getConnectionPoolTimeout() {
        return mConnectionPoolTimeout;
    }

    public static int getMaxTotalConnections() {
        return mMaxTotalConnections;
    }

    public static int getMaxConnectionsPerHost() {
        return mMaxConnectionsPerHost;
    }

    public static void main(String args[]) {

        HTTPDataSource ds = new HTTPDataSource();

        try {
            if (args.length != 1 && args.length != 2) {
                throw new Exception("Need an url"); 
            }
            String surl = args[0];
            FileOutputStream out = null;
            if (args.length == 2) {
                out = new FileOutputStream(args[1]);
            }
            System.out.println("url is " + surl);

            HttpData data = ds.getDataOnce(null, null, -1, surl, 0, 0);

            System.out.println("Response code: " + data.code);

            if (out != null) {
                FileUtils.send(data.getInputStream(), out);
            }

            data.release();
            
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
