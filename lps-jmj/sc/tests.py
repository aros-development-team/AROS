# File: tests.py
# Author: Oliver Steele
# Description: Script compiler test cases

# * P_LZ_COPYRIGHT_BEGIN ******************************************************
# * Copyright 2001-2004 Laszlo Systems, Inc.  All Rights Reserved.            *
# * Use is subject to license terms.                                          *
# * P_LZ_COPYRIGHT_END ********************************************************

# TODO: [2003-02-18 ows] add test cases for un- and obfuscated
# assignment coalescence, activation object, script pragma

from testing import *
False, True = 0, 1

resetTests()

DefineTests(
    'types',
    ([Expr(v) for v in
      '-1 0 1 1.0 1.5 1.0e2 -0 -0.0 0xffffffff "s" true false null infinity'.split()]))

DefineTests(
    'expressions',
    Exprs('this include'.split()) +
    Exprs(['b %s c' % op for op in '+-*/%&|^'] + \
          ['b %s c' % op for op in '<< >> >>> && || < > <= >= == !='.split()]) + \
    [Expr('b %s c' % op, flash=False) for op in '=== !=='.split()] + \
    Exprs(['%sb' % op for op in '++ -- + - ~ !'.split()] + \
          ['b%s' % op for op in '++ --'.split()] + \
          ['%s b' % op for op in 'delete void typeof'.split()] + \
          ['b ? c : d',
           'a-b-c',
           'b=c',
           'b+=c',
           'a && b && c',
           'a || b || c',
           'a,b']))

DefineTests(
    'variables',
    ['a = 1',
    'var a',
    'var a = 1',
    'function f(a) {return a}',
    'function f() {return g}',
    'function f() {g = 1}',
    'function f() {var v}',
    'function f() {var v=1}',
    ])

DefineTests(
    'assignment',
    ['a = b',
     'a = b = c'] + \
    ['a %s= b' % op for op in '* / % + - << >> >>> & ^ |'.split()])

DefineTests(
    'incf',
    Stmts(['a++', # push 'a'; dup; get; increment; set
           '++a', # push 'a'; dup; get; increment; set
           'a+=b=c',
           'a+=b+=c']) +
    Exprs(['b++', # push 'a', 'b'; get; increment; set; set
           '++b', # push 'b'; dup; get; increment; set; set
           'b+=c']))

DefineTests(
    'statements',
    Stmts(['a',
           '1',
           # test the empty statement:
           'function f(){do {a=b} while (c);}']))

DefineTests(
    'functions',
    ['function f() {}',
     'function f() {g()}',
     'function f() {return 1}',
     'function f(a) {g(a)}',
     'function f(a, b) {return a+b}',
     'function f() {return;}', # flash requires the semicolon
     'f = function () {return 1}',
     'f = function (a) {return a}',
     'f = function (a,b) {return a}',
     'v = f(a,b)',
     'f = function () {f(a,b)}',
     'f = function () {v = f(a,b)}',
    ])

"""DefineTests(
    'general',
    ["var a = 1 - (2 * 3); var b = a + 1",
     "var a = 1.0 - (2.0 * 3.0); var b = a + 1",
     'void(a)<=b>"c"||0(!1 && !0);',
     'function f() {void(1); delete a}',
    ])"""

DefineTests(
    'control',
    Stmts(['if (a) f();',
           'if (a) f(); else g()',
           'if (a) {} else g()',
           'if (a < b) f(); else g()',
           'if (a > b) f(); else g()',
           'with (a) {g()}',
           'with (a+b) {g();h()}',
           ]))

DefineTests(
    'iteration',
    Stmts(['for (i = 1; i < 10; i++) {g()}',
           'for (var i = 1; i < 10; i++) {g()}',
           'for (i in obj) {g()}',
           'for (var i in obj) {g()}',
           'for (i in {a:1, b:2}) {g()}',
           'while (i < j) {i++}',
           'do {i++} while (i<j)',
           ]))

DefineTests(
    'switch',
    Stmts(['switch (a) {case 1: f(); case 2: g(); default: d()}',
           'switch (a) {case 1: a(); break; default: b()}']),
    flash=False)

DefineTests(
    'break',
    Stmts(['while (i < j) {if (a) break; f()}',
           'while (i < j) {if (a) continue; f()}',
           'do {break; f()} while (a)',
           ('do {continue; f()} while (a)', {'flash': False}),
           'for (i = 1; i < 10; i++) {break; f()}',
           'for (i = 1; i < 10; i++) {continue; f()}']) +
    Stmts(['for (p in obj) {break; f()}',
           'for (p in obj) {continue; f()}'],
          flash=False))

DefineTests(
    'label',
    Stmts(['a: while (f()) b: while (g()) {break; h()}',
           'a: while (f()) b: while (g()) {break b; h()}',
           'a: while (f()) b: while (g()) {break a; h()}',
           'a: while (f()) b: for (p in obj) {break; h()}',
           'a: while (f()) b: for (p in obj) {break b; h()}',
           'a: while (f()) b: for (p in obj) {break a; h()}',
           'a: for (p in obj) b: while (g()) {break; h()}',
           'a: for (p in obj) b: while (g()) {break b; h()}',
           'a: for (p in obj) b: while (g()) {break a; h()}',
           'a: for (p in obj) b: for (p in obj) {break; h()}',
           'a: for (p in obj) b: for (p in obj) {break b; h()}',
           'a: for (p in obj) b: for (p in obj) {break a; h()}',
           ]),
    flash=False)

DefineTests(
    'return',
    Stmts(['return',
           'for (p in obj) return',
           'for (p in obj) for (q in obj2) return'],
           flash=False) +
    Stmts(['return 1',
           'for (p in obj) return 1',
           'for (p in obj) for (q in obj2) return 1',
           ],
          flash=False))

DefineTests(
    'references',
    [Expr('b.c'),
     Stmt('a.b = c.d'),
     Expr('b.c++'),
     Expr('++b.c'),
     Stmt('b.c++'),
     Stmt('++b.c'),
     Expr('b.c.d++'),
     Expr('++b.c.d'),
     Stmt('b.c.d++'),
     Stmt('++b.c.d')] +
    Stmts(['f().c',
           'f().c++',
           '++f().c',
           'a=++f().c',
           'a=f().c++'],
          flash=False)
    )
    
DefineTests(
    'objects',
    [Expr('{a: 1, b: 2, c: 3}'),
     Expr('{a: b}'),
     Expr('{a: 1, "b": 2, 3: 3}', flash=False),
     Expr('new C'),
     Expr('new a.b'),
     Expr('new C()'),
     Expr('new a.b.C(1,2)'),
     Expr('new C(1, 2)'),
     # Expr('new f()()'),
     Expr('a instanceof b', flash=False),
     Expr('delete a'),
     Expr('delete a.b'),
     Expr('typeof a'),
     Expr('b.c()'),
     Expr('b.c(1,2)'),
     ])

DefineTests(
    'arrays',
    Exprs(['[]',
           ('[,]', {'flash': False}),
           '[1,2,3]',
           ('[,2,,,3]', {'flash': False}),
           'a[0]',
           'a[1][2]',
           'a[0](1,2)']) +
    Stmts(['a[0] = 1',
           'a[1][2] = 1',
           ]) +
    Exprs(['a[0]++',
           '++a[0]',
           'a[i++]++',
           '++a[i++]',
           'a[b[i++]++]++',
           'a[++b[i++]]++',
           '++a[b[i++]++]',
           '++a[++b[i++]]',
           'a[f()]++',
           '++a[f()]',
           'a=a[f()]++',
           'a=++a[f()]',
           ]))


DefineTests(
    'properties',
    Exprs(['a.b',
           'a.b.c',
           'a.b()',
           'a.b.c()',
           'a[b]',
           'a[b][c]',
           'a[b]()',
           'a[b][c]()',
           ]))

#DefineTests(
#    'class',
#    ['class C {}',
#     'class C extends B {}',
#     'class C {var a}',
#     'class C {var a=1}',
#     'class C {var a=1, b=2}',
#     'class C extends B {var a=1}',
#     'class C {function C(x) {this.x=x}}',
#     'class C extends B {function C(x) {this.x=x}}',
#     'class C extends B {function C(x) {super(x)}}',
#     'class C {function f(x) {this.x=x}}',
#     'class C {function f(x) {super.f(a, b)}}',
#    ],
#    flash=False)

def makeConstraintFunction(name, expr):
    return """f = function (){\n#pragma 'constraintFunction'\nwith (this) this.setAttribute('%s', %s)}""" % (name, expr)

DefineTests(
    'constraints',
    [makeConstraintFunction('width', 'a.b'),
     makeConstraintFunction('width', 'parent.width'),
     makeConstraintFunction('width', 'f()'),
     makeConstraintFunction('width', 'f(c.d, e)'),
     makeConstraintFunction('width', 'parent.getMouse("x")'),
     makeConstraintFunction('width', 'a.b+f(c.d, e)'),
     ])

def CompatExprs(l):
    return ['function f() {\n#pragma "flashCompilerCompatability"\nreturn %s}' % s
            for s in l]


DefineTests(
    'inlines',
    Exprs(['getTimer()',
           'getTimer(x)']) +
    CompatExprs(
    ['removeMovieClip(x)',
     'removeMovieClip()',
     'ord(x)', 'ord()',
     'targetPath(x)', 'targetPath()',
     'getVersion(x)', 'getVersion()',
     'eval(x)', 'eval()']
    ) +
    ['function f(){\n#pragma "flashCompilerCompatability"\n#pragma "trace=flash"\ntrace(x)}',
     'function f(){\n#pragma "flashCompilerCompatability"\n#pragma "trace=debug"\ntrace(x)}',
     'function f(){\n#pragma "flashCompilerCompatability"\n#pragma "trace=off"\ntrace(x)}',
     ])

DefineTests(
    'flasm',
    [# statement flasm blocks
     '#pragma "flashCompilerCompatability"\nfunction f(){$flasm;"push 1";"pop";$end}',
     # if flasm
     '#pragma "flashCompilerCompatability"\nfunction f(){if ($flasm){"push 1";"pop"}}',
     # if flasm with flasm off
     '#pragma "flashCompilerCompatability=false"\nfunction f(){if ($flasm){"push 1";"pop"}}',
    '#pragma "flashCompilerCompatability"\nfunction f(){$flasm;"push 1.2, 2, \'str\', r:0";$end}',
     ])

DefineTests(
    'assembler',
    [# dup
    '#pragma "flashCompilerCompatability"\nfunction f() {$flasm;"push 0";"dup";$end}'
    ])

ConstraintTests = [
    ('a', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'a']}}"),
    ('a.b', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a, 'b']}}"),
    ('a.b.c', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a.b, 'c']}}"),
    ('a+b', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'a', this, 'b']}}"),
    ('a.b + c.d', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a, 'b', c, 'd']}}"),
    ('f()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [].concat(f.dependencies(this, undefined) || [])}}"),
    ('f(a)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'a'].concat(f.dependencies(this, undefined, a) || [])}}"),
    ('f("a")', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [].concat(f.dependencies(this, undefined, 'a') || [])}}"),
    ('f(a, b)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'a', this, 'b'].concat(f.dependencies(this, undefined, a, b) || [])}}"),
    ('f(a.b)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a, 'b'].concat(f.dependencies(this, undefined, a.b) || [])}}"),
    ('f() + g()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [].concat(f.dependencies(this, undefined) || []).concat(g.dependencies(this, undefined) || [])}}"),
    ('a.f()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [].concat(a.f.dependencies(this, a) || [])}}"),
    ('a.f(a)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'a'].concat(a.f.dependencies(this, a, a) || [])}}"),
    ('a.f() + b.g()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [].concat(a.f.dependencies(this, a) || []).concat(b.g.dependencies(this, b) || [])}}"),
    ('f(g(a.b))', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a, 'b'].concat(f.dependencies(this, undefined, g(a.b)) || []).concat(g.dependencies(this, undefined, a.b) || [])}}"),
    ('a.f()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [].concat(a.f.dependencies(this, a) || [])}}"),
    ('a.f(b)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'b'].concat(a.f.dependencies(this, a, b) || [])}}"),
    ('a.b + c.d.e', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a, 'b', c.d, 'e']}}"),
    ("a.b + getMouse() + other.getRelative(a, 'b', b)", "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a, 'b', this, 'a', this, 'b'].concat(getMouse.dependencies(this, undefined) || []).concat(other.getRelative.dependencies(this, other, a, 'b', b) || [])}}"),
    ('a.b.measureWidth()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [].concat(a.b.measureWidth.dependencies(this, a.b) || [])}}"),
    ('a.b.findView().c', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [a.b.findView(), 'c'].concat(a.b.findView.dependencies(this, a.b) || [])}}")
]

MetaConstraintTests = [
('a', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'a']}}"),
('a.b', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a, 'b']\narguments.callee.d.metadependencies = [this, 'a']\nreturn arguments.callee.d}}"),
('a.b.c', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a.b, 'c']\narguments.callee.d.metadependencies = [this, 'a', a, 'b']\nreturn arguments.callee.d}}"),
('a+b', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {return [this, 'a', this, 'b']}}"),
('a.b + c.d', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a, 'b', c, 'd']\narguments.callee.d.metadependencies = [this, 'a', this, 'c']\nreturn arguments.callee.d}}"),
('f()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [].concat(f.dependencies(this, undefined) || [])\narguments.callee.d.metadependencies = [this, 'f']\nreturn arguments.callee.d}}"),
('f(a)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [this, 'a'].concat(f.dependencies(this, undefined, a) || [])\narguments.callee.d.metadependencies = [this, 'f']\nreturn arguments.callee.d}}"),
('f("a")', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [].concat(f.dependencies(this, undefined, 'a') || [])\narguments.callee.d.metadependencies = [this, 'f']\nreturn arguments.callee.d}}"),
('f(a, b)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [this, 'a', this, 'b'].concat(f.dependencies(this, undefined, a, b) || [])\narguments.callee.d.metadependencies = [this, 'f']\nreturn arguments.callee.d}}"),
('f(a.b)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a, 'b'].concat(f.dependencies(this, undefined, a.b) || [])\narguments.callee.d.metadependencies = [this, 'f', this, 'a']\nreturn arguments.callee.d}}"),
('f() + g()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [].concat(f.dependencies(this, undefined) || []).concat(g.dependencies(this, undefined) || [])\narguments.callee.d.metadependencies = [this, 'f', this, 'g']\nreturn arguments.callee.d}}"),
('a.f()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [].concat(a.f.dependencies(this, a) || [])\narguments.callee.d.metadependencies = [this, 'a', a, 'f']\nreturn arguments.callee.d}}"),
('a.f(a)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [this, 'a'].concat(a.f.dependencies(this, a, a) || [])\narguments.callee.d.metadependencies = [this, 'a', a, 'f']\nreturn arguments.callee.d}}"),
('a.f() + b.g()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [].concat(a.f.dependencies(this, a) || []).concat(b.g.dependencies(this, b) || [])\narguments.callee.d.metadependencies = [this, 'a', a, 'f', this, 'b', b, 'g']\nreturn arguments.callee.d}}"),
('f(g(a.b))', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a, 'b'].concat(f.dependencies(this, undefined, g(a.b)) || []).concat(g.dependencies(this, undefined, a.b) || [])\narguments.callee.d.metadependencies = [this, 'f', this, 'g', this, 'a']\nreturn arguments.callee.d}}"),
('a.f()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [].concat(a.f.dependencies(this, a) || [])\narguments.callee.d.metadependencies = [this, 'a', a, 'f']\nreturn arguments.callee.d}}"),
('a.f(b)', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [this, 'b'].concat(a.f.dependencies(this, a, b) || [])\narguments.callee.d.metadependencies = [this, 'a', a, 'f']\nreturn arguments.callee.d}}"),
('a.b + c.d.e', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a, 'b', c.d, 'e']\narguments.callee.d.metadependencies = [this, 'a', this, 'c', c, 'd']\nreturn arguments.callee.d}}"),
("a.b + getMouse() + other.getRelative(a, 'b', b)", "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a, 'b', this, 'a', this, 'b'].concat(getMouse.dependencies(this, undefined) || []).concat(other.getRelative.dependencies(this, other, a, 'b', b) || [])\narguments.callee.d.metadependencies = [this, 'a', this, 'getMouse', this, 'other', other, 'getRelative']\nreturn arguments.callee.d}}"),
('a.b.measureWidth()', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [].concat(a.b.measureWidth.dependencies(this, a.b) || [])\narguments.callee.d.metadependencies = [this, 'a', a, 'b', a.b, 'measureWidth']\nreturn arguments.callee.d}}"),
('a.b.findView().c', "function test_dependencies() {#pragma 'warnUndefinedReferences=false'\nwith (this) {arguments.callee.d = [a.b.findView(), 'c'].concat(a.b.findView.dependencies(this, a.b) || [])\narguments.callee.d.metadependencies = [this, 'a', a, 'b', a.b, 'findView'].concat(a.b.findView.dependencies(this, a.b) || [])\nreturn arguments.callee.d}}")
    ]
