/******************************************************************************
 * RequestCache.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.cache;

import javax.servlet.http.*;
import java.io.InputStream;
import java.io.IOException;
import java.io.File;
import java.io.Serializable;
import java.io.OutputStream;
import java.util.Properties;
import java.net.MalformedURLException;
import org.openlaszlo.data.*;
import org.openlaszlo.utils.LZHttpUtils;
import org.openlaszlo.utils.FileUtils;
import org.apache.log4j.*;

/**
 * A class for maintaining a disk-backed cache of HTTP requests.
 *
 * The main entry point is the <code>getAsSWF</code> method.
 * Given a specific HTTP request and response, the item returns
 * an InputStream for a possibly-converted SWF that represents
 * the item requested.  This method may return NULL, indicating
 * that a special HTTP status code has been stuck in the response.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public abstract class RequestCache extends Cache {

    /** logger */
    private static Logger mLogger = Logger.getLogger(Cache.class);

    /** converter */
    protected Converter mConverter;

    /**
     * Creates a new <code>RequestCache</code> instance.
     *
     * @param cacheDirectory a <code>File</code> naming a directory
     * where cache files should be kept
     * @param dataSource back end data source for the cache
     */
    public RequestCache(String name, File cacheDirectory, Converter converter,
            Properties props)
        throws IOException {

        super(name, cacheDirectory, props);
        mConverter = converter;
    }

    /**
     * @return a serializable cache key for the given request
     */
    public Serializable getKey(HttpServletRequest req) 
        throws MalformedURLException {

        // This is a nice, easy to read key
        String enc = mConverter.chooseEncoding(req);
        if (enc == null) 
            enc = "";
        StringBuffer key = new StringBuffer();
        key.append(DataSource.getURL(req));
        // note: space not allowed in URLS so it's good to 
        // use here as a separator to distinguish encoded keys
        key.append(" ");
        key.append(enc);
        return key.toString();
    }

    /**
     * Using the given datasource from the incoming request,
     * get the data if it's out of data from the cache, convert it
     * to SWF, and write it out to the given response.
     * 
     * @param app absolute path name to app
     * @param req servlet request that triggered the get
     * @param res servlet response to fill out
     * @param dataSource source of data
     * @throws IOException if there's an IOException while creating
     * the response
     * @throws DataSourceException if there's an error getting the data
     * @throws ConversionException if there's an error convering the data
     */
    public void getAsSWF(
            String app,
            HttpServletRequest req, 
            HttpServletResponse res,
            DataSource dataSource)
        throws IOException, DataSourceException, ConversionException {

        // Skip the cache if it's 0 size
        if (getMaxMemSize() == 0 && getMaxDiskSize() == 0) {
            dataSource.getAsSWF(app, req, res, mConverter);
            return;
        }

        StringBuffer kb = new StringBuffer();
        kb.append(dataSource.name());
        kb.append(" ");
        String rk = getKey(req).toString();
        kb.append(rk);
        String key = kb.toString();
        String enc = mConverter.chooseEncoding(req);

        mLogger.info("requesting '" + rk + "'");
        if (enc != null) {
            mLogger.debug("encoding " + enc);
        }

        Item item = null;
        InputStream input = null;
        OutputStream output = null;
        try {
            item = findItem(key, enc, /* lock it and leave active */ true);

            // Get an input stream for a SWF version of this item;
            try {
                input = getItemStreamAsSWF(item, app, req, res, dataSource);
            } finally {
                // Unlock it while we send out the response 
                // FIXME: [2003-08-27 bloch] there is a slight race here since the source 
                // of the stream could be removed before we are finished; this would
                // be rare since we're MRU now.
                item.unlock();
            }

            // Send out the response
            if (input != null) {
                try {
                    output = res.getOutputStream();
                    long n = FileUtils.sendToStream(input, output);
                    mLogger.info(n + " bytes sent");
                } catch (FileUtils.StreamWritingException e) {
                    mLogger.warn("StreamWritingException while responding: " 
                            + e.getMessage());
                }
            } else {
                mLogger.info("Cache responding with NOT_MODIFIED");
            }
        } finally {
            FileUtils.close(output);
            FileUtils.close(input);

            // If there's an item, unlock it and update the cache
            if (item != null) {
                updateCacheAndDeactivateItem(item);
            }
        }
    }

    /**
     * @return true if the request is cacheable.
     *
     * If response headers are to be sentback, then the request is 
     * not cacheable on the server.
     *
     * If req headers are sent, then the request is not cacheable
     * on the server.
     *
     * If the cache parameter is present and set to true, 
     * the request is cacheable.  Otherwise it's not.
     */
    public boolean isCacheable(HttpServletRequest req) {
        String hds = req.getParameter("sendheaders");
        if (hds != null) {
            if (hds.equals("true")) {
                return false;
            }
        }
        hds = req.getParameter("headers");
        if (hds != null) {
            return false;
        }
        String c = req.getParameter("cache");
        if (c == null)
            return false;
        return c.equals("true");
    }

    /**
     * Return the converter for this cache
     */
    public Converter getConverter() {
        return mConverter;
    }

    /**
     * Get the item if it's been updated since the given time.  Item
     * must be locked when you call this.
     * @return null if the url hasn't been modified since the given time
     * or an input stream that can be used to read the item's content
     * as SWF.
     */
    InputStream getItemStreamAsSWF(Item item,
            String app,
            HttpServletRequest req, 
            HttpServletResponse res,
            DataSource dataSource)
        throws IOException, DataSourceException, ConversionException {

        long ifModifiedSince = -1;
        long lastModified = -1;

        CachedInfo info = item.getInfo();
        String enc = info.getEncoding();

        String hdr = req.getHeader(LZHttpUtils.IF_MODIFIED_SINCE);
        if (hdr != null) {
           mLogger.debug("req last modified time: " + hdr);
           lastModified = LZHttpUtils.getDate(hdr);
        }

        boolean doClientCache = DataSource.isClientCacheable(req); 

        // FIXME[2003-05-21 bloch]:  Max and I worked through the 
        // logic in this comment below and it seemed correct
        // but as I paste it in here it's missing an else clause...
        //
        // If (there is an entry in the cache)
        //     If there's no timestamp in the request 
        //        Use the timestamp from the cache.
        //     else 
        //        If request time is <= cache time
        //            use cache time
        // else (no entry in the cache)
        //        use -1
        //
        // Max prefers the following code to implement but I'm leaving the extant code
        // because it's too close to release for me to be making changes
        // like that.  Here's the code Max, liked:
        //
        // if (info.getLastModified() != -1) {
        //     if (ifModifiedSince == -1) {
        //         ifModifiedSince = info.getLastModified();
        //     } else {
        //         if (ifModifiedSince <= info.getLastModified()) {
        //             ifModifiedSince = info.getLastModified();
        //         }
        //     }
        // } else {
        //     ifModifiedSince = -1;
        // }
        //
        //
        // The code below has existed for a while and actually implements the logic above
        // in a slightly convoluted way because of the special dual meaning of -1; it means no 
        // last modified time at all when coming from data or the cache and it 
        // means get fresh data when making a data request (as the 3rd parameter to 
        // the getData() method below.

        if (info.getLastModified() > lastModified || info.getLastModified() == -1) {
            ifModifiedSince = info.getLastModified();
            mLogger.debug("using cached last modified time: " + ifModifiedSince);
        } else {
            ifModifiedSince = lastModified;
            mLogger.debug("using req last modified time: " + ifModifiedSince);
        }

        Data data = null;

        try {

            try {
                data = dataSource.getData(app, req, res, ifModifiedSince);
            } catch (DataSourceException e) {
                // When we get an error from the back end,
                // we must nuke the item from the cache.
                item.markDirty();
                throw e;
            } catch (IOException e) {
                item.markDirty();
                throw e;
            } catch (RuntimeException e) {
                item.markDirty();
                throw e;
            }

            if (data.notModified()) {

                mLogger.debug("Remote response: NOT_MODIFIED");
                if (lastModified >= info.getLastModified() && doClientCache) {
                    res.setStatus(HttpServletResponse.SC_NOT_MODIFIED);
                    return null;
                }

                if (item.validForData(data)) {

                    if (enc != null) {
                        res.setHeader(LZHttpUtils.CONTENT_ENCODING, enc);
                    }
                    if (doClientCache) {
                        long l = info.getLastModified();
                        if (l != -1) {
                            res.setDateHeader(LZHttpUtils.LAST_MODIFIED, l);
                        }
                    } else {
                        LZHttpUtils.noStore(res);
                    }
                    res.setContentLength( (int) info.getSize());
                    return item.getStream();
                }
            }

            mLogger.debug("path name: " + item.getPathName());

            // We know the cached item is dirty
            item.markDirty();

            // Mark the info with the new last modified time
            info.setLastModified(data.lastModified());

            // Convert the data to SWF
            InputStream input = mConverter.convertToSWF(data, req, res);
            // TODO: [2003-09-22 bloch] add content length when
            // converter api provides this info; input.available() is not reliable
            // TODO: [2003-09-22 bloch] could handle case when conversion is a no-op from swf,
            // without the above

            // Update the item with the data
            try {
                item.update(input);
                item.updateInfo();
                item.markClean();
            } finally {
                FileUtils.close(input);
            }

            // FIXME: [2003-03-13 bloch] hope that no one
            // removes the file before we're done with this
            // input stream.  This would happen only
            // when the cache is full and small; a rare
            // case in production.  
            
            InputStream str = item.getStream();

            if (enc != null) {
                res.setHeader(LZHttpUtils.CONTENT_ENCODING, enc);
            }

            if (doClientCache) {
                long l = info.getLastModified();
                if (l != -1) {
                    res.setDateHeader(LZHttpUtils.LAST_MODIFIED, l);
                }
                                      
            } else {
                LZHttpUtils.noStore(res);
            }

            res.setContentLength( (int) info.getSize());

            return str;
            
        } finally {
            if (data != null) {
                data.release();
            }
        } 
    }
}
