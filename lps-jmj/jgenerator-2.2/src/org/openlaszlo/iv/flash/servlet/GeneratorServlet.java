/*
 * $Id: GeneratorServlet.java,v 1.8 2002/07/15 02:15:03 skavish Exp $
 *
 * ===========================================================================
 *
 * The JGenerator Software License, Version 1.0
 *
 * Copyright (c) 2000 Dmitry Skavish (skavish@usa.net). All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The end-user documentation included with the redistribution, if
 *    any, must include the following acknowlegement:
 *    "This product includes software developed by Dmitry Skavish
 *     (skavish@usa.net, http://www.flashgap.com/)."
 *    Alternately, this acknowlegement may appear in the software itself,
 *    if and wherever such third-party acknowlegements normally appear.
 *
 * 4. The name "The JGenerator" must not be used to endorse or promote
 *    products derived from this software without prior written permission.
 *    For written permission, please contact skavish@usa.net.
 *
 * 5. Products derived from this software may not be called "The JGenerator"
 *    nor may "The JGenerator" appear in their names without prior written
 *    permission of Dmitry Skavish.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL DMITRY SKAVISH OR THE OTHER
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

package org.openlaszlo.iv.flash.servlet;

import java.rmi.RemoteException;
import java.io.*;
import java.net.*;
import java.util.*;

import javax.servlet.*;
import javax.servlet.http.*;

import org.openlaszlo.iv.flash.api.*;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.cache.*;

import org.openlaszlo.iv.flash.url.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.api.sound.*;
import org.openlaszlo.iv.flash.context.*;

import org.apache.log4j.*;
import org.apache.log4j.spi.*;

/**
 * Generator servlet
 * processes http requests from clients
 */
public class GeneratorServlet extends HttpServlet {

    private String errorTemplateName = null;
    private int currentThreadNum = 0;
    private int maxThreads = 0;
    private int gcAfterJobsCount = 0;
    private int jobsCountSinceLastGC = 0;
    private boolean wrapAssets = false;
    private boolean createServletContext = false;

    public void init( ServletConfig config ) throws ServletException {
        super.init( config );

        // initialize
        String installDir = getInitParameter("org.openlaszlo.iv.flash.virtualInstallDir");
        if( installDir != null ) {
            installDir = config.getServletContext().getRealPath(installDir);
        } else {
            installDir = getInitParameter("org.openlaszlo.iv.flash.installDir");
            if( installDir == null ) {
                installDir = config.getServletContext().getRealPath("/");
            }
        }

        String propFileName = getInitParameter("org.openlaszlo.iv.flash.propertiesFile");

        Util.init(installDir, propFileName);

        // initialize stat manager
        StatManager.init();

        // set some control variables
        createServletContext = PropertyManager.getBoolProperty( "org.openlaszlo.iv.flash.servletContext", false );
        maxThreads = PropertyManager.getIntProperty( "org.openlaszlo.iv.flash.maxThreads", 0 );
        gcAfterJobsCount = PropertyManager.getIntProperty( "org.openlaszlo.iv.flash.garbageCollectAfterJobCount", 0 );
        wrapAssets = PropertyManager.getBoolProperty( "org.openlaszlo.iv.flash.wrapAssets", false );

        // get name of error template
        String fileName = PropertyManager.getProperty("org.openlaszlo.iv.flash.errorTemplate","bin/error.swt");
        if( fileName != null ) {
            File errFile = Util.getSysFile( fileName );
            if( errFile.exists() ) {
                errorTemplateName = errFile.getAbsolutePath();
            }
        }

        Log.info("");
        Log.logRB(Resource.SERVERSTARTED);
        Log.info("");
    }

    /**
     * Send error to the client.
     */
    private void showTextError( String msg, HttpServletResponse res )
        throws ServletException, IOException
    {
        res.setContentType("text/html");
        PrintWriter pw = res.getWriter();
        pw.print( "<html><body><font color=red>"+msg+"</font></body></html>" );
        pw.close();
    }

    /**
     * Wrap given exception in our error.swt end send to the client
     */
    private void showError( Throwable t, String fileName, HttpServletResponse res )
        throws ServletException, IOException
    {
        String fullMessage = t.getLocalizedMessage();

        if( errorTemplateName == null ) {
            showTextError( fullMessage, res );
        } else {
            try {
                if( fullMessage == null ) fullMessage = t.getClass().getName();
                StandardContext context = new StandardContext();
                context.setValue( "bulkErrMessage", fullMessage );
                context.setValue( "template_name", fileName );
                FlashOutput fob = process(errorTemplateName, context, null);
                send( fob, res );
            } catch( Throwable tt ) {
                Log.log( tt );
                showTextError( fullMessage, res );
            }
        }
    }

    /**
     * Create generator context from servlet parameters
     */
    private Context createServletContext( HttpServletRequest req ) {
        CommandExecutor executor = new OnlineCommandExecutor();
        CommandContext context = new CommandContext( executor );
        Enumeration e = req.getParameterNames();
        while( e.hasMoreElements() ) {
            String name = (String) e.nextElement();
            String value = Util.processEscapes( req.getParameter(name) );
            context.setValue(name, value);
        }
        return context;
    }

    protected long getLastModified( HttpServletRequest req ) {
        return -1;
    }

    public void doGet( HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        doGen( req, res );
    }

    public void doPost( HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        doGen( req, res );
    }

    public void doGen( HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        if( maxThreads > 0 && currentThreadNum >= maxThreads ) {
            res.sendError( res.SC_SERVICE_UNAVAILABLE );
            return;
        }

        try {
            currentThreadNum++;

            // create context
            GeneratorServletContext context = null;
            if( createServletContext ) {
                context = GeneratorServletContext.createContext();
                context.setHttpServletRequest( req );
                context.setHttpServletResponse( res );
                context.setServletConfig( getServletConfig() );
            }


            String fileName = getRequestedFileName( req );

            // check for admin requests
            if( fileName.endsWith( "__admin__.swt" ) ) {
                adminRequest( req, res );
                return;
            }

            processTemplate( fileName, req, res );

        } finally {
            currentThreadNum--;

            // destroy context
            if( createServletContext ) GeneratorServletContext.destroyContext();

            jobsCountSinceLastGC++;
            if( gcAfterJobsCount > 0 && jobsCountSinceLastGC >= gcAfterJobsCount ) {
                System.runFinalization();
                System.gc();
                jobsCountSinceLastGC = 0;
            }
        }
    }

    /**
     * Process template
     */
    public void processTemplate( String fileName, HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        long totalTime = System.currentTimeMillis();
        long processTime = totalTime;

        FlashOutput fob = null;

        /*
        Runtime rt = Runtime.getRuntime();
        Log.debug("free memory="+rt.freeMemory()+", total memory="+rt.totalMemory()+", free/total="+(rt.freeMemory()*100L/rt.totalMemory())+"%");
        long cache_size = FontCache.getInstance().getSize();
        cache_size += MediaCache.getInstance().getSize();
        cache_size += RequestCache.getInstance().getSize();
        cache_size += XMLCache.getInstance().getSize();
        Log.debug("cache size="+cache_size);
        */

        Log.logRB(Resource.REQUESTFROM, new Object[] {req.getRemoteHost()});

        // check for request cache parameters
        long lifespan = -1L;
        String key = null;
        boolean grc = Util.toBool( req.getParameter("grc"), false ); // request cache
        grc = grc || RequestCache.getSettings().isForce();
        if( grc ) {
            lifespan = Util.toInt( req.getParameter("gre"), -1 )*1000L;
            // try to retrieve file from cache
            key = fileName+"?"+req.getQueryString();
            fob = RequestCache.getRequest( key );
        }


        boolean needToCache = grc && fob == null;

        // process template
        if( fob == null ) {
            try {
                Context context = createServletContext( req );

                String encoding = req.getParameter("genc");

                if( wrapAssets &&
                    (fileName.endsWith( ".jpg.swt" ) ||
                     fileName.endsWith( ".png.swt" ) ||
                     fileName.endsWith( ".gif.swt" )) )
                {
                    fob = wrapImage( fileName.substring(0, fileName.length()-4), context );
                } else if( wrapAssets && fileName.endsWith( ".mp3.swt" ) ) {
                    fob = wrapSound( fileName.substring(0, fileName.length()-4), context );
                } else {
                    fob = process( fileName, context, encoding );
                }

            } catch( FileNotFoundException e ) {
                Log.logRB(Resource.FILENOTFOUND, new Object[] {fileName});
                res.sendError( res.SC_NOT_FOUND );
                return;
            } catch( OutOfMemoryError e ) {
                // the only known possible problem with memory may be in a caches, so clean them up
                FontCache.getInstance().clear();
                MediaCache.getInstance().clear();
                RequestCache.getInstance().clear();
                XMLCache.getInstance().clear();
                System.gc();
                Log.logRB(Resource.PROCESSERROR, new Object[] {fileName}, e);
                showError(e, fileName, res);
                return;
            } catch( Throwable e ) {
                Log.logRB(Resource.PROCESSERROR, new Object[] {fileName}, e);
                showError(e, fileName, res);
                return;
            }
        }
        processTime = System.currentTimeMillis()-processTime;

        // cache the result
        if( needToCache ) {
            RequestCache.addRequest( key, fob, lifespan );
        }

        // everything is ok, send movie to the client
        try {
            send( fob, res );
        } catch( Throwable t ) {
            Log.logRB(Resource.SENDERROR, new Object[] {fileName}, t);
            return;
        }

        // log
        totalTime = System.currentTimeMillis()-totalTime;
        Log.logRB( Resource.PROCESSREQUEST, new Object[] { fileName, new Integer(fob.getSize()), new Long(processTime), new Long(totalTime) } );

        // generate statistics
        StatManager.addRequest( fileName, fob.getSize(), processTime, totalTime );
    }

    /**
     * Process template<BR>
     * <UL>
     * <LI>parse template
     * <LI>process (perform substitutions and generator commands)
     * <LI>generate movie
     * </UL>
     *
     * @param fileName template file name
     * @param context  generator context
     * @param encoding file encoding
     * @return generated flash content
     * @exception IVException
     * @exception IOException
     */
    protected FlashOutput process( String fileName, Context context, String encoding )
        throws IVException, IOException
    {
        FlashFile file = FlashFile.parse(fileName, false, encoding);

        file.processFile( context );

        return file.generate();
    }

    /**
     * Wrap image into .swf file
     * <p>
     * possible parameters in request:<br>
     * <CODE>
     *   http://host/image.jpg.swt?width=100&height=200&center=true
     * </CODE><br><br>
     * if there is no width and height then there is no scaling<br>
     * if there is no center then there is no translating
     *
     * @param fileName file name to wrap
     * @param context  generator context
     * @return wrapped resource
     * @exception IVException
     * @exception IOException
     */
    protected FlashOutput wrapImage( String fileName, Context context )
        throws IVException, IOException
    {
        IVUrl url = IVUrl.newUrl(fileName);

        Bitmap bitmap = (Bitmap) MediaCache.getMedia(url);
        if( bitmap == null ) {
            bitmap = Bitmap.newBitmap(url);
        }
        MediaCache.addMedia(url, bitmap, bitmap.getSize(), true);

        int width = Util.toInt(context.getValue("width"), -1) * 20;
        int height = Util.toInt(context.getValue("height"), -1) * 20;
        boolean center = Util.toBool(context.getValue("center"), false);
        boolean scale = width >= 0 && height >= 0;

        Instance inst = bitmap.newInstance( width, height, scale, center );

        Script script = new Script(1);
        script.setMain();

        script.newFrame().addInstance( inst, 1 );

        FlashFile file = FlashFile.newFlashFile();

        file.setFrameSize( inst.getBounds() );
        file.setMainScript( script );

        return file.generate();
    }

    /**
     * Wrap sound into .swf file
     * <p>
     * possible parameters in request:<br>
     *   <code>http://host/image.mp3.swt?delay=100</code>
     *
     * @param fileName file name to wrap
     * @param context  generator context
     * @return wrapped resource
     * @exception IVException
     * @exception IOException
     */
    protected FlashOutput wrapSound( String fileName, Context context )
        throws IVException, IOException
    {
        Script script = new Script(1);
        script.setMain();

        IVUrl url = IVUrl.newUrl(fileName);

        MP3Sound sound = (MP3Sound) MediaCache.getMedia(url);
        if( sound == null ) {
            sound = MP3Sound.newMP3Sound( url );
        }
        MediaCache.addMedia(url, sound, sound.getSize(), true);

        // Set the delay if provided
        int delay = Util.toInt(context.getValue("delay"), 0);

        if( delay != 0 ) {
            sound.setDelaySeek( delay );
        }

        SoundInfo soundInfo = SoundInfo.newSoundInfo( 0 );
        StartSound startSound = StartSound.newStartSound( sound, soundInfo );

        script.newFrame().addFlashObject( startSound );

        FlashFile file = FlashFile.newFlashFile();

        file.setFrameSize( GeomHelper.newRectangle(0,0,0,0) );
        file.setMainScript( script );

        return file.generate();
    }

    /**
     * Send generator output buffer to the client
     *
     * @param fob    flash data to send
     * @param res    response to send to
     * @exception ServletException
     * @exception IOException
     */
    protected void send( FlashOutput fob, HttpServletResponse res )
        throws ServletException, IOException
    {
        res.setContentLength( fob.getSize() );
        res.setContentType( "application/x-shockwave-flash" );

        ServletOutputStream sos = res.getOutputStream();
        try {
            sos.write( fob.getBuf(), 0, fob.getSize() );
        } finally {
            sos.close();
        }
    }

    /**
     * Returns absolute path of requested template
     */
    protected String getRequestedFileName( HttpServletRequest req ) {
        return getServletContext().getRealPath( req.getServletPath() );
        //return req.getRealPath( req.getServletPath() );
    }

    private void debugInfo( HttpServletRequest req ) {
        //Log.logRB(Resource.STR,"req.getPathInfo()="+req.getPathInfo());
        //Log.logRB(Resource.STR,"req.getPathTranslated()="+req.getPathTranslated());
        //Log.logRB(Resource.STR,"req.getQueryString()="+req.getQueryString());
        //Log.logRB(Resource.STR,"req.getRealPath()="+req.getRealPath(""));
        //Log.logRB(Resource.STR,"req.getRequestURI()="+req.getRequestURI());
        //Log.logRB(Resource.STR,"req.getServletPath()="+req.getServletPath());

/*        if( server == 0 ) {
          String wwwRoot = req.getPathTranslated();
          if( wwwRoot.endsWith( "\\" ) || wwwRoot.endsWith( "/" ) ) wwwRoot = wwwRoot.substring(0,wwwRoot.length()-1);
          String fileName = wwwRoot+req.getRequestURI();
          return fileName;
        } else if( server == 1 ) {
        }*/
    }

    /**
     * Admin request
     * <P>
     * Admin requests come in form of: http://host/__admin__.swt?parm=value&parm=value...
     * <P>
     * possible parameters:<BR>
     * <UL>
     * <LI>showHTMLStat
     * </UL>
     *
     * @param req
     * @param res
     * @exception ServletException
     * @exception IOException
     */
    public void adminRequest( HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        // very stupid and simple authentication
        String userName = req.getParameter( "username" );
        String password = req.getParameter( "password" );
        String myUserName = PropertyManager.getProperty( "org.openlaszlo.iv.flash.adminUserName" );
        String myPassword = PropertyManager.getProperty( "org.openlaszlo.iv.flash.adminPassword" );
        if( myUserName == null || myPassword == null ||
            userName == null || password == null     ||
            userName.length() == 0 || password.length() == 0 ||
            !myUserName.equals(userName)             ||
            !myPassword.equals(password)               )
        {
            res.sendError( res.SC_UNAUTHORIZED );
            return;
        }

        // parse admin request
        if( req.getParameter( "showHTMLStat" ) != null ) {
            showHTMLStat( req, res );
        } else if( req.getParameter( "getCurrentStat" ) != null ) {
            getCurrentStat( req, res );
        } else if( req.getParameter( "getServerXMLInfo" ) != null ) {
            getServerXMLInfo( req, res );
        } else {
            showTextError( "unknown admin request", res );
        }
    }

    /**
     * Get current stat
     */
    public void getCurrentStat( HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        res.setContentType( "application/x-www-urlformencoded" );
        PrintWriter pw = res.getWriter();

        pw.print( "threadsNum=" + currentThreadNum );
        pw.print( "&serverInfo=" + URLEncoder.encode(getServletContext().getServerInfo()) );

        StatUnit sinceStartup = StatManager.getSinceStartup();
        long upTime = System.currentTimeMillis() - sinceStartup.getStartTime();
        String upTimeStr = (upTime/3600000)+"h+"+((upTime/60000)%60)+"m+"+((upTime/1000)%60)+"s";
        pw.print( "&upTime=" + upTimeStr );
        sinceStartup.printVars(pw);
        pw.close();
    }

    /**
     * Show HTML stat
     */
    public void showHTMLStat( HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        res.setContentType("text/html");
        PrintWriter pw = res.getWriter();
        pw.print( "<html><body>" );
        pw.print( "Current number of threads running: "+currentThreadNum+"<br>" );
        pw.print( "<hr>Since startup: " );
        StatManager.getSinceStartup().print(pw);
        Vector v = StatManager.getStatistic();
        for( int i=0; i<v.size(); i++ ) {
            pw.print("<hr>");
            StatUnit su = (StatUnit) v.elementAt(i);
            su.print(pw);
        }
        pw.print("<hr>");
        pw.print( "</body></html>" );
        pw.close();
    }

    /**
     * Get server info in xml format
     */
    public void getServerXMLInfo( HttpServletRequest req, HttpServletResponse res )
        throws ServletException, IOException
    {
        res.setContentType( "text/xml" );
        PrintWriter pw = res.getWriter();

        pw.println( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" );
        pw.println( "<!-- generated by JGenerator servlet -->" );
        //pw.println( "<!DOCTYPE jgenerator-server SYSTEM \"http://www.flashgap.com/jgenerator-server.dtd\">" );

        pw.println( "<jgenerator-server>" );

        StatUnit sinceStartup = StatManager.getSinceStartup();

        // print server info
        long upTime = System.currentTimeMillis() - sinceStartup.getStartTime();
        String upTimeStr = (upTime/3600000)+"h+"+((upTime/60000)%60)+"m+"+((upTime/1000)%60)+"s";
        pw.println( "<server-info>" );
        pw.println( "<threads-num>"+currentThreadNum+"</threads-num>" );
        pw.println( "<uptime>"+upTimeStr+"</uptime>" );
        pw.println( "<description>"+getServletContext().getServerInfo()+"</description>" );
        pw.println( "</server-info>" );

        // print statistics
        pw.println( "<statistics>" );
        sinceStartup.printXML(pw,true);
        Vector v = StatManager.getStatistic();
        for( int i=0; i<v.size(); i++ ) {
            StatUnit su = (StatUnit) v.elementAt(i);
            su.printXML(pw,false);
        }
        pw.println( "</statistics>" );

        // print properties
        pw.println( "<properties>" );

/*        PropertyManager.getIntProperty( "org.openlaszlo.iv.flash.maxThreads", 0 );
        PropertyManager.getIntProperty( "org.openlaszlo.iv.flash.garbageCollectAfterJobCount", 0 );
        PropertyManager.getBoolProperty( "org.openlaszlo.iv.flash.wrapAssets", false );
        PropertyManager.getProperty("org.openlaszlo.iv.flash.errorTemplate","bin/error.swt");
*/
        pw.println( "</properties>" );

        pw.println( "</jgenerator-server>" );

        pw.close();
    }

    /**
     * Online command executor.
     * <P>
     * Adds additional commands for online version of generator
     */
    public class OnlineCommandExecutor extends CommandExecutor {

    }

}

