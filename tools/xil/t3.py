
delims = ["'''", '"""', "'", '"']

import re

pattern = '(' + re.escape (delims[0])
for delim in delims[1:]:
    pattern = pattern + '|' + re.escape (delim)
pattern = pattern + ')'

print 'pattern=',pattern
p = re.compile (pattern)

strs = [ 'Das ist ein "test"',
    "das ist ein 'test'",
    'das ist ein """test"""',
    "das ist ein '''test'''"]

for str in strs:
    m = p.search (str)
    print
    print 'Text=',str
    print `m`
    print m.group (1)
