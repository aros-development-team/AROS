/******************************************************************************
 * ResponderSTAT.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import java.util.Date;
import java.util.Properties;
import javax.servlet.ServletConfig;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.ServletOutputStream;
import org.openlaszlo.cache.RequestCache;
import org.openlaszlo.server.LPS;
import org.openlaszlo.servlets.LZServlet;
import org.openlaszlo.utils.FileUtils;
import org.apache.log4j.Logger;

public final class ResponderSTAT extends ResponderAdmin
{
    private static Logger mLogger = Logger.getLogger(ResponderSTAT.class);

    private static Date mLastCleared = new Date();

    protected void respondAdmin(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        if ( ! mCollectStat ) {
            respondWithMessage(res, "Statistics collection is off.");
            return;
        }

        res.setContentType ("text/xml");
        ServletOutputStream out = res.getOutputStream();
        try {
            String msg = statInfo(req);
            out.println(msg);
        } finally {
            FileUtils.close(out);
        }
    }

    /**
     * send cache info out the response in HTML
     */
    public static String statInfo(HttpServletRequest req)
        throws IOException
    {
        ResponderCache dataResponder = (ResponderCache)getResponder("data");
        ResponderCache mediaResponder = (ResponderCache)getResponder("media");

        if (dataResponder != null) {
            String durl = req.getParameter("durl");
            if (durl != null)
                dataResponder.mURLStat.doURLCollection(durl.equals("1"));
        }

        if (mediaResponder != null) {
            String murl = req.getParameter("murl");
            if (murl != null)
                mediaResponder.mURLStat.doURLCollection(murl.equals("1"));
        }

        String clear = req.getParameter("clear");
        if (clear != null && clear.equals("1")) {
            if (dataResponder != null)
                dataResponder.mURLStat.clear();
            if (mediaResponder != null)
                mediaResponder.mURLStat.clear();
            Responder.mSTAT_allLoadCount.reset();
            Responder.mSTAT_compileLoadCount.reset();
            Responder.mSTAT_mediaLoadCount.reset();
            Responder.mSTAT_dataLoadCount.reset();
            Responder.clearErrorSWFCount();
            mLastCleared = new Date();
        }

        RequestCache mediaCache = ResponderMEDIA.getCache();
        RequestCache dataCache  = ResponderDATA.getCache();

        StringBuffer buf = new StringBuffer("");
        buf.append("<stat");
        buf.append(" up-since=\"").append(Responder.mSTAT_startDate.toString()).append("\"");
        buf.append(" last-cleared=\"").append(mLastCleared).append("\"");
        buf.append(" total-error-count=\"").append(Responder.getErrorSWFCount()).append("\"");
        buf.append(">\n");
        {
            buf.append(LPS.getInfo(req, mContext, "info"));

            buf.append("<load>\n");
            {
                buf.append(Responder.mSTAT_compileLoadCount.toXML("application"));
                buf.append(Responder.mSTAT_mediaLoadCount.toXML("media"));
                buf.append(Responder.mSTAT_dataLoadCount.toXML("data"));
                buf.append("<persistent-connection active=\"")
                    .append(ResponderCONNECT.getActiveCount())
                    .append("\" />");
                buf.append(Responder.mSTAT_allLoadCount.toXML("all"));
            }
            buf.append("</load>\n");

            buf.append("<urls>");
            if (dataResponder != null)
                buf.append(dataResponder.mURLStat.toXML());
            if (mediaResponder != null)
                buf.append(mediaResponder.mURLStat.toXML());
            buf.append("</urls>");
        }
        buf.append("</stat>\n");
        return buf.toString();
    }

    public static Responder getResponder(String lzt)
    {
        return (Responder) LZServlet.mResponderMap.get(lzt);
        
    }

    public int getMimeType()
    {
        return MIME_TYPE_XML;
    }
}
