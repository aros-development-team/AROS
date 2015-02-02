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

# Read symbols' info from stdin.
BEGIN {
  error = 0
}

{
  if ($1 == "defined") {
    symtab[$3] = $2;
    modtab[$2] = "" modtab[$2]
  } else if ($1 == "undefined") {
    if ($3 in symtab)
      modtab[$2] = modtab[$2] " " symtab[$3];
    else if ($3 != "__gnu_local_gp" && $3 != "_gp_disp") {
      printf "%s in %s is not defined\n", $3, $2 >"/dev/stderr";
      error++;
    }
  }
  else {
    printf "error: %u: unrecognized input format\n", NR;
    error++;
  }
}

# Output the result.
END {
  if (error >= 1)
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
      if (depmod != "kernel" && depmod != mod)
	uniqmods[depmod] = 1;
    }
    modlist = ""
    for (depmod in uniqmods) {
      modlist = modlist " " depmod;
    }
    if (mod == "all_video") {
	continue;
    }
    printf "%s:%s\n", mod, modlist;
  }
  modlist = ""
  while (getline <"video.lst") {
      modlist = modlist " " $1;
  }
  printf "all_video:%s\n", modlist;
}
