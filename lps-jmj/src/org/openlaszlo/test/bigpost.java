/* *****************************************************************************
 * bigpost.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/


package org.openlaszlo.test;

import java.io.*;
import java.net.Socket;

import org.apache.commons.httpclient.*;
import org.apache.commons.httpclient.methods.*;
import org.openlaszlo.utils.FileUtils;


public class bigpost {
   static public void main (String args[]) {
       Socket socket = null;
       int i = 0;
       int j = 0;

       try {
           /* 
           String surl = args[0];
           OutputStream out = new FileOutputStream(args[1]);

           PostMethod pm = new PostMethod();

           int s = Integer.parseInt(args[2]);

           byte bytes[] = new byte[s];
           String big = new String(bytes);
           pm.setParameter("junk", big);
           pm.setParameter("lzt", "data");
           pm.setParameter("reqtype", "POST");
           pm.setParameter("cache", "false");
           pm.setParameter("sendheaders", "false");
           pm.setParameter("ccache", "false");
           pm.setParameter("url", "http://localhost:8080/lps/build.xml");

           pm.setPath(surl);

           HostConfiguration hcfg = new HostConfiguration();
           URI uri = new URI(surl);
           hcfg.setHost(uri);

           HttpClient htc = new HttpClient();
           htc.setHostConfiguration(hcfg);

           int rc = htc.executeMethod(hcfg, pm);
           System.err.println("status: " + rc);
           InputStream in = pm.getResponseBodyAsStream();
           FileUtils.send(in, out);
           out.close();
           */

         String host = args[0];
         int port = Integer.parseInt(args[1]);
         String uri = args[2];

         OutputStream outf = new FileOutputStream(args[3]);
         socket = new Socket(host, port);

         System.out.println("Made socket to " + host + ":" + port);

         //int length = Integer.MAX_VALUE;
         int length = Integer.parseInt(args[4]);
         Writer out = new OutputStreamWriter(
            socket.getOutputStream(), "ISO-8859-1");
         out.write("POST " + uri + " HTTP/1.1\r\n");
         out.write("Host: " + host + ":" + port + "\r\n");
         out.write("User-Agent: bigpost\r\n");
         out.write("Content-type: application/x-www-form-urlencoded\r\n");
         out.write("Content-length: " + length + "\r\n");
         out.write("\r\n");
         out.flush();
         out.write("lzt=data\r\n");
         out.write("reqtype=POST\r\n");
         out.write("cache=false\r\n");
         out.write("ccache=false\r\n");
         out.write("sendheaders=false\r\n");
         out.write("url=http://localhost:8080/lps/build.xml?x=");
         while(true) {
             out.write("x");
             if (i % 1000 == 0) {
                 out.flush();
                 System.err.println("" + i);
                 j = i;
             }
             i++;
         }

         //out.write("\r\n");
         //out.flush();

         // FileUtils.send(socket.getInputStream(), outf);

       } catch (Exception e) {
           System.err.println("bytes sent: " + i);
           System.err.println("bytes flushed: " + j);
           e.printStackTrace();
       } finally {
         try {
             socket.close();
         } catch (IOException e) {
         }
       }
   } 
}
