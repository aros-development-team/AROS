/***
 * Emitter.java
 */

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;
import java.util.*;
import java.nio.*;
import org.openlaszlo.sc.Instructions.*;

public interface Emitter {

  public byte[] assemble(List instrs);

  public void emit(Instruction instr);
}
