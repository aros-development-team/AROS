# File: testing.py
# Author: Oliver Steele
# Description: Script compiler testing framework

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

# Many of these functions have undescriptive, one- or two-letter
# synonyms, because they're intended to be used from the console, not
# as part of an API.

# Additional documentation is in README.TXT

from __future__ import nested_scopes
import os
false, true = 0, 1
undefined = []
CallNo = 0

#
# Interactive Testing
#

def parse(str, type=None):
    reloadSystem()
    from Compiler import Parser
    from org.openlaszlo.sc.parser import ParseException
    try:
        return Parser().parse(str, *(type and (type,) or ()))
    except ParseException, e:
        raise `e`

def pparse(str, type=None):
    """Pretty-prints the parsed string.  No return value."""
    parse(str, type).dump('')

def rt(str, type=None):
    """Pretty-prints the parsed string.  No return value."""
    reloadSystem()
    from Compiler import ParseTreePrinter
    ParseTreePrinter().print(parse(str, type))

def parse0(str, type=None):
    reloadSystem()
    from Compiler import Parser
    from org.openlaszlo.sc.parser import ParseException
    try:
        return Parser().parse0(str, *(type and (type,) or ()))
    except ParseException, e:
        raise `e`

def pparse0(str, type=None):
    """Pretty-prints the parsed string.  No return value."""
    parse0(str, type).dump('')

def compile(s=None, **options):
    # Update arguments from the argument cache, and vice versa
    global SavedTest, SavedOptions, CallNo
    if s is None:
        try:
            s = SavedTest
        except NameError:
            raise "The first call to compile must specify a source string."
    options_ = globals().get('SavedOptions', {})
    options_.update(options)
    options = options_
    if s:
        SavedTest = s
    if options:
        SavedOptions = options
    reloadSystem()
    from Compiler import Compiler
    c = Compiler(**options)
    from org.openlaszlo.sc.parser import ParseException
    try:
        CallNo += 1
        #bytes = c.compile(('#file "interactive input %d"\n#line 1\n' % CallNo) + s)
        bytes = c.compile(s)
    except ParseException, e:
        raise `e`
    writeMovie(bytes, 'test.swf')

def compileFile(fname, **options):
    class Resolver:
        def __init__(self, base):
            self.base = base
        def resolve(self, pathname):
            import os
            return os.path.join(os.path.split(self.base)[0], pathname)
    options_ = {}; options_.update(options)
    options['resolver'] = Resolver(fname)
    compile(open(fname).read(), **options)

def compileConstraint(s=None, **options):
    compile('var f = function (){\n#pragma "constraintFunction"\n%s}' % s, **options)

def writeMovie(bytes, fname):
    from org.openlaszlo.iv.flash.api import FlashFile, Script
    from org.openlaszlo.iv.flash.api.action import DoAction, Program
    file = FlashFile.newFlashFile()
    file.version = 5
    file.mainScript = Script(1)
    frame = file.mainScript.newFrame()
    program = Program(bytes, 0, len(bytes))
    frame.addFlashObject(DoAction(program))
    istr = file.generate().inputStream
    from jarray import zeros
    bytes = zeros(istr.available(), 'b')
    istr.read(bytes)
    from java.io import FileOutputStream
    ostr = FileOutputStream(fname)
    try:
        ostr.write(bytes)
    finally:
        ostr.close()
    return

c = compile
cf = compileFile

#
# Timings
#

def time(s=None, **options):
    reloadSystem() # force reload outside the timer
    import time
    t = time.time()
    compile(s, **options)
    return time.time() - t

def timings():
    reloadSystem() # force reload outside the timer
    baseline = time(doubleParse=0)
    lps = len(SavedTest.split('\n'))/baseline
    print '%2.2f lines/second (including comments)' % lps
    for k in 'parse generate assemble'.split():
        t = time(**{'double' + k.capitalize(): 1}) - baseline
        print '%s\t%2.2fs\t%2.2f%%' % \
                  (k, t, 100 * t / baseline)
    print 'total\t%2.2fs\t100%%' % baseline

def profile():
    import profile, testing
    profile.run('testing.compile()', 'profile.txt')
    import pstats
    p = pstats.Stats('profile.txt')
    #p.strip_dirs().sort_stats('cumulative').print_stats(20)
    p.strip_dirs().sort_stats('time').print_stats(20)

#
# Running Tests
#

TestKeys = None  # Default test suite to run
TestIndex = None # Default test, if TestKeys is a singleton

def reloadSystem():
    """Reload changed modules."""
    import parseinstructions, Compiler, tests
    consequences = []
    for m in parseinstructions, Compiler, tests:
        import os
        mt = getattr(m, '__time__', 0)
        ft = os.path.getmtime(m.__file__)
        if m in consequences or ft > mt:
            print m
            reload(m)
            m.__time__ = ft
            if m == parseinstructions:
                consequences += [Compiler]

def collectTests(keys=undefined, index=undefined, flashOnly=false,
                 saveArguments=false):
    # synch the args with the cache of previous values
    if saveArguments:
        global TestKeys, TestIndex
        if keys is undefined: keys = TestKeys
        if index is undefined: index = TestIndex
        TestKeys = keys
        TestIndex = index
    # lift singletons to lists
    if type(keys) == type(""):
        keys = [keys]
    # if keys is blank, run all the tests
    if not keys:
        if flashOnly:
            keys = FlashTests
        else:
            keys = PassedTests
    if len(keys) == 1 and index is not None:
        tests = [Tests[keys[0]][index]]
    else:
        tests = []
        for key in keys:
            #tests += ['\n// ' + key]
            tests += Tests[key]
    # filter out tests that the Flash compiler can't run
    if flashOnly:
        tests = [test for test in tests if test not in FlashOmittedTestCases]
    return tests

def runConstraintTests():
    reloadSystem()
    from tests import ConstraintTests
    from Compiler import nodeString, ReferenceCollector
    for test, expected in ConstraintTests:
        rc = ReferenceCollector()
        rc.visit(parse(test))
        result = nodeString(rc.computeReferences('test'))
        # print '(%r, %r),' % (test, result)
        if result != expected:
            raise '%r: expected %r; got %r' % (test, expected, result)
    from tests import MetaConstraintTests
    for test, expected in MetaConstraintTests:
        rc = ReferenceCollector(true)
        rc.visit(parse(test))
        result = nodeString(rc.computeReferences('test'))
        # print '(%r, %r),' % (test, result)
        if result != expected:
            raise '%r: expected %r; got %r' % (test, expected, result)

def test(keys=undefined, index=undefined, **options):
    """Run the tests."""
    reloadSystem()
    tests = collectTests(keys, index, saveArguments=true)
    from Compiler import Compiler
    runConstraintTests()
    for test in tests:
        c = Compiler(**options)
        try:
            from jarray import array
            bytes = c.compile(test)
            array(bytes, 'b')
        except:
            print "During compilation of %r" % test
            raise

def write():
    """Write the test source to tests.as, and the swf to sc.swf.
    The intent is that Flash compiles tests.as -> flash.swf,
    and the flasms of sc.swf and flash.swf can be compared."""
    tests = collectTests(None, flashOnly=true)
    reloadSystem()
    # put the functions first, since Flash will compile them first
    def isFn(s): return s.startswith('function')
    def insertFunctionComment(s):
        pos = s.find('{')
        return s[:pos+1] + 'src = %r;' %s + s[pos+1:]
    tests = [insertFunctionComment(s) for s in tests if isFn(s)] + \
            ['src = %r\n%s' % (s,s)
             for s in tests if not isFn(s)]
    text = '\n'.join(tests) + '\n'
    f = open('tests.as', 'w')
    f.write(text)
    f.close()
    from Compiler import Compiler
    c = Compiler(flashCompilerCompatability=true)
    bytes = c.compile(text)
    writeMovie(bytes, 'sc.swf')

RegressionOptions = {'createActivationObject': 0}

def establishRegressionBaseline():
    reloadSystem()
    tests = collectTests()
    text = '\n'.join(tests)
    from Compiler import Compiler
    writeMovie(Compiler(**RegressionOptions).compile(text), 'regression-baseline.swf')
    os.system('flasm -d regression.swf > regression-baseline.flasm')

def runRegressionTests():
    reloadSystem()
    tests = collectTests()
    text = '\n'.join(tests)
    from Compiler import Compiler
    writeMovie(Compiler(**RegressionOptions).compile(text), 'regression.swf')
    os.system('flasm -d regression.swf > regression.flasm')
    os.system('diff regression-baseline.flasm regression.flasm')

def testStaticCoverage():
    import Compiler
    Compiler.testStaticCoverage()

rl = reloadSystem
t = test
w = write


#
# Defining Tests
#

def resetTests():
    global Tests, PassedTests, FlashTests, FlashOmittedTestCases
    Tests = {}
    PassedTests = []
    FlashTests = []
    FlashOmittedTestCases = []

def DefineTests(name, tests, passed=true, flash=true):
    Tests[name] = tests
    if passed:
        PassedTests.append(name)
        if flash:
            FlashTests.append(name)

def wrap(w, s, options):
    """Returns w % s.  Options are parsed and acted upon.  If s is a
    tuple, its first element is the string, and the second element is
    an options table that should override the options argument."""
    if type(s) == type(()):
        s, u = s
        o = {}
        o.update(options)
        o.update(u)
        options = o
    s = w % s
    def parseOptions(flash=true):
        if not flash:
            FlashOmittedTestCases.append(s)
    parseOptions(**options)
    return s

def Expr(s, **options):
    """Wrap the expressions in an assignment statement."""
    return wrap('a = %s', s, options)

def Stmt(s, **options):
    """Wrap the statements in a function body."""
    return wrap('function f() {%s}', s, options)

def Exprs(l, **options):
    return [Expr(s, **options) for s in l]

def Stmts(l, **options):
    return [Stmt(s, **options) for s in l]

if __name__ == '__main__':
    runRegressionTests()
