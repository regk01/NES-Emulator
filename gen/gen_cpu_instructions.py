import json

# Load instruction data
with open('gen/all_6502.json', 'r') as f:
    data = json.load(f)

# Maps JSON addressing modes to C function names
addr_map = {
    "implied": "IMP", "accumulator": "ACC", "immediate": "IMM",
    "zeropage": "ZP", "zeropageX": "ZPX", "zeropageY": "ZPY",
    "absolute": "ABS", "absoluteX": "ABSX", "absoluteY": "ABSY",
    "indirect": "IND", "indirectX": "IZX", "indirectY": "IZY",
    "relative": "REL"
}

c_code = """#include <stdlib.h>
#include "instructions.h"
#include "cpu.h"

// Lookup table of all 256 opcodes
Instructions CPU_instructions[NUM_INSTRUCTIONS] = {
"""

for inst in data:
    opcode = inst['opcode']
    # If illegal, uses NULL
    # TODO: illegal opcodes
    if inst.get('illegal', False):
        c_code += f"    {{ {opcode}, &{addr}, NULL, {cycles}, true }},\n"
    else:
        mnemonic = inst['mnemonics'][0]
        addr = addr_map.get(inst.get('addressingMode'), "NULL")
        cycles = inst.get('cycles', 0)
        c_code += f"    {{ {opcode}, &{addr}, &{mnemonic}, {cycles}, false }},\n"

c_code += "};"

with open('core/instructions.c', 'w') as f:
    f.write(c_code)

print("Generated instructions.c successfully.")