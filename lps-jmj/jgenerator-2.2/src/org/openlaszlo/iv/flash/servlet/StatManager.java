/*
 * $Id: StatManager.java,v 1.3 2002/02/24 02:10:19 skavish Exp $
 *
 * ==========================================================================
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

import java.io.*;
import java.net.*;
import java.util.*;

import javax.servlet.*;
import javax.servlet.http.*;

import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.cache.*;


/**
 * Generator servlet statistic
 */
public class StatManager {

    static class StatDaemon extends Thread {
        public StatDaemon() {}

        public void run() {
            for(;;) {
                Date date = new Date( System.currentTimeMillis() );
                int day = date.getDay();
                try {
                    Thread.sleep(1000*60); // sleep for 1 minute
                } catch( InterruptedException e ) {
                    continue;
                }
                long now = System.currentTimeMillis();
                today.setEndTime( now );
                save();
                date = new Date( now );
                if( day != date.getDay() ) {
                    synchronized( StatManager.class ) {
                        today = new StatUnit( now );
                        statistic.addElement(today);
                    }
                }
            }
        }
    }

    private static String statFileName;
    private static StatUnit sinceStartup;
    private static StatUnit today;
    private static Vector statistic;
    private static StatDaemon statDaemon;

    private static void save() {
        try {
            FileOutputStream out = new FileOutputStream( statFileName );
            ObjectOutputStream p = new ObjectOutputStream( out );
            p.writeObject( statistic );
            p.flush();
            out.close();
        } catch( IOException e ) {
            Log.logRB(e);
        }
    }

    private synchronized static void load() {
        try {
            FileInputStream in = new FileInputStream( statFileName );
            ObjectInputStream p = new ObjectInputStream( in );
            statistic = (Vector) p.readObject();
            p.close();
            in.close();
        } catch( FileNotFoundException e ) {
            statistic = new Vector();
        } catch( Exception e ) {
            Log.logRB(e);
            statistic = new Vector();
        }
        // find today's stat
        today = null;
        Date date = new Date( System.currentTimeMillis() );
        int tday = date.getDay();
        for( int i=0; i<statistic.size(); i++ ) {
            StatUnit unit = (StatUnit) statistic.elementAt(i);
            date = new Date( unit.getStartTime() );
            if( tday == date.getDay() ) {
                today = unit;
                break;
            }
        }
        if( today == null ) {
            today = new StatUnit( System.currentTimeMillis() );
            statistic.addElement( today );
        }
    }

    public static void init() {
        sinceStartup = new StatUnit( System.currentTimeMillis() );
        statFileName = PropertyManager.getProperty("org.openlaszlo.iv.flash.statFileName","logs/stat");
        if( statFileName != null ) {
            File statFile = Util.getSysFile( statFileName );
            statFileName = statFile.getAbsolutePath();
        }
        load();
        statDaemon = new StatDaemon();
        statDaemon.start();
    }

    public static StatUnit getSinceStartup() {
        return sinceStartup;
    }

    public static Vector getStatistic() {
        return statistic;
    }

    /**
     * Add statistic for the given request
     */
    public static void addRequest( String fileName, int size, long processTime, long totalTime ) {
        sinceStartup.addRequest(fileName, size, processTime, totalTime);
        today.addRequest(fileName, size, processTime, totalTime);
    }
}

