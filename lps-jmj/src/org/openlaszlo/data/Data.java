/* *****************************************************************************
 * Data.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

import org.openlaszlo.xml.DataEncoder;
import javax.servlet.http.HttpServletResponse;
import java.io.InputStream;
import java.io.IOException;
import java.io.InterruptedIOException;

/**
 * An abstraction for holding data.  
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public abstract class Data {

    /**
     * @return input stream on the data. IO operations on the stream that
     * timeout should throw InterrupedIOExceptions
     *
     * @throws IOException on an io exception
     * @throws InterruptedIOException on a timeout
     */
    public abstract InputStream getInputStream() 
        throws InterruptedIOException, IOException;

    /**
     * @return mime type of data (or null if unknown)
     */
    public abstract String getMimeType();

    /**
     * This method should be implemented for Data's who's mime-type is XML.
     *
     * @return a string containing the data
     * @throws IOException on an IOException
     * @throws InterruptedIOException on a timeout
     */
    public String getAsString() throws InterruptedIOException, IOException {
        throw new RuntimeException("unimplemented");
    }

    /**
     * This method should be implemented for Data's who's mime-type is XML.
     *
     * Append any response meta data into the given string as XML
     */
    public void appendResponseHeadersAsXML(StringBuffer xmlResponse) {
        throw new RuntimeException("unimplemented");
    }

    /**
     * Checks if the data was not modified.
     *
     * @return true if the data was "not modified" since
     * the last time it was accessed.
     */
    public boolean notModified() {
        return false;
    }

    /**
     * @return size of the data if known or -1 if not known
     */
    public long size() {
        return -1;
    }

    /**
     * @return the lastModified time of the data or -1
     * if the last modified time is unknown. 
     */
    public long lastModified() {
        return -1;
    }

    /**
     * release any resources associated with this data
     */
    public void release() {
    }
}
