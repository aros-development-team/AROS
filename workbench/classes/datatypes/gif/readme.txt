gif.datatype distribution doesn't contain LZW algorithms. It instead loads GIF image data
without decompression and shows funny pictures. :-)

Because Unisys (and maybe other companies) have a patent on the LZW algorithm in USA and
some other countries, including LZW code is problematic.

As the Unisys patent ages out in mid 2003, gif.datatype can get complete then.

If you are in a country, where there is no patent on LZW and there are no other legal
reasons not to use LZW, you can get the LZW code for gif.datatype from:

http://pl.attitu.de/martin/codec.c

Just replace the the original codec.c and rebuild
