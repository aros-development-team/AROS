
exec:     file format elf32-m68k

Disassembly of section .text:

00000000 <main>:
       0:	4e56 0000      	linkw %fp,#0
       4:	4878 4000      	pea 4000 <Exec_OpenLibrary+0x1a>
       8:	2f3c 0002 0000 	movel #131072,%sp@-
       e:	61ff 0000 0000 	bsrl 10 <main+0x10>
      14:	508f           	addql #8,%sp
      16:	7000           	moveq #0,%d0
      18:	6000 0002      	braw 1c <main+0x1c>
      1c:	4e5e           	unlk %fp
      1e:	4e75           	rts

00000020 <pause>:
      20:	4e56 fff8      	linkw %fp,#-8
      24:	42ae fffc      	clrl %fp@(-4)
      28:	7201           	moveq #1,%d1
      2a:	2d41 fff8      	movel %d1,%fp@(-8)
      2e:	222e fffc      	movel %fp@(-4),%d1
      32:	b2ae 0008      	cmpl %fp@(8),%d1
      36:	6d02           	blts 3a <pause+0x1a>
      38:	6028           	bras 62 <pause+0x42>
      3a:	2f2e fff8      	movel %fp@(-8),%sp@-
      3e:	2f2e fff8      	movel %fp@(-8),%sp@-
      42:	61ff 0000 0000 	bsrl 44 <pause+0x24>
      48:	508f           	addql #8,%sp
      4a:	2f00           	movel %d0,%sp@-
      4c:	2f2e fff8      	movel %fp@(-8),%sp@-
      50:	61ff 0000 0000 	bsrl 52 <pause+0x32>
      56:	508f           	addql #8,%sp
      58:	2d40 fff8      	movel %d0,%fp@(-8)
      5c:	52ae fffc      	addql #1,%fp@(-4)
      60:	60cc           	bras 2e <pause+0xe>
      62:	4e5e           	unlk %fp
      64:	4e75           	rts

00000066 <drawlinehoriz>:
      66:	4e56 fff0      	linkw %fp,#-16
      6a:	2d78 fa00 fffc 	movel fffffa00 <__errno_location+0xffff9f6c>,%fp@(-4)
      70:	7000           	moveq #0,%d0
      72:	3038 fa0a      	movew fffffa0a <__errno_location+0xffff9f76>,%d0
      76:	2d40 fff8      	movel %d0,%fp@(-8)
      7a:	3238 fa08      	movew fffffa08 <__errno_location+0xffff9f74>,%d1
      7e:	3001           	movew %d1,%d0
      80:	e648           	lsrw #3,%d0
      82:	7200           	moveq #0,%d1
      84:	3200           	movew %d0,%d1
      86:	2d41 fff4      	movel %d1,%fp@(-12)
      8a:	42ae fff0      	clrl %fp@(-16)
      8e:	4aae 0008      	tstl %fp@(8)
      92:	6d0c           	blts a0 <drawlinehoriz+0x3a>
      94:	226e 0008      	moveal %fp@(8),%a1
      98:	b3ee fff8      	cmpal %fp@(-8),%a1
      9c:	6c02           	bges a0 <drawlinehoriz+0x3a>
      9e:	6002           	bras a2 <drawlinehoriz+0x3c>
      a0:	6036           	bras d8 <drawlinehoriz+0x72>
      a2:	4e71           	nop
      a4:	226e fff0      	moveal %fp@(-16),%a1
      a8:	b3ee fff4      	cmpal %fp@(-12),%a1
      ac:	6f02           	bles b0 <drawlinehoriz+0x4a>
      ae:	6028           	bras d8 <drawlinehoriz+0x72>
      b0:	202e fff4      	movel %fp@(-12),%d0
      b4:	5280           	addql #1,%d0
      b6:	2f2e 0008      	movel %fp@(8),%sp@-
      ba:	2f00           	movel %d0,%sp@-
      bc:	61ff 0000 0000 	bsrl be <drawlinehoriz+0x58>
      c2:	508f           	addql #8,%sp
      c4:	2200           	movel %d0,%d1
      c6:	d2ae fffc      	addl %fp@(-4),%d1
      ca:	2041           	moveal %d1,%a0
      cc:	d1ee fff0      	addal %fp@(-16),%a0
      d0:	50d0           	st %a0@
      d2:	52ae fff0      	addql #1,%fp@(-16)
      d6:	60cc           	bras a4 <drawlinehoriz+0x3e>
      d8:	4e5e           	unlk %fp
      da:	4e75           	rts

000000dc <drawlinevert>:
      dc:	4e56 fff0      	linkw %fp,#-16
      e0:	48e7 3c20      	moveml %d2-%d5/%a2,%sp@-
      e4:	2d78 fa00 fffc 	movel fffffa00 <__errno_location+0xffff9f6c>,%fp@(-4)
      ea:	7000           	moveq #0,%d0
      ec:	3038 fa0a      	movew fffffa0a <__errno_location+0xffff9f76>,%d0
      f0:	2d40 fff8      	movel %d0,%fp@(-8)
      f4:	7000           	moveq #0,%d0
      f6:	3038 fa08      	movew fffffa08 <__errno_location+0xffff9f74>,%d0
      fa:	2d40 fff4      	movel %d0,%fp@(-12)
      fe:	42ae fff0      	clrl %fp@(-16)
     102:	4aae 0008      	tstl %fp@(8)
     106:	6d0c           	blts 114 <drawlinevert+0x38>
     108:	2a2e 0008      	movel %fp@(8),%d5
     10c:	baae fff4      	cmpl %fp@(-12),%d5
     110:	6c02           	bges 114 <drawlinevert+0x38>
     112:	6002           	bras 116 <drawlinevert+0x3a>
     114:	607e           	bras 194 <drawlinevert+0xb8>
     116:	202e fff4      	movel %fp@(-12),%d0
     11a:	2200           	movel %d0,%d1
     11c:	e681           	asrl #3,%d1
     11e:	2d41 fff4      	movel %d1,%fp@(-12)
     122:	2a2e fff0      	movel %fp@(-16),%d5
     126:	baae fff8      	cmpl %fp@(-8),%d5
     12a:	6d02           	blts 12e <drawlinevert+0x52>
     12c:	6066           	bras 194 <drawlinevert+0xb8>
     12e:	202e fff4      	movel %fp@(-12),%d0
     132:	5280           	addql #1,%d0
     134:	2f2e fff0      	movel %fp@(-16),%sp@-
     138:	2f00           	movel %d0,%sp@-
     13a:	61ff 0000 0000 	bsrl 13c <drawlinevert+0x60>
     140:	508f           	addql #8,%sp
     142:	222e 0008      	movel %fp@(8),%d1
     146:	2401           	movel %d1,%d2
     148:	e682           	asrl #3,%d2
     14a:	d0ae fffc      	addl %fp@(-4),%d0
     14e:	2440           	moveal %d0,%a2
     150:	202e fff4      	movel %fp@(-12),%d0
     154:	5280           	addql #1,%d0
     156:	2f2e fff0      	movel %fp@(-16),%sp@-
     15a:	2f00           	movel %d0,%sp@-
     15c:	61ff 0000 0000 	bsrl 15e <drawlinevert+0x82>
     162:	508f           	addql #8,%sp
     164:	2200           	movel %d0,%d1
     166:	262e 0008      	movel %fp@(8),%d3
     16a:	2003           	movel %d3,%d0
     16c:	e680           	asrl #3,%d0
     16e:	d2ae fffc      	addl %fp@(-4),%d1
     172:	2041           	moveal %d1,%a0
     174:	7207           	moveq #7,%d1
     176:	c2ae 0008      	andl %fp@(8),%d1
     17a:	7607           	moveq #7,%d3
     17c:	9681           	subl %d1,%d3
     17e:	7801           	moveq #1,%d4
     180:	2204           	movel %d4,%d1
     182:	e7a9           	lsll %d3,%d1
     184:	1a30 0800      	moveb %a0@(00000000,%d0:l),%d5
     188:	8a01           	orb %d1,%d5
     18a:	1585 2800      	moveb %d5,%a2@(00000000,%d2:l)
     18e:	52ae fff0      	addql #1,%fp@(-16)
     192:	608e           	bras 122 <drawlinevert+0x46>
     194:	4cee 043c ffdc 	moveml %fp@(-36),%d2-%d5/%a2
     19a:	4e5e           	unlk %fp
     19c:	4e75           	rts

0000019e <setpixel>:
     19e:	4e56 fff4      	linkw %fp,#-12
     1a2:	48e7 3c20      	moveml %d2-%d5/%a2,%sp@-
     1a6:	2d78 fa00 fffc 	movel fffffa00 <__errno_location+0xffff9f6c>,%fp@(-4)
     1ac:	7000           	moveq #0,%d0
     1ae:	3038 fa0a      	movew fffffa0a <__errno_location+0xffff9f76>,%d0
     1b2:	2d40 fff8      	movel %d0,%fp@(-8)
     1b6:	7000           	moveq #0,%d0
     1b8:	3038 fa08      	movew fffffa08 <__errno_location+0xffff9f74>,%d0
     1bc:	2d40 fff4      	movel %d0,%fp@(-12)
     1c0:	4aae 0008      	tstl %fp@(8)
     1c4:	6d1c           	blts 1e2 <setpixel+0x44>
     1c6:	2a2e 0008      	movel %fp@(8),%d5
     1ca:	baae fff4      	cmpl %fp@(-12),%d5
     1ce:	6c12           	bges 1e2 <setpixel+0x44>
     1d0:	4aae 000c      	tstl %fp@(12)
     1d4:	6d0c           	blts 1e2 <setpixel+0x44>
     1d6:	2a2e 000c      	movel %fp@(12),%d5
     1da:	baae fff8      	cmpl %fp@(-8),%d5
     1de:	6c02           	bges 1e2 <setpixel+0x44>
     1e0:	6002           	bras 1e4 <setpixel+0x46>
     1e2:	606c           	bras 250 <setpixel+0xb2>
     1e4:	202e fff4      	movel %fp@(-12),%d0
     1e8:	2200           	movel %d0,%d1
     1ea:	e681           	asrl #3,%d1
     1ec:	2d41 fff4      	movel %d1,%fp@(-12)
     1f0:	202e fff4      	movel %fp@(-12),%d0
     1f4:	5280           	addql #1,%d0
     1f6:	2f2e 000c      	movel %fp@(12),%sp@-
     1fa:	2f00           	movel %d0,%sp@-
     1fc:	61ff 0000 0000 	bsrl 1fe <setpixel+0x60>
     202:	508f           	addql #8,%sp
     204:	222e 0008      	movel %fp@(8),%d1
     208:	2401           	movel %d1,%d2
     20a:	e682           	asrl #3,%d2
     20c:	d0ae fffc      	addl %fp@(-4),%d0
     210:	2440           	moveal %d0,%a2
     212:	202e fff4      	movel %fp@(-12),%d0
     216:	5280           	addql #1,%d0
     218:	2f2e 000c      	movel %fp@(12),%sp@-
     21c:	2f00           	movel %d0,%sp@-
     21e:	61ff 0000 0000 	bsrl 220 <setpixel+0x82>
     224:	508f           	addql #8,%sp
     226:	2200           	movel %d0,%d1
     228:	262e 0008      	movel %fp@(8),%d3
     22c:	2003           	movel %d3,%d0
     22e:	e680           	asrl #3,%d0
     230:	d2ae fffc      	addl %fp@(-4),%d1
     234:	2041           	moveal %d1,%a0
     236:	7207           	moveq #7,%d1
     238:	c2ae 0008      	andl %fp@(8),%d1
     23c:	7607           	moveq #7,%d3
     23e:	9681           	subl %d1,%d3
     240:	7801           	moveq #1,%d4
     242:	2204           	movel %d4,%d1
     244:	e7a9           	lsll %d3,%d1
     246:	1a30 0800      	moveb %a0@(00000000,%d0:l),%d5
     24a:	8a01           	orb %d1,%d5
     24c:	1585 2800      	moveb %d5,%a2@(00000000,%d2:l)
     250:	4cee 043c ffe0 	moveml %fp@(-32),%d2-%d5/%a2
     256:	4e5e           	unlk %fp
     258:	4e75           	rts

0000025a <clearpixel>:
     25a:	4e56 fff4      	linkw %fp,#-12
     25e:	48e7 3c20      	moveml %d2-%d5/%a2,%sp@-
     262:	2d78 fa00 fffc 	movel fffffa00 <__errno_location+0xffff9f6c>,%fp@(-4)
     268:	7000           	moveq #0,%d0
     26a:	3038 fa0a      	movew fffffa0a <__errno_location+0xffff9f76>,%d0
     26e:	2d40 fff8      	movel %d0,%fp@(-8)
     272:	7000           	moveq #0,%d0
     274:	3038 fa08      	movew fffffa08 <__errno_location+0xffff9f74>,%d0
     278:	2d40 fff4      	movel %d0,%fp@(-12)
     27c:	4aae 0008      	tstl %fp@(8)
     280:	6d1c           	blts 29e <clearpixel+0x44>
     282:	2a2e 0008      	movel %fp@(8),%d5
     286:	baae fff4      	cmpl %fp@(-12),%d5
     28a:	6c12           	bges 29e <clearpixel+0x44>
     28c:	4aae 000c      	tstl %fp@(12)
     290:	6d0c           	blts 29e <clearpixel+0x44>
     292:	2a2e 000c      	movel %fp@(12),%d5
     296:	baae fff8      	cmpl %fp@(-8),%d5
     29a:	6c02           	bges 29e <clearpixel+0x44>
     29c:	6002           	bras 2a0 <clearpixel+0x46>
     29e:	6070           	bras 310 <clearpixel+0xb6>
     2a0:	202e fff4      	movel %fp@(-12),%d0
     2a4:	2200           	movel %d0,%d1
     2a6:	e681           	asrl #3,%d1
     2a8:	2d41 fff4      	movel %d1,%fp@(-12)
     2ac:	202e fff4      	movel %fp@(-12),%d0
     2b0:	5280           	addql #1,%d0
     2b2:	2f2e 000c      	movel %fp@(12),%sp@-
     2b6:	2f00           	movel %d0,%sp@-
     2b8:	61ff 0000 0000 	bsrl 2ba <clearpixel+0x60>
     2be:	508f           	addql #8,%sp
     2c0:	222e 0008      	movel %fp@(8),%d1
     2c4:	2401           	movel %d1,%d2
     2c6:	e682           	asrl #3,%d2
     2c8:	d0ae fffc      	addl %fp@(-4),%d0
     2cc:	2440           	moveal %d0,%a2
     2ce:	202e fff4      	movel %fp@(-12),%d0
     2d2:	5280           	addql #1,%d0
     2d4:	2f2e 000c      	movel %fp@(12),%sp@-
     2d8:	2f00           	movel %d0,%sp@-
     2da:	61ff 0000 0000 	bsrl 2dc <clearpixel+0x82>
     2e0:	508f           	addql #8,%sp
     2e2:	2200           	movel %d0,%d1
     2e4:	262e 0008      	movel %fp@(8),%d3
     2e8:	2003           	movel %d3,%d0
     2ea:	e680           	asrl #3,%d0
     2ec:	d2ae fffc      	addl %fp@(-4),%d1
     2f0:	2041           	moveal %d1,%a0
     2f2:	7207           	moveq #7,%d1
     2f4:	c2ae 0008      	andl %fp@(8),%d1
     2f8:	7607           	moveq #7,%d3
     2fa:	9681           	subl %d1,%d3
     2fc:	7801           	moveq #1,%d4
     2fe:	2204           	movel %d4,%d1
     300:	e7a9           	lsll %d3,%d1
     302:	1601           	moveb %d1,%d3
     304:	4603           	notb %d3
     306:	1a30 0800      	moveb %a0@(00000000,%d0:l),%d5
     30a:	ca03           	andb %d3,%d5
     30c:	1585 2800      	moveb %d5,%a2@(00000000,%d2:l)
     310:	4cee 043c ffe0 	moveml %fp@(-32),%d2-%d5/%a2
     316:	4e5e           	unlk %fp
     318:	4e75           	rts

0000031a <clearscreen>:
     31a:	4e56 ffec      	linkw %fp,#-20
     31e:	2d78 fa00 fffc 	movel fffffa00 <__errno_location+0xffff9f6c>,%fp@(-4)
     324:	7000           	moveq #0,%d0
     326:	3038 fa0a      	movew fffffa0a <__errno_location+0xffff9f76>,%d0
     32a:	2d40 fff8      	movel %d0,%fp@(-8)
     32e:	3238 fa08      	movew fffffa08 <__errno_location+0xffff9f74>,%d1
     332:	3001           	movew %d1,%d0
     334:	e648           	lsrw #3,%d0
     336:	7200           	moveq #0,%d1
     338:	3200           	movew %d0,%d1
     33a:	2d41 fff4      	movel %d1,%fp@(-12)
     33e:	42ae fff0      	clrl %fp@(-16)
     342:	226e fff0      	moveal %fp@(-16),%a1
     346:	b3ee fff8      	cmpal %fp@(-8),%a1
     34a:	6d02           	blts 34e <clearscreen+0x34>
     34c:	6040           	bras 38e <clearscreen+0x74>
     34e:	42ae ffec      	clrl %fp@(-20)
     352:	226e ffec      	moveal %fp@(-20),%a1
     356:	b3ee fff4      	cmpal %fp@(-12),%a1
     35a:	6f02           	bles 35e <clearscreen+0x44>
     35c:	602a           	bras 388 <clearscreen+0x6e>
     35e:	202e fff4      	movel %fp@(-12),%d0
     362:	5280           	addql #1,%d0
     364:	2f2e fff0      	movel %fp@(-16),%sp@-
     368:	2f00           	movel %d0,%sp@-
     36a:	61ff 0000 0000 	bsrl 36c <clearscreen+0x52>
     370:	508f           	addql #8,%sp
     372:	2200           	movel %d0,%d1
     374:	d2ae fffc      	addl %fp@(-4),%d1
     378:	2041           	moveal %d1,%a0
     37a:	d1ee ffec      	addal %fp@(-20),%a0
     37e:	10ae 000b      	moveb %fp@(11),%a0@
     382:	52ae ffec      	addql #1,%fp@(-20)
     386:	60ca           	bras 352 <clearscreen+0x38>
     388:	52ae fff0      	addql #1,%fp@(-16)
     38c:	60b4           	bras 342 <clearscreen+0x28>
     38e:	4e5e           	unlk %fp
     390:	4e75           	rts

00000392 <flashscreen>:
     392:	4e56 fff8      	linkw %fp,#-8
     396:	42ae fffc      	clrl %fp@(-4)
     39a:	202e fffc      	movel %fp@(-4),%d0
     39e:	b0ae 0008      	cmpl %fp@(8),%d0
     3a2:	6d02           	blts 3a6 <flashscreen+0x14>
     3a4:	6044           	bras 3ea <flashscreen+0x58>
     3a6:	42a7           	clrl %sp@-
     3a8:	6100 ff70      	bsrw 31a <clearscreen>
     3ac:	588f           	addql #4,%sp
     3ae:	42ae fff8      	clrl %fp@(-8)
     3b2:	0cae 0000 270f 	cmpil #9999,%fp@(-8)
     3b8:	fff8 
     3ba:	6f02           	bles 3be <flashscreen+0x2c>
     3bc:	6006           	bras 3c4 <flashscreen+0x32>
     3be:	52ae fff8      	addql #1,%fp@(-8)
     3c2:	60ee           	bras 3b2 <flashscreen+0x20>
     3c4:	4878 00ff      	pea ff <drawlinevert+0x23>
     3c8:	6100 ff50      	bsrw 31a <clearscreen>
     3cc:	588f           	addql #4,%sp
     3ce:	42ae fff8      	clrl %fp@(-8)
     3d2:	0cae 0000 270f 	cmpil #9999,%fp@(-8)
     3d8:	fff8 
     3da:	6f02           	bles 3de <flashscreen+0x4c>
     3dc:	6006           	bras 3e4 <flashscreen+0x52>
     3de:	52ae fff8      	addql #1,%fp@(-8)
     3e2:	60ee           	bras 3d2 <flashscreen+0x40>
     3e4:	52ae fffc      	addql #1,%fp@(-4)
     3e8:	60b0           	bras 39a <flashscreen+0x8>
     3ea:	4e5e           	unlk %fp
     3ec:	4e75           	rts

000003ee <drawcross>:
     3ee:	4e56 0000      	linkw %fp,#0
     3f2:	42a7           	clrl %sp@-
     3f4:	6100 ff24      	bsrw 31a <clearscreen>
     3f8:	588f           	addql #4,%sp
     3fa:	4878 0050      	pea 50 <pause+0x30>
     3fe:	6100 fc66      	bsrw 66 <drawlinehoriz>
     402:	588f           	addql #4,%sp
     404:	4878 0050      	pea 50 <pause+0x30>
     408:	6100 fcd2      	bsrw dc <drawlinevert>
     40c:	588f           	addql #4,%sp
     40e:	2f3c 0001 86a0 	movel #100000,%sp@-
     414:	6100 fc0a      	bsrw 20 <pause>
     418:	588f           	addql #4,%sp
     41a:	4e5e           	unlk %fp
     41c:	4e75           	rts

0000041e <showsuccess>:
     41e:	4e56 fff8      	linkw %fp,#-8
     422:	7000           	moveq #0,%d0
     424:	3038 fa08      	movew fffffa08 <__errno_location+0xffff9f74>,%d0
     428:	2d40 fffc      	movel %d0,%fp@(-4)
     42c:	42ae fff8      	clrl %fp@(-8)
     430:	222e fff8      	movel %fp@(-8),%d1
     434:	b2ae fffc      	cmpl %fp@(-4),%d1
     438:	6d02           	blts 43c <showsuccess+0x1e>
     43a:	601a           	bras 456 <showsuccess+0x38>
     43c:	2f2e fff8      	movel %fp@(-8),%sp@-
     440:	6100 fc24      	bsrw 66 <drawlinehoriz>
     444:	588f           	addql #4,%sp
     446:	52ae fff8      	addql #1,%fp@(-8)
     44a:	4878 2710      	pea 2710 <Exec_AllocPooled+0x40>
     44e:	6100 fbd0      	bsrw 20 <pause>
     452:	588f           	addql #4,%sp
     454:	60da           	bras 430 <showsuccess+0x12>
     456:	42a7           	clrl %sp@-
     458:	6100 fec0      	bsrw 31a <clearscreen>
     45c:	588f           	addql #4,%sp
     45e:	4e5e           	unlk %fp
     460:	4e75           	rts
     462:	4e75           	rts

00000464 <entry>:
     464:	4e56 fff0      	linkw %fp,#-16
     468:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
     46c:	246e 0008      	moveal %fp@(8),%a2
     470:	206e 0010      	moveal %fp@(16),%a0
     474:	42ae fffc      	clrl %fp@(-4)
     478:	7440           	moveq #64,%d2
     47a:	7600           	moveq #0,%d3
     47c:	7214           	moveq #20,%d1
     47e:	23c1 0000 0000 	movel %d1,0 <main>
     484:	23c8 0000 0000 	movel %a0,0 <main>
     48a:	2f08           	movel %a0,%sp@-
     48c:	4878 0027      	pea 27 <pause+0x7>
     490:	4879 0000 0000 	pea 0 <main>
     496:	2068 fdda      	moveal %a0@(-550),%a0
     49a:	4e90           	jsr %a0@
     49c:	23c8 0000 0000 	movel %a0,0 <main>
     4a2:	4fef 000c      	lea %sp@(12),%sp
     4a6:	6606           	bnes 4ae <entry+0x4a>
     4a8:	70ff           	moveq #-1,%d0
     4aa:	6000 0374      	braw 820 <entry+0x3bc>
     4ae:	4878 0001      	pea 1 <main+0x1>
     4b2:	4879 0000 0000 	pea 0 <main>
     4b8:	6100 0370      	bsrw 82a <call_funcs>
     4bc:	2079 0000 0000 	moveal 0 <main>,%a0
     4c2:	2f08           	movel %a0,%sp@-
     4c4:	42a7           	clrl %sp@-
     4c6:	2068 fedc      	moveal %a0@(-292),%a0
     4ca:	4e90           	jsr %a0@
     4cc:	2248           	moveal %a0,%a1
     4ce:	4fef 0010      	lea %sp@(16),%sp
     4d2:	4aa9 00ac      	tstl %a1@(172)
     4d6:	6700 022a      	beqw 702 <entry+0x29e>
     4da:	4aae 000c      	tstl %fp@(12)
     4de:	6700 0184      	beqw 664 <entry+0x200>
     4e2:	2079 0000 0000 	moveal 0 <main>,%a0
     4e8:	2f08           	movel %a0,%sp@-
     4ea:	42a7           	clrl %sp@-
     4ec:	266e 000c      	moveal %fp@(12),%a3
     4f0:	486b 0001      	pea %a3@(1)
     4f4:	2068 ff3c      	moveal %a0@(-196),%a0
     4f8:	4e90           	jsr %a0@
     4fa:	2d48 fffc      	movel %a0,%fp@(-4)
     4fe:	4fef 000c      	lea %sp@(12),%sp
     502:	6608           	bnes 50c <entry+0xa8>
     504:	42ae fff8      	clrl %fp@(-8)
     508:	6000 025a      	braw 764 <entry+0x300>
     50c:	101a           	moveb %a2@+,%d0
     50e:	206e fffc      	moveal %fp@(-4),%a0
     512:	6002           	bras 516 <entry+0xb2>
     514:	101a           	moveb %a2@+,%d0
     516:	10c0           	moveb %d0,%a0@+
     518:	66fa           	bnes 514 <entry+0xb0>
     51a:	7201           	moveq #1,%d1
     51c:	2d41 fff0      	movel %d1,%fp@(-16)
     520:	206e fffc      	moveal %fp@(-4),%a0
     524:	4a10           	tstb %a0@
     526:	677a           	beqs 5a2 <entry+0x13e>
     528:	1010           	moveb %a0@,%d0
     52a:	0c00 0020      	cmpib #32,%d0
     52e:	6724           	beqs 554 <entry+0xf0>
     530:	0c00 0009      	cmpib #9,%d0
     534:	671e           	beqs 554 <entry+0xf0>
     536:	0c00 000a      	cmpib #10,%d0
     53a:	661c           	bnes 558 <entry+0xf4>
     53c:	6016           	bras 554 <entry+0xf0>
     53e:	1010           	moveb %a0@,%d0
     540:	0c00 0020      	cmpib #32,%d0
     544:	670c           	beqs 552 <entry+0xee>
     546:	0c00 0009      	cmpib #9,%d0
     54a:	6706           	beqs 552 <entry+0xee>
     54c:	0c00 000a      	cmpib #10,%d0
     550:	6606           	bnes 558 <entry+0xf4>
     552:	5288           	addql #1,%a0
     554:	4a10           	tstb %a0@
     556:	66e6           	bnes 53e <entry+0xda>
     558:	1010           	moveb %a0@,%d0
     55a:	0c00 0022      	cmpib #34,%d0
     55e:	661e           	bnes 57e <entry+0x11a>
     560:	52ae fff0      	addql #1,%fp@(-16)
     564:	5288           	addql #1,%a0
     566:	4a10           	tstb %a0@
     568:	6738           	beqs 5a2 <entry+0x13e>
     56a:	0c10 0022      	cmpib #34,%a0@
     56e:	6706           	beqs 576 <entry+0x112>
     570:	5288           	addql #1,%a0
     572:	4a10           	tstb %a0@
     574:	66f4           	bnes 56a <entry+0x106>
     576:	4a10           	tstb %a0@
     578:	6728           	beqs 5a2 <entry+0x13e>
     57a:	5288           	addql #1,%a0
     57c:	60a6           	bras 524 <entry+0xc0>
     57e:	4a00           	tstb %d0
     580:	6720           	beqs 5a2 <entry+0x13e>
     582:	52ae fff0      	addql #1,%fp@(-16)
     586:	600c           	bras 594 <entry+0x130>
     588:	0c10 000a      	cmpib #10,%a0@
     58c:	6796           	beqs 524 <entry+0xc0>
     58e:	5288           	addql #1,%a0
     590:	1010           	moveb %a0@,%d0
     592:	670e           	beqs 5a2 <entry+0x13e>
     594:	0c00 0020      	cmpib #32,%d0
     598:	678a           	beqs 524 <entry+0xc0>
     59a:	0c00 0009      	cmpib #9,%d0
     59e:	66e8           	bnes 588 <entry+0x124>
     5a0:	6082           	bras 524 <entry+0xc0>
     5a2:	2079 0000 0000 	moveal 0 <main>,%a0
     5a8:	2f08           	movel %a0,%sp@-
     5aa:	2f3c 0001 0000 	movel #65536,%sp@-
     5b0:	202e fff0      	movel %fp@(-16),%d0
     5b4:	e588           	lsll #2,%d0
     5b6:	2f00           	movel %d0,%sp@-
     5b8:	2068 ff3c      	moveal %a0@(-196),%a0
     5bc:	4e90           	jsr %a0@
     5be:	2d48 fff8      	movel %a0,%fp@(-8)
     5c2:	4fef 000c      	lea %sp@(12),%sp
     5c6:	6700 019c      	beqw 764 <entry+0x300>
     5ca:	367c 0001      	moveaw #1,%a3
     5ce:	2d4b fff4      	movel %a3,%fp@(-12)
     5d2:	206e fffc      	moveal %fp@(-4),%a0
     5d6:	4a10           	tstb %a0@
     5d8:	6700 00b6      	beqw 690 <entry+0x22c>
     5dc:	226e fff8      	moveal %fp@(-8),%a1
     5e0:	5889           	addql #4,%a1
     5e2:	1010           	moveb %a0@,%d0
     5e4:	0c00 0020      	cmpib #32,%d0
     5e8:	6724           	beqs 60e <entry+0x1aa>
     5ea:	0c00 0009      	cmpib #9,%d0
     5ee:	671e           	beqs 60e <entry+0x1aa>
     5f0:	0c00 000a      	cmpib #10,%d0
     5f4:	661c           	bnes 612 <entry+0x1ae>
     5f6:	6016           	bras 60e <entry+0x1aa>
     5f8:	1010           	moveb %a0@,%d0
     5fa:	0c00 0020      	cmpib #32,%d0
     5fe:	670c           	beqs 60c <entry+0x1a8>
     600:	0c00 0009      	cmpib #9,%d0
     604:	6706           	beqs 60c <entry+0x1a8>
     606:	0c00 000a      	cmpib #10,%d0
     60a:	6606           	bnes 612 <entry+0x1ae>
     60c:	5288           	addql #1,%a0
     60e:	4a10           	tstb %a0@
     610:	66e6           	bnes 5f8 <entry+0x194>
     612:	1010           	moveb %a0@,%d0
     614:	0c00 0022      	cmpib #34,%d0
     618:	661a           	bnes 634 <entry+0x1d0>
     61a:	5288           	addql #1,%a0
     61c:	22c8           	movel %a0,%a1@+
     61e:	52ae fff4      	addql #1,%fp@(-12)
     622:	4a10           	tstb %a0@
     624:	676a           	beqs 690 <entry+0x22c>
     626:	0c10 0022      	cmpib #34,%a0@
     62a:	672c           	beqs 658 <entry+0x1f4>
     62c:	5288           	addql #1,%a0
     62e:	4a10           	tstb %a0@
     630:	66f4           	bnes 626 <entry+0x1c2>
     632:	6024           	bras 658 <entry+0x1f4>
     634:	4a00           	tstb %d0
     636:	6758           	beqs 690 <entry+0x22c>
     638:	22c8           	movel %a0,%a1@+
     63a:	52ae fff4      	addql #1,%fp@(-12)
     63e:	6008           	bras 648 <entry+0x1e4>
     640:	0c10 000a      	cmpib #10,%a0@
     644:	6712           	beqs 658 <entry+0x1f4>
     646:	5288           	addql #1,%a0
     648:	1010           	moveb %a0@,%d0
     64a:	6744           	beqs 690 <entry+0x22c>
     64c:	0c00 0020      	cmpib #32,%d0
     650:	6706           	beqs 658 <entry+0x1f4>
     652:	0c00 0009      	cmpib #9,%d0
     656:	66e8           	bnes 640 <entry+0x1dc>
     658:	4a10           	tstb %a0@
     65a:	6734           	beqs 690 <entry+0x22c>
     65c:	4218           	clrb %a0@+
     65e:	4a10           	tstb %a0@
     660:	6680           	bnes 5e2 <entry+0x17e>
     662:	602c           	bras 690 <entry+0x22c>
     664:	7201           	moveq #1,%d1
     666:	2d41 fff0      	movel %d1,%fp@(-16)
     66a:	2641           	moveal %d1,%a3
     66c:	2d4b fff4      	movel %a3,%fp@(-12)
     670:	2079 0000 0000 	moveal 0 <main>,%a0
     676:	2f08           	movel %a0,%sp@-
     678:	42a7           	clrl %sp@-
     67a:	4878 0004      	pea 4 <main+0x4>
     67e:	2068 ff3c      	moveal %a0@(-196),%a0
     682:	4e90           	jsr %a0@
     684:	2d48 fff8      	movel %a0,%fp@(-8)
     688:	4fef 000c      	lea %sp@(12),%sp
     68c:	6700 00d6      	beqw 764 <entry+0x300>
     690:	2079 0000 0000 	moveal 0 <main>,%a0
     696:	2f08           	movel %a0,%sp@-
     698:	42a7           	clrl %sp@-
     69a:	2f02           	movel %d2,%sp@-
     69c:	2068 fd56      	moveal %a0@(-682),%a0
     6a0:	4e90           	jsr %a0@
     6a2:	2008           	movel %a0,%d0
     6a4:	266e fff8      	moveal %fp@(-8),%a3
     6a8:	2680           	movel %d0,%a3@
     6aa:	4fef 000c      	lea %sp@(12),%sp
     6ae:	6700 00b4      	beqw 764 <entry+0x300>
     6b2:	2079 0000 0000 	moveal 0 <main>,%a0
     6b8:	2f08           	movel %a0,%sp@-
     6ba:	2f02           	movel %d2,%sp@-
     6bc:	2f00           	movel %d0,%sp@-
     6be:	2068 fdc2      	moveal %a0@(-574),%a0
     6c2:	4e90           	jsr %a0@
     6c4:	4fef 000c      	lea %sp@(12),%sp
     6c8:	4a40           	tstw %d0
     6ca:	662e           	bnes 6fa <entry+0x296>
     6cc:	2079 0000 0000 	moveal 0 <main>,%a0
     6d2:	2f08           	movel %a0,%sp@-
     6d4:	2068 ff7e      	moveal %a0@(-130),%a0
     6d8:	4e90           	jsr %a0@
     6da:	588f           	addql #4,%sp
     6dc:	7278           	moveq #120,%d1
     6de:	b280           	cmpl %d0,%d1
     6e0:	6600 0082      	bnew 764 <entry+0x300>
     6e4:	d482           	addl %d2,%d2
     6e6:	2079 0000 0000 	moveal 0 <main>,%a0
     6ec:	2f08           	movel %a0,%sp@-
     6ee:	2f13           	movel %a3@,%sp@-
     6f0:	2068 fd50      	moveal %a0@(-688),%a0
     6f4:	4e90           	jsr %a0@
     6f6:	508f           	addql #8,%sp
     6f8:	6002           	bras 6fc <entry+0x298>
     6fa:	7601           	moveq #1,%d3
     6fc:	4a83           	tstl %d3
     6fe:	6790           	beqs 690 <entry+0x22c>
     700:	603a           	bras 73c <entry+0x2d8>
     702:	2079 0000 0000 	moveal 0 <main>,%a0
     708:	2f08           	movel %a0,%sp@-
     70a:	45e9 005c      	lea %a1@(92),%a2
     70e:	2f0a           	movel %a2,%sp@-
     710:	2068 fe82      	moveal %a0@(-382),%a0
     714:	4e90           	jsr %a0@
     716:	2079 0000 0000 	moveal 0 <main>,%a0
     71c:	2f08           	movel %a0,%sp@-
     71e:	2f0a           	movel %a2,%sp@-
     720:	2068 fe8e      	moveal %a0@(-370),%a0
     724:	4e90           	jsr %a0@
     726:	23c8 0000 0000 	movel %a0,0 <main>
     72c:	2d48 fff8      	movel %a0,%fp@(-8)
     730:	42ae 000c      	clrl %fp@(12)
     734:	42ae fff4      	clrl %fp@(-12)
     738:	4fef 0010      	lea %sp@(16),%sp
     73c:	4879 0000 0000 	pea 0 <main>
     742:	61ff 0000 0000 	bsrl 744 <entry+0x2e0>
     748:	588f           	addql #4,%sp
     74a:	4a80           	tstl %d0
     74c:	6616           	bnes 764 <entry+0x300>
     74e:	2f2e fff8      	movel %fp@(-8),%sp@-
     752:	2f2e fff4      	movel %fp@(-12),%sp@-
     756:	61ff 0000 0000 	bsrl 758 <entry+0x2f4>
     75c:	23c0 0000 0000 	movel %d0,0 <main>
     762:	508f           	addql #8,%sp
     764:	4aae 000c      	tstl %fp@(12)
     768:	6762           	beqs 7cc <entry+0x368>
     76a:	4aae fff8      	tstl %fp@(-8)
     76e:	6738           	beqs 7a8 <entry+0x344>
     770:	266e fff8      	moveal %fp@(-8),%a3
     774:	2013           	movel %a3@,%d0
     776:	6712           	beqs 78a <entry+0x326>
     778:	2079 0000 0000 	moveal 0 <main>,%a0
     77e:	2f08           	movel %a0,%sp@-
     780:	2f00           	movel %d0,%sp@-
     782:	2068 fd50      	moveal %a0@(-688),%a0
     786:	4e90           	jsr %a0@
     788:	508f           	addql #8,%sp
     78a:	2079 0000 0000 	moveal 0 <main>,%a0
     790:	2f08           	movel %a0,%sp@-
     792:	202e fff0      	movel %fp@(-16),%d0
     796:	e588           	lsll #2,%d0
     798:	2f00           	movel %d0,%sp@-
     79a:	2f2e fff8      	movel %fp@(-8),%sp@-
     79e:	2068 ff30      	moveal %a0@(-208),%a0
     7a2:	4e90           	jsr %a0@
     7a4:	4fef 000c      	lea %sp@(12),%sp
     7a8:	4aae fffc      	tstl %fp@(-4)
     7ac:	671e           	beqs 7cc <entry+0x368>
     7ae:	2079 0000 0000 	moveal 0 <main>,%a0
     7b4:	2f08           	movel %a0,%sp@-
     7b6:	266e 000c      	moveal %fp@(12),%a3
     7ba:	486b 0001      	pea %a3@(1)
     7be:	2f2e fffc      	movel %fp@(-4),%sp@-
     7c2:	2068 ff30      	moveal %a0@(-208),%a0
     7c6:	4e90           	jsr %a0@
     7c8:	4fef 000c      	lea %sp@(12),%sp
     7cc:	4878 ffff      	pea ffffffff <__errno_location+0xffffa56b>
     7d0:	4879 0000 0000 	pea 0 <main>
     7d6:	6152           	bsrs 82a <call_funcs>
     7d8:	2079 0000 0000 	moveal 0 <main>,%a0
     7de:	2f08           	movel %a0,%sp@-
     7e0:	2f39 0000 0000 	movel 0 <main>,%sp@-
     7e6:	2068 fe64      	moveal %a0@(-412),%a0
     7ea:	4e90           	jsr %a0@
     7ec:	4fef 0010      	lea %sp@(16),%sp
     7f0:	4ab9 0000 0000 	tstl 0 <main>
     7f6:	6722           	beqs 81a <entry+0x3b6>
     7f8:	2079 0000 0000 	moveal 0 <main>,%a0
     7fe:	2f08           	movel %a0,%sp@-
     800:	2068 ff7e      	moveal %a0@(-130),%a0
     804:	4e90           	jsr %a0@
     806:	2079 0000 0000 	moveal 0 <main>,%a0
     80c:	2f08           	movel %a0,%sp@-
     80e:	2f39 0000 0000 	movel 0 <main>,%sp@-
     814:	2068 fe88      	moveal %a0@(-376),%a0
     818:	4e90           	jsr %a0@
     81a:	2039 0000 0000 	movel 0 <main>,%d0
     820:	4cee 0c0c ffe0 	moveml %fp@(-32),%d2-%d3/%a2-%a3
     826:	4e5e           	unlk %fp
     828:	4e75           	rts

0000082a <call_funcs>:
     82a:	4e56 0000      	linkw %fp,#0
     82e:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
     832:	266e 0008      	moveal %fp@(8),%a3
     836:	4aae 000c      	tstl %fp@(12)
     83a:	6d1e           	blts 85a <call_funcs+0x30>
     83c:	4aab 0004      	tstl %a3@(4)
     840:	6730           	beqs 872 <call_funcs+0x48>
     842:	45eb 0004      	lea %a3@(4),%a2
     846:	7404           	moveq #4,%d2
     848:	2002           	movel %d2,%d0
     84a:	5882           	addql #4,%d2
     84c:	2073 0800      	moveal %a3@(00000000,%d0:l),%a0
     850:	4e90           	jsr %a0@
     852:	588a           	addql #4,%a2
     854:	4a92           	tstl %a2@
     856:	66f0           	bnes 848 <call_funcs+0x1e>
     858:	6018           	bras 872 <call_funcs+0x48>
     85a:	2413           	movel %a3@,%d2
     85c:	6714           	beqs 872 <call_funcs+0x48>
     85e:	2602           	movel %d2,%d3
     860:	e58b           	lsll #2,%d3
     862:	2003           	movel %d3,%d0
     864:	5983           	subql #4,%d3
     866:	5382           	subql #1,%d2
     868:	2073 0800      	moveal %a3@(00000000,%d0:l),%a0
     86c:	4e90           	jsr %a0@
     86e:	4a82           	tstl %d2
     870:	66f0           	bnes 862 <call_funcs+0x38>
     872:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
     878:	4e5e           	unlk %fp
     87a:	4e75           	rts

0000087c <__main>:
     87c:	4e56 0000      	linkw %fp,#0
     880:	4e5e           	unlk %fp
     882:	4e75           	rts

00000884 <main_init>:
     884:	4e56 0000      	linkw %fp,#0
     888:	48e7 0038      	moveml %a2-%a4,%sp@-
     88c:	246e 0008      	moveal %fp@(8),%a2
     890:	266e 000c      	moveal %fp@(12),%a3
     894:	61ff 0000 0000 	bsrl 896 <main_init+0x12>
     89a:	61ff 0000 0000 	bsrl 89c <main_init+0x18>
     8a0:	23ca 0000 0000 	movel %a2,0 <main>
     8a6:	157c 000a 000c 	moveb #10,%a2@(12)
     8ac:	2079 0000 0000 	moveal 0 <main>,%a0
     8b2:	217c 0000 0000 	movel #0,%a0@(8)
     8b8:	0008 
     8ba:	117c fffb 000d 	moveb #-5,%a0@(13)
     8c0:	2079 0000 0000 	moveal 0 <main>,%a0
     8c6:	317c 0703 000e 	movew #1795,%a0@(14)
     8cc:	43e8 0020      	lea %a0@(32),%a1
     8d0:	2149 0010      	movel %a1,%a0@(16)
     8d4:	4291           	clrl %a1@
     8d6:	2268 0010      	moveal %a0@(16),%a1
     8da:	49eb ffe0      	lea %a3@(-32),%a4
     8de:	234c 0004      	movel %a4,%a1@(4)
     8e2:	2168 0010 0014 	movel %a0@(16),%a0@(20)
     8e8:	d5cb           	addal %a3,%a2
     8ea:	214a 0018      	movel %a2,%a0@(24)
     8ee:	2268 0010      	moveal %a0@(16),%a1
     8f2:	2169 0004 001c 	movel %a1@(4),%a0@(28)
     8f8:	2f08           	movel %a0,%sp@-
     8fa:	61ff 0000 0000 	bsrl 8fc <main_init+0x78>
     900:	23c8 0000 0000 	movel %a0,0 <main>
     906:	21c8 0004      	movel %a0,4 <main+0x4>
     90a:	2008           	movel %a0,%d0
     90c:	4680           	notl %d0
     90e:	2140 0026      	movel %d0,%a0@(38)
     912:	2f08           	movel %a0,%sp@-
     914:	4878 0001      	pea 1 <main+0x1>
     918:	4878 1000      	pea 1000 <Dispatcher+0x18>
     91c:	2068 ff3c      	moveal %a0@(-196),%a0
     920:	4e90           	jsr %a0@
     922:	21c8 1004      	movel %a0,1004 <Dispatcher+0x1c>
     926:	4fef 0010      	lea %sp@(16),%sp
     92a:	6602           	bnes 92e <main_init+0xaa>
     92c:	60fe           	bras 92c <main_init+0xa8>
     92e:	06b8 0000 1000 	addil #4096,1004 <Dispatcher+0x1c>
     934:	1004 
     936:	2079 0000 0000 	moveal 0 <main>,%a0
     93c:	217c 0000 0000 	movel #0,%a0@(300)
     942:	012c 
     944:	4879 0000 0000 	pea 0 <main>
     94a:	61ff 0000 0000 	bsrl 94c <main_init+0xc8>
     950:	4cee 1c00 fff4 	moveml %fp@(-12),%a2-%a4
     956:	4e5e           	unlk %fp
     958:	4e75           	rts

0000095a <main_init_cont>:
     95a:	4e56 0000      	linkw %fp,#0
     95e:	2f0a           	movel %a2,%sp@-
     960:	2f02           	movel %d2,%sp@-
     962:	2478 0004      	moveal 4 <main+0x4>,%a2
     966:	2f0a           	movel %a2,%sp@-
     968:	42a7           	clrl %sp@-
     96a:	4878 0002      	pea 2 <main+0x2>
     96e:	206a ffba      	moveal %a2@(-70),%a0
     972:	4e90           	jsr %a0@
     974:	2f0a           	movel %a2,%sp@-
     976:	4879 0000 0000 	pea 0 <main>
     97c:	206a fedc      	moveal %a2@(-292),%a0
     980:	4e90           	jsr %a0@
     982:	2008           	movel %a0,%d0
     984:	4fef 0014      	lea %sp@(20),%sp
     988:	6710           	beqs 99a <main_init_cont+0x40>
     98a:	2f0a           	movel %a2,%sp@-
     98c:	42a7           	clrl %sp@-
     98e:	2f00           	movel %d0,%sp@-
     990:	206a fed6      	moveal %a2@(-298),%a0
     994:	4e90           	jsr %a0@
     996:	4fef 000c      	lea %sp@(12),%sp
     99a:	7400           	moveq #0,%d2
     99c:	42a7           	clrl %sp@-
     99e:	61ff 0000 0000 	bsrl 9a0 <main_init_cont+0x46>
     9a4:	2f02           	movel %d2,%sp@-
     9a6:	61ff 0000 0000 	bsrl 9a8 <main_init_cont+0x4e>
     9ac:	2f02           	movel %d2,%sp@-
     9ae:	61ff 0000 0000 	bsrl 9b0 <main_init_cont+0x56>
     9b4:	7000           	moveq #0,%d0
     9b6:	4fef 000c      	lea %sp@(12),%sp
     9ba:	5280           	addql #1,%d0
     9bc:	0c80 0000 3fff 	cmpil #16383,%d0
     9c2:	63f6           	blss 9ba <main_init_cont+0x60>
     9c4:	5282           	addql #1,%d2
     9c6:	0c82 0000 009f 	cmpil #159,%d2
     9cc:	63ce           	blss 99c <main_init_cont+0x42>
     9ce:	60ca           	bras 99a <main_init_cont+0x40>
     9d0:	4e71           	nop
     9d2:	4e75           	rts

000009d4 <Init_PalmHardware>:
     9d4:	4e56 0000      	linkw %fp,#0
     9d8:	70ff           	moveq #-1,%d0
     9da:	21c0 f304      	movel %d0,fffff304 <__errno_location+0xffff9870>
     9de:	307c fb0c      	moveaw #-1268,%a0
     9e2:	4250           	clrw %a0@
     9e4:	31c0 fb0e      	movew %d0,fffffb0e <__errno_location+0xffffa07a>
     9e8:	31fc 8010 fb10 	movew #-32752,fffffb10 <__errno_location+0xffffa07c>
     9ee:	30bc 00a0      	movew #160,%a0@
     9f2:	42b8 f30c      	clrl fffff30c <__errno_location+0xffff9878>
     9f6:	4238 f000      	clrb fffff000 <__errno_location+0xffff956c>
     9fa:	4e5e           	unlk %fp
     9fc:	4e75           	rts

000009fe <Init_IRQVectors>:
     9fe:	4e56 0000      	linkw %fp,#0
     a02:	21fc 0000 0000 	movel #0,70 <drawlinehoriz+0xa>
     a08:	0070 
     a0a:	21f8 0070 0080 	movel 70 <drawlinehoriz+0xa>,80 <drawlinehoriz+0x1a>
     a10:	4e5e           	unlk %fp
     a12:	4e75           	rts
     a14:	4e75           	rts
     a16:	4e75           	rts
     a18:	4e75           	rts
     a1a:	4e75           	rts
     a1c:	4e75           	rts
     a1e:	4e75           	rts

00000a20 <_sys_dispatch>:
     a20:	21fc ffff ffff 	movel #-1,fffff304 <__errno_location+0xffff9870>
     a26:	f304 
     a28:	48e7 fffe      	moveml %d0-%fp,%sp@-
     a2c:	4e68           	movel %usp,%a0
     a2e:	48e7 0080      	moveml %a0,%sp@-
     a32:	204f           	moveal %sp,%a0
     a34:	2f08           	movel %a0,%sp@-
     a36:	6100 0000      	bsrw a38 <_sys_dispatch+0x18>
     a3a:	588f           	addql #4,%sp
     a3c:	4cdf 0100      	moveml %sp@+,%a0
     a40:	4e60           	movel %a0,%usp
     a42:	4cdf 7fff      	moveml %sp@+,%d0-%fp
     a46:	31fc 8010 fb0e 	movew #-32752,fffffb0e <__errno_location+0xffffa07a>
     a4c:	21fc ffbf ffef 	movel #-4194321,fffff304 <__errno_location+0xffff9870>
     a52:	f304 
     a54:	4e73           	rte
     a56:	4e75           	rts
     a58:	4e75           	rts
     a5a:	4e75           	rts
     a5c:	4e75           	rts
     a5e:	4e75           	rts

00000a60 <switch_to_user_mode>:
     a60:	2078 1004      	moveal 1004 <Dispatcher+0x1c>,%a0
     a64:	4e60           	movel %a0,%usp
     a66:	224f           	moveal %sp,%a1
     a68:	41fa 000a      	lea %pc@(a74 <in_user_mode>),%a0
     a6c:	2f08           	movel %a0,%sp@-
     a6e:	3f3c 0000      	movew #0,%sp@-
     a72:	4e73           	rte

00000a74 <in_user_mode>:
     a74:	2069 0004      	moveal %a1@(4),%a0
     a78:	4ed0           	jmp %a0@
     a7a:	4e75           	rts

00000a7c <SaveRegs>:
     a7c:	4e56 0000      	linkw %fp,#0
     a80:	2f0b           	movel %a3,%sp@-
     a82:	2f0a           	movel %a2,%sp@-
     a84:	266e 0008      	moveal %fp@(8),%a3
     a88:	226e 000c      	moveal %fp@(12),%a1
     a8c:	2451           	moveal %a1@,%a2
     a8e:	41ea ffba      	lea %a2@(-70),%a0
     a92:	208a           	movel %a2,%a0@
     a94:	2169 0004 0004 	movel %a1@(4),%a0@(4)
     a9a:	2169 0008 0008 	movel %a1@(8),%a0@(8)
     aa0:	2169 000c 000c 	movel %a1@(12),%a0@(12)
     aa6:	2169 0010 0010 	movel %a1@(16),%a0@(16)
     aac:	2169 0014 0014 	movel %a1@(20),%a0@(20)
     ab2:	2169 0018 0018 	movel %a1@(24),%a0@(24)
     ab8:	2169 001c 001c 	movel %a1@(28),%a0@(28)
     abe:	2169 0020 0020 	movel %a1@(32),%a0@(32)
     ac4:	2169 0024 0024 	movel %a1@(36),%a0@(36)
     aca:	2169 0028 0028 	movel %a1@(40),%a0@(40)
     ad0:	2169 002c 002c 	movel %a1@(44),%a0@(44)
     ad6:	2169 0030 0030 	movel %a1@(48),%a0@(48)
     adc:	2169 0034 0034 	movel %a1@(52),%a0@(52)
     ae2:	2169 0038 0038 	movel %a1@(56),%a0@(56)
     ae8:	2169 003c 003c 	movel %a1@(60),%a0@(60)
     aee:	3169 0040 0040 	movew %a1@(64),%a0@(64)
     af4:	2169 0042 0042 	movel %a1@(66),%a0@(66)
     afa:	2748 0036      	movel %a0,%a3@(54)
     afe:	245f           	moveal %sp@+,%a2
     b00:	265f           	moveal %sp@+,%a3
     b02:	4e5e           	unlk %fp
     b04:	4e75           	rts

00000b06 <RestoreRegs>:
     b06:	4e56 0000      	linkw %fp,#0
     b0a:	2f0a           	movel %a2,%sp@-
     b0c:	246e 0008      	moveal %fp@(8),%a2
     b10:	226e 000c      	moveal %fp@(12),%a1
     b14:	206a 0036      	moveal %a2@(54),%a0
     b18:	2290           	movel %a0@,%a1@
     b1a:	2368 0004 0004 	movel %a0@(4),%a1@(4)
     b20:	2368 0008 0008 	movel %a0@(8),%a1@(8)
     b26:	2368 000c 000c 	movel %a0@(12),%a1@(12)
     b2c:	2368 0010 0010 	movel %a0@(16),%a1@(16)
     b32:	2368 0014 0014 	movel %a0@(20),%a1@(20)
     b38:	2368 0018 0018 	movel %a0@(24),%a1@(24)
     b3e:	2368 001c 001c 	movel %a0@(28),%a1@(28)
     b44:	2368 0020 0020 	movel %a0@(32),%a1@(32)
     b4a:	2368 0024 0024 	movel %a0@(36),%a1@(36)
     b50:	2368 0028 0028 	movel %a0@(40),%a1@(40)
     b56:	2368 002c 002c 	movel %a0@(44),%a1@(44)
     b5c:	2368 0030 0030 	movel %a0@(48),%a1@(48)
     b62:	2368 0034 0034 	movel %a0@(52),%a1@(52)
     b68:	2368 0038 0038 	movel %a0@(56),%a1@(56)
     b6e:	2368 003c 003c 	movel %a0@(60),%a1@(60)
     b74:	3368 0040 0040 	movew %a0@(64),%a1@(64)
     b7a:	2368 0042 0042 	movel %a0@(66),%a1@(66)
     b80:	41e8 0046      	lea %a0@(70),%a0
     b84:	2548 0036      	movel %a0,%a2@(54)
     b88:	245f           	moveal %sp@+,%a2
     b8a:	4e5e           	unlk %fp
     b8c:	4e75           	rts

00000b8e <sys_Dispatch>:
     b8e:	4e56 0000      	linkw %fp,#0
     b92:	2f0b           	movel %a3,%sp@-
     b94:	2f0a           	movel %a2,%sp@-
     b96:	266e 0008      	moveal %fp@(8),%a3
     b9a:	2478 0004      	moveal 4 <main+0x4>,%a2
     b9e:	082b 0005 0040 	btst #5,%a3@(64)
     ba4:	6600 00bc      	bnew c62 <sys_Dispatch+0xd4>
     ba8:	4a2a 0127      	tstb %a2@(295)
     bac:	6d04           	blts bb2 <sys_Dispatch+0x24>
     bae:	50ea 0127      	st %a2@(295)
     bb2:	206a 0196      	moveal %a2@(406),%a0
     bb6:	4a90           	tstl %a0@
     bb8:	6744           	beqs bfe <sys_Dispatch+0x70>
     bba:	4a2a 0127      	tstb %a2@(295)
     bbe:	6c38           	bges bf8 <sys_Dispatch+0x6a>
     bc0:	206a 0114      	moveal %a2@(276),%a0
     bc4:	1028 000f      	moveb %a0@(15),%d0
     bc8:	0c00 0002      	cmpib #2,%d0
     bcc:	661c           	bnes bea <sys_Dispatch+0x5c>
     bce:	117c 0003 000f 	moveb #3,%a0@(15)
     bd4:	2f0a           	movel %a2,%sp@-
     bd6:	2f2a 0114      	movel %a2@(276),%sp@-
     bda:	206a ffd2      	moveal %a2@(-46),%a0
     bde:	4e90           	jsr %a0@
     be0:	006a 8000 012a 	oriw #-32768,%a2@(298)
     be6:	508f           	addql #8,%sp
     be8:	6014           	bras bfe <sys_Dispatch+0x70>
     bea:	0c00 0006      	cmpib #6,%d0
     bee:	660e           	bnes bfe <sys_Dispatch+0x70>
     bf0:	006a 8000 012a 	oriw #-32768,%a2@(298)
     bf6:	6006           	bras bfe <sys_Dispatch+0x70>
     bf8:	006a 0080 012a 	oriw #128,%a2@(298)
     bfe:	302a 012a      	movew %a2@(298),%d0
     c02:	6c5e           	bges c62 <sys_Dispatch+0xd4>
     c04:	0240 7fff      	andiw #32767,%d0
     c08:	3540 012a      	movew %d0,%a2@(298)
     c0c:	206a 0114      	moveal %a2@(276),%a0
     c10:	b0fc 0000      	cmpaw #0,%a0
     c14:	6712           	beqs c28 <sys_Dispatch+0x9a>
     c16:	0c28 0006 000f 	cmpib #6,%a0@(15)
     c1c:	670a           	beqs c28 <sys_Dispatch+0x9a>
     c1e:	2f0b           	movel %a3,%sp@-
     c20:	2f08           	movel %a0,%sp@-
     c22:	6100 fe58      	bsrw a7c <SaveRegs>
     c26:	508f           	addql #8,%sp
     c28:	2f0a           	movel %a2,%sp@-
     c2a:	206a ffc6      	moveal %a2@(-58),%a0
     c2e:	4e90           	jsr %a0@
     c30:	2f0b           	movel %a3,%sp@-
     c32:	2f2a 0114      	movel %a2@(276),%sp@-
     c36:	6100 fece      	bsrw b06 <RestoreRegs>
     c3a:	206a 0114      	moveal %a2@(276),%a0
     c3e:	4fef 000c      	lea %sp@(12),%sp
     c42:	0828 0005 000e 	btst #5,%a0@(14)
     c48:	6718           	beqs c62 <sys_Dispatch+0xd4>
     c4a:	2f0a           	movel %a2,%sp@-
     c4c:	206a ff8a      	moveal %a2@(-118),%a0
     c50:	4e90           	jsr %a0@
     c52:	2f0a           	movel %a2,%sp@-
     c54:	206a ffc0      	moveal %a2@(-64),%a0
     c58:	4e90           	jsr %a0@
     c5a:	2f0a           	movel %a2,%sp@-
     c5c:	206a ff84      	moveal %a2@(-124),%a0
     c60:	4e90           	jsr %a0@
     c62:	246e fff8      	moveal %fp@(-8),%a2
     c66:	266e fffc      	moveal %fp@(-4),%a3
     c6a:	4e5e           	unlk %fp
     c6c:	4e75           	rts
     c6e:	4e75           	rts

00000c70 <allocmem>:
     c70:	4e56 0000      	linkw %fp,#0
     c74:	2f0a           	movel %a2,%sp@-
     c76:	246e 0008      	moveal %fp@(8),%a2
     c7a:	202e 000c      	movel %fp@(12),%d0
     c7e:	5e80           	addql #7,%d0
     c80:	72f8           	moveq #-8,%d1
     c82:	c081           	andl %d1,%d0
     c84:	206a 0010      	moveal %a2@(16),%a0
     c88:	43f0 0800      	lea %a0@(00000000,%d0:l),%a1
     c8c:	2549 0010      	movel %a1,%a2@(16)
     c90:	4291           	clrl %a1@
     c92:	226a 0010      	moveal %a2@(16),%a1
     c96:	2228 0004      	movel %a0@(4),%d1
     c9a:	9280           	subl %d0,%d1
     c9c:	2001           	movel %d1,%d0
     c9e:	2340 0004      	movel %d0,%a1@(4)
     ca2:	2540 001c      	movel %d0,%a2@(28)
     ca6:	2008           	movel %a0,%d0
     ca8:	245f           	moveal %sp@+,%a2
     caa:	4e5e           	unlk %fp
     cac:	4e75           	rts

00000cae <PrepareExecBase>:
     cae:	4e56 0000      	linkw %fp,#0
     cb2:	48e7 303c      	moveml %d2-%d3/%a2-%a5,%sp@-
     cb6:	262e 0008      	movel %fp@(8),%d3
     cba:	387c 0330      	moveaw #816,%a4
     cbe:	4878 05ac      	pea 5ac <entry+0x148>
     cc2:	2f03           	movel %d3,%sp@-
     cc4:	61aa           	bsrs c70 <allocmem>
     cc6:	d1cc           	addal %a4,%a0
     cc8:	23c8 0000 0000 	movel %a0,0 <main>
     cce:	4878 027c      	pea 27c <clearpixel+0x22>
     cd2:	42a7           	clrl %sp@-
     cd4:	2f08           	movel %a0,%sp@-
     cd6:	61ff 0000 0000 	bsrl cd8 <PrepareExecBase+0x2a>
     cdc:	7001           	moveq #1,%d0
     cde:	4fef 0014      	lea %sp@(20),%sp
     ce2:	47f9 0000 0000 	lea 0 <main>,%a3
     ce8:	93c9           	subal %a1,%a1
     cea:	7206           	moveq #6,%d1
     cec:	2479 0000 0000 	moveal 0 <main>,%a2
     cf2:	204a           	moveal %a2,%a0
     cf4:	91c1           	subal %d1,%a0
     cf6:	30bc 4ef9      	movew #20217,%a0@
     cfa:	217c c0ed babe 	movel #-1058161986,%a0@(2)
     d00:	0002 
     d02:	2171 b800 0002 	movel %a1@(00000000,%a3:l),%a0@(2)
     d08:	5889           	addql #4,%a1
     d0a:	5c81           	addql #6,%d1
     d0c:	5280           	addql #1,%d0
     d0e:	0c80 0000 0087 	cmpil #135,%d0
     d14:	63d6           	blss cec <PrepareExecBase+0x3e>
     d16:	2f0a           	movel %a2,%sp@-
     d18:	61ff 0000 0000 	bsrl d1a <PrepareExecBase+0x6c>
     d1e:	2079 0000 0000 	moveal 0 <main>,%a0
     d24:	117c 0009 000c 	moveb #9,%a0@(12)
     d2a:	2079 0000 0000 	moveal 0 <main>,%a0
     d30:	117c ff9c 000d 	moveb #-100,%a0@(13)
     d36:	2079 0000 0000 	moveal 0 <main>,%a0
     d3c:	217c 0000 0000 	movel #0,%a0@(8)
     d42:	0008 
     d44:	2179 0000 0000 	movel 0 <main>,%a0@(24)
     d4a:	0018 
     d4c:	317c 0029 0014 	movew #41,%a0@(20)
     d52:	317c 000b 0016 	movew #11,%a0@(22)
     d58:	317c 0001 0020 	movew #1,%a0@(32)
     d5e:	314c 0010      	movew %a4,%a0@(16)
     d62:	317c 027c 0012 	movew #636,%a0@(18)
     d68:	4228 000e      	clrb %a0@(14)
     d6c:	2079 0000 0000 	moveal 0 <main>,%a0
     d72:	43e8 0142      	lea %a0@(322),%a1
     d76:	2149 014a      	movel %a1,%a0@(330)
     d7a:	42a8 0146      	clrl %a0@(326)
     d7e:	4be8 0146      	lea %a0@(326),%a5
     d82:	228d           	movel %a5,%a1@
     d84:	117c 000a 014e 	moveb #10,%a0@(334)
     d8a:	2079 0000 0000 	moveal 0 <main>,%a0
     d90:	2a43           	moveal %d3,%a5
     d92:	2aa8 0142      	movel %a0@(322),%a5@
     d96:	43e8 0142      	lea %a0@(322),%a1
     d9a:	2b49 0004      	movel %a1,%a5@(4)
     d9e:	2451           	moveal %a1@,%a2
     da0:	2543 0004      	movel %d3,%a2@(4)
     da4:	2283           	movel %d3,%a1@
     da6:	43e8 0150      	lea %a0@(336),%a1
     daa:	2149 0158      	movel %a1,%a0@(344)
     dae:	42a8 0154      	clrl %a0@(340)
     db2:	4be8 0154      	lea %a0@(340),%a5
     db6:	228d           	movel %a5,%a1@
     db8:	117c 0008 015c 	moveb #8,%a0@(348)
     dbe:	2079 0000 0000 	moveal 0 <main>,%a0
     dc4:	43e8 015e      	lea %a0@(350),%a1
     dc8:	2149 0166      	movel %a1,%a0@(358)
     dcc:	42a8 0162      	clrl %a0@(354)
     dd0:	4be8 0162      	lea %a0@(354),%a5
     dd4:	228d           	movel %a5,%a1@
     dd6:	117c 0003 016a 	moveb #3,%a0@(362)
     ddc:	2079 0000 0000 	moveal 0 <main>,%a0
     de2:	43e8 017a      	lea %a0@(378),%a1
     de6:	2149 0182      	movel %a1,%a0@(386)
     dea:	42a8 017e      	clrl %a0@(382)
     dee:	4be8 017e      	lea %a0@(382),%a5
     df2:	228d           	movel %a5,%a1@
     df4:	117c 0009 0186 	moveb #9,%a0@(390)
     dfa:	2079 0000 0000 	moveal 0 <main>,%a0
     e00:	20a8 017a      	movel %a0@(378),%a0@
     e04:	43e8 017a      	lea %a0@(378),%a1
     e08:	2149 0004      	movel %a1,%a0@(4)
     e0c:	2451           	moveal %a1@,%a2
     e0e:	2548 0004      	movel %a0,%a2@(4)
     e12:	2288           	movel %a0,%a1@
     e14:	43e8 0188      	lea %a0@(392),%a1
     e18:	2149 0190      	movel %a1,%a0@(400)
     e1c:	42a8 018c      	clrl %a0@(396)
     e20:	4be8 018c      	lea %a0@(396),%a5
     e24:	228d           	movel %a5,%a1@
     e26:	117c 0004 0194 	moveb #4,%a0@(404)
     e2c:	2079 0000 0000 	moveal 0 <main>,%a0
     e32:	43e8 0196      	lea %a0@(406),%a1
     e36:	2149 019e      	movel %a1,%a0@(414)
     e3a:	42a8 019a      	clrl %a0@(410)
     e3e:	4be8 019a      	lea %a0@(410),%a5
     e42:	228d           	movel %a5,%a1@
     e44:	117c 0001 01a2 	moveb #1,%a0@(418)
     e4a:	2079 0000 0000 	moveal 0 <main>,%a0
     e50:	43e8 01a4      	lea %a0@(420),%a1
     e54:	2149 01ac      	movel %a1,%a0@(428)
     e58:	42a8 01a8      	clrl %a0@(424)
     e5c:	4be8 01a8      	lea %a0@(424),%a5
     e60:	228d           	movel %a5,%a1@
     e62:	117c 0001 01b0 	moveb #1,%a0@(432)
     e68:	2079 0000 0000 	moveal 0 <main>,%a0
     e6e:	43e8 0214      	lea %a0@(532),%a1
     e72:	2149 021c      	movel %a1,%a0@(540)
     e76:	42a8 0218      	clrl %a0@(536)
     e7a:	4be8 0218      	lea %a0@(536),%a5
     e7e:	228d           	movel %a5,%a1@
     e80:	117c 000e 01b0 	moveb #14,%a0@(432)
     e86:	2079 0000 0000 	moveal 0 <main>,%a0
     e8c:	43e8 0268      	lea %a0@(616),%a1
     e90:	2149 0270      	movel %a1,%a0@(624)
     e94:	42a8 026c      	clrl %a0@(620)
     e98:	41e8 026c      	lea %a0@(620),%a0
     e9c:	2288           	movel %a0,%a1@
     e9e:	7000           	moveq #0,%d0
     ea0:	588f           	addql #4,%sp
     ea2:	387c 01b2      	moveaw #434,%a4
     ea6:	2640           	moveal %d0,%a3
     ea8:	2079 0000 0000 	moveal 0 <main>,%a0
     eae:	45f3 8800      	lea %a3@(00000000,%a0:l),%a2
     eb2:	43ea 01b2      	lea %a2@(434),%a1
     eb6:	d1cc           	addal %a4,%a0
     eb8:	2348 0008      	movel %a0,%a1@(8)
     ebc:	42a9 0004      	clrl %a1@(4)
     ec0:	5888           	addql #4,%a0
     ec2:	2288           	movel %a0,%a1@
     ec4:	157c 0002 01be 	moveb #2,%a2@(446)
     eca:	49ec 0010      	lea %a4@(16),%a4
     ece:	47eb 0010      	lea %a3@(16),%a3
     ed2:	5280           	addql #1,%d0
     ed4:	7404           	moveq #4,%d2
     ed6:	b480           	cmpl %d0,%d2
     ed8:	64ce           	bccs ea8 <PrepareExecBase+0x1fa>
     eda:	2079 0000 0000 	moveal 0 <main>,%a0
     ee0:	317c 0029 0022 	movew #41,%a0@(34)
     ee6:	42a8 0032      	clrl %a0@(50)
     eea:	42a8 002e      	clrl %a0@(46)
     eee:	42a8 002a      	clrl %a0@(42)
     ef2:	74ff           	moveq #-1,%d2
     ef4:	2142 0036      	movel %d2,%a0@(54)
     ef8:	42a8 003a      	clrl %a0@(58)
     efc:	2a43           	moveal %d3,%a5
     efe:	216d 0018 003e 	movel %a5@(24),%a0@(62)
     f04:	317c 0004 0120 	movew #4,%a0@(288)
     f0a:	217c 0000 0000 	movel #0,%a0@(304)
     f10:	0130 
     f12:	42a8 0134      	clrl %a0@(308)
     f16:	217c 0000 0000 	movel #0,%a0@(312)
     f1c:	0138 
     f1e:	217c 0000 ffff 	movel #65535,%a0@(316)
     f24:	013c 
     f26:	4268 0140      	clrw %a0@(320)
     f2a:	61ff 0000 0000 	bsrl f2c <PrepareExecBase+0x27e>
     f30:	2279 0000 0000 	moveal 0 <main>,%a1
     f36:	2348 0278      	movel %a0,%a1@(632)
     f3a:	2049           	moveal %a1,%a0
     f3c:	2008           	movel %a0,%d0
     f3e:	4cee 3c0c ffe8 	moveml %fp@(-24),%d2-%d3/%a2-%a5
     f44:	4e5e           	unlk %fp
     f46:	4e75           	rts

00000f48 <PrepareAROSSupportBase>:
     f48:	4e56 0000      	linkw %fp,#0
     f4c:	41f9 0000 0000 	lea 0 <main>,%a0
     f52:	20bc 0000 0000 	movel #0,%a0@
     f58:	23fc 0000 0000 	movel #0,0 <main>
     f5e:	0000 0000 
     f62:	5988           	subql #4,%a0
     f64:	2008           	movel %a0,%d0
     f66:	4e5e           	unlk %fp
     f68:	4e75           	rts

00000f6a <AROSSupportBase_SetStdOut>:
     f6a:	4e56 0000      	linkw %fp,#0
     f6e:	23ee 0008 0000 	movel %fp@(8),0 <main>
     f74:	0000 
     f76:	4e5e           	unlk %fp
     f78:	4e75           	rts

00000f7a <_aros_not_implemented>:
     f7a:	4e56 0000      	linkw %fp,#0
     f7e:	2056           	moveal %fp@,%a0
     f80:	2f28 0008      	movel %a0@(8),%sp@-
     f84:	206e 0004      	moveal %fp@(4),%a0
     f88:	3028 fffe      	movew %a0@(-2),%d0
     f8c:	48c0           	extl %d0
     f8e:	6c02           	bges f92 <_aros_not_implemented+0x18>
     f90:	4480           	negl %d0
     f92:	2f00           	movel %d0,%sp@-
     f94:	4879 0000 0000 	pea 0 <main>
     f9a:	61ff 0000 0000 	bsrl f9c <_aros_not_implemented+0x22>
     fa0:	4e5e           	unlk %fp
     fa2:	4e75           	rts

00000fa4 <IntServer>:
     fa4:	4e56 0000      	linkw %fp,#0
     fa8:	48e7 3020      	moveml %d2-%d3/%a2,%sp@-
     fac:	262e 000c      	movel %fp@(12),%d3
     fb0:	206e 0010      	moveal %fp@(16),%a0
     fb4:	242e 0018      	movel %fp@(24),%d2
     fb8:	2450           	moveal %a0@,%a2
     fba:	4a92           	tstl %a2@
     fbc:	6720           	beqs fde <IntServer+0x3a>
     fbe:	2f02           	movel %d2,%sp@-
     fc0:	2f2a 0012      	movel %a2@(18),%sp@-
     fc4:	2f2a 000e      	movel %a2@(14),%sp@-
     fc8:	2f03           	movel %d3,%sp@-
     fca:	206a 0012      	moveal %a2@(18),%a0
     fce:	4e90           	jsr %a0@
     fd0:	4fef 0010      	lea %sp@(16),%sp
     fd4:	4a80           	tstl %d0
     fd6:	6606           	bnes fde <IntServer+0x3a>
     fd8:	2452           	moveal %a2@,%a2
     fda:	4a92           	tstl %a2@
     fdc:	66e0           	bnes fbe <IntServer+0x1a>
     fde:	4cee 040c fff4 	moveml %fp@(-12),%d2-%d3/%a2
     fe4:	4e5e           	unlk %fp
     fe6:	4e75           	rts

00000fe8 <Dispatcher>:
     fe8:	4e56 0000      	linkw %fp,#0
     fec:	2f0a           	movel %a2,%sp@-
     fee:	246e 0014      	moveal %fp@(20),%a2
     ff2:	226a 0196      	moveal %a2@(406),%a1
     ff6:	4a91           	tstl %a1@
     ff8:	674c           	beqs 1046 <Dispatcher+0x5e>
     ffa:	206a 0114      	moveal %a2@(276),%a0
     ffe:	1228 000d      	moveb %a0@(13),%d1
    1002:	b229 000d      	cmpb %a1@(13),%d1
    1006:	6e3e           	bgts 1046 <Dispatcher+0x5e>
    1008:	4a2a 0127      	tstb %a2@(295)
    100c:	6c32           	bges 1040 <Dispatcher+0x58>
    100e:	1028 000f      	moveb %a0@(15),%d0
    1012:	0c00 0002      	cmpib #2,%d0
    1016:	661a           	bnes 1032 <Dispatcher+0x4a>
    1018:	117c 0003 000f 	moveb #3,%a0@(15)
    101e:	2f0a           	movel %a2,%sp@-
    1020:	2f2a 0114      	movel %a2@(276),%sp@-
    1024:	206a ffd2      	moveal %a2@(-46),%a0
    1028:	4e90           	jsr %a0@
    102a:	006a 8000 012a 	oriw #-32768,%a2@(298)
    1030:	6014           	bras 1046 <Dispatcher+0x5e>
    1032:	0c00 0006      	cmpib #6,%d0
    1036:	660e           	bnes 1046 <Dispatcher+0x5e>
    1038:	006a 8000 012a 	oriw #-32768,%a2@(298)
    103e:	6006           	bras 1046 <Dispatcher+0x5e>
    1040:	006a 0080 012a 	oriw #128,%a2@(298)
    1046:	7000           	moveq #0,%d0
    1048:	246e fffc      	moveal %fp@(-4),%a2
    104c:	4e5e           	unlk %fp
    104e:	4e75           	rts

00001050 <idleCount>:
    1050:	4e56 0000      	linkw %fp,#0
    1054:	206e 0008      	moveal %fp@(8),%a0
    1058:	52a8 0118      	addql #1,%a0@(280)
    105c:	4e5e           	unlk %fp
    105e:	4e75           	rts

00001060 <Exec_init>:
    1060:	4e56 0000      	linkw %fp,#0
    1064:	48e7 203c      	moveml %d2/%a2-%a5,%sp@-
    1068:	266e 0010      	moveal %fp@(16),%a3
    106c:	177c 0032 0212 	moveb #50,%a3@(530)
    1072:	2f0b           	movel %a3,%sp@-
    1074:	2f3c 0001 0001 	movel #65537,%sp@-
    107a:	4878 0018      	pea 18 <main+0x18>
    107e:	206b ff3c      	moveal %a3@(-196),%a0
    1082:	4e90           	jsr %a0@
    1084:	2848           	moveal %a0,%a4
    1086:	2f0b           	movel %a3,%sp@-
    1088:	2f3c 0001 0001 	movel #65537,%sp@-
    108e:	4878 00e4      	pea e4 <drawlinevert+0x8>
    1092:	206b ff3c      	moveal %a3@(-196),%a0
    1096:	4e90           	jsr %a0@
    1098:	2448           	moveal %a0,%a2
    109a:	4fef 0018      	lea %sp@(24),%sp
    109e:	b8fc 0000      	cmpaw #0,%a4
    10a2:	6706           	beqs 10aa <Exec_init+0x4a>
    10a4:	b4fc 0000      	cmpaw #0,%a2
    10a8:	6622           	bnes 10cc <Exec_init+0x6c>
    10aa:	206b 0278      	moveal %a3@(632),%a0
    10ae:	4879 0000 0000 	pea 0 <main>
    10b4:	2068 0004      	moveal %a0@(4),%a0
    10b8:	4e90           	jsr %a0@
    10ba:	2f0b           	movel %a3,%sp@-
    10bc:	2f3c 8101 0000 	movel #-2130640896,%sp@-
    10c2:	206b ff96      	moveal %a3@(-106),%a0
    10c6:	4e90           	jsr %a0@
    10c8:	4fef 000c      	lea %sp@(12),%sp
    10cc:	397c 0001 000e 	movew #1,%a4@(14)
    10d2:	294a 0010      	movel %a2,%a4@(16)
    10d6:	297c 0000 00e4 	movel #228,%a4@(20)
    10dc:	0014 
    10de:	43ea 004a      	lea %a2@(74),%a1
    10e2:	2549 0052      	movel %a1,%a2@(82)
    10e6:	42aa 004e      	clrl %a2@(78)
    10ea:	4bea 004e      	lea %a2@(78),%a5
    10ee:	228d           	movel %a5,%a1@
    10f0:	41ea 0070      	lea %a2@(112),%a0
    10f4:	2548 0078      	movel %a0,%a2@(120)
    10f8:	42aa 0074      	clrl %a2@(116)
    10fc:	4bea 0074      	lea %a2@(116),%a5
    1100:	208d           	movel %a5,%a0@
    1102:	41ea 00d0      	lea %a2@(208),%a0
    1106:	2548 00d8      	movel %a0,%a2@(216)
    110a:	42aa 00d4      	clrl %a2@(212)
    110e:	4bea 00d4      	lea %a2@(212),%a5
    1112:	208d           	movel %a5,%a0@
    1114:	2f0c           	movel %a4,%sp@-
    1116:	2f09           	movel %a1,%sp@-
    1118:	206b ff12      	moveal %a3@(-238),%a0
    111c:	4e90           	jsr %a0@
    111e:	257c 0000 0000 	movel #0,%a2@(8)
    1124:	0008 
    1126:	422a 000d      	clrb %a2@(13)
    112a:	157c 0002 000f 	moveb #2,%a2@(15)
    1130:	257c 0000 ffff 	movel #65535,%a2@(18)
    1136:	0012 
    1138:	42aa 003a      	clrl %a2@(58)
    113c:	72ff           	moveq #-1,%d1
    113e:	2541 003e      	movel %d1,%a2@(62)
    1142:	002a 0008 000e 	orib #8,%a2@(14)
    1148:	2f0b           	movel %a3,%sp@-
    114a:	2f3c 0001 0000 	movel #65536,%sp@-
    1150:	4878 005e      	pea 5e <pause+0x3e>
    1154:	206b fd56      	moveal %a3@(-682),%a0
    1158:	4e90           	jsr %a0@
    115a:	2548 0022      	movel %a0,%a2@(34)
    115e:	4fef 0014      	lea %sp@(20),%sp
    1162:	6622           	bnes 1186 <Exec_init+0x126>
    1164:	206b 0278      	moveal %a3@(632),%a0
    1168:	4879 0000 0000 	pea 0 <main>
    116e:	2068 0004      	moveal %a0@(4),%a0
    1172:	4e90           	jsr %a0@
    1174:	2f0b           	movel %a3,%sp@-
    1176:	2f3c 8101 0000 	movel #-2130640896,%sp@-
    117c:	206b ff96      	moveal %a3@(-106),%a0
    1180:	4e90           	jsr %a0@
    1182:	4fef 000c      	lea %sp@(12),%sp
    1186:	2f3c 0001 0001 	movel #65537,%sp@-
    118c:	4878 0050      	pea 50 <pause+0x30>
    1190:	2f0a           	movel %a2,%sp@-
    1192:	61ff 0000 0000 	bsrl 1194 <Exec_init+0x134>
    1198:	226a 0022      	moveal %a2@(34),%a1
    119c:	2348 005a      	movel %a0,%a1@(90)
    11a0:	206a 0022      	moveal %a2@(34),%a0
    11a4:	4fef 000c      	lea %sp@(12),%sp
    11a8:	4aa8 005a      	tstl %a0@(90)
    11ac:	6622           	bnes 11d0 <Exec_init+0x170>
    11ae:	206b 0278      	moveal %a3@(632),%a0
    11b2:	4879 0000 0000 	pea 0 <main>
    11b8:	2068 0004      	moveal %a0@(4),%a0
    11bc:	4e90           	jsr %a0@
    11be:	2f0b           	movel %a3,%sp@-
    11c0:	2f3c 8101 0000 	movel #-2130640896,%sp@-
    11c6:	206b ff96      	moveal %a3@(-106),%a0
    11ca:	4e90           	jsr %a0@
    11cc:	4fef 000c      	lea %sp@(12),%sp
    11d0:	274a 0114      	movel %a2,%a3@(276)
    11d4:	2f0b           	movel %a3,%sp@-
    11d6:	2f3c 0001 0001 	movel #65537,%sp@-
    11dc:	4878 0020      	pea 20 <pause>
    11e0:	206b ff3c      	moveal %a3@(-196),%a0
    11e4:	4e90           	jsr %a0@
    11e6:	2848           	moveal %a0,%a4
    11e8:	2f0b           	movel %a3,%sp@-
    11ea:	2f3c 0001 0001 	movel #65537,%sp@-
    11f0:	4878 005c      	pea 5c <pause+0x3c>
    11f4:	206b ff3c      	moveal %a3@(-196),%a0
    11f8:	4e90           	jsr %a0@
    11fa:	2448           	moveal %a0,%a2
    11fc:	2f0b           	movel %a3,%sp@-
    11fe:	2f3c 0001 0001 	movel #65537,%sp@-
    1204:	4878 1000      	pea 1000 <Dispatcher+0x18>
    1208:	206b ff3c      	moveal %a3@(-196),%a0
    120c:	4e90           	jsr %a0@
    120e:	2408           	movel %a0,%d2
    1210:	4fef 0024      	lea %sp@(36),%sp
    1214:	b8fc 0000      	cmpaw #0,%a4
    1218:	670a           	beqs 1224 <Exec_init+0x1c4>
    121a:	b4fc 0000      	cmpaw #0,%a2
    121e:	6704           	beqs 1224 <Exec_init+0x1c4>
    1220:	4a82           	tstl %d2
    1222:	6622           	bnes 1246 <Exec_init+0x1e6>
    1224:	206b 0278      	moveal %a3@(632),%a0
    1228:	4879 0000 0000 	pea 0 <main>
    122e:	2068 0004      	moveal %a0@(4),%a0
    1232:	4e90           	jsr %a0@
    1234:	2f0b           	movel %a3,%sp@-
    1236:	2f3c 8101 0000 	movel #-2130640896,%sp@-
    123c:	206b ff96      	moveal %a3@(-106),%a0
    1240:	4e90           	jsr %a0@
    1242:	4fef 000c      	lea %sp@(12),%sp
    1246:	397c 0002 000e 	movew #2,%a4@(14)
    124c:	294a 0010      	movel %a2,%a4@(16)
    1250:	725c           	moveq #92,%d1
    1252:	2941 0014      	movel %d1,%a4@(20)
    1256:	2942 0018      	movel %d2,%a4@(24)
    125a:	297c 0000 1000 	movel #4096,%a4@(28)
    1260:	001c 
    1262:	41ea 004a      	lea %a2@(74),%a0
    1266:	2548 0052      	movel %a0,%a2@(82)
    126a:	42aa 004e      	clrl %a2@(78)
    126e:	4bea 004e      	lea %a2@(78),%a5
    1272:	208d           	movel %a5,%a0@
    1274:	2f0c           	movel %a4,%sp@-
    1276:	2f08           	movel %a0,%sp@-
    1278:	206b ff12      	moveal %a3@(-238),%a0
    127c:	4e90           	jsr %a0@
    127e:	2542 003a      	movel %d2,%a2@(58)
    1282:	2202           	movel %d2,%d1
    1284:	0681 0000 1000 	addil #4096,%d1
    128a:	2541 003e      	movel %d1,%a2@(62)
    128e:	2042           	moveal %d2,%a0
    1290:	41e8 0d84      	lea %a0@(3460),%a0
    1294:	2548 0036      	movel %a0,%a2@(54)
    1298:	208b           	movel %a3,%a0@
    129a:	257c 0000 0000 	movel #0,%a2@(8)
    12a0:	0008 
    12a2:	157c ff80 000d 	moveb #-128,%a2@(13)
    12a8:	257c 0000 0000 	movel #0,%a2@(70)
    12ae:	0046 
    12b0:	157c 0080 000e 	moveb #-128,%a2@(14)
    12b6:	2f0b           	movel %a3,%sp@-
    12b8:	42a7           	clrl %sp@-
    12ba:	4879 0000 0000 	pea 0 <main>
    12c0:	2f0a           	movel %a2,%sp@-
    12c2:	206b fee8      	moveal %a3@(-280),%a0
    12c6:	4e90           	jsr %a0@
    12c8:	7400           	moveq #0,%d2
    12ca:	4fef 0018      	lea %sp@(24),%sp
    12ce:	7001           	moveq #1,%d0
    12d0:	e5a8           	lsll %d2,%d0
    12d2:	0240 a038      	andiw #-24520,%d0
    12d6:	6766           	beqs 133e <Exec_init+0x2de>
    12d8:	2f0b           	movel %a3,%sp@-
    12da:	2f3c 0001 0001 	movel #65537,%sp@-
    12e0:	4878 0026      	pea 26 <pause+0x6>
    12e4:	206b ff3c      	moveal %a3@(-196),%a0
    12e8:	4e90           	jsr %a0@
    12ea:	2448           	moveal %a0,%a2
    12ec:	4fef 000c      	lea %sp@(12),%sp
    12f0:	b4fc 0000      	cmpaw #0,%a2
    12f4:	6622           	bnes 1318 <Exec_init+0x2b8>
    12f6:	206b 0278      	moveal %a3@(632),%a0
    12fa:	4879 0000 0000 	pea 0 <main>
    1300:	2068 0004      	moveal %a0@(4),%a0
    1304:	4e90           	jsr %a0@
    1306:	2f0b           	movel %a3,%sp@-
    1308:	2f3c 8100 0006 	movel #-2130706426,%sp@-
    130e:	206b ff96      	moveal %a3@(-106),%a0
    1312:	4e90           	jsr %a0@
    1314:	4fef 000c      	lea %sp@(12),%sp
    1318:	41ea 0016      	lea %a2@(22),%a0
    131c:	257c 0000 0000 	movel #0,%a2@(18)
    1322:	0012 
    1324:	2548 000e      	movel %a0,%a2@(14)
    1328:	2148 0008      	movel %a0,%a0@(8)
    132c:	42a8 0004      	clrl %a0@(4)
    1330:	4bea 001a      	lea %a2@(26),%a5
    1334:	208d           	movel %a5,%a0@
    1336:	2f0b           	movel %a3,%sp@-
    1338:	2f0a           	movel %a2,%sp@-
    133a:	2f02           	movel %d2,%sp@-
    133c:	606c           	bras 13aa <Exec_init+0x34a>
    133e:	7202           	moveq #2,%d1
    1340:	b282           	cmpl %d2,%d1
    1342:	6670           	bnes 13b4 <Exec_init+0x354>
    1344:	2f0b           	movel %a3,%sp@-
    1346:	2f3c 0001 0001 	movel #65537,%sp@-
    134c:	4878 0016      	pea 16 <main+0x16>
    1350:	206b ff3c      	moveal %a3@(-196),%a0
    1354:	4e90           	jsr %a0@
    1356:	2448           	moveal %a0,%a2
    1358:	4fef 000c      	lea %sp@(12),%sp
    135c:	b4fc 0000      	cmpaw #0,%a2
    1360:	6622           	bnes 1384 <Exec_init+0x324>
    1362:	206b 0278      	moveal %a3@(632),%a0
    1366:	4879 0000 0000 	pea 0 <main>
    136c:	2068 0004      	moveal %a0@(4),%a0
    1370:	4e90           	jsr %a0@
    1372:	2f0b           	movel %a3,%sp@-
    1374:	2f3c 8100 0006 	movel #-2130706426,%sp@-
    137a:	206b ff96      	moveal %a3@(-106),%a0
    137e:	4e90           	jsr %a0@
    1380:	4fef 000c      	lea %sp@(12),%sp
    1384:	157c 0002 000c 	moveb #2,%a2@(12)
    138a:	422a 000d      	clrb %a2@(13)
    138e:	257c 0000 0000 	movel #0,%a2@(8)
    1394:	0008 
    1396:	42aa 000e      	clrl %a2@(14)
    139a:	257c 0000 0000 	movel #0,%a2@(18)
    13a0:	0012 
    13a2:	2f0b           	movel %a3,%sp@-
    13a4:	2f0a           	movel %a2,%sp@-
    13a6:	4878 0002      	pea 2 <main+0x2>
    13aa:	206b ff60      	moveal %a3@(-160),%a0
    13ae:	4e90           	jsr %a0@
    13b0:	4fef 000c      	lea %sp@(12),%sp
    13b4:	5282           	addql #1,%d2
    13b6:	720f           	moveq #15,%d1
    13b8:	b282           	cmpl %d2,%d1
    13ba:	6c00 ff12      	bgew 12ce <Exec_init+0x26e>
    13be:	2f0b           	movel %a3,%sp@-
    13c0:	2f3c 0001 0001 	movel #65537,%sp@-
    13c6:	4878 0016      	pea 16 <main+0x16>
    13ca:	206b ff3c      	moveal %a3@(-196),%a0
    13ce:	4e90           	jsr %a0@
    13d0:	2448           	moveal %a0,%a2
    13d2:	4fef 000c      	lea %sp@(12),%sp
    13d6:	b4fc 0000      	cmpaw #0,%a2
    13da:	6622           	bnes 13fe <Exec_init+0x39e>
    13dc:	206b 0278      	moveal %a3@(632),%a0
    13e0:	4879 0000 0000 	pea 0 <main>
    13e6:	2068 0004      	moveal %a0@(4),%a0
    13ea:	4e90           	jsr %a0@
    13ec:	2f0b           	movel %a3,%sp@-
    13ee:	2f3c 8100 0006 	movel #-2130706426,%sp@-
    13f4:	206b ff96      	moveal %a3@(-106),%a0
    13f8:	4e90           	jsr %a0@
    13fa:	4fef 000c      	lea %sp@(12),%sp
    13fe:	257c 0000 0000 	movel #0,%a2@(18)
    1404:	0012 
    1406:	2f0b           	movel %a3,%sp@-
    1408:	2f0a           	movel %a2,%sp@-
    140a:	4878 0005      	pea 5 <main+0x5>
    140e:	206b ff5a      	moveal %a3@(-166),%a0
    1412:	4e90           	jsr %a0@
    1414:	2f0b           	movel %a3,%sp@-
    1416:	206b ff84      	moveal %a3@(-124),%a0
    141a:	4e90           	jsr %a0@
    141c:	61ff 0000 0000 	bsrl 141e <Exec_init+0x3be>
    1422:	2f0b           	movel %a3,%sp@-
    1424:	42a7           	clrl %sp@-
    1426:	4878 0001      	pea 1 <main+0x1>
    142a:	206b ffba      	moveal %a3@(-70),%a0
    142e:	4e90           	jsr %a0@
    1430:	91c8           	subal %a0,%a0
    1432:	2008           	movel %a0,%d0
    1434:	4cee 3c04 ffec 	moveml %fp@(-20),%d2/%a2-%a5
    143a:	4e5e           	unlk %fp
    143c:	4e75           	rts

0000143e <Exec_open>:
    143e:	4e56 0000      	linkw %fp,#0
    1442:	206e 000c      	moveal %fp@(12),%a0
    1446:	5268 0020      	addqw #1,%a0@(32)
    144a:	2008           	movel %a0,%d0
    144c:	4e5e           	unlk %fp
    144e:	4e75           	rts

00001450 <Exec_close>:
    1450:	4e56 0000      	linkw %fp,#0
    1454:	206e 0008      	moveal %fp@(8),%a0
    1458:	5368 0020      	subqw #1,%a0@(32)
    145c:	91c8           	subal %a0,%a0
    145e:	2008           	movel %a0,%d0
    1460:	4e5e           	unlk %fp
    1462:	4e75           	rts

00001464 <Exec_null>:
    1464:	4e56 0000      	linkw %fp,#0
    1468:	7000           	moveq #0,%d0
    146a:	4e5e           	unlk %fp
    146c:	4e75           	rts
    146e:	4e75           	rts

00001470 <Exec_TrapHandler>:
    1470:	4e56 0000      	linkw %fp,#0
    1474:	2f0a           	movel %a2,%sp@-
    1476:	246e 0008      	moveal %fp@(8),%a2
    147a:	2f0a           	movel %a2,%sp@-
    147c:	42a7           	clrl %sp@-
    147e:	206a fedc      	moveal %a2@(-292),%a0
    1482:	4e90           	jsr %a0@
    1484:	226a 0278      	moveal %a2@(632),%a1
    1488:	508f           	addql #8,%sp
    148a:	223c 0000 0000 	movel #0,%d1
    1490:	b0fc 0000      	cmpaw #0,%a0
    1494:	6708           	beqs 149e <Exec_TrapHandler+0x2e>
    1496:	2028 0008      	movel %a0@(8),%d0
    149a:	6702           	beqs 149e <Exec_TrapHandler+0x2e>
    149c:	2200           	movel %d0,%d1
    149e:	2f01           	movel %d1,%sp@-
    14a0:	2f08           	movel %a0,%sp@-
    14a2:	4879 0000 0000 	pea 0 <main>
    14a8:	2069 0004      	moveal %a1@(4),%a0
    14ac:	4e90           	jsr %a0@
    14ae:	246e fffc      	moveal %fp@(-4),%a2
    14b2:	4e5e           	unlk %fp
    14b4:	4e75           	rts
    14b6:	4e75           	rts

000014b8 <AllocTaskMem>:
    14b8:	4e56 0000      	linkw %fp,#0
    14bc:	48e7 3020      	moveml %d2-%d3/%a2,%sp@-
    14c0:	262e 000c      	movel %fp@(12),%d3
    14c4:	242e 0010      	movel %fp@(16),%d2
    14c8:	2079 0000 0000 	moveal 0 <main>,%a0
    14ce:	2f08           	movel %a0,%sp@-
    14d0:	42a7           	clrl %sp@-
    14d2:	4878 0018      	pea 18 <main+0x18>
    14d6:	2068 ff3c      	moveal %a0@(-196),%a0
    14da:	4e90           	jsr %a0@
    14dc:	2448           	moveal %a0,%a2
    14de:	2079 0000 0000 	moveal 0 <main>,%a0
    14e4:	2f08           	movel %a0,%sp@-
    14e6:	2f02           	movel %d2,%sp@-
    14e8:	2f03           	movel %d3,%sp@-
    14ea:	2068 ff3c      	moveal %a0@(-196),%a0
    14ee:	4e90           	jsr %a0@
    14f0:	2408           	movel %a0,%d2
    14f2:	4fef 0018      	lea %sp@(24),%sp
    14f6:	b4fc 0000      	cmpaw #0,%a2
    14fa:	671c           	beqs 1518 <AllocTaskMem+0x60>
    14fc:	4a82           	tstl %d2
    14fe:	6632           	bnes 1532 <AllocTaskMem+0x7a>
    1500:	2079 0000 0000 	moveal 0 <main>,%a0
    1506:	2f08           	movel %a0,%sp@-
    1508:	4878 0018      	pea 18 <main+0x18>
    150c:	2f0a           	movel %a2,%sp@-
    150e:	2068 ff30      	moveal %a0@(-208),%a0
    1512:	4e90           	jsr %a0@
    1514:	4fef 000c      	lea %sp@(12),%sp
    1518:	4a82           	tstl %d2
    151a:	6712           	beqs 152e <AllocTaskMem+0x76>
    151c:	2079 0000 0000 	moveal 0 <main>,%a0
    1522:	2f08           	movel %a0,%sp@-
    1524:	2f03           	movel %d3,%sp@-
    1526:	2f02           	movel %d2,%sp@-
    1528:	2068 ff30      	moveal %a0@(-208),%a0
    152c:	4e90           	jsr %a0@
    152e:	91c8           	subal %a0,%a0
    1530:	6042           	bras 1574 <AllocTaskMem+0xbc>
    1532:	357c 0001 000e 	movew #1,%a2@(14)
    1538:	2542 0010      	movel %d2,%a2@(16)
    153c:	2543 0014      	movel %d3,%a2@(20)
    1540:	2079 0000 0000 	moveal 0 <main>,%a0
    1546:	2f08           	movel %a0,%sp@-
    1548:	2068 ff7e      	moveal %a0@(-130),%a0
    154c:	4e90           	jsr %a0@
    154e:	2079 0000 0000 	moveal 0 <main>,%a0
    1554:	2f0a           	movel %a2,%sp@-
    1556:	704a           	moveq #74,%d0
    1558:	d0ae 0008      	addl %fp@(8),%d0
    155c:	2f00           	movel %d0,%sp@-
    155e:	2068 ff12      	moveal %a0@(-238),%a0
    1562:	4e90           	jsr %a0@
    1564:	2079 0000 0000 	moveal 0 <main>,%a0
    156a:	2f08           	movel %a0,%sp@-
    156c:	2068 ff78      	moveal %a0@(-136),%a0
    1570:	4e90           	jsr %a0@
    1572:	2042           	moveal %d2,%a0
    1574:	2008           	movel %a0,%d0
    1576:	4cee 040c fff4 	moveml %fp@(-12),%d2-%d3/%a2
    157c:	4e5e           	unlk %fp
    157e:	4e75           	rts

00001580 <FreeTaskMem>:
    1580:	4e56 0000      	linkw %fp,#0
    1584:	2f0a           	movel %a2,%sp@-
    1586:	2f02           	movel %d2,%sp@-
    1588:	246e 0008      	moveal %fp@(8),%a2
    158c:	242e 000c      	movel %fp@(12),%d2
    1590:	2079 0000 0000 	moveal 0 <main>,%a0
    1596:	2f08           	movel %a0,%sp@-
    1598:	2068 ff7e      	moveal %a0@(-130),%a0
    159c:	4e90           	jsr %a0@
    159e:	246a 004a      	moveal %a2@(74),%a2
    15a2:	588f           	addql #4,%sp
    15a4:	2052           	moveal %a2@,%a0
    15a6:	b0fc 0000      	cmpaw #0,%a0
    15aa:	6760           	beqs 160c <FreeTaskMem+0x8c>
    15ac:	0c6a 0001 000e 	cmpiw #1,%a2@(14)
    15b2:	664e           	bnes 1602 <FreeTaskMem+0x82>
    15b4:	b4aa 0010      	cmpl %a2@(16),%d2
    15b8:	6648           	bnes 1602 <FreeTaskMem+0x82>
    15ba:	2079 0000 0000 	moveal 0 <main>,%a0
    15c0:	2f0a           	movel %a2,%sp@-
    15c2:	2068 ff06      	moveal %a0@(-250),%a0
    15c6:	4e90           	jsr %a0@
    15c8:	2079 0000 0000 	moveal 0 <main>,%a0
    15ce:	2f08           	movel %a0,%sp@-
    15d0:	2068 ff78      	moveal %a0@(-136),%a0
    15d4:	4e90           	jsr %a0@
    15d6:	2079 0000 0000 	moveal 0 <main>,%a0
    15dc:	2f08           	movel %a0,%sp@-
    15de:	2f2a 0014      	movel %a2@(20),%sp@-
    15e2:	2f2a 0010      	movel %a2@(16),%sp@-
    15e6:	2068 ff30      	moveal %a0@(-208),%a0
    15ea:	4e90           	jsr %a0@
    15ec:	2079 0000 0000 	moveal 0 <main>,%a0
    15f2:	2f08           	movel %a0,%sp@-
    15f4:	4878 0018      	pea 18 <main+0x18>
    15f8:	2f0a           	movel %a2,%sp@-
    15fa:	2068 ff30      	moveal %a0@(-208),%a0
    15fe:	4e90           	jsr %a0@
    1600:	6018           	bras 161a <FreeTaskMem+0x9a>
    1602:	2448           	moveal %a0,%a2
    1604:	2050           	moveal %a0@,%a0
    1606:	b0fc 0000      	cmpaw #0,%a0
    160a:	66a0           	bnes 15ac <FreeTaskMem+0x2c>
    160c:	2079 0000 0000 	moveal 0 <main>,%a0
    1612:	2f08           	movel %a0,%sp@-
    1614:	2068 ff78      	moveal %a0@(-136),%a0
    1618:	4e90           	jsr %a0@
    161a:	242e fff8      	movel %fp@(-8),%d2
    161e:	246e fffc      	moveal %fp@(-4),%a2
    1622:	4e5e           	unlk %fp
    1624:	4e75           	rts

00001626 <FindTaskByID>:
    1626:	4e56 0000      	linkw %fp,#0
    162a:	202e 0008      	movel %fp@(8),%d0
    162e:	2079 0000 0000 	moveal 0 <main>,%a0
    1634:	2068 0114      	moveal %a0@(276),%a0
    1638:	93c9           	subal %a1,%a1
    163a:	0828 0003 000e 	btst #3,%a0@(14)
    1640:	6704           	beqs 1646 <FindTaskByID+0x20>
    1642:	2268 0022      	moveal %a0@(34),%a1
    1646:	b2fc 0000      	cmpaw #0,%a1
    164a:	6706           	beqs 1652 <FindTaskByID+0x2c>
    164c:	b0a9 0018      	cmpl %a1@(24),%d0
    1650:	675e           	beqs 16b0 <FindTaskByID+0x8a>
    1652:	2079 0000 0000 	moveal 0 <main>,%a0
    1658:	2068 0196      	moveal %a0@(406),%a0
    165c:	4a90           	tstl %a0@
    165e:	6720           	beqs 1680 <FindTaskByID+0x5a>
    1660:	93c9           	subal %a1,%a1
    1662:	0828 0003 000e 	btst #3,%a0@(14)
    1668:	6704           	beqs 166e <FindTaskByID+0x48>
    166a:	2268 0022      	moveal %a0@(34),%a1
    166e:	b2fc 0000      	cmpaw #0,%a1
    1672:	6706           	beqs 167a <FindTaskByID+0x54>
    1674:	b0a9 0018      	cmpl %a1@(24),%d0
    1678:	6736           	beqs 16b0 <FindTaskByID+0x8a>
    167a:	2050           	moveal %a0@,%a0
    167c:	4a90           	tstl %a0@
    167e:	66e0           	bnes 1660 <FindTaskByID+0x3a>
    1680:	2079 0000 0000 	moveal 0 <main>,%a0
    1686:	2068 01a4      	moveal %a0@(420),%a0
    168a:	4a90           	tstl %a0@
    168c:	6720           	beqs 16ae <FindTaskByID+0x88>
    168e:	93c9           	subal %a1,%a1
    1690:	0828 0003 000e 	btst #3,%a0@(14)
    1696:	6704           	beqs 169c <FindTaskByID+0x76>
    1698:	2268 0022      	moveal %a0@(34),%a1
    169c:	b2fc 0000      	cmpaw #0,%a1
    16a0:	6706           	beqs 16a8 <FindTaskByID+0x82>
    16a2:	b0a9 0018      	cmpl %a1@(24),%d0
    16a6:	6708           	beqs 16b0 <FindTaskByID+0x8a>
    16a8:	2050           	moveal %a0@,%a0
    16aa:	4a90           	tstl %a0@
    16ac:	66e0           	bnes 168e <FindTaskByID+0x68>
    16ae:	91c8           	subal %a0,%a0
    16b0:	2008           	movel %a0,%d0
    16b2:	4e5e           	unlk %fp
    16b4:	4e75           	rts
    16b6:	4e75           	rts

000016b8 <Exec_Cause>:
    16b8:	4e56 0000      	linkw %fp,#0
    16bc:	48e7 2030      	moveml %d2/%a2-%a3,%sp@-
    16c0:	266e 0008      	moveal %fp@(8),%a3
    16c4:	246e 000c      	moveal %fp@(12),%a2
    16c8:	0c2b 000b 000c 	cmpib #11,%a3@(12)
    16ce:	6764           	beqs 1734 <Exec_Cause+0x7c>
    16d0:	102b 000d      	moveb %a3@(13),%d0
    16d4:	4880           	extw %d0
    16d6:	3040           	moveaw %d0,%a0
    16d8:	7420           	moveq #32,%d2
    16da:	d488           	addl %a0,%d2
    16dc:	e882           	asrl #4,%d2
    16de:	2f0a           	movel %a2,%sp@-
    16e0:	206a ff8a      	moveal %a2@(-118),%a0
    16e4:	4e90           	jsr %a0@
    16e6:	2f0b           	movel %a3,%sp@-
    16e8:	4280           	clrl %d0
    16ea:	1002           	moveb %d2,%d0
    16ec:	e988           	lsll #4,%d0
    16ee:	0680 0000 01b2 	addil #434,%d0
    16f4:	4872 0800      	pea %a2@(00000000,%d0:l)
    16f8:	206a ff0c      	moveal %a2@(-244),%a0
    16fc:	4e90           	jsr %a0@
    16fe:	177c 000b 000c 	moveb #11,%a3@(12)
    1704:	006a 0020 0124 	oriw #32,%a2@(292)
    170a:	2f0a           	movel %a2,%sp@-
    170c:	206a ff84      	moveal %a2@(-124),%a0
    1710:	4e90           	jsr %a0@
    1712:	4fef 0010      	lea %sp@(16),%sp
    1716:	4a39 0000 0000 	tstb 0 <main>
    171c:	6616           	bnes 1734 <Exec_Cause+0x7c>
    171e:	206a 0070      	moveal %a2@(112),%a0
    1722:	b0fc 0000      	cmpaw #0,%a0
    1726:	670c           	beqs 1734 <Exec_Cause+0x7c>
    1728:	2f0a           	movel %a2,%sp@-
    172a:	2f08           	movel %a0,%sp@-
    172c:	42a7           	clrl %sp@-
    172e:	42a7           	clrl %sp@-
    1730:	42a7           	clrl %sp@-
    1732:	4e90           	jsr %a0@
    1734:	4cee 0c04 fff4 	moveml %fp@(-12),%d2/%a2-%a3
    173a:	4e5e           	unlk %fp
    173c:	4e75           	rts

0000173e <sys_Cause>:
    173e:	4e56 0000      	linkw %fp,#0
    1742:	2f0a           	movel %a2,%sp@-
    1744:	246e 0008      	moveal %fp@(8),%a2
    1748:	082a 0005 0040 	btst #5,%a2@(64)
    174e:	6600 00b0      	bnew 1800 <sys_Cause+0xc2>
    1752:	2279 0000 0000 	moveal 0 <main>,%a1
    1758:	2069 0070      	moveal %a1@(112),%a0
    175c:	b0fc 0000      	cmpaw #0,%a0
    1760:	6710           	beqs 1772 <sys_Cause+0x34>
    1762:	2f09           	movel %a1,%sp@-
    1764:	2f08           	movel %a0,%sp@-
    1766:	2f0a           	movel %a2,%sp@-
    1768:	42a7           	clrl %sp@-
    176a:	42a7           	clrl %sp@-
    176c:	4e90           	jsr %a0@
    176e:	4fef 0014      	lea %sp@(20),%sp
    1772:	2079 0000 0000 	moveal 0 <main>,%a0
    1778:	3028 012a      	movew %a0@(298),%d0
    177c:	6c00 0082      	bgew 1800 <sys_Cause+0xc2>
    1780:	0240 7fff      	andiw #32767,%d0
    1784:	3140 012a      	movew %d0,%a0@(298)
    1788:	2068 0114      	moveal %a0@(276),%a0
    178c:	b0fc 0000      	cmpaw #0,%a0
    1790:	6714           	beqs 17a6 <sys_Cause+0x68>
    1792:	0c28 0006 000f 	cmpib #6,%a0@(15)
    1798:	670c           	beqs 17a6 <sys_Cause+0x68>
    179a:	2f0a           	movel %a2,%sp@-
    179c:	2f08           	movel %a0,%sp@-
    179e:	61ff 0000 0000 	bsrl 17a0 <sys_Cause+0x62>
    17a4:	508f           	addql #8,%sp
    17a6:	2079 0000 0000 	moveal 0 <main>,%a0
    17ac:	2f08           	movel %a0,%sp@-
    17ae:	2068 ffc6      	moveal %a0@(-58),%a0
    17b2:	4e90           	jsr %a0@
    17b4:	2f0a           	movel %a2,%sp@-
    17b6:	2079 0000 0000 	moveal 0 <main>,%a0
    17bc:	2f28 0114      	movel %a0@(276),%sp@-
    17c0:	61ff 0000 0000 	bsrl 17c2 <sys_Cause+0x84>
    17c6:	2279 0000 0000 	moveal 0 <main>,%a1
    17cc:	2069 0114      	moveal %a1@(276),%a0
    17d0:	4fef 000c      	lea %sp@(12),%sp
    17d4:	0828 0005 000e 	btst #5,%a0@(14)
    17da:	6724           	beqs 1800 <sys_Cause+0xc2>
    17dc:	2f09           	movel %a1,%sp@-
    17de:	2069 ff8a      	moveal %a1@(-118),%a0
    17e2:	4e90           	jsr %a0@
    17e4:	2079 0000 0000 	moveal 0 <main>,%a0
    17ea:	2f08           	movel %a0,%sp@-
    17ec:	2068 ffc0      	moveal %a0@(-64),%a0
    17f0:	4e90           	jsr %a0@
    17f2:	2079 0000 0000 	moveal 0 <main>,%a0
    17f8:	2f08           	movel %a0,%sp@-
    17fa:	2068 ff84      	moveal %a0@(-124),%a0
    17fe:	4e90           	jsr %a0@
    1800:	246e fffc      	moveal %fp@(-4),%a2
    1804:	4e5e           	unlk %fp
    1806:	4e75           	rts

00001808 <SoftIntDispatch>:
    1808:	4e56 0000      	linkw %fp,#0
    180c:	48e7 3020      	moveml %d2-%d3/%a2,%sp@-
    1810:	246e 0018      	moveal %fp@(24),%a2
    1814:	082a 0005 0125 	btst #5,%a2@(293)
    181a:	675a           	beqs 1876 <SoftIntDispatch+0x6e>
    181c:	13fc 0001 0000 	moveb #1,0 <main>
    1822:	0000 
    1824:	026a ffdf 0124 	andiw #-33,%a2@(292)
    182a:	4203           	clrb %d3
    182c:	7400           	moveq #0,%d2
    182e:	1403           	moveb %d3,%d2
    1830:	2002           	movel %d2,%d0
    1832:	e988           	lsll #4,%d0
    1834:	0680 0000 01b2 	addil #434,%d0
    183a:	4872 0800      	pea %a2@(00000000,%d0:l)
    183e:	206a ff00      	moveal %a2@(-256),%a0
    1842:	4e90           	jsr %a0@
    1844:	588f           	addql #4,%sp
    1846:	b0fc 0000      	cmpaw #0,%a0
    184a:	671c           	beqs 1868 <SoftIntDispatch+0x60>
    184c:	117c 0002 000c 	moveb #2,%a0@(12)
    1852:	2f0a           	movel %a2,%sp@-
    1854:	2f28 0012      	movel %a0@(18),%sp@-
    1858:	2f28 000e      	movel %a0@(14),%sp@-
    185c:	2068 0012      	moveal %a0@(18),%a0
    1860:	4e90           	jsr %a0@
    1862:	4fef 000c      	lea %sp@(12),%sp
    1866:	60c6           	bras 182e <SoftIntDispatch+0x26>
    1868:	5203           	addqb #1,%d3
    186a:	0c03 0003      	cmpib #3,%d3
    186e:	63bc           	blss 182c <SoftIntDispatch+0x24>
    1870:	4239 0000 0000 	clrb 0 <main>
    1876:	4cee 040c fff4 	moveml %fp@(-12),%d2-%d3/%a2
    187c:	4e5e           	unlk %fp
    187e:	4e75           	rts

00001880 <idleTask>:
    1880:	4e56 0000      	linkw %fp,#0
    1884:	307c fa2d      	moveaw #-1491,%a0
    1888:	1010           	moveb %a0@,%d0
    188a:	5300           	subqb #1,%d0
    188c:	0200 000f      	andib #15,%d0
    1890:	1080           	moveb %d0,%a0@
    1892:	7000           	moveq #0,%d0
    1894:	5280           	addql #1,%d0
    1896:	0c80 0000 ffff 	cmpil #65535,%d0
    189c:	63f6           	blss 1894 <idleTask+0x14>
    189e:	60e8           	bras 1888 <idleTask+0x8>
    18a0:	4e71           	nop
    18a2:	4e75           	rts

000018a4 <Exec_AbortIO>:
    18a4:	4e56 0000      	linkw %fp,#0
    18a8:	2f0a           	movel %a2,%sp@-
    18aa:	246e 0008      	moveal %fp@(8),%a2
    18ae:	b5fc 0000 0400 	cmpal #1024,%a2
    18b4:	6f16           	bles 18cc <Exec_AbortIO+0x28>
    18b6:	2079 0000 0000 	moveal 0 <main>,%a0
    18bc:	2f08           	movel %a0,%sp@-
    18be:	2f0a           	movel %a2,%sp@-
    18c0:	2068 fdec      	moveal %a0@(-532),%a0
    18c4:	4e90           	jsr %a0@
    18c6:	508f           	addql #8,%sp
    18c8:	4a80           	tstl %d0
    18ca:	662c           	bnes 18f8 <Exec_AbortIO+0x54>
    18cc:	2079 0000 0000 	moveal 0 <main>,%a0
    18d2:	2068 0278      	moveal %a0@(632),%a0
    18d6:	2f0a           	movel %a2,%sp@-
    18d8:	4879 0000 0000 	pea 0 <main>
    18de:	4878 0036      	pea 36 <pause+0x16>
    18e2:	4879 0000 0000 	pea 0 <main>
    18e8:	4879 0000 0000 	pea 0 <main>
    18ee:	2068 0004      	moveal %a0@(4),%a0
    18f2:	4e90           	jsr %a0@
    18f4:	4fef 0014      	lea %sp@(20),%sp
    18f8:	202a 0014      	movel %a2@(20),%d0
    18fc:	0c80 0000 0400 	cmpil #1024,%d0
    1902:	6f16           	bles 191a <Exec_AbortIO+0x76>
    1904:	2079 0000 0000 	moveal 0 <main>,%a0
    190a:	2f08           	movel %a0,%sp@-
    190c:	2f00           	movel %d0,%sp@-
    190e:	2068 fdec      	moveal %a0@(-532),%a0
    1912:	4e90           	jsr %a0@
    1914:	508f           	addql #8,%sp
    1916:	4a80           	tstl %d0
    1918:	662e           	bnes 1948 <Exec_AbortIO+0xa4>
    191a:	2079 0000 0000 	moveal 0 <main>,%a0
    1920:	2068 0278      	moveal %a0@(632),%a0
    1924:	2f2a 0014      	movel %a2@(20),%sp@-
    1928:	4879 0000 0000 	pea 0 <main>
    192e:	4878 0037      	pea 37 <pause+0x17>
    1932:	4879 0000 0000 	pea 0 <main>
    1938:	4879 0000 0000 	pea 0 <main>
    193e:	2068 0004      	moveal %a0@(4),%a0
    1942:	4e90           	jsr %a0@
    1944:	4fef 0014      	lea %sp@(20),%sp
    1948:	206a 0014      	moveal %a2@(20),%a0
    194c:	2f08           	movel %a0,%sp@-
    194e:	2f0a           	movel %a2,%sp@-
    1950:	2068 ffde      	moveal %a0@(-34),%a0
    1954:	4e90           	jsr %a0@
    1956:	246e fffc      	moveal %fp@(-4),%a2
    195a:	4e5e           	unlk %fp
    195c:	4e75           	rts

0000195e <Exec_AddDevice>:
    195e:	4e56 0000      	linkw %fp,#0
    1962:	2f0b           	movel %a3,%sp@-
    1964:	2f0a           	movel %a2,%sp@-
    1966:	266e 0008      	moveal %fp@(8),%a3
    196a:	246e 000c      	moveal %fp@(12),%a2
    196e:	b7fc 0000 0400 	cmpal #1024,%a3
    1974:	6f10           	bles 1986 <Exec_AddDevice+0x28>
    1976:	2f0a           	movel %a2,%sp@-
    1978:	2f0b           	movel %a3,%sp@-
    197a:	206a fdec      	moveal %a2@(-532),%a0
    197e:	4e90           	jsr %a0@
    1980:	508f           	addql #8,%sp
    1982:	4a80           	tstl %d0
    1984:	6626           	bnes 19ac <Exec_AddDevice+0x4e>
    1986:	206a 0278      	moveal %a2@(632),%a0
    198a:	2f0b           	movel %a3,%sp@-
    198c:	4879 0000 0000 	pea 0 <main>
    1992:	4878 0034      	pea 34 <pause+0x14>
    1996:	4879 0000 0000 	pea 0 <main>
    199c:	4879 0000 0000 	pea 0 <main>
    19a2:	2068 0004      	moveal %a0@(4),%a0
    19a6:	4e90           	jsr %a0@
    19a8:	4fef 0014      	lea %sp@(20),%sp
    19ac:	177c 0003 000c 	moveb #3,%a3@(12)
    19b2:	002b 0002 000e 	orib #2,%a3@(14)
    19b8:	2f0a           	movel %a2,%sp@-
    19ba:	2f0b           	movel %a3,%sp@-
    19bc:	206a fe58      	moveal %a2@(-424),%a0
    19c0:	4e90           	jsr %a0@
    19c2:	2f0a           	movel %a2,%sp@-
    19c4:	206a ff7e      	moveal %a2@(-130),%a0
    19c8:	4e90           	jsr %a0@
    19ca:	2f0b           	movel %a3,%sp@-
    19cc:	486a 015e      	pea %a2@(350)
    19d0:	206a fef4      	moveal %a2@(-268),%a0
    19d4:	4e90           	jsr %a0@
    19d6:	2f0a           	movel %a2,%sp@-
    19d8:	206a ff78      	moveal %a2@(-136),%a0
    19dc:	4e90           	jsr %a0@
    19de:	246e fff8      	moveal %fp@(-8),%a2
    19e2:	266e fffc      	moveal %fp@(-4),%a3
    19e6:	4e5e           	unlk %fp
    19e8:	4e75           	rts

000019ea <Exec_AddHead>:
    19ea:	4e56 0000      	linkw %fp,#0
    19ee:	2f0b           	movel %a3,%sp@-
    19f0:	2f0a           	movel %a2,%sp@-
    19f2:	266e 0008      	moveal %fp@(8),%a3
    19f6:	246e 000c      	moveal %fp@(12),%a2
    19fa:	b5fc 0000 0400 	cmpal #1024,%a2
    1a00:	6f16           	bles 1a18 <Exec_AddHead+0x2e>
    1a02:	2079 0000 0000 	moveal 0 <main>,%a0
    1a08:	2f08           	movel %a0,%sp@-
    1a0a:	2f0a           	movel %a2,%sp@-
    1a0c:	2068 fdec      	moveal %a0@(-532),%a0
    1a10:	4e90           	jsr %a0@
    1a12:	508f           	addql #8,%sp
    1a14:	4a80           	tstl %d0
    1a16:	662c           	bnes 1a44 <Exec_AddHead+0x5a>
    1a18:	2079 0000 0000 	moveal 0 <main>,%a0
    1a1e:	2068 0278      	moveal %a0@(632),%a0
    1a22:	2f0a           	movel %a2,%sp@-
    1a24:	4879 0000 0000 	pea 0 <main>
    1a2a:	4878 003a      	pea 3a <pause+0x1a>
    1a2e:	4879 0000 0000 	pea 0 <main>
    1a34:	4879 0000 0000 	pea 0 <main>
    1a3a:	2068 0004      	moveal %a0@(4),%a0
    1a3e:	4e90           	jsr %a0@
    1a40:	4fef 0014      	lea %sp@(20),%sp
    1a44:	b7fc 0000 0400 	cmpal #1024,%a3
    1a4a:	6f16           	bles 1a62 <Exec_AddHead+0x78>
    1a4c:	2079 0000 0000 	moveal 0 <main>,%a0
    1a52:	2f08           	movel %a0,%sp@-
    1a54:	2f0b           	movel %a3,%sp@-
    1a56:	2068 fdec      	moveal %a0@(-532),%a0
    1a5a:	4e90           	jsr %a0@
    1a5c:	508f           	addql #8,%sp
    1a5e:	4a80           	tstl %d0
    1a60:	6628           	bnes 1a8a <Exec_AddHead+0xa0>
    1a62:	2079 0000 0000 	moveal 0 <main>,%a0
    1a68:	2068 0278      	moveal %a0@(632),%a0
    1a6c:	2f0b           	movel %a3,%sp@-
    1a6e:	4879 0000 0000 	pea 0 <main>
    1a74:	4878 003b      	pea 3b <pause+0x1b>
    1a78:	4879 0000 0000 	pea 0 <main>
    1a7e:	4879 0000 0000 	pea 0 <main>
    1a84:	2068 0004      	moveal %a0@(4),%a0
    1a88:	4e90           	jsr %a0@
    1a8a:	2493           	movel %a3@,%a2@
    1a8c:	254b 0004      	movel %a3,%a2@(4)
    1a90:	2053           	moveal %a3@,%a0
    1a92:	214a 0004      	movel %a2,%a0@(4)
    1a96:	268a           	movel %a2,%a3@
    1a98:	246e fff8      	moveal %fp@(-8),%a2
    1a9c:	266e fffc      	moveal %fp@(-4),%a3
    1aa0:	4e5e           	unlk %fp
    1aa2:	4e75           	rts

00001aa4 <Exec_AddLibrary>:
    1aa4:	4e56 0000      	linkw %fp,#0
    1aa8:	2f0b           	movel %a3,%sp@-
    1aaa:	2f0a           	movel %a2,%sp@-
    1aac:	266e 0008      	moveal %fp@(8),%a3
    1ab0:	246e 000c      	moveal %fp@(12),%a2
    1ab4:	b7fc 0000 0400 	cmpal #1024,%a3
    1aba:	6f10           	bles 1acc <Exec_AddLibrary+0x28>
    1abc:	2f0a           	movel %a2,%sp@-
    1abe:	2f0b           	movel %a3,%sp@-
    1ac0:	206a fdec      	moveal %a2@(-532),%a0
    1ac4:	4e90           	jsr %a0@
    1ac6:	508f           	addql #8,%sp
    1ac8:	4a80           	tstl %d0
    1aca:	6626           	bnes 1af2 <Exec_AddLibrary+0x4e>
    1acc:	206a 0278      	moveal %a2@(632),%a0
    1ad0:	2f0b           	movel %a3,%sp@-
    1ad2:	4879 0000 0000 	pea 0 <main>
    1ad8:	4878 0037      	pea 37 <pause+0x17>
    1adc:	4879 0000 0000 	pea 0 <main>
    1ae2:	4879 0000 0000 	pea 0 <main>
    1ae8:	2068 0004      	moveal %a0@(4),%a0
    1aec:	4e90           	jsr %a0@
    1aee:	4fef 0014      	lea %sp@(20),%sp
    1af2:	177c 0009 000c 	moveb #9,%a3@(12)
    1af8:	002b 0002 000e 	orib #2,%a3@(14)
    1afe:	2f0a           	movel %a2,%sp@-
    1b00:	2f0b           	movel %a3,%sp@-
    1b02:	206a fe58      	moveal %a2@(-424),%a0
    1b06:	4e90           	jsr %a0@
    1b08:	2f0a           	movel %a2,%sp@-
    1b0a:	206a ff7e      	moveal %a2@(-130),%a0
    1b0e:	4e90           	jsr %a0@
    1b10:	2f0b           	movel %a3,%sp@-
    1b12:	486a 017a      	pea %a2@(378)
    1b16:	206a fef4      	moveal %a2@(-268),%a0
    1b1a:	4e90           	jsr %a0@
    1b1c:	2f0a           	movel %a2,%sp@-
    1b1e:	206a ff78      	moveal %a2@(-136),%a0
    1b22:	4e90           	jsr %a0@
    1b24:	246e fff8      	moveal %fp@(-8),%a2
    1b28:	266e fffc      	moveal %fp@(-4),%a3
    1b2c:	4e5e           	unlk %fp
    1b2e:	4e75           	rts

00001b30 <Exec_AddMemHandler>:
    1b30:	4e56 0000      	linkw %fp,#0
    1b34:	2f0a           	movel %a2,%sp@-
    1b36:	2f02           	movel %d2,%sp@-
    1b38:	242e 0008      	movel %fp@(8),%d2
    1b3c:	246e 000c      	moveal %fp@(12),%a2
    1b40:	0c82 0000 0400 	cmpil #1024,%d2
    1b46:	6f10           	bles 1b58 <Exec_AddMemHandler+0x28>
    1b48:	2f0a           	movel %a2,%sp@-
    1b4a:	2f02           	movel %d2,%sp@-
    1b4c:	206a fdec      	moveal %a2@(-532),%a0
    1b50:	4e90           	jsr %a0@
    1b52:	508f           	addql #8,%sp
    1b54:	4a80           	tstl %d0
    1b56:	6626           	bnes 1b7e <Exec_AddMemHandler+0x4e>
    1b58:	206a 0278      	moveal %a2@(632),%a0
    1b5c:	2f02           	movel %d2,%sp@-
    1b5e:	4879 0000 0000 	pea 0 <main>
    1b64:	4878 0032      	pea 32 <pause+0x12>
    1b68:	4879 0000 0000 	pea 0 <main>
    1b6e:	4879 0000 0000 	pea 0 <main>
    1b74:	2068 0004      	moveal %a0@(4),%a0
    1b78:	4e90           	jsr %a0@
    1b7a:	4fef 0014      	lea %sp@(20),%sp
    1b7e:	2f0a           	movel %a2,%sp@-
    1b80:	206a ff7e      	moveal %a2@(-130),%a0
    1b84:	4e90           	jsr %a0@
    1b86:	2f02           	movel %d2,%sp@-
    1b88:	486a 0268      	pea %a2@(616)
    1b8c:	206a fef4      	moveal %a2@(-268),%a0
    1b90:	4e90           	jsr %a0@
    1b92:	2f0a           	movel %a2,%sp@-
    1b94:	206a ff78      	moveal %a2@(-136),%a0
    1b98:	4e90           	jsr %a0@
    1b9a:	242e fff8      	movel %fp@(-8),%d2
    1b9e:	246e fffc      	moveal %fp@(-4),%a2
    1ba2:	4e5e           	unlk %fp
    1ba4:	4e75           	rts

00001ba6 <Exec_AddMemList>:
    1ba6:	4e56 0000      	linkw %fp,#0
    1baa:	48e7 0038      	moveml %a2-%a4,%sp@-
    1bae:	226e 0008      	moveal %fp@(8),%a1
    1bb2:	246e 0014      	moveal %fp@(20),%a2
    1bb6:	266e 001c      	moveal %fp@(28),%a3
    1bba:	157c 000a 000c 	moveb #10,%a2@(12)
    1bc0:	156e 0013 000d 	moveb %fp@(19),%a2@(13)
    1bc6:	256e 0018 0008 	movel %fp@(24),%a2@(8)
    1bcc:	356e 000e 000e 	movew %fp@(14),%a2@(14)
    1bd2:	41ea 0020      	lea %a2@(32),%a0
    1bd6:	2548 0010      	movel %a0,%a2@(16)
    1bda:	4290           	clrl %a0@
    1bdc:	206a 0010      	moveal %a2@(16),%a0
    1be0:	49e9 ffe0      	lea %a1@(-32),%a4
    1be4:	214c 0004      	movel %a4,%a0@(4)
    1be8:	256a 0010 0014 	movel %a2@(16),%a2@(20)
    1bee:	d3ca           	addal %a2,%a1
    1bf0:	2549 0018      	movel %a1,%a2@(24)
    1bf4:	206a 0010      	moveal %a2@(16),%a0
    1bf8:	2568 0004 001c 	movel %a0@(4),%a2@(28)
    1bfe:	2f0b           	movel %a3,%sp@-
    1c00:	206b ff7e      	moveal %a3@(-130),%a0
    1c04:	4e90           	jsr %a0@
    1c06:	2f0a           	movel %a2,%sp@-
    1c08:	486b 0142      	pea %a3@(322)
    1c0c:	206b fef4      	moveal %a3@(-268),%a0
    1c10:	4e90           	jsr %a0@
    1c12:	2f0b           	movel %a3,%sp@-
    1c14:	206b ff78      	moveal %a3@(-136),%a0
    1c18:	4e90           	jsr %a0@
    1c1a:	4cee 1c00 fff4 	moveml %fp@(-12),%a2-%a4
    1c20:	4e5e           	unlk %fp
    1c22:	4e75           	rts

00001c24 <Exec_AddPort>:
    1c24:	4e56 0000      	linkw %fp,#0
    1c28:	2f0b           	movel %a3,%sp@-
    1c2a:	2f0a           	movel %a2,%sp@-
    1c2c:	246e 0008      	moveal %fp@(8),%a2
    1c30:	266e 000c      	moveal %fp@(12),%a3
    1c34:	b5fc 0000 0400 	cmpal #1024,%a2
    1c3a:	6f10           	bles 1c4c <Exec_AddPort+0x28>
    1c3c:	2f0b           	movel %a3,%sp@-
    1c3e:	2f0a           	movel %a2,%sp@-
    1c40:	206b fdec      	moveal %a3@(-532),%a0
    1c44:	4e90           	jsr %a0@
    1c46:	508f           	addql #8,%sp
    1c48:	4a80           	tstl %d0
    1c4a:	6626           	bnes 1c72 <Exec_AddPort+0x4e>
    1c4c:	206b 0278      	moveal %a3@(632),%a0
    1c50:	2f0a           	movel %a2,%sp@-
    1c52:	4879 0000 0000 	pea 0 <main>
    1c58:	4878 0034      	pea 34 <pause+0x14>
    1c5c:	4879 0000 0000 	pea 0 <main>
    1c62:	4879 0000 0000 	pea 0 <main>
    1c68:	2068 0004      	moveal %a0@(4),%a0
    1c6c:	4e90           	jsr %a0@
    1c6e:	4fef 0014      	lea %sp@(20),%sp
    1c72:	157c 0004 000c 	moveb #4,%a2@(12)
    1c78:	41ea 0014      	lea %a2@(20),%a0
    1c7c:	2548 001c      	movel %a0,%a2@(28)
    1c80:	42aa 0018      	clrl %a2@(24)
    1c84:	43ea 0018      	lea %a2@(24),%a1
    1c88:	2089           	movel %a1,%a0@
    1c8a:	2f0b           	movel %a3,%sp@-
    1c8c:	206b ff7e      	moveal %a3@(-130),%a0
    1c90:	4e90           	jsr %a0@
    1c92:	2f0a           	movel %a2,%sp@-
    1c94:	486b 0188      	pea %a3@(392)
    1c98:	206b fef4      	moveal %a3@(-268),%a0
    1c9c:	4e90           	jsr %a0@
    1c9e:	2f0b           	movel %a3,%sp@-
    1ca0:	206b ff78      	moveal %a3@(-136),%a0
    1ca4:	4e90           	jsr %a0@
    1ca6:	246e fff8      	moveal %fp@(-8),%a2
    1caa:	266e fffc      	moveal %fp@(-4),%a3
    1cae:	4e5e           	unlk %fp
    1cb0:	4e75           	rts

00001cb2 <Exec_AddResource>:
    1cb2:	4e56 0000      	linkw %fp,#0
    1cb6:	2f0b           	movel %a3,%sp@-
    1cb8:	2f0a           	movel %a2,%sp@-
    1cba:	266e 0008      	moveal %fp@(8),%a3
    1cbe:	246e 000c      	moveal %fp@(12),%a2
    1cc2:	b7fc 0000 0400 	cmpal #1024,%a3
    1cc8:	6f10           	bles 1cda <Exec_AddResource+0x28>
    1cca:	2f0a           	movel %a2,%sp@-
    1ccc:	2f0b           	movel %a3,%sp@-
    1cce:	206a fdec      	moveal %a2@(-532),%a0
    1cd2:	4e90           	jsr %a0@
    1cd4:	508f           	addql #8,%sp
    1cd6:	4a80           	tstl %d0
    1cd8:	6626           	bnes 1d00 <Exec_AddResource+0x4e>
    1cda:	206a 0278      	moveal %a2@(632),%a0
    1cde:	2f0b           	movel %a3,%sp@-
    1ce0:	4879 0000 0000 	pea 0 <main>
    1ce6:	4878 0030      	pea 30 <pause+0x10>
    1cea:	4879 0000 0000 	pea 0 <main>
    1cf0:	4879 0000 0000 	pea 0 <main>
    1cf6:	2068 0004      	moveal %a0@(4),%a0
    1cfa:	4e90           	jsr %a0@
    1cfc:	4fef 0014      	lea %sp@(20),%sp
    1d00:	177c 0008 000c 	moveb #8,%a3@(12)
    1d06:	2f0a           	movel %a2,%sp@-
    1d08:	206a ff7e      	moveal %a2@(-130),%a0
    1d0c:	4e90           	jsr %a0@
    1d0e:	2f0b           	movel %a3,%sp@-
    1d10:	486a 0150      	pea %a2@(336)
    1d14:	206a fef4      	moveal %a2@(-268),%a0
    1d18:	4e90           	jsr %a0@
    1d1a:	2f0a           	movel %a2,%sp@-
    1d1c:	206a ff78      	moveal %a2@(-136),%a0
    1d20:	4e90           	jsr %a0@
    1d22:	246e fff8      	moveal %fp@(-8),%a2
    1d26:	266e fffc      	moveal %fp@(-4),%a3
    1d2a:	4e5e           	unlk %fp
    1d2c:	4e75           	rts

00001d2e <Exec_AddSemaphore>:
    1d2e:	4e56 0000      	linkw %fp,#0
    1d32:	2f0a           	movel %a2,%sp@-
    1d34:	2f02           	movel %d2,%sp@-
    1d36:	242e 0008      	movel %fp@(8),%d2
    1d3a:	246e 000c      	moveal %fp@(12),%a2
    1d3e:	2f02           	movel %d2,%sp@-
    1d40:	206a fdd4      	moveal %a2@(-556),%a0
    1d44:	4e90           	jsr %a0@
    1d46:	2f0a           	movel %a2,%sp@-
    1d48:	206a ff7e      	moveal %a2@(-130),%a0
    1d4c:	4e90           	jsr %a0@
    1d4e:	2f02           	movel %d2,%sp@-
    1d50:	486a 0214      	pea %a2@(532)
    1d54:	206a fef4      	moveal %a2@(-268),%a0
    1d58:	4e90           	jsr %a0@
    1d5a:	2f0a           	movel %a2,%sp@-
    1d5c:	206a ff78      	moveal %a2@(-136),%a0
    1d60:	4e90           	jsr %a0@
    1d62:	242e fff8      	movel %fp@(-8),%d2
    1d66:	246e fffc      	moveal %fp@(-4),%a2
    1d6a:	4e5e           	unlk %fp
    1d6c:	4e75           	rts

00001d6e <Exec_AddTail>:
    1d6e:	4e56 0000      	linkw %fp,#0
    1d72:	2f0b           	movel %a3,%sp@-
    1d74:	2f0a           	movel %a2,%sp@-
    1d76:	246e 0008      	moveal %fp@(8),%a2
    1d7a:	266e 000c      	moveal %fp@(12),%a3
    1d7e:	b5fc 0000 0400 	cmpal #1024,%a2
    1d84:	6f16           	bles 1d9c <Exec_AddTail+0x2e>
    1d86:	2079 0000 0000 	moveal 0 <main>,%a0
    1d8c:	2f08           	movel %a0,%sp@-
    1d8e:	2f0a           	movel %a2,%sp@-
    1d90:	2068 fdec      	moveal %a0@(-532),%a0
    1d94:	4e90           	jsr %a0@
    1d96:	508f           	addql #8,%sp
    1d98:	4a80           	tstl %d0
    1d9a:	6628           	bnes 1dc4 <Exec_AddTail+0x56>
    1d9c:	2079 0000 0000 	moveal 0 <main>,%a0
    1da2:	2068 0278      	moveal %a0@(632),%a0
    1da6:	2f0a           	movel %a2,%sp@-
    1da8:	4879 0000 0000 	pea 0 <main>
    1dae:	4878 0038      	pea 38 <pause+0x18>
    1db2:	4879 0000 0000 	pea 0 <main>
    1db8:	4879 0000 0000 	pea 0 <main>
    1dbe:	2068 0004      	moveal %a0@(4),%a0
    1dc2:	4e90           	jsr %a0@
    1dc4:	43ea 0004      	lea %a2@(4),%a1
    1dc8:	2689           	movel %a1,%a3@
    1dca:	276a 0008 0004 	movel %a2@(8),%a3@(4)
    1dd0:	206a 0008      	moveal %a2@(8),%a0
    1dd4:	208b           	movel %a3,%a0@
    1dd6:	254b 0008      	movel %a3,%a2@(8)
    1dda:	246e fff8      	moveal %fp@(-8),%a2
    1dde:	266e fffc      	moveal %fp@(-4),%a3
    1de2:	4e5e           	unlk %fp
    1de4:	4e75           	rts

00001de6 <Exec_AddTask>:
    1de6:	4e56 0000      	linkw %fp,#0
    1dea:	48e7 2038      	moveml %d2/%a2-%a4,%sp@-
    1dee:	266e 0008      	moveal %fp@(8),%a3
    1df2:	242e 0010      	movel %fp@(16),%d2
    1df6:	286e 0014      	moveal %fp@(20),%a4
    1dfa:	b7fc 0000 0400 	cmpal #1024,%a3
    1e00:	6f10           	bles 1e12 <Exec_AddTask+0x2c>
    1e02:	2f0c           	movel %a4,%sp@-
    1e04:	2f0b           	movel %a3,%sp@-
    1e06:	206c fdec      	moveal %a4@(-532),%a0
    1e0a:	4e90           	jsr %a0@
    1e0c:	508f           	addql #8,%sp
    1e0e:	4a80           	tstl %d0
    1e10:	6626           	bnes 1e38 <Exec_AddTask+0x52>
    1e12:	206c 0278      	moveal %a4@(632),%a0
    1e16:	2f0b           	movel %a3,%sp@-
    1e18:	4879 0000 0000 	pea 0 <main>
    1e1e:	4878 0060      	pea 60 <pause+0x40>
    1e22:	4879 0000 0000 	pea 0 <main>
    1e28:	4879 0000 0000 	pea 0 <main>
    1e2e:	2068 0004      	moveal %a0@(4),%a0
    1e32:	4e90           	jsr %a0@
    1e34:	4fef 0014      	lea %sp@(20),%sp
    1e38:	4a2b 000c      	tstb %a3@(12)
    1e3c:	6606           	bnes 1e44 <Exec_AddTask+0x5e>
    1e3e:	177c 0001 000c 	moveb #1,%a3@(12)
    1e44:	4aab 0008      	tstl %a3@(8)
    1e48:	6608           	bnes 1e52 <Exec_AddTask+0x6c>
    1e4a:	277c 0000 0000 	movel #0,%a3@(8)
    1e50:	0008 
    1e52:	50eb 0010      	st %a3@(16)
    1e56:	50eb 0011      	st %a3@(17)
    1e5a:	4aab 0012      	tstl %a3@(18)
    1e5e:	6606           	bnes 1e66 <Exec_AddTask+0x80>
    1e60:	276c 013c 0012 	movel %a4@(316),%a3@(18)
    1e66:	4aab 0032      	tstl %a3@(50)
    1e6a:	6606           	bnes 1e72 <Exec_AddTask+0x8c>
    1e6c:	276c 0130 0032 	movel %a4@(304),%a3@(50)
    1e72:	102b 000e      	moveb %a3@(14),%d0
    1e76:	0000 0008      	orib #8,%d0
    1e7a:	1740 000e      	moveb %d0,%a3@(14)
    1e7e:	0800 0003      	btst #3,%d0
    1e82:	6778           	beqs 1efc <Exec_AddTask+0x116>
    1e84:	2f0c           	movel %a4,%sp@-
    1e86:	2f3c 0001 0000 	movel #65536,%sp@-
    1e8c:	4878 005e      	pea 5e <pause+0x3e>
    1e90:	206c fd56      	moveal %a4@(-682),%a0
    1e94:	4e90           	jsr %a0@
    1e96:	2448           	moveal %a0,%a2
    1e98:	274a 0022      	movel %a2,%a3@(34)
    1e9c:	4fef 000c      	lea %sp@(12),%sp
    1ea0:	6606           	bnes 1ea8 <Exec_AddTask+0xc2>
    1ea2:	91c8           	subal %a0,%a0
    1ea4:	6000 0140      	braw 1fe6 <Exec_AddTask+0x200>
    1ea8:	2f0c           	movel %a4,%sp@-
    1eaa:	42a7           	clrl %sp@-
    1eac:	206c fedc      	moveal %a4@(-292),%a0
    1eb0:	4e90           	jsr %a0@
    1eb2:	2548 0014      	movel %a0,%a2@(20)
    1eb6:	41ea 001c      	lea %a2@(28),%a0
    1eba:	2548 0024      	movel %a0,%a2@(36)
    1ebe:	42aa 0020      	clrl %a2@(32)
    1ec2:	43ea 0020      	lea %a2@(32),%a1
    1ec6:	2089           	movel %a1,%a0@
    1ec8:	41ea 0048      	lea %a2@(72),%a0
    1ecc:	2548 0050      	movel %a0,%a2@(80)
    1ed0:	42aa 004c      	clrl %a2@(76)
    1ed4:	43ea 004c      	lea %a2@(76),%a1
    1ed8:	2089           	movel %a1,%a0@
    1eda:	422a 0042      	clrb %a2@(66)
    1ede:	157c 0004 0040 	moveb #4,%a2@(64)
    1ee4:	254b 0044      	movel %a3,%a2@(68)
    1ee8:	157c 0001 0043 	moveb #1,%a2@(67)
    1eee:	356c 0140 0028 	movew %a4@(320),%a2@(40)
    1ef4:	426a 002a      	clrw %a2@(42)
    1ef8:	508f           	addql #8,%sp
    1efa:	600a           	bras 1f06 <Exec_AddTask+0x120>
    1efc:	376c 0140 0022 	movew %a4@(320),%a3@(34)
    1f02:	426b 0024      	clrw %a3@(36)
    1f06:	4aab 0036      	tstl %a3@(54)
    1f0a:	6606           	bnes 1f12 <Exec_AddTask+0x12c>
    1f0c:	276b 003e 0036 	movel %a3@(62),%a3@(54)
    1f12:	206b 003a      	moveal %a3@(58),%a0
    1f16:	70f0           	moveq #-16,%d0
    1f18:	d0ab 0036      	addl %a3@(54),%d0
    1f1c:	b088           	cmpl %a0,%d0
    1f1e:	6508           	bcss 1f28 <Exec_AddTask+0x142>
    1f20:	10fc 00e1      	moveb #-31,%a0@+
    1f24:	b088           	cmpl %a0,%d0
    1f26:	64f8           	bccs 1f20 <Exec_AddTask+0x13a>
    1f28:	4a82           	tstl %d2
    1f2a:	6604           	bnes 1f30 <Exec_AddTask+0x14a>
    1f2c:	242c 0138      	movel %a4@(312),%d2
    1f30:	2f0c           	movel %a4,%sp@-
    1f32:	2f02           	movel %d2,%sp@-
    1f34:	2f2e 000c      	movel %fp@(12),%sp@-
    1f38:	2f0b           	movel %a3,%sp@-
    1f3a:	206c ffde      	moveal %a4@(-34),%a0
    1f3e:	4e90           	jsr %a0@
    1f40:	4fef 0010      	lea %sp@(16),%sp
    1f44:	b0fc 0000      	cmpaw #0,%a0
    1f48:	6612           	bnes 1f5c <Exec_AddTask+0x176>
    1f4a:	2f0c           	movel %a4,%sp@-
    1f4c:	2f2b 0022      	movel %a3@(34),%sp@-
    1f50:	206c fd50      	moveal %a4@(-688),%a0
    1f54:	4e90           	jsr %a0@
    1f56:	91c8           	subal %a0,%a0
    1f58:	6000 008c      	braw 1fe6 <Exec_AddTask+0x200>
    1f5c:	4aab 0042      	tstl %a3@(66)
    1f60:	6706           	beqs 1f68 <Exec_AddTask+0x182>
    1f62:	002b 0040 000e 	orib #64,%a3@(14)
    1f68:	4aab 0046      	tstl %a3@(70)
    1f6c:	6706           	beqs 1f74 <Exec_AddTask+0x18e>
    1f6e:	002b 0080 000e 	orib #-128,%a3@(14)
    1f74:	2f0c           	movel %a4,%sp@-
    1f76:	206c ff8a      	moveal %a4@(-118),%a0
    1f7a:	4e90           	jsr %a0@
    1f7c:	177c 0003 000f 	moveb #3,%a3@(15)
    1f82:	2f0b           	movel %a3,%sp@-
    1f84:	45ec 0196      	lea %a4@(406),%a2
    1f88:	2f0a           	movel %a2,%sp@-
    1f8a:	206c fef4      	moveal %a4@(-268),%a0
    1f8e:	4e90           	jsr %a0@
    1f90:	206c 0114      	moveal %a4@(276),%a0
    1f94:	4fef 000c      	lea %sp@(12),%sp
    1f98:	122b 000d      	moveb %a3@(13),%d1
    1f9c:	b228 000d      	cmpb %a0@(13),%d1
    1fa0:	6f3a           	bles 1fdc <Exec_AddTask+0x1f6>
    1fa2:	0c28 0002 000f 	cmpib #2,%a0@(15)
    1fa8:	6632           	bnes 1fdc <Exec_AddTask+0x1f6>
    1faa:	4a2c 0127      	tstb %a4@(295)
    1fae:	6c06           	bges 1fb6 <Exec_AddTask+0x1d0>
    1fb0:	4a2c 0126      	tstb %a4@(294)
    1fb4:	6f08           	bles 1fbe <Exec_AddTask+0x1d8>
    1fb6:	006c 0080 012a 	oriw #128,%a4@(298)
    1fbc:	601e           	bras 1fdc <Exec_AddTask+0x1f6>
    1fbe:	117c 0003 000f 	moveb #3,%a0@(15)
    1fc4:	2f2c 0114      	movel %a4@(276),%sp@-
    1fc8:	2f0a           	movel %a2,%sp@-
    1fca:	206c fef4      	moveal %a4@(-268),%a0
    1fce:	4e90           	jsr %a0@
    1fd0:	2f0c           	movel %a4,%sp@-
    1fd2:	206c ffcc      	moveal %a4@(-52),%a0
    1fd6:	4e90           	jsr %a0@
    1fd8:	4fef 000c      	lea %sp@(12),%sp
    1fdc:	2f0c           	movel %a4,%sp@-
    1fde:	206c ff84      	moveal %a4@(-124),%a0
    1fe2:	4e90           	jsr %a0@
    1fe4:	204b           	moveal %a3,%a0
    1fe6:	2008           	movel %a0,%d0
    1fe8:	4cee 1c04 fff0 	moveml %fp@(-16),%d2/%a2-%a4
    1fee:	4e5e           	unlk %fp
    1ff0:	4e75           	rts

00001ff2 <Exec_TaskFinaliser>:
    1ff2:	4e56 0000      	linkw %fp,#0
    1ff6:	2079 0000 0000 	moveal 0 <main>,%a0
    1ffc:	2f08           	movel %a0,%sp@-
    1ffe:	2f28 0114      	movel %a0@(276),%sp@-
    2002:	2068 fee2      	moveal %a0@(-286),%a0
    2006:	4e90           	jsr %a0@
    2008:	4e5e           	unlk %fp
    200a:	4e75           	rts

0000200c <Exec_Alert>:
    200c:	4e56 0000      	linkw %fp,#0
    2010:	48e7 3038      	moveml %d2-%d3/%a2-%a4,%sp@-
    2014:	242e 0008      	movel %fp@(8),%d2
    2018:	286e 000c      	moveal %fp@(12),%a4
    201c:	2f0c           	movel %a4,%sp@-
    201e:	42a7           	clrl %sp@-
    2020:	206c fedc      	moveal %a4@(-292),%a0
    2024:	4e90           	jsr %a0@
    2026:	2648           	moveal %a0,%a3
    2028:	3f02           	movew %d2,%sp@-
    202a:	4267           	clrw %sp@-
    202c:	2002           	movel %d2,%d0
    202e:	4240           	clrw %d0
    2030:	4840           	swap %d0
    2032:	2f00           	movel %d0,%sp@-
    2034:	4879 0000 0000 	pea 0 <main>
    203a:	45f9 0000 0000 	lea 0 <main>,%a2
    2040:	4e92           	jsr %a2@
    2042:	4fef 0014      	lea %sp@(20),%sp
    2046:	4a82           	tstl %d2
    2048:	6c08           	bges 2052 <Exec_Alert+0x46>
    204a:	4879 0000 0000 	pea 0 <main>
    2050:	6006           	bras 2058 <Exec_Alert+0x4c>
    2052:	4879 0000 0000 	pea 0 <main>
    2058:	4e92           	jsr %a2@
    205a:	588f           	addql #4,%sp
    205c:	2002           	movel %d2,%d0
    205e:	4240           	clrw %d0
    2060:	4840           	swap %d0
    2062:	e048           	lsrw #8,%d0
    2064:	767f           	moveq #127,%d3
    2066:	c083           	andl %d3,%d0
    2068:	7601           	moveq #1,%d3
    206a:	b680           	cmpl %d0,%d3
    206c:	6700 00a2      	beqw 2110 <Exec_Alert+0x104>
    2070:	620c           	bhis 207e <Exec_Alert+0x72>
    2072:	7602           	moveq #2,%d3
    2074:	b680           	cmpl %d0,%d3
    2076:	6700 00e4      	beqw 215c <Exec_Alert+0x150>
    207a:	6000 00e8      	braw 2164 <Exec_Alert+0x158>
    207e:	2002           	movel %d2,%d0
    2080:	4240           	clrw %d0
    2082:	4840           	swap %d0
    2084:	4a00           	tstb %d0
    2086:	6636           	bnes 20be <Exec_Alert+0xb2>
    2088:	4879 0000 0000 	pea 0 <main>
    208e:	45f9 0000 0000 	lea 0 <main>,%a2
    2094:	4e92           	jsr %a2@
    2096:	7000           	moveq #0,%d0
    2098:	4640           	notw %d0
    209a:	c082           	andl %d2,%d0
    209c:	5580           	subql #2,%d0
    209e:	588f           	addql #4,%sp
    20a0:	761d           	moveq #29,%d3
    20a2:	b680           	cmpl %d0,%d3
    20a4:	650c           	bcss 20b2 <Exec_Alert+0xa6>
    20a6:	e588           	lsll #2,%d0
    20a8:	41f9 0000 0000 	lea 0 <main>,%a0
    20ae:	6000 0094      	braw 2144 <Exec_Alert+0x138>
    20b2:	4879 0000 0000 	pea 0 <main>
    20b8:	4e92           	jsr %a2@
    20ba:	6000 00b4      	braw 2170 <Exec_Alert+0x164>
    20be:	7600           	moveq #0,%d3
    20c0:	4603           	notb %d3
    20c2:	c083           	andl %d3,%d0
    20c4:	760b           	moveq #11,%d3
    20c6:	b680           	cmpl %d0,%d3
    20c8:	6500 00a8      	bcsw 2172 <Exec_Alert+0x166>
    20cc:	5380           	subql #1,%d0
    20ce:	e588           	lsll #2,%d0
    20d0:	41f9 0000 0000 	lea 0 <main>,%a0
    20d6:	2f30 0800      	movel %a0@(00000000,%d0:l),%sp@-
    20da:	4879 0000 0000 	pea 0 <main>
    20e0:	45f9 0000 0000 	lea 0 <main>,%a2
    20e6:	4e92           	jsr %a2@
    20e8:	7000           	moveq #0,%d0
    20ea:	4640           	notw %d0
    20ec:	c082           	andl %d2,%d0
    20ee:	0680 ffff 7fff 	addil #-32769,%d0
    20f4:	508f           	addql #8,%sp
    20f6:	7634           	moveq #52,%d3
    20f8:	b680           	cmpl %d0,%d3
    20fa:	650a           	bcss 2106 <Exec_Alert+0xfa>
    20fc:	e588           	lsll #2,%d0
    20fe:	41f9 0000 0000 	lea 0 <main>,%a0
    2104:	603e           	bras 2144 <Exec_Alert+0x138>
    2106:	4879 0000 0000 	pea 0 <main>
    210c:	4e92           	jsr %a2@
    210e:	6060           	bras 2170 <Exec_Alert+0x164>
    2110:	4879 0000 0000 	pea 0 <main>
    2116:	45f9 0000 0000 	lea 0 <main>,%a2
    211c:	4e92           	jsr %a2@
    211e:	2002           	movel %d2,%d0
    2120:	4240           	clrw %d0
    2122:	4840           	swap %d0
    2124:	588f           	addql #4,%sp
    2126:	4a00           	tstb %d0
    2128:	662a           	bnes 2154 <Exec_Alert+0x148>
    212a:	4a42           	tstw %d2
    212c:	6726           	beqs 2154 <Exec_Alert+0x148>
    212e:	7000           	moveq #0,%d0
    2130:	4640           	notw %d0
    2132:	c082           	andl %d2,%d0
    2134:	7610           	moveq #16,%d3
    2136:	b680           	cmpl %d0,%d3
    2138:	651a           	bcss 2154 <Exec_Alert+0x148>
    213a:	5380           	subql #1,%d0
    213c:	e588           	lsll #2,%d0
    213e:	41f9 0000 0000 	lea 0 <main>,%a0
    2144:	2f30 0800      	movel %a0@(00000000,%d0:l),%sp@-
    2148:	4879 0000 0000 	pea 0 <main>
    214e:	4e92           	jsr %a2@
    2150:	508f           	addql #8,%sp
    2152:	601e           	bras 2172 <Exec_Alert+0x166>
    2154:	4879 0000 0000 	pea 0 <main>
    215a:	600e           	bras 216a <Exec_Alert+0x15e>
    215c:	4879 0000 0000 	pea 0 <main>
    2162:	6006           	bras 216a <Exec_Alert+0x15e>
    2164:	4879 0000 0000 	pea 0 <main>
    216a:	61ff 0000 0000 	bsrl 216c <Exec_Alert+0x160>
    2170:	588f           	addql #4,%sp
    2172:	223c 0000 0000 	movel #0,%d1
    2178:	b6fc 0000      	cmpaw #0,%a3
    217c:	6708           	beqs 2186 <Exec_Alert+0x17a>
    217e:	202b 0008      	movel %a3@(8),%d0
    2182:	6702           	beqs 2186 <Exec_Alert+0x17a>
    2184:	2200           	movel %d0,%d1
    2186:	2f01           	movel %d1,%sp@-
    2188:	2f0b           	movel %a3,%sp@-
    218a:	4879 0000 0000 	pea 0 <main>
    2190:	61ff 0000 0000 	bsrl 2192 <Exec_Alert+0x186>
    2196:	4fef 000c      	lea %sp@(12),%sp
    219a:	4a82           	tstl %d2
    219c:	6c08           	bges 21a6 <Exec_Alert+0x19a>
    219e:	2f0c           	movel %a4,%sp@-
    21a0:	206c fd2c      	moveal %a4@(-724),%a0
    21a4:	4e90           	jsr %a0@
    21a6:	4cee 1c0c ffec 	moveml %fp@(-20),%d2-%d3/%a2-%a4
    21ac:	4e5e           	unlk %fp
    21ae:	4e75           	rts

000021b0 <Exec_AllocAbs>:
    21b0:	4e56 fffc      	linkw %fp,#-4
    21b4:	48e7 3c3c      	moveml %d2-%d5/%a2-%a5,%sp@-
    21b8:	282e 0008      	movel %fp@(8),%d4
    21bc:	242e 000c      	movel %fp@(12),%d2
    21c0:	226e 0010      	moveal %fp@(16),%a1
    21c4:	4a84           	tstl %d4
    21c6:	6700 00ee      	beqw 22b6 <Exec_AllocAbs+0x106>
    21ca:	7007           	moveq #7,%d0
    21cc:	c082           	andl %d2,%d0
    21ce:	2a44           	moveal %d4,%a5
    21d0:	4bf5 0807      	lea %a5@(00000007,%d0:l),%a5
    21d4:	200d           	movel %a5,%d0
    21d6:	78f8           	moveq #-8,%d4
    21d8:	c880           	andl %d0,%d4
    21da:	76f8           	moveq #-8,%d3
    21dc:	c682           	andl %d2,%d3
    21de:	2403           	movel %d3,%d2
    21e0:	2643           	moveal %d3,%a3
    21e2:	d7c4           	addal %d4,%a3
    21e4:	2f09           	movel %a1,%sp@-
    21e6:	2069 ff7e      	moveal %a1@(-130),%a0
    21ea:	2d49 fffc      	movel %a1,%fp@(-4)
    21ee:	4e90           	jsr %a0@
    21f0:	226e fffc      	moveal %fp@(-4),%a1
    21f4:	2869 0142      	moveal %a1@(322),%a4
    21f8:	588f           	addql #4,%sp
    21fa:	4a94           	tstl %a4@
    21fc:	6700 00b0      	beqw 22ae <Exec_AllocAbs+0xfe>
    2200:	b4ac 0014      	cmpl %a4@(20),%d2
    2204:	6500 00a0      	bcsw 22a6 <Exec_AllocAbs+0xf6>
    2208:	b4ac 0018      	cmpl %a4@(24),%d2
    220c:	6400 0098      	bccw 22a6 <Exec_AllocAbs+0xf6>
    2210:	7410           	moveq #16,%d2
    2212:	d48c           	addl %a4,%d2
    2214:	2a42           	moveal %d2,%a5
    2216:	2455           	moveal %a5@,%a2
    2218:	b4fc 0000      	cmpaw #0,%a2
    221c:	6700 0090      	beqw 22ae <Exec_AllocAbs+0xfe>
    2220:	222a 0004      	movel %a2@(4),%d1
    2224:	200a           	movel %a2,%d0
    2226:	8081           	orl %d1,%d0
    2228:	7a07           	moveq #7,%d5
    222a:	c085           	andl %d5,%d0
    222c:	660e           	bnes 223c <Exec_AllocAbs+0x8c>
    222e:	200a           	movel %a2,%d0
    2230:	d081           	addl %d1,%d0
    2232:	2212           	movel %a2@,%d1
    2234:	b280           	cmpl %d0,%d1
    2236:	621c           	bhis 2254 <Exec_AllocAbs+0xa4>
    2238:	4a81           	tstl %d1
    223a:	6718           	beqs 2254 <Exec_AllocAbs+0xa4>
    223c:	2f09           	movel %a1,%sp@-
    223e:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    2244:	2069 ff96      	moveal %a1@(-106),%a0
    2248:	2d49 fffc      	movel %a1,%fp@(-4)
    224c:	4e90           	jsr %a0@
    224e:	508f           	addql #8,%sp
    2250:	226e fffc      	moveal %fp@(-4),%a1
    2254:	200a           	movel %a2,%d0
    2256:	d0aa 0004      	addl %a2@(4),%d0
    225a:	b7c0           	cmpal %d0,%a3
    225c:	623a           	bhis 2298 <Exec_AllocAbs+0xe8>
    225e:	b68a           	cmpl %a2,%d3
    2260:	6536           	bcss 2298 <Exec_AllocAbs+0xe8>
    2262:	b7c0           	cmpal %d0,%a3
    2264:	6710           	beqs 2276 <Exec_AllocAbs+0xc6>
    2266:	2692           	movel %a2@,%a3@
    2268:	200a           	movel %a2,%d0
    226a:	d0aa 0004      	addl %a2@(4),%d0
    226e:	908b           	subl %a3,%d0
    2270:	2740 0004      	movel %d0,%a3@(4)
    2274:	248b           	movel %a3,%a2@
    2276:	b68a           	cmpl %a2,%d3
    2278:	670a           	beqs 2284 <Exec_AllocAbs+0xd4>
    227a:	2a43           	moveal %d3,%a5
    227c:	9bca           	subal %a2,%a5
    227e:	254d 0004      	movel %a5,%a2@(4)
    2282:	6004           	bras 2288 <Exec_AllocAbs+0xd8>
    2284:	2a42           	moveal %d2,%a5
    2286:	2a92           	movel %a2@,%a5@
    2288:	99ac 001c      	subl %d4,%a4@(28)
    228c:	2f09           	movel %a1,%sp@-
    228e:	2069 ff78      	moveal %a1@(-136),%a0
    2292:	4e90           	jsr %a0@
    2294:	2043           	moveal %d3,%a0
    2296:	6020           	bras 22b8 <Exec_AllocAbs+0x108>
    2298:	240a           	movel %a2,%d2
    229a:	2452           	moveal %a2@,%a2
    229c:	b4fc 0000      	cmpaw #0,%a2
    22a0:	6600 ff7e      	bnew 2220 <Exec_AllocAbs+0x70>
    22a4:	6008           	bras 22ae <Exec_AllocAbs+0xfe>
    22a6:	2854           	moveal %a4@,%a4
    22a8:	4a94           	tstl %a4@
    22aa:	6600 ff54      	bnew 2200 <Exec_AllocAbs+0x50>
    22ae:	2f09           	movel %a1,%sp@-
    22b0:	2069 ff78      	moveal %a1@(-136),%a0
    22b4:	4e90           	jsr %a0@
    22b6:	91c8           	subal %a0,%a0
    22b8:	2008           	movel %a0,%d0
    22ba:	4cee 3c3c ffdc 	moveml %fp@(-36),%d2-%d5/%a2-%a5
    22c0:	4e5e           	unlk %fp
    22c2:	4e75           	rts

000022c4 <Exec_Allocate>:
    22c4:	4e56 0000      	linkw %fp,#0
    22c8:	48e7 3038      	moveml %d2-%d3/%a2-%a4,%sp@-
    22cc:	286e 0008      	moveal %fp@(8),%a4
    22d0:	242e 000c      	movel %fp@(12),%d2
    22d4:	266e 0010      	moveal %fp@(16),%a3
    22d8:	b9fc 0000 0400 	cmpal #1024,%a4
    22de:	6f10           	bles 22f0 <Exec_Allocate+0x2c>
    22e0:	2f0b           	movel %a3,%sp@-
    22e2:	2f0c           	movel %a4,%sp@-
    22e4:	206b fdec      	moveal %a3@(-532),%a0
    22e8:	4e90           	jsr %a0@
    22ea:	508f           	addql #8,%sp
    22ec:	4a80           	tstl %d0
    22ee:	6622           	bnes 2312 <Exec_Allocate+0x4e>
    22f0:	2f0c           	movel %a4,%sp@-
    22f2:	4879 0000 0000 	pea 0 <main>
    22f8:	4878 0054      	pea 54 <pause+0x34>
    22fc:	4879 0000 0000 	pea 0 <main>
    2302:	4879 0000 0000 	pea 0 <main>
    2308:	61ff 0000 0000 	bsrl 230a <Exec_Allocate+0x46>
    230e:	4fef 0014      	lea %sp@(20),%sp
    2312:	4a82           	tstl %d2
    2314:	6700 0088      	beqw 239e <Exec_Allocate+0xda>
    2318:	2002           	movel %d2,%d0
    231a:	5e80           	addql #7,%d0
    231c:	74f8           	moveq #-8,%d2
    231e:	c480           	andl %d0,%d2
    2320:	b4ac 001c      	cmpl %a4@(28),%d2
    2324:	6278           	bhis 239e <Exec_Allocate+0xda>
    2326:	45ec 0010      	lea %a4@(16),%a2
    232a:	2252           	moveal %a2@,%a1
    232c:	b2fc 0000      	cmpaw #0,%a1
    2330:	676c           	beqs 239e <Exec_Allocate+0xda>
    2332:	2229 0004      	movel %a1@(4),%d1
    2336:	2009           	movel %a1,%d0
    2338:	8081           	orl %d1,%d0
    233a:	7607           	moveq #7,%d3
    233c:	c083           	andl %d3,%d0
    233e:	6650           	bnes 2390 <Exec_Allocate+0xcc>
    2340:	b481           	cmpl %d1,%d2
    2342:	6238           	bhis 237c <Exec_Allocate+0xb8>
    2344:	6604           	bnes 234a <Exec_Allocate+0x86>
    2346:	2491           	movel %a1@,%a2@
    2348:	6014           	bras 235e <Exec_Allocate+0x9a>
    234a:	41f1 2800      	lea %a1@(00000000,%d2:l),%a0
    234e:	2488           	movel %a0,%a2@
    2350:	2448           	moveal %a0,%a2
    2352:	2491           	movel %a1@,%a2@
    2354:	2629 0004      	movel %a1@(4),%d3
    2358:	9682           	subl %d2,%d3
    235a:	2543 0004      	movel %d3,%a2@(4)
    235e:	95ac 001c      	subl %d2,%a4@(28)
    2362:	2049           	moveal %a1,%a0
    2364:	2202           	movel %d2,%d1
    2366:	e489           	lsrl #2,%d1
    2368:	6006           	bras 2370 <Exec_Allocate+0xac>
    236a:	20fc c0de dbad 	movel #-1059136595,%a0@+
    2370:	2001           	movel %d1,%d0
    2372:	5381           	subql #1,%d1
    2374:	4a80           	tstl %d0
    2376:	66f2           	bnes 236a <Exec_Allocate+0xa6>
    2378:	2049           	moveal %a1,%a0
    237a:	6024           	bras 23a0 <Exec_Allocate+0xdc>
    237c:	2449           	moveal %a1,%a2
    237e:	2251           	moveal %a1@,%a1
    2380:	b2fc 0000      	cmpaw #0,%a1
    2384:	6718           	beqs 239e <Exec_Allocate+0xda>
    2386:	200a           	movel %a2,%d0
    2388:	d0aa 0004      	addl %a2@(4),%d0
    238c:	b089           	cmpl %a1,%d0
    238e:	65a2           	bcss 2332 <Exec_Allocate+0x6e>
    2390:	2f0b           	movel %a3,%sp@-
    2392:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    2398:	206b ff96      	moveal %a3@(-106),%a0
    239c:	4e90           	jsr %a0@
    239e:	91c8           	subal %a0,%a0
    23a0:	2008           	movel %a0,%d0
    23a2:	4cee 1c0c ffec 	moveml %fp@(-20),%d2-%d3/%a2-%a4
    23a8:	4e5e           	unlk %fp
    23aa:	4e75           	rts

000023ac <Exec_AllocEntry>:
    23ac:	4e56 0000      	linkw %fp,#0
    23b0:	48e7 3e38      	moveml %d2-%d6/%a2-%a4,%sp@-
    23b4:	266e 0008      	moveal %fp@(8),%a3
    23b8:	286e 000c      	moveal %fp@(12),%a4
    23bc:	4280           	clrl %d0
    23be:	302b 000e      	movew %a3@(14),%d0
    23c2:	e788           	lsll #3,%d0
    23c4:	7c10           	moveq #16,%d6
    23c6:	dc80           	addl %d0,%d6
    23c8:	2f0c           	movel %a4,%sp@-
    23ca:	4878 0001      	pea 1 <main+0x1>
    23ce:	2f06           	movel %d6,%sp@-
    23d0:	206c ff3c      	moveal %a4@(-196),%a0
    23d4:	4e90           	jsr %a0@
    23d6:	2448           	moveal %a0,%a2
    23d8:	4fef 000c      	lea %sp@(12),%sp
    23dc:	b4fc 0000      	cmpaw #0,%a2
    23e0:	671a           	beqs 23fc <Exec_AllocEntry+0x50>
    23e2:	6c12           	bges 23f6 <Exec_AllocEntry+0x4a>
    23e4:	2f0c           	movel %a4,%sp@-
    23e6:	2f06           	movel %d6,%sp@-
    23e8:	2f0a           	movel %a2,%sp@-
    23ea:	206c ff30      	moveal %a4@(-208),%a0
    23ee:	4e90           	jsr %a0@
    23f0:	95ca           	subal %a2,%a2
    23f2:	4fef 000c      	lea %sp@(12),%sp
    23f6:	b4fc 0000      	cmpaw #0,%a2
    23fa:	660a           	bnes 2406 <Exec_AllocEntry+0x5a>
    23fc:	207c 8000 0001 	moveal #-2147483647,%a0
    2402:	6000 0094      	braw 2498 <Exec_AllocEntry+0xec>
    2406:	356b 000e 000e 	movew %a3@(14),%a2@(14)
    240c:	422a 000c      	clrb %a2@(12)
    2410:	422a 000d      	clrb %a2@(13)
    2414:	42aa 0008      	clrl %a2@(8)
    2418:	7800           	moveq #0,%d4
    241a:	4a6b 000e      	tstw %a3@(14)
    241e:	6776           	beqs 2496 <Exec_AllocEntry+0xea>
    2420:	2604           	movel %d4,%d3
    2422:	2404           	movel %d4,%d2
    2424:	2f0c           	movel %a4,%sp@-
    2426:	2a04           	movel %d4,%d5
    2428:	2f33 2810      	movel %a3@(00000010,%d2:l),%sp@-
    242c:	2f33 2814      	movel %a3@(00000014,%d2:l),%sp@-
    2430:	206c ff3c      	moveal %a4@(-196),%a0
    2434:	4e90           	jsr %a0@
    2436:	2588 2810      	movel %a0,%a2@(00000010,%d2:l)
    243a:	4fef 000c      	lea %sp@(12),%sp
    243e:	6644           	bnes 2484 <Exec_AllocEntry+0xd8>
    2440:	223c 8000 0000 	movel #-2147483648,%d1
    2446:	82b3 2810      	orl %a3@(00000010,%d2:l),%d1
    244a:	2641           	moveal %d1,%a3
    244c:	5384           	subql #1,%d4
    244e:	4a85           	tstl %d5
    2450:	6722           	beqs 2474 <Exec_AllocEntry+0xc8>
    2452:	2604           	movel %d4,%d3
    2454:	e78b           	lsll #3,%d3
    2456:	2f0c           	movel %a4,%sp@-
    2458:	2404           	movel %d4,%d2
    245a:	2f32 3814      	movel %a2@(00000014,%d3:l),%sp@-
    245e:	2f32 3810      	movel %a2@(00000010,%d3:l),%sp@-
    2462:	206c ff30      	moveal %a4@(-208),%a0
    2466:	4e90           	jsr %a0@
    2468:	4fef 000c      	lea %sp@(12),%sp
    246c:	5183           	subql #8,%d3
    246e:	5384           	subql #1,%d4
    2470:	4a82           	tstl %d2
    2472:	66e2           	bnes 2456 <Exec_AllocEntry+0xaa>
    2474:	2f0c           	movel %a4,%sp@-
    2476:	2f06           	movel %d6,%sp@-
    2478:	2f0a           	movel %a2,%sp@-
    247a:	206c ff30      	moveal %a4@(-208),%a0
    247e:	4e90           	jsr %a0@
    2480:	204b           	moveal %a3,%a0
    2482:	6014           	bras 2498 <Exec_AllocEntry+0xec>
    2484:	25b3 2814 2814 	movel %a3@(00000014,%d2:l),%a2@(00000014,%d2:l)
    248a:	5082           	addql #8,%d2
    248c:	5284           	addql #1,%d4
    248e:	362b 000e      	movew %a3@(14),%d3
    2492:	b684           	cmpl %d4,%d3
    2494:	628e           	bhis 2424 <Exec_AllocEntry+0x78>
    2496:	204a           	moveal %a2,%a0
    2498:	2008           	movel %a0,%d0
    249a:	4cee 1c7c ffe0 	moveml %fp@(-32),%d2-%d6/%a2-%a4
    24a0:	4e5e           	unlk %fp
    24a2:	4e75           	rts

000024a4 <Exec_AllocMem>:
    24a4:	4e56 ffec      	linkw %fp,#-20
    24a8:	48e7 3f3c      	moveml %d2-%d7/%a2-%a5,%sp@-
    24ac:	242e 0008      	movel %fp@(8),%d2
    24b0:	2a2e 000c      	movel %fp@(12),%d5
    24b4:	226e 0010      	moveal %fp@(16),%a1
    24b8:	7800           	moveq #0,%d4
    24ba:	2c02           	movel %d2,%d6
    24bc:	6700 01a0      	beqw 265e <Exec_AllocMem+0x1ba>
    24c0:	704f           	moveq #79,%d0
    24c2:	d086           	addl %d6,%d0
    24c4:	74f8           	moveq #-8,%d2
    24c6:	c480           	andl %d0,%d2
    24c8:	2f09           	movel %a1,%sp@-
    24ca:	2069 ff7e      	moveal %a1@(-130),%a0
    24ce:	2d49 fff0      	movel %a1,%fp@(-16)
    24d2:	4e90           	jsr %a0@
    24d4:	226e fff0      	moveal %fp@(-16),%a1
    24d8:	2629 0268      	movel %a1@(616),%d3
    24dc:	588f           	addql #4,%sp
    24de:	2229 0142      	movel %a1@(322),%d1
    24e2:	6000 0110      	braw 25f4 <Exec_AllocMem+0x150>
    24e6:	2a41           	moveal %d1,%a5
    24e8:	4280           	clrl %d0
    24ea:	302d 000e      	movew %a5@(14),%d0
    24ee:	2e3c 8005 0000 	movel #-2147155968,%d7
    24f4:	8087           	orl %d7,%d0
    24f6:	4680           	notl %d0
    24f8:	c085           	andl %d5,%d0
    24fa:	6600 00f4      	bnew 25f0 <Exec_AllocMem+0x14c>
    24fe:	b4ad 001c      	cmpl %a5@(28),%d2
    2502:	6200 00ec      	bhiw 25f0 <Exec_AllocMem+0x14c>
    2506:	99cc           	subal %a4,%a4
    2508:	47ed 0010      	lea %a5@(16),%a3
    250c:	2453           	moveal %a3@,%a2
    250e:	b4fc 0000      	cmpaw #0,%a2
    2512:	6700 00dc      	beqw 25f0 <Exec_AllocMem+0x14c>
    2516:	200a           	movel %a2,%d0
    2518:	80aa 0004      	orl %a2@(4),%d0
    251c:	7e07           	moveq #7,%d7
    251e:	c087           	andl %d7,%d0
    2520:	6720           	beqs 2542 <Exec_AllocMem+0x9e>
    2522:	2f09           	movel %a1,%sp@-
    2524:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    252a:	2069 ff96      	moveal %a1@(-106),%a0
    252e:	2d41 ffec      	movel %d1,%fp@(-20)
    2532:	2d49 fff0      	movel %a1,%fp@(-16)
    2536:	4e90           	jsr %a0@
    2538:	508f           	addql #8,%sp
    253a:	226e fff0      	moveal %fp@(-16),%a1
    253e:	222e ffec      	movel %fp@(-20),%d1
    2542:	b4aa 0004      	cmpl %a2@(4),%d2
    2546:	6208           	bhis 2550 <Exec_AllocMem+0xac>
    2548:	284b           	moveal %a3,%a4
    254a:	0805 0012      	btst #18,%d5
    254e:	6736           	beqs 2586 <Exec_AllocMem+0xe2>
    2550:	264a           	moveal %a2,%a3
    2552:	2453           	moveal %a3@,%a2
    2554:	b4fc 0000      	cmpaw #0,%a2
    2558:	672c           	beqs 2586 <Exec_AllocMem+0xe2>
    255a:	200b           	movel %a3,%d0
    255c:	d0ab 0004      	addl %a3@(4),%d0
    2560:	b08a           	cmpl %a2,%d0
    2562:	65b2           	bcss 2516 <Exec_AllocMem+0x72>
    2564:	2f09           	movel %a1,%sp@-
    2566:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    256c:	2069 ff96      	moveal %a1@(-106),%a0
    2570:	2d41 ffec      	movel %d1,%fp@(-20)
    2574:	2d49 fff0      	movel %a1,%fp@(-16)
    2578:	4e90           	jsr %a0@
    257a:	508f           	addql #8,%sp
    257c:	222e ffec      	movel %fp@(-20),%d1
    2580:	226e fff0      	moveal %fp@(-16),%a1
    2584:	6090           	bras 2516 <Exec_AllocMem+0x72>
    2586:	b8fc 0000      	cmpaw #0,%a4
    258a:	6764           	beqs 25f0 <Exec_AllocMem+0x14c>
    258c:	264c           	moveal %a4,%a3
    258e:	2454           	moveal %a4@,%a2
    2590:	b4aa 0004      	cmpl %a2@(4),%d2
    2594:	6606           	bnes 259c <Exec_AllocMem+0xf8>
    2596:	2892           	movel %a2@,%a4@
    2598:	284a           	moveal %a2,%a4
    259a:	6028           	bras 25c4 <Exec_AllocMem+0x120>
    259c:	0805 0012      	btst #18,%d5
    25a0:	670c           	beqs 25ae <Exec_AllocMem+0x10a>
    25a2:	200a           	movel %a2,%d0
    25a4:	d0aa 0004      	addl %a2@(4),%d0
    25a8:	2840           	moveal %d0,%a4
    25aa:	99c2           	subal %d2,%a4
    25ac:	6008           	bras 25b6 <Exec_AllocMem+0x112>
    25ae:	4bf2 2800      	lea %a2@(00000000,%d2:l),%a5
    25b2:	288d           	movel %a5,%a4@
    25b4:	284a           	moveal %a2,%a4
    25b6:	2653           	moveal %a3@,%a3
    25b8:	2692           	movel %a2@,%a3@
    25ba:	246a 0004      	moveal %a2@(4),%a2
    25be:	95c2           	subal %d2,%a2
    25c0:	274a 0004      	movel %a2,%a3@(4)
    25c4:	2a41           	moveal %d1,%a5
    25c6:	95ad 001c      	subl %d2,%a5@(28)
    25ca:	2f09           	movel %a1,%sp@-
    25cc:	2069 ff78      	moveal %a1@(-136),%a0
    25d0:	4e90           	jsr %a0@
    25d2:	588f           	addql #4,%sp
    25d4:	0805 0010      	btst #16,%d5
    25d8:	6712           	beqs 25ec <Exec_AllocMem+0x148>
    25da:	204c           	moveal %a4,%a0
    25dc:	2202           	movel %d2,%d1
    25de:	e489           	lsrl #2,%d1
    25e0:	6002           	bras 25e4 <Exec_AllocMem+0x140>
    25e2:	4298           	clrl %a0@+
    25e4:	2001           	movel %d1,%d0
    25e6:	5381           	subql #1,%d1
    25e8:	4a80           	tstl %d0
    25ea:	66f6           	bnes 25e2 <Exec_AllocMem+0x13e>
    25ec:	280c           	movel %a4,%d4
    25ee:	606e           	bras 265e <Exec_AllocMem+0x1ba>
    25f0:	2a41           	moveal %d1,%a5
    25f2:	2215           	movel %a5@,%d1
    25f4:	2a41           	moveal %d1,%a5
    25f6:	4a95           	tstl %a5@
    25f8:	6600 feec      	bnew 24e6 <Exec_AllocMem+0x42>
    25fc:	4a85           	tstl %d5
    25fe:	6c0c           	bges 260c <Exec_AllocMem+0x168>
    2600:	601c           	bras 261e <Exec_AllocMem+0x17a>
    2602:	7e01           	moveq #1,%d7
    2604:	8fae fffc      	orl %d7,%fp@(-4)
    2608:	6000 fed4      	braw 24de <Exec_AllocMem+0x3a>
    260c:	2d42 fff4      	movel %d2,%fp@(-12)
    2610:	2d45 fff8      	movel %d5,%fp@(-8)
    2614:	42ae fffc      	clrl %fp@(-4)
    2618:	2a43           	moveal %d3,%a5
    261a:	4a95           	tstl %a5@
    261c:	660c           	bnes 262a <Exec_AllocMem+0x186>
    261e:	2f09           	movel %a1,%sp@-
    2620:	2069 ff78      	moveal %a1@(-136),%a0
    2624:	4e90           	jsr %a0@
    2626:	588f           	addql #4,%sp
    2628:	6034           	bras 265e <Exec_AllocMem+0x1ba>
    262a:	2f09           	movel %a1,%sp@-
    262c:	2a43           	moveal %d3,%a5
    262e:	2f2d 000e      	movel %a5@(14),%sp@-
    2632:	486e fff4      	pea %fp@(-12)
    2636:	206d 0012      	moveal %a5@(18),%a0
    263a:	2d49 fff0      	movel %a1,%fp@(-16)
    263e:	4e90           	jsr %a0@
    2640:	4fef 000c      	lea %sp@(12),%sp
    2644:	226e fff0      	moveal %fp@(-16),%a1
    2648:	7e01           	moveq #1,%d7
    264a:	be80           	cmpl %d0,%d7
    264c:	67b4           	beqs 2602 <Exec_AllocMem+0x15e>
    264e:	2615           	movel %a5@,%d3
    2650:	7efe           	moveq #-2,%d7
    2652:	cfae fffc      	andl %d7,%fp@(-4)
    2656:	4a80           	tstl %d0
    2658:	67be           	beqs 2618 <Exec_AllocMem+0x174>
    265a:	6000 fe82      	braw 24de <Exec_AllocMem+0x3a>
    265e:	4a84           	tstl %d4
    2660:	6760           	beqs 26c2 <Exec_AllocMem+0x21e>
    2662:	2a44           	moveal %d4,%a5
    2664:	2a86           	movel %d6,%a5@
    2666:	5084           	addql #8,%d4
    2668:	4878 0020      	pea 20 <pause>
    266c:	4878 00db      	pea db <drawlinehoriz+0x75>
    2670:	2f04           	movel %d4,%sp@-
    2672:	61ff 0000 0000 	bsrl 2674 <Exec_AllocMem+0x1d0>
    2678:	4fef 000c      	lea %sp@(12),%sp
    267c:	7e20           	moveq #32,%d7
    267e:	d887           	addl %d7,%d4
    2680:	0805 0010      	btst #16,%d5
    2684:	661a           	bnes 26a0 <Exec_AllocMem+0x1fc>
    2686:	2044           	moveal %d4,%a0
    2688:	70b8           	moveq #-72,%d0
    268a:	d082           	addl %d2,%d0
    268c:	2200           	movel %d0,%d1
    268e:	e489           	lsrl #2,%d1
    2690:	6006           	bras 2698 <Exec_AllocMem+0x1f4>
    2692:	20fc c0de dbad 	movel #-1059136595,%a0@+
    2698:	2001           	movel %d1,%d0
    269a:	5381           	subql #1,%d1
    269c:	4a80           	tstl %d0
    269e:	66f2           	bnes 2692 <Exec_AllocMem+0x1ee>
    26a0:	2006           	movel %d6,%d0
    26a2:	5e80           	addql #7,%d0
    26a4:	7ef8           	moveq #-8,%d7
    26a6:	c087           	andl %d7,%d0
    26a8:	2a46           	moveal %d6,%a5
    26aa:	41ed ffe0      	lea %a5@(-32),%a0
    26ae:	9088           	subl %a0,%d0
    26b0:	2f00           	movel %d0,%sp@-
    26b2:	4878 00db      	pea db <drawlinehoriz+0x75>
    26b6:	2a44           	moveal %d4,%a5
    26b8:	4875 6800      	pea %a5@(00000000,%d6:l)
    26bc:	61ff 0000 0000 	bsrl 26be <Exec_AllocMem+0x21a>
    26c2:	2044           	moveal %d4,%a0
    26c4:	2008           	movel %a0,%d0
    26c6:	4cee 3cfc ffc4 	moveml %fp@(-60),%d2-%d7/%a2-%a5
    26cc:	4e5e           	unlk %fp
    26ce:	4e75           	rts

000026d0 <Exec_AllocPooled>:
    26d0:	4e56 0000      	linkw %fp,#0
    26d4:	48e7 2038      	moveml %d2/%a2-%a4,%sp@-
    26d8:	242e 000c      	movel %fp@(12),%d2
    26dc:	286e 0010      	moveal %fp@(16),%a4
    26e0:	266e 0008      	moveal %fp@(8),%a3
    26e4:	b4ab 0020      	cmpl %a3@(32),%d2
    26e8:	633c           	blss 2726 <Exec_AllocPooled+0x56>
    26ea:	7210           	moveq #16,%d1
    26ec:	d481           	addl %d1,%d2
    26ee:	2f0c           	movel %a4,%sp@-
    26f0:	2f2b 0018      	movel %a3@(24),%sp@-
    26f4:	2f02           	movel %d2,%sp@-
    26f6:	206c ff3c      	moveal %a4@(-196),%a0
    26fa:	4e90           	jsr %a0@
    26fc:	2448           	moveal %a0,%a2
    26fe:	4fef 000c      	lea %sp@(12),%sp
    2702:	b4fc 0000      	cmpaw #0,%a2
    2706:	6606           	bnes 270e <Exec_AllocPooled+0x3e>
    2708:	91c8           	subal %a0,%a0
    270a:	6000 00b0      	braw 27bc <Exec_AllocPooled+0xec>
    270e:	2542 0008      	movel %d2,%a2@(8)
    2712:	2f0a           	movel %a2,%sp@-
    2714:	486b 000c      	pea %a3@(12)
    2718:	206c ff12      	moveal %a4@(-238),%a0
    271c:	4e90           	jsr %a0@
    271e:	41ea 0010      	lea %a2@(16),%a0
    2722:	6000 0098      	braw 27bc <Exec_AllocPooled+0xec>
    2726:	2453           	moveal %a3@,%a2
    2728:	4a92           	tstl %a2@
    272a:	6658           	bnes 2784 <Exec_AllocPooled+0xb4>
    272c:	2f0c           	movel %a4,%sp@-
    272e:	2f2b 0018      	movel %a3@(24),%sp@-
    2732:	7220           	moveq #32,%d1
    2734:	d2ab 001c      	addl %a3@(28),%d1
    2738:	2f01           	movel %d1,%sp@-
    273a:	206c ff3c      	moveal %a4@(-196),%a0
    273e:	4e90           	jsr %a0@
    2740:	2448           	moveal %a0,%a2
    2742:	4fef 000c      	lea %sp@(12),%sp
    2746:	b4fc 0000      	cmpaw #0,%a2
    274a:	67bc           	beqs 2708 <Exec_AllocPooled+0x38>
    274c:	41ea 0020      	lea %a2@(32),%a0
    2750:	2548 0010      	movel %a0,%a2@(16)
    2754:	4290           	clrl %a0@
    2756:	206a 0010      	moveal %a2@(16),%a0
    275a:	216b 001c 0004 	movel %a3@(28),%a0@(4)
    2760:	256a 0010 0014 	movel %a2@(16),%a2@(20)
    2766:	222a 0010      	movel %a2@(16),%d1
    276a:	d2ab 001c      	addl %a3@(28),%d1
    276e:	2541 0018      	movel %d1,%a2@(24)
    2772:	256b 001c 001c 	movel %a3@(28),%a2@(28)
    2778:	2f0a           	movel %a2,%sp@-
    277a:	2f0b           	movel %a3,%sp@-
    277c:	206c ff12      	moveal %a4@(-238),%a0
    2780:	4e90           	jsr %a0@
    2782:	508f           	addql #8,%sp
    2784:	2f0c           	movel %a4,%sp@-
    2786:	2f02           	movel %d2,%sp@-
    2788:	2f0a           	movel %a2,%sp@-
    278a:	206c ff48      	moveal %a4@(-184),%a0
    278e:	4e90           	jsr %a0@
    2790:	4fef 000c      	lea %sp@(12),%sp
    2794:	b0fc 0000      	cmpaw #0,%a0
    2798:	6604           	bnes 279e <Exec_AllocPooled+0xce>
    279a:	2452           	moveal %a2@,%a2
    279c:	608a           	bras 2728 <Exec_AllocPooled+0x58>
    279e:	082b 0000 0019 	btst #0,%a3@(25)
    27a4:	6716           	beqs 27bc <Exec_AllocPooled+0xec>
    27a6:	2248           	moveal %a0,%a1
    27a8:	2002           	movel %d2,%d0
    27aa:	5680           	addql #3,%d0
    27ac:	2400           	movel %d0,%d2
    27ae:	e48a           	lsrl #2,%d2
    27b0:	6002           	bras 27b4 <Exec_AllocPooled+0xe4>
    27b2:	4299           	clrl %a1@+
    27b4:	2002           	movel %d2,%d0
    27b6:	5382           	subql #1,%d2
    27b8:	4a80           	tstl %d0
    27ba:	66f6           	bnes 27b2 <Exec_AllocPooled+0xe2>
    27bc:	2008           	movel %a0,%d0
    27be:	4cee 1c04 fff0 	moveml %fp@(-16),%d2/%a2-%a4
    27c4:	4e5e           	unlk %fp
    27c6:	4e75           	rts

000027c8 <Exec_AllocSignal>:
    27c8:	4e56 0000      	linkw %fp,#0
    27cc:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    27d0:	262e 0008      	movel %fp@(8),%d3
    27d4:	266e 000c      	moveal %fp@(12),%a3
    27d8:	2f0b           	movel %a3,%sp@-
    27da:	42a7           	clrl %sp@-
    27dc:	206b fedc      	moveal %a3@(-292),%a0
    27e0:	4e90           	jsr %a0@
    27e2:	2448           	moveal %a0,%a2
    27e4:	222a 0012      	movel %a2@(18),%d1
    27e8:	508f           	addql #8,%sp
    27ea:	4a83           	tstl %d3
    27ec:	6c52           	bges 2840 <Exec_AllocSignal+0x78>
    27ee:	4681           	notl %d1
    27f0:	2001           	movel %d1,%d0
    27f2:	4480           	negl %d0
    27f4:	2401           	movel %d1,%d2
    27f6:	c480           	andl %d0,%d2
    27f8:	70ff           	moveq #-1,%d0
    27fa:	4a82           	tstl %d2
    27fc:	677a           	beqs 2878 <Exec_AllocSignal+0xb0>
    27fe:	70ff           	moveq #-1,%d0
    2800:	4640           	notw %d0
    2802:	c082           	andl %d2,%d0
    2804:	56c0           	sne %d0
    2806:	7210           	moveq #16,%d1
    2808:	c280           	andl %d0,%d1
    280a:	203c ff00 ff00 	movel #-16711936,%d0
    2810:	c082           	andl %d2,%d0
    2812:	6702           	beqs 2816 <Exec_AllocSignal+0x4e>
    2814:	5081           	addql #8,%d1
    2816:	2041           	moveal %d1,%a0
    2818:	203c f0f0 f0f0 	movel #-252645136,%d0
    281e:	c082           	andl %d2,%d0
    2820:	6702           	beqs 2824 <Exec_AllocSignal+0x5c>
    2822:	5888           	addql #4,%a0
    2824:	203c cccc cccc 	movel #-858993460,%d0
    282a:	c082           	andl %d2,%d0
    282c:	6702           	beqs 2830 <Exec_AllocSignal+0x68>
    282e:	5488           	addql #2,%a0
    2830:	2608           	movel %a0,%d3
    2832:	203c aaaa aaaa 	movel #-1431655766,%d0
    2838:	c082           	andl %d2,%d0
    283a:	6714           	beqs 2850 <Exec_AllocSignal+0x88>
    283c:	5283           	addql #1,%d3
    283e:	6010           	bras 2850 <Exec_AllocSignal+0x88>
    2840:	7001           	moveq #1,%d0
    2842:	2400           	movel %d0,%d2
    2844:	e7aa           	lsll %d3,%d2
    2846:	2001           	movel %d1,%d0
    2848:	c082           	andl %d2,%d0
    284a:	6704           	beqs 2850 <Exec_AllocSignal+0x88>
    284c:	70ff           	moveq #-1,%d0
    284e:	6028           	bras 2878 <Exec_AllocSignal+0xb0>
    2850:	85aa 0012      	orl %d2,%a2@(18)
    2854:	4682           	notl %d2
    2856:	c5aa 001e      	andl %d2,%a2@(30)
    285a:	c5aa 0016      	andl %d2,%a2@(22)
    285e:	2f0b           	movel %a3,%sp@-
    2860:	206b ff8a      	moveal %a3@(-118),%a0
    2864:	4e90           	jsr %a0@
    2866:	c5aa 001a      	andl %d2,%a2@(26)
    286a:	2f0b           	movel %a3,%sp@-
    286c:	206b ff84      	moveal %a3@(-124),%a0
    2870:	4e90           	jsr %a0@
    2872:	1003           	moveb %d3,%d0
    2874:	4880           	extw %d0
    2876:	48c0           	extl %d0
    2878:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    287e:	4e5e           	unlk %fp
    2880:	4e75           	rts

00002882 <Exec_AllocTrap>:
    2882:	4e56 0000      	linkw %fp,#0
    2886:	4879 0000 0000 	pea 0 <main>
    288c:	4879 0000 0000 	pea 0 <main>
    2892:	61ff 0000 0000 	bsrl 2894 <Exec_AllocTrap+0x12>
    2898:	70ff           	moveq #-1,%d0
    289a:	4e5e           	unlk %fp
    289c:	4e75           	rts

0000289e <Exec_AllocVec>:
    289e:	4e56 0000      	linkw %fp,#0
    28a2:	2f02           	movel %d2,%sp@-
    28a4:	206e 0010      	moveal %fp@(16),%a0
    28a8:	242e 0008      	movel %fp@(8),%d2
    28ac:	5082           	addql #8,%d2
    28ae:	2f08           	movel %a0,%sp@-
    28b0:	2f2e 000c      	movel %fp@(12),%sp@-
    28b4:	2f02           	movel %d2,%sp@-
    28b6:	2068 ff3c      	moveal %a0@(-196),%a0
    28ba:	4e90           	jsr %a0@
    28bc:	b0fc 0000      	cmpaw #0,%a0
    28c0:	6706           	beqs 28c8 <Exec_AllocVec+0x2a>
    28c2:	2082           	movel %d2,%a0@
    28c4:	5088           	addql #8,%a0
    28c6:	6002           	bras 28ca <Exec_AllocVec+0x2c>
    28c8:	91c8           	subal %a0,%a0
    28ca:	2008           	movel %a0,%d0
    28cc:	242e fffc      	movel %fp@(-4),%d2
    28d0:	4e5e           	unlk %fp
    28d2:	4e75           	rts

000028d4 <Exec_AttemptSemaphore>:
    28d4:	4e56 0000      	linkw %fp,#0
    28d8:	48e7 0038      	moveml %a2-%a4,%sp@-
    28dc:	246e 0008      	moveal %fp@(8),%a2
    28e0:	286e 000c      	moveal %fp@(12),%a4
    28e4:	2f0c           	movel %a4,%sp@-
    28e6:	42a7           	clrl %sp@-
    28e8:	206c fedc      	moveal %a4@(-292),%a0
    28ec:	4e90           	jsr %a0@
    28ee:	2648           	moveal %a0,%a3
    28f0:	508f           	addql #8,%sp
    28f2:	0c2a 000f 000c 	cmpib #15,%a2@(12)
    28f8:	6718           	beqs 2912 <Exec_AttemptSemaphore+0x3e>
    28fa:	2f2b 0008      	movel %a3@(8),%sp@-
    28fe:	2f0b           	movel %a3,%sp@-
    2900:	2f0a           	movel %a2,%sp@-
    2902:	4879 0000 0000 	pea 0 <main>
    2908:	61ff 0000 0000 	bsrl 290a <Exec_AttemptSemaphore+0x36>
    290e:	4fef 0010      	lea %sp@(16),%sp
    2912:	2f0c           	movel %a4,%sp@-
    2914:	206c ff7e      	moveal %a4@(-130),%a0
    2918:	4e90           	jsr %a0@
    291a:	302a 002c      	movew %a2@(44),%d0
    291e:	3200           	movew %d0,%d1
    2920:	5241           	addqw #1,%d1
    2922:	3541 002c      	movew %d1,%a2@(44)
    2926:	588f           	addql #4,%sp
    2928:	0c40 ffff      	cmpiw #-1,%d0
    292c:	660a           	bnes 2938 <Exec_AttemptSemaphore+0x64>
    292e:	254b 0028      	movel %a3,%a2@(40)
    2932:	526a 000e      	addqw #1,%a2@(14)
    2936:	6010           	bras 2948 <Exec_AttemptSemaphore+0x74>
    2938:	b7ea 0028      	cmpal %a2@(40),%a3
    293c:	6606           	bnes 2944 <Exec_AttemptSemaphore+0x70>
    293e:	526a 000e      	addqw #1,%a2@(14)
    2942:	6004           	bras 2948 <Exec_AttemptSemaphore+0x74>
    2944:	3540 002c      	movew %d0,%a2@(44)
    2948:	2f0c           	movel %a4,%sp@-
    294a:	206c ff78      	moveal %a4@(-136),%a0
    294e:	4e90           	jsr %a0@
    2950:	b7ea 0028      	cmpal %a2@(40),%a3
    2954:	57c0           	seq %d0
    2956:	4880           	extw %d0
    2958:	3040           	moveaw %d0,%a0
    295a:	2008           	movel %a0,%d0
    295c:	4480           	negl %d0
    295e:	4cee 1c00 fff4 	moveml %fp@(-12),%a2-%a4
    2964:	4e5e           	unlk %fp
    2966:	4e75           	rts

00002968 <Exec_AttemptSemaphoreShared>:
    2968:	4e56 0000      	linkw %fp,#0
    296c:	48e7 0038      	moveml %a2-%a4,%sp@-
    2970:	246e 0008      	moveal %fp@(8),%a2
    2974:	286e 000c      	moveal %fp@(12),%a4
    2978:	2f0c           	movel %a4,%sp@-
    297a:	42a7           	clrl %sp@-
    297c:	206c fedc      	moveal %a4@(-292),%a0
    2980:	4e90           	jsr %a0@
    2982:	2648           	moveal %a0,%a3
    2984:	508f           	addql #8,%sp
    2986:	0c2a 000f 000c 	cmpib #15,%a2@(12)
    298c:	6718           	beqs 29a6 <Exec_AttemptSemaphoreShared+0x3e>
    298e:	2f2b 0008      	movel %a3@(8),%sp@-
    2992:	2f0b           	movel %a3,%sp@-
    2994:	2f0a           	movel %a2,%sp@-
    2996:	4879 0000 0000 	pea 0 <main>
    299c:	61ff 0000 0000 	bsrl 299e <Exec_AttemptSemaphoreShared+0x36>
    29a2:	4fef 0010      	lea %sp@(16),%sp
    29a6:	2f0c           	movel %a4,%sp@-
    29a8:	206c ff7e      	moveal %a4@(-130),%a0
    29ac:	4e90           	jsr %a0@
    29ae:	322a 002c      	movew %a2@(44),%d1
    29b2:	3241           	moveaw %d1,%a1
    29b4:	5249           	addqw #1,%a1
    29b6:	3549 002c      	movew %a1,%a2@(44)
    29ba:	588f           	addql #4,%sp
    29bc:	0c41 ffff      	cmpiw #-1,%d1
    29c0:	660a           	bnes 29cc <Exec_AttemptSemaphoreShared+0x64>
    29c2:	42aa 0028      	clrl %a2@(40)
    29c6:	526a 000e      	addqw #1,%a2@(14)
    29ca:	6010           	bras 29dc <Exec_AttemptSemaphoreShared+0x74>
    29cc:	202a 0028      	movel %a2@(40),%d0
    29d0:	b7c0           	cmpal %d0,%a3
    29d2:	67f2           	beqs 29c6 <Exec_AttemptSemaphoreShared+0x5e>
    29d4:	4a80           	tstl %d0
    29d6:	67ee           	beqs 29c6 <Exec_AttemptSemaphoreShared+0x5e>
    29d8:	3541 002c      	movew %d1,%a2@(44)
    29dc:	2f0c           	movel %a4,%sp@-
    29de:	206c ff78      	moveal %a4@(-136),%a0
    29e2:	4e90           	jsr %a0@
    29e4:	b7ea 0028      	cmpal %a2@(40),%a3
    29e8:	57c0           	seq %d0
    29ea:	4880           	extw %d0
    29ec:	3040           	moveaw %d0,%a0
    29ee:	2008           	movel %a0,%d0
    29f0:	4480           	negl %d0
    29f2:	4cee 1c00 fff4 	moveml %fp@(-12),%a2-%a4
    29f8:	4e5e           	unlk %fp
    29fa:	4e75           	rts

000029fc <Exec_AvailMem>:
    29fc:	4e56 0000      	linkw %fp,#0
    2a00:	48e7 3838      	moveml %d2-%d4/%a2-%a4,%sp@-
    2a04:	262e 0008      	movel %fp@(8),%d3
    2a08:	286e 000c      	moveal %fp@(12),%a4
    2a0c:	7400           	moveq #0,%d2
    2a0e:	2f0c           	movel %a4,%sp@-
    2a10:	206c ff7e      	moveal %a4@(-130),%a0
    2a14:	4e90           	jsr %a0@
    2a16:	266c 0142      	moveal %a4@(322),%a3
    2a1a:	588f           	addql #4,%sp
    2a1c:	4a93           	tstl %a3@
    2a1e:	6700 0082      	beqw 2aa2 <Exec_AvailMem+0xa6>
    2a22:	4280           	clrl %d0
    2a24:	302b 000e      	movew %a3@(14),%d0
    2a28:	283c 800f 0000 	movel #-2146500608,%d4
    2a2e:	8084           	orl %d4,%d0
    2a30:	4680           	notl %d0
    2a32:	c083           	andl %d3,%d0
    2a34:	6666           	bnes 2a9c <Exec_AvailMem+0xa0>
    2a36:	0803 0011      	btst #17,%d3
    2a3a:	674a           	beqs 2a86 <Exec_AvailMem+0x8a>
    2a3c:	246b 0010      	moveal %a3@(16),%a2
    2a40:	b4fc 0000      	cmpaw #0,%a2
    2a44:	6756           	beqs 2a9c <Exec_AvailMem+0xa0>
    2a46:	222a 0004      	movel %a2@(4),%d1
    2a4a:	200a           	movel %a2,%d0
    2a4c:	8081           	orl %d1,%d0
    2a4e:	7807           	moveq #7,%d4
    2a50:	c084           	andl %d4,%d0
    2a52:	660e           	bnes 2a62 <Exec_AvailMem+0x66>
    2a54:	200a           	movel %a2,%d0
    2a56:	d081           	addl %d1,%d0
    2a58:	2212           	movel %a2@,%d1
    2a5a:	b280           	cmpl %d0,%d1
    2a5c:	6214           	bhis 2a72 <Exec_AvailMem+0x76>
    2a5e:	4a81           	tstl %d1
    2a60:	6710           	beqs 2a72 <Exec_AvailMem+0x76>
    2a62:	2f0c           	movel %a4,%sp@-
    2a64:	2f3c 8100 000c 	movel #-2130706420,%sp@-
    2a6a:	206c ff96      	moveal %a4@(-106),%a0
    2a6e:	4e90           	jsr %a0@
    2a70:	508f           	addql #8,%sp
    2a72:	202a 0004      	movel %a2@(4),%d0
    2a76:	b480           	cmpl %d0,%d2
    2a78:	6402           	bccs 2a7c <Exec_AvailMem+0x80>
    2a7a:	2400           	movel %d0,%d2
    2a7c:	2452           	moveal %a2@,%a2
    2a7e:	b4fc 0000      	cmpaw #0,%a2
    2a82:	66c2           	bnes 2a46 <Exec_AvailMem+0x4a>
    2a84:	6016           	bras 2a9c <Exec_AvailMem+0xa0>
    2a86:	0803 0013      	btst #19,%d3
    2a8a:	670c           	beqs 2a98 <Exec_AvailMem+0x9c>
    2a8c:	202b 0018      	movel %a3@(24),%d0
    2a90:	90ab 0014      	subl %a3@(20),%d0
    2a94:	d480           	addl %d0,%d2
    2a96:	6004           	bras 2a9c <Exec_AvailMem+0xa0>
    2a98:	d4ab 001c      	addl %a3@(28),%d2
    2a9c:	2653           	moveal %a3@,%a3
    2a9e:	4a93           	tstl %a3@
    2aa0:	6680           	bnes 2a22 <Exec_AvailMem+0x26>
    2aa2:	2f0c           	movel %a4,%sp@-
    2aa4:	206c ff78      	moveal %a4@(-136),%a0
    2aa8:	4e90           	jsr %a0@
    2aaa:	2002           	movel %d2,%d0
    2aac:	4cee 1c1c ffe8 	moveml %fp@(-24),%d2-%d4/%a2-%a4
    2ab2:	4e5e           	unlk %fp
    2ab4:	4e75           	rts

00002ab6 <Exec_CacheClearE>:
    2ab6:	4e56 0000      	linkw %fp,#0
    2aba:	4e5e           	unlk %fp
    2abc:	4e75           	rts

00002abe <Exec_CacheClearU>:
    2abe:	4e56 0000      	linkw %fp,#0
    2ac2:	206e 0008      	moveal %fp@(8),%a0
    2ac6:	2f08           	movel %a0,%sp@-
    2ac8:	4878 0808      	pea 808 <entry+0x3a4>
    2acc:	4878 ffff      	pea ffffffff <__errno_location+0xffffa56b>
    2ad0:	42a7           	clrl %sp@-
    2ad2:	2068 fd80      	moveal %a0@(-640),%a0
    2ad6:	4e90           	jsr %a0@
    2ad8:	4e5e           	unlk %fp
    2ada:	4e75           	rts

00002adc <Exec_CacheControl>:
    2adc:	4e56 0000      	linkw %fp,#0
    2ae0:	4879 0000 0000 	pea 0 <main>
    2ae6:	4879 0000 0000 	pea 0 <main>
    2aec:	61ff 0000 0000 	bsrl 2aee <Exec_CacheControl+0x12>
    2af2:	7000           	moveq #0,%d0
    2af4:	4e5e           	unlk %fp
    2af6:	4e75           	rts

00002af8 <Exec_CachePostDMA>:
    2af8:	4e56 0000      	linkw %fp,#0
    2afc:	4879 0000 0000 	pea 0 <main>
    2b02:	4879 0000 0000 	pea 0 <main>
    2b08:	61ff 0000 0000 	bsrl 2b0a <Exec_CachePostDMA+0x12>
    2b0e:	4e5e           	unlk %fp
    2b10:	4e75           	rts

00002b12 <Exec_CachePreDMA>:
    2b12:	4e56 0000      	linkw %fp,#0
    2b16:	4879 0000 0000 	pea 0 <main>
    2b1c:	4879 0000 0000 	pea 0 <main>
    2b22:	61ff 0000 0000 	bsrl 2b24 <Exec_CachePreDMA+0x12>
    2b28:	91c8           	subal %a0,%a0
    2b2a:	2008           	movel %a0,%d0
    2b2c:	4e5e           	unlk %fp
    2b2e:	4e75           	rts

00002b30 <Exec_CheckIO>:
    2b30:	4e56 0000      	linkw %fp,#0
    2b34:	226e 0008      	moveal %fp@(8),%a1
    2b38:	0829 0000 001e 	btst #0,%a1@(30)
    2b3e:	660a           	bnes 2b4a <Exec_CheckIO+0x1a>
    2b40:	91c8           	subal %a0,%a0
    2b42:	0c29 0005 000c 	cmpib #5,%a1@(12)
    2b48:	6702           	beqs 2b4c <Exec_CheckIO+0x1c>
    2b4a:	2049           	moveal %a1,%a0
    2b4c:	2008           	movel %a0,%d0
    2b4e:	4e5e           	unlk %fp
    2b50:	4e75           	rts

00002b52 <Exec_ChildFree>:
    2b52:	4e56 0000      	linkw %fp,#0
    2b56:	4879 0000 0000 	pea 0 <main>
    2b5c:	4879 0000 0000 	pea 0 <main>
    2b62:	61ff 0000 0000 	bsrl 2b64 <Exec_ChildFree+0x12>
    2b68:	4e5e           	unlk %fp
    2b6a:	4e75           	rts

00002b6c <Exec_ChildOrphan>:
    2b6c:	4e56 0000      	linkw %fp,#0
    2b70:	4879 0000 0000 	pea 0 <main>
    2b76:	4879 0000 0000 	pea 0 <main>
    2b7c:	61ff 0000 0000 	bsrl 2b7e <Exec_ChildOrphan+0x12>
    2b82:	4e5e           	unlk %fp
    2b84:	4e75           	rts

00002b86 <Exec_ChildStatus>:
    2b86:	4e56 0000      	linkw %fp,#0
    2b8a:	4879 0000 0000 	pea 0 <main>
    2b90:	4879 0000 0000 	pea 0 <main>
    2b96:	61ff 0000 0000 	bsrl 2b98 <Exec_ChildStatus+0x12>
    2b9c:	4e5e           	unlk %fp
    2b9e:	4e75           	rts

00002ba0 <Exec_ChildWait>:
    2ba0:	4e56 0000      	linkw %fp,#0
    2ba4:	4879 0000 0000 	pea 0 <main>
    2baa:	4879 0000 0000 	pea 0 <main>
    2bb0:	61ff 0000 0000 	bsrl 2bb2 <Exec_ChildWait+0x12>
    2bb6:	4e5e           	unlk %fp
    2bb8:	4e75           	rts

00002bba <Exec_CloseDevice>:
    2bba:	4e56 0000      	linkw %fp,#0
    2bbe:	2f0b           	movel %a3,%sp@-
    2bc0:	2f0a           	movel %a2,%sp@-
    2bc2:	266e 0008      	moveal %fp@(8),%a3
    2bc6:	246e 000c      	moveal %fp@(12),%a2
    2bca:	2f0a           	movel %a2,%sp@-
    2bcc:	206a ff7e      	moveal %a2@(-130),%a0
    2bd0:	4e90           	jsr %a0@
    2bd2:	588f           	addql #4,%sp
    2bd4:	206b 0014      	moveal %a3@(20),%a0
    2bd8:	b0fc 0000      	cmpaw #0,%a0
    2bdc:	6712           	beqs 2bf0 <Exec_CloseDevice+0x36>
    2bde:	2f08           	movel %a0,%sp@-
    2be0:	2f0b           	movel %a3,%sp@-
    2be2:	2068 fff6      	moveal %a0@(-10),%a0
    2be6:	4e90           	jsr %a0@
    2be8:	70ff           	moveq #-1,%d0
    2bea:	2740 0014      	movel %d0,%a3@(20)
    2bee:	508f           	addql #8,%sp
    2bf0:	2f0a           	movel %a2,%sp@-
    2bf2:	206a ff78      	moveal %a2@(-136),%a0
    2bf6:	4e90           	jsr %a0@
    2bf8:	246e fff8      	moveal %fp@(-8),%a2
    2bfc:	266e fffc      	moveal %fp@(-4),%a3
    2c00:	4e5e           	unlk %fp
    2c02:	4e75           	rts

00002c04 <Exec_CloseLibrary>:
    2c04:	4e56 0000      	linkw %fp,#0
    2c08:	2f0b           	movel %a3,%sp@-
    2c0a:	2f0a           	movel %a2,%sp@-
    2c0c:	246e 0008      	moveal %fp@(8),%a2
    2c10:	266e 000c      	moveal %fp@(12),%a3
    2c14:	b4fc 0000      	cmpaw #0,%a2
    2c18:	6752           	beqs 2c6c <Exec_CloseLibrary+0x68>
    2c1a:	b5fc 0000 0400 	cmpal #1024,%a2
    2c20:	6f10           	bles 2c32 <Exec_CloseLibrary+0x2e>
    2c22:	2f0b           	movel %a3,%sp@-
    2c24:	2f0a           	movel %a2,%sp@-
    2c26:	206b fdec      	moveal %a3@(-532),%a0
    2c2a:	4e90           	jsr %a0@
    2c2c:	508f           	addql #8,%sp
    2c2e:	4a80           	tstl %d0
    2c30:	6622           	bnes 2c54 <Exec_CloseLibrary+0x50>
    2c32:	2f0a           	movel %a2,%sp@-
    2c34:	4879 0000 0000 	pea 0 <main>
    2c3a:	4878 004d      	pea 4d <pause+0x2d>
    2c3e:	4879 0000 0000 	pea 0 <main>
    2c44:	4879 0000 0000 	pea 0 <main>
    2c4a:	61ff 0000 0000 	bsrl 2c4c <Exec_CloseLibrary+0x48>
    2c50:	4fef 0014      	lea %sp@(20),%sp
    2c54:	2f0b           	movel %a3,%sp@-
    2c56:	206b ff7e      	moveal %a3@(-130),%a0
    2c5a:	4e90           	jsr %a0@
    2c5c:	2f0a           	movel %a2,%sp@-
    2c5e:	206a fff6      	moveal %a2@(-10),%a0
    2c62:	4e90           	jsr %a0@
    2c64:	2f0b           	movel %a3,%sp@-
    2c66:	206b ff78      	moveal %a3@(-136),%a0
    2c6a:	4e90           	jsr %a0@
    2c6c:	246e fff8      	moveal %fp@(-8),%a2
    2c70:	266e fffc      	moveal %fp@(-4),%a3
    2c74:	4e5e           	unlk %fp
    2c76:	4e75           	rts

00002c78 <Exec_CopyMem>:
    2c78:	4e56 0000      	linkw %fp,#0
    2c7c:	2f03           	movel %d3,%sp@-
    2c7e:	2f02           	movel %d2,%sp@-
    2c80:	242e 0010      	movel %fp@(16),%d2
    2c84:	6700 0086      	beqw 2d0c <Exec_CopyMem+0x94>
    2c88:	206e 0008      	moveal %fp@(8),%a0
    2c8c:	226e 000c      	moveal %fp@(12),%a1
    2c90:	2008           	movel %a0,%d0
    2c92:	4480           	negl %d0
    2c94:	7603           	moveq #3,%d3
    2c96:	c083           	andl %d3,%d0
    2c98:	b480           	cmpl %d0,%d2
    2c9a:	6402           	bccs 2c9e <Exec_CopyMem+0x26>
    2c9c:	2002           	movel %d2,%d0
    2c9e:	9480           	subl %d0,%d2
    2ca0:	4a80           	tstl %d0
    2ca2:	6706           	beqs 2caa <Exec_CopyMem+0x32>
    2ca4:	12d8           	moveb %a0@+,%a1@+
    2ca6:	5380           	subql #1,%d0
    2ca8:	66fa           	bnes 2ca4 <Exec_CopyMem+0x2c>
    2caa:	2009           	movel %a1,%d0
    2cac:	7603           	moveq #3,%d3
    2cae:	c083           	andl %d3,%d0
    2cb0:	6630           	bnes 2ce2 <Exec_CopyMem+0x6a>
    2cb2:	2002           	movel %d2,%d0
    2cb4:	e488           	lsrl #2,%d0
    2cb6:	7207           	moveq #7,%d1
    2cb8:	c280           	andl %d0,%d1
    2cba:	e6a8           	lsrl %d3,%d0
    2cbc:	4a81           	tstl %d1
    2cbe:	671a           	beqs 2cda <Exec_CopyMem+0x62>
    2cc0:	22d8           	movel %a0@+,%a1@+
    2cc2:	5381           	subql #1,%d1
    2cc4:	66fa           	bnes 2cc0 <Exec_CopyMem+0x48>
    2cc6:	6012           	bras 2cda <Exec_CopyMem+0x62>
    2cc8:	22d8           	movel %a0@+,%a1@+
    2cca:	22d8           	movel %a0@+,%a1@+
    2ccc:	22d8           	movel %a0@+,%a1@+
    2cce:	22d8           	movel %a0@+,%a1@+
    2cd0:	22d8           	movel %a0@+,%a1@+
    2cd2:	22d8           	movel %a0@+,%a1@+
    2cd4:	22d8           	movel %a0@+,%a1@+
    2cd6:	22d8           	movel %a0@+,%a1@+
    2cd8:	5380           	subql #1,%d0
    2cda:	4a80           	tstl %d0
    2cdc:	66ea           	bnes 2cc8 <Exec_CopyMem+0x50>
    2cde:	7603           	moveq #3,%d3
    2ce0:	c483           	andl %d3,%d2
    2ce2:	7207           	moveq #7,%d1
    2ce4:	c282           	andl %d2,%d1
    2ce6:	2002           	movel %d2,%d0
    2ce8:	e688           	lsrl #3,%d0
    2cea:	4a81           	tstl %d1
    2cec:	671a           	beqs 2d08 <Exec_CopyMem+0x90>
    2cee:	12d8           	moveb %a0@+,%a1@+
    2cf0:	5381           	subql #1,%d1
    2cf2:	66fa           	bnes 2cee <Exec_CopyMem+0x76>
    2cf4:	6012           	bras 2d08 <Exec_CopyMem+0x90>
    2cf6:	12d8           	moveb %a0@+,%a1@+
    2cf8:	12d8           	moveb %a0@+,%a1@+
    2cfa:	12d8           	moveb %a0@+,%a1@+
    2cfc:	12d8           	moveb %a0@+,%a1@+
    2cfe:	12d8           	moveb %a0@+,%a1@+
    2d00:	12d8           	moveb %a0@+,%a1@+
    2d02:	12d8           	moveb %a0@+,%a1@+
    2d04:	12d8           	moveb %a0@+,%a1@+
    2d06:	5380           	subql #1,%d0
    2d08:	4a80           	tstl %d0
    2d0a:	66ea           	bnes 2cf6 <Exec_CopyMem+0x7e>
    2d0c:	241f           	movel %sp@+,%d2
    2d0e:	261f           	movel %sp@+,%d3
    2d10:	4e5e           	unlk %fp
    2d12:	4e75           	rts

00002d14 <Exec_CopyMemQuick>:
    2d14:	4e56 0000      	linkw %fp,#0
    2d18:	226e 0008      	moveal %fp@(8),%a1
    2d1c:	206e 000c      	moveal %fp@(12),%a0
    2d20:	202e 0010      	movel %fp@(16),%d0
    2d24:	e488           	lsrl #2,%d0
    2d26:	7207           	moveq #7,%d1
    2d28:	c280           	andl %d0,%d1
    2d2a:	e688           	lsrl #3,%d0
    2d2c:	4a81           	tstl %d1
    2d2e:	671a           	beqs 2d4a <Exec_CopyMemQuick+0x36>
    2d30:	20d9           	movel %a1@+,%a0@+
    2d32:	5381           	subql #1,%d1
    2d34:	66fa           	bnes 2d30 <Exec_CopyMemQuick+0x1c>
    2d36:	6012           	bras 2d4a <Exec_CopyMemQuick+0x36>
    2d38:	20d9           	movel %a1@+,%a0@+
    2d3a:	20d9           	movel %a1@+,%a0@+
    2d3c:	20d9           	movel %a1@+,%a0@+
    2d3e:	20d9           	movel %a1@+,%a0@+
    2d40:	20d9           	movel %a1@+,%a0@+
    2d42:	20d9           	movel %a1@+,%a0@+
    2d44:	20d9           	movel %a1@+,%a0@+
    2d46:	20d9           	movel %a1@+,%a0@+
    2d48:	5380           	subql #1,%d0
    2d4a:	4a80           	tstl %d0
    2d4c:	66ea           	bnes 2d38 <Exec_CopyMemQuick+0x24>
    2d4e:	4e5e           	unlk %fp
    2d50:	4e75           	rts

00002d52 <Exec_CreateIORequest>:
    2d52:	4e56 0000      	linkw %fp,#0
    2d56:	2f03           	movel %d3,%sp@-
    2d58:	2f02           	movel %d2,%sp@-
    2d5a:	242e 0008      	movel %fp@(8),%d2
    2d5e:	262e 000c      	movel %fp@(12),%d3
    2d62:	206e 0010      	moveal %fp@(16),%a0
    2d66:	4a82           	tstl %d2
    2d68:	6604           	bnes 2d6e <Exec_CreateIORequest+0x1c>
    2d6a:	91c8           	subal %a0,%a0
    2d6c:	601e           	bras 2d8c <Exec_CreateIORequest+0x3a>
    2d6e:	2f08           	movel %a0,%sp@-
    2d70:	2f3c 0001 0001 	movel #65537,%sp@-
    2d76:	2f03           	movel %d3,%sp@-
    2d78:	2068 ff3c      	moveal %a0@(-196),%a0
    2d7c:	4e90           	jsr %a0@
    2d7e:	b0fc 0000      	cmpaw #0,%a0
    2d82:	6708           	beqs 2d8c <Exec_CreateIORequest+0x3a>
    2d84:	2142 000e      	movel %d2,%a0@(14)
    2d88:	3143 0012      	movew %d3,%a0@(18)
    2d8c:	2008           	movel %a0,%d0
    2d8e:	242e fff8      	movel %fp@(-8),%d2
    2d92:	262e fffc      	movel %fp@(-4),%d3
    2d96:	4e5e           	unlk %fp
    2d98:	4e75           	rts

00002d9a <Exec_CreateMsgPort>:
    2d9a:	4e56 0000      	linkw %fp,#0
    2d9e:	2f0b           	movel %a3,%sp@-
    2da0:	2f0a           	movel %a2,%sp@-
    2da2:	266e 0008      	moveal %fp@(8),%a3
    2da6:	2f0b           	movel %a3,%sp@-
    2da8:	2f3c 0001 0001 	movel #65537,%sp@-
    2dae:	4878 0022      	pea 22 <pause+0x2>
    2db2:	206b ff3c      	moveal %a3@(-196),%a0
    2db6:	4e90           	jsr %a0@
    2db8:	2448           	moveal %a0,%a2
    2dba:	4fef 000c      	lea %sp@(12),%sp
    2dbe:	b4fc 0000      	cmpaw #0,%a2
    2dc2:	674a           	beqs 2e0e <Exec_CreateMsgPort+0x74>
    2dc4:	2f0b           	movel %a3,%sp@-
    2dc6:	4878 ffff      	pea ffffffff <__errno_location+0xffffa56b>
    2dca:	206b feb8      	moveal %a3@(-328),%a0
    2dce:	4e90           	jsr %a0@
    2dd0:	508f           	addql #8,%sp
    2dd2:	0c00 ffff      	cmpib #-1,%d0
    2dd6:	6728           	beqs 2e00 <Exec_CreateMsgPort+0x66>
    2dd8:	1540 000f      	moveb %d0,%a2@(15)
    2ddc:	43ea 0018      	lea %a2@(24),%a1
    2de0:	2549 0014      	movel %a1,%a2@(20)
    2de4:	43ea 0014      	lea %a2@(20),%a1
    2de8:	2549 001c      	movel %a1,%a2@(28)
    2dec:	422a 000e      	clrb %a2@(14)
    2df0:	157c 0004 000c 	moveb #4,%a2@(12)
    2df6:	256b 0114 0010 	movel %a3@(276),%a2@(16)
    2dfc:	204a           	moveal %a2,%a0
    2dfe:	6010           	bras 2e10 <Exec_CreateMsgPort+0x76>
    2e00:	2f0b           	movel %a3,%sp@-
    2e02:	4878 0022      	pea 22 <pause+0x2>
    2e06:	2f0a           	movel %a2,%sp@-
    2e08:	206b ff30      	moveal %a3@(-208),%a0
    2e0c:	4e90           	jsr %a0@
    2e0e:	91c8           	subal %a0,%a0
    2e10:	2008           	movel %a0,%d0
    2e12:	246e fff8      	moveal %fp@(-8),%a2
    2e16:	266e fffc      	moveal %fp@(-4),%a3
    2e1a:	4e5e           	unlk %fp
    2e1c:	4e75           	rts

00002e1e <Exec_CreatePool>:
    2e1e:	4e56 0000      	linkw %fp,#0
    2e22:	48e7 3820      	moveml %d2-%d4/%a2,%sp@-
    2e26:	262e 0008      	movel %fp@(8),%d3
    2e2a:	242e 000c      	movel %fp@(12),%d2
    2e2e:	282e 0010      	movel %fp@(16),%d4
    2e32:	206e 0014      	moveal %fp@(20),%a0
    2e36:	93c9           	subal %a1,%a1
    2e38:	b882           	cmpl %d2,%d4
    2e3a:	624a           	bhis 2e86 <Exec_CreatePool+0x68>
    2e3c:	2002           	movel %d2,%d0
    2e3e:	5e80           	addql #7,%d0
    2e40:	74f8           	moveq #-8,%d2
    2e42:	c480           	andl %d0,%d2
    2e44:	2f08           	movel %a0,%sp@-
    2e46:	2f03           	movel %d3,%sp@-
    2e48:	4878 0024      	pea 24 <pause+0x4>
    2e4c:	2068 ff3c      	moveal %a0@(-196),%a0
    2e50:	4e90           	jsr %a0@
    2e52:	2248           	moveal %a0,%a1
    2e54:	b2fc 0000      	cmpaw #0,%a1
    2e58:	672c           	beqs 2e86 <Exec_CreatePool+0x68>
    2e5a:	2349 0008      	movel %a1,%a1@(8)
    2e5e:	42a9 0004      	clrl %a1@(4)
    2e62:	45e9 0004      	lea %a1@(4),%a2
    2e66:	228a           	movel %a2,%a1@
    2e68:	41e9 000c      	lea %a1@(12),%a0
    2e6c:	2348 0014      	movel %a0,%a1@(20)
    2e70:	42a9 0010      	clrl %a1@(16)
    2e74:	45e9 0010      	lea %a1@(16),%a2
    2e78:	208a           	movel %a2,%a0@
    2e7a:	2343 0018      	movel %d3,%a1@(24)
    2e7e:	2342 001c      	movel %d2,%a1@(28)
    2e82:	2344 0020      	movel %d4,%a1@(32)
    2e86:	2049           	moveal %a1,%a0
    2e88:	2008           	movel %a0,%d0
    2e8a:	4cee 041c fff0 	moveml %fp@(-16),%d2-%d4/%a2
    2e90:	4e5e           	unlk %fp
    2e92:	4e75           	rts

00002e94 <Exec_Deallocate>:
    2e94:	4e56 fffc      	linkw %fp,#-4
    2e98:	48e7 3c3c      	moveml %d2-%d5/%a2-%a5,%sp@-
    2e9c:	262e 0008      	movel %fp@(8),%d3
    2ea0:	222e 000c      	movel %fp@(12),%d1
    2ea4:	242e 0010      	movel %fp@(16),%d2
    2ea8:	226e 0014      	moveal %fp@(20),%a1
    2eac:	6700 00ca      	beqw 2f78 <Exec_Deallocate+0xe4>
    2eb0:	7007           	moveq #7,%d0
    2eb2:	c081           	andl %d1,%d0
    2eb4:	2a42           	moveal %d2,%a5
    2eb6:	4bf5 0807      	lea %a5@(00000007,%d0:l),%a5
    2eba:	200d           	movel %a5,%d0
    2ebc:	74f8           	moveq #-8,%d2
    2ebe:	c480           	andl %d0,%d2
    2ec0:	2a43           	moveal %d3,%a5
    2ec2:	49ed 0010      	lea %a5@(16),%a4
    2ec6:	2454           	moveal %a4@,%a2
    2ec8:	78f8           	moveq #-8,%d4
    2eca:	c881           	andl %d1,%d4
    2ecc:	2644           	moveal %d4,%a3
    2ece:	2a0b           	movel %a3,%d5
    2ed0:	da82           	addl %d2,%d5
    2ed2:	b4fc 0000      	cmpaw #0,%a2
    2ed6:	660c           	bnes 2ee4 <Exec_Deallocate+0x50>
    2ed8:	2742 0004      	movel %d2,%a3@(4)
    2edc:	4293           	clrl %a3@
    2ede:	288b           	movel %a3,%a4@
    2ee0:	6000 0092      	braw 2f74 <Exec_Deallocate+0xe0>
    2ee4:	222a 0004      	movel %a2@(4),%d1
    2ee8:	200a           	movel %a2,%d0
    2eea:	8081           	orl %d1,%d0
    2eec:	7807           	moveq #7,%d4
    2eee:	c084           	andl %d4,%d0
    2ef0:	660e           	bnes 2f00 <Exec_Deallocate+0x6c>
    2ef2:	200a           	movel %a2,%d0
    2ef4:	d081           	addl %d1,%d0
    2ef6:	2212           	movel %a2@,%d1
    2ef8:	b280           	cmpl %d0,%d1
    2efa:	621c           	bhis 2f18 <Exec_Deallocate+0x84>
    2efc:	4a81           	tstl %d1
    2efe:	6718           	beqs 2f18 <Exec_Deallocate+0x84>
    2f00:	2f09           	movel %a1,%sp@-
    2f02:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    2f08:	2069 ff96      	moveal %a1@(-106),%a0
    2f0c:	2d49 fffc      	movel %a1,%fp@(-4)
    2f10:	4e90           	jsr %a0@
    2f12:	508f           	addql #8,%sp
    2f14:	226e fffc      	moveal %fp@(-4),%a1
    2f18:	b7ca           	cmpal %a2,%a3
    2f1a:	6206           	bhis 2f22 <Exec_Deallocate+0x8e>
    2f1c:	b5c5           	cmpal %d5,%a2
    2f1e:	640c           	bccs 2f2c <Exec_Deallocate+0x98>
    2f20:	601e           	bras 2f40 <Exec_Deallocate+0xac>
    2f22:	284a           	moveal %a2,%a4
    2f24:	2452           	moveal %a2@,%a2
    2f26:	b4fc 0000      	cmpaw #0,%a2
    2f2a:	66b8           	bnes 2ee4 <Exec_Deallocate+0x50>
    2f2c:	2a43           	moveal %d3,%a5
    2f2e:	41ed 0010      	lea %a5@(16),%a0
    2f32:	b1cc           	cmpal %a4,%a0
    2f34:	6722           	beqs 2f58 <Exec_Deallocate+0xc4>
    2f36:	200c           	movel %a4,%d0
    2f38:	d0ac 0004      	addl %a4@(4),%d0
    2f3c:	b7c0           	cmpal %d0,%a3
    2f3e:	6410           	bccs 2f50 <Exec_Deallocate+0xbc>
    2f40:	2f09           	movel %a1,%sp@-
    2f42:	2f3c 0100 0009 	movel #16777225,%sp@-
    2f48:	2069 ff96      	moveal %a1@(-106),%a0
    2f4c:	4e90           	jsr %a0@
    2f4e:	6028           	bras 2f78 <Exec_Deallocate+0xe4>
    2f50:	b7c0           	cmpal %d0,%a3
    2f52:	6604           	bnes 2f58 <Exec_Deallocate+0xc4>
    2f54:	264c           	moveal %a4,%a3
    2f56:	6002           	bras 2f5a <Exec_Deallocate+0xc6>
    2f58:	288b           	movel %a3,%a4@
    2f5a:	b5c5           	cmpal %d5,%a2
    2f5c:	660c           	bnes 2f6a <Exec_Deallocate+0xd6>
    2f5e:	4a85           	tstl %d5
    2f60:	6708           	beqs 2f6a <Exec_Deallocate+0xd6>
    2f62:	2a45           	moveal %d5,%a5
    2f64:	daad 0004      	addl %a5@(4),%d5
    2f68:	2452           	moveal %a2@,%a2
    2f6a:	268a           	movel %a2,%a3@
    2f6c:	9a8b           	subl %a3,%d5
    2f6e:	2745 0004      	movel %d5,%a3@(4)
    2f72:	2a43           	moveal %d3,%a5
    2f74:	d5ad 001c      	addl %d2,%a5@(28)
    2f78:	4cee 3c3c ffdc 	moveml %fp@(-36),%d2-%d5/%a2-%a5
    2f7e:	4e5e           	unlk %fp
    2f80:	4e75           	rts

00002f82 <Exec_Debug>:
    2f82:	4e56 0000      	linkw %fp,#0
    2f86:	4879 0000 0000 	pea 0 <main>
    2f8c:	4879 0000 0000 	pea 0 <main>
    2f92:	61ff 0000 0000 	bsrl 2f94 <Exec_Debug+0x12>
    2f98:	4e5e           	unlk %fp
    2f9a:	4e75           	rts

00002f9c <Exec_DeleteIORequest>:
    2f9c:	4e56 0000      	linkw %fp,#0
    2fa0:	206e 0008      	moveal %fp@(8),%a0
    2fa4:	226e 000c      	moveal %fp@(12),%a1
    2fa8:	b0fc 0000      	cmpaw #0,%a0
    2fac:	6712           	beqs 2fc0 <Exec_DeleteIORequest+0x24>
    2fae:	2f09           	movel %a1,%sp@-
    2fb0:	4280           	clrl %d0
    2fb2:	3028 0012      	movew %a0@(18),%d0
    2fb6:	2f00           	movel %d0,%sp@-
    2fb8:	2f08           	movel %a0,%sp@-
    2fba:	2069 ff30      	moveal %a1@(-208),%a0
    2fbe:	4e90           	jsr %a0@
    2fc0:	4e5e           	unlk %fp
    2fc2:	4e75           	rts

00002fc4 <Exec_DeleteMsgPort>:
    2fc4:	4e56 0000      	linkw %fp,#0
    2fc8:	2f0b           	movel %a3,%sp@-
    2fca:	2f0a           	movel %a2,%sp@-
    2fcc:	266e 0008      	moveal %fp@(8),%a3
    2fd0:	246e 000c      	moveal %fp@(12),%a2
    2fd4:	b6fc 0000      	cmpaw #0,%a3
    2fd8:	671e           	beqs 2ff8 <Exec_DeleteMsgPort+0x34>
    2fda:	2f0a           	movel %a2,%sp@-
    2fdc:	4280           	clrl %d0
    2fde:	102b 000f      	moveb %a3@(15),%d0
    2fe2:	2f00           	movel %d0,%sp@-
    2fe4:	206a feb2      	moveal %a2@(-334),%a0
    2fe8:	4e90           	jsr %a0@
    2fea:	2f0a           	movel %a2,%sp@-
    2fec:	4878 0022      	pea 22 <pause+0x2>
    2ff0:	2f0b           	movel %a3,%sp@-
    2ff2:	206a ff30      	moveal %a2@(-208),%a0
    2ff6:	4e90           	jsr %a0@
    2ff8:	246e fff8      	moveal %fp@(-8),%a2
    2ffc:	266e fffc      	moveal %fp@(-4),%a3
    3000:	4e5e           	unlk %fp
    3002:	4e75           	rts

00003004 <Exec_DeletePool>:
    3004:	4e56 0000      	linkw %fp,#0
    3008:	48e7 2030      	moveml %d2/%a2-%a3,%sp@-
    300c:	266e 000c      	moveal %fp@(12),%a3
    3010:	246e 0008      	moveal %fp@(8),%a2
    3014:	b4fc 0000      	cmpaw #0,%a2
    3018:	675a           	beqs 3074 <Exec_DeletePool+0x70>
    301a:	7420           	moveq #32,%d2
    301c:	d4aa 001c      	addl %a2@(28),%d2
    3020:	2f0a           	movel %a2,%sp@-
    3022:	206b ff00      	moveal %a3@(-256),%a0
    3026:	4e90           	jsr %a0@
    3028:	2008           	movel %a0,%d0
    302a:	588f           	addql #4,%sp
    302c:	6712           	beqs 3040 <Exec_DeletePool+0x3c>
    302e:	2f0b           	movel %a3,%sp@-
    3030:	2f02           	movel %d2,%sp@-
    3032:	2f00           	movel %d0,%sp@-
    3034:	206b ff30      	moveal %a3@(-208),%a0
    3038:	4e90           	jsr %a0@
    303a:	4fef 000c      	lea %sp@(12),%sp
    303e:	60e0           	bras 3020 <Exec_DeletePool+0x1c>
    3040:	486a 000c      	pea %a2@(12)
    3044:	206b ff00      	moveal %a3@(-256),%a0
    3048:	4e90           	jsr %a0@
    304a:	588f           	addql #4,%sp
    304c:	b0fc 0000      	cmpaw #0,%a0
    3050:	6714           	beqs 3066 <Exec_DeletePool+0x62>
    3052:	2f0b           	movel %a3,%sp@-
    3054:	2f28 0008      	movel %a0@(8),%sp@-
    3058:	2f08           	movel %a0,%sp@-
    305a:	206b ff30      	moveal %a3@(-208),%a0
    305e:	4e90           	jsr %a0@
    3060:	4fef 000c      	lea %sp@(12),%sp
    3064:	60da           	bras 3040 <Exec_DeletePool+0x3c>
    3066:	2f0b           	movel %a3,%sp@-
    3068:	4878 0024      	pea 24 <pause+0x4>
    306c:	2f0a           	movel %a2,%sp@-
    306e:	206b ff30      	moveal %a3@(-208),%a0
    3072:	4e90           	jsr %a0@
    3074:	4cee 0c04 fff4 	moveml %fp@(-12),%d2/%a2-%a3
    307a:	4e5e           	unlk %fp
    307c:	4e75           	rts

0000307e <Exec_Dispatch>:
    307e:	4e56 0000      	linkw %fp,#0
    3082:	48e7 0038      	moveml %a2-%a4,%sp@-
    3086:	286e 0008      	moveal %fp@(8),%a4
    308a:	266c 0114      	moveal %a4@(276),%a3
    308e:	486c 0196      	pea %a4@(406)
    3092:	206c ff00      	moveal %a4@(-256),%a0
    3096:	4e90           	jsr %a0@
    3098:	2448           	moveal %a0,%a2
    309a:	588f           	addql #4,%sp
    309c:	b4fc 0000      	cmpaw #0,%a2
    30a0:	6768           	beqs 310a <Exec_Dispatch+0x8c>
    30a2:	082b 0006 000e 	btst #6,%a3@(14)
    30a8:	670a           	beqs 30b4 <Exec_Dispatch+0x36>
    30aa:	2f0c           	movel %a4,%sp@-
    30ac:	206b 0042      	moveal %a3@(66),%a0
    30b0:	4e90           	jsr %a0@
    30b2:	588f           	addql #4,%sp
    30b4:	176c 0127 0011 	moveb %a4@(295),%a3@(17)
    30ba:	176c 0126 0010 	moveb %a4@(294),%a3@(16)
    30c0:	157c 0002 000f 	moveb #2,%a2@(15)
    30c6:	196a 0011 0127 	moveb %a2@(17),%a4@(295)
    30cc:	196a 0010 0126 	moveb %a2@(16),%a4@(294)
    30d2:	294a 0114      	movel %a2,%a4@(276)
    30d6:	202a 0036      	movel %a2@(54),%d0
    30da:	b0aa 003a      	cmpl %a2@(58),%d0
    30de:	6306           	blss 30e6 <Exec_Dispatch+0x68>
    30e0:	b0aa 003e      	cmpl %a2@(62),%d0
    30e4:	6510           	bcss 30f6 <Exec_Dispatch+0x78>
    30e6:	2f0c           	movel %a4,%sp@-
    30e8:	2f3c 8100 000e 	movel #-2130706418,%sp@-
    30ee:	206c ff96      	moveal %a4@(-106),%a0
    30f2:	4e90           	jsr %a0@
    30f4:	508f           	addql #8,%sp
    30f6:	4a2a 000e      	tstb %a2@(14)
    30fa:	6c08           	bges 3104 <Exec_Dispatch+0x86>
    30fc:	2f0c           	movel %a4,%sp@-
    30fe:	206a 0046      	moveal %a2@(70),%a0
    3102:	4e90           	jsr %a0@
    3104:	52ac 011c      	addql #1,%a4@(284)
    3108:	600c           	bras 3116 <Exec_Dispatch+0x98>
    310a:	4879 0000 0000 	pea 0 <main>
    3110:	61ff 0000 0000 	bsrl 3112 <Exec_Dispatch+0x94>
    3116:	4cee 1c00 fff4 	moveml %fp@(-12),%a2-%a4
    311c:	4e5e           	unlk %fp
    311e:	4e75           	rts

00003120 <Exec_DoIO>:
    3120:	4e56 0000      	linkw %fp,#0
    3124:	2f0b           	movel %a3,%sp@-
    3126:	2f0a           	movel %a2,%sp@-
    3128:	246e 0008      	moveal %fp@(8),%a2
    312c:	266e 000c      	moveal %fp@(12),%a3
    3130:	157c 0001 001e 	moveb #1,%a2@(30)
    3136:	422a 000c      	clrb %a2@(12)
    313a:	206a 0014      	moveal %a2@(20),%a0
    313e:	2f08           	movel %a0,%sp@-
    3140:	2f0a           	movel %a2,%sp@-
    3142:	2068 ffe4      	moveal %a0@(-28),%a0
    3146:	4e90           	jsr %a0@
    3148:	508f           	addql #8,%sp
    314a:	082a 0000 001e 	btst #0,%a2@(30)
    3150:	660a           	bnes 315c <Exec_DoIO+0x3c>
    3152:	2f0b           	movel %a3,%sp@-
    3154:	2f0a           	movel %a2,%sp@-
    3156:	206b fe28      	moveal %a3@(-472),%a0
    315a:	4e90           	jsr %a0@
    315c:	102a 001f      	moveb %a2@(31),%d0
    3160:	4880           	extw %d0
    3162:	48c0           	extl %d0
    3164:	246e fff8      	moveal %fp@(-8),%a2
    3168:	266e fffc      	moveal %fp@(-4),%a3
    316c:	4e5e           	unlk %fp
    316e:	4e75           	rts

00003170 <Exec_Enqueue>:
    3170:	4e56 0000      	linkw %fp,#0
    3174:	2f0a           	movel %a2,%sp@-
    3176:	206e 0008      	moveal %fp@(8),%a0
    317a:	246e 000c      	moveal %fp@(12),%a2
    317e:	2250           	moveal %a0@,%a1
    3180:	4a91           	tstl %a1@
    3182:	6710           	beqs 3194 <Exec_Enqueue+0x24>
    3184:	102a 000d      	moveb %a2@(13),%d0
    3188:	b029 000d      	cmpb %a1@(13),%d0
    318c:	6e06           	bgts 3194 <Exec_Enqueue+0x24>
    318e:	2251           	moveal %a1@,%a1
    3190:	4a91           	tstl %a1@
    3192:	66f4           	bnes 3188 <Exec_Enqueue+0x18>
    3194:	2569 0004 0004 	movel %a1@(4),%a2@(4)
    319a:	2069 0004      	moveal %a1@(4),%a0
    319e:	208a           	movel %a2,%a0@
    31a0:	234a 0004      	movel %a2,%a1@(4)
    31a4:	2489           	movel %a1,%a2@
    31a6:	245f           	moveal %sp@+,%a2
    31a8:	4e5e           	unlk %fp
    31aa:	4e75           	rts

000031ac <Exec_Exception>:
    31ac:	4e56 0000      	linkw %fp,#0
    31b0:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    31b4:	266e 0008      	moveal %fp@(8),%a3
    31b8:	2f0b           	movel %a3,%sp@-
    31ba:	42a7           	clrl %sp@-
    31bc:	206b fedc      	moveal %a3@(-292),%a0
    31c0:	4e90           	jsr %a0@
    31c2:	2448           	moveal %a0,%a2
    31c4:	022a 00df 000e 	andib #-33,%a2@(14)
    31ca:	162b 0126      	moveb %a3@(294),%d3
    31ce:	422b 0126      	clrb %a3@(294)
    31d2:	508f           	addql #8,%sp
    31d4:	603a           	bras 3210 <Exec_Exception+0x64>
    31d6:	b382           	eorl %d1,%d2
    31d8:	b182           	eorl %d0,%d2
    31da:	2f0b           	movel %a3,%sp@-
    31dc:	206b ff84      	moveal %a3@(-124),%a0
    31e0:	4e90           	jsr %a0@
    31e2:	588f           	addql #4,%sp
    31e4:	206a 002a      	moveal %a2@(42),%a0
    31e8:	b0fc 0000      	cmpaw #0,%a0
    31ec:	6714           	beqs 3202 <Exec_Exception+0x56>
    31ee:	2f0b           	movel %a3,%sp@-
    31f0:	2f02           	movel %d2,%sp@-
    31f2:	2f2a 0026      	movel %a2@(38),%sp@-
    31f6:	4e90           	jsr %a0@
    31f8:	2540 001e      	movel %d0,%a2@(30)
    31fc:	4fef 000c      	lea %sp@(12),%sp
    3200:	6004           	bras 3206 <Exec_Exception+0x5a>
    3202:	42aa 001e      	clrl %a2@(30)
    3206:	2f0b           	movel %a3,%sp@-
    3208:	206b ff8a      	moveal %a3@(-118),%a0
    320c:	4e90           	jsr %a0@
    320e:	588f           	addql #4,%sp
    3210:	222a 001e      	movel %a2@(30),%d1
    3214:	202a 001a      	movel %a2@(26),%d0
    3218:	2401           	movel %d1,%d2
    321a:	c480           	andl %d0,%d2
    321c:	66b8           	bnes 31d6 <Exec_Exception+0x2a>
    321e:	1743 0126      	moveb %d3,%a3@(294)
    3222:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    3228:	4e5e           	unlk %fp
    322a:	4e75           	rts

0000322c <Exec_FindName>:
    322c:	4e56 0000      	linkw %fp,#0
    3230:	2f0a           	movel %a2,%sp@-
    3232:	2f02           	movel %d2,%sp@-
    3234:	206e 0008      	moveal %fp@(8),%a0
    3238:	242e 000c      	movel %fp@(12),%d2
    323c:	2210           	movel %a0@,%d1
    323e:	6018           	bras 3258 <Exec_FindName+0x2c>
    3240:	202a 0008      	movel %a2@(8),%d0
    3244:	6710           	beqs 3256 <Exec_FindName+0x2a>
    3246:	2f02           	movel %d2,%sp@-
    3248:	2f00           	movel %d0,%sp@-
    324a:	61ff 0000 0000 	bsrl 324c <Exec_FindName+0x20>
    3250:	508f           	addql #8,%sp
    3252:	4a80           	tstl %d0
    3254:	6716           	beqs 326c <Exec_FindName+0x40>
    3256:	2212           	movel %a2@,%d1
    3258:	2241           	moveal %d1,%a1
    325a:	4a91           	tstl %a1@
    325c:	56c0           	sne %d0
    325e:	4880           	extw %d0
    3260:	48c0           	extl %d0
    3262:	c280           	andl %d0,%d1
    3264:	2441           	moveal %d1,%a2
    3266:	b4fc 0000      	cmpaw #0,%a2
    326a:	66d4           	bnes 3240 <Exec_FindName+0x14>
    326c:	204a           	moveal %a2,%a0
    326e:	2008           	movel %a0,%d0
    3270:	242e fff8      	movel %fp@(-8),%d2
    3274:	246e fffc      	moveal %fp@(-4),%a2
    3278:	4e5e           	unlk %fp
    327a:	4e75           	rts

0000327c <Exec_FindPort>:
    327c:	4e56 0000      	linkw %fp,#0
    3280:	206e 000c      	moveal %fp@(12),%a0
    3284:	2f2e 0008      	movel %fp@(8),%sp@-
    3288:	4868 0188      	pea %a0@(392)
    328c:	2068 feee      	moveal %a0@(-274),%a0
    3290:	4e90           	jsr %a0@
    3292:	2008           	movel %a0,%d0
    3294:	4e5e           	unlk %fp
    3296:	4e75           	rts

00003298 <Exec_FindResident>:
    3298:	4e56 0000      	linkw %fp,#0
    329c:	2f0a           	movel %a2,%sp@-
    329e:	2f02           	movel %d2,%sp@-
    32a0:	242e 0008      	movel %fp@(8),%d2
    32a4:	206e 000c      	moveal %fp@(12),%a0
    32a8:	2468 012c      	moveal %a0@(300),%a2
    32ac:	b4fc 0000      	cmpaw #0,%a2
    32b0:	6730           	beqs 32e2 <Exec_FindResident+0x4a>
    32b2:	4a92           	tstl %a2@
    32b4:	672c           	beqs 32e2 <Exec_FindResident+0x4a>
    32b6:	2012           	movel %a2@,%d0
    32b8:	6c0a           	bges 32c4 <Exec_FindResident+0x2c>
    32ba:	223c 7fff ffff 	movel #2147483647,%d1
    32c0:	c280           	andl %d0,%d1
    32c2:	2441           	moveal %d1,%a2
    32c4:	2f02           	movel %d2,%sp@-
    32c6:	2052           	moveal %a2@,%a0
    32c8:	2f28 000e      	movel %a0@(14),%sp@-
    32cc:	61ff 0000 0000 	bsrl 32ce <Exec_FindResident+0x36>
    32d2:	508f           	addql #8,%sp
    32d4:	4a80           	tstl %d0
    32d6:	6604           	bnes 32dc <Exec_FindResident+0x44>
    32d8:	2052           	moveal %a2@,%a0
    32da:	6008           	bras 32e4 <Exec_FindResident+0x4c>
    32dc:	588a           	addql #4,%a2
    32de:	4a92           	tstl %a2@
    32e0:	66d4           	bnes 32b6 <Exec_FindResident+0x1e>
    32e2:	91c8           	subal %a0,%a0
    32e4:	2008           	movel %a0,%d0
    32e6:	242e fff8      	movel %fp@(-8),%d2
    32ea:	246e fffc      	moveal %fp@(-4),%a2
    32ee:	4e5e           	unlk %fp
    32f0:	4e75           	rts

000032f2 <Exec_FindSemaphore>:
    32f2:	4e56 0000      	linkw %fp,#0
    32f6:	206e 000c      	moveal %fp@(12),%a0
    32fa:	2f2e 0008      	movel %fp@(8),%sp@-
    32fe:	4868 0214      	pea %a0@(532)
    3302:	2068 feee      	moveal %a0@(-274),%a0
    3306:	4e90           	jsr %a0@
    3308:	2008           	movel %a0,%d0
    330a:	4e5e           	unlk %fp
    330c:	4e75           	rts

0000330e <Exec_FindTask>:
    330e:	4e56 0000      	linkw %fp,#0
    3312:	48e7 3020      	moveml %d2-%d3/%a2,%sp@-
    3316:	262e 0008      	movel %fp@(8),%d3
    331a:	246e 000c      	moveal %fp@(12),%a2
    331e:	660c           	bnes 332c <Exec_FindTask+0x1e>
    3320:	206a 0114      	moveal %a2@(276),%a0
    3324:	6056           	bras 337c <Exec_FindTask+0x6e>
    3326:	242a 0114      	movel %a2@(276),%d2
    332a:	6046           	bras 3372 <Exec_FindTask+0x64>
    332c:	2f0a           	movel %a2,%sp@-
    332e:	206a ff8a      	moveal %a2@(-118),%a0
    3332:	4e90           	jsr %a0@
    3334:	2f03           	movel %d3,%sp@-
    3336:	486a 0196      	pea %a2@(406)
    333a:	206a feee      	moveal %a2@(-274),%a0
    333e:	4e90           	jsr %a0@
    3340:	2408           	movel %a0,%d2
    3342:	4fef 000c      	lea %sp@(12),%sp
    3346:	662a           	bnes 3372 <Exec_FindTask+0x64>
    3348:	2f03           	movel %d3,%sp@-
    334a:	486a 01a4      	pea %a2@(420)
    334e:	206a feee      	moveal %a2@(-274),%a0
    3352:	4e90           	jsr %a0@
    3354:	2408           	movel %a0,%d2
    3356:	508f           	addql #8,%sp
    3358:	6618           	bnes 3372 <Exec_FindTask+0x64>
    335a:	206a 0114      	moveal %a2@(276),%a0
    335e:	2068 0008      	moveal %a0@(8),%a0
    3362:	2243           	moveal %d3,%a1
    3364:	6006           	bras 336c <Exec_FindTask+0x5e>
    3366:	5289           	addql #1,%a1
    3368:	4a00           	tstb %d0
    336a:	67ba           	beqs 3326 <Exec_FindTask+0x18>
    336c:	1011           	moveb %a1@,%d0
    336e:	b018           	cmpb %a0@+,%d0
    3370:	67f4           	beqs 3366 <Exec_FindTask+0x58>
    3372:	2f0a           	movel %a2,%sp@-
    3374:	206a ff84      	moveal %a2@(-124),%a0
    3378:	4e90           	jsr %a0@
    337a:	2042           	moveal %d2,%a0
    337c:	2008           	movel %a0,%d0
    337e:	4cee 040c fff4 	moveml %fp@(-12),%d2-%d3/%a2
    3384:	4e5e           	unlk %fp
    3386:	4e75           	rts

00003388 <Exec_Forbid>:
    3388:	4e56 0000      	linkw %fp,#0
    338c:	206e 0008      	moveal %fp@(8),%a0
    3390:	5228 0127      	addqb #1,%a0@(295)
    3394:	4e5e           	unlk %fp
    3396:	4e75           	rts

00003398 <Exec_FreeEntry>:
    3398:	4e56 0000      	linkw %fp,#0
    339c:	48e7 3830      	moveml %d2-%d4/%a2-%a3,%sp@-
    33a0:	246e 0008      	moveal %fp@(8),%a2
    33a4:	266e 000c      	moveal %fp@(12),%a3
    33a8:	7600           	moveq #0,%d3
    33aa:	4a6a 000e      	tstw %a2@(14)
    33ae:	6724           	beqs 33d4 <Exec_FreeEntry+0x3c>
    33b0:	2803           	movel %d3,%d4
    33b2:	2403           	movel %d3,%d2
    33b4:	2f0b           	movel %a3,%sp@-
    33b6:	2f32 2814      	movel %a2@(00000014,%d2:l),%sp@-
    33ba:	2f32 2810      	movel %a2@(00000010,%d2:l),%sp@-
    33be:	206b ff30      	moveal %a3@(-208),%a0
    33c2:	4e90           	jsr %a0@
    33c4:	4fef 000c      	lea %sp@(12),%sp
    33c8:	5082           	addql #8,%d2
    33ca:	5283           	addql #1,%d3
    33cc:	382a 000e      	movew %a2@(14),%d4
    33d0:	b883           	cmpl %d3,%d4
    33d2:	62e0           	bhis 33b4 <Exec_FreeEntry+0x1c>
    33d4:	2f0b           	movel %a3,%sp@-
    33d6:	4280           	clrl %d0
    33d8:	302a 000e      	movew %a2@(14),%d0
    33dc:	e788           	lsll #3,%d0
    33de:	2240           	moveal %d0,%a1
    33e0:	4869 0010      	pea %a1@(16)
    33e4:	2f0a           	movel %a2,%sp@-
    33e6:	206b ff30      	moveal %a3@(-208),%a0
    33ea:	4e90           	jsr %a0@
    33ec:	4cee 0c1c ffec 	moveml %fp@(-20),%d2-%d4/%a2-%a3
    33f2:	4e5e           	unlk %fp
    33f4:	4e75           	rts

000033f6 <Exec_FreeMem>:
    33f6:	4e56 fffc      	linkw %fp,#-4
    33fa:	48e7 3e3c      	moveml %d2-%d6/%a2-%a5,%sp@-
    33fe:	266e 0008      	moveal %fp@(8),%a3
    3402:	262e 000c      	movel %fp@(12),%d3
    3406:	226e 0010      	moveal %fp@(16),%a1
    340a:	2843           	moveal %d3,%a4
    340c:	6700 0236      	beqw 3644 <Exec_FreeMem+0x24e>
    3410:	200b           	movel %a3,%d0
    3412:	7807           	moveq #7,%d4
    3414:	c084           	andl %d4,%d0
    3416:	2a0b           	movel %a3,%d5
    3418:	78f8           	moveq #-8,%d4
    341a:	ca84           	andl %d4,%d5
    341c:	2645           	moveal %d5,%a3
    341e:	47eb ffd8      	lea %a3@(-40),%a3
    3422:	2a43           	moveal %d3,%a5
    3424:	4bf5 084f      	lea %a5@(0000004f,%d0:l),%a5
    3428:	200d           	movel %a5,%d0
    342a:	2604           	movel %d4,%d3
    342c:	c680           	andl %d0,%d3
    342e:	b9d3           	cmpal %a3@,%a4
    3430:	6730           	beqs 3462 <Exec_FreeMem+0x6c>
    3432:	2f09           	movel %a1,%sp@-
    3434:	42a7           	clrl %sp@-
    3436:	2069 fedc      	moveal %a1@(-292),%a0
    343a:	2d49 fffc      	movel %a1,%fp@(-4)
    343e:	4e90           	jsr %a0@
    3440:	2f28 0008      	movel %a0@(8),%sp@-
    3444:	2f08           	movel %a0,%sp@-
    3446:	2f0c           	movel %a4,%sp@-
    3448:	2f13           	movel %a3@,%sp@-
    344a:	486b 0028      	pea %a3@(40)
    344e:	4879 0000 0000 	pea 0 <main>
    3454:	61ff 0000 0000 	bsrl 3456 <Exec_FreeMem+0x60>
    345a:	4fef 0020      	lea %sp@(32),%sp
    345e:	226e fffc      	moveal %fp@(-4),%a1
    3462:	45eb 0008      	lea %a3@(8),%a2
    3466:	741f           	moveq #31,%d2
    3468:	0c12 00db      	cmpib #-37,%a2@
    346c:	672a           	beqs 3498 <Exec_FreeMem+0xa2>
    346e:	2f09           	movel %a1,%sp@-
    3470:	42a7           	clrl %sp@-
    3472:	2069 fedc      	moveal %a1@(-292),%a0
    3476:	2d49 fffc      	movel %a1,%fp@(-4)
    347a:	4e90           	jsr %a0@
    347c:	2f28 0008      	movel %a0@(8),%sp@-
    3480:	2f08           	movel %a0,%sp@-
    3482:	2f0a           	movel %a2,%sp@-
    3484:	4879 0000 0000 	pea 0 <main>
    348a:	61ff 0000 0000 	bsrl 348c <Exec_FreeMem+0x96>
    3490:	4fef 0018      	lea %sp@(24),%sp
    3494:	226e fffc      	moveal %fp@(-4),%a1
    3498:	528a           	addql #1,%a2
    349a:	51ca ffcc      	dbf %d2,3468 <Exec_FreeMem+0x72>
    349e:	4242           	clrw %d2
    34a0:	5382           	subql #1,%d2
    34a2:	64c4           	bccs 3468 <Exec_FreeMem+0x72>
    34a4:	45f3 c828      	lea %a3@(00000028,%a4:l),%a2
    34a8:	200c           	movel %a4,%d0
    34aa:	5e80           	addql #7,%d0
    34ac:	78f8           	moveq #-8,%d4
    34ae:	c084           	andl %d4,%d0
    34b0:	41ec ffe0      	lea %a4@(-32),%a0
    34b4:	2400           	movel %d0,%d2
    34b6:	9488           	subl %a0,%d2
    34b8:	5382           	subql #1,%d2
    34ba:	7aff           	moveq #-1,%d5
    34bc:	ba82           	cmpl %d2,%d5
    34be:	673c           	beqs 34fc <Exec_FreeMem+0x106>
    34c0:	0c12 00db      	cmpib #-37,%a2@
    34c4:	672a           	beqs 34f0 <Exec_FreeMem+0xfa>
    34c6:	2f09           	movel %a1,%sp@-
    34c8:	42a7           	clrl %sp@-
    34ca:	2069 fedc      	moveal %a1@(-292),%a0
    34ce:	2d49 fffc      	movel %a1,%fp@(-4)
    34d2:	4e90           	jsr %a0@
    34d4:	2f28 0008      	movel %a0@(8),%sp@-
    34d8:	2f08           	movel %a0,%sp@-
    34da:	2f0a           	movel %a2,%sp@-
    34dc:	4879 0000 0000 	pea 0 <main>
    34e2:	61ff 0000 0000 	bsrl 34e4 <Exec_FreeMem+0xee>
    34e8:	4fef 0018      	lea %sp@(24),%sp
    34ec:	226e fffc      	moveal %fp@(-4),%a1
    34f0:	528a           	addql #1,%a2
    34f2:	51ca ffcc      	dbf %d2,34c0 <Exec_FreeMem+0xca>
    34f6:	4242           	clrw %d2
    34f8:	5382           	subql #1,%d2
    34fa:	64c4           	bccs 34c0 <Exec_FreeMem+0xca>
    34fc:	284b           	moveal %a3,%a4
    34fe:	240c           	movel %a4,%d2
    3500:	d483           	addl %d3,%d2
    3502:	2f09           	movel %a1,%sp@-
    3504:	2069 ff7e      	moveal %a1@(-130),%a0
    3508:	2d49 fffc      	movel %a1,%fp@(-4)
    350c:	4e90           	jsr %a0@
    350e:	226e fffc      	moveal %fp@(-4),%a1
    3512:	2c29 0142      	movel %a1@(322),%d6
    3516:	588f           	addql #4,%sp
    3518:	6000 0114      	braw 362e <Exec_FreeMem+0x238>
    351c:	2a46           	moveal %d6,%a5
    351e:	b7ed 0014      	cmpal %a5@(20),%a3
    3522:	6500 0106      	bcsw 362a <Exec_FreeMem+0x234>
    3526:	202d 0018      	movel %a5@(24),%d0
    352a:	b7c0           	cmpal %d0,%a3
    352c:	6400 00fc      	bccw 362a <Exec_FreeMem+0x234>
    3530:	b082           	cmpl %d2,%d0
    3532:	6418           	bccs 354c <Exec_FreeMem+0x156>
    3534:	2f09           	movel %a1,%sp@-
    3536:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    353c:	2069 ff96      	moveal %a1@(-106),%a0
    3540:	2d49 fffc      	movel %a1,%fp@(-4)
    3544:	4e90           	jsr %a0@
    3546:	508f           	addql #8,%sp
    3548:	226e fffc      	moveal %fp@(-4),%a1
    354c:	2a46           	moveal %d6,%a5
    354e:	47ed 0010      	lea %a5@(16),%a3
    3552:	2453           	moveal %a3@,%a2
    3554:	b4fc 0000      	cmpaw #0,%a2
    3558:	660c           	bnes 3566 <Exec_FreeMem+0x170>
    355a:	2943 0004      	movel %d3,%a4@(4)
    355e:	4294           	clrl %a4@
    3560:	268c           	movel %a4,%a3@
    3562:	6000 00b8      	braw 361c <Exec_FreeMem+0x226>
    3566:	222a 0004      	movel %a2@(4),%d1
    356a:	200a           	movel %a2,%d0
    356c:	8081           	orl %d1,%d0
    356e:	7807           	moveq #7,%d4
    3570:	c084           	andl %d4,%d0
    3572:	660e           	bnes 3582 <Exec_FreeMem+0x18c>
    3574:	200a           	movel %a2,%d0
    3576:	d081           	addl %d1,%d0
    3578:	2212           	movel %a2@,%d1
    357a:	b280           	cmpl %d0,%d1
    357c:	621c           	bhis 359a <Exec_FreeMem+0x1a4>
    357e:	4a81           	tstl %d1
    3580:	6718           	beqs 359a <Exec_FreeMem+0x1a4>
    3582:	2f09           	movel %a1,%sp@-
    3584:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    358a:	2069 ff96      	moveal %a1@(-106),%a0
    358e:	2d49 fffc      	movel %a1,%fp@(-4)
    3592:	4e90           	jsr %a0@
    3594:	508f           	addql #8,%sp
    3596:	226e fffc      	moveal %fp@(-4),%a1
    359a:	b9ca           	cmpal %a2,%a4
    359c:	621e           	bhis 35bc <Exec_FreeMem+0x1c6>
    359e:	b5c2           	cmpal %d2,%a2
    35a0:	6424           	bccs 35c6 <Exec_FreeMem+0x1d0>
    35a2:	2f09           	movel %a1,%sp@-
    35a4:	2f3c 8100 0009 	movel #-2130706423,%sp@-
    35aa:	2069 ff96      	moveal %a1@(-106),%a0
    35ae:	2d49 fffc      	movel %a1,%fp@(-4)
    35b2:	4e90           	jsr %a0@
    35b4:	508f           	addql #8,%sp
    35b6:	226e fffc      	moveal %fp@(-4),%a1
    35ba:	600a           	bras 35c6 <Exec_FreeMem+0x1d0>
    35bc:	264a           	moveal %a2,%a3
    35be:	2452           	moveal %a2@,%a2
    35c0:	b4fc 0000      	cmpaw #0,%a2
    35c4:	66a0           	bnes 3566 <Exec_FreeMem+0x170>
    35c6:	2a46           	moveal %d6,%a5
    35c8:	41ed 0010      	lea %a5@(16),%a0
    35cc:	b1cb           	cmpal %a3,%a0
    35ce:	6730           	beqs 3600 <Exec_FreeMem+0x20a>
    35d0:	200b           	movel %a3,%d0
    35d2:	d0ab 0004      	addl %a3@(4),%d0
    35d6:	b9c0           	cmpal %d0,%a4
    35d8:	6418           	bccs 35f2 <Exec_FreeMem+0x1fc>
    35da:	2f09           	movel %a1,%sp@-
    35dc:	2f3c 8100 0009 	movel #-2130706423,%sp@-
    35e2:	2069 ff96      	moveal %a1@(-106),%a0
    35e6:	2d49 fffc      	movel %a1,%fp@(-4)
    35ea:	4e90           	jsr %a0@
    35ec:	508f           	addql #8,%sp
    35ee:	226e fffc      	moveal %fp@(-4),%a1
    35f2:	200b           	movel %a3,%d0
    35f4:	d0ab 0004      	addl %a3@(4),%d0
    35f8:	b9c0           	cmpal %d0,%a4
    35fa:	6604           	bnes 3600 <Exec_FreeMem+0x20a>
    35fc:	284b           	moveal %a3,%a4
    35fe:	6002           	bras 3602 <Exec_FreeMem+0x20c>
    3600:	268c           	movel %a4,%a3@
    3602:	b5c2           	cmpal %d2,%a2
    3604:	660c           	bnes 3612 <Exec_FreeMem+0x21c>
    3606:	4a82           	tstl %d2
    3608:	6708           	beqs 3612 <Exec_FreeMem+0x21c>
    360a:	2a42           	moveal %d2,%a5
    360c:	d4ad 0004      	addl %a5@(4),%d2
    3610:	2452           	moveal %a2@,%a2
    3612:	288a           	movel %a2,%a4@
    3614:	948c           	subl %a4,%d2
    3616:	2942 0004      	movel %d2,%a4@(4)
    361a:	2a46           	moveal %d6,%a5
    361c:	d7ad 001c      	addl %d3,%a5@(28)
    3620:	2f09           	movel %a1,%sp@-
    3622:	2069 ff78      	moveal %a1@(-136),%a0
    3626:	4e90           	jsr %a0@
    3628:	601a           	bras 3644 <Exec_FreeMem+0x24e>
    362a:	2a46           	moveal %d6,%a5
    362c:	2c15           	movel %a5@,%d6
    362e:	2a46           	moveal %d6,%a5
    3630:	4a95           	tstl %a5@
    3632:	6600 fee8      	bnew 351c <Exec_FreeMem+0x126>
    3636:	2f09           	movel %a1,%sp@-
    3638:	2f3c 8100 0005 	movel #-2130706427,%sp@-
    363e:	2069 ff96      	moveal %a1@(-106),%a0
    3642:	4e90           	jsr %a0@
    3644:	4cee 3c7c ffd8 	moveml %fp@(-40),%d2-%d6/%a2-%a5
    364a:	4e5e           	unlk %fp
    364c:	4e75           	rts

0000364e <Exec_FreePooled>:
    364e:	4e56 0000      	linkw %fp,#0
    3652:	48e7 003c      	moveml %a2-%a5,%sp@-
    3656:	206e 000c      	moveal %fp@(12),%a0
    365a:	2a6e 0010      	moveal %fp@(16),%a5
    365e:	286e 0014      	moveal %fp@(20),%a4
    3662:	266e 0008      	moveal %fp@(8),%a3
    3666:	bbeb 0020      	cmpal %a3@(32),%a5
    366a:	6338           	blss 36a4 <Exec_FreePooled+0x56>
    366c:	45e8 fff0      	lea %a0@(-16),%a2
    3670:	2f0a           	movel %a2,%sp@-
    3672:	206c ff06      	moveal %a4@(-250),%a0
    3676:	4e90           	jsr %a0@
    3678:	41ed 0010      	lea %a5@(16),%a0
    367c:	226a 0008      	moveal %a2@(8),%a1
    3680:	588f           	addql #4,%sp
    3682:	b1c9           	cmpal %a1,%a0
    3684:	6716           	beqs 369c <Exec_FreePooled+0x4e>
    3686:	2f0d           	movel %a5,%sp@-
    3688:	4869 fff0      	pea %a1@(-16)
    368c:	4879 0000 0000 	pea 0 <main>
    3692:	61ff 0000 0000 	bsrl 3694 <Exec_FreePooled+0x46>
    3698:	4fef 000c      	lea %sp@(12),%sp
    369c:	2f0c           	movel %a4,%sp@-
    369e:	2f2a 0008      	movel %a2@(8),%sp@-
    36a2:	603c           	bras 36e0 <Exec_FreePooled+0x92>
    36a4:	2453           	moveal %a3@,%a2
    36a6:	b1ea 0014      	cmpal %a2@(20),%a0
    36aa:	653e           	bcss 36ea <Exec_FreePooled+0x9c>
    36ac:	b1ea 0018      	cmpal %a2@(24),%a0
    36b0:	6438           	bccs 36ea <Exec_FreePooled+0x9c>
    36b2:	2f0c           	movel %a4,%sp@-
    36b4:	2f0d           	movel %a5,%sp@-
    36b6:	2f08           	movel %a0,%sp@-
    36b8:	2f0a           	movel %a2,%sp@-
    36ba:	206c ff42      	moveal %a4@(-190),%a0
    36be:	4e90           	jsr %a0@
    36c0:	4fef 0010      	lea %sp@(16),%sp
    36c4:	222a 001c      	movel %a2@(28),%d1
    36c8:	b2ab 001c      	cmpl %a3@(28),%d1
    36cc:	6620           	bnes 36ee <Exec_FreePooled+0xa0>
    36ce:	2f0a           	movel %a2,%sp@-
    36d0:	206c ff06      	moveal %a4@(-250),%a0
    36d4:	4e90           	jsr %a0@
    36d6:	2f0c           	movel %a4,%sp@-
    36d8:	7220           	moveq #32,%d1
    36da:	d2ab 001c      	addl %a3@(28),%d1
    36de:	2f01           	movel %d1,%sp@-
    36e0:	2f0a           	movel %a2,%sp@-
    36e2:	206c ff30      	moveal %a4@(-208),%a0
    36e6:	4e90           	jsr %a0@
    36e8:	6004           	bras 36ee <Exec_FreePooled+0xa0>
    36ea:	2452           	moveal %a2@,%a2
    36ec:	60b8           	bras 36a6 <Exec_FreePooled+0x58>
    36ee:	4cee 3c00 fff0 	moveml %fp@(-16),%a2-%a5
    36f4:	4e5e           	unlk %fp
    36f6:	4e75           	rts

000036f8 <Exec_FreeSignal>:
    36f8:	4e56 0000      	linkw %fp,#0
    36fc:	2f0a           	movel %a2,%sp@-
    36fe:	2f02           	movel %d2,%sp@-
    3700:	242e 0008      	movel %fp@(8),%d2
    3704:	246e 000c      	moveal %fp@(12),%a2
    3708:	72ff           	moveq #-1,%d1
    370a:	b282           	cmpl %d2,%d1
    370c:	671c           	beqs 372a <Exec_FreeSignal+0x32>
    370e:	2f0a           	movel %a2,%sp@-
    3710:	206a ff7e      	moveal %a2@(-130),%a0
    3714:	4e90           	jsr %a0@
    3716:	206a 0114      	moveal %a2@(276),%a0
    371a:	70fe           	moveq #-2,%d0
    371c:	e5b8           	roll %d2,%d0
    371e:	c1a8 0012      	andl %d0,%a0@(18)
    3722:	2f0a           	movel %a2,%sp@-
    3724:	206a ff78      	moveal %a2@(-136),%a0
    3728:	4e90           	jsr %a0@
    372a:	242e fff8      	movel %fp@(-8),%d2
    372e:	246e fffc      	moveal %fp@(-4),%a2
    3732:	4e5e           	unlk %fp
    3734:	4e75           	rts

00003736 <Exec_FreeTrap>:
    3736:	4e56 0000      	linkw %fp,#0
    373a:	4879 0000 0000 	pea 0 <main>
    3740:	4879 0000 0000 	pea 0 <main>
    3746:	61ff 0000 0000 	bsrl 3748 <Exec_FreeTrap+0x12>
    374c:	4e5e           	unlk %fp
    374e:	4e75           	rts

00003750 <Exec_FreeVec>:
    3750:	4e56 0000      	linkw %fp,#0
    3754:	202e 0008      	movel %fp@(8),%d0
    3758:	226e 000c      	moveal %fp@(12),%a1
    375c:	6710           	beqs 376e <Exec_FreeVec+0x1e>
    375e:	2040           	moveal %d0,%a0
    3760:	5188           	subql #8,%a0
    3762:	2f09           	movel %a1,%sp@-
    3764:	2f10           	movel %a0@,%sp@-
    3766:	2f08           	movel %a0,%sp@-
    3768:	2069 ff30      	moveal %a1@(-208),%a0
    376c:	4e90           	jsr %a0@
    376e:	4e5e           	unlk %fp
    3770:	4e75           	rts

00003772 <Exec_GetCC>:
    3772:	4e56 0000      	linkw %fp,#0
    3776:	4879 0000 0000 	pea 0 <main>
    377c:	4879 0000 0000 	pea 0 <main>
    3782:	61ff 0000 0000 	bsrl 3784 <Exec_GetCC+0x12>
    3788:	7000           	moveq #0,%d0
    378a:	4640           	notw %d0
    378c:	4e5e           	unlk %fp
    378e:	4e75           	rts

00003790 <Exec_GetMsg>:
    3790:	4e56 0000      	linkw %fp,#0
    3794:	48e7 2030      	moveml %d2/%a2-%a3,%sp@-
    3798:	266e 0008      	moveal %fp@(8),%a3
    379c:	246e 000c      	moveal %fp@(12),%a2
    37a0:	b7fc 0000 0400 	cmpal #1024,%a3
    37a6:	6f10           	bles 37b8 <Exec_GetMsg+0x28>
    37a8:	2f0a           	movel %a2,%sp@-
    37aa:	2f0b           	movel %a3,%sp@-
    37ac:	206a fdec      	moveal %a2@(-532),%a0
    37b0:	4e90           	jsr %a0@
    37b2:	508f           	addql #8,%sp
    37b4:	4a80           	tstl %d0
    37b6:	6622           	bnes 37da <Exec_GetMsg+0x4a>
    37b8:	2f0b           	movel %a3,%sp@-
    37ba:	4879 0000 0000 	pea 0 <main>
    37c0:	4878 0037      	pea 37 <pause+0x17>
    37c4:	4879 0000 0000 	pea 0 <main>
    37ca:	4879 0000 0000 	pea 0 <main>
    37d0:	61ff 0000 0000 	bsrl 37d2 <Exec_GetMsg+0x42>
    37d6:	4fef 0014      	lea %sp@(20),%sp
    37da:	2f0a           	movel %a2,%sp@-
    37dc:	206a ff8a      	moveal %a2@(-118),%a0
    37e0:	4e90           	jsr %a0@
    37e2:	486b 0014      	pea %a3@(20)
    37e6:	206a ff00      	moveal %a2@(-256),%a0
    37ea:	4e90           	jsr %a0@
    37ec:	2408           	movel %a0,%d2
    37ee:	2f0a           	movel %a2,%sp@-
    37f0:	206a ff84      	moveal %a2@(-124),%a0
    37f4:	4e90           	jsr %a0@
    37f6:	4fef 000c      	lea %sp@(12),%sp
    37fa:	4a82           	tstl %d2
    37fc:	6736           	beqs 3834 <Exec_GetMsg+0xa4>
    37fe:	0c82 0000 0400 	cmpil #1024,%d2
    3804:	6f10           	bles 3816 <Exec_GetMsg+0x86>
    3806:	2f0a           	movel %a2,%sp@-
    3808:	2f02           	movel %d2,%sp@-
    380a:	206a fdec      	moveal %a2@(-532),%a0
    380e:	4e90           	jsr %a0@
    3810:	508f           	addql #8,%sp
    3812:	4a80           	tstl %d0
    3814:	661e           	bnes 3834 <Exec_GetMsg+0xa4>
    3816:	2f02           	movel %d2,%sp@-
    3818:	4879 0000 0000 	pea 0 <main>
    381e:	4878 0042      	pea 42 <pause+0x22>
    3822:	4879 0000 0000 	pea 0 <main>
    3828:	4879 0000 0000 	pea 0 <main>
    382e:	61ff 0000 0000 	bsrl 3830 <Exec_GetMsg+0xa0>
    3834:	2042           	moveal %d2,%a0
    3836:	2008           	movel %a0,%d0
    3838:	4cee 0c04 fff4 	moveml %fp@(-12),%d2/%a2-%a3
    383e:	4e5e           	unlk %fp
    3840:	4e75           	rts

00003842 <Exec_InitCode>:
    3842:	4e56 0000      	linkw %fp,#0
    3846:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    384a:	262e 0008      	movel %fp@(8),%d3
    384e:	242e 000c      	movel %fp@(12),%d2
    3852:	266e 0010      	moveal %fp@(16),%a3
    3856:	246b 012c      	moveal %a3@(300),%a2
    385a:	b4fc 0000      	cmpaw #0,%a2
    385e:	6738           	beqs 3898 <Exec_InitCode+0x56>
    3860:	4a92           	tstl %a2@
    3862:	6734           	beqs 3898 <Exec_InitCode+0x56>
    3864:	2012           	movel %a2@,%d0
    3866:	6c0a           	bges 3872 <Exec_InitCode+0x30>
    3868:	223c 7fff ffff 	movel #2147483647,%d1
    386e:	c280           	andl %d0,%d1
    3870:	2441           	moveal %d1,%a2
    3872:	2052           	moveal %a2@,%a0
    3874:	b428 000b      	cmpb %a0@(11),%d2
    3878:	6218           	bhis 3892 <Exec_InitCode+0x50>
    387a:	1003           	moveb %d3,%d0
    387c:	c028 000a      	andb %a0@(10),%d0
    3880:	6710           	beqs 3892 <Exec_InitCode+0x50>
    3882:	2f0b           	movel %a3,%sp@-
    3884:	42a7           	clrl %sp@-
    3886:	2f08           	movel %a0,%sp@-
    3888:	206b ff9c      	moveal %a3@(-100),%a0
    388c:	4e90           	jsr %a0@
    388e:	4fef 000c      	lea %sp@(12),%sp
    3892:	588a           	addql #4,%a2
    3894:	4a92           	tstl %a2@
    3896:	66cc           	bnes 3864 <Exec_InitCode+0x22>
    3898:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    389e:	4e5e           	unlk %fp
    38a0:	4e75           	rts

000038a2 <Exec_InitResident>:
    38a2:	4e56 0000      	linkw %fp,#0
    38a6:	48e7 203c      	moveml %d2/%a2-%a5,%sp@-
    38aa:	206e 0008      	moveal %fp@(8),%a0
    38ae:	242e 000c      	movel %fp@(12),%d2
    38b2:	2a6e 0010      	moveal %fp@(16),%a5
    38b6:	0c50 4afc      	cmpiw #19196,%a0@
    38ba:	6608           	bnes 38c4 <Exec_InitResident+0x22>
    38bc:	2668 0002      	moveal %a0@(2),%a3
    38c0:	b1cb           	cmpal %a3,%a0
    38c2:	6706           	beqs 38ca <Exec_InitResident+0x28>
    38c4:	91c8           	subal %a0,%a0
    38c6:	6000 00bc      	braw 3984 <Exec_InitResident+0xe2>
    38ca:	4a2b 000a      	tstb %a3@(10)
    38ce:	6c00 00a8      	bgew 3978 <Exec_InitResident+0xd6>
    38d2:	286b 0016      	moveal %a3@(22),%a4
    38d6:	2f0d           	movel %a5,%sp@-
    38d8:	2f02           	movel %d2,%sp@-
    38da:	2f14           	movel %a4@,%sp@-
    38dc:	42a7           	clrl %sp@-
    38de:	2f2c 0008      	movel %a4@(8),%sp@-
    38e2:	2f2c 0004      	movel %a4@(4),%sp@-
    38e6:	206d ffae      	moveal %a5@(-82),%a0
    38ea:	4e90           	jsr %a0@
    38ec:	2448           	moveal %a0,%a2
    38ee:	4fef 0018      	lea %sp@(24),%sp
    38f2:	b4fc 0000      	cmpaw #0,%a2
    38f6:	677c           	beqs 3974 <Exec_InitResident+0xd2>
    38f8:	156b 000c 000c 	moveb %a3@(12),%a2@(12)
    38fe:	256b 000e 0008 	movel %a3@(14),%a2@(8)
    3904:	4240           	clrw %d0
    3906:	102b 000b      	moveb %a3@(11),%d0
    390a:	3540 0014      	movew %d0,%a2@(20)
    390e:	256b 0012 0018 	movel %a3@(18),%a2@(24)
    3914:	157c 0006 000e 	moveb #6,%a2@(14)
    391a:	206c 000c      	moveal %a4@(12),%a0
    391e:	b0fc 0000      	cmpaw #0,%a0
    3922:	670e           	beqs 3932 <Exec_InitResident+0x90>
    3924:	2f0d           	movel %a5,%sp@-
    3926:	2f02           	movel %d2,%sp@-
    3928:	2f0a           	movel %a2,%sp@-
    392a:	4e90           	jsr %a0@
    392c:	2448           	moveal %a0,%a2
    392e:	4fef 000c      	lea %sp@(12),%sp
    3932:	b4fc 0000      	cmpaw #0,%a2
    3936:	673c           	beqs 3974 <Exec_InitResident+0xd2>
    3938:	4280           	clrl %d0
    393a:	102b 000c      	moveb %a3@(12),%d0
    393e:	7208           	moveq #8,%d1
    3940:	b280           	cmpl %d0,%d1
    3942:	6726           	beqs 396a <Exec_InitResident+0xc8>
    3944:	6d08           	blts 394e <Exec_InitResident+0xac>
    3946:	7203           	moveq #3,%d1
    3948:	b280           	cmpl %d0,%d1
    394a:	670a           	beqs 3956 <Exec_InitResident+0xb4>
    394c:	6026           	bras 3974 <Exec_InitResident+0xd2>
    394e:	7209           	moveq #9,%d1
    3950:	b280           	cmpl %d0,%d1
    3952:	670c           	beqs 3960 <Exec_InitResident+0xbe>
    3954:	601e           	bras 3974 <Exec_InitResident+0xd2>
    3956:	2f0d           	movel %a5,%sp@-
    3958:	2f0a           	movel %a2,%sp@-
    395a:	206d fe52      	moveal %a5@(-430),%a0
    395e:	6012           	bras 3972 <Exec_InitResident+0xd0>
    3960:	2f0d           	movel %a5,%sp@-
    3962:	2f0a           	movel %a2,%sp@-
    3964:	206d fe76      	moveal %a5@(-394),%a0
    3968:	6008           	bras 3972 <Exec_InitResident+0xd0>
    396a:	2f0d           	movel %a5,%sp@-
    396c:	2f0a           	movel %a2,%sp@-
    396e:	206d fe1c      	moveal %a5@(-484),%a0
    3972:	4e90           	jsr %a0@
    3974:	204a           	moveal %a2,%a0
    3976:	600c           	bras 3984 <Exec_InitResident+0xe2>
    3978:	2f0d           	movel %a5,%sp@-
    397a:	2f02           	movel %d2,%sp@-
    397c:	42a7           	clrl %sp@-
    397e:	206b 0016      	moveal %a3@(22),%a0
    3982:	4e90           	jsr %a0@
    3984:	2008           	movel %a0,%d0
    3986:	4cee 3c04 ffec 	moveml %fp@(-20),%d2/%a2-%a5
    398c:	4e5e           	unlk %fp
    398e:	4e75           	rts

00003990 <Exec_InitSemaphore>:
    3990:	4e56 0000      	linkw %fp,#0
    3994:	2f0a           	movel %a2,%sp@-
    3996:	206e 0008      	moveal %fp@(8),%a0
    399a:	43e8 0014      	lea %a0@(20),%a1
    399e:	2149 0010      	movel %a1,%a0@(16)
    39a2:	4291           	clrl %a1@
    39a4:	45e8 0010      	lea %a0@(16),%a2
    39a8:	214a 0018      	movel %a2,%a0@(24)
    39ac:	117c 000f 000c 	moveb #15,%a0@(12)
    39b2:	4268 000e      	clrw %a0@(14)
    39b6:	42a8 0028      	clrl %a0@(40)
    39ba:	317c ffff 002c 	movew #-1,%a0@(44)
    39c0:	245f           	moveal %sp@+,%a2
    39c2:	4e5e           	unlk %fp
    39c4:	4e75           	rts

000039c6 <Exec_InitStruct>:
    39c6:	4e56 0000      	linkw %fp,#0
    39ca:	48e7 3e20      	moveml %d2-%d6/%a2,%sp@-
    39ce:	2a2e 000c      	movel %fp@(12),%d5
    39d2:	202e 0010      	movel %fp@(16),%d0
    39d6:	246e 0014      	moveal %fp@(20),%a2
    39da:	7800           	moveq #0,%d4
    39dc:	2200           	movel %d0,%d1
    39de:	e489           	lsrl #2,%d1
    39e0:	7c03           	moveq #3,%d6
    39e2:	c086           	andl %d6,%d0
    39e4:	2245           	moveal %d5,%a1
    39e6:	4a81           	tstl %d1
    39e8:	6706           	beqs 39f0 <Exec_InitStruct+0x2a>
    39ea:	4299           	clrl %a1@+
    39ec:	5381           	subql #1,%d1
    39ee:	66fa           	bnes 39ea <Exec_InitStruct+0x24>
    39f0:	2200           	movel %d0,%d1
    39f2:	6706           	beqs 39fa <Exec_InitStruct+0x34>
    39f4:	4219           	clrb %a1@+
    39f6:	5381           	subql #1,%d1
    39f8:	66fa           	bnes 39f4 <Exec_InitStruct+0x2e>
    39fa:	206e 0008      	moveal %fp@(8),%a0
    39fe:	2245           	moveal %d5,%a1
    3a00:	4a10           	tstb %a0@
    3a02:	6700 0120      	beqw 3b24 <Exec_InitStruct+0x15e>
    3a06:	7600           	moveq #0,%d3
    3a08:	2403           	movel %d3,%d2
    3a0a:	1210           	moveb %a0@,%d1
    3a0c:	1001           	moveb %d1,%d0
    3a0e:	ec08           	lsrb #6,%d0
    3a10:	1600           	moveb %d0,%d3
    3a12:	1001           	moveb %d1,%d0
    3a14:	e808           	lsrb #4,%d0
    3a16:	0200 0003      	andib #3,%d0
    3a1a:	1400           	moveb %d0,%d2
    3a1c:	7c0f           	moveq #15,%d6
    3a1e:	c286           	andl %d6,%d1
    3a20:	7c02           	moveq #2,%d6
    3a22:	bc83           	cmpl %d3,%d6
    3a24:	6712           	beqs 3a38 <Exec_InitStruct+0x72>
    3a26:	6c08           	bges 3a30 <Exec_InitStruct+0x6a>
    3a28:	7c03           	moveq #3,%d6
    3a2a:	bc83           	cmpl %d3,%d6
    3a2c:	6712           	beqs 3a40 <Exec_InitStruct+0x7a>
    3a2e:	6018           	bras 3a48 <Exec_InitStruct+0x82>
    3a30:	4a83           	tstl %d3
    3a32:	6d14           	blts 3a48 <Exec_InitStruct+0x82>
    3a34:	5288           	addql #1,%a0
    3a36:	6010           	bras 3a48 <Exec_InitStruct+0x82>
    3a38:	5288           	addql #1,%a0
    3a3a:	7800           	moveq #0,%d4
    3a3c:	1818           	moveb %a0@+,%d4
    3a3e:	6008           	bras 3a48 <Exec_InitStruct+0x82>
    3a40:	283c 00ff ffff 	movel #16777215,%d4
    3a46:	c898           	andl %a0@+,%d4
    3a48:	7c01           	moveq #1,%d6
    3a4a:	bc82           	cmpl %d2,%d6
    3a4c:	6722           	beqs 3a70 <Exec_InitStruct+0xaa>
    3a4e:	6d06           	blts 3a56 <Exec_InitStruct+0x90>
    3a50:	4a82           	tstl %d2
    3a52:	670a           	beqs 3a5e <Exec_InitStruct+0x98>
    3a54:	6030           	bras 3a86 <Exec_InitStruct+0xc0>
    3a56:	7c02           	moveq #2,%d6
    3a58:	bc82           	cmpl %d2,%d6
    3a5a:	673c           	beqs 3a98 <Exec_InitStruct+0xd2>
    3a5c:	6028           	bras 3a86 <Exec_InitStruct+0xc0>
    3a5e:	2008           	movel %a0,%d0
    3a60:	5680           	addql #3,%d0
    3a62:	7cfc           	moveq #-4,%d6
    3a64:	cc80           	andl %d0,%d6
    3a66:	2046           	moveal %d6,%a0
    3a68:	2009           	movel %a1,%d0
    3a6a:	5680           	addql #3,%d0
    3a6c:	7cfc           	moveq #-4,%d6
    3a6e:	6010           	bras 3a80 <Exec_InitStruct+0xba>
    3a70:	2008           	movel %a0,%d0
    3a72:	5280           	addql #1,%d0
    3a74:	7cfe           	moveq #-2,%d6
    3a76:	cc80           	andl %d0,%d6
    3a78:	2046           	moveal %d6,%a0
    3a7a:	2009           	movel %a1,%d0
    3a7c:	5280           	addql #1,%d0
    3a7e:	7cfe           	moveq #-2,%d6
    3a80:	cc80           	andl %d0,%d6
    3a82:	2246           	moveal %d6,%a1
    3a84:	6012           	bras 3a98 <Exec_InitStruct+0xd2>
    3a86:	2f0a           	movel %a2,%sp@-
    3a88:	2f3c 8000 0003 	movel #-2147483645,%sp@-
    3a8e:	206a ff96      	moveal %a2@(-106),%a0
    3a92:	4e90           	jsr %a0@
    3a94:	6000 008e      	braw 3b24 <Exec_InitStruct+0x15e>
    3a98:	7c01           	moveq #1,%d6
    3a9a:	bc83           	cmpl %d3,%d6
    3a9c:	6740           	beqs 3ade <Exec_InitStruct+0x118>
    3a9e:	6d06           	blts 3aa6 <Exec_InitStruct+0xe0>
    3aa0:	4a83           	tstl %d3
    3aa2:	670c           	beqs 3ab0 <Exec_InitStruct+0xea>
    3aa4:	606e           	bras 3b14 <Exec_InitStruct+0x14e>
    3aa6:	7c03           	moveq #3,%d6
    3aa8:	bc83           	cmpl %d3,%d6
    3aaa:	6d68           	blts 3b14 <Exec_InitStruct+0x14e>
    3aac:	2245           	moveal %d5,%a1
    3aae:	d3c4           	addal %d4,%a1
    3ab0:	7c01           	moveq #1,%d6
    3ab2:	bc82           	cmpl %d2,%d6
    3ab4:	6718           	beqs 3ace <Exec_InitStruct+0x108>
    3ab6:	6d06           	blts 3abe <Exec_InitStruct+0xf8>
    3ab8:	4a82           	tstl %d2
    3aba:	670a           	beqs 3ac6 <Exec_InitStruct+0x100>
    3abc:	6056           	bras 3b14 <Exec_InitStruct+0x14e>
    3abe:	7c02           	moveq #2,%d6
    3ac0:	bc82           	cmpl %d2,%d6
    3ac2:	6712           	beqs 3ad6 <Exec_InitStruct+0x110>
    3ac4:	604e           	bras 3b14 <Exec_InitStruct+0x14e>
    3ac6:	22d8           	movel %a0@+,%a1@+
    3ac8:	5381           	subql #1,%d1
    3aca:	6afa           	bpls 3ac6 <Exec_InitStruct+0x100>
    3acc:	6046           	bras 3b14 <Exec_InitStruct+0x14e>
    3ace:	32d8           	movew %a0@+,%a1@+
    3ad0:	5381           	subql #1,%d1
    3ad2:	6afa           	bpls 3ace <Exec_InitStruct+0x108>
    3ad4:	603e           	bras 3b14 <Exec_InitStruct+0x14e>
    3ad6:	12d8           	moveb %a0@+,%a1@+
    3ad8:	5381           	subql #1,%d1
    3ada:	6afa           	bpls 3ad6 <Exec_InitStruct+0x110>
    3adc:	6036           	bras 3b14 <Exec_InitStruct+0x14e>
    3ade:	7c01           	moveq #1,%d6
    3ae0:	bc82           	cmpl %d2,%d6
    3ae2:	671a           	beqs 3afe <Exec_InitStruct+0x138>
    3ae4:	6d06           	blts 3aec <Exec_InitStruct+0x126>
    3ae6:	4a82           	tstl %d2
    3ae8:	670a           	beqs 3af4 <Exec_InitStruct+0x12e>
    3aea:	6028           	bras 3b14 <Exec_InitStruct+0x14e>
    3aec:	7c02           	moveq #2,%d6
    3aee:	bc82           	cmpl %d2,%d6
    3af0:	6718           	beqs 3b0a <Exec_InitStruct+0x144>
    3af2:	6020           	bras 3b14 <Exec_InitStruct+0x14e>
    3af4:	2018           	movel %a0@+,%d0
    3af6:	22c0           	movel %d0,%a1@+
    3af8:	5381           	subql #1,%d1
    3afa:	6afa           	bpls 3af6 <Exec_InitStruct+0x130>
    3afc:	6016           	bras 3b14 <Exec_InitStruct+0x14e>
    3afe:	3018           	movew %a0@+,%d0
    3b00:	48c0           	extl %d0
    3b02:	32c0           	movew %d0,%a1@+
    3b04:	5381           	subql #1,%d1
    3b06:	6afa           	bpls 3b02 <Exec_InitStruct+0x13c>
    3b08:	600a           	bras 3b14 <Exec_InitStruct+0x14e>
    3b0a:	7000           	moveq #0,%d0
    3b0c:	1018           	moveb %a0@+,%d0
    3b0e:	12c0           	moveb %d0,%a1@+
    3b10:	5381           	subql #1,%d1
    3b12:	6afa           	bpls 3b0e <Exec_InitStruct+0x148>
    3b14:	2008           	movel %a0,%d0
    3b16:	5280           	addql #1,%d0
    3b18:	7cfe           	moveq #-2,%d6
    3b1a:	cc80           	andl %d0,%d6
    3b1c:	2046           	moveal %d6,%a0
    3b1e:	4a10           	tstb %a0@
    3b20:	6600 fee8      	bnew 3a0a <Exec_InitStruct+0x44>
    3b24:	4cee 047c ffe8 	moveml %fp@(-24),%d2-%d6/%a2
    3b2a:	4e5e           	unlk %fp
    3b2c:	4e75           	rts

00003b2e <Exec_Insert>:
    3b2e:	4e56 0000      	linkw %fp,#0
    3b32:	48e7 0038      	moveml %a2-%a4,%sp@-
    3b36:	246e 0008      	moveal %fp@(8),%a2
    3b3a:	226e 000c      	moveal %fp@(12),%a1
    3b3e:	266e 0010      	moveal %fp@(16),%a3
    3b42:	b6fc 0000      	cmpaw #0,%a3
    3b46:	672c           	beqs 3b74 <Exec_Insert+0x46>
    3b48:	2013           	movel %a3@,%d0
    3b4a:	6710           	beqs 3b5c <Exec_Insert+0x2e>
    3b4c:	2280           	movel %d0,%a1@
    3b4e:	234b 0004      	movel %a3,%a1@(4)
    3b52:	2053           	moveal %a3@,%a0
    3b54:	2149 0004      	movel %a1,%a0@(4)
    3b58:	2689           	movel %a1,%a3@
    3b5a:	6026           	bras 3b82 <Exec_Insert+0x54>
    3b5c:	49ea 0004      	lea %a2@(4),%a4
    3b60:	228c           	movel %a4,%a1@
    3b62:	236a 0008 0004 	movel %a2@(8),%a1@(4)
    3b68:	206a 0008      	moveal %a2@(8),%a0
    3b6c:	2089           	movel %a1,%a0@
    3b6e:	2549 0008      	movel %a1,%a2@(8)
    3b72:	600e           	bras 3b82 <Exec_Insert+0x54>
    3b74:	2292           	movel %a2@,%a1@
    3b76:	234a 0004      	movel %a2,%a1@(4)
    3b7a:	2052           	moveal %a2@,%a0
    3b7c:	2149 0004      	movel %a1,%a0@(4)
    3b80:	2489           	movel %a1,%a2@
    3b82:	4cdf 1c00      	moveml %sp@+,%a2-%a4
    3b86:	4e5e           	unlk %fp
    3b88:	4e75           	rts

00003b8a <Exec_MakeFunctions>:
    3b8a:	4e56 0000      	linkw %fp,#0
    3b8e:	48e7 2038      	moveml %d2/%a2-%a4,%sp@-
    3b92:	266e 0008      	moveal %fp@(8),%a3
    3b96:	206e 000c      	moveal %fp@(12),%a0
    3b9a:	202e 0010      	movel %fp@(16),%d0
    3b9e:	286e 0014      	moveal %fp@(20),%a4
    3ba2:	7401           	moveq #1,%d2
    3ba4:	4a80           	tstl %d0
    3ba6:	6732           	beqs 3bda <Exec_MakeFunctions+0x50>
    3ba8:	2448           	moveal %a0,%a2
    3baa:	0c52 ffff      	cmpiw #-1,%a2@
    3bae:	6756           	beqs 3c06 <Exec_MakeFunctions+0x7c>
    3bb0:	43eb fffa      	lea %a3@(-6),%a1
    3bb4:	32bc 4ef9      	movew #20217,%a1@
    3bb8:	237c c0ed babe 	movel #-1058161986,%a1@(2)
    3bbe:	0002 
    3bc0:	4a52           	tstw %a2@
    3bc2:	6708           	beqs 3bcc <Exec_MakeFunctions+0x42>
    3bc4:	3052           	moveaw %a2@,%a0
    3bc6:	d1c0           	addal %d0,%a0
    3bc8:	2348 0002      	movel %a0,%a1@(2)
    3bcc:	548a           	addql #2,%a2
    3bce:	5d89           	subql #6,%a1
    3bd0:	5282           	addql #1,%d2
    3bd2:	0c52 ffff      	cmpiw #-1,%a2@
    3bd6:	66dc           	bnes 3bb4 <Exec_MakeFunctions+0x2a>
    3bd8:	602c           	bras 3c06 <Exec_MakeFunctions+0x7c>
    3bda:	2248           	moveal %a0,%a1
    3bdc:	72ff           	moveq #-1,%d1
    3bde:	b291           	cmpl %a1@,%d1
    3be0:	6724           	beqs 3c06 <Exec_MakeFunctions+0x7c>
    3be2:	41eb fffa      	lea %a3@(-6),%a0
    3be6:	30bc 4ef9      	movew #20217,%a0@
    3bea:	217c c0ed babe 	movel #-1058161986,%a0@(2)
    3bf0:	0002 
    3bf2:	2011           	movel %a1@,%d0
    3bf4:	6704           	beqs 3bfa <Exec_MakeFunctions+0x70>
    3bf6:	2140 0002      	movel %d0,%a0@(2)
    3bfa:	5889           	addql #4,%a1
    3bfc:	5d88           	subql #6,%a0
    3bfe:	5282           	addql #1,%d2
    3c00:	72ff           	moveq #-1,%d1
    3c02:	b291           	cmpl %a1@,%d1
    3c04:	66e0           	bnes 3be6 <Exec_MakeFunctions+0x5c>
    3c06:	2002           	movel %d2,%d0
    3c08:	e588           	lsll #2,%d0
    3c0a:	d082           	addl %d2,%d0
    3c0c:	d082           	addl %d2,%d0
    3c0e:	220b           	movel %a3,%d1
    3c10:	9280           	subl %d0,%d1
    3c12:	2001           	movel %d1,%d0
    3c14:	240b           	movel %a3,%d2
    3c16:	9480           	subl %d0,%d2
    3c18:	2f0c           	movel %a4,%sp@-
    3c1a:	4878 0808      	pea 808 <entry+0x3a4>
    3c1e:	2f02           	movel %d2,%sp@-
    3c20:	2f00           	movel %d0,%sp@-
    3c22:	206c fd80      	moveal %a4@(-640),%a0
    3c26:	4e90           	jsr %a0@
    3c28:	2002           	movel %d2,%d0
    3c2a:	4cee 1c04 fff0 	moveml %fp@(-16),%d2/%a2-%a4
    3c30:	4e5e           	unlk %fp
    3c32:	4e75           	rts

00003c34 <Exec_MakeLibrary>:
    3c34:	4e56 0000      	linkw %fp,#0
    3c38:	48e7 383c      	moveml %d2-%d4/%a2-%a5,%sp@-
    3c3c:	266e 0008      	moveal %fp@(8),%a3
    3c40:	262e 000c      	movel %fp@(12),%d3
    3c44:	282e 0010      	movel %fp@(16),%d4
    3c48:	2a6e 0014      	moveal %fp@(20),%a5
    3c4c:	286e 001c      	moveal %fp@(28),%a4
    3c50:	7400           	moveq #0,%d2
    3c52:	0c53 ffff      	cmpiw #-1,%a3@
    3c56:	6614           	bnes 3c6c <Exec_MakeLibrary+0x38>
    3c58:	41eb 0002      	lea %a3@(2),%a0
    3c5c:	0c58 ffff      	cmpiw #-1,%a0@+
    3c60:	6716           	beqs 3c78 <Exec_MakeLibrary+0x44>
    3c62:	5c82           	addql #6,%d2
    3c64:	0c58 ffff      	cmpiw #-1,%a0@+
    3c68:	66f8           	bnes 3c62 <Exec_MakeLibrary+0x2e>
    3c6a:	600c           	bras 3c78 <Exec_MakeLibrary+0x44>
    3c6c:	204b           	moveal %a3,%a0
    3c6e:	6002           	bras 3c72 <Exec_MakeLibrary+0x3e>
    3c70:	5c82           	addql #6,%d2
    3c72:	72ff           	moveq #-1,%d1
    3c74:	b298           	cmpl %a0@+,%d1
    3c76:	66f8           	bnes 3c70 <Exec_MakeLibrary+0x3c>
    3c78:	2002           	movel %d2,%d0
    3c7a:	5e80           	addql #7,%d0
    3c7c:	74f8           	moveq #-8,%d2
    3c7e:	c480           	andl %d0,%d2
    3c80:	2f0c           	movel %a4,%sp@-
    3c82:	2f3c 0001 0001 	movel #65537,%sp@-
    3c88:	4875 2800      	pea %a5@(00000000,%d2:l)
    3c8c:	206c ff3c      	moveal %a4@(-196),%a0
    3c90:	4e90           	jsr %a0@
    3c92:	2448           	moveal %a0,%a2
    3c94:	4fef 000c      	lea %sp@(12),%sp
    3c98:	b4fc 0000      	cmpaw #0,%a2
    3c9c:	6754           	beqs 3cf2 <Exec_MakeLibrary+0xbe>
    3c9e:	d5c2           	addal %d2,%a2
    3ca0:	0c53 ffff      	cmpiw #-1,%a3@
    3ca4:	660a           	bnes 3cb0 <Exec_MakeLibrary+0x7c>
    3ca6:	2f0c           	movel %a4,%sp@-
    3ca8:	2f0b           	movel %a3,%sp@-
    3caa:	486b 0002      	pea %a3@(2)
    3cae:	6006           	bras 3cb6 <Exec_MakeLibrary+0x82>
    3cb0:	2f0c           	movel %a4,%sp@-
    3cb2:	42a7           	clrl %sp@-
    3cb4:	2f0b           	movel %a3,%sp@-
    3cb6:	2f0a           	movel %a2,%sp@-
    3cb8:	206c ffa8      	moveal %a4@(-88),%a0
    3cbc:	4e90           	jsr %a0@
    3cbe:	4fef 0010      	lea %sp@(16),%sp
    3cc2:	3542 0010      	movew %d2,%a2@(16)
    3cc6:	354d 0012      	movew %a5,%a2@(18)
    3cca:	4a83           	tstl %d3
    3ccc:	6712           	beqs 3ce0 <Exec_MakeLibrary+0xac>
    3cce:	2f0c           	movel %a4,%sp@-
    3cd0:	42a7           	clrl %sp@-
    3cd2:	2f0a           	movel %a2,%sp@-
    3cd4:	2f03           	movel %d3,%sp@-
    3cd6:	206c ffb4      	moveal %a4@(-76),%a0
    3cda:	4e90           	jsr %a0@
    3cdc:	4fef 0010      	lea %sp@(16),%sp
    3ce0:	4a84           	tstl %d4
    3ce2:	670e           	beqs 3cf2 <Exec_MakeLibrary+0xbe>
    3ce4:	2f0c           	movel %a4,%sp@-
    3ce6:	2f2e 0018      	movel %fp@(24),%sp@-
    3cea:	2f0a           	movel %a2,%sp@-
    3cec:	2244           	moveal %d4,%a1
    3cee:	4e91           	jsr %a1@
    3cf0:	2448           	moveal %a0,%a2
    3cf2:	204a           	moveal %a2,%a0
    3cf4:	2008           	movel %a0,%d0
    3cf6:	4cee 3c1c ffe4 	moveml %fp@(-28),%d2-%d4/%a2-%a5
    3cfc:	4e5e           	unlk %fp
    3cfe:	4e75           	rts

00003d00 <Exec_ObtainQuickVector>:
    3d00:	4e56 0000      	linkw %fp,#0
    3d04:	4879 0000 0000 	pea 0 <main>
    3d0a:	4879 0000 0000 	pea 0 <main>
    3d10:	61ff 0000 0000 	bsrl 3d12 <Exec_ObtainQuickVector+0x12>
    3d16:	7000           	moveq #0,%d0
    3d18:	4e5e           	unlk %fp
    3d1a:	4e75           	rts

00003d1c <Exec_ObtainSemaphore>:
    3d1c:	4e56 fff4      	linkw %fp,#-12
    3d20:	48e7 0038      	moveml %a2-%a4,%sp@-
    3d24:	266e 0008      	moveal %fp@(8),%a3
    3d28:	286e 000c      	moveal %fp@(12),%a4
    3d2c:	246c 0114      	moveal %a4@(276),%a2
    3d30:	0c2b 000f 000c 	cmpib #15,%a3@(12)
    3d36:	6726           	beqs 3d5e <Exec_ObtainSemaphore+0x42>
    3d38:	2f2a 0008      	movel %a2@(8),%sp@-
    3d3c:	2f0a           	movel %a2,%sp@-
    3d3e:	2f0b           	movel %a3,%sp@-
    3d40:	4879 0000 0000 	pea 0 <main>
    3d46:	61ff 0000 0000 	bsrl 3d48 <Exec_ObtainSemaphore+0x2c>
    3d4c:	2f0c           	movel %a4,%sp@-
    3d4e:	2f3c 0100 0008 	movel #16777224,%sp@-
    3d54:	206c ff96      	moveal %a4@(-106),%a0
    3d58:	4e90           	jsr %a0@
    3d5a:	4fef 0018      	lea %sp@(24),%sp
    3d5e:	2f0c           	movel %a4,%sp@-
    3d60:	206c ff7e      	moveal %a4@(-130),%a0
    3d64:	4e90           	jsr %a0@
    3d66:	302b 002c      	movew %a3@(44),%d0
    3d6a:	3200           	movew %d0,%d1
    3d6c:	5241           	addqw #1,%d1
    3d6e:	3741 002c      	movew %d1,%a3@(44)
    3d72:	588f           	addql #4,%sp
    3d74:	0c40 ffff      	cmpiw #-1,%d0
    3d78:	660a           	bnes 3d84 <Exec_ObtainSemaphore+0x68>
    3d7a:	274a 0028      	movel %a2,%a3@(40)
    3d7e:	526b 000e      	addqw #1,%a3@(14)
    3d82:	6034           	bras 3db8 <Exec_ObtainSemaphore+0x9c>
    3d84:	b5eb 0028      	cmpal %a3@(40),%a2
    3d88:	6606           	bnes 3d90 <Exec_ObtainSemaphore+0x74>
    3d8a:	526b 000e      	addqw #1,%a3@(14)
    3d8e:	6028           	bras 3db8 <Exec_ObtainSemaphore+0x9c>
    3d90:	2d4a fffc      	movel %a2,%fp@(-4)
    3d94:	72ef           	moveq #-17,%d1
    3d96:	c3aa 001a      	andl %d1,%a2@(26)
    3d9a:	486e fff4      	pea %fp@(-12)
    3d9e:	486b 0010      	pea %a3@(16)
    3da2:	206c ff0c      	moveal %a4@(-244),%a0
    3da6:	4e90           	jsr %a0@
    3da8:	2f0c           	movel %a4,%sp@-
    3daa:	4878 0010      	pea 10 <main+0x10>
    3dae:	206c fec4      	moveal %a4@(-316),%a0
    3db2:	4e90           	jsr %a0@
    3db4:	4fef 0010      	lea %sp@(16),%sp
    3db8:	2f0c           	movel %a4,%sp@-
    3dba:	206c ff78      	moveal %a4@(-136),%a0
    3dbe:	4e90           	jsr %a0@
    3dc0:	4cee 1c00 ffe8 	moveml %fp@(-24),%a2-%a4
    3dc6:	4e5e           	unlk %fp
    3dc8:	4e75           	rts

00003dca <Exec_ObtainSemaphoreList>:
    3dca:	4e56 0000      	linkw %fp,#0
    3dce:	48e7 3038      	moveml %d2-%d3/%a2-%a4,%sp@-
    3dd2:	286e 0008      	moveal %fp@(8),%a4
    3dd6:	266e 000c      	moveal %fp@(12),%a3
    3dda:	2f0b           	movel %a3,%sp@-
    3ddc:	42a7           	clrl %sp@-
    3dde:	206b fedc      	moveal %a3@(-292),%a0
    3de2:	4e90           	jsr %a0@
    3de4:	2608           	movel %a0,%d3
    3de6:	4242           	clrw %d2
    3de8:	2f0b           	movel %a3,%sp@-
    3dea:	206b ff7e      	moveal %a3@(-130),%a0
    3dee:	4e90           	jsr %a0@
    3df0:	2454           	moveal %a4@,%a2
    3df2:	4fef 000c      	lea %sp@(12),%sp
    3df6:	4a92           	tstl %a2@
    3df8:	6738           	beqs 3e32 <Exec_ObtainSemaphoreList+0x68>
    3dfa:	302a 002c      	movew %a2@(44),%d0
    3dfe:	3240           	moveaw %d0,%a1
    3e00:	5249           	addqw #1,%a1
    3e02:	3549 002c      	movew %a1,%a2@(44)
    3e06:	0c40 ffff      	cmpiw #-1,%d0
    3e0a:	6718           	beqs 3e24 <Exec_ObtainSemaphoreList+0x5a>
    3e0c:	2543 0024      	movel %d3,%a2@(36)
    3e10:	486a 001c      	pea %a2@(28)
    3e14:	486a 0010      	pea %a2@(16)
    3e18:	206b ff0c      	moveal %a3@(-244),%a0
    3e1c:	4e90           	jsr %a0@
    3e1e:	5242           	addqw #1,%d2
    3e20:	508f           	addql #8,%sp
    3e22:	6008           	bras 3e2c <Exec_ObtainSemaphoreList+0x62>
    3e24:	526a 000e      	addqw #1,%a2@(14)
    3e28:	2543 0028      	movel %d3,%a2@(40)
    3e2c:	2452           	moveal %a2@,%a2
    3e2e:	4a92           	tstl %a2@
    3e30:	66c8           	bnes 3dfa <Exec_ObtainSemaphoreList+0x30>
    3e32:	4a42           	tstw %d2
    3e34:	6f22           	bles 3e58 <Exec_ObtainSemaphoreList+0x8e>
    3e36:	2454           	moveal %a4@,%a2
    3e38:	4a92           	tstl %a2@
    3e3a:	671c           	beqs 3e58 <Exec_ObtainSemaphoreList+0x8e>
    3e3c:	b6aa 0028      	cmpl %a2@(40),%d3
    3e40:	6710           	beqs 3e52 <Exec_ObtainSemaphoreList+0x88>
    3e42:	2f0b           	movel %a3,%sp@-
    3e44:	4878 0010      	pea 10 <main+0x10>
    3e48:	206b fec4      	moveal %a3@(-316),%a0
    3e4c:	4e90           	jsr %a0@
    3e4e:	508f           	addql #8,%sp
    3e50:	60e6           	bras 3e38 <Exec_ObtainSemaphoreList+0x6e>
    3e52:	2452           	moveal %a2@,%a2
    3e54:	5342           	subqw #1,%d2
    3e56:	60e0           	bras 3e38 <Exec_ObtainSemaphoreList+0x6e>
    3e58:	4a42           	tstw %d2
    3e5a:	6722           	beqs 3e7e <Exec_ObtainSemaphoreList+0xb4>
    3e5c:	3242           	moveaw %d2,%a1
    3e5e:	2f09           	movel %a1,%sp@-
    3e60:	4879 0000 0000 	pea 0 <main>
    3e66:	61ff 0000 0000 	bsrl 3e68 <Exec_ObtainSemaphoreList+0x9e>
    3e6c:	2f0b           	movel %a3,%sp@-
    3e6e:	2f3c 0100 0010 	movel #16777232,%sp@-
    3e74:	206b ff96      	moveal %a3@(-106),%a0
    3e78:	4e90           	jsr %a0@
    3e7a:	4fef 0010      	lea %sp@(16),%sp
    3e7e:	2f0b           	movel %a3,%sp@-
    3e80:	206b ff78      	moveal %a3@(-136),%a0
    3e84:	4e90           	jsr %a0@
    3e86:	4cee 1c0c ffec 	moveml %fp@(-20),%d2-%d3/%a2-%a4
    3e8c:	4e5e           	unlk %fp
    3e8e:	4e75           	rts

00003e90 <Exec_ObtainSemaphoreShared>:
    3e90:	4e56 fff4      	linkw %fp,#-12
    3e94:	48e7 0038      	moveml %a2-%a4,%sp@-
    3e98:	266e 0008      	moveal %fp@(8),%a3
    3e9c:	286e 000c      	moveal %fp@(12),%a4
    3ea0:	246c 0114      	moveal %a4@(276),%a2
    3ea4:	0c2b 000f 000c 	cmpib #15,%a3@(12)
    3eaa:	6726           	beqs 3ed2 <Exec_ObtainSemaphoreShared+0x42>
    3eac:	2f2a 0008      	movel %a2@(8),%sp@-
    3eb0:	2f0a           	movel %a2,%sp@-
    3eb2:	2f0b           	movel %a3,%sp@-
    3eb4:	4879 0000 0000 	pea 0 <main>
    3eba:	61ff 0000 0000 	bsrl 3ebc <Exec_ObtainSemaphoreShared+0x2c>
    3ec0:	2f0c           	movel %a4,%sp@-
    3ec2:	2f3c 0100 0008 	movel #16777224,%sp@-
    3ec8:	206c ff96      	moveal %a4@(-106),%a0
    3ecc:	4e90           	jsr %a0@
    3ece:	4fef 0018      	lea %sp@(24),%sp
    3ed2:	2f0c           	movel %a4,%sp@-
    3ed4:	206c ff7e      	moveal %a4@(-130),%a0
    3ed8:	4e90           	jsr %a0@
    3eda:	302b 002c      	movew %a3@(44),%d0
    3ede:	3200           	movew %d0,%d1
    3ee0:	5241           	addqw #1,%d1
    3ee2:	3741 002c      	movew %d1,%a3@(44)
    3ee6:	588f           	addql #4,%sp
    3ee8:	0c40 ffff      	cmpiw #-1,%d0
    3eec:	660a           	bnes 3ef8 <Exec_ObtainSemaphoreShared+0x68>
    3eee:	42ab 0028      	clrl %a3@(40)
    3ef2:	526b 000e      	addqw #1,%a3@(14)
    3ef6:	6048           	bras 3f40 <Exec_ObtainSemaphoreShared+0xb0>
    3ef8:	202b 0028      	movel %a3@(40),%d0
    3efc:	b5c0           	cmpal %d0,%a2
    3efe:	67f2           	beqs 3ef2 <Exec_ObtainSemaphoreShared+0x62>
    3f00:	4a80           	tstl %d0
    3f02:	67ee           	beqs 3ef2 <Exec_ObtainSemaphoreShared+0x62>
    3f04:	41ea 0001      	lea %a2@(1),%a0
    3f08:	2d48 fffc      	movel %a0,%fp@(-4)
    3f0c:	2f08           	movel %a0,%sp@-
    3f0e:	2f0a           	movel %a2,%sp@-
    3f10:	4879 0000 0000 	pea 0 <main>
    3f16:	61ff 0000 0000 	bsrl 3f18 <Exec_ObtainSemaphoreShared+0x88>
    3f1c:	72ef           	moveq #-17,%d1
    3f1e:	c3aa 001a      	andl %d1,%a2@(26)
    3f22:	486e fff4      	pea %fp@(-12)
    3f26:	486b 0010      	pea %a3@(16)
    3f2a:	206c ff0c      	moveal %a4@(-244),%a0
    3f2e:	4e90           	jsr %a0@
    3f30:	2f0c           	movel %a4,%sp@-
    3f32:	4878 0010      	pea 10 <main+0x10>
    3f36:	206c fec4      	moveal %a4@(-316),%a0
    3f3a:	4e90           	jsr %a0@
    3f3c:	4fef 001c      	lea %sp@(28),%sp
    3f40:	2f0c           	movel %a4,%sp@-
    3f42:	206c ff78      	moveal %a4@(-136),%a0
    3f46:	4e90           	jsr %a0@
    3f48:	4cee 1c00 ffe8 	moveml %fp@(-24),%a2-%a4
    3f4e:	4e5e           	unlk %fp
    3f50:	4e75           	rts

00003f52 <Exec_OldOpenLibrary>:
    3f52:	4e56 0000      	linkw %fp,#0
    3f56:	206e 000c      	moveal %fp@(12),%a0
    3f5a:	2f08           	movel %a0,%sp@-
    3f5c:	42a7           	clrl %sp@-
    3f5e:	2f2e 0008      	movel %fp@(8),%sp@-
    3f62:	2068 fdda      	moveal %a0@(-550),%a0
    3f66:	4e90           	jsr %a0@
    3f68:	2008           	movel %a0,%d0
    3f6a:	4e5e           	unlk %fp
    3f6c:	4e75           	rts

00003f6e <Exec_OpenDevice>:
    3f6e:	4e56 0000      	linkw %fp,#0
    3f72:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    3f76:	242e 0008      	movel %fp@(8),%d2
    3f7a:	246e 0010      	moveal %fp@(16),%a2
    3f7e:	266e 0018      	moveal %fp@(24),%a3
    3f82:	50c3           	st %d3
    3f84:	2f0b           	movel %a3,%sp@-
    3f86:	206b ff7e      	moveal %a3@(-130),%a0
    3f8a:	4e90           	jsr %a0@
    3f8c:	2f02           	movel %d2,%sp@-
    3f8e:	486b 015e      	pea %a3@(350)
    3f92:	206b feee      	moveal %a3@(-274),%a0
    3f96:	4e90           	jsr %a0@
    3f98:	4fef 000c      	lea %sp@(12),%sp
    3f9c:	b0fc 0000      	cmpaw #0,%a0
    3fa0:	672c           	beqs 3fce <Exec_OpenDevice+0x60>
    3fa2:	422a 001f      	clrb %a2@(31)
    3fa6:	2548 0014      	movel %a0,%a2@(20)
    3faa:	42aa 0018      	clrl %a2@(24)
    3fae:	2f08           	movel %a0,%sp@-
    3fb0:	2f2e 0014      	movel %fp@(20),%sp@-
    3fb4:	2f2e 000c      	movel %fp@(12),%sp@-
    3fb8:	2f0a           	movel %a2,%sp@-
    3fba:	2068 fffc      	moveal %a0@(-4),%a0
    3fbe:	4e90           	jsr %a0@
    3fc0:	162a 001f      	moveb %a2@(31),%d3
    3fc4:	4fef 0010      	lea %sp@(16),%sp
    3fc8:	6704           	beqs 3fce <Exec_OpenDevice+0x60>
    3fca:	42aa 0014      	clrl %a2@(20)
    3fce:	2f0b           	movel %a3,%sp@-
    3fd0:	206b ff78      	moveal %a3@(-136),%a0
    3fd4:	4e90           	jsr %a0@
    3fd6:	1003           	moveb %d3,%d0
    3fd8:	4880           	extw %d0
    3fda:	48c0           	extl %d0
    3fdc:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    3fe2:	4e5e           	unlk %fp
    3fe4:	4e75           	rts

00003fe6 <Exec_OpenLibrary>:
    3fe6:	4e56 0000      	linkw %fp,#0
    3fea:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    3fee:	242e 0008      	movel %fp@(8),%d2
    3ff2:	262e 000c      	movel %fp@(12),%d3
    3ff6:	266e 0010      	moveal %fp@(16),%a3
    3ffa:	2f0b           	movel %a3,%sp@-
    3ffc:	206b ff7e      	moveal %a3@(-130),%a0
    4000:	4e90           	jsr %a0@
    4002:	2f02           	movel %d2,%sp@-
    4004:	486b 017a      	pea %a3@(378)
    4008:	206b feee      	moveal %a3@(-274),%a0
    400c:	4e90           	jsr %a0@
    400e:	2448           	moveal %a0,%a2
    4010:	4fef 000c      	lea %sp@(12),%sp
    4014:	b4fc 0000      	cmpaw #0,%a2
    4018:	671c           	beqs 4036 <Exec_OpenLibrary+0x50>
    401a:	4280           	clrl %d0
    401c:	302a 0014      	movew %a2@(20),%d0
    4020:	b680           	cmpl %d0,%d3
    4022:	6210           	bhis 4034 <Exec_OpenLibrary+0x4e>
    4024:	2f0a           	movel %a2,%sp@-
    4026:	2f03           	movel %d3,%sp@-
    4028:	206a fffc      	moveal %a2@(-4),%a0
    402c:	4e90           	jsr %a0@
    402e:	2448           	moveal %a0,%a2
    4030:	508f           	addql #8,%sp
    4032:	6002           	bras 4036 <Exec_OpenLibrary+0x50>
    4034:	95ca           	subal %a2,%a2
    4036:	2f0b           	movel %a3,%sp@-
    4038:	206b ff78      	moveal %a3@(-136),%a0
    403c:	4e90           	jsr %a0@
    403e:	204a           	moveal %a2,%a0
    4040:	2008           	movel %a0,%d0
    4042:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    4048:	4e5e           	unlk %fp
    404a:	4e75           	rts

0000404c <Exec_OpenResource>:
    404c:	4e56 0000      	linkw %fp,#0
    4050:	2f0a           	movel %a2,%sp@-
    4052:	2f02           	movel %d2,%sp@-
    4054:	242e 0008      	movel %fp@(8),%d2
    4058:	246e 000c      	moveal %fp@(12),%a2
    405c:	2f0a           	movel %a2,%sp@-
    405e:	206a ff7e      	moveal %a2@(-130),%a0
    4062:	4e90           	jsr %a0@
    4064:	2f02           	movel %d2,%sp@-
    4066:	486a 0150      	pea %a2@(336)
    406a:	206a feee      	moveal %a2@(-274),%a0
    406e:	4e90           	jsr %a0@
    4070:	2408           	movel %a0,%d2
    4072:	2f0a           	movel %a2,%sp@-
    4074:	206a ff78      	moveal %a2@(-136),%a0
    4078:	4e90           	jsr %a0@
    407a:	2042           	moveal %d2,%a0
    407c:	2008           	movel %a0,%d0
    407e:	242e fff8      	movel %fp@(-8),%d2
    4082:	246e fffc      	moveal %fp@(-4),%a2
    4086:	4e5e           	unlk %fp
    4088:	4e75           	rts

0000408a <Exec_Permit>:
    408a:	4e56 0000      	linkw %fp,#0
    408e:	206e 0008      	moveal %fp@(8),%a0
    4092:	5328 0127      	subqb #1,%a0@(295)
    4096:	4e5e           	unlk %fp
    4098:	4e75           	rts

0000409a <Exec_Procure>:
    409a:	4e56 0000      	linkw %fp,#0
    409e:	48e7 0038      	moveml %a2-%a4,%sp@-
    40a2:	246e 0008      	moveal %fp@(8),%a2
    40a6:	266e 000c      	moveal %fp@(12),%a3
    40aa:	286e 0010      	moveal %fp@(16),%a4
    40ae:	377c 0018 0012 	movew #24,%a3@(18)
    40b4:	7201           	moveq #1,%d1
    40b6:	b2ab 0008      	cmpl %a3@(8),%d1
    40ba:	6606           	bnes 40c2 <Exec_Procure+0x28>
    40bc:	42ab 0014      	clrl %a3@(20)
    40c0:	6010           	bras 40d2 <Exec_Procure+0x38>
    40c2:	2f0c           	movel %a4,%sp@-
    40c4:	42a7           	clrl %sp@-
    40c6:	206c fedc      	moveal %a4@(-292),%a0
    40ca:	4e90           	jsr %a0@
    40cc:	2748 0014      	movel %a0,%a3@(20)
    40d0:	508f           	addql #8,%sp
    40d2:	2f0c           	movel %a4,%sp@-
    40d4:	206c ff7e      	moveal %a4@(-130),%a0
    40d8:	4e90           	jsr %a0@
    40da:	302a 002c      	movew %a2@(44),%d0
    40de:	3200           	movew %d0,%d1
    40e0:	5241           	addqw #1,%d1
    40e2:	3541 002c      	movew %d1,%a2@(44)
    40e6:	588f           	addql #4,%sp
    40e8:	0c40 ffff      	cmpiw #-1,%d0
    40ec:	6618           	bnes 4106 <Exec_Procure+0x6c>
    40ee:	256b 0014 0028 	movel %a3@(20),%a2@(40)
    40f4:	526a 000e      	addqw #1,%a2@(14)
    40f8:	274a 0014      	movel %a2,%a3@(20)
    40fc:	2f0c           	movel %a4,%sp@-
    40fe:	2f0a           	movel %a2,%sp@-
    4100:	206c fe88      	moveal %a4@(-376),%a0
    4104:	6036           	bras 413c <Exec_Procure+0xa2>
    4106:	202b 0014      	movel %a3@(20),%d0
    410a:	b0aa 0028      	cmpl %a2@(40),%d0
    410e:	6612           	bnes 4122 <Exec_Procure+0x88>
    4110:	526a 000e      	addqw #1,%a2@(14)
    4114:	274a 0014      	movel %a2,%a3@(20)
    4118:	2f0c           	movel %a4,%sp@-
    411a:	2f0a           	movel %a2,%sp@-
    411c:	206c fe88      	moveal %a4@(-376),%a0
    4120:	601a           	bras 413c <Exec_Procure+0xa2>
    4122:	4a80           	tstl %d0
    4124:	6706           	beqs 412c <Exec_Procure+0x92>
    4126:	42ab 0008      	clrl %a3@(8)
    412a:	6006           	bras 4132 <Exec_Procure+0x98>
    412c:	7201           	moveq #1,%d1
    412e:	2741 0008      	movel %d1,%a3@(8)
    4132:	2f0b           	movel %a3,%sp@-
    4134:	486a 0010      	pea %a2@(16)
    4138:	206c ff0c      	moveal %a4@(-244),%a0
    413c:	4e90           	jsr %a0@
    413e:	584f           	addqw #4,%sp
    4140:	2e8c           	movel %a4,%sp@
    4142:	206c ff78      	moveal %a4@(-136),%a0
    4146:	4e90           	jsr %a0@
    4148:	7000           	moveq #0,%d0
    414a:	4cee 1c00 fff4 	moveml %fp@(-12),%a2-%a4
    4150:	4e5e           	unlk %fp
    4152:	4e75           	rts

00004154 <Exec_PutMsg>:
    4154:	4e56 0000      	linkw %fp,#0
    4158:	48e7 2038      	moveml %d2/%a2-%a4,%sp@-
    415c:	286e 0008      	moveal %fp@(8),%a4
    4160:	266e 000c      	moveal %fp@(12),%a3
    4164:	246e 0010      	moveal %fp@(16),%a2
    4168:	b7fc 0000 0400 	cmpal #1024,%a3
    416e:	6f10           	bles 4180 <Exec_PutMsg+0x2c>
    4170:	2f0a           	movel %a2,%sp@-
    4172:	2f0b           	movel %a3,%sp@-
    4174:	206a fdec      	moveal %a2@(-532),%a0
    4178:	4e90           	jsr %a0@
    417a:	508f           	addql #8,%sp
    417c:	4a80           	tstl %d0
    417e:	6622           	bnes 41a2 <Exec_PutMsg+0x4e>
    4180:	2f0b           	movel %a3,%sp@-
    4182:	4879 0000 0000 	pea 0 <main>
    4188:	4878 003a      	pea 3a <pause+0x1a>
    418c:	4879 0000 0000 	pea 0 <main>
    4192:	4879 0000 0000 	pea 0 <main>
    4198:	61ff 0000 0000 	bsrl 419a <Exec_PutMsg+0x46>
    419e:	4fef 0014      	lea %sp@(20),%sp
    41a2:	b9fc 0000 0400 	cmpal #1024,%a4
    41a8:	6f10           	bles 41ba <Exec_PutMsg+0x66>
    41aa:	2f0a           	movel %a2,%sp@-
    41ac:	2f0c           	movel %a4,%sp@-
    41ae:	206a fdec      	moveal %a2@(-532),%a0
    41b2:	4e90           	jsr %a0@
    41b4:	508f           	addql #8,%sp
    41b6:	4a80           	tstl %d0
    41b8:	6622           	bnes 41dc <Exec_PutMsg+0x88>
    41ba:	2f0c           	movel %a4,%sp@-
    41bc:	4879 0000 0000 	pea 0 <main>
    41c2:	4878 003b      	pea 3b <pause+0x1b>
    41c6:	4879 0000 0000 	pea 0 <main>
    41cc:	4879 0000 0000 	pea 0 <main>
    41d2:	61ff 0000 0000 	bsrl 41d4 <Exec_PutMsg+0x80>
    41d8:	4fef 0014      	lea %sp@(20),%sp
    41dc:	2f0a           	movel %a2,%sp@-
    41de:	206a ff8a      	moveal %a2@(-118),%a0
    41e2:	4e90           	jsr %a0@
    41e4:	177c 0005 000c 	moveb #5,%a3@(12)
    41ea:	2f0b           	movel %a3,%sp@-
    41ec:	486c 0014      	pea %a4@(20)
    41f0:	206a ff0c      	moveal %a2@(-244),%a0
    41f4:	4e90           	jsr %a0@
    41f6:	4fef 000c      	lea %sp@(12),%sp
    41fa:	202c 0010      	movel %a4@(16),%d0
    41fe:	677c           	beqs 427c <Exec_PutMsg+0x128>
    4200:	0c80 0000 0400 	cmpil #1024,%d0
    4206:	6f10           	bles 4218 <Exec_PutMsg+0xc4>
    4208:	2f0a           	movel %a2,%sp@-
    420a:	2f00           	movel %d0,%sp@-
    420c:	206a fdec      	moveal %a2@(-532),%a0
    4210:	4e90           	jsr %a0@
    4212:	508f           	addql #8,%sp
    4214:	4a80           	tstl %d0
    4216:	6624           	bnes 423c <Exec_PutMsg+0xe8>
    4218:	2f2c 0010      	movel %a4@(16),%sp@-
    421c:	4879 0000 0000 	pea 0 <main>
    4222:	4878 004b      	pea 4b <pause+0x2b>
    4226:	4879 0000 0000 	pea 0 <main>
    422c:	4879 0000 0000 	pea 0 <main>
    4232:	61ff 0000 0000 	bsrl 4234 <Exec_PutMsg+0xe0>
    4238:	4fef 0014      	lea %sp@(20),%sp
    423c:	102c 000e      	moveb %a4@(14),%d0
    4240:	7403           	moveq #3,%d2
    4242:	c082           	andl %d2,%d0
    4244:	7401           	moveq #1,%d2
    4246:	b480           	cmpl %d0,%d2
    4248:	6724           	beqs 426e <Exec_PutMsg+0x11a>
    424a:	6d30           	blts 427c <Exec_PutMsg+0x128>
    424c:	4a80           	tstl %d0
    424e:	662c           	bnes 427c <Exec_PutMsg+0x128>
    4250:	2f0a           	movel %a2,%sp@-
    4252:	4281           	clrl %d1
    4254:	122c 000f      	moveb %a4@(15),%d1
    4258:	2002           	movel %d2,%d0
    425a:	e3a8           	lsll %d1,%d0
    425c:	2f00           	movel %d0,%sp@-
    425e:	2f2c 0010      	movel %a4@(16),%sp@-
    4262:	206a febe      	moveal %a2@(-322),%a0
    4266:	4e90           	jsr %a0@
    4268:	4fef 000c      	lea %sp@(12),%sp
    426c:	600e           	bras 427c <Exec_PutMsg+0x128>
    426e:	2f0a           	movel %a2,%sp@-
    4270:	2f2c 0010      	movel %a4@(16),%sp@-
    4274:	206a ff4e      	moveal %a2@(-178),%a0
    4278:	4e90           	jsr %a0@
    427a:	508f           	addql #8,%sp
    427c:	2f0a           	movel %a2,%sp@-
    427e:	206a ff84      	moveal %a2@(-124),%a0
    4282:	4e90           	jsr %a0@
    4284:	4cee 1c04 fff0 	moveml %fp@(-16),%d2/%a2-%a4
    428a:	4e5e           	unlk %fp
    428c:	4e75           	rts

0000428e <Exec_RawDoFmt>:
    428e:	4e56 ffe8      	linkw %fp,#-24
    4292:	48e7 3f3c      	moveml %d2-%d7/%a2-%a5,%sp@-
    4296:	266e 0008      	moveal %fp@(8),%a3
    429a:	286e 000c      	moveal %fp@(12),%a4
    429e:	2a6e 0014      	moveal %fp@(20),%a5
    42a2:	4a13           	tstb %a3@
    42a4:	6700 0276      	beqw 451c <Exec_RawDoFmt+0x28e>
    42a8:	7c00           	moveq #0,%d6
    42aa:	1013           	moveb %a3@,%d0
    42ac:	0c00 0025      	cmpib #37,%d0
    42b0:	6600 0252      	bnew 4504 <Exec_RawDoFmt+0x276>
    42b4:	42ae fff2      	clrl %fp@(-14)
    42b8:	7e20           	moveq #32,%d7
    42ba:	2d47 ffea      	movel %d7,%fp@(-22)
    42be:	42ae ffee      	clrl %fp@(-18)
    42c2:	7a00           	moveq #0,%d5
    42c4:	74ff           	moveq #-1,%d2
    42c6:	2605           	movel %d5,%d3
    42c8:	2805           	movel %d5,%d4
    42ca:	528b           	addql #1,%a3
    42cc:	0c13 002d      	cmpib #45,%a3@
    42d0:	660a           	bnes 42dc <Exec_RawDoFmt+0x4e>
    42d2:	327c 002d      	moveaw #45,%a1
    42d6:	2d49 fff2      	movel %a1,%fp@(-14)
    42da:	528b           	addql #1,%a3
    42dc:	0c13 0030      	cmpib #48,%a3@
    42e0:	6608           	bnes 42ea <Exec_RawDoFmt+0x5c>
    42e2:	7e30           	moveq #48,%d7
    42e4:	2d47 ffea      	movel %d7,%fp@(-22)
    42e8:	528b           	addql #1,%a3
    42ea:	1013           	moveb %a3@,%d0
    42ec:	0600 ffd0      	addib #-48,%d0
    42f0:	0c00 0009      	cmpib #9,%d0
    42f4:	6220           	bhis 4316 <Exec_RawDoFmt+0x88>
    42f6:	7200           	moveq #0,%d1
    42f8:	2005           	movel %d5,%d0
    42fa:	e788           	lsll #3,%d0
    42fc:	2040           	moveal %d0,%a0
    42fe:	d1c5           	addal %d5,%a0
    4300:	41f0 58d0      	lea %a0@(ffffffd0,%d5:l),%a0
    4304:	121b           	moveb %a3@+,%d1
    4306:	2a08           	movel %a0,%d5
    4308:	da81           	addl %d1,%d5
    430a:	1013           	moveb %a3@,%d0
    430c:	0600 ffd0      	addib #-48,%d0
    4310:	0c00 0009      	cmpib #9,%d0
    4314:	63e2           	blss 42f8 <Exec_RawDoFmt+0x6a>
    4316:	0c13 002e      	cmpib #46,%a3@
    431a:	6630           	bnes 434c <Exec_RawDoFmt+0xbe>
    431c:	528b           	addql #1,%a3
    431e:	1013           	moveb %a3@,%d0
    4320:	0600 ffd0      	addib #-48,%d0
    4324:	0c00 0009      	cmpib #9,%d0
    4328:	6222           	bhis 434c <Exec_RawDoFmt+0xbe>
    432a:	7400           	moveq #0,%d2
    432c:	2202           	movel %d2,%d1
    432e:	2002           	movel %d2,%d0
    4330:	e788           	lsll #3,%d0
    4332:	2040           	moveal %d0,%a0
    4334:	d1c2           	addal %d2,%a0
    4336:	41f0 28d0      	lea %a0@(ffffffd0,%d2:l),%a0
    433a:	121b           	moveb %a3@+,%d1
    433c:	2408           	movel %a0,%d2
    433e:	d481           	addl %d1,%d2
    4340:	1013           	moveb %a3@,%d0
    4342:	0600 ffd0      	addib #-48,%d0
    4346:	0c00 0009      	cmpib #9,%d0
    434a:	63e2           	blss 432e <Exec_RawDoFmt+0xa0>
    434c:	0c13 006c      	cmpib #108,%a3@
    4350:	6604           	bnes 4356 <Exec_RawDoFmt+0xc8>
    4352:	766c           	moveq #108,%d3
    4354:	528b           	addql #1,%a3
    4356:	1c13           	moveb %a3@,%d6
    4358:	7e64           	moveq #100,%d7
    435a:	be86           	cmpl %d6,%d7
    435c:	6748           	beqs 43a6 <Exec_RawDoFmt+0x118>
    435e:	6d14           	blts 4374 <Exec_RawDoFmt+0xe6>
    4360:	7e62           	moveq #98,%d7
    4362:	be86           	cmpl %d6,%d7
    4364:	672c           	beqs 4392 <Exec_RawDoFmt+0x104>
    4366:	6d00 00ea      	bltw 4452 <Exec_RawDoFmt+0x1c4>
    436a:	4a86           	tstl %d6
    436c:	6700 0108      	beqw 4476 <Exec_RawDoFmt+0x1e8>
    4370:	6000 010a      	braw 447c <Exec_RawDoFmt+0x1ee>
    4374:	7e75           	moveq #117,%d7
    4376:	be86           	cmpl %d6,%d7
    4378:	672c           	beqs 43a6 <Exec_RawDoFmt+0x118>
    437a:	6d0c           	blts 4388 <Exec_RawDoFmt+0xfa>
    437c:	7e73           	moveq #115,%d7
    437e:	be86           	cmpl %d6,%d7
    4380:	6700 00ae      	beqw 4430 <Exec_RawDoFmt+0x1a2>
    4384:	6000 00f6      	braw 447c <Exec_RawDoFmt+0x1ee>
    4388:	7e78           	moveq #120,%d7
    438a:	be86           	cmpl %d6,%d7
    438c:	6772           	beqs 4400 <Exec_RawDoFmt+0x172>
    438e:	6000 00ec      	braw 447c <Exec_RawDoFmt+0x1ee>
    4392:	200c           	movel %a4,%d0
    4394:	5680           	addql #3,%d0
    4396:	7efc           	moveq #-4,%d7
    4398:	ce80           	andl %d0,%d7
    439a:	2847           	moveal %d7,%a4
    439c:	245c           	moveal %a4@+,%a2
    439e:	7800           	moveq #0,%d4
    43a0:	181a           	moveb %a2@+,%d4
    43a2:	6000 00a6      	braw 444a <Exec_RawDoFmt+0x1bc>
    43a6:	4a83           	tstl %d3
    43a8:	670e           	beqs 43b8 <Exec_RawDoFmt+0x12a>
    43aa:	200c           	movel %a4,%d0
    43ac:	5680           	addql #3,%d0
    43ae:	7efc           	moveq #-4,%d7
    43b0:	ce80           	andl %d0,%d7
    43b2:	2847           	moveal %d7,%a4
    43b4:	241c           	movel %a4@+,%d2
    43b6:	6004           	bras 43bc <Exec_RawDoFmt+0x12e>
    43b8:	341c           	movew %a4@+,%d2
    43ba:	48c2           	extl %d2
    43bc:	0c13 0064      	cmpib #100,%a3@
    43c0:	660e           	bnes 43d0 <Exec_RawDoFmt+0x142>
    43c2:	4a82           	tstl %d2
    43c4:	6c0a           	bges 43d0 <Exec_RawDoFmt+0x142>
    43c6:	327c 0001      	moveaw #1,%a1
    43ca:	2d49 ffee      	movel %a1,%fp@(-18)
    43ce:	4482           	negl %d2
    43d0:	244e           	moveal %fp,%a2
    43d2:	4878 000a      	pea a <main+0xa>
    43d6:	2f02           	movel %d2,%sp@-
    43d8:	61ff 0000 0000 	bsrl 43da <Exec_RawDoFmt+0x14c>
    43de:	508f           	addql #8,%sp
    43e0:	0600 0030      	addib #48,%d0
    43e4:	1500           	moveb %d0,%a2@-
    43e6:	4878 000a      	pea a <main+0xa>
    43ea:	2f02           	movel %d2,%sp@-
    43ec:	61ff 0000 0000 	bsrl 43ee <Exec_RawDoFmt+0x160>
    43f2:	508f           	addql #8,%sp
    43f4:	2400           	movel %d0,%d2
    43f6:	5284           	addql #1,%d4
    43f8:	4a82           	tstl %d2
    43fa:	66d6           	bnes 43d2 <Exec_RawDoFmt+0x144>
    43fc:	6000 0082      	braw 4480 <Exec_RawDoFmt+0x1f2>
    4400:	4a83           	tstl %d3
    4402:	670e           	beqs 4412 <Exec_RawDoFmt+0x184>
    4404:	200c           	movel %a4,%d0
    4406:	5680           	addql #3,%d0
    4408:	7efc           	moveq #-4,%d7
    440a:	ce80           	andl %d0,%d7
    440c:	2847           	moveal %d7,%a4
    440e:	221c           	movel %a4@+,%d1
    4410:	6004           	bras 4416 <Exec_RawDoFmt+0x188>
    4412:	7200           	moveq #0,%d1
    4414:	321c           	movew %a4@+,%d1
    4416:	244e           	moveal %fp,%a2
    4418:	41f9 0000 0000 	lea 0 <main>,%a0
    441e:	700f           	moveq #15,%d0
    4420:	c081           	andl %d1,%d0
    4422:	1530 0800      	moveb %a0@(00000000,%d0:l),%a2@-
    4426:	e889           	lsrl #4,%d1
    4428:	5284           	addql #1,%d4
    442a:	4a81           	tstl %d1
    442c:	66f0           	bnes 441e <Exec_RawDoFmt+0x190>
    442e:	6050           	bras 4480 <Exec_RawDoFmt+0x1f2>
    4430:	200c           	movel %a4,%d0
    4432:	5680           	addql #3,%d0
    4434:	7efc           	moveq #-4,%d7
    4436:	ce80           	andl %d0,%d7
    4438:	2847           	moveal %d7,%a4
    443a:	245c           	moveal %a4@+,%a2
    443c:	204a           	moveal %a2,%a0
    443e:	4a18           	tstb %a0@+
    4440:	66fc           	bnes 443e <Exec_RawDoFmt+0x1b0>
    4442:	200a           	movel %a2,%d0
    4444:	9088           	subl %a0,%d0
    4446:	2800           	movel %d0,%d4
    4448:	4684           	notl %d4
    444a:	b484           	cmpl %d4,%d2
    444c:	6432           	bccs 4480 <Exec_RawDoFmt+0x1f2>
    444e:	2802           	movel %d2,%d4
    4450:	602e           	bras 4480 <Exec_RawDoFmt+0x1f2>
    4452:	45ee fff6      	lea %fp@(-10),%a2
    4456:	7801           	moveq #1,%d4
    4458:	4a83           	tstl %d3
    445a:	6712           	beqs 446e <Exec_RawDoFmt+0x1e0>
    445c:	200c           	movel %a4,%d0
    445e:	5680           	addql #3,%d0
    4460:	7efc           	moveq #-4,%d7
    4462:	ce80           	andl %d0,%d7
    4464:	2847           	moveal %d7,%a4
    4466:	14ac 0003      	moveb %a4@(3),%a2@
    446a:	588c           	addql #4,%a4
    446c:	6012           	bras 4480 <Exec_RawDoFmt+0x1f2>
    446e:	14ac 0001      	moveb %a4@(1),%a2@
    4472:	548c           	addql #2,%a4
    4474:	600a           	bras 4480 <Exec_RawDoFmt+0x1f2>
    4476:	538b           	subql #1,%a3
    4478:	95ca           	subal %a2,%a2
    447a:	6004           	bras 4480 <Exec_RawDoFmt+0x1f2>
    447c:	244b           	moveal %a3,%a2
    447e:	7801           	moveq #1,%d4
    4480:	528b           	addql #1,%a3
    4482:	4aae fff2      	tstl %fp@(-14)
    4486:	6622           	bnes 44aa <Exec_RawDoFmt+0x21c>
    4488:	242e ffee      	movel %fp@(-18),%d2
    448c:	d484           	addl %d4,%d2
    448e:	ba82           	cmpl %d2,%d5
    4490:	6318           	blss 44aa <Exec_RawDoFmt+0x21c>
    4492:	7600           	moveq #0,%d3
    4494:	2f0d           	movel %a5,%sp@-
    4496:	162e ffed      	moveb %fp@(-19),%d3
    449a:	2f03           	movel %d3,%sp@-
    449c:	226e 0010      	moveal %fp@(16),%a1
    44a0:	4e91           	jsr %a1@
    44a2:	508f           	addql #8,%sp
    44a4:	5282           	addql #1,%d2
    44a6:	ba82           	cmpl %d2,%d5
    44a8:	62ea           	bhis 4494 <Exec_RawDoFmt+0x206>
    44aa:	4aae ffee      	tstl %fp@(-18)
    44ae:	670e           	beqs 44be <Exec_RawDoFmt+0x230>
    44b0:	2f0d           	movel %a5,%sp@-
    44b2:	4878 002d      	pea 2d <pause+0xd>
    44b6:	226e 0010      	moveal %fp@(16),%a1
    44ba:	4e91           	jsr %a1@
    44bc:	508f           	addql #8,%sp
    44be:	7400           	moveq #0,%d2
    44c0:	b882           	cmpl %d2,%d4
    44c2:	6316           	blss 44da <Exec_RawDoFmt+0x24c>
    44c4:	2602           	movel %d2,%d3
    44c6:	2f0d           	movel %a5,%sp@-
    44c8:	161a           	moveb %a2@+,%d3
    44ca:	2f03           	movel %d3,%sp@-
    44cc:	226e 0010      	moveal %fp@(16),%a1
    44d0:	4e91           	jsr %a1@
    44d2:	508f           	addql #8,%sp
    44d4:	5282           	addql #1,%d2
    44d6:	b882           	cmpl %d2,%d4
    44d8:	62ec           	bhis 44c6 <Exec_RawDoFmt+0x238>
    44da:	4aae fff2      	tstl %fp@(-14)
    44de:	6736           	beqs 4516 <Exec_RawDoFmt+0x288>
    44e0:	242e ffee      	movel %fp@(-18),%d2
    44e4:	d484           	addl %d4,%d2
    44e6:	ba82           	cmpl %d2,%d5
    44e8:	632c           	blss 4516 <Exec_RawDoFmt+0x288>
    44ea:	7600           	moveq #0,%d3
    44ec:	2f0d           	movel %a5,%sp@-
    44ee:	162e ffed      	moveb %fp@(-19),%d3
    44f2:	2f03           	movel %d3,%sp@-
    44f4:	226e 0010      	moveal %fp@(16),%a1
    44f8:	4e91           	jsr %a1@
    44fa:	508f           	addql #8,%sp
    44fc:	5282           	addql #1,%d2
    44fe:	ba82           	cmpl %d2,%d5
    4500:	62ea           	bhis 44ec <Exec_RawDoFmt+0x25e>
    4502:	6012           	bras 4516 <Exec_RawDoFmt+0x288>
    4504:	2f0d           	movel %a5,%sp@-
    4506:	42a7           	clrl %sp@-
    4508:	1f40 0003      	moveb %d0,%sp@(3)
    450c:	226e 0010      	moveal %fp@(16),%a1
    4510:	4e91           	jsr %a1@
    4512:	528b           	addql #1,%a3
    4514:	508f           	addql #8,%sp
    4516:	4a13           	tstb %a3@
    4518:	6600 fd90      	bnew 42aa <Exec_RawDoFmt+0x1c>
    451c:	2f0d           	movel %a5,%sp@-
    451e:	42a7           	clrl %sp@-
    4520:	226e 0010      	moveal %fp@(16),%a1
    4524:	4e91           	jsr %a1@
    4526:	204c           	moveal %a4,%a0
    4528:	2008           	movel %a0,%d0
    452a:	4cee 3cfc ffc0 	moveml %fp@(-64),%d2-%d7/%a2-%a5
    4530:	4e5e           	unlk %fp
    4532:	4e75           	rts

00004534 <Exec_RawIOInit>:
    4534:	4e56 0000      	linkw %fp,#0
    4538:	4e5e           	unlk %fp
    453a:	4e75           	rts

0000453c <Exec_RawMayGetChar>:
    453c:	4e56 0000      	linkw %fp,#0
    4540:	70ff           	moveq #-1,%d0
    4542:	4e5e           	unlk %fp
    4544:	4e75           	rts

00004546 <Exec_ReleaseSemaphore>:
    4546:	4e56 0000      	linkw %fp,#0
    454a:	48e7 3038      	moveml %d2-%d3/%a2-%a4,%sp@-
    454e:	266e 0008      	moveal %fp@(8),%a3
    4552:	286e 000c      	moveal %fp@(12),%a4
    4556:	0c2b 000f 000c 	cmpib #15,%a3@(12)
    455c:	672e           	beqs 458c <Exec_ReleaseSemaphore+0x46>
    455e:	2f0c           	movel %a4,%sp@-
    4560:	42a7           	clrl %sp@-
    4562:	206c fedc      	moveal %a4@(-292),%a0
    4566:	4e90           	jsr %a0@
    4568:	2f28 0008      	movel %a0@(8),%sp@-
    456c:	2f0c           	movel %a4,%sp@-
    456e:	42a7           	clrl %sp@-
    4570:	206c fedc      	moveal %a4@(-292),%a0
    4574:	4e90           	jsr %a0@
    4576:	584f           	addqw #4,%sp
    4578:	2e88           	movel %a0,%sp@
    457a:	2f0b           	movel %a3,%sp@-
    457c:	4879 0000 0000 	pea 0 <main>
    4582:	61ff 0000 0000 	bsrl 4584 <Exec_ReleaseSemaphore+0x3e>
    4588:	4fef 0018      	lea %sp@(24),%sp
    458c:	2f0c           	movel %a4,%sp@-
    458e:	206c ff7e      	moveal %a4@(-130),%a0
    4592:	4e90           	jsr %a0@
    4594:	536b 000e      	subqw #1,%a3@(14)
    4598:	302b 002c      	movew %a3@(44),%d0
    459c:	3600           	movew %d0,%d3
    459e:	5343           	subqw #1,%d3
    45a0:	3743 002c      	movew %d3,%a3@(44)
    45a4:	588f           	addql #4,%sp
    45a6:	322b 000e      	movew %a3@(14),%d1
    45aa:	6600 00c2      	bnew 466e <Exec_ReleaseSemaphore+0x128>
    45ae:	5340           	subqw #1,%d0
    45b0:	6b00 00b0      	bmiw 4662 <Exec_ReleaseSemaphore+0x11c>
    45b4:	246b 0010      	moveal %a3@(16),%a2
    45b8:	4a92           	tstl %a2@
    45ba:	6700 00a6      	beqw 4662 <Exec_ReleaseSemaphore+0x11c>
    45be:	082a 0000 000b 	btst #0,%a2@(11)
    45c4:	675a           	beqs 4620 <Exec_ReleaseSemaphore+0xda>
    45c6:	42ab 0028      	clrl %a3@(40)
    45ca:	4a92           	tstl %a2@
    45cc:	6700 00b4      	beqw 4682 <Exec_ReleaseSemaphore+0x13c>
    45d0:	2412           	movel %a2@,%d2
    45d2:	082a 0000 000b 	btst #0,%a2@(11)
    45d8:	673e           	beqs 4618 <Exec_ReleaseSemaphore+0xd2>
    45da:	2f0a           	movel %a2,%sp@-
    45dc:	206c ff06      	moveal %a4@(-250),%a0
    45e0:	4e90           	jsr %a0@
    45e2:	76fe           	moveq #-2,%d3
    45e4:	c7aa 0008      	andl %d3,%a2@(8)
    45e8:	526b 000e      	addqw #1,%a3@(14)
    45ec:	588f           	addql #4,%sp
    45ee:	202a 0008      	movel %a2@(8),%d0
    45f2:	6714           	beqs 4608 <Exec_ReleaseSemaphore+0xc2>
    45f4:	2f0c           	movel %a4,%sp@-
    45f6:	4878 0010      	pea 10 <main+0x10>
    45fa:	2f00           	movel %d0,%sp@-
    45fc:	206c febe      	moveal %a4@(-322),%a0
    4600:	4e90           	jsr %a0@
    4602:	4fef 000c      	lea %sp@(12),%sp
    4606:	6010           	bras 4618 <Exec_ReleaseSemaphore+0xd2>
    4608:	254b 0014      	movel %a3,%a2@(20)
    460c:	2f0c           	movel %a4,%sp@-
    460e:	2f0a           	movel %a2,%sp@-
    4610:	206c fe88      	moveal %a4@(-376),%a0
    4614:	4e90           	jsr %a0@
    4616:	508f           	addql #8,%sp
    4618:	2442           	moveal %d2,%a2
    461a:	4a92           	tstl %a2@
    461c:	66b2           	bnes 45d0 <Exec_ReleaseSemaphore+0x8a>
    461e:	6062           	bras 4682 <Exec_ReleaseSemaphore+0x13c>
    4620:	2f0a           	movel %a2,%sp@-
    4622:	206c ff06      	moveal %a4@(-250),%a0
    4626:	4e90           	jsr %a0@
    4628:	526b 000e      	addqw #1,%a3@(14)
    462c:	588f           	addql #4,%sp
    462e:	202a 0008      	movel %a2@(8),%d0
    4632:	671a           	beqs 464e <Exec_ReleaseSemaphore+0x108>
    4634:	2740 0028      	movel %d0,%a3@(40)
    4638:	2f0c           	movel %a4,%sp@-
    463a:	4878 0010      	pea 10 <main+0x10>
    463e:	2f2a 0008      	movel %a2@(8),%sp@-
    4642:	206c febe      	moveal %a4@(-322),%a0
    4646:	4e90           	jsr %a0@
    4648:	4fef 000c      	lea %sp@(12),%sp
    464c:	6034           	bras 4682 <Exec_ReleaseSemaphore+0x13c>
    464e:	276a 0014 0028 	movel %a2@(20),%a3@(40)
    4654:	254b 0014      	movel %a3,%a2@(20)
    4658:	2f0c           	movel %a4,%sp@-
    465a:	2f0a           	movel %a2,%sp@-
    465c:	206c fe88      	moveal %a4@(-376),%a0
    4660:	601c           	bras 467e <Exec_ReleaseSemaphore+0x138>
    4662:	42ab 0028      	clrl %a3@(40)
    4666:	377c ffff 002c 	movew #-1,%a3@(44)
    466c:	6014           	bras 4682 <Exec_ReleaseSemaphore+0x13c>
    466e:	4a41           	tstw %d1
    4670:	6c10           	bges 4682 <Exec_ReleaseSemaphore+0x13c>
    4672:	2f0c           	movel %a4,%sp@-
    4674:	2f3c 0100 0008 	movel #16777224,%sp@-
    467a:	206c ff96      	moveal %a4@(-106),%a0
    467e:	4e90           	jsr %a0@
    4680:	508f           	addql #8,%sp
    4682:	2f0c           	movel %a4,%sp@-
    4684:	206c ff78      	moveal %a4@(-136),%a0
    4688:	4e90           	jsr %a0@
    468a:	4cee 1c0c ffec 	moveml %fp@(-20),%d2-%d3/%a2-%a4
    4690:	4e5e           	unlk %fp
    4692:	4e75           	rts

00004694 <Exec_ReleaseSemaphoreList>:
    4694:	4e56 0000      	linkw %fp,#0
    4698:	2f0b           	movel %a3,%sp@-
    469a:	2f0a           	movel %a2,%sp@-
    469c:	206e 0008      	moveal %fp@(8),%a0
    46a0:	266e 000c      	moveal %fp@(12),%a3
    46a4:	2450           	moveal %a0@,%a2
    46a6:	4a92           	tstl %a2@
    46a8:	6712           	beqs 46bc <Exec_ReleaseSemaphoreList+0x28>
    46aa:	2f0b           	movel %a3,%sp@-
    46ac:	2f0a           	movel %a2,%sp@-
    46ae:	206b fdc8      	moveal %a3@(-568),%a0
    46b2:	4e90           	jsr %a0@
    46b4:	508f           	addql #8,%sp
    46b6:	2452           	moveal %a2@,%a2
    46b8:	4a92           	tstl %a2@
    46ba:	66ee           	bnes 46aa <Exec_ReleaseSemaphoreList+0x16>
    46bc:	246e fff8      	moveal %fp@(-8),%a2
    46c0:	266e fffc      	moveal %fp@(-4),%a3
    46c4:	4e5e           	unlk %fp
    46c6:	4e75           	rts

000046c8 <Exec_RemDevice>:
    46c8:	4e56 0000      	linkw %fp,#0
    46cc:	2f0b           	movel %a3,%sp@-
    46ce:	2f0a           	movel %a2,%sp@-
    46d0:	266e 0008      	moveal %fp@(8),%a3
    46d4:	246e 000c      	moveal %fp@(12),%a2
    46d8:	2f0a           	movel %a2,%sp@-
    46da:	206a ff7e      	moveal %a2@(-130),%a0
    46de:	4e90           	jsr %a0@
    46e0:	2f0b           	movel %a3,%sp@-
    46e2:	206b fff0      	moveal %a3@(-16),%a0
    46e6:	4e90           	jsr %a0@
    46e8:	2f0a           	movel %a2,%sp@-
    46ea:	206a ff78      	moveal %a2@(-136),%a0
    46ee:	4e90           	jsr %a0@
    46f0:	246e fff8      	moveal %fp@(-8),%a2
    46f4:	266e fffc      	moveal %fp@(-4),%a3
    46f8:	4e5e           	unlk %fp
    46fa:	4e75           	rts

000046fc <Exec_RemHead>:
    46fc:	4e56 0000      	linkw %fp,#0
    4700:	226e 0008      	moveal %fp@(8),%a1
    4704:	2051           	moveal %a1@,%a0
    4706:	2050           	moveal %a0@,%a0
    4708:	b0fc 0000      	cmpaw #0,%a0
    470c:	6708           	beqs 4716 <Exec_RemHead+0x1a>
    470e:	2149 0004      	movel %a1,%a0@(4)
    4712:	2051           	moveal %a1@,%a0
    4714:	2290           	movel %a0@,%a1@
    4716:	2008           	movel %a0,%d0
    4718:	4e5e           	unlk %fp
    471a:	4e75           	rts

0000471c <Exec_RemIntServer>:
    471c:	4e56 0000      	linkw %fp,#0
    4720:	2f0a           	movel %a2,%sp@-
    4722:	2f02           	movel %d2,%sp@-
    4724:	242e 000c      	movel %fp@(12),%d2
    4728:	246e 0010      	moveal %fp@(16),%a2
    472c:	2f0a           	movel %a2,%sp@-
    472e:	206a ff8a      	moveal %a2@(-118),%a0
    4732:	4e90           	jsr %a0@
    4734:	2f02           	movel %d2,%sp@-
    4736:	206a ff06      	moveal %a2@(-250),%a0
    473a:	4e90           	jsr %a0@
    473c:	2f0a           	movel %a2,%sp@-
    473e:	206a ff84      	moveal %a2@(-124),%a0
    4742:	4e90           	jsr %a0@
    4744:	242e fff8      	movel %fp@(-8),%d2
    4748:	246e fffc      	moveal %fp@(-4),%a2
    474c:	4e5e           	unlk %fp
    474e:	4e75           	rts

00004750 <Exec_RemLibrary>:
    4750:	4e56 0000      	linkw %fp,#0
    4754:	2f0b           	movel %a3,%sp@-
    4756:	2f0a           	movel %a2,%sp@-
    4758:	266e 0008      	moveal %fp@(8),%a3
    475c:	246e 000c      	moveal %fp@(12),%a2
    4760:	2f0a           	movel %a2,%sp@-
    4762:	206a ff7e      	moveal %a2@(-130),%a0
    4766:	4e90           	jsr %a0@
    4768:	2f0b           	movel %a3,%sp@-
    476a:	206b fff0      	moveal %a3@(-16),%a0
    476e:	4e90           	jsr %a0@
    4770:	2f0a           	movel %a2,%sp@-
    4772:	206a ff78      	moveal %a2@(-136),%a0
    4776:	4e90           	jsr %a0@
    4778:	246e fff8      	moveal %fp@(-8),%a2
    477c:	266e fffc      	moveal %fp@(-4),%a3
    4780:	4e5e           	unlk %fp
    4782:	4e75           	rts

00004784 <Exec_RemMemHandler>:
    4784:	4e56 0000      	linkw %fp,#0
    4788:	2f0a           	movel %a2,%sp@-
    478a:	2f02           	movel %d2,%sp@-
    478c:	242e 0008      	movel %fp@(8),%d2
    4790:	246e 000c      	moveal %fp@(12),%a2
    4794:	2f0a           	movel %a2,%sp@-
    4796:	206a ff7e      	moveal %a2@(-130),%a0
    479a:	4e90           	jsr %a0@
    479c:	2f02           	movel %d2,%sp@-
    479e:	206a ff06      	moveal %a2@(-250),%a0
    47a2:	4e90           	jsr %a0@
    47a4:	2f0a           	movel %a2,%sp@-
    47a6:	206a ff78      	moveal %a2@(-136),%a0
    47aa:	4e90           	jsr %a0@
    47ac:	242e fff8      	movel %fp@(-8),%d2
    47b0:	246e fffc      	moveal %fp@(-4),%a2
    47b4:	4e5e           	unlk %fp
    47b6:	4e75           	rts

000047b8 <Exec_Remove>:
    47b8:	4e56 0000      	linkw %fp,#0
    47bc:	206e 0008      	moveal %fp@(8),%a0
    47c0:	2268 0004      	moveal %a0@(4),%a1
    47c4:	2290           	movel %a0@,%a1@
    47c6:	2250           	moveal %a0@,%a1
    47c8:	2368 0004 0004 	movel %a0@(4),%a1@(4)
    47ce:	4e5e           	unlk %fp
    47d0:	4e75           	rts

000047d2 <Exec_RemPort>:
    47d2:	4e56 0000      	linkw %fp,#0
    47d6:	2f0a           	movel %a2,%sp@-
    47d8:	2f02           	movel %d2,%sp@-
    47da:	242e 0008      	movel %fp@(8),%d2
    47de:	246e 000c      	moveal %fp@(12),%a2
    47e2:	2f0a           	movel %a2,%sp@-
    47e4:	206a ff7e      	moveal %a2@(-130),%a0
    47e8:	4e90           	jsr %a0@
    47ea:	2f02           	movel %d2,%sp@-
    47ec:	206a ff06      	moveal %a2@(-250),%a0
    47f0:	4e90           	jsr %a0@
    47f2:	2f0a           	movel %a2,%sp@-
    47f4:	206a ff78      	moveal %a2@(-136),%a0
    47f8:	4e90           	jsr %a0@
    47fa:	242e fff8      	movel %fp@(-8),%d2
    47fe:	246e fffc      	moveal %fp@(-4),%a2
    4802:	4e5e           	unlk %fp
    4804:	4e75           	rts

00004806 <Exec_RemResource>:
    4806:	4e56 0000      	linkw %fp,#0
    480a:	2f0a           	movel %a2,%sp@-
    480c:	2f02           	movel %d2,%sp@-
    480e:	242e 0008      	movel %fp@(8),%d2
    4812:	246e 000c      	moveal %fp@(12),%a2
    4816:	2f0a           	movel %a2,%sp@-
    4818:	206a ff7e      	moveal %a2@(-130),%a0
    481c:	4e90           	jsr %a0@
    481e:	2f02           	movel %d2,%sp@-
    4820:	206a ff06      	moveal %a2@(-250),%a0
    4824:	4e90           	jsr %a0@
    4826:	2f0a           	movel %a2,%sp@-
    4828:	206a ff78      	moveal %a2@(-136),%a0
    482c:	4e90           	jsr %a0@
    482e:	242e fff8      	movel %fp@(-8),%d2
    4832:	246e fffc      	moveal %fp@(-4),%a2
    4836:	4e5e           	unlk %fp
    4838:	4e75           	rts

0000483a <Exec_RemSemaphore>:
    483a:	4e56 0000      	linkw %fp,#0
    483e:	2f0a           	movel %a2,%sp@-
    4840:	2f02           	movel %d2,%sp@-
    4842:	242e 0008      	movel %fp@(8),%d2
    4846:	246e 000c      	moveal %fp@(12),%a2
    484a:	2f0a           	movel %a2,%sp@-
    484c:	206a ff7e      	moveal %a2@(-130),%a0
    4850:	4e90           	jsr %a0@
    4852:	2f02           	movel %d2,%sp@-
    4854:	206a ff06      	moveal %a2@(-250),%a0
    4858:	4e90           	jsr %a0@
    485a:	2f0a           	movel %a2,%sp@-
    485c:	206a ff78      	moveal %a2@(-136),%a0
    4860:	4e90           	jsr %a0@
    4862:	242e fff8      	movel %fp@(-8),%d2
    4866:	246e fffc      	moveal %fp@(-4),%a2
    486a:	4e5e           	unlk %fp
    486c:	4e75           	rts

0000486e <Exec_RemTail>:
    486e:	4e56 0000      	linkw %fp,#0
    4872:	2f0a           	movel %a2,%sp@-
    4874:	206e 0008      	moveal %fp@(8),%a0
    4878:	2228 0008      	movel %a0@(8),%d1
    487c:	2441           	moveal %d1,%a2
    487e:	4aaa 0004      	tstl %a2@(4)
    4882:	56c0           	sne %d0
    4884:	4880           	extw %d0
    4886:	48c0           	extl %d0
    4888:	c280           	andl %d0,%d1
    488a:	2241           	moveal %d1,%a1
    488c:	2009           	movel %a1,%d0
    488e:	670e           	beqs 489e <Exec_RemTail+0x30>
    4890:	2069 0004      	moveal %a1@(4),%a0
    4894:	2091           	movel %a1@,%a0@
    4896:	2051           	moveal %a1@,%a0
    4898:	2169 0004 0004 	movel %a1@(4),%a0@(4)
    489e:	2040           	moveal %d0,%a0
    48a0:	245f           	moveal %sp@+,%a2
    48a2:	4e5e           	unlk %fp
    48a4:	4e75           	rts

000048a6 <Exec_RemTask>:
    48a6:	4e56 0000      	linkw %fp,#0
    48aa:	48e7 2030      	moveml %d2/%a2-%a3,%sp@-
    48ae:	266e 0008      	moveal %fp@(8),%a3
    48b2:	246e 000c      	moveal %fp@(12),%a2
    48b6:	b6fc 0000      	cmpaw #0,%a3
    48ba:	6604           	bnes 48c0 <Exec_RemTask+0x1a>
    48bc:	266a 0114      	moveal %a2@(276),%a3
    48c0:	7400           	moveq #0,%d2
    48c2:	082b 0003 000e 	btst #3,%a3@(14)
    48c8:	6704           	beqs 48ce <Exec_RemTask+0x28>
    48ca:	242b 0022      	movel %a3@(34),%d2
    48ce:	2f0a           	movel %a2,%sp@-
    48d0:	206a ff7e      	moveal %a2@(-130),%a0
    48d4:	4e90           	jsr %a0@
    48d6:	588f           	addql #4,%sp
    48d8:	486b 004a      	pea %a3@(74)
    48dc:	206a ff00      	moveal %a2@(-256),%a0
    48e0:	4e90           	jsr %a0@
    48e2:	2008           	movel %a0,%d0
    48e4:	588f           	addql #4,%sp
    48e6:	670e           	beqs 48f6 <Exec_RemTask+0x50>
    48e8:	2f0a           	movel %a2,%sp@-
    48ea:	2f00           	movel %d0,%sp@-
    48ec:	206a ff1e      	moveal %a2@(-226),%a0
    48f0:	4e90           	jsr %a0@
    48f2:	508f           	addql #8,%sp
    48f4:	60e2           	bras 48d8 <Exec_RemTask+0x32>
    48f6:	4a82           	tstl %d2
    48f8:	670c           	beqs 4906 <Exec_RemTask+0x60>
    48fa:	2f0a           	movel %a2,%sp@-
    48fc:	2f02           	movel %d2,%sp@-
    48fe:	206a fd50      	moveal %a2@(-688),%a0
    4902:	4e90           	jsr %a0@
    4904:	508f           	addql #8,%sp
    4906:	2f0a           	movel %a2,%sp@-
    4908:	206a ff8a      	moveal %a2@(-118),%a0
    490c:	4e90           	jsr %a0@
    490e:	588f           	addql #4,%sp
    4910:	b7ea 0114      	cmpal %a2@(276),%a3
    4914:	6612           	bnes 4928 <Exec_RemTask+0x82>
    4916:	177c 0006 000f 	moveb #6,%a3@(15)
    491c:	50ea 0127      	st %a2@(295)
    4920:	2f0a           	movel %a2,%sp@-
    4922:	206a ffcc      	moveal %a2@(-52),%a0
    4926:	6006           	bras 492e <Exec_RemTask+0x88>
    4928:	2f0b           	movel %a3,%sp@-
    492a:	206a ff06      	moveal %a2@(-250),%a0
    492e:	4e90           	jsr %a0@
    4930:	2e8a           	movel %a2,%sp@
    4932:	206a ff84      	moveal %a2@(-124),%a0
    4936:	4e90           	jsr %a0@
    4938:	2f0a           	movel %a2,%sp@-
    493a:	206a ff78      	moveal %a2@(-136),%a0
    493e:	4e90           	jsr %a0@
    4940:	4cee 0c04 fff4 	moveml %fp@(-12),%d2/%a2-%a3
    4946:	4e5e           	unlk %fp
    4948:	4e75           	rts

0000494a <Exec_ReplyMsg>:
    494a:	4e56 0000      	linkw %fp,#0
    494e:	48e7 2038      	moveml %d2/%a2-%a4,%sp@-
    4952:	266e 0008      	moveal %fp@(8),%a3
    4956:	286e 000c      	moveal %fp@(12),%a4
    495a:	2f0c           	movel %a4,%sp@-
    495c:	206c ff8a      	moveal %a4@(-118),%a0
    4960:	4e90           	jsr %a0@
    4962:	246b 000e      	moveal %a3@(14),%a2
    4966:	588f           	addql #4,%sp
    4968:	b4fc 0000      	cmpaw #0,%a2
    496c:	6608           	bnes 4976 <Exec_ReplyMsg+0x2c>
    496e:	177c 0006 000c 	moveb #6,%a3@(12)
    4974:	605a           	bras 49d0 <Exec_ReplyMsg+0x86>
    4976:	177c 0007 000c 	moveb #7,%a3@(12)
    497c:	2f0b           	movel %a3,%sp@-
    497e:	486a 0014      	pea %a2@(20)
    4982:	206c ff0c      	moveal %a4@(-244),%a0
    4986:	4e90           	jsr %a0@
    4988:	508f           	addql #8,%sp
    498a:	206a 0010      	moveal %a2@(16),%a0
    498e:	b0fc 0000      	cmpaw #0,%a0
    4992:	673c           	beqs 49d0 <Exec_ReplyMsg+0x86>
    4994:	102a 000e      	moveb %a2@(14),%d0
    4998:	7403           	moveq #3,%d2
    499a:	c082           	andl %d2,%d0
    499c:	7401           	moveq #1,%d2
    499e:	b480           	cmpl %d0,%d2
    49a0:	6722           	beqs 49c4 <Exec_ReplyMsg+0x7a>
    49a2:	6d2c           	blts 49d0 <Exec_ReplyMsg+0x86>
    49a4:	4a80           	tstl %d0
    49a6:	6628           	bnes 49d0 <Exec_ReplyMsg+0x86>
    49a8:	2f0c           	movel %a4,%sp@-
    49aa:	4281           	clrl %d1
    49ac:	122a 000f      	moveb %a2@(15),%d1
    49b0:	2002           	movel %d2,%d0
    49b2:	e3a8           	lsll %d1,%d0
    49b4:	2f00           	movel %d0,%sp@-
    49b6:	2f08           	movel %a0,%sp@-
    49b8:	206c febe      	moveal %a4@(-322),%a0
    49bc:	4e90           	jsr %a0@
    49be:	4fef 000c      	lea %sp@(12),%sp
    49c2:	600c           	bras 49d0 <Exec_ReplyMsg+0x86>
    49c4:	2f0c           	movel %a4,%sp@-
    49c6:	2f08           	movel %a0,%sp@-
    49c8:	206c ff4e      	moveal %a4@(-178),%a0
    49cc:	4e90           	jsr %a0@
    49ce:	508f           	addql #8,%sp
    49d0:	2f0c           	movel %a4,%sp@-
    49d2:	206c ff84      	moveal %a4@(-124),%a0
    49d6:	4e90           	jsr %a0@
    49d8:	4cee 1c04 fff0 	moveml %fp@(-16),%d2/%a2-%a4
    49de:	4e5e           	unlk %fp
    49e0:	4e75           	rts

000049e2 <Exec_Reschedule>:
    49e2:	4e56 0000      	linkw %fp,#0
    49e6:	206e 0008      	moveal %fp@(8),%a0
    49ea:	226e 000c      	moveal %fp@(12),%a1
    49ee:	4280           	clrl %d0
    49f0:	1028 000f      	moveb %a0@(15),%d0
    49f4:	7206           	moveq #6,%d1
    49f6:	b280           	cmpl %d0,%d1
    49f8:	6550           	bcss 4a4a <Exec_Reschedule+0x68>
    49fa:	d080           	addl %d0,%d0
    49fc:	303b 0806      	movew %pc@(4a04 <Exec_Reschedule+0x22>,%d0:l),%d0
    4a00:	4efb 0002      	jmp %pc@(4a04 <Exec_Reschedule+0x22>,%d0:w)
    4a04:	002a 000e 002a 	orib #14,%a2@(42)
    4a0a:	000e 001c      	orib #28,%fp
    4a0e:	002a 0046 2f08 	orib #70,%a2@(12040)
    4a14:	4869 0196      	pea %a1@(406)
    4a18:	2069 fef4      	moveal %a1@(-268),%a0
    4a1c:	4e90           	jsr %a0@
    4a1e:	602a           	bras 4a4a <Exec_Reschedule+0x68>
    4a20:	2f08           	movel %a0,%sp@-
    4a22:	4869 01a4      	pea %a1@(420)
    4a26:	2069 ff0c      	moveal %a1@(-244),%a0
    4a2a:	4e90           	jsr %a0@
    4a2c:	601c           	bras 4a4a <Exec_Reschedule+0x68>
    4a2e:	4879 0000 0000 	pea 0 <main>
    4a34:	4878 0073      	pea 73 <drawlinehoriz+0xd>
    4a38:	4879 0000 0000 	pea 0 <main>
    4a3e:	4879 0000 0000 	pea 0 <main>
    4a44:	61ff 0000 0000 	bsrl 4a46 <Exec_Reschedule+0x64>
    4a4a:	4e5e           	unlk %fp
    4a4c:	4e75           	rts

00004a4e <Exec_SendIO>:
    4a4e:	4e56 0000      	linkw %fp,#0
    4a52:	206e 0008      	moveal %fp@(8),%a0
    4a56:	4228 001e      	clrb %a0@(30)
    4a5a:	4228 000c      	clrb %a0@(12)
    4a5e:	2268 0014      	moveal %a0@(20),%a1
    4a62:	2f09           	movel %a1,%sp@-
    4a64:	2f08           	movel %a0,%sp@-
    4a66:	2069 ffe4      	moveal %a1@(-28),%a0
    4a6a:	4e90           	jsr %a0@
    4a6c:	4e5e           	unlk %fp
    4a6e:	4e75           	rts

00004a70 <Exec_SetExcept>:
    4a70:	4e56 0000      	linkw %fp,#0
    4a74:	48e7 3830      	moveml %d2-%d4/%a2-%a3,%sp@-
    4a78:	242e 0008      	movel %fp@(8),%d2
    4a7c:	262e 000c      	movel %fp@(12),%d3
    4a80:	266e 0010      	moveal %fp@(16),%a3
    4a84:	246b 0114      	moveal %a3@(276),%a2
    4a88:	2f0b           	movel %a3,%sp@-
    4a8a:	206b ff8a      	moveal %a3@(-118),%a0
    4a8e:	4e90           	jsr %a0@
    4a90:	282a 001e      	movel %a2@(30),%d4
    4a94:	2003           	movel %d3,%d0
    4a96:	4680           	notl %d0
    4a98:	c084           	andl %d4,%d0
    4a9a:	c483           	andl %d3,%d2
    4a9c:	8082           	orl %d2,%d0
    4a9e:	2540 001e      	movel %d0,%a2@(30)
    4aa2:	c0aa 001a      	andl %a2@(26),%d0
    4aa6:	588f           	addql #4,%sp
    4aa8:	6724           	beqs 4ace <Exec_SetExcept+0x5e>
    4aaa:	002a 0020 000e 	orib #32,%a2@(14)
    4ab0:	4a2b 0127      	tstb %a3@(295)
    4ab4:	6c06           	bges 4abc <Exec_SetExcept+0x4c>
    4ab6:	4a2b 0126      	tstb %a3@(294)
    4aba:	6f08           	bles 4ac4 <Exec_SetExcept+0x54>
    4abc:	006b 0080 012a 	oriw #128,%a3@(298)
    4ac2:	600a           	bras 4ace <Exec_SetExcept+0x5e>
    4ac4:	2f0b           	movel %a3,%sp@-
    4ac6:	206b ffcc      	moveal %a3@(-52),%a0
    4aca:	4e90           	jsr %a0@
    4acc:	588f           	addql #4,%sp
    4ace:	2f0b           	movel %a3,%sp@-
    4ad0:	206b ff84      	moveal %a3@(-124),%a0
    4ad4:	4e90           	jsr %a0@
    4ad6:	2004           	movel %d4,%d0
    4ad8:	4cee 0c1c ffec 	moveml %fp@(-20),%d2-%d4/%a2-%a3
    4ade:	4e5e           	unlk %fp
    4ae0:	4e75           	rts

00004ae2 <Exec_SetFunction>:
    4ae2:	4e56 0000      	linkw %fp,#0
    4ae6:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    4aea:	266e 0008      	moveal %fp@(8),%a3
    4aee:	262e 0010      	movel %fp@(16),%d3
    4af2:	246e 0014      	moveal %fp@(20),%a2
    4af6:	4878 0006      	pea 6 <main+0x6>
    4afa:	222e 000c      	movel %fp@(12),%d1
    4afe:	4481           	negl %d1
    4b00:	2f01           	movel %d1,%sp@-
    4b02:	61ff 0000 0000 	bsrl 4b04 <Exec_SetFunction+0x22>
    4b08:	508f           	addql #8,%sp
    4b0a:	2400           	movel %d0,%d2
    4b0c:	2f0a           	movel %a2,%sp@-
    4b0e:	206a ff7e      	moveal %a2@(-130),%a0
    4b12:	4e90           	jsr %a0@
    4b14:	002b 0002 000e 	orib #2,%a3@(14)
    4b1a:	2002           	movel %d2,%d0
    4b1c:	e588           	lsll #2,%d0
    4b1e:	d082           	addl %d2,%d0
    4b20:	d082           	addl %d2,%d0
    4b22:	204b           	moveal %a3,%a0
    4b24:	91c0           	subal %d0,%a0
    4b26:	2428 0002      	movel %a0@(2),%d2
    4b2a:	30bc 4ef9      	movew #20217,%a0@
    4b2e:	2143 0002      	movel %d3,%a0@(2)
    4b32:	2f0a           	movel %a2,%sp@-
    4b34:	206a fd86      	moveal %a2@(-634),%a0
    4b38:	4e90           	jsr %a0@
    4b3a:	2f0a           	movel %a2,%sp@-
    4b3c:	206a ff78      	moveal %a2@(-136),%a0
    4b40:	4e90           	jsr %a0@
    4b42:	2f0a           	movel %a2,%sp@-
    4b44:	2f0b           	movel %a3,%sp@-
    4b46:	206a fe58      	moveal %a2@(-424),%a0
    4b4a:	4e90           	jsr %a0@
    4b4c:	2042           	moveal %d2,%a0
    4b4e:	2008           	movel %a0,%d0
    4b50:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    4b56:	4e5e           	unlk %fp
    4b58:	4e75           	rts

00004b5a <Exec_SetIntVector>:
    4b5a:	4e56 0000      	linkw %fp,#0
    4b5e:	48e7 2030      	moveml %d2/%a2-%a3,%sp@-
    4b62:	242e 0008      	movel %fp@(8),%d2
    4b66:	266e 000c      	moveal %fp@(12),%a3
    4b6a:	246e 0010      	moveal %fp@(16),%a2
    4b6e:	2f0a           	movel %a2,%sp@-
    4b70:	206a ff8a      	moveal %a2@(-118),%a0
    4b74:	4e90           	jsr %a0@
    4b76:	2002           	movel %d2,%d0
    4b78:	d080           	addl %d0,%d0
    4b7a:	d082           	addl %d2,%d0
    4b7c:	e588           	lsll #2,%d0
    4b7e:	2432 085c      	movel %a2@(0000005c,%d0:l),%d2
    4b82:	258b 085c      	movel %a3,%a2@(0000005c,%d0:l)
    4b86:	588f           	addql #4,%sp
    4b88:	670e           	beqs 4b98 <Exec_SetIntVector+0x3e>
    4b8a:	25ab 000e 0854 	movel %a3@(14),%a2@(00000054,%d0:l)
    4b90:	25ab 0012 0858 	movel %a3@(18),%a2@(00000058,%d0:l)
    4b96:	600a           	bras 4ba2 <Exec_SetIntVector+0x48>
    4b98:	72ff           	moveq #-1,%d1
    4b9a:	2581 0854      	movel %d1,%a2@(00000054,%d0:l)
    4b9e:	2581 0858      	movel %d1,%a2@(00000058,%d0:l)
    4ba2:	2f0a           	movel %a2,%sp@-
    4ba4:	206a ff84      	moveal %a2@(-124),%a0
    4ba8:	4e90           	jsr %a0@
    4baa:	2042           	moveal %d2,%a0
    4bac:	2008           	movel %a0,%d0
    4bae:	4cee 0c04 fff4 	moveml %fp@(-12),%d2/%a2-%a3
    4bb4:	4e5e           	unlk %fp
    4bb6:	4e75           	rts

00004bb8 <Exec_SetSignal>:
    4bb8:	4e56 0000      	linkw %fp,#0
    4bbc:	48e7 3820      	moveml %d2-%d4/%a2,%sp@-
    4bc0:	242e 0008      	movel %fp@(8),%d2
    4bc4:	282e 000c      	movel %fp@(12),%d4
    4bc8:	246e 0010      	moveal %fp@(16),%a2
    4bcc:	2f0a           	movel %a2,%sp@-
    4bce:	206a ff8a      	moveal %a2@(-118),%a0
    4bd2:	4e90           	jsr %a0@
    4bd4:	226a 0114      	moveal %a2@(276),%a1
    4bd8:	41e9 001a      	lea %a1@(26),%a0
    4bdc:	2610           	movel %a0@,%d3
    4bde:	2004           	movel %d4,%d0
    4be0:	4680           	notl %d0
    4be2:	c083           	andl %d3,%d0
    4be4:	c484           	andl %d4,%d2
    4be6:	8082           	orl %d2,%d0
    4be8:	2080           	movel %d0,%a0@
    4bea:	2f0a           	movel %a2,%sp@-
    4bec:	206a ff84      	moveal %a2@(-124),%a0
    4bf0:	4e90           	jsr %a0@
    4bf2:	2003           	movel %d3,%d0
    4bf4:	4cee 041c fff0 	moveml %fp@(-16),%d2-%d4/%a2
    4bfa:	4e5e           	unlk %fp
    4bfc:	4e75           	rts

00004bfe <Exec_SetSR>:
    4bfe:	4e56 0000      	linkw %fp,#0
    4c02:	70ff           	moveq #-1,%d0
    4c04:	4e5e           	unlk %fp
    4c06:	4e75           	rts

00004c08 <Exec_SetTaskPri>:
    4c08:	4e56 0000      	linkw %fp,#0
    4c0c:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    4c10:	266e 0008      	moveal %fp@(8),%a3
    4c14:	242e 000c      	movel %fp@(12),%d2
    4c18:	246e 0010      	moveal %fp@(16),%a2
    4c1c:	2f0a           	movel %a2,%sp@-
    4c1e:	206a ff8a      	moveal %a2@(-118),%a0
    4c22:	4e90           	jsr %a0@
    4c24:	162b 000d      	moveb %a3@(13),%d3
    4c28:	1742 000d      	moveb %d2,%a3@(13)
    4c2c:	102b 000f      	moveb %a3@(15),%d0
    4c30:	588f           	addql #4,%sp
    4c32:	0c00 0004      	cmpib #4,%d0
    4c36:	6756           	beqs 4c8e <Exec_SetTaskPri+0x86>
    4c38:	0c00 0003      	cmpib #3,%d0
    4c3c:	6618           	bnes 4c56 <Exec_SetTaskPri+0x4e>
    4c3e:	2f0b           	movel %a3,%sp@-
    4c40:	206a ff06      	moveal %a2@(-250),%a0
    4c44:	4e90           	jsr %a0@
    4c46:	2f0b           	movel %a3,%sp@-
    4c48:	486a 0196      	pea %a2@(406)
    4c4c:	206a fef4      	moveal %a2@(-268),%a0
    4c50:	4e90           	jsr %a0@
    4c52:	4fef 000c      	lea %sp@(12),%sp
    4c56:	4a2a 0127      	tstb %a2@(295)
    4c5a:	6c06           	bges 4c62 <Exec_SetTaskPri+0x5a>
    4c5c:	4a2a 0126      	tstb %a2@(294)
    4c60:	6f08           	bles 4c6a <Exec_SetTaskPri+0x62>
    4c62:	006a 0080 012a 	oriw #128,%a2@(298)
    4c68:	6024           	bras 4c8e <Exec_SetTaskPri+0x86>
    4c6a:	206a 0114      	moveal %a2@(276),%a0
    4c6e:	117c 0003 000f 	moveb #3,%a0@(15)
    4c74:	2f2a 0114      	movel %a2@(276),%sp@-
    4c78:	486a 0196      	pea %a2@(406)
    4c7c:	206a fef4      	moveal %a2@(-268),%a0
    4c80:	4e90           	jsr %a0@
    4c82:	2f0a           	movel %a2,%sp@-
    4c84:	206a ffcc      	moveal %a2@(-52),%a0
    4c88:	4e90           	jsr %a0@
    4c8a:	4fef 000c      	lea %sp@(12),%sp
    4c8e:	2f0a           	movel %a2,%sp@-
    4c90:	206a ff84      	moveal %a2@(-124),%a0
    4c94:	4e90           	jsr %a0@
    4c96:	1003           	moveb %d3,%d0
    4c98:	4880           	extw %d0
    4c9a:	48c0           	extl %d0
    4c9c:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    4ca2:	4e5e           	unlk %fp
    4ca4:	4e75           	rts

00004ca6 <Exec_Signal>:
    4ca6:	4e56 0000      	linkw %fp,#0
    4caa:	48e7 2038      	moveml %d2/%a2-%a4,%sp@-
    4cae:	266e 0008      	moveal %fp@(8),%a3
    4cb2:	242e 000c      	movel %fp@(12),%d2
    4cb6:	246e 0010      	moveal %fp@(16),%a2
    4cba:	2f0a           	movel %a2,%sp@-
    4cbc:	206a ff8a      	moveal %a2@(-118),%a0
    4cc0:	4e90           	jsr %a0@
    4cc2:	202b 001a      	movel %a3@(26),%d0
    4cc6:	8082           	orl %d2,%d0
    4cc8:	2740 001a      	movel %d0,%a3@(26)
    4ccc:	c0ab 001e      	andl %a3@(30),%d0
    4cd0:	588f           	addql #4,%sp
    4cd2:	672e           	beqs 4d02 <Exec_Signal+0x5c>
    4cd4:	002b 0020 000e 	orib #32,%a3@(14)
    4cda:	0c2b 0002 000f 	cmpib #2,%a3@(15)
    4ce0:	6620           	bnes 4d02 <Exec_Signal+0x5c>
    4ce2:	4a2a 0127      	tstb %a2@(295)
    4ce6:	6c72           	bges 4d5a <Exec_Signal+0xb4>
    4ce8:	4a2a 0126      	tstb %a2@(294)
    4cec:	6e6c           	bgts 4d5a <Exec_Signal+0xb4>
    4cee:	206a 0114      	moveal %a2@(276),%a0
    4cf2:	117c 0003 000f 	moveb #3,%a0@(15)
    4cf8:	2f2a 0114      	movel %a2@(276),%sp@-
    4cfc:	486a 0196      	pea %a2@(406)
    4d00:	606c           	bras 4d6e <Exec_Signal+0xc8>
    4d02:	0c2b 0004 000f 	cmpib #4,%a3@(15)
    4d08:	6676           	bnes 4d80 <Exec_Signal+0xda>
    4d0a:	202b 0016      	movel %a3@(22),%d0
    4d0e:	80ab 001e      	orl %a3@(30),%d0
    4d12:	c0ab 001a      	andl %a3@(26),%d0
    4d16:	6768           	beqs 4d80 <Exec_Signal+0xda>
    4d18:	177c 0003 000f 	moveb #3,%a3@(15)
    4d1e:	2f0b           	movel %a3,%sp@-
    4d20:	206a ff06      	moveal %a2@(-250),%a0
    4d24:	4e90           	jsr %a0@
    4d26:	2f0b           	movel %a3,%sp@-
    4d28:	49ea 0196      	lea %a2@(406),%a4
    4d2c:	2f0c           	movel %a4,%sp@-
    4d2e:	206a fef4      	moveal %a2@(-268),%a0
    4d32:	4e90           	jsr %a0@
    4d34:	206a 0114      	moveal %a2@(276),%a0
    4d38:	4fef 000c      	lea %sp@(12),%sp
    4d3c:	122b 000d      	moveb %a3@(13),%d1
    4d40:	b228 000d      	cmpb %a0@(13),%d1
    4d44:	6f3a           	bles 4d80 <Exec_Signal+0xda>
    4d46:	0c28 0002 000f 	cmpib #2,%a0@(15)
    4d4c:	6632           	bnes 4d80 <Exec_Signal+0xda>
    4d4e:	4a2a 0127      	tstb %a2@(295)
    4d52:	6c06           	bges 4d5a <Exec_Signal+0xb4>
    4d54:	4a2a 0126      	tstb %a2@(294)
    4d58:	6f08           	bles 4d62 <Exec_Signal+0xbc>
    4d5a:	006a 0080 012a 	oriw #128,%a2@(298)
    4d60:	601e           	bras 4d80 <Exec_Signal+0xda>
    4d62:	117c 0003 000f 	moveb #3,%a0@(15)
    4d68:	2f2a 0114      	movel %a2@(276),%sp@-
    4d6c:	2f0c           	movel %a4,%sp@-
    4d6e:	206a fef4      	moveal %a2@(-268),%a0
    4d72:	4e90           	jsr %a0@
    4d74:	2f0a           	movel %a2,%sp@-
    4d76:	206a ffcc      	moveal %a2@(-52),%a0
    4d7a:	4e90           	jsr %a0@
    4d7c:	4fef 000c      	lea %sp@(12),%sp
    4d80:	2f0a           	movel %a2,%sp@-
    4d82:	206a ff84      	moveal %a2@(-124),%a0
    4d86:	4e90           	jsr %a0@
    4d88:	4cee 1c04 fff0 	moveml %fp@(-16),%d2/%a2-%a4
    4d8e:	4e5e           	unlk %fp
    4d90:	4e75           	rts

00004d92 <Exec_SumKickData>:
    4d92:	4e56 0000      	linkw %fp,#0
    4d96:	4879 0000 0000 	pea 0 <main>
    4d9c:	4879 0000 0000 	pea 0 <main>
    4da2:	61ff 0000 0000 	bsrl 4da4 <Exec_SumKickData+0x12>
    4da8:	7000           	moveq #0,%d0
    4daa:	4e5e           	unlk %fp
    4dac:	4e75           	rts

00004dae <Exec_SumLibrary>:
    4dae:	4e56 0000      	linkw %fp,#0
    4db2:	48e7 3830      	moveml %d2-%d4/%a2-%a3,%sp@-
    4db6:	246e 0008      	moveal %fp@(8),%a2
    4dba:	266e 000c      	moveal %fp@(12),%a3
    4dbe:	2f0b           	movel %a3,%sp@-
    4dc0:	206b ff7e      	moveal %a3@(-130),%a0
    4dc4:	4e90           	jsr %a0@
    4dc6:	102a 000e      	moveb %a2@(14),%d0
    4dca:	0200 0005      	andib #5,%d0
    4dce:	588f           	addql #4,%sp
    4dd0:	0c00 0004      	cmpib #4,%d0
    4dd4:	6670           	bnes 4e46 <Exec_SumLibrary+0x98>
    4dd6:	7600           	moveq #0,%d3
    4dd8:	182a 000e      	moveb %a2@(14),%d4
    4ddc:	1004           	moveb %d4,%d0
    4dde:	0000 0001      	orib #1,%d0
    4de2:	0200 00fd      	andib #-3,%d0
    4de6:	1540 000e      	moveb %d0,%a2@(14)
    4dea:	2f0b           	movel %a3,%sp@-
    4dec:	206b ff78      	moveal %a3@(-136),%a0
    4df0:	4e90           	jsr %a0@
    4df2:	7400           	moveq #0,%d2
    4df4:	362a 0010      	movew %a2@(16),%d3
    4df8:	41f2 3800      	lea %a2@(00000000,%d3:l),%a0
    4dfc:	588f           	addql #4,%sp
    4dfe:	b5c8           	cmpal %a0,%a2
    4e00:	6306           	blss 4e08 <Exec_SumLibrary+0x5a>
    4e02:	d498           	addl %a0@+,%d2
    4e04:	b5c8           	cmpal %a0,%a2
    4e06:	62fa           	bhis 4e02 <Exec_SumLibrary+0x54>
    4e08:	2f0b           	movel %a3,%sp@-
    4e0a:	206b ff7e      	moveal %a3@(-130),%a0
    4e0e:	4e90           	jsr %a0@
    4e10:	102a 000e      	moveb %a2@(14),%d0
    4e14:	1200           	moveb %d0,%d1
    4e16:	0201 00fe      	andib #-2,%d1
    4e1a:	1541 000e      	moveb %d1,%a2@(14)
    4e1e:	588f           	addql #4,%sp
    4e20:	0800 0001      	btst #1,%d0
    4e24:	66b2           	bnes 4dd8 <Exec_SumLibrary+0x2a>
    4e26:	0804 0001      	btst #1,%d4
    4e2a:	6616           	bnes 4e42 <Exec_SumLibrary+0x94>
    4e2c:	b4aa 001c      	cmpl %a2@(28),%d2
    4e30:	6710           	beqs 4e42 <Exec_SumLibrary+0x94>
    4e32:	2f0b           	movel %a3,%sp@-
    4e34:	2f3c 8100 0003 	movel #-2130706429,%sp@-
    4e3a:	206b ff96      	moveal %a3@(-106),%a0
    4e3e:	4e90           	jsr %a0@
    4e40:	508f           	addql #8,%sp
    4e42:	2542 001c      	movel %d2,%a2@(28)
    4e46:	2f0b           	movel %a3,%sp@-
    4e48:	206b ff78      	moveal %a3@(-136),%a0
    4e4c:	4e90           	jsr %a0@
    4e4e:	4cee 0c1c ffec 	moveml %fp@(-20),%d2-%d4/%a2-%a3
    4e54:	4e5e           	unlk %fp
    4e56:	4e75           	rts

00004e58 <Exec_SuperState>:
    4e58:	4e56 0000      	linkw %fp,#0
    4e5c:	91c8           	subal %a0,%a0
    4e5e:	2008           	movel %a0,%d0
    4e60:	4e5e           	unlk %fp
    4e62:	4e75           	rts

00004e64 <Exec_TaggedOpenLibrary>:
    4e64:	4e56 0000      	linkw %fp,#0
    4e68:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    4e6c:	262e 0008      	movel %fp@(8),%d3
    4e70:	246e 000c      	moveal %fp@(12),%a2
    4e74:	6f54           	bles 4eca <Exec_TaggedOpenLibrary+0x66>
    4e76:	2f0a           	movel %a2,%sp@-
    4e78:	42a7           	clrl %sp@-
    4e7a:	2003           	movel %d3,%d0
    4e7c:	5380           	subql #1,%d0
    4e7e:	2400           	movel %d0,%d2
    4e80:	e58a           	lsll #2,%d2
    4e82:	47f9 0000 0000 	lea 0 <main>,%a3
    4e88:	2f33 2800      	movel %a3@(00000000,%d2:l),%sp@-
    4e8c:	206a fdda      	moveal %a2@(-550),%a0
    4e90:	4e90           	jsr %a0@
    4e92:	2008           	movel %a0,%d0
    4e94:	4fef 000c      	lea %sp@(12),%sp
    4e98:	6648           	bnes 4ee2 <Exec_TaggedOpenLibrary+0x7e>
    4e9a:	2f0a           	movel %a2,%sp@-
    4e9c:	2f33 2800      	movel %a3@(00000000,%d2:l),%sp@-
    4ea0:	206a ffa2      	moveal %a2@(-94),%a0
    4ea4:	4e90           	jsr %a0@
    4ea6:	2008           	movel %a0,%d0
    4ea8:	508f           	addql #8,%sp
    4eaa:	6722           	beqs 4ece <Exec_TaggedOpenLibrary+0x6a>
    4eac:	2f0a           	movel %a2,%sp@-
    4eae:	42a7           	clrl %sp@-
    4eb0:	2f00           	movel %d0,%sp@-
    4eb2:	206a ff9c      	moveal %a2@(-100),%a0
    4eb6:	4e90           	jsr %a0@
    4eb8:	2f0a           	movel %a2,%sp@-
    4eba:	42a7           	clrl %sp@-
    4ebc:	2f33 2800      	movel %a3@(00000000,%d2:l),%sp@-
    4ec0:	206a fdda      	moveal %a2@(-550),%a0
    4ec4:	4e90           	jsr %a0@
    4ec6:	2008           	movel %a0,%d0
    4ec8:	6618           	bnes 4ee2 <Exec_TaggedOpenLibrary+0x7e>
    4eca:	4a83           	tstl %d3
    4ecc:	6d04           	blts 4ed2 <Exec_TaggedOpenLibrary+0x6e>
    4ece:	91c8           	subal %a0,%a0
    4ed0:	6010           	bras 4ee2 <Exec_TaggedOpenLibrary+0x7e>
    4ed2:	2003           	movel %d3,%d0
    4ed4:	4680           	notl %d0
    4ed6:	e588           	lsll #2,%d0
    4ed8:	41f9 0000 0000 	lea 0 <main>,%a0
    4ede:	2070 0800      	moveal %a0@(00000000,%d0:l),%a0
    4ee2:	2008           	movel %a0,%d0
    4ee4:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    4eea:	4e5e           	unlk %fp
    4eec:	4e75           	rts

00004eee <Exec_TypeOfMem>:
    4eee:	4e56 0000      	linkw %fp,#0
    4ef2:	48e7 3020      	moveml %d2-%d3/%a2,%sp@-
    4ef6:	262e 0008      	movel %fp@(8),%d3
    4efa:	246e 000c      	moveal %fp@(12),%a2
    4efe:	7400           	moveq #0,%d2
    4f00:	2f0a           	movel %a2,%sp@-
    4f02:	206a ff7e      	moveal %a2@(-130),%a0
    4f06:	4e90           	jsr %a0@
    4f08:	206a 0142      	moveal %a2@(322),%a0
    4f0c:	588f           	addql #4,%sp
    4f0e:	4a90           	tstl %a0@
    4f10:	671a           	beqs 4f2c <Exec_TypeOfMem+0x3e>
    4f12:	b6a8 0014      	cmpl %a0@(20),%d3
    4f16:	650e           	bcss 4f26 <Exec_TypeOfMem+0x38>
    4f18:	b6a8 0018      	cmpl %a0@(24),%d3
    4f1c:	6408           	bccs 4f26 <Exec_TypeOfMem+0x38>
    4f1e:	4282           	clrl %d2
    4f20:	3428 000e      	movew %a0@(14),%d2
    4f24:	6006           	bras 4f2c <Exec_TypeOfMem+0x3e>
    4f26:	2050           	moveal %a0@,%a0
    4f28:	4a90           	tstl %a0@
    4f2a:	66e6           	bnes 4f12 <Exec_TypeOfMem+0x24>
    4f2c:	2f0a           	movel %a2,%sp@-
    4f2e:	206a ff78      	moveal %a2@(-136),%a0
    4f32:	4e90           	jsr %a0@
    4f34:	2002           	movel %d2,%d0
    4f36:	4cee 040c fff4 	moveml %fp@(-12),%d2-%d3/%a2
    4f3c:	4e5e           	unlk %fp
    4f3e:	4e75           	rts

00004f40 <Exec_UserState>:
    4f40:	4e56 0000      	linkw %fp,#0
    4f44:	4e5e           	unlk %fp
    4f46:	4e75           	rts

00004f48 <Exec_Vacate>:
    4f48:	4e56 0000      	linkw %fp,#0
    4f4c:	48e7 003c      	moveml %a2-%a5,%sp@-
    4f50:	286e 0008      	moveal %fp@(8),%a4
    4f54:	2a6e 000c      	moveal %fp@(12),%a5
    4f58:	266e 0010      	moveal %fp@(16),%a3
    4f5c:	2f0b           	movel %a3,%sp@-
    4f5e:	206b ff7e      	moveal %a3@(-130),%a0
    4f62:	4e90           	jsr %a0@
    4f64:	42ad 0014      	clrl %a5@(20)
    4f68:	246c 0010      	moveal %a4@(16),%a2
    4f6c:	588f           	addql #4,%sp
    4f6e:	4a92           	tstl %a2@
    4f70:	6720           	beqs 4f92 <Exec_Vacate+0x4a>
    4f72:	bbca           	cmpal %a2,%a5
    4f74:	6616           	bnes 4f8c <Exec_Vacate+0x44>
    4f76:	2f0a           	movel %a2,%sp@-
    4f78:	206b ff06      	moveal %a3@(-250),%a0
    4f7c:	4e90           	jsr %a0@
    4f7e:	536c 002c      	subqw #1,%a4@(44)
    4f82:	2f0b           	movel %a3,%sp@-
    4f84:	2f0a           	movel %a2,%sp@-
    4f86:	206b fe88      	moveal %a3@(-376),%a0
    4f8a:	600e           	bras 4f9a <Exec_Vacate+0x52>
    4f8c:	2452           	moveal %a2@,%a2
    4f8e:	4a92           	tstl %a2@
    4f90:	66e0           	bnes 4f72 <Exec_Vacate+0x2a>
    4f92:	2f0b           	movel %a3,%sp@-
    4f94:	2f0c           	movel %a4,%sp@-
    4f96:	206b fdc8      	moveal %a3@(-568),%a0
    4f9a:	4e90           	jsr %a0@
    4f9c:	2f0b           	movel %a3,%sp@-
    4f9e:	206b ff78      	moveal %a3@(-136),%a0
    4fa2:	4e90           	jsr %a0@
    4fa4:	4cee 3c00 fff0 	moveml %fp@(-16),%a2-%a5
    4faa:	4e5e           	unlk %fp
    4fac:	4e75           	rts

00004fae <Exec_Wait>:
    4fae:	4e56 0000      	linkw %fp,#0
    4fb2:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    4fb6:	262e 0008      	movel %fp@(8),%d3
    4fba:	266e 000c      	moveal %fp@(12),%a3
    4fbe:	2f0b           	movel %a3,%sp@-
    4fc0:	42a7           	clrl %sp@-
    4fc2:	206b fedc      	moveal %a3@(-292),%a0
    4fc6:	4e90           	jsr %a0@
    4fc8:	2448           	moveal %a0,%a2
    4fca:	2f0b           	movel %a3,%sp@-
    4fcc:	206b ff8a      	moveal %a3@(-118),%a0
    4fd0:	4e90           	jsr %a0@
    4fd2:	602a           	bras 4ffe <Exec_Wait+0x50>
    4fd4:	2543 0016      	movel %d3,%a2@(22)
    4fd8:	142b 0127      	moveb %a3@(295),%d2
    4fdc:	50eb 0127      	st %a3@(295)
    4fe0:	157c 0004 000f 	moveb #4,%a2@(15)
    4fe6:	2f0a           	movel %a2,%sp@-
    4fe8:	486b 01a4      	pea %a3@(420)
    4fec:	206b fef4      	moveal %a3@(-268),%a0
    4ff0:	4e90           	jsr %a0@
    4ff2:	2f0b           	movel %a3,%sp@-
    4ff4:	206b ffcc      	moveal %a3@(-52),%a0
    4ff8:	4e90           	jsr %a0@
    4ffa:	1742 0127      	moveb %d2,%a3@(295)
    4ffe:	4fef 000c      	lea %sp@(12),%sp
    5002:	202a 001a      	movel %a2@(26),%d0
    5006:	c083           	andl %d3,%d0
    5008:	67ca           	beqs 4fd4 <Exec_Wait+0x26>
    500a:	222a 001a      	movel %a2@(26),%d1
    500e:	2401           	movel %d1,%d2
    5010:	c483           	andl %d3,%d2
    5012:	2003           	movel %d3,%d0
    5014:	4680           	notl %d0
    5016:	c280           	andl %d0,%d1
    5018:	2541 001a      	movel %d1,%a2@(26)
    501c:	2f0b           	movel %a3,%sp@-
    501e:	206b ff84      	moveal %a3@(-124),%a0
    5022:	4e90           	jsr %a0@
    5024:	2002           	movel %d2,%d0
    5026:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    502c:	4e5e           	unlk %fp
    502e:	4e75           	rts

00005030 <Exec_WaitIO>:
    5030:	4e56 0000      	linkw %fp,#0
    5034:	48e7 3030      	moveml %d2-%d3/%a2-%a3,%sp@-
    5038:	246e 0008      	moveal %fp@(8),%a2
    503c:	266e 000c      	moveal %fp@(12),%a3
    5040:	082a 0000 001e 	btst #0,%a2@(30)
    5046:	662c           	bnes 5074 <Exec_WaitIO+0x44>
    5048:	7400           	moveq #0,%d2
    504a:	7601           	moveq #1,%d3
    504c:	0c2a 0005 000c 	cmpib #5,%a2@(12)
    5052:	6620           	bnes 5074 <Exec_WaitIO+0x44>
    5054:	2f0b           	movel %a3,%sp@-
    5056:	206a 000e      	moveal %a2@(14),%a0
    505a:	1428 000f      	moveb %a0@(15),%d2
    505e:	2003           	movel %d3,%d0
    5060:	e5a8           	lsll %d2,%d0
    5062:	2f00           	movel %d0,%sp@-
    5064:	206b fec4      	moveal %a3@(-316),%a0
    5068:	4e90           	jsr %a0@
    506a:	508f           	addql #8,%sp
    506c:	082a 0000 001e 	btst #0,%a2@(30)
    5072:	67d8           	beqs 504c <Exec_WaitIO+0x1c>
    5074:	0c2a 0007 000c 	cmpib #7,%a2@(12)
    507a:	6618           	bnes 5094 <Exec_WaitIO+0x64>
    507c:	2f0b           	movel %a3,%sp@-
    507e:	206b ff8a      	moveal %a3@(-118),%a0
    5082:	4e90           	jsr %a0@
    5084:	2f0a           	movel %a2,%sp@-
    5086:	206b ff06      	moveal %a3@(-250),%a0
    508a:	4e90           	jsr %a0@
    508c:	2f0b           	movel %a3,%sp@-
    508e:	206b ff84      	moveal %a3@(-124),%a0
    5092:	4e90           	jsr %a0@
    5094:	102a 001f      	moveb %a2@(31),%d0
    5098:	4880           	extw %d0
    509a:	48c0           	extl %d0
    509c:	4cee 0c0c fff0 	moveml %fp@(-16),%d2-%d3/%a2-%a3
    50a2:	4e5e           	unlk %fp
    50a4:	4e75           	rts

000050a6 <Exec_WaitPort>:
    50a6:	4e56 0000      	linkw %fp,#0
    50aa:	48e7 3830      	moveml %d2-%d4/%a2-%a3,%sp@-
    50ae:	246e 0008      	moveal %fp@(8),%a2
    50b2:	266e 000c      	moveal %fp@(12),%a3
    50b6:	41ea 0014      	lea %a2@(20),%a0
    50ba:	202a 001c      	movel %a2@(28),%d0
    50be:	b1c0           	cmpal %d0,%a0
    50c0:	6620           	bnes 50e2 <Exec_WaitPort+0x3c>
    50c2:	7400           	moveq #0,%d2
    50c4:	7801           	moveq #1,%d4
    50c6:	2600           	movel %d0,%d3
    50c8:	2f0b           	movel %a3,%sp@-
    50ca:	142a 000f      	moveb %a2@(15),%d2
    50ce:	2004           	movel %d4,%d0
    50d0:	e5a8           	lsll %d2,%d0
    50d2:	2f00           	movel %d0,%sp@-
    50d4:	206b fec4      	moveal %a3@(-316),%a0
    50d8:	4e90           	jsr %a0@
    50da:	508f           	addql #8,%sp
    50dc:	b6aa 001c      	cmpl %a2@(28),%d3
    50e0:	67e6           	beqs 50c8 <Exec_WaitPort+0x22>
    50e2:	206a 0014      	moveal %a2@(20),%a0
    50e6:	2008           	movel %a0,%d0
    50e8:	4cee 0c1c ffec 	moveml %fp@(-20),%d2-%d4/%a2-%a3
    50ee:	4e5e           	unlk %fp
    50f0:	4e75           	rts
    50f2:	4e75           	rts

000050f4 <Exec_AddIntServer>:
    50f4:	4e56 0000      	linkw %fp,#0
    50f8:	48e7 3020      	moveml %d2-%d3/%a2,%sp@-
    50fc:	262e 0008      	movel %fp@(8),%d3
    5100:	242e 000c      	movel %fp@(12),%d2
    5104:	246e 0010      	moveal %fp@(16),%a2
    5108:	2f0a           	movel %a2,%sp@-
    510a:	206a ff8a      	moveal %a2@(-118),%a0
    510e:	4e90           	jsr %a0@
    5110:	2f02           	movel %d2,%sp@-
    5112:	2003           	movel %d3,%d0
    5114:	d080           	addl %d0,%d0
    5116:	d083           	addl %d3,%d0
    5118:	e588           	lsll #2,%d0
    511a:	2f32 0854      	movel %a2@(00000054,%d0:l),%sp@-
    511e:	206a fef4      	moveal %a2@(-268),%a0
    5122:	4e90           	jsr %a0@
    5124:	2f0a           	movel %a2,%sp@-
    5126:	206a ff84      	moveal %a2@(-124),%a0
    512a:	4e90           	jsr %a0@
    512c:	4cee 040c fff4 	moveml %fp@(-12),%d2-%d3/%a2
    5132:	4e5e           	unlk %fp
    5134:	4e75           	rts
    5136:	4e75           	rts

00005138 <Exec_ColdReboot>:
    5138:	4eae ff88      	jsr %fp@(-120)
    513c:	7000           	moveq #0,%d0
    513e:	72ff           	moveq #-1,%d1
    5140:	4eae fd78      	jsr %fp@(-648)
    5144:	4bfa 0006      	lea %pc@(514c <resetus>),%a5
    5148:	4eae ffe2      	jsr %fp@(-30)

0000514c <resetus>:
    514c:	41f9 0100 0000 	lea 1000000 <__errno_location+0xffa56c>,%a0
    5152:	91e8 ffec      	subal %a0@(-20),%a0
    5156:	2068 0004      	moveal %a0@(4),%a0
    515a:	5548           	subqw #2,%a0
    515c:	4e70           	reset
    515e:	4ed0           	jmp %a0@

00005160 <Exec_Disable>:
    5160:	4e56 0000      	linkw %fp,#0
    5164:	206e 0008      	moveal %fp@(8),%a0
    5168:	1028 0126      	moveb %a0@(294),%d0
    516c:	5228 0126      	addqb #1,%a0@(294)
    5170:	4a00           	tstb %d0
    5172:	6c08           	bges 517c <Exec_Disable+0x1c>
    5174:	00b8 0040 0010 	oril #4194320,fffff304 <__errno_location+0xffff9870>
    517a:	f304 
    517c:	4e5e           	unlk %fp
    517e:	4e75           	rts

00005180 <Exec_Enable>:
    5180:	4e56 0000      	linkw %fp,#0
    5184:	206e 0008      	moveal %fp@(8),%a0
    5188:	1028 0126      	moveb %a0@(294),%d0
    518c:	1200           	moveb %d0,%d1
    518e:	5301           	subqb #1,%d1
    5190:	1141 0126      	moveb %d1,%a0@(294)
    5194:	5300           	subqb #1,%d0
    5196:	6a1c           	bpls 51b4 <Exec_Enable+0x34>
    5198:	02b8 ffbf ffef 	andil #-4194321,fffff304 <__errno_location+0xffff9870>
    519e:	f304 
    51a0:	4a28 012b      	tstb %a0@(299)
    51a4:	6c0e           	bges 51b4 <Exec_Enable+0x34>
    51a6:	0268 ff7f 012a 	andiw #-129,%a0@(298)
    51ac:	2f08           	movel %a0,%sp@-
    51ae:	2068 ffcc      	moveal %a0@(-52),%a0
    51b2:	4e90           	jsr %a0@
    51b4:	4e5e           	unlk %fp
    51b6:	4e75           	rts

000051b8 <Exec_PrepareContext>:
    51b8:	4e56 0000      	linkw %fp,#0
    51bc:	48e7 0038      	moveml %a2-%a4,%sp@-
    51c0:	286e 0008      	moveal %fp@(8),%a4
    51c4:	226c 0036      	moveal %a4@(54),%a1
    51c8:	45e9 ffb6      	lea %a1@(-74),%a2
    51cc:	22ae 0010      	movel %fp@(16),%a1@
    51d0:	204a           	moveal %a2,%a0
    51d2:	47e9 fffb      	lea %a1@(-5),%a3
    51d6:	4218           	clrb %a0@+
    51d8:	b7c8           	cmpal %a0,%a3
    51da:	64fa           	bccs 51d6 <Exec_PrepareContext+0x1e>
    51dc:	2489           	movel %a1,%a2@
    51de:	256e 000c 0042 	movel %fp@(12),%a2@(66)
    51e4:	294a 0036      	movel %a2,%a4@(54)
    51e8:	7001           	moveq #1,%d0
    51ea:	4cdf 1c00      	moveml %sp@+,%a2-%a4
    51ee:	4e5e           	unlk %fp
    51f0:	4e75           	rts
    51f2:	4e75           	rts

000051f4 <Exec_RawPutChar>:
    51f4:	4e56 0000      	linkw %fp,#0
    51f8:	4e5e           	unlk %fp
    51fa:	4e75           	rts

000051fc <Exec_StackSwap>:
    51fc:	221f           	movel %sp@+,%d1
    51fe:	2057           	moveal %sp@,%a0
    5200:	2c78 0004      	moveal 4 <main+0x4>,%fp
    5204:	6100 0000      	bsrw 5206 <Exec_StackSwap+0xa>
    5208:	2276 0800      	moveal %fp@(00000000,%d0:l),%a1
    520c:	6100 0000      	bsrw 520e <Exec_StackSwap+0x12>
    5210:	43f1 0800      	lea %a1@(00000000,%d0:l),%a1
    5214:	2001           	movel %d1,%d0
    5216:	4eae ff88      	jsr %fp@(-120)
    521a:	2211           	movel %a1@,%d1
    521c:	22d0           	movel %a0@,%a1@+
    521e:	20c1           	movel %d1,%a0@+
    5220:	2211           	movel %a1@,%d1
    5222:	2290           	movel %a0@,%a1@
    5224:	20c1           	movel %d1,%a0@+
    5226:	220f           	movel %sp,%d1
    5228:	2e50           	moveal %a0@,%sp
    522a:	2081           	movel %d1,%a0@
    522c:	4eae ff82      	jsr %fp@(-126)
    5230:	2f00           	movel %d0,%sp@-
    5232:	4e75           	rts

00005234 <Exec_Supervisor>:
    5234:	4e75           	rts
    5236:	2878 0004      	moveal 4 <main+0x4>,%a4
    523a:	4eec ffe2      	jmp %a4@(-30)
    523e:	007c 0000      	oriw #0,%sr
    5242:	40e7           	movew %sr,%sp@-
    5244:	4ed5           	jmp %a5@

00005246 <_TrapLevel8>:
    5246:	660c           	bnes 5254 <pv>
    5248:	2f7c 0000 0000 	movel #0,%sp@(2)
    524e:	0002 
    5250:	4ed5           	jmp %a5@

00005252 <end>:
    5252:	4e75           	rts

00005254 <pv>:
    5254:	2f3c 0000 0008 	movel #8,%sp@-
    525a:	6000 0002      	braw 525e <_TrapEntry>

0000525e <_TrapEntry>:
    525e:	007c 0700      	oriw #1792,%sr
    5262:	594f           	subqw #4,%sp
    5264:	2f08           	movel %a0,%sp@-
    5266:	2078 0004      	moveal 4 <main+0x4>,%a0
    526a:	6100 0000      	bsrw 526c <_TrapEntry+0xe>
    526e:	2070 0800      	moveal %a0@(00000000,%d0:l),%a0
    5272:	6100 0000      	bsrw 5274 <_TrapEntry+0x16>
    5276:	2f70 0800 0004 	movel %a0@(00000000,%d0:l),%sp@(4)
    527c:	205f           	moveal %sp@+,%a0
    527e:	4e75           	rts

00005280 <Exec_Switch>:
    5280:	4e56 0000      	linkw %fp,#0
    5284:	2f0b           	movel %a3,%sp@-
    5286:	2f0a           	movel %a2,%sp@-
    5288:	246e 0008      	moveal %fp@(8),%a2
    528c:	266a 0114      	moveal %a2@(276),%a3
    5290:	2f0a           	movel %a2,%sp@-
    5292:	206a ff8a      	moveal %a2@(-118),%a0
    5296:	4e90           	jsr %a0@
    5298:	588f           	addql #4,%sp
    529a:	0c2b 0002 000f 	cmpib #2,%a3@(15)
    52a0:	6710           	beqs 52b2 <Exec_Switch+0x32>
    52a2:	082b 0005 000e 	btst #5,%a3@(14)
    52a8:	6608           	bnes 52b2 <Exec_Switch+0x32>
    52aa:	006a 8000 012a 	oriw #-32768,%a2@(298)
    52b0:	4e40           	trap #0
    52b2:	2f0a           	movel %a2,%sp@-
    52b4:	206a ff84      	moveal %a2@(-124),%a0
    52b8:	4e90           	jsr %a0@
    52ba:	246e fff8      	moveal %fp@(-8),%a2
    52be:	266e fffc      	moveal %fp@(-4),%a3
    52c2:	4e5e           	unlk %fp
    52c4:	4e75           	rts
    52c6:	4e75           	rts

000052c8 <get_offsetof_ThisTask>:
    52c8:	203c 0000 0114 	movel #276,%d0
    52ce:	4e75           	rts
    52d0:	4e71           	nop

000052d2 <get_offsetof_tc_SPLower>:
    52d2:	703a           	moveq #58,%d0
    52d4:	4e75           	rts
    52d6:	4e71           	nop

000052d8 <get_offsetof_tc_TrapCode>:
    52d8:	7032           	moveq #50,%d0
    52da:	4e75           	rts
    52dc:	4e71           	nop
    52de:	4e75           	rts

000052e0 <debugmem>:
    52e0:	4e56 0000      	linkw %fp,#0
    52e4:	4e5e           	unlk %fp
    52e6:	4e75           	rts

000052e8 <kprintf>:
    52e8:	4e56 0000      	linkw %fp,#0
    52ec:	486e 000c      	pea %fp@(12)
    52f0:	2f2e 0008      	movel %fp@(8),%sp@-
    52f4:	6104           	bsrs 52fa <vkprintf>
    52f6:	4e5e           	unlk %fp
    52f8:	4e75           	rts

000052fa <vkprintf>:
    52fa:	4e56 ffcc      	linkw %fp,#-52
    52fe:	48e7 3f3c      	moveml %d2-%d7/%a2-%a5,%sp@-
    5302:	2a6e 0008      	moveal %fp@(8),%a5
    5306:	282e 000c      	movel %fp@(12),%d4
    530a:	bafc 0000      	cmpaw #0,%a5
    530e:	6616           	bnes 5326 <vkprintf+0x2c>
    5310:	4878 0006      	pea 6 <main+0x6>
    5314:	4879 0000 0000 	pea 0 <main>
    531a:	61ff 0000 0000 	bsrl 531c <vkprintf+0x22>
    5320:	7006           	moveq #6,%d0
    5322:	6000 03f8      	braw 571c <vkprintf+0x422>
    5326:	7600           	moveq #0,%d3
    5328:	4a15           	tstb %a5@
    532a:	6700 03ee      	beqw 571a <vkprintf+0x420>
    532e:	2a03           	movel %d3,%d5
    5330:	43ee ffe0      	lea %fp@(-32),%a1
    5334:	2d49 ffd4      	movel %a1,%fp@(-44)
    5338:	1015           	moveb %a5@,%d0
    533a:	0c00 0025      	cmpib #37,%d0
    533e:	6600 03ba      	bnew 56fa <vkprintf+0x400>
    5342:	99cc           	subal %a4,%a4
    5344:	528d           	addql #1,%a5
    5346:	0c15 0030      	cmpib #48,%a5@
    534a:	660a           	bnes 5356 <vkprintf+0x5c>
    534c:	2c3c 0000 0000 	movel #0,%d6
    5352:	528d           	addql #1,%a5
    5354:	6006           	bras 535c <vkprintf+0x62>
    5356:	2c3c 0000 0000 	movel #0,%d6
    535c:	1015           	moveb %a5@,%d0
    535e:	0600 ffd0      	addib #-48,%d0
    5362:	0c00 0009      	cmpib #9,%d0
    5366:	6218           	bhis 5380 <vkprintf+0x86>
    5368:	2f0d           	movel %a5,%sp@-
    536a:	61ff 0000 0000 	bsrl 536c <vkprintf+0x72>
    5370:	2840           	moveal %d0,%a4
    5372:	588f           	addql #4,%sp
    5374:	1015           	moveb %a5@,%d0
    5376:	0600 ffd0      	addib #-48,%d0
    537a:	0c00 0009      	cmpib #9,%d0
    537e:	6314           	blss 5394 <vkprintf+0x9a>
    5380:	1015           	moveb %a5@,%d0
    5382:	0c00 002e      	cmpib #46,%d0
    5386:	670c           	beqs 5394 <vkprintf+0x9a>
    5388:	0c00 002d      	cmpib #45,%d0
    538c:	6706           	beqs 5394 <vkprintf+0x9a>
    538e:	0c00 002b      	cmpib #43,%d0
    5392:	6622           	bnes 53b6 <vkprintf+0xbc>
    5394:	528d           	addql #1,%a5
    5396:	1215           	moveb %a5@,%d1
    5398:	1001           	moveb %d1,%d0
    539a:	0600 ffd0      	addib #-48,%d0
    539e:	0c00 0009      	cmpib #9,%d0
    53a2:	63f0           	blss 5394 <vkprintf+0x9a>
    53a4:	0c01 002e      	cmpib #46,%d1
    53a8:	67ea           	beqs 5394 <vkprintf+0x9a>
    53aa:	0c01 002d      	cmpib #45,%d1
    53ae:	67e4           	beqs 5394 <vkprintf+0x9a>
    53b0:	0c01 002b      	cmpib #43,%d1
    53b4:	67de           	beqs 5394 <vkprintf+0x9a>
    53b6:	1a15           	moveb %a5@,%d5
    53b8:	7e6c           	moveq #108,%d7
    53ba:	be85           	cmpl %d5,%d7
    53bc:	6700 017e      	beqw 553c <vkprintf+0x242>
    53c0:	6d1e           	blts 53e0 <vkprintf+0xe6>
    53c2:	7e53           	moveq #83,%d7
    53c4:	be85           	cmpl %d5,%d7
    53c6:	6738           	beqs 5400 <vkprintf+0x106>
    53c8:	6d0a           	blts 53d4 <vkprintf+0xda>
    53ca:	7e25           	moveq #37,%d7
    53cc:	be85           	cmpl %d5,%d7
    53ce:	6720           	beqs 53f0 <vkprintf+0xf6>
    53d0:	6000 0188      	braw 555a <vkprintf+0x260>
    53d4:	7e63           	moveq #99,%d7
    53d6:	be85           	cmpl %d5,%d7
    53d8:	6700 00ba      	beqw 5494 <vkprintf+0x19a>
    53dc:	6000 017c      	braw 555a <vkprintf+0x260>
    53e0:	7e70           	moveq #112,%d7
    53e2:	be85           	cmpl %d5,%d7
    53e4:	677a           	beqs 5460 <vkprintf+0x166>
    53e6:	7e73           	moveq #115,%d7
    53e8:	be85           	cmpl %d5,%d7
    53ea:	6714           	beqs 5400 <vkprintf+0x106>
    53ec:	6000 016c      	braw 555a <vkprintf+0x260>
    53f0:	2079 0000 0000 	moveal 0 <main>,%a0
    53f6:	2f08           	movel %a0,%sp@-
    53f8:	4878 0025      	pea 25 <pause+0x5>
    53fc:	6000 030a      	braw 5708 <vkprintf+0x40e>
    5400:	2244           	moveal %d4,%a1
    5402:	5884           	addql #4,%d4
    5404:	2451           	moveal %a1@,%a2
    5406:	b4fc 0000      	cmpaw #0,%a2
    540a:	6606           	bnes 5412 <vkprintf+0x118>
    540c:	45f9 0000 0000 	lea 0 <main>,%a2
    5412:	0c15 0053      	cmpib #83,%a5@
    5416:	6616           	bnes 542e <vkprintf+0x134>
    5418:	2079 0000 0000 	moveal 0 <main>,%a0
    541e:	2f08           	movel %a0,%sp@-
    5420:	4878 0022      	pea 22 <pause+0x2>
    5424:	2068 fdfe      	moveal %a0@(-514),%a0
    5428:	4e90           	jsr %a0@
    542a:	5283           	addql #1,%d3
    542c:	508f           	addql #8,%sp
    542e:	2f0a           	movel %a2,%sp@-
    5430:	61ff 0000 0000 	bsrl 5432 <vkprintf+0x138>
    5436:	2400           	movel %d0,%d2
    5438:	2f02           	movel %d2,%sp@-
    543a:	2f0a           	movel %a2,%sp@-
    543c:	61ff 0000 0000 	bsrl 543e <vkprintf+0x144>
    5442:	d682           	addl %d2,%d3
    5444:	4fef 000c      	lea %sp@(12),%sp
    5448:	0c15 0053      	cmpib #83,%a5@
    544c:	6600 02c4      	bnew 5712 <vkprintf+0x418>
    5450:	2079 0000 0000 	moveal 0 <main>,%a0
    5456:	2f08           	movel %a0,%sp@-
    5458:	4878 0022      	pea 22 <pause+0x2>
    545c:	6000 02aa      	braw 5708 <vkprintf+0x40e>
    5460:	2644           	moveal %d4,%a3
    5462:	5884           	addql #4,%d4
    5464:	2213           	movel %a3@,%d1
    5466:	45ee ffd8      	lea %fp@(-40),%a2
    546a:	41ee ffe0      	lea %fp@(-32),%a0
    546e:	700f           	moveq #15,%d0
    5470:	c081           	andl %d1,%d0
    5472:	43f9 0000 0000 	lea 0 <main>,%a1
    5478:	1131 0800      	moveb %a1@(00000000,%d0:l),%a0@-
    547c:	e889           	lsrl #4,%d1
    547e:	b5c8           	cmpal %a0,%a2
    5480:	66ec           	bnes 546e <vkprintf+0x174>
    5482:	4878 0008      	pea 8 <main+0x8>
    5486:	486e ffd8      	pea %fp@(-40)
    548a:	61ff 0000 0000 	bsrl 548c <vkprintf+0x192>
    5490:	6000 027e      	braw 5710 <vkprintf+0x416>
    5494:	5884           	addql #4,%d4
    5496:	2644           	moveal %d4,%a3
    5498:	142b ffff      	moveb %a3@(-1),%d2
    549c:	1002           	moveb %d2,%d0
    549e:	0600 ffe0      	addib #-32,%d0
    54a2:	0c00 0060      	cmpib #96,%d0
    54a6:	6306           	blss 54ae <vkprintf+0x1b4>
    54a8:	0c02 009f      	cmpib #-97,%d2
    54ac:	6318           	blss 54c6 <vkprintf+0x1cc>
    54ae:	2079 0000 0000 	moveal 0 <main>,%a0
    54b4:	2f08           	movel %a0,%sp@-
    54b6:	42a7           	clrl %sp@-
    54b8:	1f42 0003      	moveb %d2,%sp@(3)
    54bc:	2068 fdfe      	moveal %a0@(-514),%a0
    54c0:	4e90           	jsr %a0@
    54c2:	6000 024c      	braw 5710 <vkprintf+0x416>
    54c6:	4878 0004      	pea 4 <main+0x4>
    54ca:	4879 0000 0000 	pea 0 <main>
    54d0:	61ff 0000 0000 	bsrl 54d2 <vkprintf+0x1d8>
    54d6:	2079 0000 0000 	moveal 0 <main>,%a0
    54dc:	2f08           	movel %a0,%sp@-
    54de:	1002           	moveb %d2,%d0
    54e0:	e808           	lsrb #4,%d0
    54e2:	7e0f           	moveq #15,%d7
    54e4:	c087           	andl %d7,%d0
    54e6:	43f9 0000 0000 	lea 0 <main>,%a1
    54ec:	1031 0800      	moveb %a1@(00000000,%d0:l),%d0
    54f0:	0280 0000 00ff 	andil #255,%d0
    54f6:	2f00           	movel %d0,%sp@-
    54f8:	2068 fdfe      	moveal %a0@(-514),%a0
    54fc:	4e90           	jsr %a0@
    54fe:	2079 0000 0000 	moveal 0 <main>,%a0
    5504:	2f08           	movel %a0,%sp@-
    5506:	2007           	movel %d7,%d0
    5508:	c082           	andl %d2,%d0
    550a:	47f9 0000 0000 	lea 0 <main>,%a3
    5510:	1033 0800      	moveb %a3@(00000000,%d0:l),%d0
    5514:	0280 0000 00ff 	andil #255,%d0
    551a:	2f00           	movel %d0,%sp@-
    551c:	2068 fdfe      	moveal %a0@(-514),%a0
    5520:	4e90           	jsr %a0@
    5522:	2079 0000 0000 	moveal 0 <main>,%a0
    5528:	2f08           	movel %a0,%sp@-
    552a:	4878 0027      	pea 27 <pause+0x7>
    552e:	2068 fdfe      	moveal %a0@(-514),%a0
    5532:	4e90           	jsr %a0@
    5534:	4fef 0020      	lea %sp@(32),%sp
    5538:	6000 01d8      	braw 5712 <vkprintf+0x418>
    553c:	102d 0001      	moveb %a5@(1),%d0
    5540:	0c00 0075      	cmpib #117,%d0
    5544:	6712           	beqs 5558 <vkprintf+0x25e>
    5546:	0c00 0064      	cmpib #100,%d0
    554a:	670c           	beqs 5558 <vkprintf+0x25e>
    554c:	0c00 0078      	cmpib #120,%d0
    5550:	6706           	beqs 5558 <vkprintf+0x25e>
    5552:	0c00 0058      	cmpib #88,%d0
    5556:	6602           	bnes 555a <vkprintf+0x260>
    5558:	528d           	addql #1,%a5
    555a:	0c15 0064      	cmpib #100,%a5@
    555e:	6700 0186      	beqw 56e6 <vkprintf+0x3ec>
    5562:	2644           	moveal %d4,%a3
    5564:	5884           	addql #4,%d4
    5566:	2413           	movel %a3@,%d2
    5568:	4a82           	tstl %d2
    556a:	6656           	bnes 55c2 <vkprintf+0x2c8>
    556c:	b8fc 0000      	cmpaw #0,%a4
    5570:	6604           	bnes 5576 <vkprintf+0x27c>
    5572:	387c 0001      	moveaw #1,%a4
    5576:	2246           	moveal %d6,%a1
    5578:	0c11 0020      	cmpib #32,%a1@
    557c:	661c           	bnes 559a <vkprintf+0x2a0>
    557e:	538c           	subql #1,%a4
    5580:	6018           	bras 559a <vkprintf+0x2a0>
    5582:	200c           	movel %a4,%d0
    5584:	7e08           	moveq #8,%d7
    5586:	be8c           	cmpl %a4,%d7
    5588:	6c02           	bges 558c <vkprintf+0x292>
    558a:	2007           	movel %d7,%d0
    558c:	2f00           	movel %d0,%sp@-
    558e:	2f06           	movel %d6,%sp@-
    5590:	61ff 0000 0000 	bsrl 5592 <vkprintf+0x298>
    5596:	518c           	subql #8,%a4
    5598:	508f           	addql #8,%sp
    559a:	b8fc 0000      	cmpaw #0,%a4
    559e:	6ee2           	bgts 5582 <vkprintf+0x288>
    55a0:	2246           	moveal %d6,%a1
    55a2:	0c11 0020      	cmpib #32,%a1@
    55a6:	6614           	bnes 55bc <vkprintf+0x2c2>
    55a8:	2079 0000 0000 	moveal 0 <main>,%a0
    55ae:	2f08           	movel %a0,%sp@-
    55b0:	4878 0030      	pea 30 <pause+0x10>
    55b4:	2068 fdfe      	moveal %a0@(-514),%a0
    55b8:	4e90           	jsr %a0@
    55ba:	508f           	addql #8,%sp
    55bc:	5283           	addql #1,%d3
    55be:	6000 0152      	braw 5712 <vkprintf+0x418>
    55c2:	347c 0020      	moveaw #32,%a2
    55c6:	1015           	moveb %a5@,%d0
    55c8:	0c00 0064      	cmpib #100,%d0
    55cc:	6708           	beqs 55d6 <vkprintf+0x2dc>
    55ce:	0c00 0075      	cmpib #117,%d0
    55d2:	6678           	bnes 564c <vkprintf+0x352>
    55d4:	6028           	bras 55fe <vkprintf+0x304>
    55d6:	4aae ffd0      	tstl %fp@(-48)
    55da:	6c1e           	bges 55fa <vkprintf+0x300>
    55dc:	2079 0000 0000 	moveal 0 <main>,%a0
    55e2:	2f08           	movel %a0,%sp@-
    55e4:	4878 002d      	pea 2d <pause+0xd>
    55e8:	2068 fdfe      	moveal %a0@(-514),%a0
    55ec:	4e90           	jsr %a0@
    55ee:	5283           	addql #1,%d3
    55f0:	242e ffd0      	movel %fp@(-48),%d2
    55f4:	4482           	negl %d2
    55f6:	508f           	addql #8,%sp
    55f8:	6004           	bras 55fe <vkprintf+0x304>
    55fa:	242e ffd0      	movel %fp@(-48),%d2
    55fe:	4a82           	tstl %d2
    5600:	6700 009c      	beqw 569e <vkprintf+0x3a4>
    5604:	266e ffd4      	moveal %fp@(-44),%a3
    5608:	d7ca           	addal %a2,%a3
    560a:	2d4b ffcc      	movel %a3,%fp@(-52)
    560e:	b4fc 0000      	cmpaw #0,%a2
    5612:	6700 008a      	beqw 569e <vkprintf+0x3a4>
    5616:	538a           	subql #1,%a2
    5618:	4878 000a      	pea a <main+0xa>
    561c:	2f02           	movel %d2,%sp@-
    561e:	61ff 0000 0000 	bsrl 5620 <vkprintf+0x326>
    5624:	508f           	addql #8,%sp
    5626:	53ae ffcc      	subql #1,%fp@(-52)
    562a:	226e ffcc      	moveal %fp@(-52),%a1
    562e:	47f9 0000 0000 	lea 0 <main>,%a3
    5634:	12b3 0800      	moveb %a3@(00000000,%d0:l),%a1@
    5638:	4878 000a      	pea a <main+0xa>
    563c:	2f02           	movel %d2,%sp@-
    563e:	61ff 0000 0000 	bsrl 5640 <vkprintf+0x346>
    5644:	508f           	addql #8,%sp
    5646:	2400           	movel %d0,%d2
    5648:	6754           	beqs 569e <vkprintf+0x3a4>
    564a:	60c2           	bras 560e <vkprintf+0x314>
    564c:	0c15 0078      	cmpib #120,%a5@
    5650:	6626           	bnes 5678 <vkprintf+0x37e>
    5652:	4a82           	tstl %d2
    5654:	6748           	beqs 569e <vkprintf+0x3a4>
    5656:	206e ffd4      	moveal %fp@(-44),%a0
    565a:	d1ca           	addal %a2,%a0
    565c:	b4fc 0000      	cmpaw #0,%a2
    5660:	673c           	beqs 569e <vkprintf+0x3a4>
    5662:	538a           	subql #1,%a2
    5664:	700f           	moveq #15,%d0
    5666:	c082           	andl %d2,%d0
    5668:	43f9 0000 0000 	lea 0 <main>,%a1
    566e:	1131 0800      	moveb %a1@(00000000,%d0:l),%a0@-
    5672:	e88a           	lsrl #4,%d2
    5674:	6728           	beqs 569e <vkprintf+0x3a4>
    5676:	60e4           	bras 565c <vkprintf+0x362>
    5678:	4a82           	tstl %d2
    567a:	6722           	beqs 569e <vkprintf+0x3a4>
    567c:	223c 0000 0000 	movel #0,%d1
    5682:	206e ffd4      	moveal %fp@(-44),%a0
    5686:	d1ca           	addal %a2,%a0
    5688:	b4fc 0000      	cmpaw #0,%a2
    568c:	6710           	beqs 569e <vkprintf+0x3a4>
    568e:	538a           	subql #1,%a2
    5690:	700f           	moveq #15,%d0
    5692:	c082           	andl %d2,%d0
    5694:	2641           	moveal %d1,%a3
    5696:	1133 0800      	moveb %a3@(00000000,%d0:l),%a0@-
    569a:	e88a           	lsrl #4,%d2
    569c:	66ea           	bnes 5688 <vkprintf+0x38e>
    569e:	49f2 c8e0      	lea %a2@(ffffffe0,%a4:l),%a4
    56a2:	b8fc 0000      	cmpaw #0,%a4
    56a6:	6f1e           	bles 56c6 <vkprintf+0x3cc>
    56a8:	200c           	movel %a4,%d0
    56aa:	7e08           	moveq #8,%d7
    56ac:	be8c           	cmpl %a4,%d7
    56ae:	6c02           	bges 56b2 <vkprintf+0x3b8>
    56b0:	2007           	movel %d7,%d0
    56b2:	2f00           	movel %d0,%sp@-
    56b4:	2f06           	movel %d6,%sp@-
    56b6:	61ff 0000 0000 	bsrl 56b8 <vkprintf+0x3be>
    56bc:	518c           	subql #8,%a4
    56be:	508f           	addql #8,%sp
    56c0:	b8fc 0000      	cmpaw #0,%a4
    56c4:	6ee2           	bgts 56a8 <vkprintf+0x3ae>
    56c6:	7e20           	moveq #32,%d7
    56c8:	9e8a           	subl %a2,%d7
    56ca:	2f07           	movel %d7,%sp@-
    56cc:	226e ffd4      	moveal %fp@(-44),%a1
    56d0:	4872 9800      	pea %a2@(00000000,%a1:l)
    56d4:	61ff 0000 0000 	bsrl 56d6 <vkprintf+0x3dc>
    56da:	2643           	moveal %d3,%a3
    56dc:	41eb 0020      	lea %a3@(32),%a0
    56e0:	2608           	movel %a0,%d3
    56e2:	968a           	subl %a2,%d3
    56e4:	602a           	bras 5710 <vkprintf+0x416>
    56e6:	2244           	moveal %d4,%a1
    56e8:	5884           	addql #4,%d4
    56ea:	2411           	movel %a1@,%d2
    56ec:	2d42 ffd0      	movel %d2,%fp@(-48)
    56f0:	6c00 fe76      	bgew 5568 <vkprintf+0x26e>
    56f4:	4482           	negl %d2
    56f6:	6000 fe70      	braw 5568 <vkprintf+0x26e>
    56fa:	2079 0000 0000 	moveal 0 <main>,%a0
    5700:	2f08           	movel %a0,%sp@-
    5702:	42a7           	clrl %sp@-
    5704:	1f40 0003      	moveb %d0,%sp@(3)
    5708:	2068 fdfe      	moveal %a0@(-514),%a0
    570c:	4e90           	jsr %a0@
    570e:	5283           	addql #1,%d3
    5710:	508f           	addql #8,%sp
    5712:	528d           	addql #1,%a5
    5714:	4a15           	tstb %a5@
    5716:	6600 fc20      	bnew 5338 <vkprintf+0x3e>
    571a:	2003           	movel %d3,%d0
    571c:	4cee 3cfc ffa4 	moveml %fp@(-92),%d2-%d7/%a2-%a5
    5722:	4e5e           	unlk %fp
    5724:	4e75           	rts
    5726:	4e75           	rts

00005728 <rkprintf>:
    5728:	4e56 0000      	linkw %fp,#0
    572c:	486e 0018      	pea %fp@(24)
    5730:	2f2e 0014      	movel %fp@(20),%sp@-
    5734:	61ff 0000 0000 	bsrl 5736 <rkprintf+0xe>
    573a:	4e5e           	unlk %fp
    573c:	4e75           	rts
    573e:	4e75           	rts

00005740 <RawPutChars>:
    5740:	4e56 0000      	linkw %fp,#0
    5744:	48e7 3020      	moveml %d2-%d3/%a2,%sp@-
    5748:	246e 0008      	moveal %fp@(8),%a2
    574c:	262e 000c      	movel %fp@(12),%d3
    5750:	4a12           	tstb %a2@
    5752:	6722           	beqs 5776 <RawPutChars+0x36>
    5754:	7400           	moveq #0,%d2
    5756:	5383           	subql #1,%d3
    5758:	70ff           	moveq #-1,%d0
    575a:	b083           	cmpl %d3,%d0
    575c:	6718           	beqs 5776 <RawPutChars+0x36>
    575e:	2079 0000 0000 	moveal 0 <main>,%a0
    5764:	2f08           	movel %a0,%sp@-
    5766:	141a           	moveb %a2@+,%d2
    5768:	2f02           	movel %d2,%sp@-
    576a:	2068 fdfe      	moveal %a0@(-514),%a0
    576e:	4e90           	jsr %a0@
    5770:	508f           	addql #8,%sp
    5772:	4a12           	tstb %a2@
    5774:	66e0           	bnes 5756 <RawPutChars+0x16>
    5776:	4cee 040c fff4 	moveml %fp@(-12),%d2-%d3/%a2
    577c:	4e5e           	unlk %fp
    577e:	4e75           	rts

00005780 <atoi>:
    5780:	4e56 0000      	linkw %fp,#0
    5784:	4878 000a      	pea a <main+0xa>
    5788:	42a7           	clrl %sp@-
    578a:	2f2e 0008      	movel %fp@(8),%sp@-
    578e:	61ff 0000 0000 	bsrl 5790 <atoi+0x10>
    5794:	4fef 000c      	lea %sp@(12),%sp
    5798:	2200           	movel %d0,%d1
    579a:	2001           	movel %d1,%d0
    579c:	6000 0002      	braw 57a0 <atoi+0x20>
    57a0:	4e5e           	unlk %fp
    57a2:	4e75           	rts

000057a4 <memset>:
    57a4:	4e56 fff4      	linkw %fp,#-12
    57a8:	2f02           	movel %d2,%sp@-
    57aa:	2d6e 0008 fffc 	movel %fp@(8),%fp@(-4)
    57b0:	7003           	moveq #3,%d0
    57b2:	c0ae fffc      	andl %fp@(-4),%d0
    57b6:	4a80           	tstl %d0
    57b8:	670a           	beqs 57c4 <memset+0x20>
    57ba:	4aae 0010      	tstl %fp@(16)
    57be:	6606           	bnes 57c6 <memset+0x22>
    57c0:	6000 0002      	braw 57c4 <memset+0x20>
    57c4:	6012           	bras 57d8 <memset+0x34>
    57c6:	206e fffc      	moveal %fp@(-4),%a0
    57ca:	10ae 000f      	moveb %fp@(15),%a0@
    57ce:	52ae fffc      	addql #1,%fp@(-4)
    57d2:	53ae 0010      	subql #1,%fp@(16)
    57d6:	60d8           	bras 57b0 <memset+0xc>
    57d8:	7404           	moveq #4,%d2
    57da:	b4ae 0010      	cmpl %fp@(16),%d2
    57de:	644e           	bccs 582e <memset+0x8a>
    57e0:	2d6e fffc fff8 	movel %fp@(-4),%fp@(-8)
    57e6:	7400           	moveq #0,%d2
    57e8:	4602           	notb %d2
    57ea:	c4ae 000c      	andl %fp@(12),%d2
    57ee:	2d42 fff4      	movel %d2,%fp@(-12)
    57f2:	222e fff4      	movel %fp@(-12),%d1
    57f6:	2001           	movel %d1,%d0
    57f8:	e188           	lsll #8,%d0
    57fa:	81ae fff4      	orl %d0,%fp@(-12)
    57fe:	222e fff4      	movel %fp@(-12),%d1
    5802:	2001           	movel %d1,%d0
    5804:	4840           	swap %d0
    5806:	4240           	clrw %d0
    5808:	81ae fff4      	orl %d0,%fp@(-12)
    580c:	7404           	moveq #4,%d2
    580e:	b4ae 0010      	cmpl %fp@(16),%d2
    5812:	6502           	bcss 5816 <memset+0x72>
    5814:	6012           	bras 5828 <memset+0x84>
    5816:	206e fff8      	moveal %fp@(-8),%a0
    581a:	20ae fff4      	movel %fp@(-12),%a0@
    581e:	58ae fff8      	addql #4,%fp@(-8)
    5822:	59ae 0010      	subql #4,%fp@(16)
    5826:	60e4           	bras 580c <memset+0x68>
    5828:	2d6e fff8 fffc 	movel %fp@(-8),%fp@(-4)
    582e:	4e71           	nop
    5830:	53ae 0010      	subql #1,%fp@(16)
    5834:	74ff           	moveq #-1,%d2
    5836:	b4ae 0010      	cmpl %fp@(16),%d2
    583a:	6602           	bnes 583e <memset+0x9a>
    583c:	600e           	bras 584c <memset+0xa8>
    583e:	206e fffc      	moveal %fp@(-4),%a0
    5842:	10ae 000f      	moveb %fp@(15),%a0@
    5846:	52ae fffc      	addql #1,%fp@(-4)
    584a:	60e4           	bras 5830 <memset+0x8c>
    584c:	202e 0008      	movel %fp@(8),%d0
    5850:	2040           	moveal %d0,%a0
    5852:	6000 0002      	braw 5856 <memset+0xb2>
    5856:	2008           	movel %a0,%d0
    5858:	241f           	movel %sp@+,%d2
    585a:	4e5e           	unlk %fp
    585c:	4e75           	rts
    585e:	4e75           	rts

00005860 <strcmp>:
    5860:	4e56 fffc      	linkw %fp,#-4
    5864:	4e71           	nop
    5866:	206e 0008      	moveal %fp@(8),%a0
    586a:	1010           	moveb %a0@,%d0
    586c:	49c0           	extbl %d0
    586e:	206e 000c      	moveal %fp@(12),%a0
    5872:	1210           	moveb %a0@,%d1
    5874:	49c1           	extbl %d1
    5876:	9081           	subl %d1,%d0
    5878:	2200           	movel %d0,%d1
    587a:	2d41 fffc      	movel %d1,%fp@(-4)
    587e:	4a81           	tstl %d1
    5880:	660c           	bnes 588e <strcmp+0x2e>
    5882:	206e 0008      	moveal %fp@(8),%a0
    5886:	4a10           	tstb %a0@
    5888:	6606           	bnes 5890 <strcmp+0x30>
    588a:	6000 0002      	braw 588e <strcmp+0x2e>
    588e:	600a           	bras 589a <strcmp+0x3a>
    5890:	52ae 0008      	addql #1,%fp@(8)
    5894:	52ae 000c      	addql #1,%fp@(12)
    5898:	60cc           	bras 5866 <strcmp+0x6>
    589a:	222e fffc      	movel %fp@(-4),%d1
    589e:	2001           	movel %d1,%d0
    58a0:	6000 0002      	braw 58a4 <strcmp+0x44>
    58a4:	4e5e           	unlk %fp
    58a6:	4e75           	rts

000058a8 <strlen>:
    58a8:	4e56 fffc      	linkw %fp,#-4
    58ac:	2d6e 0008 fffc 	movel %fp@(8),%fp@(-4)
    58b2:	206e 0008      	moveal %fp@(8),%a0
    58b6:	4a10           	tstb %a0@
    58b8:	6602           	bnes 58bc <strlen+0x14>
    58ba:	6006           	bras 58c2 <strlen+0x1a>
    58bc:	52ae 0008      	addql #1,%fp@(8)
    58c0:	60f0           	bras 58b2 <strlen+0xa>
    58c2:	222e 0008      	movel %fp@(8),%d1
    58c6:	92ae fffc      	subl %fp@(-4),%d1
    58ca:	2001           	movel %d1,%d0
    58cc:	6000 0002      	braw 58d0 <strlen+0x28>
    58d0:	4e5e           	unlk %fp
    58d2:	4e75           	rts

000058d4 <strtol>:
    58d4:	4e56 0000      	linkw %fp,#0
    58d8:	7000           	moveq #0,%d0
    58da:	4e5e           	unlk %fp
    58dc:	4e75           	rts
    58de:	4e75           	rts

000058e0 <__mulsi3>:
    58e0:	2042           	moveal %d2,%a0
    58e2:	2243           	moveal %d3,%a1
    58e4:	4caf 000f 0004 	movemw %sp@(4),%d0-%d3
    58ea:	3f00           	movew %d0,%sp@-
    58ec:	6a00 0006      	bplw 58f4 <L_00>
    58f0:	4441           	negw %d1
    58f2:	4040           	negxw %d0

000058f4 <L_00>:
    58f4:	4a42           	tstw %d2
    58f6:	6a00 0008      	bplw 5900 <L_01>
    58fa:	4443           	negw %d3
    58fc:	4042           	negxw %d2
    58fe:	4657           	notw %sp@

00005900 <L_01>:
    5900:	48c0           	extl %d0
    5902:	6700 0004      	beqw 5908 <L_1>
    5906:	c0c3           	muluw %d3,%d0

00005908 <L_1>:
    5908:	4a42           	tstw %d2
    590a:	6700 0006      	beqw 5912 <L_2>
    590e:	c4c1           	muluw %d1,%d2
    5910:	d042           	addw %d2,%d0

00005912 <L_2>:
    5912:	4840           	swap %d0
    5914:	4240           	clrw %d0
    5916:	c2c3           	muluw %d3,%d1
    5918:	d081           	addl %d1,%d0
    591a:	2609           	movel %a1,%d3
    591c:	2408           	movel %a0,%d2
    591e:	4a5f           	tstw %sp@+
    5920:	6a00 0004      	bplw 5926 <L_3>
    5924:	4480           	negl %d0

00005926 <L_3>:
    5926:	4e75           	rts

00005928 <__udivsi3>:
    5928:	2042           	moveal %d2,%a0
    592a:	2243           	moveal %d3,%a1
    592c:	4280           	clrl %d0
    592e:	242f 0008      	movel %sp@(8),%d2
    5932:	6700 004e      	beqw 5982 <L_9>
    5936:	222f 0004      	movel %sp@(4),%d1
    593a:	b282           	cmpl %d2,%d1
    593c:	6500 003e      	bcsw 597c <L_8>
    5940:	4a6f 0008      	tstw %sp@(8)
    5944:	6600 0020      	bnew 5966 <L_2>
    5948:	3601           	movew %d1,%d3
    594a:	4241           	clrw %d1
    594c:	4841           	swap %d1
    594e:	6700 0004      	beqw 5954 <L_00>
    5952:	82c2           	divuw %d2,%d1

00005954 <L_00>:
    5954:	3001           	movew %d1,%d0
    5956:	4840           	swap %d0
    5958:	3203           	movew %d3,%d1
    595a:	82c2           	divuw %d2,%d1
    595c:	3001           	movew %d1,%d0
    595e:	4241           	clrw %d1
    5960:	4841           	swap %d1
    5962:	6000 0018      	braw 597c <L_8>

00005966 <L_2>:
    5966:	761f           	moveq #31,%d3

00005968 <L_3>:
    5968:	d281           	addl %d1,%d1
    596a:	d180           	addxl %d0,%d0
    596c:	b082           	cmpl %d2,%d0
    596e:	6500 0006      	bcsw 5976 <L_01>
    5972:	9082           	subl %d2,%d0
    5974:	5241           	addqw #1,%d1

00005976 <L_01>:
    5976:	51cb fff0      	dbf %d3,5968 <L_3>
    597a:	c141           	exg %d0,%d1

0000597c <L_8>:
    597c:	2609           	movel %a1,%d3
    597e:	2408           	movel %a0,%d2
    5980:	4e75           	rts

00005982 <L_9>:
    5982:	82c2           	divuw %d2,%d1
    5984:	6000 fff6      	braw 597c <L_8>

00005988 <__umodsi3>:
    5988:	2f2f 0008      	movel %sp@(8),%sp@-
    598c:	2f2f 0008      	movel %sp@(8),%sp@-
    5990:	6196           	bsrs 5928 <__udivsi3>
    5992:	508f           	addql #8,%sp
    5994:	2001           	movel %d1,%d0
    5996:	4e75           	rts

00005998 <setjmp>:
    5998:	201f           	movel %sp@+,%d0
    599a:	221f           	movel %sp@+,%d1
    599c:	4878 0001      	pea 1 <main+0x1>
    59a0:	2f01           	movel %d1,%sp@-
    59a2:	2f00           	movel %d0,%sp@-
    59a4:	4ef9 0000 0000 	jmp 0 <main>
    59aa:	4e75           	rts

000059ac <__sigsetjmp>:
    59ac:	4e56 0000      	linkw %fp,#0
    59b0:	202e 000c      	movel %fp@(12),%d0
    59b4:	206e 0008      	moveal %fp@(8),%a0
    59b8:	48d0 00fe      	moveml %d1-%d7,%a0@
    59bc:	216e 0004 001c 	movel %fp@(4),%a0@(28)
    59c2:	48e8 3e00 0020 	moveml %a1-%a5,%a0@(32)
    59c8:	2156 0034      	movel %fp@,%a0@(52)
    59cc:	43ee 0008      	lea %fp@(8),%a1
    59d0:	2149 0038      	movel %a1,%a0@(56)
    59d4:	f228 f0ff 003c 	fmovemx %fp0-%fp7,%a0@(60)
    59da:	2f00           	movel %d0,%sp@-
    59dc:	2f08           	movel %a0,%sp@-
    59de:	61ff 0000 0000 	bsrl 59e0 <__sigsetjmp+0x34>
    59e4:	4e5e           	unlk %fp
    59e6:	4e75           	rts

000059e8 <__setjmp>:
    59e8:	4e56 0000      	linkw %fp,#0
    59ec:	206e 0008      	moveal %fp@(8),%a0
    59f0:	48d0 00fe      	moveml %d1-%d7,%a0@
    59f4:	216e 0004 001c 	movel %fp@(4),%a0@(28)
    59fa:	48e8 3e00 0020 	moveml %a1-%a5,%a0@(32)
    5a00:	2156 0034      	movel %fp@,%a0@(52)
    5a04:	43ee 0008      	lea %fp@(8),%a1
    5a08:	2149 0038      	movel %a1,%a0@(56)
    5a0c:	f228 f0ff 003c 	fmovemx %fp0-%fp7,%a0@(60)
    5a12:	4280           	clrl %d0
    5a14:	4e5e           	unlk %fp
    5a16:	4e75           	rts

00005a18 <__sigjmp_save>:
    5a18:	4e56 0000      	linkw %fp,#0
    5a1c:	2f0a           	movel %a2,%sp@-
    5a1e:	2f02           	movel %d2,%sp@-
    5a20:	246e 0008      	moveal %fp@(8),%a2
    5a24:	4282           	clrl %d2
    5a26:	4aae 000c      	tstl %fp@(12)
    5a2a:	6714           	beqs 5a40 <__sigjmp_save+0x28>
    5a2c:	486a 00a0      	pea %a2@(160)
    5a30:	42a7           	clrl %sp@-
    5a32:	42a7           	clrl %sp@-
    5a34:	61ff 0000 0000 	bsrl 5a36 <__sigjmp_save+0x1e>
    5a3a:	4a80           	tstl %d0
    5a3c:	6602           	bnes 5a40 <__sigjmp_save+0x28>
    5a3e:	7401           	moveq #1,%d2
    5a40:	2542 009c      	movel %d2,%a2@(156)
    5a44:	4280           	clrl %d0
    5a46:	242e fff8      	movel %fp@(-8),%d2
    5a4a:	246e fffc      	moveal %fp@(-4),%a2
    5a4e:	4e5e           	unlk %fp
    5a50:	4e75           	rts
    5a52:	4e75           	rts

00005a54 <__sigprocmask>:
    5a54:	707e           	moveq #126,%d0
    5a56:	2243           	moveal %d3,%a1
    5a58:	262f 000c      	movel %sp@(12),%d3
    5a5c:	2042           	moveal %d2,%a0
    5a5e:	242f 0008      	movel %sp@(8),%d2
    5a62:	222f 0004      	movel %sp@(4),%d1
    5a66:	4e40           	trap #0
    5a68:	2408           	movel %a0,%d2
    5a6a:	2609           	movel %a1,%d3
    5a6c:	0c80 ffff f001 	cmpil #-4095,%d0
    5a72:	64ff 0000 0000 	bccl 5a74 <__sigprocmask+0x20>
    5a78:	4e75           	rts
    5a7a:	4e75           	rts

00005a7c <__syscall_error>:
    5a7c:	4480           	negl %d0
    5a7e:	23c0 0000 0000 	movel %d0,0 <main>
    5a84:	2f00           	movel %d0,%sp@-
    5a86:	61ff 0000 0000 	bsrl 5a88 <__syscall_error+0xc>
    5a8c:	209f           	movel %sp@+,%a0@
    5a8e:	70ff           	moveq #-1,%d0
    5a90:	2040           	moveal %d0,%a0
    5a92:	4e75           	rts

00005a94 <__errno_location>:
    5a94:	4e56 0000      	linkw %fp,#0
    5a98:	41f9 0000 0000 	lea 0 <main>,%a0
    5a9e:	2008           	movel %a0,%d0
    5aa0:	4e5e           	unlk %fp
    5aa2:	4e75           	rts
