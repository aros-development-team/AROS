/***
 * InstructionCollector.java
 *
 * Description: Instruction buffer for the assembler
 *
 * Assembly consists of two passes, one to create the constant
 * pool, and another to assemble the instructions to byte sequences.
 * The InstructionBuffer holds instructions between these passes.
 *
 * The InstructionBuffer will be replaced by a FlowAnalyzer, which will
 * perform basic-block analysis.  That's the main justification for
 * keeping this class here, instead of adding a wrapper around
 * Assembler the way peep-hole optimization is done.
 *
 * During the first pass (as instructions are collected), the buffer
 * scans the instruction sequence for string arguments to PUSH
 * instructions.  It computes an occurrence count for each string, and
 * sorts the list of strings that occurred more than once by occurrence
 * count.  The first 64K of these are placed in the constant pool.
 * (The sort assures that PUSH can use one-byte indices for the most
 * frequently-referenced strings.)
 *
 * During the second pass, each instruction is passed to the assembler,
 * and the resulting bytecodes are appended to an accumulated bytecode
 * sequence.
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

public class InstructionCollector extends ArrayList {
  public int nextLabel;
  public ConstantCollector constantCollector;
  public boolean constantsGenerated;

  public InstructionCollector(boolean disableConstantPool, boolean sortConstantPool) {
    super();
    this.nextLabel = 0;
    if (! disableConstantPool) {
      this.constantCollector = sortConstantPool ? new SortedConstantCollector() : new ConstantCollector();
    } else {
      this.constantCollector = null;
    }
    this.constantsGenerated = false;
  }

  public void emit(Instruction instr) {
    // Update the constant pool.
    if (this.constantCollector != null && instr instanceof PUSHInstruction) {
      PUSHInstruction push = (PUSHInstruction)instr;
      for (Iterator i = push.args.iterator(); i.hasNext(); ) {
        Object next = i.next();
        if (next instanceof String) {
          this.constantCollector.add(next);
        }
      }
    }
    super.add(instr);
  }

  public void push(Object value) {
    this.emit(Instructions.PUSH.__call__(value));
  }

  public void generateConstants() {
    // Only okay to call this once.
    assert (! this.constantsGenerated);
    ConstantCollector pool = this.constantCollector;
    // TODO: [2003-07-15 ptw] (krank) turn off the constant pool
    // for simplicity for now, but someday eliminate that
    if (pool != null  && (! pool.isEmpty())) {
      // TODO: [2003-03-06 ptw] Make CONSTANTS its own class?
      super.add(0, Instructions.CONSTANTS.__call__(pool.getConstants()));
      this.constantsGenerated = true;
    }
  }

  public void appendInstructions(Instruction[] instrs) {
    // TODO [2003-03-06 ptw] Why not relabel all instructions? (I.e.,
    // move this to emit)
    Map labels = new HashMap();
    for (int i = 0; i < instrs.length; i++) {
      Instruction instr = instrs[i];
      if (instr instanceof LABELInstruction) {
        // Rename labels uniquely
        Object label = ((LABELInstruction)instr).name;
        Object newLabel;
        if (labels.containsKey(label)) {
          newLabel = labels.get(label);
        } else {
          newLabel = this.newLabel();
          labels.put(label, newLabel);
        }
        instr = Instructions.LABEL.__call__(newLabel);
      } else if (instr instanceof TargetInstruction) {
        TargetInstruction target = (TargetInstruction)instr;
        Object label = target.getTarget();
        Object newLabel;
        if (labels.containsKey(label)) {
          newLabel = labels.get(label);
        } else {
          newLabel = this.newLabel();
          labels.put(label, newLabel);
        }
        instr = target.replaceTarget(newLabel);
      }
      this.emit(instr);
    }
  }

  public List getInstructions(boolean generateConstants) {
    if (! this.constantsGenerated && generateConstants) {
      this.generateConstants();
    }
    return this;
  }

  public String newLabel() {
    return "L" + this.nextLabel++;
  }

  public static class ConstantCollector extends ArrayList {
    public Object[] getConstants() {
      return this.toArray();
    }
  }


  // Long way to go for a closure
  public static class ConstantSorter implements Comparator {
    public int compare(Object o1, Object o2) {
      Map.Entry me1 = (Map.Entry)o1;
      int n1 = ((Integer)me1.getValue()).intValue();
      Map.Entry me2 = (Map.Entry)o2;
      int n2 = ((Integer)me2.getValue()).intValue();
      // Sort larger to the front (higher usage)
      // Longer string wins in a tie
      if (n1 == n2) {
        int l1 = ((String)me1.getKey()).length();
        int l2 = ((String)me2.getKey()).length();
        return l2 - l1;
      } else {
        return n2 - n1;
      }
    }

    public boolean equals (Object other) {
      // Too specific?  Do we care?
      return this == other;
    }
  }

  // There is probably some idiom for singletons that I don't know
  private static ConstantSorter sorter = new ConstantSorter();

  // This is kind of like a sorted set, but delays the sorting until
  // you ask for values from the set, and has a special limit on the
  // number of values that can be in the set.
  public static class SortedConstantCollector extends ConstantCollector {
    public Map usageCount;
    public boolean updated;

    SortedConstantCollector() {
      super();
      this.usageCount = new HashMap();
      this.updated = false;
    }

    public void add(int index, Object value) {
      this.updated = false;
      if (this.usageCount.containsKey(value)) {
        int n = ((Integer)this.usageCount.get(value)).intValue();
        this.usageCount.put(value, new Integer(n + 1));
      } else {
        this.usageCount.put(value, new Integer(1));
      }
    }

    public boolean add(Object value) {
      this.add(this.size(), value);
      return true;
    }

    public boolean addAll(int index, Collection c) {
      for (Iterator i = c.iterator(); i.hasNext(); index++) {
        this.add(index, i.next());
      }
      return true;
    }

    public boolean addAll(Collection c) {
      return this.addAll(this.size(), c);
    }

    public void clear() {
      this.updated = false;
      this.usageCount.clear();
      super.clear();
    }

    public boolean contains(Object value) {
      // Should this return if value was ever added, or if value will
      // be in the permitted subset?
      return this.usageCount.containsKey(value);
    }

    public int indexOf(Object value) {
      this.update();
      return super.indexOf(value);
    }

    public boolean isEmpty() {
      return this.usageCount.size() == 0;
    }

    public int lastIndexOf(Object value) {
      this.update();
      return super.lastIndexOf(value);
    }

    private Object removeInternal(int index) {
      this.updated = false;
      Object value = super.remove(index);
      this.usageCount.remove(value);
      return value;
    }

    public Object remove(int index) {
      this.update();
      return this.removeInternal(index);
    }

    protected void removeRange(int fromIndex, int toIndex) {
      this.update();
      for (int i = fromIndex; i < toIndex; i++) {
        this.removeInternal(i);
      }
    }

    public Object set(int index, Object value) {
      this.update();
      this.remove(index);
      this.add(value);
      return value;
    }

    public Object[] toArray() {
      this.update();
      return super.toArray();
    }

    public Object[] toArray(Object[] array) {
      this.update();
      return super.toArray(array);
    }

    public String toString() {
      this.update();
      return super.toString();
    }

    private void update() {
      if (! this.updated) {
        super.clear();
        ArrayList sorted = new ArrayList();
        for (Iterator i = this.usageCount.entrySet().iterator(); i.hasNext(); ) {
          sorted.add(i.next());
        }
        Collections.sort(sorted, sorter);
        // Total size of an action must be < 65535, opcode + length
        // field is 3 bytes, also must account for encoding of strings
        int room = 65535 - 3;
        String encoding = "Cp1252";
        String runtime = Instructions.getRuntime();
        if (runtime.equals("swf6") || runtime.equals("swf7")) {
          encoding = "UTF-8";
        }
        try {
          for (Iterator i = sorted.iterator(); i.hasNext(); ) {
            String symbol = (String)((Map.Entry)i.next()).getKey();
            room -= (symbol.getBytes(encoding).length + 1);
            if (room <= 0) break;
            super.add(symbol);
          }
        } catch (UnsupportedEncodingException e) {
          assert false : "this can't happen";
        }
        this.updated = true;
      }
    }

    public Object[] getConstants() {
      return this.toArray();
    }
  }
}
