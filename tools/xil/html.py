
import xil

class HTMLParser (xil.Parser):
    begin = '<'
    end = '>'

class HTMLEnvHandler (xil.EnvHandler):
    def __init__ (self, name):
	xil.EnvHandler.__init__ (self, name, '/'+name)

class H1Handler (HTMLEnvHandler):
    def __init__ (self):
	HTMLEnvHandler.__init__ (self, 'H1')

if __name__ == '__main__':
    input = '''
<H1>Einführung</H1>

    das ist der Text.

<DEF title="Definition der Definition">
Das ist der Text der Definition
</DEF>
'''

    import cStringIO, sys, string

    class Chapter (H1Handler):
	def handle (self, parser, args, text):
	    parser.write ('\\chapter{%s}' % text)

    class DefHandler (HTMLEnvHandler):
	def __init__ (self):
	    HTMLEnvHandler.__init__ (self, 'def')

	def handle (self, parser, args, text):
	    parser.write ('\\def{%s}%s' % (args['TITLE'], string.strip (text)))

    rf = cStringIO.StringIO (input)

    parser = HTMLParser (rf, sys.stdout,
	{
	    'H1': Chapter (),
	    'DEF': DefHandler (),
	}
    )
    parser.run ()

