/******************************************************************************
 * LZViewer.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets;

import java.io.File;
import java.net.URL;
import java.io.FileWriter;
import java.io.FileInputStream;
import java.io.IOException;
import javax.servlet.*;
import javax.servlet.http.*;

import org.openlaszlo.cm.CompilationManager;
import org.openlaszlo.compiler.CompilationError;
import org.openlaszlo.compiler.Canvas;
import org.openlaszlo.utils.StringUtils;
import org.openlaszlo.xml.internal.XMLUtils;
import org.openlaszlo.servlets.LZBindingListener;


/** Allows one to view and play with the source for an LZX file.
 *                                                                
 * A temporary LZX file with a unique session id will be created for each client
 * session accessing this page. You may want to may want to occassionally remove
 * these temp files.
 */
public class LZViewer extends HttpServlet {

    public void doGet(HttpServletRequest request, HttpServletResponse response)
        throws ServletException, IOException
    {

        // Turn off client caching as best we can 
        response.setHeader("Cache-control", "no-cache");
        response.setDateHeader("Expires", 0);
        response.setContentType ("text/html");

        String url = (String)request.getAttribute("LZF_URL");
        if (url == null) {
            throw new ServletException("No LZF_URL attribute set");
        }
        String uri = (new URL(url)).getFile();
        String fileName = (String)request.getAttribute("LZF_FILENAME");
        if (fileName == null) {
            throw new ServletException("No LZF_FILENAME attribute set");
        }
        File file = new File(fileName);
        CompilationManager compMgr = (CompilationManager)request.getAttribute("LZF_COMPMGR");
        if (compMgr == null) {
            throw new ServletException("No LZF_COMPMGR attribute set");
        }
 
        // Grab the LZX property  
        String lzx = request.getParameter("LZX");

        if (lzx != null) {

            HttpSession session = request.getSession();

            // Construct a temporary file and copy LZX source to it 
            File tempDir = new File(file.getParent() + File.separator);
            tempDir.mkdirs();

            // Associate a temporary filename to a session. "File.deleteOnExit()"
            // method guarantees the file will be removed when the server exits.
            String tempFileName = "__tmp-" + session.getId() + ".lzx";
            String fullFileName = tempDir.getPath() + File.separator + tempFileName;
            File tempFile = new File(fullFileName);
            tempFile.deleteOnExit();
            FileWriter writer = new FileWriter(tempFile);
            writer.write(lzx);
            writer.close();

            LZBindingListener lz = (LZBindingListener)session.getAttribute("laszlo");
            if (lz == null) {
                lz = new LZBindingListener(fullFileName);
                session.setAttribute("laszlo", lz);
            }

            // Adjust the URI 
            
            int slashIndex = uri.lastIndexOf('/');
            uri = uri.substring(0, slashIndex) + "/" + tempFileName;
            fileName = fullFileName;

        } else {
            // Silently truncate impossibly long source files
            int length = (int)file.length(); 
            byte[] array = new byte[length];
            FileInputStream in = new FileInputStream(file);
            in.read(array);
            lzx = new String(array);
        }

        // Get the canvas 
        int width  = 500;
        int height = 500;
        CompilationError error = null;
        try {
            Canvas canvas = compMgr.getCanvas(fileName);
            width  = canvas.getWidth();
            height = canvas.getHeight();
        }
        catch (CompilationError e) {
            error = e;
        }

        // Display HTML
        ServletOutputStream  out = response.getOutputStream();
        out.println("<html><head><title>LZViewer</title></head>");
        if (error == null) {
            out.println("<body onload=\"openWin();\" onunload=\"closeWin();\">");
            out.println("<!-- Pop-up a window with the app in it -->");
            out.println("<script>");
            out.println("viewerWin = null;");
            out.println("function openWin() {");
            out.println("    closeWin();");
            out.print  ("    viewerWin = window.open('" + uri + "', 'viewer',");
            out.print  ("'width=" + width + ",height=" + height + ",toolbar=no,location=no,");
            out.println("directories=no,status=no,menubars=no,scrollbars=no,resizable=no');");
            out.println("    viewerWin.focus();");
            out.println("}");
            out.println("function closeWin() {");
            out.println("    if (viewerWin && ! viewerWin.closed) {");
            out.println("        viewerWin.close();");
            out.println("    }");
            out.println("}");
            out.println("</script>");
        } else {
            String details = "";
            if (error.getLineNumber() != null) {
                details += ", line " + error.getLineNumber();
                if (error.getColumnNumber() != null) {
                    details += ", column " + error.getColumnNumber();
                }
            }
            out.println("<p><font color=#ff0000>Syntax Error" + details + " </font>: ");
            out.println(XMLUtils.escapeAmpersands(error.getErrorMessage()));
        }

        lzx = XMLUtils.escapeAmpersands(lzx);
        lzx = StringUtils.replace(lzx, '<', "&lt;");

        out.println("<!-- Display a form with the LZX text in it -->");
        out.println("<form action=\"" + uri + "?lzt=filter&filter=/LZViewer\" method=\"post\">");
        out.println("<input type=\"submit\" value=\"Update\">");
        out.println("<textarea name=\"LZX\" cols=\"80\" rows=\"36\">");
        out.println(lzx);
        out.println("</textarea>");
        out.println("</form>");
        out.println("</body></html>");
    }

    public void doPost(HttpServletRequest request, HttpServletResponse response)
        throws ServletException, IOException
    {
        doGet(request, response);
    }
}
