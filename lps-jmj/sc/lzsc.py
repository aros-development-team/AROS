# File: lzsc.py

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

# Jythonc doesn't preserve the module comment, so use a string constant
# instead.
USAGE = """Usage: lzsc [options] scriptfile
  Compile scriptfile to an action block and place in a dummy swf file named outputswf.

  Options:
    --delete
    --incremental
    --debug
    --profile
    --krank
    --runtime=[swf5|swf6|swf7]
    -Dname=value
    --option compilerOption[=value]
    -o outputswf
"""

# Our modification to the jythonc compiler special-cases this syntax.
import org.openlaszlo.sc.CompilerException as CompilerException
False, True = 0, 1

def usage(errcode, msg=''):
    print USAGE % globals()
    if msg:
        print msg
    import sys
    sys.exit(errcode)

def compile(outf, scriptfile, **options):
    import Compiler
    class Resolver:
        def __init__(self, base):
            self.base = base
        def resolve(self, pathname):
            import os
            return os.path.join(os.path.split(self.base)[0], pathname)
    c = Compiler.Compiler(flashCompilerCompatability=True,
                          processIncludes=1,
                          resolver=Resolver(scriptfile),
                          **options)
    f = None
    try:
        f = open(scriptfile)
        bytes = c.compile(f.read())
    finally:
        if f:
            f.close()
    # new a Flash file, stuff bytes into it
    
    import org.openlaszlo.iv.flash.api.FlashFile as FlashFile;
    import org.openlaszlo.iv.flash.api.Script as Script;
    import org.openlaszlo.iv.flash.api.action.Program as Program;
    import org.openlaszlo.iv.flash.api.Frame as Frame;
    import org.openlaszlo.iv.flash.api.action.DoAction as DoAction;

    newfile = FlashFile.newFlashFile()
    if options[Compiler.RUNTIME] == 'swf6':
        newfile.setVersion(6)
    elif options[Compiler.RUNTIME] == 'swf7':
        newfile.setVersion(7)
    else:
        assert options[Compiler.RUNTIME] == 'swf5'
        newfile.setVersion(5)        
    mainScript = Script(1)
    mainScript.setMain()
    newfile.setMainScript(mainScript) 
    frame = newfile.getMainScript().getFrameAt(0)
    program = Program(bytes, 0, len(bytes))
    block = DoAction(program)
    frame.addFlashObject(block)
    flashbuf = newfile.generate()
    instream = flashbuf.getInputStream()
    
    import org.openlaszlo.utils.FileUtils as FileUtils
    import java.io.FileOutputStream as FileOutputStream
               
    outstream = FileOutputStream(outf)
    FileUtils.send(instream, outstream);
    outstream.close()


def main(args):
    import Compiler
    import getopt
    try:
        opts, args = getopt.getopt(
            args,
            'D:o:hgpk',
            ['help', 'incremental', 'default=', 'delete','runtime=',
             'debug', 'profile', 'krank', 'option='])
    except getopt.error, msg:
        usage(1, msg)
    outf = None
    deleteFile = False
    scriptFile = None
    incremental = False
    import org.openlaszlo.server.LPS as LPS
    defaultRuntime = LPS.getProperty('compiler.runtime.default', 'swf6')
    # default constants
    compileTimeConstants = {'$debug': False,
                            '$krank': False, 
                            '$profile': False,
                            '$runtime': defaultRuntime,
                            '$swf5': defaultRuntime == 'swf5',
                            '$swf6': defaultRuntime == 'swf6',
                            '$swf7': defaultRuntime == 'swf7'}
    # default to conditional compilation, swf6 runtime
    compilerOptions = {
        Compiler.CONDITIONAL_COMPILATION: True,
        Compiler.RUNTIME: defaultRuntime,
        Compiler.CACHE_COMPILES: True,
        }
    for opt, arg in opts:
        if opt in ('-h', '--help'):
            usage(0)
        elif opt == '-o':
            outf = arg
        elif opt == '-D':
            if '=' not in arg: usage(1, msg)
            key, value = arg.split('=', 1)
            if value in ('true', 'false'):
                value = (value == 'true')
            compileTimeConstants[key] = value
        elif opt == '--default':
            scriptFile = arg
        elif opt == '--delete':
            deleteFile = True
        elif opt == '--incremental':
            incremental = True
            compilerOptions['cacheCompiles'] = True
            compilerOptions['progress'] = True
        elif opt == '--option':
            key, value = arg, True
            if '=' in key:
                key, value = key.split('=', 1)
            if value in ('true', 'false'):
                value = (value == 'true')
            compilerOptions[key] = value
        elif opt in ('-g', '--debug'):
            compilerOptions['debug'] = True
            compileTimeConstants['$debug'] = True
        elif opt in ('-p', '--profile'):
            compilerOptions['profile'] = True
            compileTimeConstants['$profile'] = True
        elif opt in ('-k', '--krank'):
            compilerOptions['krank'] = True
            compileTimeConstants['$krank'] = True
        elif opt == '--runtime':
            if arg and not (arg == 'swf5' or arg == 'swf6' or arg == 'swf7'):
                usage(1, 'runtime must be one of swf5, swf6 or swf7')
            compilerOptions[Compiler.RUNTIME] = arg
            if arg == 'swf7':
                compileTimeConstants['$runtime'] = arg
                # deprecated
                compileTimeConstants['$swf5'] = False
                compileTimeConstants['$swf6'] = False
                compileTimeConstants['$swf7'] = True
            elif arg == 'swf6':
                compileTimeConstants['$runtime'] = arg
                # deprecated
                compileTimeConstants['$swf5'] = False
                compileTimeConstants['$swf6'] = True
                compileTimeConstants['$swf7'] = False
            else:
                compileTimeConstants['$runtime'] = 'swf5'
                # deprecated
                compileTimeConstants['$swf5'] = True
                compileTimeConstants['$swf6'] = False
                compileTimeConstants['$swf7'] = False
    compilerOptions['compileTimeConstants'] = compileTimeConstants
    if not outf: usage(1, ' -o is required')
    if not args and scriptFile: args = [scriptFile]
    if len(args) != 1: usage(1, 'exactly one file argument is required')
    while True:
        firstTime = False
        if deleteFile:
            import os
            try:
                os.remove(outf)
            except: pass
        from java.lang.System import currentTimeMillis
        time = currentTimeMillis()
        try:
            compile(outf, args[0], **compilerOptions)
        except CompilerException, e:
            print e
            print 'Compilation aborted.'
            if not incremental:
                return 1
        time = currentTimeMillis() - time
        if not incremental:
            break
        print 'Compiled %s in %.2f seconds' % (outf, time/1000.)
        response = raw_input('Compile again [Enter | q + Enter]: ').lower().strip()
        if response.startswith('q'):
            return

def test(argline=''):
    lfcPath = '../../lfc'
    main(['-o', lfcPath + '/LFC6-debug.lzl',
          '--option', 'nameFunctions=true', '-D$debug=true',
          '--option', 'generateFunction2=true',
          '--option', 'cacheCompiles=false',
          '--option', 'warnGlobalAssignments=true',
          '--runtime=swf6',
          lfcPath + '/LaszloLibrary.as'])

if __name__ == '__main__':
    import sys
    status = main(sys.argv[1:])
    if status:
        sys.exit(status)
