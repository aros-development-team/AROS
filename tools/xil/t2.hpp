test
test
<rem>auskommentiert</rem><sws>
<def name=test arg1='hallo' arg2='hallo, welt'
><expand>arg1=$arg1 arg2=$arg2</expand><
/def>
<rem>Das gibt *eine* Leerzeile in der Ausgabe</rem><sws>

<test>
<test arg1='test'>
<test arg2='test'>
<sws>

<bdef name=btest arg1='hallo' arg2='hallo, welt'
>arg1=$arg1 arg2=$arg2 body=$body<
/bdef>
<sws>

<btest>body1</btest>
<btest arg1=test>body2</btest>
<btest arg2=test>
body3 <expand>arg1=$arg1 arg2=$arg2</expand>
</btest>
<sws>

<edef name=btt begin='<B><TT>' end='</TT></B>'>
<sws>

<btt>test</btt>
<sws>

<set test1='''foo bar'''><sws>
<expand>$test1</expand>
<sws>

<verb text="<b$test">
<verbatim>
<xxx><bdef><test><xxx>
</verbatim>
<sws>

<python code='''
print __name__

def PrintDict (dict):
    print '{'
    for key, item in dict.items ():
	print '  %s: %s' % (key, item)
    print '}'

#print 'globals='
#PrintDict (globals())
#print 'locals='
#PrintDict (locals())
print 'arg1=',args['ARG1']
''' arg1='test'>
<sws>

<set chapter=0 section=0 subsection=0><sws>
<REM><python code='''
print `vardb.keys ()`,`vardb.varstack`
'''></REM><sws>

<bdef name=chapter><sws>
    <python code='''
#print 'CHAPTER:',`vardb.pool`
vardb['chapter'] = int (vardb['chapter']) + 1
'''><sws>
    <expand>Kapitel $chapter $body</expand><sws>
    <set section=0 subsection=0
></bdef><sws>

<bdef name=section><python code='''
vardb['section'] = int (vardb['section']) + 1'''
><expand>$chapter.$section $body</expand><set subsection=0></bdef><sws>

<bdef name=subsection><python 
code='''vardb['subsection'] = int (vardb['subsection']) + 1'''
><expand>$chapter.$section.$subsection $body</expand></bdef><sws>

<chapter>Einleitung</chapter>
<section>Übersicht</section>
<chapter>Übersicht</chapter>
<section>A</section>
<section>B</section>
<subsection>B</subsection>
<section>C</section>
