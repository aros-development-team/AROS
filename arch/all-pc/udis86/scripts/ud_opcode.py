# udis86 - scripts/ud_opcode.py
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
import copy
from typing import Optional, ItemsView, Union, List, Dict

XMLInstruction = Dict[str, Union[List[str], str]]

EMPTY_EFLAGS = "____________"


class TypeCollisionError(Exception):
    """
    Class to signal a type collision error between instructions
    """
    pass


class CollisionError(Exception):
    def __init__(self, obj1, obj2):
        self.obj1, self.obj2 = obj1, obj2


class UdInsnDef:
    """
    An x86 instruction definition
    """
    def __init__(self, **ins_data):
        self.mnemonic = ins_data['mnemonic']
        self.eflags = ins_data['eflags'] if 'eflags' in ins_data else EMPTY_EFLAGS
        self.firstOpAccess = ins_data['firstOpAccess']
        self.secondOpAccess = ins_data['secondOpAccess']
        self.access = ins_data['access']
        self.implicitRegUse = ins_data['implicitRegUse']
        self.implicitRegDef = ins_data['implicitRegDef']
        self.prefixes = ins_data['prefixes']
        self.opcodes = ins_data['opcodes']
        self.operands = ins_data['operands']
        self._cpuid = ins_data['cpuid']
        self._opcexts = {}

        for opc in self.opcodes:
            if opc.startswith('/'):
                e, v = opc.split('=')
                self._opcexts[e] = v

    def has_prefix(self, pfx: str) -> bool:
        return pfx in self.prefixes

    @property
    def vendor(self) -> Optional[str]:
        return self._opcexts.get('/vendor', None)

    @property
    def mode(self) -> Optional[str]:
        return self._opcexts.get('/m', None)

    @property
    def o_size(self) -> Optional[str]:
        return self._opcexts.get('/o', None)

    def is_def_64(self) -> bool:
        return self.has_prefix('def64')

    def __str__(self) -> str:
        return f"{self.mnemonic} {', '.join(self.operands)} {' '.join(self.opcodes)}"


UdOpcodeTableEntry = Union[UdInsnDef, "UdOpcodeTable"]


class UdOpcodeTable:
    """
    A single table of instruction definitions, indexed by a decode field.
    """

    @classmethod
    def vendor2idx(cls, vendor: str) -> int:
        return 0 if vendor == 'amd' else (1 if vendor == 'intel' else 2)

    @classmethod
    def vex2idx(cls, vex: str) -> int:
        if vex.startswith("none_"):
            vex = vex[5:]
        vex_opc_ext_map = {
            'none': 0x0,
            '0f': 0x1,
            '0f38': 0x2,
            '0f3a': 0x3,
            '66': 0x4,
            '66_0f': 0x5,
            '66_0f38': 0x6,
            '66_0f3a': 0x7,
            'f3': 0x8,
            'f3_0f': 0x9,
            'f3_0f38': 0xa,
            'f3_0f3a': 0xb,
            'f2': 0xc,
            'f2_0f': 0xd,
            'f2_0f38': 0xe,
            'f2_0f3a': 0xf,
            # XOP (3 byte VEX encoding)...
            '08': 0x10,
            '66_08': 0x11,
            'f3_08': 0x12,
            'f2_08': 0x13,
            '09': 0x14,
            '66_09': 0x15,
            'f3_09': 0x16,
            'f2_09': 0x17,
            '0a': 0x18,
            '66_0a': 0x19,
            'f3_0a': 0x1a,
            'f2_0a': 0x1b,
            '10': 0,
        }
        return vex_opc_ext_map[vex]

    # A mapping of opcode extensions to their representational values used in the opcode map.
    OpcExtMap = {
        '/rm': lambda v: int(v, 16),
        '/x87': lambda v: int(v, 16),
        '/3dnow': lambda v: int(v, 16),
        '/reg': lambda v: int(v, 16),
        # modrm.mod
        # (!11, 11)    => (00b, 01b)
        '/mod': lambda v: 0 if v == '!11' else 1,
        # Mode extensions:
        # (16, 32, 64) => (00, 01, 02)
        '/o': lambda v: int(int(v) / 32),
        '/a': lambda v: int(int(v) / 32),
        # Disassembly mode 
        # (!64, 64)    => (00b, 01b)
        '/m': lambda v: 1 if v == '64' else 0,
        # SSE
        # none => 0
        # f2   => 1
        # f3   => 2
        # 66   => 3
        '/sse': lambda v: (0 if v == 'none' else int(((int(v, 16) & 0xf) + 1) / 2)),
        # AVX
        '/vex': lambda v: UdOpcodeTable.vex2idx(v),
        '/vexw': lambda v: 0 if v == '0' else 1,
        '/vexl': lambda v: 0 if v == '0' else 1,
        # Vendor
        '/vendor': lambda v: UdOpcodeTable.vendor2idx(v)
    }

    _TableInfo = {
        'opctbl': {'label': 'UD_TAB__OPC_TABLE', 'size': 256},
        '/sse': {'label': 'UD_TAB__OPC_SSE', 'size': 4},
        '/reg': {'label': 'UD_TAB__OPC_REG', 'size': 8},
        '/rm': {'label': 'UD_TAB__OPC_RM', 'size': 8},
        '/mod': {'label': 'UD_TAB__OPC_MOD', 'size': 2},
        '/m': {'label': 'UD_TAB__OPC_MODE', 'size': 2},
        '/x87': {'label': 'UD_TAB__OPC_X87', 'size': 64},
        '/a': {'label': 'UD_TAB__OPC_ASIZE', 'size': 3},
        '/o': {'label': 'UD_TAB__OPC_OSIZE', 'size': 3},
        '/3dnow': {'label': 'UD_TAB__OPC_3DNOW', 'size': 256},
        '/vendor': {'label': 'UD_TAB__OPC_VENDOR', 'size': 3},
        '/vex': {'label': 'UD_TAB__OPC_VEX', 'size': 28},
        '/vexw': {'label': 'UD_TAB__OPC_VEX_W', 'size': 2},
        '/vexl': {'label': 'UD_TAB__OPC_VEX_L', 'size': 2},
    }

    def __init__(self, typ: str):
        assert typ in self._TableInfo
        self._typ = typ
        self._limit = 0
        self._entries = {}

    def size(self) -> int:
        return self._TableInfo[self._typ]['size']

    def entries(self) -> ItemsView:
        return self._entries.items()

    def num_entries(self) -> int:
        return len(self._entries.keys())

    def set_limit(self, new_limit: int):
        self._limit = new_limit

    def limit(self) -> int:
        return self._limit

    def label(self) -> str:
        return self._TableInfo[self._typ]['label']

    def typ(self) -> str:
        return self._typ

    def meta(self) -> str:
        return self._typ

    def __str__(self):
        return f"table-{self._typ}"

    def add(self, opc: str, obj: UdOpcodeTableEntry):
        typ = self.get_type_for_opcode(opc)
        idx = self.get_idx_for_opcode(opc)
        if self._typ != typ or idx in self._entries:
            raise TypeCollisionError(f"{self._typ} <-> {typ} ({opc}) 2")
        self._entries[idx] = obj

    def lookup(self, opc: str) -> Optional[UdOpcodeTableEntry]:
        typ = self.get_type_for_opcode(opc)
        idx = self.get_idx_for_opcode(opc)
        if self._typ != typ:
            raise TypeCollisionError(f"{self._typ} <-> {typ} ({opc})")
        return self._entries.get(idx, None)

    def get_entry(self, index: int) -> Optional[UdOpcodeTableEntry]:
        """
        Returns the entry at a given index of the table, None if there is none. Raises an exception if the index is out
        of bounds.
        """
        if index < self.size():
            return self._entries.get(index, None)
        raise IndexError(f"index out of bounds: {index}")

    def set_entry(self, index: int, entry: UdOpcodeTableEntry):
        if index < self.size():
            self._entries[index] = entry
        else:
            raise IndexError(f"index out of bounds: {index} (current size is {self.size()}")

    @classmethod
    def get_type_for_opcode(cls, opc: str) -> str:
        if opc.startswith('/'):
            return opc.split('=')[0]
        return 'opctbl'

    @classmethod
    def get_idx_for_opcode(cls, opc: str) -> int:
        if opc.startswith('/'):
            typ, v = opc.split('=')
            return cls.OpcExtMap[typ](v)
        # plain opctbl opcode
        return int(opc, 16)

    @classmethod
    def get_labels(cls) -> List[str]:
        """Returns a list of all labels"""
        return [v['label'] for v in cls._TableInfo.values()]


class UdOpcodeTables(object):
    """
    Collection of opcode tables
    """

    def add_table_for_type(self, typ: str) -> UdOpcodeTable:
        """
        Create a new opcode table of a give type `typ`.
        """
        tbl = UdOpcodeTable(typ)
        self._tables.append(tbl)
        return tbl

    def build_opcode_tree(self, opcodes: List[str], obj: UdOpcodeTableEntry) -> UdOpcodeTable:
        """
        Recursively construct a tree entry mapping a string of opcodes to an object.
        """
        if len(opcodes) == 0:
            return obj
        opc = opcodes[0]
        tbl = self.add_table_for_type(UdOpcodeTable.get_type_for_opcode(opc))
        tbl.add(opc, self.build_opcode_tree(opcodes[1:], obj))
        return tbl

    def walk(self, tbl: UdOpcodeTable, opcodes: List[str]) -> Optional[UdOpcodeTableEntry]:
        """
        Walk down the opcode tree, starting at a given opcode table, given a string of opcodes. Return None if unable
        to walk, the object at the leaf otherwise.
        """
        opc = opcodes[0]
        e = tbl.lookup(opc)
        if e is None:
            return None
        elif isinstance(e, UdOpcodeTable) and len(opcodes[1:]):
            return self.walk(e, opcodes[1:])
        return e

    def map(self, tbl: UdOpcodeTable, opcodes: List[str], obj: UdOpcodeTableEntry):
        """
        Create a mapping from a given string of opcodes to an object in the opcode tree. Constructs tree branches as
        needed.
        """
        opc = opcodes[0]
        e = tbl.lookup(opc)
        if e is None:
            tbl.add(opc, self.build_opcode_tree(opcodes[1:], obj))
        else:
            if len(opcodes[1:]) == 0:
                raise CollisionError(e, obj)
            self.map(e, opcodes[1:], obj)

    def __init__(self, xml: str):
        self._tables = []
        self._insns = []
        self._mnemonics = {}

        # The root table is always a 256 entry opctbl, indexed by a plain opcode byte
        self.root = self.add_table_for_type('opctbl')

        if os.getenv("UD_OPCODE_DEBUG"):
            self._logFh = open("opcodeTables.log", "w")

        # add an invalid instruction entry without any mapping in the opcode tables.
        self.invalidInsn = UdInsnDef(
            mnemonic="invalid",
            eflags=EMPTY_EFLAGS,
            firstOpAccess="",
            secondOpAccess="",
            access=[],
            implicitRegUse=[],
            implicitRegDef=[],
            opcodes=[],
            cpuid=[],
            operands=[],
            prefixes=[])
        self._insns.append(self.invalidInsn)

        # Construct UdOpcodeTables object from the given udis86 optable.xml
        for insn in self.parse_xml_optable(xml):
            self.add_instruction(insn)
        self.patch_avx_2byte()
        self.merge_sse_none_entries()
        self.print_statistics()

    def log(self, s: str):
        if os.getenv("UD_OPCODE_DEBUG"):
            self._logFh.write(s + "\n")

    def merge_sse_none_entries(self):
        """
        Merge sse tables with only one entry for /sse=none
        """
        for table in self._tables:
            for k, e in table.entries():
                if isinstance(e, UdOpcodeTable) and e.typ() == '/sse':
                    if e.num_entries() == 1:
                        sse = e.lookup("/sse=none")
                        if sse:
                            table.set_entry(k, sse)
        unique_tables = {}

        def gen_table_list(tbl: UdOpcodeTable):
            if tbl not in unique_tables:
                self._tables.append(tbl)
            unique_tables[tbl] = 1
            for _, entry in tbl.entries():
                if isinstance(entry, UdOpcodeTable):
                    gen_table_list(entry)

        self._tables = []
        gen_table_list(self.root)

    def patch_avx_2byte(self):
        # create avx tables
        for pp in (None, 'f2', 'f3', '66'):
            for m in (None, '0f', '0f38', '0f3a'):
                if pp is None and m is None:
                    continue
                if pp is None:
                    vex = m
                elif m is None:
                    vex = pp
                else:
                    vex = pp + '_' + m
                table = self.walk(self.root, ['c4', f'/vex={vex}'])
                self.map(self.root, ['c5', f'/vex={vex}'], table)

    def _add_instruction(self, **ins_data):
        # Canonicalize opcode list
        opcexts = ins_data['opcexts']
        opcodes = list(ins_data['opcodes'])

        # Re-order vex
        if '/vex' in opcexts:
            assert opcodes[0] in ('c4', 'c5', '8f')
            opcodes.insert(1, '/vex=' + opcexts['/vex'])

        # Add extensions. The order is important, and determines how well the opcode table is packed. Also note,
        # /sse must be before /o, because /sse may consume operand size prefix affect the outcome of /o.
        for ext in ('/mod', '/x87', '/reg', '/rm', '/sse', '/o', '/a', '/m', '/vexw', '/vexl', '/3dnow', '/vendor'):
            if ext in opcexts:
                opcodes.append(f"{ext}={opcexts[ext]}")

        insn = UdInsnDef(
            mnemonic=ins_data['mnemonic'],
            eflags=ins_data['eflags'],
            firstOpAccess=ins_data['firstOpAccess'],
            secondOpAccess=ins_data['secondOpAccess'],
            implicitRegUse=ins_data['implicitRegUse'],
            implicitRegDef=ins_data['implicitRegDef'],
            prefixes=ins_data['prefixes'],
            operands=ins_data['operands'],
            access=ins_data['access'],
            opcodes=opcodes,
            cpuid=ins_data['cpuid'])

        try:
            self.map(self.root, opcodes, insn)
        except CollisionError as e:
            self.pretty_print()
            print(opcodes, insn, str(e.obj1), str(e.obj2))
            raise
        except Exception:
            self.pretty_print()
            raise

        self._insns.append(insn)
        # add to lookup by mnemonic structure
        if insn.mnemonic not in self._mnemonics:
            self._mnemonics[insn.mnemonic] = [insn]
        else:
            self._mnemonics[insn.mnemonic].append(insn)

    def _add_sse2_avx_instruction(self, **ins_data):
        """
        Add an instruction definition containing an avx cpuid bit, but declared in its legacy SSE form. The function
        splits the definition to create two new definitions, one for SSE and one promoted to an AVX form.
        """

        # SSE
        sse_mnemonic = ins_data['mnemonic']
        sse_eflags = ins_data['eflags']
        sse_first_op_access = ins_data['firstOpAccess']
        sse_second_op_access = ins_data['secondOpAccess']
        sse_implicit_reg_use = ins_data['implicitRegUse']
        sse_implicit_reg_def = ins_data['implicitRegDef']
        sse_opcodes = ins_data['opcodes']

        # remove vex opcode extensions
        sse_opcexts = {e: v for (e, v) in ins_data['opcexts'].items() if not e.startswith('/vex')}
        # strip out avx operands, preserving relative ordering of remaining operands
        sse_operands = [opr for opr in ins_data['operands'] if opr not in ('H', 'L')]

        sse_access = []

        for idx in range(len(sse_operands)):
            if idx+1 > len(ins_data['access']):
                break
            sse_access.append(ins_data['access'][idx])

        # strip out avx prefixes
        sse_prefixes = [pfx for pfx in ins_data['prefixes'] if not pfx.startswith('vex')]

        # strip out avx bits from cpuid
        sse_cpuid = [flag for flag in ins_data['cpuid'] if not flag.startswith('avx')]

        self._add_instruction(
            mnemonic=sse_mnemonic,
            eflags=sse_eflags,
            firstOpAccess=sse_first_op_access,
            secondOpAccess=sse_second_op_access,
            implicitRegUse=sse_implicit_reg_use,
            implicitRegDef=sse_implicit_reg_def,
            prefixes=sse_prefixes,
            opcodes=sse_opcodes,
            opcexts=sse_opcexts,
            operands=sse_operands,
            access=sse_access,
            cpuid=sse_cpuid)

        # AVX
        vex_mnemonic = 'v' + ins_data['mnemonic']
        vex_eflags = ins_data['eflags']
        vex_first_op_access = ins_data['firstOpAccess']
        vex_second_op_access = ins_data['secondOpAccess']
        vex_implicit_reg_use = ins_data['implicitRegUse']
        vex_implicit_reg_def = ins_data['implicitRegDef']
        vex_prefixes = ins_data['prefixes']
        vex_opcodes = ['c4']
        vex_opcexts = {e: ins_data['opcexts'][e] for e in ins_data['opcexts'] if e != '/sse'}
        vex_opcexts['/vex'] = ins_data['opcexts']['/sse'] + '_' + '0f'
        if ins_data['opcodes'][1] in ('38', '3a'):
            vex_opcexts['/vex'] += ins_data['opcodes'][1]
            vex_opcodes.extend(ins_data['opcodes'][2:])
        else:
            vex_opcodes.extend(ins_data['opcodes'][1:])
        vex_operands = []
        for o in ins_data['operands']:
            # make the operand size explicit: x
            if o in ('V', 'W', 'H', 'U', 'M'):
                o = o + 'x'
            vex_operands.append(o)

        vex_access = ins_data['access']
        vex_cpuid = [flag for flag in ins_data['cpuid'] if not flag.startswith('sse')]

        self._add_instruction(
            mnemonic=vex_mnemonic,
            eflags=vex_eflags,
            firstOpAccess=vex_first_op_access,
            secondOpAccess=vex_second_op_access,
            implicitRegUse=vex_implicit_reg_use,
            implicitRegDef=vex_implicit_reg_def,
            prefixes=vex_prefixes,
            opcodes=vex_opcodes,
            opcexts=vex_opcexts,
            operands=vex_operands,
            access=vex_access,
            cpuid=vex_cpuid)

    def add_instruction(self, instruction: XMLInstruction):
        opcodes = []
        opcexts = {}

        if not instruction['opcodes']:
            self._mnemonics[instruction['mnemonic']] = [UdInsnDef(
                mnemonic=instruction['mnemonic'],
                flags=instruction['eflags'],
                firstOpAccess=instruction['firstOpAccess'],
                secondOpAccess=instruction['secondOpAccess'],
                implicitRegUse=instruction['implicitRegUse'],
                implicitRegDef=instruction['implicitRegDef'],
                opcodes=[],
                cpuid=[],
                operands=[],
                access=[],
                prefixes=[]
            )]
            return

        # pack plain opcodes first, and collect opcode extensions
        for opc in instruction['opcodes']:
            if not opc.startswith('/'):
                opcodes.append(opc)
            else:
                e, v = opc.split('=')
                opcexts[e] = v

        # treat vendor as an opcode extension
        if len(instruction['vendor']):
            opcexts['/vendor'] = instruction['vendor'][0]

        if instruction['mnemonic'] in ('lds', 'les'):
            # Massage lds and les, which share the same prefix as AVX instructions, to work well with the opcode tree.
            opcexts['/vex'] = 'none'
        elif '/vex' in opcexts:
            # A proper avx instruction definition; make sure there are no legacy opcode extensions
            assert '/sse' not in opcodes

            # make sure the opcode definitions don't already include the avx prefixes.
            if len(opcodes) > 0:
                assert opcodes[0] not in ('c4', 'c5')

            # An avx only instruction is defined by the /vex= opcode extension. They do not include the c4 (long form)
            # or c5 (short form) prefix. As part of opcode table generate, here we create the long form definition,
            # and then patch the table for c5 in a later stage.
            # Construct a long-form definition of the avx instruction
            if 'xop' in instruction['cpuid']:
                opcodes.insert(0, '8f')
            else:
                opcodes.insert(0, 'c4')
        elif opcodes[0] == '0f' and opcodes[1] != '0f' and '/sse' not in opcexts:
            # Make all 2-byte opcode form instructions play nice with sse opcode maps.
            opcexts['/sse'] = 'none'

        # legacy sse defs that get promoted to avx
        fn = self._add_instruction
        if 'avx' in instruction['cpuid']:
            if '/sse' in opcexts:
                fn = self._add_sse2_avx_instruction
            else:
                avx_operands = []
                for o in instruction['operands']:
                    # make the operand size explicit: x
                    if o in ('V', 'W', 'H', 'U', 'M'):
                        o = o + 'x'
                    avx_operands.append(o)
                instruction['operands'] = avx_operands

        fn(mnemonic=instruction['mnemonic'],
           eflags=instruction['eflags'],
           firstOpAccess=instruction['firstOpAccess'],
           secondOpAccess=instruction['secondOpAccess'],
           implicitRegUse=instruction['implicitRegUse'],
           implicitRegDef=instruction['implicitRegDef'],
           prefixes=instruction['prefixes'],
           opcodes=opcodes,
           opcexts=opcexts,
           operands=instruction['operands'],
           access=instruction['access'],
           cpuid=instruction['cpuid'])

    def get_instructions(self) -> List[UdInsnDef]:
        """Returns a list of all instructions in the collection"""
        return self._insns

    def get_tables(self) -> List[UdOpcodeTable]:
        """Returns a list of all tables in the collection"""
        return self._tables

    def get_mnemonics(self) -> List[str]:
        """Returns a sorted list of mnemonics"""
        return sorted(self._mnemonics.keys())

    def pretty_print(self):
        def print_walk(tbl, indent=""):
            for k, e in tbl.entries():
                if isinstance(e, UdOpcodeTable):
                    self.log("%s    |-<%02x> %s" % (indent, k, e))
                    print_walk(e, indent + "    |")
                elif isinstance(e, UdInsnDef):
                    self.log("%s    |-<%02x> %s" % (indent, k, e))
        print_walk(self.root)

    def print_statistics(self):
        tables = self.get_tables()
        self.log("stats: ")
        self.log(f"  Num tables    = {len(tables)}")
        self.log(f"  Num insnDefs  = {len(self.get_instructions())}")
        self.log(f"  Num insns     = {len(self.get_mnemonics())}")

        total_size = 0
        total_entries = 0
        for table in tables:
            total_size += table.size()
            total_entries += table.num_entries()
        self.log(f"  Packing Ratio = {(total_entries * 100) / total_size}%")
        self.log("--------------------")

        self.pretty_print()

    @staticmethod
    def parse_xml_optable(xml: str) -> List[XMLInstruction]:
        """
        Parse udis86 optable.xml file and return list of instruction definitions.
        """
        from xml.dom import minidom

        xml_doc = minidom.parse(xml)
        tl_node = xml_doc.firstChild
        insns = []

        while tl_node and tl_node.localName != "x86optable":
            tl_node = tl_node.nextSibling

        for insn_node in tl_node.childNodes:
            if not insn_node.localName:
                continue
            if insn_node.localName != "instruction":
                raise Exception(f"warning: invalid insn node - {insn_node.localName}")
            mnemonic = insn_node.getElementsByTagName('mnemonic')[0].firstChild.data
            vendor = ''
            cpuid = []
            global_eflags = EMPTY_EFLAGS
            global_first_op_access = "R"
            global_second_op_access = "R"
            global_implicit_reg_use = []
            global_implicit_reg_def = []

            for node in insn_node.childNodes:
                if node.localName == 'vendor':
                    vendor = node.firstChild.data.split()
                elif node.localName == 'cpuid':
                    cpuid = node.firstChild.data.split()
                elif node.localName == 'eflags':
                    global_eflags = node.firstChild.data
                elif node.localName == 'first_operand_access':
                    global_first_op_access = node.firstChild.data
                elif node.localName == 'second_operand_access':
                    global_second_op_access = node.firstChild.data
                elif node.localName == 'implicit_register_use':
                    global_implicit_reg_use.append(node.firstChild.data)
                elif node.localName == 'implicit_register_def':
                    global_implicit_reg_def.append(node.firstChild.data)

            for node in insn_node.childNodes:
                if node.localName == 'def':
                    eflags = copy.deepcopy(global_eflags)
                    first_op_access = copy.deepcopy(global_first_op_access)
                    second_op_access = copy.deepcopy(global_second_op_access)
                    implicit_reg_use = copy.deepcopy(global_implicit_reg_use)
                    implicit_reg_def = copy.deepcopy(global_implicit_reg_def)
                    insn_def = {'pfx': []}
                    for child_node in node.childNodes:
                        if not child_node.localName:
                            continue
                        if child_node.localName in ('pfx', 'opc', 'opr', 'access', 'vendor', 'cpuid'):
                            insn_def[child_node.localName] = child_node.firstChild.data.split()
                        elif child_node.localName == 'eflags':
                            eflags = child_node.firstChild.data
                        elif child_node.localName == 'first_operand_access':
                            first_op_access = child_node.firstChild.data
                        elif child_node.localName == 'second_operand_access':
                            second_op_access = child_node.firstChild.data
                        elif child_node.localName == 'implicit_register_use':
                            implicit_reg_use.append(child_node.firstChild.data)
                        elif child_node.localName == 'implicit_register_def':
                            implicit_reg_def.append(child_node.firstChild.data)
                        elif child_node.localName == 'mode':
                            insn_def['pfx'].extend(child_node.firstChild.data.split())
                    insns.append({
                        'prefixes': insn_def.get('pfx', []),
                        'mnemonic': mnemonic,
                        'eflags': eflags,
                        'firstOpAccess': first_op_access,
                        'secondOpAccess': second_op_access,
                        'implicitRegUse': implicit_reg_use,
                        'implicitRegDef': implicit_reg_def,
                        'opcodes': insn_def.get('opc', []),
                        'operands': insn_def.get('opr', []),
                        'vendor': insn_def.get('vendor', vendor),
                        'access': insn_def.get('access', []),
                        'cpuid': insn_def.get('cpuid', cpuid)
                    })
        return insns
