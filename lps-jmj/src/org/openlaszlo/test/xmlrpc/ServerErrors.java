/* ****************************************************************************
 * ServerErrors.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.test.xmlrpc;

public class ServerErrors
{
	public ServerErrors()


	{}


	public ServerErrors(java.util.Map arg)
	{
		// nothing here
	}

	public void makeException()
	{
		throw new RuntimeException("Something bad happened");
	}

	public void invokeMe(Object arg)
	{}

	public String forceTimeout(String val) throws InterruptedException
	{
		Thread.sleep(Long.parseLong(val));
		return "got timeout?";
	}

	void noAccess() {}
};