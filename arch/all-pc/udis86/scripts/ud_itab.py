# udis86 - scripts/ud_itab.py
#
# Copyright (c) 2009, 2013 Vivek Thampi
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import sys
from typing import TextIO, List

from ud_opcode import UdOpcodeTable, UdOpcodeTables, UdInsnDef


class UdItabGenerator:
    OperandDict = {
        "Av": ["OP_A", "SZ_V"],
        "B": ["OP_B", "SZ_DQ"],
        "BM": ["OP_BM", "SZ_DQ"],
        "BMqR": ["OP_BMR", "SZ_QO"],
        "BMdqR": ["OP_BMR", "SZ_DQO"],
        "E": ["OP_E", "SZ_NA"],
        "Eb": ["OP_E", "SZ_B"],
        "Ew": ["OP_E", "SZ_W"],
        "Ev": ["OP_E", "SZ_V"],
        "Ed": ["OP_E", "SZ_D"],
        "Ey": ["OP_E", "SZ_Y"],
        "Eq": ["OP_E", "SZ_Q"],
        "Ez": ["OP_E", "SZ_Z"],
        "Erdq": ["OP_E", "SZ_RDQ"],
        "Fv": ["OP_F", "SZ_V"],
        "G": ["OP_G", "SZ_NA"],
        "Gb": ["OP_G", "SZ_B"],
        "Gw": ["OP_G", "SZ_W"],
        "Gv": ["OP_G", "SZ_V"],
        "Gy": ["OP_G", "SZ_Y"],
        "Gd": ["OP_G", "SZ_D"],
        "Gq": ["OP_G", "SZ_Q"],
        "Gz": ["OP_G", "SZ_Z"],
        "M": ["OP_M", "SZ_NA"],
        "Mb": ["OP_M", "SZ_B"],
        "Mw": ["OP_M", "SZ_W"],
        "Ms": ["OP_M", "SZ_W"],
        "Md": ["OP_M", "SZ_D"],
        "Mq": ["OP_M", "SZ_Q"],
        "Mdq": ["OP_M", "SZ_DQ"],
        "Mqq": ["OP_M", "SZ_QQ"],
        "Mrdq": ["OP_M", "SZ_RDQ"],
        "Mv": ["OP_M", "SZ_V"],
        "Mx": ["OP_M", "SZ_X"],
        "Mt": ["OP_M", "SZ_T"],
        "Mo": ["OP_M", "SZ_O"],
        "MbRd": ["OP_MR", "SZ_BD"],
        "MbRv": ["OP_MR", "SZ_BV"],
        "MwRv": ["OP_MR", "SZ_WV"],
        "MwRd": ["OP_MR", "SZ_WD"],
        "MwRy": ["OP_MR", "SZ_WY"],
        "MdRy": ["OP_MR", "SZ_DY"],
        "Kb": ["OP_K", "SZ_B"],
        "Kw": ["OP_K", "SZ_W"],
        "Kd": ["OP_K", "SZ_D"],
        "Kq": ["OP_K", "SZ_Q"],
        "KMb": ["OP_KM", "SZ_B"],
        "KMw": ["OP_KM", "SZ_W"],
        "KMd": ["OP_KM", "SZ_D"],
        "KMq": ["OP_KM", "SZ_Q"],
        "KHb": ["OP_KH", "SZ_B"],
        "KHw": ["OP_KH", "SZ_W"],
        "KHd": ["OP_KH", "SZ_D"],
        "KHq": ["OP_KH", "SZ_Q"],
        "I1": ["OP_I1", "SZ_NA"],
        "I3": ["OP_I3", "SZ_NA"],
        "Ib": ["OP_I", "SZ_B"],
        "Id": ["OP_I", "SZ_D"],
        "Iw": ["OP_I", "SZ_W"],
        "Iv": ["OP_I", "SZ_V"],
        "Iz": ["OP_I", "SZ_Z"],
        "sIb": ["OP_sI", "SZ_B"],
        "sIz": ["OP_sI", "SZ_Z"],
        "sIv": ["OP_sI", "SZ_V"],
        "Jv": ["OP_J", "SZ_V"],
        "Jz": ["OP_J", "SZ_Z"],
        "Jb": ["OP_J", "SZ_B"],
        "R": ["OP_R", "SZ_RDQ"],
        "Rv": ["OP_R", "SZ_V"],
        "C": ["OP_C", "SZ_NA"],
        "D": ["OP_D", "SZ_NA"],
        "S": ["OP_S", "SZ_W"],
        "Ob": ["OP_O", "SZ_B"],
        "Ow": ["OP_O", "SZ_W"],
        "Ov": ["OP_O", "SZ_V"],
        "U": ["OP_U", "SZ_O"],
        "Ux": ["OP_U", "SZ_X"],
        "V": ["OP_V", "SZ_DQ"],
        "Vdq": ["OP_V", "SZ_DQ"],
        "Vqq": ["OP_V", "SZ_QQ"],
        "Vsd": ["OP_V", "SZ_Q"],
        "Vx": ["OP_V", "SZ_X"],
        "HRv": ["OP_HR", "SZ_V"],
        "HRd": ["OP_HR", "SZ_D"],
        "HRq": ["OP_HR", "SZ_Q"],
        "H": ["OP_H", "SZ_X"],
        "Hx": ["OP_H", "SZ_X"],
        "Hdq": ["OP_H", "SZ_DQ"],
        "Hqq": ["OP_H", "SZ_QQ"],
        "W": ["OP_W", "SZ_DQ"],
        "Wdq": ["OP_W", "SZ_DQ"],
        "Wqq": ["OP_W", "SZ_QQ"],
        "Wsd": ["OP_W", "SZ_Q"],
        "Wx": ["OP_W", "SZ_X"],
        "L": ["OP_L", "SZ_O"],
        "Lx": ["OP_L", "SZ_X"],
        "Ldq": ["OP_L", "SZ_DQ"],
        "Lqq": ["OP_L", "SZ_QQ"],
        "MbU": ["OP_MU", "SZ_BO"],
        "MwU": ["OP_MU", "SZ_WO"],
        "MdU": ["OP_MU", "SZ_DO"],
        "MqU": ["OP_MU", "SZ_QO"],
        "MdqU": ["OP_MU", "SZ_DQO"],
        "MqqU": ["OP_MU", "SZ_QQO"],
        "XSd": ["OP_XS", "SZ_D"],
        "XSq": ["OP_XS", "SZ_Q"],
        "XSXd": ["OP_XSX", "SZ_D"],
        "XSXq": ["OP_XSX", "SZ_Q"],
        "XSYd": ["OP_XSY", "SZ_D"],
        "XSYq": ["OP_XSY", "SZ_Q"],
        "N": ["OP_N", "SZ_Q"],
        "P": ["OP_P", "SZ_Q"],
        "Q": ["OP_Q", "SZ_Q"],
        "AL": ["OP_AL", "SZ_B"],
        "AX": ["OP_AX", "SZ_W"],
        "eAX": ["OP_eAX", "SZ_Z"],
        "rAX": ["OP_rAX", "SZ_V"],
        "CL": ["OP_CL", "SZ_B"],
        "CX": ["OP_CX", "SZ_W"],
        "eCX": ["OP_eCX", "SZ_Z"],
        "rCX": ["OP_rCX", "SZ_V"],
        "DL": ["OP_DL", "SZ_B"],
        "DX": ["OP_DX", "SZ_W"],
        "eDX": ["OP_eDX", "SZ_Z"],
        "rDX": ["OP_rDX", "SZ_V"],
        "R0b": ["OP_R0", "SZ_B"],
        "R1b": ["OP_R1", "SZ_B"],
        "R2b": ["OP_R2", "SZ_B"],
        "R3b": ["OP_R3", "SZ_B"],
        "R4b": ["OP_R4", "SZ_B"],
        "R5b": ["OP_R5", "SZ_B"],
        "R6b": ["OP_R6", "SZ_B"],
        "R7b": ["OP_R7", "SZ_B"],
        "R0w": ["OP_R0", "SZ_W"],
        "R1w": ["OP_R1", "SZ_W"],
        "R2w": ["OP_R2", "SZ_W"],
        "R3w": ["OP_R3", "SZ_W"],
        "R4w": ["OP_R4", "SZ_W"],
        "R5w": ["OP_R5", "SZ_W"],
        "R6w": ["OP_R6", "SZ_W"],
        "R7w": ["OP_R7", "SZ_W"],
        "R0v": ["OP_R0", "SZ_V"],
        "R1v": ["OP_R1", "SZ_V"],
        "R2v": ["OP_R2", "SZ_V"],
        "R3v": ["OP_R3", "SZ_V"],
        "R4v": ["OP_R4", "SZ_V"],
        "R5v": ["OP_R5", "SZ_V"],
        "R6v": ["OP_R6", "SZ_V"],
        "R7v": ["OP_R7", "SZ_V"],
        "R0z": ["OP_R0", "SZ_Z"],
        "R1z": ["OP_R1", "SZ_Z"],
        "R2z": ["OP_R2", "SZ_Z"],
        "R3z": ["OP_R3", "SZ_Z"],
        "R4z": ["OP_R4", "SZ_Z"],
        "R5z": ["OP_R5", "SZ_Z"],
        "R6z": ["OP_R6", "SZ_Z"],
        "R7z": ["OP_R7", "SZ_Z"],
        "R0y": ["OP_R0", "SZ_Y"],
        "R1y": ["OP_R1", "SZ_Y"],
        "R2y": ["OP_R2", "SZ_Y"],
        "R3y": ["OP_R3", "SZ_Y"],
        "R4y": ["OP_R4", "SZ_Y"],
        "R5y": ["OP_R5", "SZ_Y"],
        "R6y": ["OP_R6", "SZ_Y"],
        "R7y": ["OP_R7", "SZ_Y"],
        "ES": ["OP_ES", "SZ_NA"],
        "CS": ["OP_CS", "SZ_NA"],
        "DS": ["OP_DS", "SZ_NA"],
        "SS": ["OP_SS", "SZ_NA"],
        "GS": ["OP_GS", "SZ_NA"],
        "FS": ["OP_FS", "SZ_NA"],
        "ST0": ["OP_ST0", "SZ_NA"],
        "ST1": ["OP_ST1", "SZ_NA"],
        "ST2": ["OP_ST2", "SZ_NA"],
        "ST3": ["OP_ST3", "SZ_NA"],
        "ST4": ["OP_ST4", "SZ_NA"],
        "ST5": ["OP_ST5", "SZ_NA"],
        "ST6": ["OP_ST6", "SZ_NA"],
        "ST7": ["OP_ST7", "SZ_NA"],
        "IMP_XMM0": ["OP_IMP_XMM0", "SZ_NA"],
        "NONE": ["OP_NONE", "SZ_NA"],
    }

    AccessDict = {
        "N": "UD_ACCESS_NONE",
        "R": "UD_ACCESS_READ",
        "W": "UD_ACCESS_WRITE",
        "RW": "UD_ACCESS_READ|UD_ACCESS_WRITE",
        "WR": "UD_ACCESS_READ|UD_ACCESS_WRITE",
    }
    
    # opcode prefix dictionary
    PrefixDict = {
        "rep": "P_str",
        "repz": "P_strz",
        "aso": "P_aso",
        "oso": "P_oso",
        "rexw": "P_rexw",
        "rexb": "P_rexb",
        "rexx": "P_rexx",
        "rexr": "P_rexr",
        "vexl": "P_vexl",
        "vexw": "P_vexw",
        "seg": "P_seg",
        "inv64": "P_inv64",
        "def64": "P_def64",
        "cast": "P_cast",
    }

    MnemonicAliases = ("invalid", "3dnow", "none", "db", "pause")

    def __init__(self, tables: UdOpcodeTables):
        self.tables = tables
        self._insnIndexMap, i = {}, 0
        for insn in tables.get_instructions():
            self._insnIndexMap[insn], i = i, i + 1

        self._tableIndexMap, i = {}, 0
        for table in tables.get_tables():
            self._tableIndexMap[table], i = i, i + 1

    def get_insn_index(self, insn: UdInsnDef) -> int:
        return self._insnIndexMap[insn]

    def get_table_index(self, table: UdOpcodeTable) -> int:
        return self._tableIndexMap[table]

    def get_table_name(self, table: UdOpcodeTable) -> str:
        return f"ud_itab__{self.get_table_index(table)}"

    def gen_opcode_table(self, table: UdOpcodeTable, fh: TextIO, is_global: bool = False):
        """
        Emit Opcode Table in C.
        """
        fh.write("\n")
        if not is_global:
            fh.write('static ')
        fh.write(f"const uint16_t {self.get_table_name(table)}[] = {{\n")
        limit = 0
        for i in range(table.size()):
            if i > 0 and i % 4 == 0:
                fh.write("\n")
            if i % 4 == 0:
                fh.write(f"  /* {i:2x} */")
            e = table.get_entry(i)
            if e is None:
                fh.write(f"{'INVALID':>12},")
                limit += 1
            elif isinstance(e, UdOpcodeTable):
                fh.write(f"{f'GROUP({self.get_table_index(e)})':>12},")
                limit += 1
            elif isinstance(e, UdInsnDef):
                fh.write(f"{self.get_insn_index(e):>12},")
                limit += 1
        table.set_limit(limit - 1)
        fh.write("\n")
        fh.write("};\n")

    def gen_opcode_tables(self, fh: TextIO):
        tables = self.tables.get_tables()
        for table in tables:
            self.gen_opcode_table(table, fh, table is self.tables.root)

    def gen_opcode_tables_lookup_index(self, fh: TextIO):
        fh.write("\n\n")
        fh.write("struct ud_lookup_table_list_entry ud_lookup_table_list[] = {\n")
        for table in self.tables.get_tables():
            if table.limit() > 255:
                print(f"error: invalid table limit: {table.limit()} \n")
            fh.write(f'    /* {self.get_table_index(table):03d} */ '
                     f'{{ {self.get_table_name(table)}, {table.label()}, "{table.meta()}", {table.limit()} }},\n')
        fh.write("};")

    def gen_insn_table(self, fh: TextIO):
        fh.write("struct ud_itab_entry ud_itab[] = {\n")
        for insn in self.tables.get_instructions():
            opr_c = ["O_NONE", "O_NONE", "O_NONE", "O_NONE"]
            acc_c = ["UD_ACCESS_NONE", "UD_ACCESS_NONE", "UD_ACCESS_NONE", "UD_ACCESS_NONE"]
            pfx_c = []
            
            opr = insn.operands
            for i in range(len(opr)):
                if not (opr[i] in self.OperandDict.keys()):
                    print("error: invalid operand declaration: %s\n" % opr[i])
                opr_c[i] = "O_" + opr[i]
            opr = f"{opr_c[0]}, {opr_c[1]}, {opr_c[2]}, {opr_c[3]}"

            op1_access = "UD_OP_ACCESS_READ"
            op2_access = "UD_OP_ACCESS_READ"

            if insn.firstOpAccess == "W":
                op1_access = "UD_OP_ACCESS_WRITE"
            elif insn.firstOpAccess == "RW":
                op1_access = "UD_OP_ACCESS_READ | UD_OP_ACCESS_WRITE"

            if insn.secondOpAccess == "W":
                op2_access = "UD_OP_ACCESS_WRITE"
            elif insn.secondOpAccess == "RW":
                op2_access = "UD_OP_ACCESS_READ | UD_OP_ACCESS_WRITE"

            acc = insn.access
            for i in range(len(acc)): 
                if not (acc[i] in self.AccessDict.keys()):
                    print("error: invalid operand declaration: %s\n" % acc[i])
                acc_c[i] = self.AccessDict[acc[i]]
            acc = f"{acc_c[0]}, {acc_c[1]}, {acc_c[2]}, {acc_c[3]}"
            
            for p in insn.prefixes:
                if p not in self.PrefixDict.keys():
                    print(f"error: invalid prefix specification: {p} \n")
                pfx_c.append(self.PrefixDict[p])
            if len(insn.prefixes) == 0:
                pfx_c.append("P_none")
            pfx = "|".join(pfx_c)

            flag_map = {
                '_': 'UD_FLAG_UNCHANGED',
                'T': 'UD_FLAG_TESTED',
                'M': 'UD_FLAG_MODIFIED',
                'R': 'UD_FLAG_RESET',
                'S': 'UD_FLAG_SET',
                'U': 'UD_FLAG_UNDEFINED',
                'P': 'UD_FLAG_PRIOR'
            }
            eflags = ", ".join(map(lambda f: flag_map[f], [flag for flag in insn.eflags]))
            
            implicit_uses = ", ".join(map(lambda r: "UD_R_" + r.upper(), insn.implicitRegUse))
            implicit_defs = ", ".join(map(lambda r: "UD_R_" + r.upper(), insn.implicitRegDef))

            if len(implicit_uses) > 0:
                implicit_uses += ", "
            if len(implicit_defs) > 0:
                implicit_defs += ", "

            implicit_uses += "UD_NONE"
            implicit_defs += "UD_NONE"

            fh.write("  /* %04d */ { UD_I%s, %s, %s, %s, %s, { {%s} }, {%s}, {%s}, %s },\n" % (
                self.get_insn_index(insn), insn.mnemonic, opr, op1_access, op2_access, pfx, eflags,
                implicit_uses, implicit_defs, acc
            ))
        fh.write("};\n")

    def get_mnemonics(self) -> List[str]:
        mnemonics = self.tables.get_mnemonics()
        mnemonics.extend(self.MnemonicAliases)
        return mnemonics

    def gen_mnemonics_list(self, fh: TextIO):
        fh.write("\n\n")
        fh.write("const char* ud_mnemonics_str[] = {\n    ")
        fh.write(",\n    ".join([f'"{m}"' for m in self.get_mnemonics()]))
        fh.write("\n};\n")

    def generate_itab_header(self, file_path: str):
        with open(file_path, "w") as fh:
            # Generate Table Type Enumeration
            fh.write("#ifndef UD_ITAB_H\n")
            fh.write("#define UD_ITAB_H\n\n")

            fh.write("/* itab.h -- generated by udis86:scripts/ud_itab.py, do no edit */\n\n")

            # table type enumeration
            fh.write("/* ud_table_type -- lookup table types (see decode.c) */\n")
            fh.write("enum ud_table_type {\n    ")
            enum = UdOpcodeTable.get_labels()
            fh.write(",\n    ".join(enum))
            fh.write("\n};\n\n")

            # mnemonic enumeration
            fh.write("/* ud_mnemonic -- mnemonic constants */\n")
            enum = "enum ud_mnemonic_code {\n    "
            enum += ",\n    ".join([f"UD_I{m}" for m in self.get_mnemonics()])
            enum += ",\n    UD_MAX_MNEMONIC_CODE"
            enum += "\n} UD_ATTR_PACKED;\n"
            fh.write(enum)
            fh.write("\n")

            fh.write("extern const char * ud_mnemonics_str[];\n")
            fh.write("\n#endif /* UD_ITAB_H */\n")

    def generate_itab_source(self, file_path: str):
        with open(file_path, "w") as fh:
            fh.write("/* itab.c -- generated by udis86:scripts/ud_itab.py, do no edit */\n")
            fh.write("#include \"decode.h\"\n\n")

            fh.write("#define GROUP(n) (0x8000 | (n))\n")
            fh.write(f"#define INVALID  {self.get_insn_index(self.tables.invalidInsn)}\n\n")

            self.gen_opcode_tables(fh)
            self.gen_opcode_tables_lookup_index(fh)

            # Macros defining short-names for operands
            fh.write("\n\n/* itab entry operand definitions (for readability) */\n")
            for o in sorted(self.OperandDict.keys()):
                fh.write(f"#define O_{o:<7} {{ {self.OperandDict[o][0] + ',':<12} {self.OperandDict[o][1]:<8} }}\n")
            fh.write("\n")

            self.gen_insn_table(fh)
            self.gen_mnemonics_list(fh)

    def generate_itab_files(self, location: str):
        self.generate_itab_source(os.path.join(location, "itab.c"))
        self.generate_itab_header(os.path.join(location, "itab.h"))


def usage():
    print("usage: ud_itab.py <optable.xml> <output-path>")


def main():
    if len(sys.argv) != 3:
        usage()
        sys.exit(1)

    tables = UdOpcodeTables(xml=sys.argv[1])
    itab = UdItabGenerator(tables)
    itab.generate_itab_files(sys.argv[2])


if __name__ == '__main__':
    main()
