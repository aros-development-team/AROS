/* *****************************************************************************
 * DataSourceException.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.data;

/**
 * General exception used by data sources.
 */
public class DataSourceException
    extends Exception
{
    /**
     * Constructs a DataSourceException with no specified detail message.
     */
    public DataSourceException()
    {
        super();
    }

    /**
     * Constructs a DataSourceException with a detail message.
     * @param s the detail message.
     */
    public DataSourceException(String s)
    {
        super(s);
    }
}
