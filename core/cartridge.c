#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cartridge.h"
#include "core.h"

void Cartridge_connect_core(Cartridge *self, NES_CORE *core) {
    self->core = core;
    self->mapper->core = core;
}

bool Cartridge_cpu_read(Cartridge *self, uint16 addr, byte *data) {
    uint32 mapped_addr = 0;
    Mapper_Return_Type type = self->mapper->cpuMapRead(self->mapper, addr, &mapped_addr);

    switch (type) {
        case PRG_ROM:
            *data = self->prg_rom[mapped_addr];
            return true;
        case PRG_RAM:
            if (self->prg_ram) {
                *data = self->prg_ram[mapped_addr];
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool Cartridge_cpu_write(Cartridge *self, uint16 addr, byte val) {
    uint32 mapped_addr = 0;
    Mapper_Return_Type type = self->mapper->cpuMapWrite(self->mapper, addr, &mapped_addr, val);

    if (type == PRG_RAM && self->prg_ram) {
        self->prg_ram[mapped_addr] = val;
        return true;
    }
    return false;
}

bool Cartridge_ppu_read(Cartridge *self, uint16 addr, byte *data) {
    uint32 mapped_addr = 0;
    Mapper_Return_Type type = self->mapper->ppuMapRead(self->mapper, addr, &mapped_addr);

    switch (type) {
        case CHR_ROM:
            *data = self->chr_rom[mapped_addr];
            return true;
        case CHR_RAM:
            if (self->is_chr_ram) {
                *data = self->chr_rom[mapped_addr];
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool Cartridge_ppu_write(Cartridge *self, uint16 addr, byte val) {
    uint32 mapped_addr = 0;
    Mapper_Return_Type type = self->mapper->ppuMapWrite(self->mapper, addr, &mapped_addr);

    if (type == CHR_RAM && self->is_chr_ram) {
        self->chr_rom[mapped_addr] = val;
        return true;
    }

    return false;
}

struct InesHeader {
    byte nes_constant[4];
    byte prg_rom_size; // 16KB units
    byte chr_rom_size; // 8KB units
    union {
        struct {
            byte name_table_arrangement: 1;
            byte has_prg_ram: 1;
            byte has_trainer: 1;
            byte alternative_name_table_arrangement: 1;
            byte lower_mapper_number: 4;
        };
        byte raw;
    } flag_6;
    union {
        struct {
            byte VS_unisystem: 1;
            byte playChoice_10: 1;
            byte isNES_2_0: 2;
            byte upper_mapper_number: 4;
        };
        byte raw;
    } flag_7;
    byte prg_ram_size;
    byte tv_system;
    byte ignored;
    byte padding[5];
};

void Cartridge_load_rom(Cartridge *self, char *name) {
    FILE *file = fopen(name, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open %s\n", name);
        exit(EXIT_FAILURE);
    }
    struct InesHeader header;
    fread(&header, sizeof(struct InesHeader), 1, file);
    if (memcmp(header.nes_constant, "NES\x1a", 4) != 0) {
        fprintf(stderr, "Invalid iNES Header! Found: %02X %02X %02X %02X\n", 
                header.nes_constant[0], header.nes_constant[1], 
                header.nes_constant[2], header.nes_constant[3]);
        exit(EXIT_FAILURE);
    }
    if (header.chr_rom_size == 0) {
        // CHR RAM is almost always 8KB (1 unit)
        self->n_chr_banks = 1; 
        self->is_chr_ram = true; // You should add this bool to your Cartridge struct
    } else {
        self->n_chr_banks = header.chr_rom_size;
        self->is_chr_ram = false;
    }
    self->n_prg_banks = header.prg_rom_size;
    self->mirroring_mode = header.flag_6.name_table_arrangement ? Vertical: Horizontal; // TODO: use alternative nametable arrangement for mappers
    // Note ^^ this is just what the initial file says. it isn't used after. ppu depends on the one in the mapper
    // because some mappers can change it dynamically.
    self->mapper_id = ((header.flag_7.upper_mapper_number) << 4) | (header.flag_6.lower_mapper_number);

    if (header.flag_6.has_trainer) fseek(file, 512, SEEK_CUR);
    self->prg_rom = malloc(16384 * self->n_prg_banks);
    if (self->prg_rom == NULL) {
        fprintf(stderr, "Failed to allocate prg_rom buffer\n");
        exit(EXIT_FAILURE);
    }
    self->chr_rom = malloc(8192 * self->n_chr_banks);
    if (self->chr_rom == NULL) {
        fprintf(stderr, "Failed to allocate chr_rom buffer\n");
        exit(EXIT_FAILURE);
    }
    self->prg_ram = calloc(1, 8192);
    if (self->prg_ram == NULL) {
        fprintf(stderr, "Failed to allocate prg_ram buffer\n");
        exit(EXIT_FAILURE);
    }
    fread(self->prg_rom, 16384 * self->n_prg_banks, 1, file);
    if (self->is_chr_ram) {
        // Clear the RAM so it doesn't contain garbage data
        memset(self->chr_rom, 0, 8192 * self->n_chr_banks);
    } else {
        // Only read from file if it's actually ROM
        fread(self->chr_rom, 8192 * self->n_chr_banks, 1, file);
    }

    self->mapper = Mapper_create(self->mapper_id, self->n_prg_banks, self->is_chr_ram ? 0: self->n_chr_banks, self->mirroring_mode);
    fclose(file);
}

void Cartridge_free_rom(Cartridge *self) {
    free(self->chr_rom);
    free(self->prg_rom);
    free(self->prg_ram);

    Mapper_destroy(self->mapper);
}