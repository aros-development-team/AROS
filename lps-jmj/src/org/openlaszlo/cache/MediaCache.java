/******************************************************************************
 * MediaCache.java
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.cache;

import java.io.File;
import java.io.IOException;
import java.util.Properties;

import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.openlaszlo.data.MediaConverter;

/**
 * A media cache
 *
 * @author <a href="mailto:bloch@laszlosystems.com">Eric Bloch</a>
 */
public class MediaCache extends RequestCache {

    public MediaCache(File cacheDirectory, Properties props)
        throws IOException {

        super("mcache", cacheDirectory, new MediaConverter(), props);
    }
}
