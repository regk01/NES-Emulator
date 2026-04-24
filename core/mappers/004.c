#include "mapper.h"
#include "core.h"

void mapper004_update_bank_registers(Mapper *self) {
    Mapper004_State *st = (Mapper004_State *)self->state;
    byte prg_mask = (self->n_prg_banks * 2);
    byte chr_mask = (self->n_chr_banks * 2);

    if (st->chr_inversion) {
        st->chr_banks[0] = (st->r[2] % chr_mask) * 0x400;
        st->chr_banks[1] = (st->r[3] % chr_mask) * 0x400;
        st->chr_banks[2] = (st->r[4] % chr_mask) * 0x400;
        st->chr_banks[3] = (st->r[5] % chr_mask) * 0x400;
        st->chr_banks[4] = ((st->r[0] % chr_mask) & 0xFE) * 0x400;
        st->chr_banks[5] = ((st->r[0] % chr_mask) & 0xFE) * 0x400 + 0x400;
        st->chr_banks[6] = ((st->r[1] % chr_mask) & 0xFE) * 0x400;
        st->chr_banks[7] = ((st->r[1] % chr_mask) & 0xFE) * 0x400 + 0x400;
    } else {
        st->chr_banks[0] = ((st->r[0] % chr_mask) & 0xFE) * 0x400;
        st->chr_banks[1] = ((st->r[0] % chr_mask) & 0xFE) * 0x400 + 0x400;
        st->chr_banks[2] = ((st->r[1] % chr_mask) & 0xFE) * 0x400;
        st->chr_banks[3] = ((st->r[1] % chr_mask) & 0xFE) * 0x400 + 0x400;
        st->chr_banks[4] = (st->r[2] % chr_mask) * 0x400;
        st->chr_banks[5] = (st->r[3] % chr_mask) * 0x400;
        st->chr_banks[6] = (st->r[4] % chr_mask) * 0x400;
        st->chr_banks[7] = (st->r[5] % chr_mask) * 0x400;
    }

    if (st->prg_rom_bank_mode) {
        st->prg_banks[0] = ((st->r[6] % prg_mask) & 0x3F) * 0x2000;
        st->prg_banks[2] = (self->n_prg_banks * 2 - 2) * 0x2000;
    } else {
        st->prg_banks[0] = (self->n_prg_banks * 2 - 2) * 0x2000;
        st->prg_banks[2] = ((st->r[6] % prg_mask) & 0x3F) * 0x2000;
    }

    st->prg_banks[1] = ((st->r[7] % prg_mask) & 0x3F) * 0x2000;
    st->prg_banks[3] = (self->n_prg_banks * 2 - 1) * 0x2000;
}

void mapper004_scanline(Mapper *self) {
    Mapper004_State *st = (Mapper004_State *)self->state;
    if (st->irq_counter == 0) {
        st->irq_counter = st->irq_latch;
    } else {
        st->irq_counter--;
    }
    if (st->irq_counter == 0 && st->irq_enabled) self->core->irq_pending = true;
}

Mapper_Return_Type mapper004_cpuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    Mapper004_State *st = (Mapper004_State *)self->state;

    if (0x6000 <= addr && addr <= 0x7FFF) {
        *mapped_addr = addr & 0x1FFF;
        return PRG_RAM;
    } else if (0x8000 <= addr && addr <= 0x9FFF) {
        *mapped_addr = st->prg_banks[0] + (addr & 0x1FFF);
        return PRG_ROM;
    } else if (0xA000 <= addr && addr <= 0xBFFF) {
        *mapped_addr = st->prg_banks[1] + (addr & 0x1FFF);
        return PRG_ROM;
    } else if (0xC000 <= addr && addr <= 0xDFFF) {
        *mapped_addr = st->prg_banks[2] + (addr & 0x1FFF);
        return PRG_ROM;
    } else if (0xE000 <= addr && addr <= 0xFFFF) {
        *mapped_addr = st->prg_banks[3] + (addr & 0x1FFF);
        return PRG_ROM;
    }

    return NONE;
}

Mapper_Return_Type mapper004_cpuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr, uint8 data) {
    byte is_odd = addr & 0x01;
    Mapper004_State *st = (Mapper004_State *)self->state;

    if (addr >= 0x6000 && addr <= 0x7FFF) {
        *mapped_addr = addr & 0x1FFF;
        return PRG_RAM;
    } else if (0x8000 <= addr && addr <= 0x9FFF) {
        if (!is_odd) { // bank select
            st->target_reg = data & 0x07;
            st->prg_rom_bank_mode = (data & 0x40) > 0;
            st->chr_inversion = (data & 0x80) > 0;
        }
        else { // bank data
            st->r[st->target_reg] = data;
            mapper004_update_bank_registers(self);
        }
    } else if (0xA000 <= addr && addr <= 0xBFFF) {
        if (!is_odd) { // name table arrangement
            st->name_table_arrangement = (data & 0x01) ? Horizontal: Vertical;
        }
        else { // prg ram protect
            // Do nothing cus not needed according to nesdev said
        }
    } else if (0xC000 <= addr && addr <= 0xDFFF) {
        if (!is_odd) { // irq latch
            st->irq_latch = data;
        }
        else { // irq reload
            st->irq_counter = 0;
        }
    } else if (0xE000 <= addr && addr <= 0xFFFF) {
        if (!is_odd) {
            st->irq_enabled = false;
            self->core->irq_pending = false;
        } else {
            st->irq_enabled = true;
        }
    }

    return NONE;
}

Mapper_Return_Type mapper004_ppuMapRead(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    Mapper004_State *st = (Mapper004_State *)self->state;

    if (0x0000 <= addr && addr <= 0x03FF) {
        *mapped_addr = st->chr_banks[0] + (addr & 0x3FF);
        return CHR_ROM;
    } else if (0x0400 <= addr && addr <= 0x07FF) {
        *mapped_addr = st->chr_banks[1] + (addr & 0x3FF);
        return CHR_ROM;
    } else if (0x0800 <= addr && addr <= 0x0BFF) {
        *mapped_addr = st->chr_banks[2] + (addr & 0x3FF);
        return CHR_ROM;
    } else if (0x0C00 <= addr && addr <= 0x0FFF) {
        *mapped_addr = st->chr_banks[3] + (addr & 0x3FF);
        return CHR_ROM;
    } else if (0x1000 <= addr && addr <= 0x13FF) {
        *mapped_addr = st->chr_banks[4] + (addr & 0x3FF);
        return CHR_ROM;
    } else if (0x1400 <= addr && addr <= 0x17FF) {
        *mapped_addr = st->chr_banks[5] + (addr & 0x3FF);
        return CHR_ROM;
    } else if (0x1800 <= addr && addr <= 0x1BFF) {
        *mapped_addr = st->chr_banks[6] + (addr & 0x3FF);
        return CHR_ROM;
    } else if (0x1C00 <= addr && addr <= 0x1FFF) {
        *mapped_addr = st->chr_banks[7] + (addr & 0x3FF);
        return CHR_ROM;
    }

    return NONE;
}

Mapper_Return_Type mapper004_ppuMapWrite(Mapper *self, uint16 addr, uint32 *mapped_addr) {
    return NONE;
}

void mapper004_reset(Mapper *self) {
    Mapper004_State *st = (Mapper004_State *)self->state;

    st->target_reg = 0x00;
    for (int32 i = 0; i < 8; i++) st->r[i] = 0;
    st->prg_rom_bank_mode = false;
    st->chr_inversion = false;

    st->name_table_arrangement = Horizontal;

    st->irq_latch = 0;
    st->irq_counter = 0;
    st->irq_enabled = false;

    for (int32 i = 0; i < 8; i++) st->chr_banks[i] = 0;
    for (int32 i = 0; i < 4; i++) st->prg_banks[i] = 0;

	st->prg_banks[0] = 0 * 0x2000;
	st->prg_banks[1] = 1 * 0x2000;
	st->prg_banks[2] = (self->n_prg_banks * 2 - 2) * 0x2000;
	st->prg_banks[3] = (self->n_prg_banks * 2 - 1) * 0x2000;
}