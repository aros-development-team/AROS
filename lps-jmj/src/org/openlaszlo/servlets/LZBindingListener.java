/* *****************************************************************************
 * LZBindingListener.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets;

import java.io.File;
import java.util.HashSet;
import java.util.Iterator;
import javax.servlet.ServletContext;
import javax.servlet.http.HttpSessionBindingEvent;
import javax.servlet.http.HttpSessionBindingListener;
import org.apache.log4j.*;

/**
 * This class implements an HttpSessionBindingListener and is used to remove
 * temporary session files.<p>
 */
public class LZBindingListener
    implements HttpSessionBindingListener
{
    /**
     * Logger 
     */
    private static Logger mLogger  = Logger.getLogger(LZBindingListener.class);

    /**
     * Store temporary filename to remove during valueUnbound().
     */
    HashSet mTempFileNameSet;

    /**
     * Constructor.
     */
    public LZBindingListener(String fileName)
    {
        mTempFileNameSet = new HashSet();
        addTempFile(fileName);
    }

    /**
     * At the moment, don't do anything when value is bound, aside from loggin
     * session id.
     */
    public void valueBound(HttpSessionBindingEvent event) 
    {
        mLogger.info("SessId bound " + event.getSession().getId());
    }

    /**
     * Remove the file when session value is unbound.
     */
    public synchronized void valueUnbound(HttpSessionBindingEvent event) 
    {
        mLogger.info("SessID unbound " + event.getSession().getId());

        // Remove all temporary files.
        Iterator iter = mTempFileNameSet.iterator();
        while (iter.hasNext()) {
            String fileName = (String)iter.next();
            File file = new File(fileName);
            file.delete();
        }
        mTempFileNameSet.clear();
    }


    /**
     * Add a temporary file name to remove when unbinding.
     */
    public synchronized void addTempFile(String fileName)
    {
        if (fileName == null || fileName.length() == 0)
            return;

        if (! mTempFileNameSet.contains(fileName))
            mTempFileNameSet.add(fileName);
    }


    /**
     * Deletes a requested temporary file. 
     */
    public synchronized boolean delTempFile(String fileName) 
    {
        if (fileName != null && mTempFileNameSet.contains(fileName)) {
            mTempFileNameSet.remove(fileName);
            File file = new File(fileName);
            return file.delete();
        }

        return false;
    }

}
