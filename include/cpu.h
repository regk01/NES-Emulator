#ifndef CPU_H
#define CPU_H

#include "types.h"
#include "instructions.h"

#ifdef BIT
#undef BIT
#endif

#define STACK_BOTTOM 0x100
#define STACK_TOP 0x1FF

typedef struct NES_CORE NES_CORE;

typedef struct CPU {
    // Registers
    byte a; // accumulator
    byte x; // index
    byte y; // index
    byte sp; // stack pointer
    uint16 pc; // program counter
    byte sr; // status register

    byte opcode; // current opcode
    uint32 cycles; // cycles left
    uint16 addr_abs; // address the instruction uses
    uint16 addr_rel; // for branch instructions
    byte fetched; // data byte fetched

    NES_CORE *core;
    Instructions *lookup;
    uint64 clock_count; // debugging stuff

    byte dma_dummy;
    byte dma_data;
    byte dma_addr;
    byte dma_page;
} CPU;

typedef enum CPU_FLAG {
    C = 0, // carry
    Z = 1, // zero
    I = 2, // interrupt disable
    D = 3, // decimal -> not used by NES
    B = 4, // break
    U = 5, // unused
    V = 6, // overflow
    N = 7, // negative
} CPU_FLAG;

void CPU_init(CPU *self);
byte CPU_read(CPU *self, uint16 addr);
void CPU_write(CPU *self, uint16 addr, byte val);
void CPU_fetch(CPU *self);
void CPU_clock(CPU *self);
void CPU_reset(CPU *self);

void CPU_connect_core(CPU *self, NES_CORE *core);

// Interrupts
void CPU_NMI(CPU *self);
void CPU_IRQ(CPU *self);

// Addressing Modes
uint8 IMP(CPU *self); uint8 ACC(CPU *self); uint8 IMM(CPU *self);
uint8 ZP(CPU *self); uint8 ZPX(CPU *self); uint8 ZPY(CPU *self);
uint8 ABS(CPU *self); uint8 ABSX(CPU *self); uint8 ABSY(CPU *self);
uint8 IND(CPU *self); uint8 IZX(CPU *self); uint8 IZY(CPU *self);
uint8 REL(CPU *self);

// Instructions
uint8 LDA(CPU *self); uint8 STA(CPU *self); uint8 ADC(CPU *self); uint8 SBC(CPU *self); uint8 AND(CPU *self);
uint8 ORA(CPU *self); uint8 EOR(CPU *self); uint8 CMP(CPU *self); uint8 ASL(CPU *self); uint8 LSR(CPU *self);
uint8 ROL(CPU *self); uint8 ROR(CPU *self); uint8 INC(CPU *self); uint8 DEC(CPU *self); uint8 LDX(CPU *self);
uint8 STX(CPU *self); uint8 BCC(CPU *self); uint8 BCS(CPU *self); uint8 BEQ(CPU *self); uint8 BIT(CPU *self);
uint8 BMI(CPU *self); uint8 BNE(CPU *self); uint8 BPL(CPU *self); uint8 BRK(CPU *self); uint8 BVC(CPU *self);
uint8 BVS(CPU *self); uint8 CLC(CPU *self); uint8 CLD(CPU *self); uint8 CLI(CPU *self); uint8 CLV(CPU *self);
uint8 CPX(CPU *self); uint8 CPY(CPU *self); uint8 DEX(CPU *self); uint8 DEY(CPU *self); uint8 INX(CPU *self);
uint8 INY(CPU *self); uint8 JMP(CPU *self); uint8 JSR(CPU *self); uint8 NOP(CPU *self); uint8 PHA(CPU *self);
uint8 PHP(CPU *self); uint8 PLA(CPU *self); uint8 PLP(CPU *self); uint8 RTI(CPU *self); uint8 RTS(CPU *self);
uint8 SEC(CPU *self); uint8 SED(CPU *self); uint8 SEI(CPU *self); uint8 LDY(CPU *self); uint8 STY(CPU *self);
uint8 TAX(CPU *self); uint8 TAY(CPU *self); uint8 TYA(CPU *self); uint8 TSX(CPU *self); uint8 TXA(CPU *self);
uint8 TXS(CPU *self);

#endif