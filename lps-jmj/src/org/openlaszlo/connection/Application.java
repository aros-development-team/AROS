/* *****************************************************************************
 * Application.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.connection;

import java.io.*;
import java.util.*;
import javax.servlet.http.*;
import javax.servlet.*;
import org.openlaszlo.auth.*;
import org.openlaszlo.utils.*;
import org.apache.log4j.*;

public class Application 
{
    private static final int RANGE_ALL = 0;
    private static final int RANGE_USER = 1;
    private static final int RANGE_AGENT = 2;

    private static Hashtable mApplications = new Hashtable();
    static private Logger mLogger  = Logger.getLogger(Application.class);

    static public Application getApplication(String name)
    {
        return getApplication(name, true);
    }

    synchronized static public Application getApplication(String name, boolean create)
    {
        Application application = (Application)mApplications.get(name);
        if (application == null && create) {
            application = new Application(name);
            mApplications.put(name, application);
        }
        return application;
    }

    synchronized static public void removeApplication(Application application) {
        ConnectionGroup group = application.getConnectionGroup();
        if (group != null) {
            group.unregisterApplication(application);
        }
        mApplications.remove(application);
    }

    private String mName;
    private long mLastModifiedTime = 0;
    private long mHeartbeat = 0;
    private Authentication mAuthenticator = null;
    private boolean mSendUserDisconnect = false;
    private ConnectionGroup mGroup = null;

    /** Username lookup */
    private MultiMap mUsers = new MultiMap();

    /** Connections */
    private Hashtable mConnections = new Hashtable();

    /** Agents */
    private Hashtable mAgents = new Hashtable();

    private Application(String name) {
        mName = name;
    }

    public String getName() {
        return mName;
    }

    public long getLastModifiedTime() {
        return mLastModifiedTime;
    }

    public void setLastModifiedTime(long lmt) {
        mLastModifiedTime = lmt;
    }

    public long getHeartbeat() {
        return mHeartbeat;
    }

    public void setHeartbeat(long heartbeat) {
        mHeartbeat = heartbeat;
    }

    public Authentication getAuthenticator() {
        return mAuthenticator;
    }

    public void setAuthenticator(Authentication authenticator) {
        mAuthenticator = authenticator;
    }

    public boolean doSendUserDisconnect() {
        return mSendUserDisconnect;
    }

    public void setSendUserDisconnect(boolean sud) {
        mSendUserDisconnect = sud;
    }

    public ConnectionGroup getConnectionGroup() {
        return mGroup;
    }

    public void setConnectionGroup(ConnectionGroup group) {
        if (mGroup != null) {
            mGroup.unregisterApplication(this);
        }
        group.registerApplication(this);
        mGroup = group;
    }


    //------------------------------------------------------------
    // Application wrappers for connection group method calls.
    //------------------------------------------------------------
    private void checkGroup() {
        if (mGroup == null)
            throw new RuntimeException("connection group not set"); 
    }

    public void register(HTTPConnection connection) {
        synchronized (mUsers) {
            mUsers.put(connection.getUsername(), connection);
            mConnections.put(connection.getCID(), connection);
        }
    }

    public void unregister(HTTPConnection connection) {
        synchronized (mUsers) {
            mUsers.remove(connection.getUsername(), connection);
            mConnections.remove(connection.getCID());
        }
    }

    public void setAgents(Set agentSet) {
        mAgents = new Hashtable();
        if (agentSet != null) {
            synchronized (mAgents) {
                Iterator iter = agentSet.iterator();
                while (iter.hasNext()) {
                    ConnectionAgent agent = 
                        ConnectionAgent.getAgent((String)iter.next());
                    mAgents.put(agent.getURL(), agent);
                }
            }
        }
    }


    /** 
     * Send message to a list of users
     * @param userList list of users to send message or '*' for everyone on the system
     * @param mesg message to send 
     * @return number of messages sent 
     */
    public int sendMessage(String users, String mesg, String range, 
                           StringBuffer xmlResult)
    {
        mLogger.debug("sendMessage(users=" + users + ",range=" + range + ",mesg=" + mesg + ")");

        if (users == null || users.equals(""))
            return 0;

        int r = RANGE_ALL;
        if (range == null || range.equals("")) {
            r = RANGE_ALL;
        } else if (range.equals("user")) {
            r = RANGE_USER;
        } else if (range.equals("agent")) {
            r = RANGE_AGENT;
        }

        if (users.equals("*")) {
            return sendAllMessage(mesg, r, xmlResult);
        } 

        int count = 0;
        StringTokenizer st = new StringTokenizer(users, ", ");
        while (st.hasMoreTokens()) {
            String username = (String)st.nextToken();

            if (r == RANGE_ALL || r == RANGE_USER) {
                synchronized (mUsers) {
                    Set usernameSet = (Set)mUsers.get(username);
                    if (usernameSet != null) {
                        Iterator iter = usernameSet.iterator();

                        while (iter.hasNext()) {
                            HTTPConnection connection = (HTTPConnection)iter.next();
                            try {
                                mLogger.debug("send to " + connection.getUsername());
                                connection.send(mesg);
                                ++count;
                            } catch (IOException e) {
                                mLogger.debug("user " + connection.getUsername() +
                                              " not connected");
                                iter.remove();
                            }
                        }

                        if (usernameSet.size() == 0)
                            mUsers.remove(username);
                    }
                }
            }

            if (r == RANGE_ALL || r == RANGE_AGENT) {
                ConnectionAgent agent = null;
                try {
                    synchronized (mAgents) {
                        agent = (ConnectionAgent)mAgents.get(username);
                    }
                    if (agent != null) {
                        if (xmlResult != null) {
                            StringBuffer tmp = 
                                new StringBuffer("<agent url=\"" + agent.getURL() + "\" >");
                            tmp.append(agent.send(mesg));
                            tmp.append("</agent>");
                            xmlResult.append(tmp.toString());
                        } else {
                            agent.send(mesg);
                        }
                        ++count;
                    }
                } catch (IOException e) {
                    mLogger.warn("IOException: agent " + agent.getURL());
                }
            }
            
        }
        return count;
    }

    private int sendAllMessage(String mesg, int range, StringBuffer xmlResult) 
    {
        int count = 0;
        Iterator iter;

        if (range == RANGE_ALL || range == RANGE_USER) {
            synchronized (mUsers) {
                iter = mConnections.entrySet().iterator();
                while (iter.hasNext()) {
                    Map.Entry entry = (Map.Entry)iter.next();
                    HTTPConnection connection = (HTTPConnection)entry.getValue();
                    try {
                        connection.send(mesg);
                        ++count;
                        mLogger.debug(connection.getUsername() + " sent message");
                    } catch (IOException e) {
                        iter.remove();
                        mLogger.debug(connection.getUsername() + " not connected");
                    }
                }
            }
        }

        if (range == RANGE_ALL || range == RANGE_AGENT) {
            synchronized (mAgents) {
                iter = mAgents.entrySet().iterator();
                while (iter.hasNext()) {
                    Map.Entry entry = (Map.Entry)iter.next();
                    ConnectionAgent agent = (ConnectionAgent)entry.getValue();
                    try {
                        if (xmlResult != null) {
                            String result = agent.send(mesg);
                            StringBuffer tmp =
                                new StringBuffer("<agent url=\"" + agent.getURL() + "\" >");
                            tmp.append(result);
                            tmp.append("</agent>");
                            xmlResult.append(tmp.toString());
                        } else {
                            agent.send(mesg);
                        }
                        ++count;
                    } catch (IOException e) {
                        mLogger.warn("IOException: agent " + agent.getURL());
                    }
                }
            }
        }

        if (xmlResult != null) {
            xmlResult.insert(0, "<send count=\"" + count + "\" >");
            xmlResult.append("</send>");
        }

        return count;
    }


    /** 
     * Return an xml string list of connected users. Agents are not considered
     * users.
     */
    public Set list(String users) {
        synchronized (mUsers) {
            Set set = new HashSet();
            mLogger.debug("list(users=" + users + ")");

            if (users.equals("*")) {
                Iterator iter = mUsers.keySet().iterator();
                while (iter.hasNext()) {
                    set.add((String)iter.next());
                }
            } else {
                StringTokenizer st = new StringTokenizer(users, ", ");
                while (st.hasMoreTokens()) {
                    String username = (String)st.nextToken();
                    if (mUsers.containsKey(username))
                        set.add(username);
                }
            }
            return set;
        }
    }



    public HTTPConnection getConnection(String cid) {
        synchronized (mUsers) {
            if (cid == null || cid.equals(""))
                return null;
            return (HTTPConnection) mConnections.get(cid);
        }
    }


    //----------------------------------------------------------------------
    // Info functions
    //----------------------------------------------------------------------

    synchronized static public void dumpApplicationsXML(StringBuffer buf, 
                                                        boolean details) {
        Set s = mApplications.entrySet();
        Iterator iter = s.iterator();
        buf.append("<applications>");
        while (iter.hasNext()) {
            Map.Entry e = (Map.Entry) iter.next();
            Application app = (Application)e.getValue();
            app.dumpXML(buf, details);
        }
        buf.append("</applications>");
    }

    public void dumpXML(StringBuffer buf, boolean details) {
        Authentication a = mAuthenticator;
        ConnectionGroup g = mGroup;
        buf.append("<application ")
            .append(" name=\"").append(mName).append("\"")
            .append(" group=\"").append(( g!=null ? g.getName() : "none" )).append("\"")
            .append(" heartbeat=\"").append(mHeartbeat).append("\"")
            .append(" authenticator=\"").append(( a!=null ? a.getClass().toString() : "none" )).append("\"")
            .append(" senduserdisconnect=\"").append(mSendUserDisconnect).append("\"")
            .append(" >");
//         dumpUsersXML(buf, details);
        dumpConnectionsXML(buf, details);
        dumpAgentsXML(buf, details);
        buf.append("</application>");
    }

    public void dumpConnectionsXML(StringBuffer buf, boolean details) {
        synchronized (mUsers) {
            dumpTableXML("connection", mConnections, buf, details);
        }
    }

    public void dumpAgentsXML(StringBuffer buf, boolean details) {
        synchronized (mAgents) {
            dumpTableXML("agent", mAgents, buf, details);
        }
    }

    public void dumpUsersXML(StringBuffer buf, boolean details) {
        synchronized (mUsers) {
            dumpMultiTableXML("user", mUsers, buf, details);
        }
    }

    public static void dumpMultiTableXML(String table, Map map, 
                                         StringBuffer buf, boolean details)
    {
        Set s = map.entrySet();
        Iterator iter = s.iterator();
        buf.append("<").append(table).append("-table>");
        while (iter.hasNext()) {
            Map.Entry e = (Map.Entry)iter.next();
            String k = (String)e.getKey();
            if (details) {
                buf.append("<" + table + " name=\"" + k + "\">");
                Set set = (Set)e.getValue();
                Iterator iter1 = set.iterator();
                while (iter1.hasNext()) {
                    Object o = iter1.next();
                    buf.append(o.toString());
                }
                buf.append("</" + table + ">\n");
            } else {
                buf.append("<" + table + " name=\"").append(k).append("\" />\n");
            }

        }
        buf.append("</").append(table).append("-table>\n");
    }


    public static void dumpTableXML(String table, Map map, 
                                    StringBuffer buf, boolean details)
    {
        Set s = map.entrySet();
        Iterator iter = s.iterator();
        buf.append("<").append(table).append("-table>");
        while (iter.hasNext()) {
            Map.Entry e = (Map.Entry)iter.next();
            String k = (String)e.getKey();
            if (details) {
                Object v = e.getValue();
                buf.append(v.toString());
            } else {
                buf.append("<" + table + " name=\"").append(k).append("\" />");
            }

        }
        buf.append("</").append(table).append("-table>\n");
    }
}
