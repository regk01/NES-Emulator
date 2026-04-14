// NROM
#include "mapper.h"

Mapper_Return_Type mapper000_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        *mapped_addr = addr & 0x1FFF;
        return PRG_RAM;
    }

    if (addr >= 0x8000) {
        *mapped_addr = addr & (self->n_prg_banks > 1 ? 0x7FFF : 0x3FFF);
        return PRG_ROM;
    }

    return NONE;
}

Mapper_Return_Type mapper000_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data) {
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        *mapped_addr = addr & 0x1FFF;
        return PRG_RAM;
    }
    return NONE;
}

Mapper_Return_Type mapper000_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    if (addr <= 0x1FFF) {
        *mapped_addr = addr;
        return (self->n_chr_banks == 0) ? CHR_RAM : CHR_ROM;
    }
    return NONE;
}

Mapper_Return_Type mapper000_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    if (addr <= 0x1FFF && self->n_chr_banks == 0) {
        *mapped_addr = addr;
        return CHR_RAM;
    }
    return NONE;
}

void mapper000_reset(Mapper *self) {
    // nothing to reset for this mapper
}