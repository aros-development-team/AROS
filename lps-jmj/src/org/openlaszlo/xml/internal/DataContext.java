/* ****************************************************************************
 * DataContext.java
 *
 * Compile XML directly to SWF bytecodes.
 * ****************************************************************************/

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.xml.internal;

import java.io.*;
import java.util.*;
import org.openlaszlo.utils.HashIntTable;
import org.apache.commons.httpclient.Header;
    
public class DataContext {
    public HashIntTable cpool = new HashIntTable(256, -1);
    public HashIntTable cpool_first = new HashIntTable(256, -1);
    public int pool_data_length = 0;
    public int flashVersion = 5;
    public String encoding = "Cp1252";

    public DataContext () {
    }
    
    public DataContext (int flashVersion) {
        this.flashVersion = flashVersion;
    }

    public void setEncoding(String encoding) {
        this.encoding = encoding;
    }

}

