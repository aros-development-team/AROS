/* *****************************************************************************
 * CompilerMediaCache.java
* ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.compiler;

import org.openlaszlo.media.Transcoder;
import org.openlaszlo.media.TranscoderException;
import org.openlaszlo.cache.Cache;
import org.openlaszlo.cache.CachedInfo;
import org.openlaszlo.utils.FileUtils;
import org.openlaszlo.server.LPS;

import org.apache.log4j.*;

import java.util.Properties;
import java.io.File;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.FileOutputStream;

/**
 * A class for maintaining a disk-backed cache of transcoded media
 * files for the compiler.
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public class CompilerMediaCache extends Cache {

    /** Logger. */
    private static Logger mLogger = Logger.getLogger(CompilerMediaCache.class);

    /** Properties */
    private static Properties mProperties = null;
    
    /** See the constructor. */
    protected File mCacheDirectory;

    /**
     * Creates a new <code>CompilerMediaCache</code> instance.
     */
    public CompilerMediaCache(File cacheDirectory, Properties props) 
        throws IOException {
        super("cmcache", cacheDirectory, props);
        this.mCacheDirectory = cacheDirectory;
        if (props == null) {
            this.mProperties = new Properties();
        } else {
            this.mProperties = props;
        }

    }

    /**
     * Return properties object
     * There is one property <code>forcetranscode</code>
     * which when set to <code>true</code> will force the
     * cache to always transcode requests.
     */
    public Properties getProperties() {
        return mProperties;
    }

    /**
     * Transcode the given input file from the fromType to toType
     * @return the transcoded file
     * @param inputFile file to be transcoded
     * @param fromType type of file to be transcoded
     * @param toType type of file to transcode into
     */
    public synchronized File transcode(
            File inputFile, 
            String fromType, 
            String toType) 
        throws TranscoderException, 
               FileNotFoundException, 
               IOException {

        mLogger.debug("transcoding from " + fromType + " to " + toType);
        if (fromType.equalsIgnoreCase(toType)) {
            return inputFile;
        }

        if (!inputFile.exists()) {
            throw new FileNotFoundException(inputFile.getPath());
        }

        // Key should be relative to webapp path and have
        // consistent path separator
        String key = FileUtils.relativePath(inputFile, LPS.HOME()) + ":" + toType;

        /* we don't use the cache's encoding support; we do it ourselves */
        String enc = null; 
        boolean lockit = false;
        Item item = findItem(key, null, lockit);

        String inputFilePath = inputFile.getAbsolutePath();
        File cacheFile = item.getFile();
        String cacheFilePath = cacheFile.getAbsolutePath();
        mLogger.debug("transcoding input: " + inputFilePath + 
                " output: " + cacheFilePath);

        InputStream input = null;
        FileOutputStream output = null;

        if (!cacheFile.exists() || !inputFile.canRead() ||
            inputFile.lastModified() > cacheFile.lastModified() ||
            mProperties.getProperty("forcetranscode", "false") == "true") {
    
            item.markDirty();

            mLogger.debug("transcoding...");

            CachedInfo info = item.getInfo();
            try {
                input = Transcoder.transcode(inputFile, fromType, toType);
                mLogger.debug("done transcoding");
                item.update(input, null);
                info.setLastModified(cacheFile.lastModified());
                item.updateInfo();
                item.markClean();
            } finally {
                FileUtils.close(input);
            }
        } else { 
            mLogger.debug("using cached transcode");
        }

        updateCache(item);

        return cacheFile;
    }
}
