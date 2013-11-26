# -*- coding: iso-8859-15 -*-

import os, string

class MyTemplate(string.Template):
    # Template with () brackets instead of {}
    pattern = r"""
    \$(
      (?P<escaped>\$)                   |       # Escape sequence of two delimiters
      (?P<named>[_a-z][_a-z0-9]*)       |       # delimiter and a Python identifier
      \((?P<braced>[_a-z][_a-z0-9]*)\)  |       # delimiter and a braced identifier
      (?P<invalid>)                             # Other ill-formed delimiter exprs
    )
    """


class VarList(dict):
    def __getitem__(self, key):
        try:
            return dict.__getitem__(self, key)
        except KeyError:
            env_val = os.getenv(key)
            if env_val:
                return env_val
        raise KeyError("[MMAKE] can't find variable '%s'" % (key))
        #return "?$(%s)" % (key)


    def subst(self, str):
        """ Replace $(<variabe>) by variable's value.
        
        Keyword arguments:
        str -- the string whose parameters should be substituted
        """

        template = MyTemplate(str)
        try:
            return template.substitute(self) # calls __getitem__
        except ValueError:
            # handle e.g. $(addprefix -I,)
            return str


    def split_subst(self, str):
        elements = str.split()
        for index in range(len(elements)):
            elements[index] = self.subst(elements[index])
            if " " in elements[index]:
                raise ValueError("[MMAKE] no space allowed")
        return elements


if __name__ == "__main__":
    vl = VarList()
    vl["abc"] = "xxx"
    vl["def"] = "yyy"
    #print vl.subst("dkd $(abc) $(HOME) kdk $(INVALID)")
    print vl.subst("dkd $(abc) $(HOME) kdk ")
    print vl
