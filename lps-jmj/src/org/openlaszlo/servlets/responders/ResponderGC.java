/******************************************************************************
 * ResponderGC.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.ServletOutputStream;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.cache.RequestCache;
import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.cm.CompilationManager;
import org.apache.log4j.Logger;

public final class ResponderGC extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderGC.class);

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        System.gc();
        respondWithXML(res, "<gc>ran garbage collector</gc>");
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
