#!/bin/sh
# floppy test

PATH=.:$PATH

DUMPS=../Dumps
FFSDUMP=$DUMPS/testffs.adf
OFSDUMP=$DUMPS/testofs.adf
HDDUMP=$DUMPS/testhd.adf

BOOTDIR=../Boot
BOOTBLK=$BOOTDIR/stdboot3.bbk

CHECK=../Check


bootdisk $BOOTBLK
rm newdev
echo "-----"

cp $FFSDUMP testffs_adf
del_test testffs_adf
rm testffs_adf
echo "-----"

dir_test $FFSDUMP
echo "-----"

cp $FFSDUMP testffs_adf
dir_test2 testffs_adf
rm testffs_adf
echo "-----"

fl_test $FFSDUMP $BOOTBLK
rm newdev
echo "-----"

fl_test2 $HDDUMP
echo "-----"

file_test $FFSDUMP $OFSDUMP
diff mod.distant $CHECK/mod.And.DistantCall
diff moon_gif $CHECK/MOON.GIF
rm mod.distant moon_gif
echo "-----"

cp $FFSDUMP testffs_adf
file_test2 testffs_adf $CHECK/MOON.GIF
diff moon__gif $CHECK/MOON.GIF
rm moon__gif
rm testffs_adf
echo "-----"

cp $OFSDUMP testofs_adf
file_test3 testofs_adf $CHECK/MOON.GIF
diff moon__gif $CHECK/MOON.GIF
rm moon__gif
rm testofs_adf
echo "-----"

rename
rm newdev
echo "-----"

rename2
rm newdev

undel
rm newdev
echo "-----"

cp $FFSDUMP testffs_adf
undel2 testffs_adf
diff mod.distant $CHECK/mod.And.DistantCall
rm mod.distant testffs_adf
echo "-----"

cp $OFSDUMP testofs_adf
undel3 testofs_adf
diff moon_gif $CHECK/MOON.GIF
rm moon_gif testofs_adf
echo "-----"
