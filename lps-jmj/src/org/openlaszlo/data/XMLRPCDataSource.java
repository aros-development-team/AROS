/* ****************************************************************************
 * XMLRPCDataSource.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import java.io.*;
import java.util.*;
import java.net.MalformedURLException;
import javax.servlet.http.*;
import org.apache.xmlrpc.*;
import org.openlaszlo.server.LPS;
import org.openlaszlo.xml.internal.*;
import org.openlaszlo.media.MimeType;
import org.apache.log4j.*;

/**
 *
 */
public class XMLRPCDataSource extends DataSource
{
    private static Logger mLogger  = Logger.getLogger(XMLRPCDataSource.class);

    XmlRpcServer xmlrpc = new XmlRpcServer();

    /**
     * @return unique name of this data source
     */
    public String name() 
    {
        return "xmlrpc";
    }

    /**
     * Sends system information to client.
     * 
     * @throws DataSourceException if there was a problem retrieving or sending
     * the data.
     */
    public Data getData(String app, HttpServletRequest req, 
                        HttpServletResponse res, long lastModifiedTime)
        throws DataSourceException {
        mLogger.debug("getData");

        int swfversion = LPS.getSWFVersionNum(req);
        try {
            if (! req.getMethod().equals("POST"))
                return compileFault("Remote request must be POST", swfversion);

            String url = getHTTPURL(getURL(req));
            if (url == null) {
                return compileFault("invalid url specified: " + url, swfversion);
            }

            String postbody = req.getParameter("lzpostbody");
            if (postbody != null) {
                url += "?lzpostbody=" + postbody;
            }

            Data data = HTTPDataSource.getHTTPData(req, res, url, -1);
            return new XMLRPCData(data.getAsString().getBytes(), swfversion);

        } catch (Exception e) {
            return compileFault(e, swfversion);
        }
    }

    String getHTTPURL(String url) {
        if (url != null && url.startsWith("xmlrpc://")) 
            return "http" + url.substring(6);
        return null;
    }


    /**
     * Compile fault exception message.
     */
    Data compileFault(Exception e, int swfversion) {
        mLogger.error("compileFault", e);
        return compileFault(e.getMessage(), swfversion);
    }

    /**
     * Compile fault response.
     */
    Data compileFault(String mesg, int swfversion) {
        mLogger.error("compileFault mesg: " + mesg);
        try {
            byte[] d = XMLRPCCompiler.compileFault(XMLUtils.escapeXml(mesg), 
                                                   swfversion);
            return new XMLRPCData().setResult(d);
        } catch (Exception e) {
            mLogger.error("Exception", e);
            // this is an error since we can't build a fault response
            throw new Error(e.getMessage());
        }
    }

    /**
     * A data object to hold an xmlrpc response.
     */
    public class XMLRPCData extends Data 
    {
        byte[] mResult;

        public XMLRPCData() { }

        public XMLRPCData(byte[] result, int swfversion) 
            throws IOException {
            InputStreamReader reader = new InputStreamReader(new ByteArrayInputStream(result));
            mResult = XMLRPCCompiler.compile(reader, result.length, swfversion);
        }

        public String getMimeType() {
            return MimeType.SWF;
        }


        public XMLRPCData setResult(byte[] result) {
            mResult = result;
            return this;
        }

        /**
         * @return the encoded XML
         */
        public InputStream getInputStream() 
            throws IOException {
            return new ByteArrayInputStream(mResult);
        }

        public long size() {
            return mResult.length;
        }
    }
}
