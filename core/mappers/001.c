// AI gen
#include "mapper.h"
#include <stdlib.h>

// 1. RESET: Initialize registers to power-on/reset defaults
void mapper001_reset(Mapper *self) {
    Mapper001_State *st = (Mapper001_State *)self->state;
    st->shift_register = 0x10; // 1 is the "sentinel" bit to detect 5 shifts
    st->control_reg = 0x1C;    // Default: PRG mode 3 (fixed last bank)
    st->chr_bank_0 = 0;
    st->chr_bank_1 = 0;
    st->prg_bank = 0;
}

Mapper_Return_Type mapper001_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data) {
    // Handle PRG RAM (usually $6000-$7FFF)
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        *mapped_addr = addr & 0x1FFF;
        return PRG_RAM;
    }

    if (addr < 0x8000) return NONE;

    Mapper001_State *st = (Mapper001_State *)self->state;

    // Reset shift register if Bit 7 is set
    if (data & 0x80) {
        st->shift_register = 0x10;
        st->control_reg |= 0x0C; 
    } else {
        bool complete = st->shift_register & 0x01;
        st->shift_register >>= 1;
        st->shift_register |= (data & 0x01) << 4;

        if (complete) {
            uint8 target = (addr >> 13) & 0x03;
            uint8 val = st->shift_register; // The 5-bit value collected
            
            switch (target) {
                case 0: // Control Register
                    st->control_reg = val;
                    switch (st->control_reg & 0x03) {
                        case 0: self->mirroring_mode = Single_Screen_0; break;
                        case 1: self->mirroring_mode = Single_Screen_1; break;
                        case 2: self->mirroring_mode = Vertical;        break;
                        case 3: self->mirroring_mode = Horizontal;      break;
                    }
                    break;
                case 1: st->chr_bank_0 = val; break;
                case 2: st->chr_bank_1 = val; break;
                case 3: st->prg_bank   = val; break;
            }
            st->shift_register = 0x10;
        }
    }
    // Return NONE because we are updating internal registers, not PRG memory
    return NONE; 
}

Mapper_Return_Type mapper001_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    // Handle PRG RAM ($6000-$7FFF)
    if (addr >= 0x6000 && addr <= 0x7FFF) {
        *mapped_addr = addr & 0x1FFF;
        return PRG_RAM;
    }

    if (addr < 0x8000) return NONE;

    Mapper001_State *st = (Mapper001_State *)self->state;
    uint8 prg_mode = (st->control_reg >> 2) & 0x03;

    if (prg_mode <= 1) {
        // 32KB Mode
        *mapped_addr = (st->prg_bank & 0x0E) * 0x4000 + (addr & 0x7FFF);
    } else if (prg_mode == 2) {
        // Fix first bank at $8000, switchable at $C000
        if (addr <= 0xBFFF) *mapped_addr = 0x0000 + (addr & 0x3FFF);
        else *mapped_addr = (st->prg_bank & 0x0F) * 0x4000 + (addr & 0x3FFF);
    } else {
        // Switchable at $8000, fix last bank at $C000
        if (addr <= 0xBFFF) *mapped_addr = (st->prg_bank & 0x0F) * 0x4000 + (addr & 0x3FFF);
        else *mapped_addr = (self->n_prg_banks - 1) * 0x4000 + (addr & 0x3FFF);
    }

    return PRG_ROM;
}

Mapper_Return_Type mapper001_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    if (addr > 0x1FFF) return NONE;

    Mapper001_State *st = (Mapper001_State *)self->state;
    Mapper_Return_Type type = (self->n_chr_banks == 0) ? CHR_RAM : CHR_ROM;

    if (!(st->control_reg & 0x10)) {
        // 8KB CHR Mode
        *mapped_addr = (st->chr_bank_0 & 0x1E) * 0x1000 + (addr & 0x1FFF);
    } else {
        // 4KB CHR Mode
        if (addr <= 0x0FFF) *mapped_addr = (uint32)st->chr_bank_0 * 0x1000 + (addr & 0x0FFF);
        else *mapped_addr = (uint32)st->chr_bank_1 * 0x1000 + (addr & 0x0FFF);
    }

    return type;
}

Mapper_Return_Type mapper001_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    // We only return CHR_RAM if the cartridge was initialized with 0 CHR ROM banks
    if (addr <= 0x1FFF && self->n_chr_banks == 0) {
        *mapped_addr = addr; 
        return CHR_RAM;
    }
    
    // Most MMC1 games with CHR ROM will return NONE here as you can't write to ROM
    return NONE;
}