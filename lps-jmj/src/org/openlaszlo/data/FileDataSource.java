/* *****************************************************************************
 * FileDataSource.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import java.io.*;
import java.net.URL;
import java.net.MalformedURLException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.log4j.Logger;
import org.openlaszlo.utils.ChainedException;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.media.MimeType;

/**
 * File Transport
 */
public class FileDataSource extends DataSource
{
    private static Logger mLogger  = Logger.getLogger(FileDataSource.class);

    /**
     * @return name
     */
    public String name() {
        return "file";
    }

    /**
     * @return the data from this request
     * @param app absolute pathnane to app file
     * @param req request in progress
     * @param lastModifiedTime this is the timestamp on the
     * currently cached item; this time can be used as the datasource
     * sees fit (or ignored) in constructing the results.  If 
     * the value is -1, assume there is no currently cached item.
     */
    public Data getData(String app, HttpServletRequest req, 
                        HttpServletResponse res, long lastModifiedTime) 
        throws IOException, DataSourceException {
        return getFileData(app, req, res, null, lastModifiedTime);
    }

    static public Data getFileData(String app, HttpServletRequest req, 
                                   HttpServletResponse res, String urlStr, 
                                   long lastModifiedTime) 
        throws IOException, DataSourceException {

        if (urlStr == null) {
            urlStr = DataSource.getURL(req);
        }

        URL url = new URL(urlStr);

        String protocol = url.getProtocol();

        if ( protocol == null || ! protocol.equals("file")){
            mLogger.error( " bad protocol for " + url );
            throw new DataSourceException("protocol " + protocol + "is not 'file:' ");
        }

        String filename = url.getFile();
        mLogger.debug("filename " + filename);

        if ( filename == null || filename.equals("") ) {
            throw new DataSourceException("empty filename");
        }

        // For relative urls, add app path before 
        if (filename.charAt(0) != '/') {
            mLogger.debug("app " + app);
            String appdir = app.substring(0, 
                            app.lastIndexOf(File.separatorChar) + 1);
            mLogger.debug("appdir " + appdir);
            filename = appdir + filename;
            mLogger.debug("filename " + filename);
        }

        // Cope with Windows wackiness.
        if (File.separatorChar == '\\') {
            while (filename.startsWith("/")) {
                filename = filename.substring(1);
            }
            filename = filename.replace('/', '\\');
        }

        FileData data = new FileData(filename, lastModifiedTime);

        // proxy date header
        res.setDateHeader(LZHttpUtils.LAST_MODIFIED, data.lastModified());

        return data;
    }

    /**
     * A class for holding on to results of a File fetch.
     *
     * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
     */

    public static class FileData extends Data {

        /** response code */
        public FileInputStream str = null;

        /** file */
        public final File file;

        /** lastModifiedTime from request (or -1)*/
        public final long lastModifiedTime;

        /** 
         * @param f file
         */
        public FileData(String filename, long lm) throws IOException {
            mLogger.debug("filename " + filename);
            File f = new File(filename);
            if (f == null) {
                throw new IOException("can't construct file");
            } else if (!f.exists()) {
                throw new IOException(filename + " doesn't exist.");
            } else if (!f.canRead()) {
                throw new IOException("can't read " + filename);
            } 
            lastModifiedTime = lm;
            file = f;
        }
    
        /**
         * @return size of file
         */
        public long size() {
            try {
                return FileUtils.fileSize(file);
            } catch (Exception e) {
                throw new ChainedException(e);
            }
        }
    
        /**
         * @return true if the data was "not modified"
         * compared to the cached lastModified time.
         */
        public boolean notModified() {

            long l = lastModified();
            if (l == -1) {
                return false;
            }

            return (l <= lastModifiedTime);
        }
    
        /**
         * @return the lastModified time of the data
         */
        public long lastModified() {

            long l = file.lastModified();
            if (l == 0) {
                l = -1;
            }
            // Truncate to nearest second
            l = ((l)/1000L) * 1000L;
            mLogger.debug("lm is " + l);
            return l;
        }
    
        /**
         * append response headers
         */
        public void appendResponseHeadersAsXML(StringBuffer xmlResponse) {
            // TODO: [2003-04-17 bloch] should this be a string or a long?
            xmlResponse.append("<header name=\"Last-Modified\" " 
                             + "value=\""  + lastModified() + "\" />");
        }
    
        /**
         * release any resources associated with this data
         */
        public synchronized void release() {
            try {
                if (str != null) {
                    str.close();
                    str = null;
                }
            } catch (Exception e) {
                mLogger.warn("ignoring exception while closing stream: ", e);
            }
        }

        /**
         * @return mime type 
         */
        public String getMimeType() {
            return MimeType.fromExtension(file.getPath());
        }

        /**
         * @return input stream
         */
        public synchronized InputStream getInputStream() throws IOException {
            if (str == null) {
                str = new FileInputStream(file);
            }
            return str;
        }

        /**
         * @return string
         */
        public String getAsString() throws IOException  {
            return FileUtils.readFileString(file);
        }
    }
}
