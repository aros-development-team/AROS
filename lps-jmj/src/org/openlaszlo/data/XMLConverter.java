/* Todo
+ make search for null bytes optional

*/


/* *****************************************************************************
 * XMLConverter.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import java.io.*;
import java.util.Properties;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.apache.log4j.*;

import org.openlaszlo.xml.internal.DataCompiler;
import org.openlaszlo.xml.internal.DataCompilerException;
import org.openlaszlo.utils.ContentEncoding;

import org.openlaszlo.media.MimeType;
import org.openlaszlo.server.LPS;

/**
 * XML Converter
 *
 */
public class XMLConverter extends Converter {
    
    private static Logger mLogger  = Logger.getLogger(XMLConverter.class);

    /** Data compiler */
    private DataCompiler mCompiler = new DataCompiler();

    /**
     * Convert incoming XML to SWF, adding some HTTP info as well.
     */
    public InputStream convertToSWF(Data data, HttpServletRequest req,
                                    HttpServletResponse res)
       throws ConversionException, IOException {

        String mimeType = data.getMimeType();

        if (mimeType != null && mimeType.equals(MimeType.SWF)) {
            return data.getInputStream();
        }

        if (mimeType == null || !mimeType.equals(MimeType.XML)) {
            mLogger.warn("back-end mime-type is " + mimeType + ", treating as text/xml" );
        }

        String body = data.getAsString();

        // TODO: [2003-04-26 bloch] perhaps there should be a config
        // to turn this check on/off.
        //
        // Strip out possible null characters from input.  Null
        // characters are bad because they get passed by JGenerator
        // through to Flash and really screw up zero-terminated string
        // constants, causing corrupted Flash byte codes.
        if ("true".equals(LPS.getProperty("data.removenulls", "false"))) {
            StringBuffer buf = new StringBuffer();
            int nchars = body.length();
            for (int i = 0; i < nchars; i++) {
                char ch = body.charAt(i);
                if (ch != 0) {
                    buf.append(ch);
                }
            }
            body = buf.toString();
        }

        // Get headers
        String sendheaders = req.getParameter("sendheaders");
        StringBuffer headerbuf = new StringBuffer();
        headerbuf.append("<headers>\n");
        if (sendheaders == null || sendheaders.equals("true") ) {
            data.appendResponseHeadersAsXML(headerbuf);
        }
        headerbuf.append("</headers>");
        String headers = headerbuf.toString();

        if (mLogger.isDebugEnabled()) {
            mLogger.info("Output:" + body.length());
            mLogger.info("Output:\n" + body);
            mLogger.info("Output Headers:" + headers.length());
            mLogger.info("Output Headers:\n" + headers);
        }

        boolean trimWhitespace = "true".equals(req.getParameter("trimwhitespace"));

        boolean compress = "true".equals(req.getParameter("compress"));

        try {
            return mCompiler.compile(body, headers, LPS.getSWFVersionNum(req), 
                                     true, trimWhitespace, compress);

        } catch (DataCompilerException dce) {
            throw new ConversionException(dce.getMessage());
        } catch (IOException ie) {
            throw new ConversionException(ie.getMessage());
        }
    }

    /**
     * @return the encoding that should be used when responding
     * to this request or null for no encoding.  For now, the only
     * acceptable values besides null are "gzip" and "deflate".
     */
    public String chooseEncoding(HttpServletRequest req) {

        String e = req.getParameter("enc");
        if (e == null || e.equals("false")) {
            return null;
        }

        String enc = ContentEncoding.chooseEncoding(req);
        mLogger.debug("Encoding: " + enc);
        return enc;
    }

}
