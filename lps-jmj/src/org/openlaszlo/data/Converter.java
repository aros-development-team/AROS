/* *****************************************************************************
 * Converter.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import java.io.InputStream;
import java.io.File;
import java.io.IOException;
import java.io.FileNotFoundException;

import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.utils.TempFileInputStream;
import org.openlaszlo.utils.ChainedException;

import org.apache.log4j.Logger;

/**
 * Base class for data conversion and encoding
 */
public abstract class Converter {

    private static Logger mLogger = Logger.getLogger(Converter.class);

    /** 
     * @return an input stream that can read the
     * converted data that came from the given request
     * as SWF.
     */
    public abstract InputStream convertToSWF(Data data, 
            HttpServletRequest req, HttpServletResponse res)
       throws ConversionException, IOException;

    /**
     * @return the HTTP content-encoding that should be used when responding
     * to this request or null for no encoding.  For now, the only
     * acceptable values besides null are "gzip" and "deflate".
     */
    public abstract String chooseEncoding(HttpServletRequest req);

    /**
     * @return stream of encoded data
     * @param in input stream to read
     * @param enc encoding to use
     * @param req request
     */
    public InputStream encode(HttpServletRequest req, InputStream in, 
            String enc) throws IOException {

        if (enc != null && !enc.equals("")) {
            File tempFile = File.createTempFile("lzuc-", null);
            mLogger.debug("Temporary file is " + tempFile.getAbsolutePath());
            try {
                FileUtils.encode(in, tempFile, enc);
            } catch (IOException e) {
                FileUtils.close(in);
                throw e;
            }
            try {
                in = new TempFileInputStream(tempFile);
            } catch (FileNotFoundException e) {
                throw new ChainedException(e);
            }
        }
        return in;
    }
}
