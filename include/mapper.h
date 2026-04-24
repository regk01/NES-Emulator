#ifndef MAPPER_H
#define MAPPER_H

#include "types.h"

typedef struct NES_CORE NES_CORE;

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
    NES_CORE *core;
    uint8 id;
    uint8 n_prg_banks;
    uint8 n_chr_banks;
    Mirroring_Mode mirroring_mode;

    Mapper_Return_Type (*cpuMapRead)(struct Mapper *self, uint16 addr, uint32 *mapped_addr);
    Mapper_Return_Type (*cpuMapWrite)(struct Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
    Mapper_Return_Type (*ppuMapRead)(struct Mapper *self, uint16 addr, uint32 *mapped_addr);
    Mapper_Return_Type (*ppuMapWrite)(struct Mapper *self, uint16 addr, uint32 *mapped_addr);
    void (*scanline)(struct Mapper *self);
    void (*reset)(struct Mapper *self);

    void *state;
} Mapper;

Mapper *Mapper_create(uint8 id, uint8 n_prg_banks, uint8 n_chr_banks, Mirroring_Mode mirroring_mode);
void Mapper_destroy(Mapper *self);

// NROM
Mapper_Return_Type mapper000_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper000_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
Mapper_Return_Type mapper000_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper000_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr);
void mapper000_scanline(Mapper *self);
void mapper000_reset(Mapper *self);

// MMC1
typedef struct {
    byte shift_register;
    byte control_reg;
    byte chr_bank_0;
    byte chr_bank_1;
    byte prg_bank;
} Mapper001_State;

Mapper_Return_Type mapper001_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper001_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
Mapper_Return_Type mapper001_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper001_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr);
void mapper001_scanline(Mapper *self);
void mapper001_reset(Mapper *self);

// UxROM
typedef struct {
    byte bank_select;
} Mapper002_State;

Mapper_Return_Type mapper002_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper002_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
Mapper_Return_Type mapper002_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper002_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr);
void mapper002_scanline(Mapper *self);
void mapper002_reset(Mapper *self);

// MMC3
typedef struct {
    byte target_reg;
    byte r[8];
    byte prg_rom_bank_mode;
    byte chr_inversion;

    Mirroring_Mode name_table_arrangement;

    byte irq_latch;
    byte irq_counter;
    bool irq_enabled;

    uint32 chr_banks[8];
    uint32 prg_banks[4];
} Mapper004_State;

Mapper_Return_Type mapper004_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper004_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data);
Mapper_Return_Type mapper004_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr);
Mapper_Return_Type mapper004_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr);
void mapper004_scanline(Mapper *self);
void mapper004_reset(Mapper *self);

#endif