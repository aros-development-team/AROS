/******************************************************************************
 * SystemProp.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

import org.apache.xmlrpc.*;
import java.util.*;

public class SystemProp
{
	public static Hashtable getProperties()
	{
		return System.getProperties();
	}

    public static void main(String argv[])
    {
        WebServer ws = new WebServer(8181);
        SystemProp se = new SystemProp();
        ws.addHandler("localservice", se);
        ws.start();
    }
}