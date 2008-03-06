#! /usr/bin/awk -f
#
# Copyright (C) 2006  Free Software Foundation, Inc.
#
# This genmoddep.awk is free software; the author
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.

# Read defined symbols from stdin.
BEGIN {
  while (getline <"/dev/stdin") {
    symtab[$1] = $2
  }
}

# The first line contains a module name.
FNR == 1 {
  module = $1
  next
};

# The rest is undefined symbols.
{
  if ($1 in symtab) {
    modtab[module] = modtab[module] " " symtab[$1];
  }
  else {
    printf "%s in %s is not defined\n", $1, module >"/dev/stderr";
    error++;
    exit;
  }
}

# Output the result.
END {
  if (error == 1)
    exit 1;
  
  for (mod in modtab) {
    # Remove duplications.
    split(modtab[mod], depmods, " ");
    for (depmod in uniqmods) {
      delete uniqmods[depmod];
    }
    for (i in depmods) {
      depmod = depmods[i];
      # Ignore kernel, as always loaded.
      if (depmod != "kernel")
	uniqmods[depmod] = 1;
    }
    modlist = ""
    for (depmod in uniqmods) {
      modlist = modlist " " depmod;
    }
    printf "%s:%s\n", mod, modlist;
  }
}
