/****************************************************************************
 * DataSource.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import java.io.*;
import java.net.MalformedURLException;
import java.util.HashMap;
import java.util.StringTokenizer;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.commons.httpclient.URI;
import org.apache.commons.httpclient.URIException;
import org.apache.log4j.Logger;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.media.MimeType;

/**
 * Base class for server side LZX data/media sources.
 */
abstract public class DataSource
{
    private static Logger mLogger  = Logger.getLogger(DataSource.class);

    /**
     * Get unique name of this data source.
     *
     * @return unique name of this data source.
     */
    public abstract String name();

    /**
     * Get the data for this request.
     *
     * @return the data for this request.
     * @param app absolute pathnane to app file.
     * @param req request in progress.
     * @param res response object.
     * @param lastModifiedTime this is the timestamp on the currently cached
     * item; this time can be used as the datasource sees fit (or ignored) in
     * constructing the results.  If the value is -1, assume there is no
     * currently cached item.
     *
     * @throws DataSourceException if there was a problem with the data source.
     * @throws IOException if there was a problem retrieving the data.
     * @throws InterrupedIOException if there was a timeout retrieving the data.
     */
    public abstract Data getData(String app, HttpServletRequest req, 
                                 HttpServletResponse res, long lastModifiedTime)
        throws InterruptedIOException, IOException, DataSourceException;


    /**
     * Determine the datasource from the incoming request,
     * get the data, convert it to SWF, and write it out
     * to the given response.
     *
     * @param app absolute path name to app requesting data
     * @param req request
     * @param res response
     * @param converter converter to use
     * @throws DataSourceException if the data source encounters an error
     * @throws ConversionException if the conversion to SWF returns an error
     * @throws IOException if there is an IO error
     */
    final public void getAsSWF(
            String app,
            HttpServletRequest req, 
            HttpServletResponse res, 
            Converter converter) 
        throws DataSourceException, ConversionException, IOException {

        Data data = null;
        InputStream input = null;
        OutputStream output = null;
        long size = -1;
        long since = -1;

        // Check to see if client side caching is on
        boolean doClientCache = isClientCacheable(req);
        if (doClientCache) {
            String hdr = req.getHeader(LZHttpUtils.IF_MODIFIED_SINCE);
            if (hdr != null) {
               mLogger.debug("req last modified time: " + hdr);
               since = LZHttpUtils.getDate(hdr);
            }
        }

        mLogger.info("requesting URL: '" + DataSource.getURL(req) + "'");
        try {
            data = getData(app, req, res, since);
    
            if (data.notModified()) {
                mLogger.info("NOT_MODIFIED");
                res.setStatus(HttpServletResponse.SC_NOT_MODIFIED);
                return;
            }

            mLogger.debug("got data");

            if (!data.getMimeType().equals(MimeType.SWF)) {

                input = converter.convertToSWF(data, req, res);
                size = input.available();
                mLogger.debug("converted to " + size + " bytes of SWF");
                // FIXME: [2003-09-22 bloch] input.available not realiable

                String enc = converter.chooseEncoding(req);
                if (enc != null && !enc.equals("")) {
                    input = converter.encode(req, input, enc);
                    res.setHeader(LZHttpUtils.CONTENT_ENCODING, enc);
                    size = input.available();
                } 

            } else {
                mLogger.debug("remote content was SWF");
                input = data.getInputStream();
                size = data.size();
            }

            if (size != -1) {
                mLogger.debug("setting content length: " + size);
                res.setContentLength((int)size);
            }

            if (doClientCache) {
                long t = data.lastModified();
                if (t != -1) {
                    res.setDateHeader(LZHttpUtils.LAST_MODIFIED, t);
                }
            } else {
                LZHttpUtils.noStore(res);
            }

            try {
                output = res.getOutputStream();
                long n = FileUtils.sendToStream(input, output);
                mLogger.info(n + " bytes sent");
            } catch (FileUtils.StreamWritingException e) {
                mLogger.warn("StreamWritingException while responding: " + 
                        e.getMessage());
            }

        } finally {
            if (data != null) {
                data.release();
            }
            FileUtils.close(output);
            FileUtils.close(input);
        } 
    }

    /**
     * Determine the datasource from the incoming request,
     * get the data, and write it out
     * to the given response.
     *
     * @param app pathname to app on disk
     * @param req http request
     * @param res http response
     * @throws DataSourceException if the data source encounters an error
     * @throws IOException if there is an IO error
     */
    final public void get(
            String app,
            HttpServletRequest req, 
            HttpServletResponse res)
        throws DataSourceException, IOException {

        Data data = null;
        InputStream input = null;
        OutputStream output = null;
        long size = -1;

        mLogger.info("requesting URL: '" + DataSource.getURL(req) + "'");

        try {

            data = getData(app, req, res, -1);
    
            input = data.getInputStream();
            // FIXME: [2003-04-26 bloch] Reenable content-length header at some point.
            // Client will sometimes bail in this situation:
            // 1) set content length and bit client interrupts/closes socket before
            // all content comes down.
            // 2) next time the urls is hit, the client requests, server responds,
            // but nothing shows in the browser.  This seems to happen only
            // when the content is coming from 
            // size = data.size();
            size = -1;

            if (size != -1) {
                mLogger.debug("setting content length: " + size);
                res.setContentLength((int)size);
            }

            // Hopefully back end tells the truth
            res.setContentType(data.getMimeType());

            output = res.getOutputStream();
            long n = FileUtils.send(input, output);
            mLogger.info(n + " bytes sent");

        } finally {
            if (data != null) {
                data.release();
            }
            FileUtils.close(output);
            FileUtils.close(input);
        } 
    }


    /**
     * Return true if the request is marked cacheable on the client
     */
    final static public boolean isClientCacheable(HttpServletRequest req) {
        String str = req.getParameter("ccache");
        return (str != null && str.equals("true"));
    }


    /**
     * Get the URL query paramter for this request.
     *
     * @param req servlet request object to retrieve URL parameter.
     * @return the 'URL' for the request.
     * @throws MalformedURLException if the url parameter is missing from the request.
     */
    final static public String getURL(HttpServletRequest req) throws 
        MalformedURLException {

        String surl = req.getParameter("url");
        if (surl == null) {
            throw new MalformedURLException("'url' parameter missing from request");
        }
        mLogger.debug("'url' is " + surl);
        return LZHttpUtils.modifyWEBAPP(req, surl);
    }


    /**
     * Utility function to get a hash map of query parameters from 
     * an url string.
     *
     * @param url string containing an URL to parse query parameters.
     * @return hash map of query parameters, or an empty hash map if no query
     * parameters exist.
     */
    final static public HashMap getQueryString(String url) 
    {
        HashMap map = new HashMap();
        try {
            URI uri = new URI(url.toCharArray());
            String query = uri.getQuery();
            if (query != null) {
                StringTokenizer st = new StringTokenizer(query, "&");
                while (st.hasMoreTokens()) {
                    String param = st.nextToken();
                    int i = param.indexOf("=");
                    if (i != -1) {
                        String k = param.substring(0, i);
                        String v = param.substring(i+1, param.length());
                        map.put(k, v);
                    }
                }
            }
        } catch (URIException e) {
            mLogger.debug("URIException: " + e.getMessage());
        } 
        return map;
    }
}
