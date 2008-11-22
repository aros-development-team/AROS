#! /bin/sh

set -e

autoconf
autoheader
echo timestamp > stamp-h.in
for rmk in conf/*.rmk; do
  ruby genmk.rb < $rmk > `echo $rmk | sed 's/\.rmk$/.mk/'`
done
./gendistlist.sh > DISTLIST

exit 0
