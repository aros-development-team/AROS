/******************************************************************************
 * ResponderEVAL.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets.responders;

import java.io.*;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.ServletOutputStream;
import org.openlaszlo.compiler.Compiler;
import org.openlaszlo.compiler.CompilationEnvironment;
import org.openlaszlo.media.MimeType;
import org.openlaszlo.sc.ScriptCompiler;
import org.openlaszlo.utils.FileUtils;
import org.apache.log4j.Logger;

public final class ResponderEVAL extends Responder
{
    private static Logger mLogger = Logger.getLogger(ResponderEVAL.class);

    protected void respondImpl(HttpServletRequest req, HttpServletResponse res)
        throws IOException
    {
        ServletOutputStream out = res.getOutputStream();

        String script = req.getParameter("lz_script");
        boolean logmsg = false;

        String seqnum = req.getParameter("lzrdbseq");

        String lz_log = req.getParameter("lz_log");

        if ((lz_log != null) && lz_log.equals("true")) {
            logmsg = true;
        }

        if (logmsg) {
            // Just write to the log and let the output connection close
            mLogger.info("CLIENT_LOG " + script);
            byte[] action = new byte[0];
            int swfversion = 5;
            ScriptCompiler.writeScriptToStream(action, out, swfversion);          
            out.flush();
            FileUtils.close(out);
        } else {
            mLogger.info("doEval for " + script+", seqnum="+seqnum);
            try {
                res.setContentType(MimeType.SWF);
                Compiler compiler = new Compiler();
                String swfversion = req.getParameter("lzr");
                // For back compatibility, should an older app that doesn't pass "lzr" arg
                // be running somehow.
                if (swfversion == null) {
                    swfversion = "swf5";
                }
                compiler.compileAndWriteToSWF(script, seqnum, out, swfversion);
            } catch (Exception e) {
                mLogger.info("LZServlet got error compiling/writing SWF!" + e);
                StringWriter err = new StringWriter();
                e.printStackTrace(new PrintWriter(err));
                mLogger.info(err.toString());
            }
        }
    }

    public int getMimeType()
    {
        return MIME_TYPE_SWF;
    }
}
