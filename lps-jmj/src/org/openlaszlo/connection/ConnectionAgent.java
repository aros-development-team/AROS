/******************************************************************************
 * ConnectionAgent.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.connection;

import java.io.*;
import java.util.*;
import java.net.*;
import java.security.*;
import javax.servlet.*;
import javax.servlet.http.*;
import org.openlaszlo.compiler.*;
import org.openlaszlo.data.*;
import org.openlaszlo.server.*;
import org.openlaszlo.utils.*;
import org.apache.log4j.*;
import org.jdom.input.*;
import org.jdom.*;

public class ConnectionAgent
{
    private static Logger mLogger = Logger.getLogger(ConnectionAgent.class);

    String mURL;

    private static Hashtable mAgents = new Hashtable();

    private ConnectionAgent(String url) 
    {
        try{
            mURL = url;

            String host = new URL(url).getHost();
            if (host == null || host.equals(""))
                throw new RuntimeException("bad host in url");

            mLogger.debug("Agent " + url);

        } catch (MalformedURLException e) {
            throw new RuntimeException(e.getMessage());
        }
    }

    synchronized static public ConnectionAgent getAgent(String url)
    {
        return getAgent(url, true);
    }

    synchronized static public ConnectionAgent getAgent(String url, boolean create)
    {
        ConnectionAgent agent = (ConnectionAgent) mAgents.get(url);
        if (agent == null && create) {
            agent = new ConnectionAgent(url);
            mAgents.put(url, agent);
        }
        return agent;
    }


    public String getURL()
    {
        return mURL;
    }

    public String send(String msg) throws IOException {
        String surl = mURL + "?xml=" + URLEncoder.encode(msg); 
        Data data = null;
        try {
            data = HTTPDataSource.getHTTPData(null, null, surl, -1);
            return data.getAsString();
        } catch (DataSourceException e) {
            throw new IOException(e.getMessage());
        } finally {
            if (data != null)
                data.release();
        }
    }

    synchronized static public void dumpAgentsXML(StringBuffer buf, boolean details)
    {
        Application.dumpTableXML("agent", mAgents, buf, details);
    }

    public String toString() {
        return new StringBuffer("<agent ")
            .append(" url=\"").append(mURL).append("\"")
            .append(" />")
            .toString();
    }
}
