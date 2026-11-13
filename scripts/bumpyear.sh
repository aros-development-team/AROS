#!/bin/bash

# Update the year in various files which are supposed
# to print the current year.

START_YEAR=1995
NEW_YEAR=$(date +"%Y")
OLD_YEAR=$((NEW_YEAR-1))
SUB=s/$START_YEAR-$OLD_YEAR/$START_YEAR-$NEW_YEAR/g
echo "$SUB"

SOURCE_PATH=.
DOC_PATH=./documentation

sed -i "$SUB" $SOURCE_PATH/arch/m68k-amiga/boot/ext_entry.S
sed -i "$SUB" $SOURCE_PATH/arch/m68k-amiga/boot/rom_entry.S
sed -i "$SUB" $SOURCE_PATH/rom/dos/boot.c
sed -i "$SUB" $SOURCE_PATH/rom/exec/taggedopenlibrary.c

sed -i "$SUB" $DOC_PATH/targets/www/template/languages/cs.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/de.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/el.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/en.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/es.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/fi.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/fr.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/it.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/nl.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/pl.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/pt.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/ru.txt
sed -i "$SUB" $DOC_PATH/targets/www/template/languages/sv.txt

# warning: the following are within a submodule
sed -i "$SUB" $SOURCE_PATH/workbench/system/AboutAROS/catalogs/aboutaros.cd
for catalog in `find $SOURCE_PATH/workbench/system/AboutAROS/catalogs/ -name *.ct` ; do sed -i "$SUB" $catalog ;
done

START_YEAR=2002
SUB=s/$START_YEAR-$OLD_YEAR/$START_YEAR-$NEW_YEAR/g
echo "$SUB"

sed -i "$SUB" $SOURCE_PATH/workbench/libs/muimaster/catalogs/muimaster.cd
for catalog in `find $SOURCE_PATH/workbench/libs/muimaster/catalogs -name *.ct` ; do sed -i "$SUB" $catalog ;
done
