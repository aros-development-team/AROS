/******************************************************************************
 * MimeType.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.media;

import org.openlaszlo.utils.FileUtils;

/**
 * A class for defining mimetypes and utilities.
 *
 * TODO:[2002-12-3 bloch] Someday this should be refactored for plugins
 *
 * @author Eric Bloch
 * @version 1.0
 */
public class MimeType {
    public static final String TEXT = "text/html";
    public static final String SWF = "application/x-shockwave-flash";
    public static final String JPEG = "image/jpeg";
    public static final String GIF = "image/gif";
    public static final String PNG = "image/png";
    public static final String MP3 = "audio/mpeg";
    public static final String XMP3 = "audio/x-mpeg";
    public static final String XML = "text/xml";
    public static final String HTML = "text/html";

    public static final String UNKNOWN = "unknown";

    /**
     * @return a mimetype string based on the extension (chars after the '.')
     * @return <code>UNKNOWN</code> if no guess can be made
     */
    public static final String fromExtension(final String name) {
        String extension = FileUtils.getExtension(name);
        if (extension == null) {
            return UNKNOWN;
        }
        if (extension.equals("") ) {
            return UNKNOWN;
        }
        if (extension.equalsIgnoreCase("swf")) {
            return SWF;
        }
        if (extension.equalsIgnoreCase("jpg") ||
            extension.equalsIgnoreCase("jpeg")) {
            return JPEG;
        }
        if (extension.equalsIgnoreCase("gif")) {
            return GIF;
        }
        if (extension.equalsIgnoreCase("png")) {
            return PNG;
        }
        if (extension.equalsIgnoreCase("xml")) {
            return XML;
        }
        if (extension.equalsIgnoreCase("lzx")) {
            return XML;
        }
        if (extension.equalsIgnoreCase("html") ||
            extension.equalsIgnoreCase("htm")) {
            return HTML;
        }
        if (extension.equalsIgnoreCase("mp3") ||
            extension.equalsIgnoreCase("mpeg")) {
            return MP3;
        }
        return UNKNOWN;
    }

    /**
     * @param mimeType mimetype to give extension of
     * @return a file type extension string based on the extension (chars after the '.').
     * If no such mimetype is found, the mimeType string is returned.
     */
    public static final String toExtension(final String mimeType) {

        if (mimeType == MimeType.JPEG) {
            return "jpeg";
        }
        if (mimeType == MimeType.SWF) {
            return "swf";
        }
        if (mimeType == MimeType.GIF) {
            return "gif";
        }
        if (mimeType == MimeType.PNG) {
            return "png";
        }
        if (mimeType == MimeType.MP3) {
            return "mp3";
        }
        if (mimeType == MimeType.XMP3) {
            return "mp3";
        }

        return mimeType;
    }
}
