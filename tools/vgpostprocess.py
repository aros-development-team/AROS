import re, os, sys

class Module:
    def __init__(self):
        self.name = ""
        self.id = -1
        self.symbols = []
        self.minaddr = 0xFFFFFFFF
        self.maxaddr = 0

def loadsymbolinformation():
    reg = r'S\|(.*)\|(.*)\|(.*)\|(.*)'
    modules = []
    symbols = []
    lastmodname = ""
    module = None

    with open(sys.argv[2]) as f:
        for line in f:
            match = re.match(reg, line)
            if match:
                modname = match.group(1)
                symbolname = match.group(2)
                start = int(match.group(3), 16)
                end = int(match.group(4), 16)

                # New module is needed
                if (modname != lastmodname):
                    if (module):
                        print "%s %x %x" % (module.name, module.minaddr, module.maxaddr)
                    module = Module()
                    module.name = modname
                    modules.append(module)
                    lastmodname = modname

                # Flags entries with no name
                if (symbolname == ""):
                    symbolname = "<no name>"

                # Ignore entries with no address
                if (start != 0 and end != 0):
                    symbol = (symbolname, start, end)
                    module.minaddr = min(start, module.minaddr)
                    module.maxaddr = max(end, module.maxaddr)
                    module.symbols.append(symbol)

    return modules

def addmodulespec(line, module, output):
    if (line[0] != 'c'):
        if (module.id == -1):
            module.id = addmodulespec.nextmoduleid
            addmodulespec.nextmoduleid += 1
            output.write("ob=(" + str(module.id) + ") " + module.name + os.linesep)
        else:
            output.write("ob=(" + str(module.id) + ")" + os.linesep)

def main():

    modules = loadsymbolinformation()

    addmodulespec.nextmoduleid = 1000

    print "Processing..."

    reg = r'([c]*fn=\(\d*\) )0x([a-f0-9]*)'
    output = open(sys.argv[1] + ".processed", "w")
    with open(sys.argv[1]) as log:
        for line in log:
            match = re.match(reg, line)
            if match:
                address = int(match.group(2), 16)
                for module in modules:
                    # Skip modules that can't host this address
                    if (address < module.minaddr):
                        continue
                    if (address > module.maxaddr):
                        continue

                    for symbol in module.symbols:
                        if (symbol[1] <= address and symbol[2] >= address):
                            # Write out additional module spec
                            addmodulespec(line, module, output)

                            # Build out actuall symbol information
                            line = match.group(1) + symbol[0] + os.linesep
                            break
            
            output.write(line)

    output.close()

# Usage: vgpostprocess.py callgrind.out.<pid> symbols.out

if __name__ == "__main__":
    main()
