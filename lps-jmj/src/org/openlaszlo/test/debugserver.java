/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

import java.net.*;
import java.io.*;

public class debugserver {
    static boolean closed = false;

    public static void main(String[] args) throws IOException {

        ServerSocket serverSocket = null;
        int port= 5559; 
        if (args.length > 0) {
            try {
                port = Integer.parseInt(args[0]);
            } catch (NumberFormatException e) {
                System.err.println("could not parse port number "+args[0]);
            }
        }

        while (true) {

            try {
                serverSocket = new ServerSocket(port);
            } catch (IOException e) {
                System.err.println("Could not listen on port "+port);
                System.exit(1);
            }



            closed = false;
            try {
                System.out.println("Listening on port "+port);
                final Socket clientSocket = serverSocket.accept();
                 
                System.out.println("Accepting connection on port "+port);

                PrintWriter out = new PrintWriter(clientSocket.getOutputStream(), true);
                final BufferedReader in = new BufferedReader(
                    new InputStreamReader(
                        clientSocket.getInputStream()));
                String inputLine, outputLine;

                int seqnum = 1;

                // socket reader thread
                new Thread() {
                    public void run() {   
                        try {
                            FileWriter of = new FileWriter("lzdebug.log");
                            PrintWriter outf = new PrintWriter(of);
                            while (true) {
                                try {
                                    int ch = in.read();
                                    if (ch == -1) {
                                        debugserver.closed = true;
                                        return;
                                    }
                                    if (ch == 0) {
                                        System.out.println("");
                                        outf.println("");
                                        outf.flush();
                                    } else {
                                        System.out.write(ch);
                                        outf.write(ch);
                                    }
                                    if (clientSocket.isClosed()) {
                                        debugserver.closed = true;
                                        return;
                                    }
                                } catch (IOException e) {
                                    System.out.println(e);
                                    debugserver.closed = true;
                                    return;
                                }
                            }
                        } catch (IOException e) { }
                    }
                }.start();

                // If there are command line args, open each one as a filename and send as exec
                if (args.length > 1) {
                    for (int n = 1; n < args.length; n++) {
                        String fname = args[n];
                        try {
                            FileInputStream fis = new FileInputStream(fname);
                            byte b[] = new byte[fis.available()];
                            fis.read(b);
                            String cmd = new String(b);
                            System.out.println("Sent: "+cmd);
                            out.write("<exec seq='"+(seqnum++)+"'>"+escapeXml(cmd)+"</exec>\000");
                            out.flush();
                        } catch (IOException e) {
                            System.out.println(e);
                        }
                    }
                } 

                BufferedReader tty = new BufferedReader(new InputStreamReader(System.in));
                String expr;

                while (!closed && (expr = tty.readLine()) != null) {
                    if (expr.trim().equals("")) continue;
                    out.write("<eval seq='"+(seqnum++)+"'>"+ escapeXml(expr)+"</eval>\000");
                    out.flush();
                }


                out.close();
                in.close();
                clientSocket.close();
                serverSocket.close();
                System.out.println("Connection closed");
            } catch (RuntimeException e) {
                System.out.println("Exception "+e);
            }
        }
    }

    /**
     * Escape the 5 entities defined by XML. 
     * These are: '<', '>', '\', '&', '"'.
     * 
     * @param s an xml string
     * @return an escaped xml string
     */
    public static String escapeXml(String s) {
        if (s == null) return null;
        StringBuffer sb = new StringBuffer();
        for(int i=0; i<s.length(); i++) {
            char c = s.charAt(i);
            if (c == '<') {
                sb.append("&lt;");
            } else if (c == '>') {
                sb.append("&gt;");
            } else if (c == '\'') {
                sb.append("&apos;");
            } else if (c == '&') {
                sb.append("&amp;");
            } else if (c == '"') {
                sb.append("&quot;");
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }


}
