version 5.0
set nocompatible
let cpo_save=&cpo
set cpo=B
map! <xHome> <Home>
map! <xEnd> <End>
map! <S-xF4> <S-F4>
map! <S-xF3> <S-F3>
map! <S-xF2> <S-F2>
map! <S-xF1> <S-F1>
map! <xF4> <F4>
map! <xF3> <F3>
map! <xF2> <F2>
map! <xF1> <F1>
map <xHome> <Home>
map <xEnd> <End>
map <S-xF4> <S-F4>
map <S-xF3> <S-F3>
map <S-xF2> <S-F2>
map <S-xF1> <S-F1>
map <xF4> <F4>
map <xF3> <F3>
map <xF2> <F2>
map <xF1> <F1>
let &cpo=cpo_save
unlet cpo_save
set backspace=2
set filetype=c
set mouse=a
set syntax=c
set tabstop=3
let v:this_session=expand("<sfile>:p")
1,9999bd
execute "cd " . expand("<sfile>:p:h")
let shmsave = &shortmess | set shortmess=aoO
e +285 filehandles2.c
badd +20 bitmap.c
badd +17 blockaccess.c
badd +11 checksums.c
badd +10 error.c
badd +19 extstrings.c
badd +117 filehandles1.c
badd +33 filehandles3.c
badd +9 hashing.c
badd +353 main.c
badd +35 misc.c
badd +21 volumes.c
badd +76 afsblocks.h
badd +1 afshandler.h
badd +4 bitmap.h
badd +19 blockaccess.h
badd +10 checksums.h
badd +15 error.h
badd +11 extstrings.h
badd +4 filehandles1.h
badd +9 filehandles2.h
badd +8 filehandles3.h
badd +4 filehandles.h
badd +8 hashing.h
badd +6 misc.h
badd +10 volumes.h
badd +1 mmakefile.src
let sbsave = &splitbelow | set splitbelow
b misc.h
sb filehandles2.c
let &splitbelow = sbsave
if (&lines == 51)
  normal t
  resize 24
  normal j
  resize 24
else
  normal =
endif
normal t1Gzt8G0
normal j271Gzt285G030l
normal 2w
let &shortmess = shmsave
let sessionextra=expand("<sfile>:p:r")."x.vim"
if file_readable(sessionextra)
	execute "source " . sessionextra
endif
