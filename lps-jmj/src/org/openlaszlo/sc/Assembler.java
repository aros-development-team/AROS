/***
 * Assembler.java
 *
 * Description: The assembler translates an assembly instruction into
 * a sequence of bytes.
 */

/* J_LZ_COPYRIGHT_BEGIN *******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.              *
* Use is subject to license terms.                                            *
* J_LZ_COPYRIGHT_END *********************************************************/

package org.openlaszlo.sc;
import java.io.*;
import java.nio.*;
import java.util.*;
import org.openlaszlo.sc.Instructions.*;
import org.openlaszlo.sc.Emitter;

// Assembly is relatively straightforward except for the PUSH
// instruction, and label resolution.
//
// During the assembly of PUSH, strings in the constant pool are
// replaced by constant-pool references.
//
// Note that the assembler, not the code generator, is responsible for
// recognizing which string arguments to a PUSH instruction can refer
// to the constant pool.
//
// Label targets are are entered in a table mapping label names to
// offsets.  Instructions that contain label sources are processed as
// follows: a backwards reference is replaced by a byte offset, while a
// forward-reference is entered in a table mapping a label name to a
// list of offsets whence the label is referenced.  When a label is
// encountered, the forward-reference table is used to backpatch
// forward references.

public class Assembler implements Emitter {
  Hashtable labels;
  ByteBuffer bytes;
  private static byte[] backingStore;
  Hashtable constants;

  public static class Label {
    Object name;
    ByteBuffer bytes;
    int location;
    List references;

    public Label(Object name) {
      this.name = name;
      this.location = -1;
      this.references = new ArrayList();
    }

    public boolean isResolved() {
      return this.location != -1;
    }

    public void setLocation(ByteBuffer bytes) {
      assert (! this.isResolved()) : "Label.setLocation() called on resolved label";
      this.location = bytes.position();
      // Backpatch forward jumps
      for (Iterator i =  this.references.iterator(); i.hasNext(); ) {
        int patchloc = ((Integer)i.next()).intValue();
        int offset = location - patchloc - 2;
        if (offset != (short)offset) {
            // throw new CompilerException("Jump offset too large");
        }
        bytes.putShort(patchloc, (short)offset);
      }
      this.references = null;
    }

    public void addReference(int patchloc) {
      assert (! this.isResolved()) : "adding reference to resolved label";
      this.references.add(new Integer(patchloc));
    }

    public String toString() {
      return this.name.toString();
    }
  }

  private synchronized byte[] getBacking() {
    byte[] b = backingStore;
    backingStore = null;
    return b;
  }

  private synchronized void setBacking(byte[] b) {
    backingStore = b;
  }

  public Assembler() {
    this.labels = new Hashtable(); // {String -> Label}
    // Try to reuse the backing buffer
    byte[] bs = getBacking();
    if (bs != null) {
      this.bytes = ByteBuffer.wrap(bs);
    } else {
      // Room to not grow immediately.  See emit.
      this.bytes = ByteBuffer.allocate((1<<14) + (1<<16));
    }
    this.constants = new Hashtable(); // {String -> int}
  }

  public byte[] assemble(List instrs) {
    for (Iterator i = instrs.iterator(); i.hasNext(); ) {
      this.emit((Instruction)i.next());
    }
    List unresolvedLabels = new ArrayList();
    // One wonders why this couldn't be an Iterator
    for (Enumeration i = this.labels.elements(); i.hasMoreElements(); ) {
      Label label = (Label)i.nextElement();
      if (! label.isResolved()) {
        unresolvedLabels.add(label);
      }
    }
    //System.out.println("assembled: " + this.bytes.toString());
    assert (unresolvedLabels.size() == 0) : "unresolved labels: " + unresolvedLabels;
    // TODO [2004-03-04 ptw] be more efficient than this!
    byte[] result = new byte[this.bytes.position()];
    this.bytes.flip();
    this.bytes.get(result);
    // Save the backing buffer
    setBacking(bytes.array());
    return result;
  }

  public Label getLabel(Object name) {
    if (! this.labels.containsKey(name)) {
      this.labels.put(name, new Label(name));
    }
    return (Label)this.labels.get(name);
  }

  public void emit(Instruction instr) {
    // Verify there is room for a maximal instruction (1<<16)
    // TODO: [2004-08-02 ptw] 1<<16 does not work.  Why?
    // As a temporary kludge, double that.
    if (! (bytes.remaining() > 1<<18)) {
      // TODO [2004-03-11 ptw] Spool to file above a certain size
      ByteBuffer newBytes = ByteBuffer.allocate(bytes.capacity() * 2);
      ByteBuffer oldBytes = bytes;
      bytes.flip();
      newBytes.put(bytes);
      bytes = newBytes;
      //System.out.println("Grow buffer: " + oldBytes + " => " + newBytes);
    }
    if (instr instanceof ConcreteInstruction && ((ConcreteInstruction)instr).op == Actions.CONSTANTS) {
      // Initialize the constant map
      this.constants = new Hashtable();
      for (ListIterator i = ((ConcreteInstruction)instr).args.listIterator(); i.hasNext(); ) {
        int index = i.nextIndex();
        this.constants.put(i.next(), new Integer(index));
      }
      // Fall through to the general case
    }
    if (instr instanceof LABELInstruction) {
      Object name = ((LABELInstruction)instr).name;
      Label label = this.getLabel(name);
      assert (! label.isResolved()) : "duplicate label" + label;
      // Get the current location, and save it for backjumps
      label.setLocation(bytes);
    } else if (instr instanceof TargetInstruction) {
      TargetInstruction target = (TargetInstruction)instr;
      Label label = this.getLabel(target.getTarget());
      int loc = label.location;
      if (loc == -1) {
        // Target location isn't yet available.  Use a null
        // offset, and add the address to be patched to this
        // label's list of backpatch locations.
        target.writeBytes(this.bytes, this.constants);
        int patchloc = bytes.position() - 2;
        label.addReference(patchloc);
      } else {
        // Target computation requires that we write the instruction first!
        target.targetOffset = 0;
        target.writeBytes(this.bytes, this.constants);
        int offset = loc - bytes.position();
        if (offset != (short)offset) {
            throw new CompilerException("Jump offset too large");
        }
        bytes.putShort(bytes.position() - 2, (short)offset);
        }
    } else {
      instr.writeBytes(this.bytes, this.constants);
    }
  }
}
