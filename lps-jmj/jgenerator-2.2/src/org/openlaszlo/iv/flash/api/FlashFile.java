/*
 * $Id: FlashFile.java,v 1.11 2002/08/02 03:15:17 skavish Exp $
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

package org.openlaszlo.iv.flash.api;

import java.io.*;
import java.util.*;
import java.util.zip.*;
import java.awt.geom.Rectangle2D;
import org.openlaszlo.iv.flash.util.*;
import org.openlaszlo.iv.flash.url.*;

import org.openlaszlo.iv.flash.cache.*;
import org.openlaszlo.iv.flash.parser.*;
import org.openlaszlo.iv.flash.api.text.*;
import org.openlaszlo.iv.flash.api.image.*;
import org.openlaszlo.iv.flash.context.*;

/**
 * Class represents a Flash file (either .swt or .swf)
 *
 * @author Dmitry Skavish
 */
public class FlashFile {

    private String fileName;                // file name
    private String fileDir;                 // file directory

    private int version;                    // version of flash file format
    private Rectangle2D frameSize;          // size of the movie in twixels
    private int frameRate;                  // frame rate (number of frames/second left shifted by 8)
    private int fileSize;                   // file size in bytes
    private Script main;                    // main timeline

    private int processCount = 0;
    private boolean isTemplate = false;     // true - swt, false - swf
    private FlashFile defaultSymbolFile;    // file of default flash symbols

    private String encoding;                // default encoding for this file

    private boolean fullParsing;            // true - full parsing, lazy parsing is disabled

    private IVMap defsByID = new IVMap();   // all definitions defined in the file

    private Hashtable defsByName = new Hashtable();     // all definitions defined and named in the file

    private Hashtable exportTable = new Hashtable();    // table of all exported assets (definitions exported by ExportAssets)

    private Hashtable externalFiles = new Hashtable();  // all external media loaded from within the file

    private boolean isCompressed = false;   // true if the file is compressed (flash 6)

    public FlashFile() {}

    public String getFileName() {
        return fileName;
    }
    public void setFileName( String fileName ) {
        this.fileName = Util.translatePath(fileName);
    }

    public String getFileDir() {
        return fileDir;
    }
    public void setFileDir( String fileDir ) {
        this.fileDir = Util.translatePath(fileDir);
    }

    /**
     * Returns absolute file name
     *
     * @return absolute file name
     */
    public String getFullName() {
        return new File( fileDir, fileName ).getAbsolutePath();
    }

    public void setVersion( int version ) {
        this.version = version;
    }

    public void setFrameSize( Rectangle2D r ) {
        this.frameSize = r;
    }

    public void setFileSize( int size ) {
        this.fileSize = size;
    }

    public void setFrameRate( int fr ) {
        this.frameRate = fr;
    }

    public void setEncoding( String encoding ) {
        this.encoding = encoding;
    }

    public void setMainScript( Script main ) {
        this.main = main;
    }

    public void setCompressed( boolean isCompressed ) {
        this.isCompressed = isCompressed;
    }

    public boolean isCompressed() {
        return isCompressed;
    }

    public int getVersion() {
        return version;
    }

    public Rectangle2D getFrameSize() {
        return frameSize;
    }

    public int getFileSize() {
        return fileSize;
    }

    /**
     * Return framerate of the file
     * <P>
     * getFrameRate()>>8 = number of frames per second
     *
     * @return framerate of the file
     */
    public int getFrameRate() {
        return frameRate;
    }

    public String getEncoding() {
        return encoding;
    }

    public Script getMainScript() {
        return main;
    }

    public void setTemplate( boolean isTemplate ) {
        this.isTemplate = isTemplate;
    }

    public void setFullParsing( boolean fullParsing ) {
        this.fullParsing = fullParsing;
    }

    public boolean isFullParsing() {
        return fullParsing;
    }

    /**
     * Returns true if the file is Flash template, otherwise returns false
     * <p>
     * The criteria is not the file extention (.swt), but MM Generator special 'template' tag
     */
    public boolean isTemplate() {
        return isTemplate;
    }

    /**
     * Adds specified FlashDef into export table
     *
     * @param name   linkage name
     * @param def    specified definition
     */
    public void addDefInAssets( String name, FlashDef def ) {
        exportTable.put(name, def);
    }

    /**
     * Returns FlashDef from export table by its linkage name
     *
     * @param name   linkage name of asset
     * @return found asset or null
     */
    public FlashDef getDefInAssets( String name ) {
        return (FlashDef) exportTable.get(name);
    }

    /**
     * Returns definition by ID
     *
     * @param id     definition ID
     * @return definition or null
     */
    public FlashDef getDef( int id ) {
        return defsByID.get(id);
    }

    /**
     * Adds definition by ID
     *
     * @param def    definition to be added
     */
    public void addDef( FlashDef def ) {
        defsByID.add(def);
    }

    /**
     * Adds definition by name
     *
     * @param name   name of the definition
     * @param def    definition
     */
    public void addDefToLibrary( String name, FlashDef def ) {
        if( !PropertyManager.symCaseSensitive ) name = name.toUpperCase();
        defsByName.put(name, def);
    }

    /**
     * Finds definition by name in this file and all its external files (if any)
     *
     * @param name   name of the definition
     * @return found definition or null
     */
    public FlashDef getDefFromLibrary( String name ) {
        if( !PropertyManager.symCaseSensitive ) name = name.toUpperCase();
        FlashDef def = (FlashDef) defsByName.get(name);
        if( def != null ) return def;
        if( externalFiles.size() > 0 ) {
            Enumeration files = externalFiles.elements();
            while( files.hasMoreElements() ) {
                Object o = files.nextElement();
                if( o instanceof FlashFile ) {
                    FlashFile file = (FlashFile) o;
                def = file.getDefFromLibrary( name );
                if( def != null ) return def;
            }
        }
        }
        return null;
    }

    /**
     * Reads, parses, and adds external file to this file (if it is not here yet)
     *
     * @param fileName name of the file to be added
     * @param cache    if true then cache the file in the MediaCache
     * @return added flash file
     * @exception IVException
     */
    public synchronized FlashFile addExternalFile( String fileName, boolean cache ) throws IVException {
        try {
            Object o = addExternalMedia(fileName, cache);
            if( o instanceof FlashFile) return (FlashFile) o;
            return null;
        } catch( IOException e ) {
            throw new IVException(Resource.ERRREADINGFILE, new Object[] {fileName}, e);
        }
    }

    /**
     * Reads an external media and adds it to this file (if it is not here yet) and to media cache
     *
     * @param fileName name of the file to be added
     * @param cache    if true then cache the file in the MediaCache
     * @return FlashFile or Bitmap
     * @exception IVException
     */
    public synchronized Object addExternalMedia( IVUrl url, boolean cache ) throws IVException, IOException {
        String fileName = url.getName();

        // try to retrieve from this file's cache
        Object media = getExternalMedia( fileName );
        if( media != null ) return media;

            // try to retrive from media cache
        media = MediaCache.getMedia( fileName );

        if( media == null ) {
            // read the media
                InputStream is = url.getInputStream();
            FlashBuffer fb = new FlashBuffer(is);

            // detect media type
            int firstByte = fb.getUByte();
            if( firstByte == 'F' || firstByte == 'C' ) {    // check for swf format
                if( fb.getUByte() == 'W' && fb.getUByte() == 'S' ) {
                String myEncoding = url.getEncoding();
                    media = parse(fileName, fb, isFullParsing(), myEncoding!=null?myEncoding:getEncoding());
                } else {
                    throw new IVException( Resource.UNSUPMEDIA, new Object[] {fileName} );
                }
            } else {
                media = Bitmap.newBitmap(fb);
                if( media == null ) {
                    throw new IVException( Resource.UNSUPMEDIA, new Object[] {fileName} );
                }
            }

            // add to media cache depending on url parameters and 'cache' variable
            // 1.5 multiplier is because parsed data are bigger than the original ones (a little bit)
            MediaCache.addMedia( url, media, (fb.getSize()*3)/2, cache );
        }

        // add to this file's cache
        addExternalMedia(fileName, media);

        return media;
    }

    /**
     * Reads an external media and adds it to this file (if it is not here yet) and to media cache
     *
     * @param fileName name of the file to be added
     * @param cache    if true then cache the file in the MediaCache
     * @return FlashFile or Bitmap
     * @exception IVException
     */
    public synchronized Object addExternalMedia( String fileName, boolean cache ) throws IVException, IOException {
        return addExternalMedia(IVUrl.newUrl(fileName, this), cache);
    }

    /**
     * Finds external media by its file name
     *
     * @param filename name of the file to be searched
     * @return found media or null
     */
    public Object getExternalMedia( String filename ) {
        return externalFiles.get( filename );
    }

    /**
     * Adds external media to this file (if it is not here yet)
     *
     * @param filename name of the file to be added
     * @param object media to be added
     */
    public void addExternalMedia( String filename, Object object ) {
        externalFiles.put( filename, object );
    }

    /**
     * Finds external file by file name
     *
     * @param filename name of the file to be searched
     * @return found file or null
     */
    public FlashFile getExternalFile( String filename ) {
        Object o = getExternalMedia(filename);
        if( o instanceof FlashFile ) {
            return (FlashFile) o;
        }
        return null;
    }

    /**
     * Adds external file to this file (if it is not here yet)
     *
     * @param filename name of the file to be added
     * @param file     flash file to be added
     */
    public void addExternalFile( String filename, FlashFile file ) {
        addExternalMedia(filename, file);
    }

    /**
     * Returns default symbol file
     *
     * @return default symbol file
     */
    public synchronized FlashFile getDefaultSymbolFile() {
        if( defaultSymbolFile != null ) return defaultSymbolFile;
        String fileName = PropertyManager.getProperty("org.openlaszlo.iv.flash.defaultSymbolFile","bin/DefaultSymbolFile.swt");
        fileName = Util.getSysFile(fileName).getAbsolutePath();
        try {
            defaultSymbolFile = addExternalFile( fileName, true );
            return defaultSymbolFile;
        } catch( IVException e ) {
            Log.log(e);
            return null;
        }
    }

    /**
     * Searches font by its name in this file and all its external files
     *
     * @param name   font name
     * @return font or null
     */
    public Font getFont( String name ) {
        for( Enumeration e = defsByID.values(); e.hasMoreElements(); ) {
            FlashDef def = (FlashDef) e.nextElement();
            if( def instanceof FontDef ) {
                Font font = ((FontDef)def).getFont();
                if( font.getFontName().equalsIgnoreCase(name) ) return font;
            }
        }
        if( externalFiles.size() > 0 ) {
            Enumeration files = externalFiles.elements();
            while( files.hasMoreElements() ) {
                Object o = files.nextElement();
                if( o instanceof FlashFile ) {
                    FlashFile file = (FlashFile) o;
                Font font = file.getFont( name );
                if( font != null ) return font;
            }
        }
        }
        return null;
    }

    /**
     * Returns all fonts defined within this file (fonts from external files are not included)
     *
     * @return vector of FontDef objects
     */
    public IVVector getLocalFonts() {
        IVVector v = new IVVector();
        Enumeration e = definitions();
        while( e.hasMoreElements() ) {
            FlashDef def = (FlashDef) e.nextElement();
            if( def instanceof FontDef ) v.addElement( ((FontDef)def).getFont() );
        }
        return v;
    }

    /**
     * Finds script by name defined within this file or any external files
     *
     * @param name   script name
     * @return found script or null
     */
    public Script getScript( String name ) {
        FlashDef def = getDefFromLibrary( name );
        if( def instanceof Script ) return (Script) def;
        if ( def == null ) return getScriptInAssets( name );
        return null;
    }

    /**
     * Finds script by name defined as an ExportAsset within this file or any external files
     *
     * @param name   script name
     * @return found script or null
     */
    public Script getScriptInAssets( String name ) {
        FlashDef def = getDefInAssets(name.toUpperCase());
        if( def != null && def instanceof Script ) return (Script) def;
        if( externalFiles.size() > 0 ) {
            Enumeration files = externalFiles.elements();
            while( files.hasMoreElements() ) {
                Object o = files.nextElement();
                if( o instanceof FlashFile ) {
                    FlashFile file = (FlashFile) o;
                def = file.getScriptInAssets( name );
                if( def != null ) return (Script) def;
            }
        }
        }
        return null;
    }

    /**
     * Returns Enumeration of all definitions (FlashDef) defined within this file
     *
     * @return Enumeration of FlashDef
     */
    public Enumeration definitions() {
        return new Enumeration() {
            private Enumeration e = defsByID.values();
            private int names = 0;
            public boolean hasMoreElements() {
                for(;;) {
                    switch(names) {
                        case 0:
                            if( e.hasMoreElements() ) return true;
                            e = defsByName.elements();
                            names = 1;
                        case 1:
                            if( e.hasMoreElements() ) return true;
                            names = 2;
                        case 2:
                            return false;
                    }
                }
            }
            public Object nextElement() {
                return e.nextElement();
            }
        };
    }

    /**
     * Processes this file in the specified context
     *
     * @param context context to process in
     * @return this file
     * @exception IVException
     */
    public FlashFile processFile( Context context ) throws IVException {
        if( !isTemplate() && version < 6 ) return this;
        processScript( main, new StandardContext( context ) );
        return this;
    }

    /**
     * Processes the specified script in the specified context
     *
     * @param script  specified script
     * @param context specified context
     * @return processed script
     * @exception IVException
     */
    public Script processScript( Script script, Context context ) throws IVException {
        return (Script) processObject(script, context);
    }

    /**
     * Processes the specified object in the specified context
     *
     * @param fobj    specified object
     * @param context specified context
     * @return processed object
     * @exception IVException
     */
    public FlashObject processObject( FlashObject fobj, Context context ) throws IVException {
        processCount++;
        if( processCount < 50 ) {
            if( context == null ) context = new StandardContext();
            fobj.process(this, context);
        } else {
            throw new IVException(Resource.INFINITELOOP);
        }
        processCount--;
        fobj.setProcessed();
        return fobj;
    }

    /**
     * Generates file into FlashOutput buffer
     *
     * @return output buffer
     * @exception IVException
     */
    public FlashOutput generate() throws IVException {
        FlashOutput fob = new FlashOutput(this, getFileSize());

        // write signature
        fob.writeByte( isCompressed?'C':'F' );
        fob.writeByte( 'W' );
        fob.writeByte( 'S' );
        // write version
        fob.writeByte( version );
        // skip file size
        fob.skip(4);

        int filesize;
        if( isCompressed ) {
            FlashOutput b_fob = new FlashOutput(this, getFileSize());
            b_fob.write( frameSize );       // write file rect
            b_fob.writeWord( frameRate );   // write frame rate
            main.generate( b_fob );         // write main timeline
            filesize = b_fob.getSize()+8;
            try {
                OutputStream os = new DeflaterOutputStream(fob.getOutputStream());
                os.write(b_fob.getBuf(), 0, b_fob.getSize());
                os.close();
            } catch( IOException e ) {
                throw new IVException(e);
            }
        } else {
            fob.write( frameSize );         // write file rect
            fob.writeWord( frameRate );     // write frame rate
            main.generate( fob );           // write main timeline
            filesize = fob.getSize();
        }

        // write file size
        fob.writeDWordAt( filesize, 4 );

        return fob;
    }


    /**
     * Generates file into FlashOutput buffer
     *
     * @return output buffer
     * @exception IVException
     */
    public FlashOutput generate(FontsCollector fc, FontsCollector pfc, 
                                boolean hasPreloader) 
        throws IVException {

        FlashOutput fob = new FlashOutput(this, getFileSize());

        // write signature
        fob.writeByte( isCompressed?'C':'F' );
        fob.writeByte( 'W' );
        fob.writeByte( 'S' );
        // write version
        fob.writeByte( version );
        // skip file size
        fob.skip(4);

        int filesize;
        if( isCompressed ) {
            FlashOutput b_fob = new FlashOutput(this, getFileSize());
            b_fob.write( frameSize );       // write file rect
            b_fob.writeWord( frameRate );   // write frame rate
            main.generate( b_fob, fc, pfc, hasPreloader );  // write main timeline
            filesize = b_fob.getSize()+8;
            try {
                OutputStream os = new DeflaterOutputStream(fob.getOutputStream());
                os.write(b_fob.getBuf(), 0, b_fob.getSize());
                os.close();
            } catch( IOException e ) {
                throw new IVException(e);
            }
        } else {
            fob.write( frameSize );         // write file rect
            fob.writeWord( frameRate );     // write frame rate
            main.generate( fob, fc, pfc, hasPreloader );  // write main timeline
            filesize = fob.getSize();
        }

        // write file size
        fob.writeDWordAt( filesize, 4 );

        return fob;
    }


    /**
     * Parses Flash file from input stream
     *
     * @param fileName file name, used only for establishing the file's current directory
     * @param is       input stream used to read the file's content
     * @return created flash file
     * @exception IVException
     */
    public static FlashFile parse( String fileName, InputStream is ) throws IVException {
        return parse(fileName, is, false, null);
    }

    public static FlashFile parse( String fileName, InputStream is, boolean isFullParsing, String encoding ) throws IVException {
        FlashFile file = newFlashFileForParse( fileName, isFullParsing, encoding );

        try {
            Parser parser = new Parser();
            parser.parseStream( is, file );
            return file;
        } catch( IOException e ) {
            throw new IVException(Resource.ERRPARSETEMPLATE,  new Object[] {file.getFullName()}, e);
        } finally {
            try {
                is.close();
            } catch( Exception ee ) {}
        }
    }

    /**
     * Parses Flash file from buffer
     *
     * @param fileName file name, used only for establishing the file's current directory
     * @param fb       flash buffer to read the file from
     * @return created flash file
     * @exception IVException
     */
    public static FlashFile parse( String fileName, FlashBuffer fb ) throws IVException {
        return parse(fileName, fb, false, null);
    }

    public static FlashFile parse( String fileName, FlashBuffer fb, boolean isFullParsing, String encoding ) throws IVException {
        FlashFile file = newFlashFileForParse(fileName, isFullParsing, encoding);
        Parser parser = new Parser();
        parser.parseBuffer( fb, file );
        return file;
    }

    /**
     * Parses Flash file from file
     *
     * @param fileName file name of the flash file
     * @return created flash file
     * @exception IVException
     * @exception FileNotFoundException
     */
    public static FlashFile parse( String fileName ) throws IVException, FileNotFoundException {
        return parse(fileName, false, null);
    }

    public static FlashFile parse( String fileName, boolean isFullParsing, String encoding )
        throws IVException, FileNotFoundException
    {
        File file = new File(fileName);
        String path = file.getAbsolutePath();

        try {
            FileInputStream is = new FileInputStream(file);
            FlashFile flashFile = parse(path, is, isFullParsing, encoding);
            return flashFile;
        } catch( FileNotFoundException e ) {
            throw e;
        } catch( IOException e ) {
            throw new IVException(Resource.ERRPARSETEMPLATE, new Object[] {path}, e);
        }
    }

    /**
     * Creates empty flash file with default settings
     *
     * @return default flash file
     */
    public static FlashFile newFlashFile() {
        FlashFile file = new FlashFile();
        file.setVersion( 5 );
        file.setFrameSize( GeomHelper.newRectangle(0, 0, 540*20, 400*20) );
        file.setFrameRate( 12<<8 );
        file.setFileSize( 100 );    // does not have much sense
        file.encoding = PropertyManager.defaultEncoding;

        return file;
    }

    /**
     * Creates a copy of this flash file
     *
     * @return copy of this flash file
     */
    public synchronized FlashFile copyFile() {
        FlashFile file = new FlashFile();
        file.setFileName( fileName );
        file.setFileDir( fileDir );
        file.setVersion( version );
        file.setFrameSize( (Rectangle2D) frameSize.clone() );
        file.setFrameRate( frameRate );
        file.setFileSize( fileSize );
        file.processCount = processCount;
        file.isTemplate = isTemplate;
        file.isCompressed = isCompressed;
        file.encoding = encoding;

        // copy main script
        ScriptCopier copier = new ScriptCopier();
        file.main = (Script) main.getCopy( copier );

        // copier now contains all copied definitions
        // copy defsByID
        for( Enumeration e = defsByID.values(); e.hasMoreElements(); ) {
            FlashDef def = (FlashDef) e.nextElement();
            FlashDef myDef = copier.get( def );
            if( myDef != null ) file.defsByID.add( myDef );
        }

        // copy defsByName
        for( Enumeration e = defsByName.keys(); e.hasMoreElements(); ) {
            Object key = e.nextElement();
            FlashDef def = (FlashDef) defsByName.get( key );
            FlashDef myDef = copier.get( def );
            if( myDef != null ) file.defsByName.put( key, myDef );
        }

        // copy exportTable
        for( Enumeration e = exportTable.keys(); e.hasMoreElements(); ) {
            Object key = e.nextElement();
            FlashDef def = (FlashDef) exportTable.get(key);
            FlashDef myDef = copier.get( def );
            if( myDef != null ) file.exportTable.put( key, myDef );
        }

        // it's enough just to clone external files
        file.externalFiles = (Hashtable) externalFiles.clone();

        return file;
    }

    public void printContent( PrintStream out ) {
        out.println( "Flash: version="+version+" size="+getFileSize() );
        out.println( "  frame: ("+frameSize.toString()+")" );
        out.println( "  frameRate="+(frameRate>>8) );
        main.printContent( out, "" );
    }

    private static FlashFile newFlashFileForParse( String fileName, boolean isFullParsing, String encoding ) {
        fileName = Util.translatePath(fileName);
        int index = fileName.lastIndexOf( Util.fileSeparator );
        String dir, name;
        if( index >= 0 ) {
            dir = fileName.substring(0, index);
            name = fileName.substring(index+1);
        } else {
            dir = "";
            name = fileName;
        }
        FlashFile flashFile = new FlashFile();
        flashFile.setFileName( name );
        flashFile.setFileDir( dir );
        flashFile.setFullParsing(isFullParsing);
        flashFile.setEncoding(encoding!=null?encoding:PropertyManager.defaultEncoding);
        return flashFile;
    }

}

