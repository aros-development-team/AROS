# Experimental SConstruct file for SCons build system
# Take care about paths which only exists on my PC

# Get default environment
env = Environment()

# Where the fake compiler and FlexCat resides
tooldir = '/home/mazze/projects/aros-linux-i386-dbg/bin/linux-x86_64/tools'

# Some more variables. "#" means directory from where SCons was started
platform = 'i386'
arosdir = '#' + platform + '/bin/' + platform + '/AROS'
gendir  = '#' + platform + '/bin/' + platform + '/gen'

# Clone the default environment
targetEnv = env.Clone()

# Add a builder for source files from *.cd files by FlexCat
targetEnv['FLEXCAT_SD'] = tooldir + '/C_h_orig.sd'

def locale_h_generator(source, target, env, for_signature):
    return 'FlexCat %s %s=%s' % (source[0], target[0], env['FLEXCAT_SD'])

bld = Builder(generator = locale_h_generator)
targetEnv.Append(BUILDERS = {'Locale_H' : bld})

# Set tool search path
targetEnv['ENV']['PATH'] = ['/bin','/usr/bin', tooldir]

# Tools and settings
targetEnv.Replace(CC  = 'i386-linux-aros-gcc')
targetEnv.Replace(CXX = '') # Doesn't exist as fake compiler
targetEnv.Replace(AR  = 'i386-linux-aros-ar')
targetEnv.Replace(AS  = 'i386-linux-aros-as')
targetEnv.Replace(RANLIB  = 'i386-linux-aros-ranlib')

# SCons needs to know where includes and libs are (for dependency checking)
targetEnv.Replace(CPPPATH = '/home/mazze/projects/aros-linux-i386-dbg/bin/linux-i386/AROS/Development/include')
targetEnv.Replace(LIBPATH = '/home/mazze/projects/aros-linux-i386-dbg/bin/linux-i386/AROS/Development/lib')

# Default CFLAGS and LIBS
targetEnv.Replace(CCFLAGS = '-Wall -Wno-pointer-sign')
targetEnv.Replace(LIBS = ['arosc', 'arossupport', 'amiga'])

# Export environment and some variables to sub SConscript files
Export('targetEnv', 'platform', 'arosdir', 'gendir')

# Call sub SConscript files
SConscript([
    'workbench/tools/ScreenGrabber/SConscript',
    'workbench/tools/commodities/SConscript'
])


# print targetEnv.Dump()
