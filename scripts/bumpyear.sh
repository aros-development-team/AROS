#!/bin/bash

START_YEAR=1995
OLD_YEAR=2013
NEW_YEAR=2014
SUB=s/$START_YEAR-$OLD_YEAR/$START_YEAR-$NEW_YEAR/g
echo "$SUB"

sed -i "$SUB" AROS/rom/dos/boot.c
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/aboutaros.cd
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/croatian.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/czech.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/danish.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/dutch.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/finnish.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/french.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/german.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/italian.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/polish.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/portuguese.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/russian.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/spanish.ct
sed -i "$SUB" AROS/workbench/system/AboutAROS/catalogs/swedish.ct

sed -i "$SUB" Documentation/targets/www/template/languages/cs
sed -i "$SUB" Documentation/targets/www/template/languages/de
sed -i "$SUB" Documentation/targets/www/template/languages/el
sed -i "$SUB" Documentation/targets/www/template/languages/en
sed -i "$SUB" Documentation/targets/www/template/languages/es
sed -i "$SUB" Documentation/targets/www/template/languages/fi
sed -i "$SUB" Documentation/targets/www/template/languages/fr
sed -i "$SUB" Documentation/targets/www/template/languages/it
sed -i "$SUB" Documentation/targets/www/template/languages/nl
sed -i "$SUB" Documentation/targets/www/template/languages/pl
sed -i "$SUB" Documentation/targets/www/template/languages/pt
sed -i "$SUB" Documentation/targets/www/template/languages/ru
sed -i "$SUB" Documentation/targets/www/template/languages/sv
