/******************************************************************************
 * DataEncoderException.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml;

/**
 * General exception used by the DataEncoder.
 */
public class DataEncoderException extends Exception {

    /**
     * Constructs a DataEncoderException with no specified detail message.
     */
    public DataEncoderException()
    {
        super();
    }

    /**
     * Constructs a DataEncoderException with a detail message.
     * @param s the detail message.
     */
    public DataEncoderException(String s)
    {
        super(s);
    }
}
