/* *****************************************************************************
 * FileUtils.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.utils;

import org.openlaszlo.server.LPS;

import java.io.File;
import java.io.RandomAccessFile;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.FilenameFilter;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileOutputStream;
import java.io.FilterOutputStream;
import java.io.FilterInputStream;
import java.io.ByteArrayOutputStream;
import java.io.PushbackInputStream;

import java.io.ByteArrayInputStream;

import java.io.Writer;
import java.io.Writer;
import java.io.Reader;
import java.io.StringWriter;
import java.util.*;
import java.util.zip.*; 

import org.apache.log4j.*;
import org.apache.oro.text.regex.*;

// A dir is absolute if it begins with "" (the empty string to
// the left of the initial '/'), or a drive letter.
class AbsolutePathnameTester {
     static boolean test(List dirs) {
        if (!dirs.isEmpty()) {
            String dir = (String) dirs.get(0);
            // Add '/' since c:/ will be split to c:, which
            // isn't absolute.  Also turns "" into "/" which
            // is absolute, so the UNIX case can share this
            // test.
            return new File(dir + "/").isAbsolute();
        }
        return false;
    }
}

/**
 * A utility class containing file utility functions.
 *
 * @author Oliver Steele
 * @author Eric Bloch
 */
public abstract class FileUtils {

    private static Logger mLogger = Logger.getLogger(FileUtils.class);

    // TODO: [2003-09-23 bloch] could build out the encoder a bit more
    public static final String GZIP = "gzip";
    public static final String DEFLATE = "deflate";
    public static final int GZIP_HEADER_LENGTH = 10;
    public static final int MIN_GZIP_PEEK = 2;
    public static final int MIN_SWF_PEEK = 9;

    public static int BUFFER_SIZE = 10240;
    public static int THROTTLE = 0;

    static {
        try {
            String throttle = LPS.getProperty("lps.throttle");
            BUFFER_SIZE = Integer.parseInt(LPS.getProperty("buffer.size", "10240"));
            if (throttle != null) {
                THROTTLE = Integer.parseInt(throttle);
                BUFFER_SIZE = 1024;
                mLogger.info("throttle " + throttle);
            }


        } catch (RuntimeException e) {
            mLogger.error("Exception initializing FileUtils", e);
        } catch (Exception e) {
            mLogger.error("Exception initializing FileUtils", e);
        }
    } 

    /** Returns the contents of <var>file</var> as a byte[].
     * @param file a File
     * @return a byte[]
     * @throws IOException if an error occurs
     */
    public static byte[] readFileBytes(File file) 
        throws IOException
    {
        java.io.InputStream istr = new java.io.FileInputStream(file);
        byte bytes[] = new byte[istr.available()];
        istr.read(bytes);
        istr.close();
        return bytes;
    }

    /** Returns the contents of <var>file</var> as a String, using UTF-8 character encoding <var>encoding</var>.
     * @param file a File
     * @return a String
     * @throws IOException if an error occurs
     */

    public static String readFileString(File file) 
        throws IOException
    {
        byte data[] = readFileBytes(file);
        return new String(data, "UTF-8");
    }

    /** Returns the contents of <var>file</var> as a String, using character encoding <var>encoding</var>.
     * @param file a File
     * @param encoding a character set encoding name
     * @return a String
     * @throws IOException if an error occurs
     */

    public static String readFileString(File file, String defaultEncoding) 
        throws IOException
    {
        Reader reader = makeXMLReaderForFile(file.getAbsolutePath(), defaultEncoding);
        StringWriter s = new StringWriter();
        send(reader, s);
        reader.close();
        return s.toString();
    }


    /** Attempt to deduce the encoding of an XML file, by looking for the "encoding" attribute in the
     * XML declaration.
     * Default is to return "UTF-8"
     * @param pathname a filename
     * @return the encoding name
     * @throws IOException if an error occurs
     */
    public static String getXMLEncodingFromFile(String pathname, String defaultEncoding)
      throws IOException {
        java.io.FileInputStream ifs = new java.io.FileInputStream(pathname);
        ByteArrayOutputStream bout = new ByteArrayOutputStream();
        send(ifs, bout);
        Perl5Matcher matcher = new Perl5Matcher();
        try {
            Perl5Compiler compiler = new Perl5Compiler();
            Pattern pattern = compiler.compile("[^<]*\\s*<[?]xml\\s+[^>]*encoding=[\"'](.*)['\"][^>]*?>");
            if (matcher.contains(new String(bout.toByteArray()), pattern)) {
                MatchResult result = matcher.getMatch();
                String encoding = result.group(1);
                return encoding;
            } else {
                return "UTF-8";
            }
        } catch (MalformedPatternException e) {
            throw new RuntimeException(e.getMessage());
        }
    }

    /**
     * Set up a reader for an XML file with the correct charset encoding, and strip
     * out any Unicode Byte Order Mark if there is one present. We need to scan the file
     * once to try to parse the charset encoding in the XML declaration.
     * @param pathname the name of a file to open
     * @return reader stream for the file
     */
    public static Reader makeXMLReaderForFile (String pathname, String defaultEncoding)
      throws IOException {
        String encoding = getXMLEncodingFromFile(pathname, defaultEncoding);
        java.io.FileInputStream ifs = new java.io.FileInputStream(pathname);
        java.io.PushbackInputStream pbis = new java.io.PushbackInputStream(ifs, 1024);
        // We will ignore the byte order mark encoding for now,
        // hopefully no one is going to be using UTF16. I don't want
        // to deal right now with the case where the XML encoding
        // directive conflicts with the byte order mark.
        FileUtils.stripByteOrderMark(pbis);
        return new java.io.InputStreamReader(pbis, encoding);
    }
    
    /** Read a (pushback-able) byte input stream looking for some form of
        Unicode Byte Order Mark Defaults. If found, strip it out.
     * @param pbis a byte input stream
     * @return encoding name, defaults to null if we have no byte order mark
     */
    public static String stripByteOrderMark (PushbackInputStream pbis) throws IOException {
        // We need to peek at the stream and if the first three chars
        // are a UTF-8 or UTF-16 encoded BOM (byte order mark) we will
        // discard them. 
        int c1 = pbis.read();
        int c2 = pbis.read();
        int c3 = pbis.read();
        if (c1 == 0xFF & c2 == 0xFE) {
            // UTF16 Big Endian BOM
            // discard the first two chars
            pbis.unread(c3);
            return "UTF-16BE";
        } else if (c1 == 0xFE & c2 == 0xFF) {
            // UTF16 Little Endian BOM
            // discard the first two chars
            pbis.unread(c3);
            return "UTF-16LE";
        } else if (c1 == 0xEF && c2 == 0xBB && c3 == 0xBF) {
            // UTF-8 BOM
            // discard all three chars
            return "UTF-8";
        } else {
            // Otherwise put back the chars we just read and proceed
            pbis.unread(c3);
            pbis.unread(c2);
            pbis.unread(c1);
            return null;
        }
    }

    /**
     * @param file file to read
     * @return size of file
     * @throws FileNotFoundException
     * @throws IOException
     */
    static public long fileSize(File file) 
        throws IOException, FileNotFoundException {

        RandomAccessFile raf = null;
        try {
            raf = new RandomAccessFile(file, "r");
            return raf.length();
        } finally {
            if (raf != null) {
                try {
                    raf.close();
                } catch (Exception e) {
                    mLogger.warn("ignoring exception while closing random access file: ", e);
                }
            }
        }
    }
    /**
     * @param file file to read
     * @return size of file or -1 if it can't be determined.
     */ 
    static public long getSize(File file) {
        try {
            return fileSize(file);
        } catch (Exception e) {
            return -1;
        }
    }

    /** Returns the base name (without the extension) of
     * <var>pathname</var>.  For example,
     * <code>FileUtils.getBase("a/b.c") == "a/b"</code>.
     * @param pathname a String
     * @return a String
     */
    public static String getBase(String pathname) {
        int pos = pathname.lastIndexOf('.');
        if (pos >= 0) {
            return pathname.substring(0, pos);
        } else {
            return pathname;
        }
    }

    /** 
     * @return the ending extension
     * @param pathname a string
     */
    public static String getExtension(String pathname) {
        int pos = pathname.lastIndexOf('.');
        if (pos >= 0 && pos != pathname.length() - 1) {
            return pathname.substring(pos+1);
        }
        return "";
    }

    /** Remove the : from Windows Drive specifier found
     *  in Absolute path names.
     *
     * @param src
     * @return a fixed string
     */
    public static String fixAbsWindowsPaths(String src) {

        // See if we're on Windows
        if (File.separatorChar == '/')
            return src;

        // One char strings
        if (src.length() < 2)
            return src;

        // Check to make sure we have a Windows absolute path
        if (src.charAt(1) != ':') 
            return src;

        // Sanity check to make sure we really have a drive specifier
        char c = src.charAt(0);
        int t = Character.getType(c);
        boolean isAscii = (t == Character.UPPERCASE_LETTER ||
                           t == Character.LOWERCASE_LETTER);
        if (!isAscii) {
            return src;
        } 
        
        // Remove the : that was the 2nd character.
        return src.substring(0,1) + src.substring(2);
    }

    /**
     * Insure a file and all parent directories exist.
     * @param file the file
     * @throws IOException
     */
    public static void makeFileAndParentDirs(File file)
        throws IOException {
        File dir = file.getParentFile();
        if (dir != null)
            dir.mkdirs();
        file.createNewFile();
    }

    /** Delete an entire directory tree. Named after the Python
     * function.
     * @return true if full removal of cache was successful, otherwise false.
     */
    public static boolean rmTree(File file) {
        boolean delTreeOk = true;
        boolean delFileOk = true;
        File[] files = file.listFiles(); // null if not a directory
        if (files != null) {
            for (Iterator iter = Arrays.asList(files).iterator();
                 iter.hasNext(); ) {
                if (! rmTree((File) iter.next()))
                    delTreeOk = false;
            }
        }
        delFileOk = file.delete();

        return (delFileOk && delTreeOk);
    }
    
    /**
     * Read the input stream and write contents to output stream.
     * @param input stream to read
     * @param output stream to write
     * @return number of bytes written to output
     * @throws IOException if there's an error reading or writing
     * the steams
     */
    public static int send(InputStream input, OutputStream output) 
        throws IOException {

        int available = input.available();
        int bsize;
        if (available == 0) {
            bsize = BUFFER_SIZE;
        } else {
            bsize = Math.min(input.available(), BUFFER_SIZE);
        }
        return send(input, output, bsize);
    }
    
    /**
     * Exception when there's an error writing a stream
     */
    public static class StreamWritingException  extends IOException {
        public StreamWritingException (String s)
        {
            super(s);
        }
    }

    /**
     * Exception when there's an error reading a stream
     */
    public static class StreamReadingException  extends IOException {
        public StreamReadingException (String s)
        {
            super(s);
        }
    }

    
    /**
     * Read the input character stream and write contents to output stream.
     * @param input stream to read
     * @param output stream to write
     * @return number of bytes written to output
     * @throws IOException if there's an error reading or writing
     * the steams
     */
    public static int send(Reader input, Writer output) 
        throws IOException {
        int bsize = BUFFER_SIZE;
        return send(input, output, bsize);
    }


    /**
     * Read the input stream and write contents to output
     * stream using a buffer of the requested size.
     * @param input stream to read
     * @param output stream to write
     * @param size size of buffer to use
     * @return number of bytes written to output
     * @throws IOException if there's an error reading or writing
     * the steams
     */
    public static int send(Reader input, Writer output, int size) 
        throws IOException {
        int c = 0;
        char[] buffer = new char[size];
        int b = 0;
        while((b = input.read(buffer)) > 0) {
            c += b;
            output.write(buffer, 0, b);
        }
        return c;
    }


    /**
     * Read the input stream and write contents to output
     * stream using a buffer of the requested size.
     * @param input stream to read
     * @param output stream to write
     * @param size size of buffer to use
     * @return number of bytes written to output
     * @throws IOException if there's an error reading or writing
     * the steams
     */
    public static int send(InputStream input, OutputStream output, int size) 
        throws IOException {
        int c = 0;
        byte[] buffer = new byte[size];
        int b = 0;
        while((b = input.read(buffer)) > 0) {
            c += b;
            output.write(buffer, 0, b);
        }
        return c;
    }

    /**
     * Read the input stream and write contents to output stream.
     * @param input stream to read
     * @param output stream to write
     * @return number of bytes written to output
     * @throws StreamWritingException if there's an error writing output
     * @throws StreamReadingException if there's an error reading input
     */
    public static int sendToStream(InputStream input, OutputStream output)
        throws IOException {

        int available = input.available();
        int bsize;
        if (available == 0) {
            bsize = BUFFER_SIZE;
        } else {
            bsize = Math.min(input.available(), BUFFER_SIZE);
        }
        return sendToStream(input, output, bsize);
    }

    /**
     * Read the input stream and write contents to output
     * stream using a buffer of the requested size.
     * @param input stream to read
     * @param output stream to write
     * @param size size of buffer to use
     * @return number of bytes written to output
     * @throws StreamWritingException if there's an error writing output
     * @throws StreamReadingException if there's an error reading input
     */
    public static int sendToStream(InputStream input,
            OutputStream output, int size)
        throws IOException {
        int c = 0;
        byte[] buffer = new byte[size];
        int b = 0;
        while(true) {
            try {
                // Until end of stream
                if ((b = input.read(buffer)) <= 0) {
                    return c;
                }
            } catch (IOException e) {
                throw new StreamReadingException(e.getMessage());
            }
            c += b;
            try {
                output.write(buffer, 0, b);
            } catch (IOException e) {
                throw new StreamWritingException(e.getMessage());
            }
            if (THROTTLE != 0) {
                try {
                    mLogger.info("Sleeping " + THROTTLE + " msecs ");
                    Thread.currentThread().sleep(THROTTLE);
                } catch (InterruptedException e) {
                    mLogger.warn("interrupted during sleep", e);
                }
            }
        }
    }

    /**
     * Read the input stream and write contents to output stream.
     * @param input stream to read
     * @param output stream to write
     * @return number of bytes written to output
     * @throws IOException
     */
    public static int escapeHTMLAndSend(InputStream input, OutputStream output) 
        throws IOException {

        int available = input.available();
        int bsize;
        if (available == 0) {
            bsize = BUFFER_SIZE;
        } else {
            bsize = Math.min(input.available(), BUFFER_SIZE);
        }
        return escapeHTMLAndSend(input, output, bsize);
    }
    /**
     * Read the input stream and write contents to output
     * stream using a buffer of the requested size.
     * @param input stream to read
     * @param output stream to write
     * @param size size of buffer to use
     * @param return number of bytes written to output
     * @throws IOException
     */
    public static int escapeHTMLAndSend(InputStream input, OutputStream output, int size) 
        throws IOException {
        int c = 0;
        byte[] buffer = new byte[size];
        byte[] buffer2 = new byte[size*4]; // big enuff to escape each char
        int b = 0;
        int b2 = 0;
        while((b = input.read(buffer)) > 0) {
            b2 = escapeHTML(buffer, b, buffer2);
            c += b2;
            output.write(buffer2, 0, b2);
        }
        return c;
    }

    /**
     * copy input into output escaping HTML chars
     * escaped.  Output buffer must be 4 x the input buffer size.
     *
     * @param ib input buffer
     * @param buflen input buffer size;
     * @param ob output buffer size;
     * @param return number of bytes written to ob
     */
    public static int escapeHTML(byte[] ib, int buflen, byte[] ob) {
        int bc = 0;
        for(int i = 0; i < buflen; i++) {
            if (ib[i] == '<') {
                ob[bc++] = '&';
                ob[bc++] = 'l';
                ob[bc++] = 't';
                ob[bc++] = ';';
            } else if (ib[i] == '>') {
                ob[bc++] = '&';
                ob[bc++] = 'g';
                ob[bc++] = 't';
                ob[bc++] = ';';
            } else {
                ob[bc++] = ib[i];
            }
        }

        return bc;
    }

    public static class RelativizationError extends Error {
        RelativizationError(String message) {
            super(message);
        }
    }

    /** Return a path that resolves to the same file relative to
     * dest as path does relative to source. Paths use '/' as
     * separators. source and dest are assumed to be directories. */
    public static String adjustRelativePath(String path,
                                            String source, String dest)
        throws RelativizationError
    {
        final String separator = "/";
        if (path.endsWith(separator)) {
            return path;
        }
        if (source.endsWith(separator)) {
            source = source.substring(0, source.length() - 1);
        }
        if (dest.endsWith(separator)) {
            dest = dest.substring(0, dest.length() - 1);
        }
        List sourcedirs = new Vector(Arrays.asList(StringUtils.split(source, separator)));
        List destdirs = new Vector(Arrays.asList(StringUtils.split(dest, separator)));
        normalizePath(sourcedirs);
        normalizePath(destdirs);
        // Remove the common prefix
        while (!sourcedirs.isEmpty() && !destdirs.isEmpty() &&
               ((String) sourcedirs.get(0)).equals((String) destdirs.get(0))) {
            sourcedirs.remove(0);
            destdirs.remove(0);
        }
        // If either dir is absolute, we can't relativize a file in
        // one to the other.  Do this after removing the common
        // prefix, since if both files are absolute and are not on
        // different drives (Windows only) then a pathname that's
        // relative to one can be rewritten relative to the other.
        //
        //System.err.println("" + sourcedirs + " -> " + AbsolutePathnameTester.test(sourcedirs));
        if (AbsolutePathnameTester.test(sourcedirs) ||
            AbsolutePathnameTester.test(destdirs)) {
            throw new RelativizationError(
              "can't create a pathname relative to " +dest +
              " that refers to a file relative to "+ source);
        }
        List components = new Vector();
        // '..'s to get out of the source directory...
        for (Iterator iter = sourcedirs.iterator(); iter.hasNext(); ) {
            iter.next();
            components.add("..");
        }
        // ...and directory names to get into the dest directory
        for (Iterator iter = destdirs.iterator(); iter.hasNext(); ) {
            components.add(iter.next());
        }
        components.add(path);
        return StringUtils.join(components, separator);
    }

    /**
     * @return the path to the given file, relative to the
     * given directory name.  If the file isn't inside the directory,
     * just return the entire file name.  Path separators are
     * modified to always be '/'.  
     */
    public static String relativePath(File file, String directoryName) {
        try {
            String fileName = file.getCanonicalPath().replace('\\', '/');
            String dirName = new File(directoryName).getCanonicalPath().replace('\\', '/');
            if (!fileName.startsWith(dirName)) {
                return fileName;
            }
            return fileName.substring(dirName.length());
        } catch (IOException ioe) {
            throw new ChainedException(ioe);
        }
    }

    /**
     * @return the path to the given file, relative to the
     * given directory name.  If the file isn't inside the directory,
     * just return the entire file name.  Path separators are
     * modified to always be '/'.  
     */
    public static String relativePath(String filename, String directoryName) {
        return relativePath(new File(filename), directoryName);
    }

    public static String toURLPath(File file) {
        // Can't call File.toURL, since it looks at the disk
        return StringUtils.join(StringUtils.split(file.getPath(),
                                                  file.separator),
                                "/");
    }

    /** Remove "." and non-top "..".  Destructive. */
    public static void normalizePath(List path) {
        for (int i = 0; i < path.size(); i++) {
            String component = (String) path.get(i);
            if (component.equals(".") || component.equals("")) {
                path.remove(i--);
            } else if (component.equals("..") && i > 0) {
                path.remove(i--);
                path.remove(i--);
            }
        }
    }

    /**
     * Encode the given file as requested to an outputFile.
     * @param inputFile file to be encoded
     * @param outputFile encoded file
     * @param encoding type of encoding to use ("gzip" or "deflate" supported).
     * @throws ChainedException if the requested encoding is not supported.
     */
    static public void encode(File inputFile, File outputFile, String encoding) 
        throws IOException {

        FileInputStream    in = null;

        try {    
            in = new FileInputStream(inputFile);
            encode(in, outputFile, encoding);
        } finally {
            if (in != null)
                in.close();
        }
    }

    /**
     * Decode the given file as requested to an outputFile.
     * @param inputFile file to be decoded
     * @param outputFile decoded file
     * @param encoding type of encoding to use ("gzip" or "deflate" supported).
     * @throws ChainedException if the requested encoding is not supported.
     */
    static public void decode(File inputFile, File outputFile, String encoding) 
        throws IOException {

        mLogger.debug("Starting decoding from " + encoding);
        FilterInputStream in = null;
        OutputStream out = new FileOutputStream(outputFile);
        InputStream ins = new FileInputStream(inputFile);

        try {    
            if (encoding.equals(GZIP)) {
                in = new GZIPInputStream(ins);
            } else if (encoding.equals(DEFLATE)) { 
                in = new InflaterInputStream(ins);
            } else { 
                String message = "Unsuppored encoding: " + encoding;
                mLogger.error(message);
                throw new ChainedException(message);
            } 
    
            // Note: use fixed size for buffer.  The send(in, out) method
            // will use in.available() to guess a good buffersize but
            // that won't work out well for these input streams.
            int b = FileUtils.send(in, out, BUFFER_SIZE);
            mLogger.debug("decoded into "  + b + " bytes" );
        } finally {
            close(in);
            close(ins);
            close(out);
        }
        mLogger.debug("Done decoding from " + encoding);
    }

    /**
     * Encode the given file as requested to an outputFile.
     * @param inputFile file to be encodeded
     * @param outputFile encoded file
     * @param encoding type of encoding to use ("gzip" or "deflate" supported).
     * @throws ChainedException if the requested encoding is not supported.
     */
    static public void encode(InputStream input, File outputFile, String encoding) 
        throws IOException {

        FileOutputStream   out = null;
        FilterOutputStream filter  = null;

        mLogger.debug("Encoding into " + encoding);

        try {    
            FileUtils.makeFileAndParentDirs(outputFile);

            out = new FileOutputStream(outputFile);

            if (encoding.equals(GZIP)) {
                filter = new GZIPOutputStream(out);
            } else if (encoding.equals(DEFLATE)) { 
                filter = new DeflaterOutputStream(out);
            } else { 
                String message = "Unsupported encoding: " + encoding;
                mLogger.error(message);
                throw new ChainedException(message);
            } 

            FileUtils.send(input, filter, BUFFER_SIZE);

        } finally {
            if (filter != null)
                close(filter);
            if (out != null)
                close(out);
        }

        mLogger.debug("Done encoding into " + encoding);
    }

    /**
     * Make sure you really want to ignore the exception
     * before using this.
     * @param in stream to close w/out throwing an exception
     */
    static public void close(InputStream in) {
        try {
            if (in != null) {
                in.close();
            }
        } catch (Exception e) {
        }
    }

    /**
     * Make sure you really want to ignore the exception
     * before using this.
     * @param out stream to close w/out throwing an exception
     */
    static public void close(OutputStream out) {
        try {
            if (out != null) {
                out.close();
            }
        } catch (Exception e) {
        }
    }

    /**
     * Make sure you really want to ignore the exception
     * before using this.
     * @param w write to close w/out throwing an exception
     */
    static public void close(Writer w) {
        try {
            if (w != null) {
                w.close();
            }
        } catch (Exception e) {
        }
    }

    /**
     * Remove all files in the given list that are immediate children
     * of this directory.  This does <b>not</b> recursively remove
     * files in subdirecetories.
     *
     * @return true if all files to be removed, could be removed
     */
    static public boolean deleteFiles(File dir, String[] fileNames) {
        boolean ok = true;

        for(int i = 0; i < fileNames.length; i++) {
            File file = new File(dir + File.separator + fileNames[i]);
            try {
                if (!file.isDirectory()) {
                    file.delete();
                }
            } catch (Throwable e) {
                mLogger.error("can't delete file: " + fileNames[i]);
                ok = false;
            }
        }
        return ok;
    }

    /**
     * Remove all files that are immediate children
     * of this directory.  This does <b>not</b> recursively remove
     * files in subdirecetories.
     *
     * @return true if all files to be removed, could be removed
     */
    static public boolean deleteFiles(File dir) {

        if (!dir.isDirectory()) {
            return false;
        }
        String[] fileNames = dir.list();

        return deleteFiles(dir, fileNames);
    }

    /**
     * Remove all files that start with the following name
     * in the given directory.  This does <b>not</b> recursively remove
     * files in subdirecetories.
     * @return true if all files to be removed, could be removed
     */
    static public boolean deleteFilesStartingWith(File dir, String name) {

        boolean ok = true;

        /**
         * Filter that selects all files that begin with a
         * given file name.  See java.io.FilenameFilter.
         */
        class StartsWithNameFilter implements FilenameFilter {
            final String _name;
        
            StartsWithNameFilter(String n) {
                _name = n;
            }
            public boolean accept(File d, String n) {
                return (n.startsWith(_name));
            }
        }

        String[] fileNames = dir.list(new StartsWithNameFilter(name));

        return deleteFiles(dir, fileNames);
    }
}
