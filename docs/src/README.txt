About the AROS-dict personal dictionary file:

This file contains a personal dictionary for AROS. This means that if you
use a spell checker to check an AROS related document for spelling
mistakes, you can use this file to check the words that are normally not
part of the English language, but are part of the AROS development
vocabulary. You use it alongside an English dictionary.

If you use AROS-dict, please keep the following in mind:

1. For maximum compatibility, please keep the list the way it is at the
moment of writing this: every line contains either whitespace or a
dictionary word.
Do not use comments, mark-up or anything else that your particular spell
checking program might allow. The list need not be sorted, though I am not
sure if every spell checker in use might agree. If you sort the list and
then commit your changes to the AROS dev repository, please keep to the
following sort order: [STILL TO DO]

2. Do not add anything but AROS specific words. Of course it can happen
that there are words in my main (English) dictionary that are not in
yours. There is no way of checking this, so you can add these words to the
AROS dictionary.

I myself use ispell for spell checking. It comes standard with Linux
(including an English dictionary). I believe it can also be found on
Aminet. [CHECK THIS]

Suppose I want to check file bbbb.src in aaaa, I would call ispell as
follows:

	ispell -p $TOP/docs/src/AROS-dict aaaa/bbbb.src

assuming $TOP will be expanded to my local AROS directory. If you
do not specify the exact location of the directory where the personal
dictionary is, ispell will assume it has to write to your home directory.
In other words, if you are spell checking a file in
~johndoe/AROS/docs/src/, and you issue:

	ispell -p AROS-dict example.src

ispell will not write to ~johndoe/AROS/docs/src/AROS-dict, but to
~johndoe/AROS-dict. (This is all AFAIK, it may differ under different
circumstances. It doesn't hurt to use the exact path, though.)



20-2-1999, Branko Collin
