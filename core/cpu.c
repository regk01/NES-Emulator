#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"
#include "bus.h"
#include "core.h"
#include "logger.h"

// helper
// checks current opcode
bool isNESBranch(CPU *self) {
    return (self->opcode & 0x1F) == 0x10;
}

void CPU_init(CPU *self) {
    self->lookup = CPU_instructions;
    CPU_reset(self);
}

void CPU_connect_core(CPU *self, NES_CORE *core) {
    self->core = core;
}

byte CPU_read(CPU *self, uint16 addr) {
    return BUS_cpu_read(self->core, addr);
}

void CPU_write(CPU *self, uint16 addr, byte val) {
    BUS_cpu_write(self->core, addr, val);
}

// Returns function pointer of addr mode function
void *CPU_get_addr_mode(CPU *self) {
    return self->lookup[self->opcode].addr_mode;
};

// stores in fetched var
void CPU_fetch(CPU *self) {
    if ((CPU_get_addr_mode(self) != &IMP) && (CPU_get_addr_mode(self) != &ACC)) {
        self->fetched = CPU_read(self, self->addr_abs);
    };
}

byte CPU_get_flag(CPU *self, CPU_FLAG flag) {
    return (self->sr >> flag) & 0x01;
}

void CPU_set_flag(CPU *self, CPU_FLAG flag, bool val) {
    if (val) {
        self->sr |= 0x01 << flag;
    } else {
        self->sr &= ~(0x01 << flag);
    }
}

void CPU_stack_push(CPU *self, byte val) {
    CPU_write(self, STACK_BOTTOM + self->sp, val);
    self->sp--;
}

byte CPU_stack_pop(CPU *self) {
    self->sp++;
    byte temp = CPU_read(self, STACK_BOTTOM + self->sp);
    return temp;
}

// Cycles function
void CPU_clock(CPU *self) {
    if (self->core->dma_active) {
        if (self->dma_dummy) {
            if (self->clock_count % 2 == 1) {
                self->dma_dummy = false;
            }
        } else {
            if (self->clock_count % 2 == 0) {
                self->dma_data = BUS_cpu_read(self->core, (self->dma_page << 8) | self->dma_addr);
            } else {
                self->core->ppu.OAM.raw[self->dma_addr] = self->dma_data;
                self->dma_addr++;
                
                if (self->dma_addr == 0) {
                    self->core->dma_active = false;
                }
            }
        }
        self->clock_count++;
        return;
    }

    if (self->cycles == 0) {
        if (self->core->nmi_pending) {
            self->core->nmi_pending = false;
            CPU_NMI(self);
        } else if (self->core->irq_pending && !CPU_get_flag(self, I)) {
            // printf("IRQ Executed at PC: %04X\n", self->pc);
            // self->core->irq_pending = false;
            CPU_IRQ(self);
        } else {
            self->opcode = CPU_read(self, self->pc++);
            self->cycles = self->lookup[self->opcode].cycles;
            CPU_set_flag(self, U, true);

            if (self->core->loggingEnabled) {
                log_msg(0,
                        "PC:%04X OP:%02X A:%02X X:%02X Y:%02X P:%02X SP:%02X\n",
                        self->pc-1, self->opcode, self->a, self->x, self->y, 
                        self->sr, self->sp
                );
            }
            uint8 page_penalty = self->lookup[self->opcode].addr_mode(self);
            uint8 op_penalty = self->lookup[self->opcode].operation(self);

            if (isNESBranch(self) && op_penalty) {
                self->cycles += op_penalty + page_penalty;
            } else {
                self->cycles += op_penalty & page_penalty;
            }
            CPU_set_flag(self, U, true);
        }
    }

    self->clock_count++;
    self->cycles--;
}

// RESET (RES)
void CPU_reset(CPU *self) {
    byte low = CPU_read(self, 0xFFFC);
    byte high = CPU_read(self, 0xFFFD);
    self->pc = (high << 8) | low;

    self->a = 0;
    self->x = 0;
    self->y = 0;
    self->sp = 0xFD;
    self->sr = 0;
    CPU_set_flag(self, U, 1);
    CPU_set_flag(self, I, 1);

    self->addr_abs = 0x00;
    self->addr_rel = 0x00;
    self->fetched = 0x00;

	self->dma_page = 0x00;
	self->dma_addr = 0x00;
	self->dma_data = 0x00;
	self->dma_dummy = true;
	self->core->dma_active = false;

    self->cycles = 8;
}

// Interrupts
void CPU_NMI(CPU *self) {
    CPU_stack_push(self, (self->pc >> 8) & 0x00FF); // high byte first
    CPU_stack_push(self, self->pc & 0x00FF); // then low byte

    CPU_set_flag(self, U, true);
    CPU_set_flag(self, B, false);
    CPU_stack_push(self, self->sr);

    CPU_set_flag(self, I, true);

    byte low = CPU_read(self, 0xFFFA);
    byte high = CPU_read(self, 0xFFFB);
    self->pc = (high << 8) | low;

    self->cycles = 7;
}

void CPU_IRQ(CPU *self) {
    CPU_stack_push(self, (self->pc >> 8) & 0x00FF); // high byte first
    CPU_stack_push(self, self->pc & 0x00FF); // then low byte

    CPU_set_flag(self, U, true);
    CPU_set_flag(self, B, false);
    CPU_stack_push(self, self->sr);

    CPU_set_flag(self, I, true);

    byte low = CPU_read(self, 0xFFFE);
    byte high = CPU_read(self, 0xFFFF);
    self->pc = (high << 8) | low;

    self->cycles = 7;
}

// Addressing Modes

uint8 IMP(CPU *self) {
    return 0;
}

uint8 ACC(CPU *self) {
    return 0;
}

uint8 IMM(CPU *self) {
    self->addr_abs = self->pc++;
    return 0;
}

uint8 ZP(CPU *self) {
    self->addr_abs = CPU_read(self, self->pc++);
    self->addr_abs &= 0x00FF;
    return 0;
}

uint8 ZPX(CPU *self) {
    byte base = CPU_read(self, self->pc++);
    self->addr_abs = base + self->x;
    self->addr_abs &= 0x00FF;
    return 0;
}

uint8 ZPY(CPU *self) {
    byte base = CPU_read(self, self->pc++);
    self->addr_abs = base + self->y;
    self->addr_abs &= 0x00FF;
    return 0;
}

uint8 ABS(CPU *self) {
    byte low = CPU_read(self, self->pc++);
    byte high = CPU_read(self, self->pc++);
    self->addr_abs = (high << 8) | low;
    return 0;
}

uint8 ABSX(CPU *self) {
    byte low = CPU_read(self, self->pc++);
    byte high = CPU_read(self, self->pc++);
    uint16 base = (high << 8) | low;
    self->addr_abs = base + self->x;

    if ((self->addr_abs & 0xFF00) != (base & 0xFF00)) return 1;
    return 0;
}

uint8 ABSY(CPU *self) {
    byte low = CPU_read(self, self->pc++);
    byte high = CPU_read(self, self->pc++);
    uint16 base = (high << 8) | low;
    self->addr_abs = base + self->y;

    if ((self->addr_abs & 0xFF00) != (base & 0xFF00)) return 1;
    return 0;
}

uint8 IND(CPU *self) {
    byte low = CPU_read(self, self->pc++);
    byte high = CPU_read(self, self->pc++);
    uint16 ptr = (high << 8) | low;

    if (low == 0xFF) { // Hardware Bug: Cannot cross page boundary
        self->addr_abs = (CPU_read(self, ptr & 0xFF00) << 8) | CPU_read(self, ptr);
    } else {
        self->addr_abs = (CPU_read(self, ptr + 1) << 8) | CPU_read(self, ptr);
    }
    return 0;
}

uint8 IZX(CPU *self) {
    byte base = CPU_read(self, self->pc++);
    base += self->x;
    
    byte low = CPU_read(self, (uint16)(base & 0x00FF));
    byte high = CPU_read(self, (uint16)((base + 1) & 0x00FF));

    self->addr_abs = (high << 8) | low;
    return 0;
}

uint8 IZY(CPU *self) {
    byte base = CPU_read(self, self->pc++);

    byte low = CPU_read(self, (uint16)(base & 0x00FF));
    byte high = CPU_read(self, (uint16)((base + 1) & 0x00FF));

    self->addr_abs =((high << 8) | low);
    uint16 base_addr = self->addr_abs;
    self->addr_abs += self->y;

    if ((self->addr_abs & 0xFF00) != (base_addr & 0xFF00)) return 1;
    return 0;
}

uint8 REL(CPU *self) {
    self->addr_rel = CPU_read(self, self->pc++);
    if (self->addr_rel & 0x80) self->addr_rel |= 0xFF00;
    return 0;
}

// Instructions

uint8 LDA(CPU *self) {
    CPU_fetch(self);
    self->a = self->fetched;
    CPU_set_flag(self, Z, self->a == 0);
    CPU_set_flag(self, N, self->a & 0x80);
    return 1;
}

uint8 STA(CPU *self) {
    CPU_write(self, self->addr_abs, self->a);
    return 0;
}

uint8 ADC(CPU *self) {
    CPU_fetch(self);
    uint16 temp = (uint16)self->fetched + (uint16)self->a + (uint16)CPU_get_flag(self, C);

    CPU_set_flag(self, C, temp > 0xFF);
    CPU_set_flag(self, Z, (temp & 0x00FF) == 0);
    CPU_set_flag(self, N, temp & 0x80);
    CPU_set_flag(self, V, (~((uint16)self->a ^ self->fetched) & ((uint16)self->a ^ temp) & 0x80));

    self->a = temp & 0x00FF;
    return 1;
}

uint8 SBC(CPU *self) {
    CPU_fetch(self);
    uint16 val = self->fetched ^ 0x00FF;
    uint16 temp = val + self->a + (byte)CPU_get_flag(self, C);

    CPU_set_flag(self, C, temp > 0xFF);
    CPU_set_flag(self, Z, (temp & 0x00FF) == 0);
    CPU_set_flag(self, N, temp & 0x80);
    CPU_set_flag(self, V, (~((uint16)self->a ^ val) & ((uint16)self->a ^ temp) & 0x80));

    self->a = temp & 0x00FF;
    return 1;
}

uint8 AND(CPU *self) {
    CPU_fetch(self);
    uint16 temp = self->a & self->fetched;
    CPU_set_flag(self, Z, (temp & 0x00FF) == 0);
    CPU_set_flag(self, N, temp & 0x80);

    self->a = temp;
    return 1;
}

uint8 ORA(CPU *self) {
    CPU_fetch(self);
    uint16 temp = self->a | self->fetched;
    CPU_set_flag(self, Z, (temp & 0x00FF) == 0);
    CPU_set_flag(self, N, temp & 0x80);

    self->a = temp;
    return 1;
}

uint8 EOR(CPU *self) {
    CPU_fetch(self);
    uint16 temp = self->a ^ self->fetched;
    CPU_set_flag(self, Z, (temp & 0x00FF) == 0);
    CPU_set_flag(self, N, temp & 0x80);

    self->a = temp;
    return 1;
}

uint8 CMP(CPU *self) {
    CPU_fetch(self);
    uint16 temp = self->a - self->fetched;
    CPU_set_flag(self, C, self->a >= self->fetched);
    CPU_set_flag(self, Z, (temp & 0x00FF) == 0);
    CPU_set_flag(self, N, temp & 0x80);
    return 1;
}

uint8 ASL(CPU *self) {
    if (CPU_get_addr_mode(self) == &ACC) {
        byte temp = self->a << 1;
        CPU_set_flag(self, C, self->a & 0x80);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, temp & 0x80);
        self->a = temp;
    } else {
        CPU_fetch(self);
        byte temp = self->fetched << 1;
        CPU_set_flag(self, C, self->fetched & 0x80);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, temp & 0x80);
        CPU_write(self, self->addr_abs, temp);
    }
    return 0;
}

uint8 LSR(CPU *self) {
    if (CPU_get_addr_mode(self) == &ACC) {
        byte temp = self->a >> 1;
        CPU_set_flag(self, C, self->a & 0x01);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, 0);
        self->a = temp;
    } else {
        CPU_fetch(self);
        byte temp = self->fetched >> 1;
        CPU_set_flag(self, C, self->fetched & 0x01);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, 0);
        CPU_write(self, self->addr_abs, temp);
    }
    return 0;
}

uint8 ROL(CPU *self) {
    if (CPU_get_addr_mode(self) == &ACC) {
        byte temp = self->a << 1;
        temp |= CPU_get_flag(self, C);
        CPU_set_flag(self, C, self->a & 0x80);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, temp & 0x80);
        self->a = temp;
    } else {
        CPU_fetch(self);
        byte temp = self->fetched << 1;
        temp |= CPU_get_flag(self, C);
        CPU_set_flag(self, C, self->fetched & 0x80);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, temp & 0x80);
        CPU_write(self, self->addr_abs, temp);
    }
    return 0;
}

uint8 ROR(CPU *self) {
    if (CPU_get_addr_mode(self) == &ACC) {
        byte temp = self->a >> 1;
        temp |= CPU_get_flag(self, C) << 7;
        CPU_set_flag(self, C, self->a & 0x01);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, temp & 0x80);
        self->a = temp;
    } else {
        CPU_fetch(self);
        byte temp = self->fetched >> 1;
        temp |= CPU_get_flag(self, C) << 7;
        CPU_set_flag(self, C, self->fetched & 0x01);
        CPU_set_flag(self, Z, temp == 0);
        CPU_set_flag(self, N, temp & 0x80);
        CPU_write(self, self->addr_abs, temp);
    }
    return 0;
}

uint8 INC(CPU *self) {
    CPU_fetch(self);
    byte temp = (uint16)self->fetched + 1;
    CPU_write(self, self->addr_abs, temp);
    CPU_set_flag(self, Z, temp == 0);
    CPU_set_flag(self, N, temp & 0x80);
    return 0;
}

uint8 DEC(CPU *self) {
    CPU_fetch(self);
    byte temp = self->fetched - 1;
    CPU_write(self, self->addr_abs, temp);
    CPU_set_flag(self, Z, temp == 0);
    CPU_set_flag(self, N, temp & 0x80);
    return 0;
}

uint8 LDX(CPU *self) {
    CPU_fetch(self);
    self->x = self->fetched;
    CPU_set_flag(self, Z, self->x == 0);
    CPU_set_flag(self, N, self->x & 0x80);
    return 1;
}

uint8 STX(CPU *self) {
    CPU_write(self, self->addr_abs, self->x);
    return 0;
}

uint8 BCC(CPU *self) {
    if (CPU_get_flag(self, C) == 0x00) { // It's a branch
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 BCS(CPU *self) {
    if (CPU_get_flag(self, C) == 0x01) { // It's a branch
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 BEQ(CPU *self) {
    if (CPU_get_flag(self, Z) == 0x01) {
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 BIT(CPU *self) {
    CPU_fetch(self);
    byte temp = self->a & self->fetched;
    CPU_set_flag(self, Z, temp == 0);
    CPU_set_flag(self, V, self->fetched & 0x40);
    CPU_set_flag(self, N, self->fetched & 0x80);

    return 0;
}

uint8 BMI(CPU *self) {
    if (CPU_get_flag(self, N) == 0x01) { // It's a branch
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 BNE(CPU *self) {
    if (CPU_get_flag(self, Z) == 0x00) { // It's a branch
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 BPL(CPU *self) {
    if (CPU_get_flag(self, N) == 0x00) { // It's a branch
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 BRK(CPU *self) {
    CPU_stack_push(self, ((self->pc + 2) >> 8) & 0x00FF); // high byte first
    CPU_stack_push(self, ((self->pc + 2)) & 0x00FF); // then low byte

    CPU_set_flag(self, U, true);
    byte temp = self->sr;
    CPU_set_flag(self, B, true);
    CPU_stack_push(self, self->sr);
    self->sr = temp;

    CPU_set_flag(self, I, true);

    byte low = CPU_read(self, 0xFFFE);
    byte high = CPU_read(self, 0xFFFF);
    self->pc = (high << 8) | low;

    return 0;
}

uint8 BVC(CPU *self) {
    if (CPU_get_flag(self, V) == 0x00) { // It's a branch
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 BVS(CPU *self) {
    if (CPU_get_flag(self, V) == 0x01) { // It's a branch
        self->pc += self->addr_rel;
        return 1;
    } else {
        return 0;
    }
}

uint8 CLC(CPU *self) {
    CPU_set_flag(self, C, false);
    return 0;
}

uint8 CLD(CPU *self) {
    CPU_set_flag(self, D, false);
    return 0;
}

uint8 CLI(CPU *self) {
    CPU_set_flag(self, I, false);
    return 0;
}

uint8 CLV(CPU *self) {
    CPU_set_flag(self, V, false);
    return 0;
}

uint8 CPX(CPU *self) {
    CPU_fetch(self);
    CPU_set_flag(self, C, self->x >= self->fetched);
    CPU_set_flag(self, Z, self->x == self->fetched);

    byte temp = self->x - self->fetched;
    CPU_set_flag(self, N, temp & 0x80);
    return 0;
}

uint8 CPY(CPU *self) {
    CPU_fetch(self);
    CPU_set_flag(self, C, self->y >= self->fetched);
    CPU_set_flag(self, Z, self->y == self->fetched);

    byte temp = self->y - self->fetched;
    CPU_set_flag(self, N, temp & 0x80);
    return 0;
}

uint8 DEX(CPU *self) {
    self->x -= 1;
    CPU_set_flag(self, Z, self->x == 0);
    CPU_set_flag(self, N, self->x & 0x80);
    return 0;
}

uint8 DEY(CPU *self) {
    self->y -= 1;
    CPU_set_flag(self, Z, self->y == 0);
    CPU_set_flag(self, N, self->y & 0x80);
    return 0;
}

uint8 INX(CPU *self) {
    self-> x += 1;
    CPU_set_flag(self, Z, self->x == 0);
    CPU_set_flag(self, N, self->x & 0x80);
    return 0;
}

uint8 INY(CPU *self) {
    self-> y += 1;
    CPU_set_flag(self, Z, self->y == 0);
    CPU_set_flag(self, N, self->y & 0x80);
    return 0;
}

uint8 JMP(CPU *self) {
    self->pc = self->addr_abs;
    return 0;
}

uint8 JSR(CPU *self) {
    CPU_stack_push(self, ((self->pc - 1) >> 8) & 0x00FF); // high byte first
    CPU_stack_push(self, ((self->pc - 1)) & 0x00FF); // then low byte
    self->pc = self->addr_abs;
    return 0;
}

uint8 NOP(CPU *self) {
    return 0;
}

uint8 PHA(CPU *self) {
    CPU_stack_push(self, self->a);
    return 0;
}

uint8 PHP(CPU *self) {
    byte temp = self->sr;
    CPU_set_flag(self, B, true);
    CPU_stack_push(self, self->sr);
    self->sr = temp;
    return 0;
}

uint8 PLA(CPU *self) {
    self->a = CPU_stack_pop(self);
    CPU_set_flag(self, Z, self->a == 0);
    CPU_set_flag(self, N, self->a & 0x80);
    return 0;
}

uint8 PLP(CPU *self) {
    // byte temp = CPU_stack_pop(self);
    // self->sr = (temp & 0xEF) | 0x20;
    self->sr = CPU_stack_pop(self);
    CPU_set_flag(self, U, true);
    CPU_set_flag(self, B, false);
    return 0;
}

uint8 RTI(CPU *self) {
    self->sr = CPU_stack_pop(self);
    CPU_set_flag(self, U, true);
    CPU_set_flag(self, B, false);

    byte low = CPU_stack_pop(self);
    byte high = CPU_stack_pop(self);
    self->pc = (high << 8) | low;
    return 0;
}

uint8 RTS(CPU *self) {
    byte low = CPU_stack_pop(self);
    byte high = CPU_stack_pop(self);
    self->pc = ((high << 8) | low) + 1;
    return 0;
}

uint8 SEC(CPU *self) {
    CPU_set_flag(self, C, true);
    return 0;
}

uint8 SED(CPU *self) {
    CPU_set_flag(self, D, true);
    return 0;
}

uint8 SEI(CPU *self) {
    CPU_set_flag(self, I, true);
    return 0;
}

uint8 LDY(CPU *self) {
    CPU_fetch(self);
    self->y = self->fetched;
    CPU_set_flag(self, Z, self->y == 0);
    CPU_set_flag(self, N, self->y & 0x80);
    return 1;
}

uint8 STY(CPU *self) {
    CPU_write(self, self->addr_abs, self->y);
    return 0;
}

uint8 TAX(CPU *self) {
    self->x = self->a;
    CPU_set_flag(self, Z, self->x == 0);
    CPU_set_flag(self, N, self->x & 0x80);
    return 0;
}

uint8 TAY(CPU *self) {
    self->y = self->a;
    CPU_set_flag(self, Z, self->y == 0);
    CPU_set_flag(self, N, self->y & 0x80);
    return 0;
}

uint8 TYA(CPU *self) {
    self->a = self->y;
    CPU_set_flag(self, Z, self->a == 0);
    CPU_set_flag(self, N, self->a & 0x80);
    return 0;
}

uint8 TSX(CPU *self) {
    self->x = self->sp;
    CPU_set_flag(self, Z, self->x == 0);
    CPU_set_flag(self, N, self->x & 0x80);
    return 0;
}

uint8 TXA(CPU *self) {
    self->a = self->x;
    CPU_set_flag(self, Z, self->a == 0);
    CPU_set_flag(self, N, self->a & 0x80);
    return 0;
}

uint8 TXS(CPU *self) {
    self->sp = self->x;
    CPU_set_flag(self, Z, self->sp == 0);
    CPU_set_flag(self, N, self->sp & 0x80);
    return 0;
}
