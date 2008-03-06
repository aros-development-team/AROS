#! /bin/sh

set -e

autoconf
autoheader
for rmk in conf/*.rmk; do
  ruby genmk.rb < $rmk > `echo $rmk | sed 's/\.rmk$/.mk/'`
done

exit 0
