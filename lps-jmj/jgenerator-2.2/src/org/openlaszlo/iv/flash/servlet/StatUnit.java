/*
 * $Id: StatUnit.java,v 1.4 2002/04/26 03:38:54 skavish Exp $
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
import java.text.*;

public class StatUnit implements Serializable {

    public static final long serialVersionUID = -6212560619436466903L;

    private long startTime;
    private long endTime;
    private long totalSize = 0;
    private long totalProcessTime = 0;
    private long totalTotalTime = 0;
    private int totalRequests = 0;
    private int maxSize = Integer.MIN_VALUE;
    private String maxFile = "";
    private int minSize = Integer.MAX_VALUE;
    private String minFile = "";

    public StatUnit() {
    }

    public StatUnit( long startTime ) {
        this.startTime = startTime;
    }

    public void print( PrintWriter p ) {
        SimpleDateFormat formatter = new SimpleDateFormat ("MM/dd/yyyy hh:mm:ss z");
        String startStr = formatter.format( new Date(startTime) );
        String endStr = endTime == 0? "present": formatter.format( new Date(endTime) );
        p.print( "Statistics for period from "+startStr+" to "+endStr+"<br>" );
        p.print( "  Number of requests "+totalRequests+"<br>" );
        p.print( "  Total size "+totalSize+" bytes<br>" );
        p.print( "  Total time "+totalTotalTime+"ms<br>" );
        p.print( "  Max size/file "+maxSize+"/'"+maxFile+"'<br>" );
        p.print( "  Min size/file "+minSize+"/'"+minFile+"'<br>" );
        p.print( "  Average size "+getAverageSize()+" bytes<br>" );
        p.print( "  Average processing time "+getAverageProcessTime()+"ms<br>" );
        p.print( "  Average total processing time "+getAverageTotalTime()+"ms<br>" );
    }

    public void printVars( PrintWriter p ) {
        SimpleDateFormat formatter = new SimpleDateFormat ("MM/dd/yyyy hh:mm:ss z");
        String startStr = formatter.format( new Date(startTime) );
        long endTm = endTime == 0? System.currentTimeMillis(): endTime;
        String endStr = formatter.format( new Date(endTm) );
        p.print( "&startTime=" + URLEncoder.encode(startStr) );
        p.print( "&endTime=" + URLEncoder.encode(endStr) );
        p.print( "&numRequests=" + totalRequests  );
        p.print( "&totalSize=" + totalSize );
        p.print( "&totalTime=" + totalTotalTime );
        p.print( "&maxSize=" + maxSize );
        p.print( "&maxFile=" + URLEncoder.encode(maxFile) );
        p.print( "&minSize=" + minSize );
        p.print( "&minFile=" + URLEncoder.encode(minFile) );
        p.print( "&averageSize=" + getAverageSize()  );
        p.print( "&averageProcessTime=" + getAverageProcessTime()  );
        p.print( "&averageTotalTime=" + getAverageTotalTime() );
    }

    public void printXML( PrintWriter p, boolean current ) {
        if( current ) {
            p.println( "<stat-block current=\"true\">" );
        } else {
            p.println( "<stat-block>" );
        }
        SimpleDateFormat formatter = new SimpleDateFormat ("MM/dd/yyyy hh:mm:ss z");
        String startStr = formatter.format( new Date(startTime) );
        long endTm = endTime == 0? System.currentTimeMillis(): endTime;
        String endStr = formatter.format( new Date(endTm) );
        p.println( "<start-time>"+startStr+"</start-time>" );
        p.println( "<end-time>"+endStr+"</end-time>" );
        p.println( "<requests-num>"+totalRequests+"</requests-num>" );
        p.println( "<total-size>"+totalSize+"</total-size>" );
        p.println( "<total-time>"+totalTotalTime+"</total-time>" );
        p.println( "<max-size>"+maxSize+"</max-size>" );
        p.println( "<max-file>"+maxFile+"</max-file>" );
        p.println( "<min-size>"+minSize+"</min-size>" );
        p.println( "<min-file>"+minFile+"</min-file>" );
        p.println( "<average-time>"+getAverageProcessTime()+"</average-time>" );
        p.println( "<average-size>"+getAverageSize()+"</average-size>" );
        p.println( "</stat-block>" );
    }

    public synchronized void addRequest( String fileName, int size, long processTime, long totalTime ) {
        totalSize += size;
        totalProcessTime += processTime;
        totalTotalTime += totalTime;
        totalRequests++;
        if( size > maxSize ) {
            maxSize = size;
            maxFile = fileName;
        }
        if( size < minSize ) {
            minSize = size;
            minFile = fileName;
        }
    }

    public void setStartTime( long time ) {
        startTime = time;
    }
    public long getStartTime() {
        return startTime;
    }
    public void setEndTime( long time ) {
        endTime = time;
    }
    public long getEndTime() {
        return endTime;
    }
    public int getAverageSize() {
        if( totalRequests == 0 ) return 0;
        return (int) (totalSize / totalRequests);
    }
    public long getAverageProcessTime() {
        if( totalRequests == 0 ) return 0;
        return totalProcessTime / totalRequests;
    }
    public long getAverageTotalTime() {
        if( totalRequests == 0 ) return 0;
        return totalTotalTime / totalRequests;
    }
}

