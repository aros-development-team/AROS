Notes on Python:
- There's good tutorials and references at python.org.
- Any variable that is assigned in a function is local unless it's
  declared global by the 'global v' statement.  This is the opposite
  of ECMAScript.
- Python doesn't have a boolean type.  The modules define true and
  false, but they aren't available at the command line unless you
  define them yourself.

Interactively testing the compiler:
- In cygwin or Unix, execute sci to start a Jython session.  An
  example session is below.  ">" is the unix prompt, and ">>>" is the
  Jython prompt.  "#" precedes commentary.
  > ./sci
  # "Jython 2.1" message is printed here.
  >>> import testing
  >>> testing.c('a = b', printInstructions=0)
  # Shows the assembly, and creates tests.swf.
  >>> c = testing.c           # Create a shortcut
  >>> c('a = b+c')            # Compile another program
  >>> testing.t()             # Silently runs the test suite
  >>> testing.t(printInstructions=1)
                              # Shows assembly
  >>> print testing.w.__doc__ # Describes the 'w' function
  >>> tests.w()

Flash comparison:
- Start a Jython session, as above, and:
  >>> import testing
  >>> testing.w()
- Use the Flash tool to compiles test.fla -> flash.sc
- In Unix or cygwin:
  > flasm -d sc.swf > sc.flasm
  > flasm -d flash.swf > flash.flasm
  > diff sc.flasm flash.flasm

Regression testing:
 >>> import testing
 >>> testing.runRegressionTests()

* T_LZ_COPYRIGHT_BEGIN ******************************************************
* Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
* Use is subject to license terms.                                          *
* T_LZ_COPYRIGHT_END ********************************************************
