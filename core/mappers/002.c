#include "mapper.h"

void mapper002_scanline(Mapper *self) {}

Mapper_Return_Type mapper002_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    if (0x8000 <= addr && addr <= 0xBFFF)  {
        Mapper002_State *st = (Mapper002_State *)self->state;
        *mapped_addr = (st->bank_select % self->n_prg_banks) * 0x4000 + (addr & 0x3FFF);
        return PRG_ROM;
    } else if (0xC000 <= addr && addr <= 0xFFFF) {
        *mapped_addr = (self->n_prg_banks - 1) * 0x4000 + (addr & 0x3FFF);
        return PRG_ROM;
    }

    return NONE;
}

Mapper_Return_Type mapper002_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data) {
    if (0x8000 <= addr)  {
        Mapper002_State *st = (Mapper002_State *)self->state;
        st->bank_select = data & 0x07;
    }
    return NONE;
}

Mapper_Return_Type mapper002_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    if (addr <= 0x1FFF) {
        *mapped_addr = addr; 
        return (self->n_chr_banks == 0) ? CHR_RAM : CHR_ROM;
    }

    return NONE;
}

Mapper_Return_Type mapper002_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    if (addr <= 0x1FFF && self->n_chr_banks == 0) {
        *mapped_addr = addr; 
        return CHR_RAM;
    }

    return NONE;
}

void mapper002_reset(Mapper *self) {
    Mapper002_State *st = (Mapper002_State *)self->state;
    st->bank_select = 0x00;
}