/* *****************************************************************************
 * AuthenticationException.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.auth;

/**
 * Authorization exception class.
 */
public class AuthenticationException
    extends Exception
{
    public AuthenticationException()
    {
        super();
    }

    public AuthenticationException(String s)
    {
        super(s);
    }
}
