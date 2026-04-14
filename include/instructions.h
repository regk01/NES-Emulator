#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "types.h"

#define NUM_INSTRUCTIONS 256

typedef struct CPU CPU;

typedef struct Instructions {
    byte opcode; // its opcode
    uint8 (*addr_mode)(CPU *self); // Instruction's addressing mode
    uint8 (*operation)(CPU *self); // Instruction's operation

    uint8 cycles; // cycles needed
    bool isIllegal; // illegal opcode or not
} Instructions;

extern Instructions CPU_instructions[NUM_INSTRUCTIONS];

#endif