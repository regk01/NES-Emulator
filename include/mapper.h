#ifndef MAPPER_H
#define MAPPER_H

#include "types.h"

typedef enum Mirroring_Mode {
    Horizontal = 0,
    Vertical = 1,
    Single_Screen_0 = 2,
    Single_Screen_1 = 3,
    FourScreen = 4
} Mirroring_Mode;

typedef enum Mapper_Return_Type {
    NONE = 0,
    PRG_ROM = 1,
    PRG_RAM = 2,
    CHR_ROM = 3,
    CHR_RAM = 4,
} Mapper_Return_Type;

typedef struct Mapper {
    uint8 id;
    uint8 n_prg_banks;
    uint8 n_chr_banks;
    Mirroring_Mode mirroring_mode;

    Mapper_Return_Type (*cpuMapRead)(struct Mapper *self, uint16 addr, uint32 *mapped_addr);
    Mapper_Return_Type (*cpuMapWrite)(struct Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
    Mapper_Return_Type (*ppuMapRead)(struct Mapper *self, uint16 addr, uint32 *mapped_addr);
    Mapper_Return_Type (*ppuMapWrite)(struct Mapper *self, uint16 addr, uint32 *mapped_addr);
    void (*reset)(struct Mapper *self);

    void *state;
} Mapper;

Mapper *Mapper_create(uint8 id, uint8 n_prg_banks, uint8 n_chr_banks, Mirroring_Mode mirroring_mode);

// NROM
Mapper_Return_Type mapper000_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper000_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
Mapper_Return_Type mapper000_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper000_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr);
void mapper000_reset(Mapper *self);

typedef struct {
    uint8 shift_register;
    uint8 control_reg;
    uint8 chr_bank_0;
    uint8 chr_bank_1;
    uint8 prg_bank;
} Mapper001_State;

Mapper_Return_Type mapper001_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper001_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
Mapper_Return_Type mapper001_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper001_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr);
void mapper001_reset(Mapper *self);

#endif