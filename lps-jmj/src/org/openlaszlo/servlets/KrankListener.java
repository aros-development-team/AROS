/******************************************************************************
 * KrankListener.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.servlets;

import java.net.*;
import java.io.*;
import org.jdom.*;
import org.jdom.input.*;
import org.jdom.output.*;
import org.openlaszlo.compiler.CompilationError;
import org.openlaszlo.server.LPS;
import java.util.*;

import org.openlaszlo.utils.FileUtils;

import org.apache.log4j.Logger;
/**
   Listen on port 4444 for krank serialization data, write to tmp file

*/

public class KrankListener extends Thread {
    private static Logger  mLogger = Logger.getLogger(KrankListener.class);
    public boolean busy = false;
    public String appname = "";
    // record how long the run took
    public long starttime;
    public long duration;
    public String appQueryString = "";

    public static final String IDLE = "IDLE";
    public static final String BUSY = "BUSY";
    public static final String FINISHED = "FINISHED";
    public static final String ABORTED = "ABORTED";
    public String state = IDLE;

    private Socket clientSocket;
    private ServerSocket serverSocket;


    ////////////////////////////////////////////////////////////////
    // args

    String prefix;
    File xmlFile;
    File krankedSWFfilecopy;
    File krankedSWF;
    File basepath;
    File targetSWF;
    File targetSWFgz;
    Properties myprops;
    int krankPortNum = 4444;


    public KrankListener() {};

    public KrankListener(
        String prefix,
        File xmlFile,
        File krankedSWF,
        File krankedSWFfilecopy,
        File basepath,
        File targetSWF,
        File targetSWFgz,
        Properties myprops) {

        this.prefix = prefix;
        this.xmlFile = xmlFile;
        this.krankedSWF = krankedSWF;
        this.krankedSWFfilecopy = krankedSWFfilecopy;
        this.basepath = basepath;
        this.targetSWF = targetSWF;
        this.targetSWFgz = targetSWFgz;
        this.myprops = myprops;
        this.krankPortNum = LPS.getKrankPort();
    }


    ////////////////////////////////////////////////////////////////


    // Maybe the only way to interrupt a thread which is waiting on I/O is
    // to close the socket. 
    public void closeSocket() {
        try {
            if (clientSocket != null) {
                clientSocket.close();
            } 
        } catch (IOException e) { }
            
        try {
            if (serverSocket != null) {
                serverSocket.close();
            } 
        } catch (IOException e) { }
    }


    public void setState(String s) {
        state = s;
    }

    public String getState() {
        return state;
    }

    public boolean isBusy () {
        return state.equals(BUSY);
    }

    public boolean isFinished () {
        return state.equals(FINISHED);
    }

    public boolean isAborted () {
        return state.equals(ABORTED);
    }

    public boolean isIdle () {
        return state.equals(IDLE);
    }

    public void setBusy (boolean b) {
        if (b) {
            setState(BUSY);
        } else {
            setState(IDLE);
        }
    }

    public long starttime () {
        return starttime;
    }
    public long getDuration () {
        return duration;
    }

    public void setDuration (long d) {
        duration = d;
    }

    public void setAppname (String s) {
        appname = s;
    }
    public String getAppname () {
        return appname;
    }

    public void setAppQueryString (String s) {
        appQueryString = s;
    }
    public String getAppQueryString () {
        return appQueryString;
    }

    public void run() {
        try {
            setState(BUSY);
            mLogger.debug("starting KrankListener on app "+prefix+" now");
            setAppname(prefix);
            listen(xmlFile);

            // Ask the cache for the location of the kranked swf
            // file but make sure it's not gzip'd. The file should
            // exist because we wouldn't have gotten here if the
            // server hadn't delivered it to the client, and the
            // client ran it and completed sending the xml
            // serialization data back to the server listener. If
            // the file doesn't exist for some reason, then, hey, no
            // big deal, we'll throw an exception, and better luck
            // next time.

            mLogger.info("kranked swf file is at "+krankedSWF.getAbsolutePath());

            FileInputStream in = new FileInputStream(krankedSWF);
            // Copy to source tree
            FileOutputStream out = new FileOutputStream(krankedSWFfilecopy);
            FileUtils.send(in, out);
            FileUtils.close(out);
            FileUtils.close(in);

            mLogger.debug("basepath = "+basepath);
            new org.openlaszlo.sc.Regenerator().compile(myprops, new String[] { basepath.getAbsolutePath()});
            setState(FINISHED);
            mLogger.debug("kranking on "+prefix+" finished!");

        } catch (Exception e) {
            mLogger.error("Exception caught while invoking KrankListener.listen("+xmlFile+"): "+e+":"+e.getMessage());
            setState(ABORTED);
            throw new CompilationError("Error invoking KrankListener.listen("+xmlFile+"): "+e+":"+e.getMessage());
        } finally {
            // clean up
            xmlFile.deleteOnExit();
            krankedSWFfilecopy.deleteOnExit();
        }
    }


    public  void listen (File outputFile) throws IOException, CompilationError {
        mLogger.info("KrankListener.listen("+outputFile+")");
        starttime = System.currentTimeMillis();
        serverSocket = null;
        try {
            serverSocket = new ServerSocket(krankPortNum);
        } catch (IOException e) {
            mLogger.error("Could not listen on port: "+krankPortNum+".");
            throw new CompilationError("Krank listener could not listen on port "+krankPortNum+" "+outputFile.getAbsolutePath());
        }

        mLogger.info("listening for connection on port "+krankPortNum);

        clientSocket = null;
        try {
            clientSocket = serverSocket.accept();
        } catch (IOException e) {
            mLogger.error("Accept failed.");
            throw new CompilationError("Krank listener accept() failed for" + outputFile.getAbsolutePath());
        }

        mLogger.info("accept on port "+krankPortNum+", sending output to '"+outputFile.getAbsolutePath()+"'");
        // TODO: [2003-8-25 hqm] We open the stream in CP1252, windows
        // charset, because that's probably what we wrote the Flash 5
        // swf in. For Flash 6, this will be UTF-8.
        BufferedReader in = new BufferedReader(
                new InputStreamReader(
                clientSocket.getInputStream(), "Cp1252"));
     
        PrintWriter sockout = new PrintWriter(clientSocket.getOutputStream());
        String inputLine;
        FileOutputStream fs = new FileOutputStream(outputFile);
        PrintWriter out = new PrintWriter(fs);

        try {
            int n = 0;
            int NLINES = 500;
            while ((inputLine = in.readLine()) != null) {
                inputLine = inputLine.replace((char)0, ' ');
                //mLogger.debug("SOCKREAD: "+inputLine);
                n++;
                // Perform some XML fixup on the raw input line
                out.println(inputLine);
                if ((n % NLINES) == 0) {
                    mLogger.info("...read "+n+" lines...");
                }
                // </_top> indicates that the xml file is complete
                if (inputLine.indexOf("</_top>") >= 0) {
                    mLogger.debug("got </_top>, closing socket");
                    break;
                }
            }
        } catch (IOException e) {
            setState(ABORTED);
            throw new CompilationError(e);
        } finally {
            sockout.write("OK"+'\000');
            sockout.flush();
            out.flush();
            out.close();
            in.close();

            clientSocket.close();
            serverSocket.close();
        }
        mLogger.info("data stream from client closed, output is in '"+outputFile.getAbsolutePath()+"'");
    }
}
