#!/bin/sh
PATH=.:$PATH

#floppy test

DUMPS=../Dumps
FFSDUMP=$DUMPS/testffs.adf
OFSDUMP=$DUMPS/testofs.adf
HDDUMP=$DUMPS/testhd.adf

BOOTDIR=../Boot
BOOTBLK=$BOOTDIR/stdboot3.bbk

CHECK=../Check

hd_test /home/root/hd.adf /home/root/idh2.adf
echo "-----"

hd_test2
rm newdev
echo "-----"

hd_test3
rm newdev
echo "-----"

hardfile /home/root/hardfile.hdf
