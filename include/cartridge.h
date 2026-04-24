#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include "types.h"
#include "mapper.h"

typedef struct NES_CORE NES_CORE;

typedef struct Cartridge {
    NES_CORE *core;

    byte *prg_rom;
    byte *chr_rom;
    byte *prg_ram;
    byte is_chr_ram;
    Mapper *mapper;

    Mirroring_Mode mirroring_mode;

    uint8 mapper_id;
    uint8 n_prg_banks;
    uint8 n_chr_banks;
} Cartridge;

bool Cartridge_cpu_read(Cartridge *self, uint16 addr, byte *data);
bool Cartridge_cpu_write(Cartridge *self, uint16 addr,  byte val);

bool Cartridge_ppu_read(Cartridge *self, uint16 addr, byte *data);
bool Cartridge_ppu_write(Cartridge *self, uint16 addr, byte val);

void Cartridge_load_rom(Cartridge *self, char *name);
void Cartridge_free_rom(Cartridge *self);

void Cartridge_connect_core(Cartridge *self, NES_CORE *core);

#endif