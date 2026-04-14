#include <stdio.h>
#include "ppu.h"
#include "core.h"
#include "cartridge.h"
#include "mapper.h"
 
// checks for ppu name table idx and accounts for different scrolling types
byte PPU_get_name_table_idx(PPU *self, uint16 addr) {
    switch(self->core->cart.mapper->mirroring_mode) {
        case Horizontal:
            return (addr & 0x0800) ? 1 : 0;
        case Vertical:
            return (addr & 0x0400) ? 1 : 0;
        case Single_Screen_0:
            return 0;
        case Single_Screen_1:
            return 1;
        case FourScreen:
            return (addr & 0x0C00) >> 10; // 0 = $2000, 1 = $2400, 2 = $2800, 3 = $2C00;
        default:
            return 0;
    }
}

void PPU_update_NMI(PPU *self) {
    if (self->control.enable_vblank_nmi && self->status.vblank) {
        self->core->nmi_pending = true;
    } else {
        self->core->nmi_pending = false;
    }
}

byte PPU_cpu_read(PPU *self, uint16 addr) {
    addr &= 0x0007;
    byte data = 0x00;
    switch (addr) {
        case 0x0000: // Control Register
            break;
        case 0x0001: // Mask Register
            break;
        case 0x0002: // Status Register
            data = (self->status.reg & 0xE0) | (self->ppu_data_buffer & 0x1F);
            self->status.vblank = 0;
            PPU_update_NMI(self);
            self->w = 0;
            break;
        case 0x0003: // OAM Address Register
            break;
        case 0x0004: // OAM Data Register
            if (1 <= self->cycle && self->cycle <= 64) {
                data = 0xFF;
            } else if (65 <= self->cycle && self->cycle <= 256) {
                data = self->OAM_latch;
            } else {
                data = self->OAM.raw[self->oam_addr];
            }
            break;
        case 0x0005: // Scroll Position Register
            break;
        case 0x0006: // PPU Address Register
            break;
        case 0x0007: // PPU Data Register
            data = self->ppu_data_buffer;
            self->ppu_data_buffer = PPU_read(self, self->v.reg & 0x3FFF);
            if (self->v.reg >= 0x3F00) {
                data = self->ppu_data_buffer;
                self->ppu_data_buffer = PPU_read(self, (self->v.reg & 0x3FFF) - 0x1000);
            }

            self->v.reg += (self->control.vram_increment ? 32: 1);
            break;
    }

    return data;
}

void PPU_cpu_write(PPU *self, uint16 addr, byte val) {
    addr &= 0x0007;
    switch (addr) {
        case 0x0000: // Control Register
            byte curr_control = self->control.enable_vblank_nmi;
            self->control.reg = val;
            if (!curr_control) PPU_update_NMI(self);
            self->t.name_table_select = (self->control.nametable_y << 1) | self->control.nametable_x;
            break;
        case 0x0001: // Mask Register
            self->mask.reg = val;
            break;
        case 0x0002: // Status Register
            break;
        case 0x0003: // OAM Address Register
            self->oam_addr = val;
            break;
        case 0x0004: // OAM Data Register
            self->OAM.raw[self->oam_addr] = val;
            self->oam_addr++;
            break;
        case 0x0005: // Scroll Position Register
            if (self->w == 0x00) {
                self->x = val & 0x07;
                self->t.coarse_x = val >> 3;
                self->w = 1;
            } else {
                self->t.fine_y = val & 0x07;
                self->t.coarse_y = val >> 3;
                self->w = 0;
            }
            break;
        case 0x0006: // PPU Address Register
            if (self->w == 0x00) {
                self->t.reg = (uint16)((val & 0x3F) << 8) | (self->t.reg & 0x00FF);
                self->t.reg = (self->t.reg & ~(1 << 14));
                self->w = 1;
            } else {
                self->t.reg = (self->t.reg & 0xFF00) | (uint16)val;
                self->v.reg = self->t.reg;
                self->w = 0;
            }
            break;
        case 0x0007: // PPU Data Register
            PPU_write(self, self->v.reg, val);
            self->v.reg += (self->control.vram_increment ? 32: 1);
            break;
    }
}

byte PPU_read(PPU *self, uint16 addr) {
    byte data = 0x00;
    addr &= 0x3FFF;
    if (Cartridge_ppu_read(&(self->core->cart), addr, &data)) {
        // Let cartridge check first
    } else if (0x2000 <= addr && addr <= 0x3EFF) { // Nametables
        // I don't think the code should reach here for four screen mode. The mapper should catch that
        // and return if needed. Keep in mind though
        uint16 normalized_addr = 0x2000 | (addr & 0x0FFF);
        data = self->name_tables[PPU_get_name_table_idx(self, normalized_addr)][normalized_addr & 0x03FF];
    } else if (0x3F00 <= addr && addr <= 0x3FFF) { // Palette ram indexes
        addr &= 0x001F;
        if ((addr & 0x03) == 0x00) addr &= 0x000F;
        data = self->palette_RAM_idxs[addr] & (self->mask.greyscale ? 0x30: 0x3F);
    }

    return data;
}

void PPU_write(PPU *self, uint16 addr, byte val) {
    addr &= 0x3FFF;
    if (Cartridge_ppu_write(&(self->core->cart), addr, val)) {
        // Let cartridge check first
    } else if (0x2000 <= addr && addr <= 0x3EFF) { // Nametables
        // I don't think the code should reach here for four screen mode. The mapper should catch that
        // and return if needed. Keep in mind though
        addr &= 0x0FFF;
        uint16 normalized_addr = 0x2000 | (addr & 0x0FFF);
        self->name_tables[PPU_get_name_table_idx(self, normalized_addr)][normalized_addr & 0x3FF] = val;
    } else if (0x3F00 <= addr && addr <= 0x3FFF) { // Palette ram indexes
        addr &= 0x001F;
        if (addr == 0x0010) addr = 0x0000;
        if (addr == 0x0014) addr = 0x0004;
        if (addr == 0x0018) addr = 0x0008;
        if (addr == 0x001C) addr = 0x000C;
        self->palette_RAM_idxs[addr] = val;
    }
}

void PPU_init(PPU *self) {
    self->control.reg = 0x00;
    self->mask.reg = 0x00;
    self->status.reg = 0x00;
    self->v.reg = 0x00; // vram address
    self->t.reg = 0x00; // tram address
    self->ppu_data_buffer = 0x00; // buffer for reads
    self->x = 0x00; // fine x
    self->w = 0x00; // write toggle

    self->scanline = 0x00;
    self->cycle = 0x00;

    // Background stuff
    self->next_tile_id = 0x00;
    self->next_tile_attrib = 0x00;
    self->next_tile_pattern_table_low = 0x00;
    self->next_tile_pattern_table_high = 0x00;

    self->pattern_table_shifter_low = 0x00;
    self->pattern_table_shifter_high = 0x00;
    self->attrib_table_shifter_low = 0x00;
    self->attrib_table_shifter_high = 0x00;

    self->n_sprite = 0x00;
    self->m_index = 0x00;
    self->sprite_count = 0;
    self->sprite_0_hit_possible = false;
    self->next_sprite_pattern_table_low = 0x00;
    self->next_sprite_pattern_table_high = 0x00;
    self->OAM_latch = 0x00;
}

void PPU_fetch_name_table_byte(PPU *self) {
    self->next_tile_id = PPU_read(self, 0x2000 | (self->v.reg & 0x0FFF));
}

void PPU_fetch_attribute_table_byte(PPU *self) {
    self->next_tile_attrib = PPU_read(self, 0x23C0 |
                                    (self->v.name_table_select << 10) |
                                    ((self->v.coarse_y >> 2) << 3) |
                                    (self->v.coarse_x >> 2));
    if (self->v.coarse_y & 0x02) self->next_tile_attrib >>= 4; // bottom
    if (self->v.coarse_x & 0x02) self->next_tile_attrib >>= 2; // right

    self->next_tile_attrib &= 0x03; // val between 0-3
}

void PPU_fetch_pattern_table_low(PPU *self) {
    self->next_tile_pattern_table_low = PPU_read(
        self,
        (uint16)self->control.pattern_background_addr << 12 | (uint16)self->next_tile_id << 4 | self->v.fine_y
    );
}

void PPU_fetch_pattern_table_high(PPU *self) {
    self->next_tile_pattern_table_high = PPU_read(
        self,
        ((uint16)self->control.pattern_background_addr << 12 | (uint16)self->next_tile_id << 4 | self->v.fine_y) + 8
    );
}

void PPU_load_background_shifters(PPU *self) {
    self->pattern_table_shifter_low = (self->pattern_table_shifter_low & 0xFF00) | self->next_tile_pattern_table_low;
    self->pattern_table_shifter_high = (self->pattern_table_shifter_high & 0xFF00) | self->next_tile_pattern_table_high;

    self->attrib_table_shifter_low = (self->attrib_table_shifter_low & 0xFF00) | (self->next_tile_attrib & 0x01 ? 0xFF: 0x00);
    self->attrib_table_shifter_high = (self->attrib_table_shifter_high & 0xFF00) | (self->next_tile_attrib & 0x02 ? 0xFF : 0x00);
}

void PPU_update_background_shifters(PPU *self) {
    if (self->mask.enable_background_rendering) {
        self->pattern_table_shifter_low <<= 1;
        self->pattern_table_shifter_high <<= 1;
        self->attrib_table_shifter_low <<= 1;
        self->attrib_table_shifter_high <<= 1;
    }
}

void PPU_increment_coarse_x(PPU *self) {
    if (self->mask.enable_background_rendering || self->mask.enable_sprite_rendering) {
        if (self->v.coarse_x == 31) {
            self->v.coarse_x = 0;
            self->v.name_table_select ^= 0x01;
        } else {
            self->v.coarse_x++;
        }
    }
}

void PPU_increment_y(PPU *self) {
    if (self->mask.enable_background_rendering || self->mask.enable_sprite_rendering) {
        if (self->v.fine_y < 7) {
            self->v.fine_y++;
        } else {
            self->v.fine_y = 0;
            if (self->v.coarse_y == 29) {
                self->v.coarse_y = 0;
                self->v.name_table_select ^= 0x02;
            } else if (self->v.coarse_y == 31) {
                self->v.coarse_y = 0;
            } else {
                self->v.coarse_y += 1;
            }
        }
    }
}

void PPU_transfer_address_x(PPU *self) {
    if (self->mask.enable_background_rendering || self->mask.enable_sprite_rendering) {
        self->v.coarse_x = self->t.coarse_x;
        self->v.name_table_select = (self->v.name_table_select & 0x02) | (self->t.name_table_select & 0x01);
    }
}

void PPU_transfer_address_y(PPU *self) {
    if (self->mask.enable_background_rendering || self->mask.enable_sprite_rendering) {
        self->v.coarse_y = self->t.coarse_y;
        self->v.name_table_select = (self->t.name_table_select & 0x02) | (self->v.name_table_select & 0x01);
        self->v.fine_y = self->t.fine_y;
    }
}

byte flip_byte(byte b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void PPU_load_sprite_shifters(PPU *self, byte idx) {
    byte low = self->next_sprite_pattern_table_low;
    byte high = self->next_sprite_pattern_table_high;
    if (self->secondary_OAM.sprites[idx].attributes & 0x40) {
        low = flip_byte(low);
        high = flip_byte(high); 
    }
    for (int32 i=0; i < 4; i++) {
        self->active_sprites.raw[4*idx + i] = self->secondary_OAM.raw[4*idx + i];
    }
    self->sprite_pattern_table_shifter_low[idx] = low;
    self->sprite_pattern_table_shifter_high[idx] = high;
}

void PPU_update_sprite_shifters(PPU *self) {
    if (self->mask.enable_sprite_rendering) {
        for (int32 i = 0; i < self->active_sprite_count; i++) {
            if (self->active_sprites.sprites[i].x_pos > 0) {
                self->active_sprites.sprites[i].x_pos--;
            } else {
                self->sprite_pattern_table_shifter_low[i] <<= 1;
                self->sprite_pattern_table_shifter_high[i] <<= 1;
            }
        }
    }
}

uint16 PPU_calc_sprite_tile_addr_low(PPU *self, byte idx) {
    uint16 addr = 0x00;
    byte row = self->scanline - self->secondary_OAM.sprites[idx].y_pos;
    byte sprite_height = self->control.sprite_size ? 16: 8;

    bool vertical = false;
    if (self->secondary_OAM.sprites[idx].attributes & 0x80) { // Vertical Flip
        vertical = true;
        row = (sprite_height - 1) - row;
    }

    if (sprite_height == 8) {
        addr = (self->control.pattern_sprite_addr) << 12 | (self->secondary_OAM.sprites[idx].tile_index) << 4 | (row & 0x07);
    } else {
        byte pattern_sprite_addr = self->secondary_OAM.sprites[idx].tile_index & 0x01;
        byte tile_index = self->secondary_OAM.sprites[idx].tile_index & 0xFE;
        if (row >= 8) {
            tile_index++;
            row -= 8;
        }
        addr = (pattern_sprite_addr) << 12 | (tile_index) << 4 | (row & 0x07);
    }

    return addr;
}

// using a fixed palette
uint32 PPU_get_color(PPU *self, byte palette, byte pixel) {
    byte color_index = PPU_read(self, 0x3F00 + (palette << 2) + pixel) & 0x3F;
    return NES_palette[color_index];
}

void PPU_set_pixel_color(PPU *self, int64 scanline, int64 cycle, uint32 color) {
    if ((1 <= cycle && cycle <= 256) && (0 <= scanline && scanline <= 239)) {
        self->core->screen[scanline*256 + cycle - 1] = color;
    }
}

void PPU_clock(PPU *self) {
    if (-1 <= self->scanline && self->scanline <= 239) {
        // Pre-render scanlines and Visible scanlines and fetching for next scanline
        if (self->is_odd_frame && self->scanline == 0 && self->cycle == 0 && (self->mask.enable_background_rendering || self->mask.enable_sprite_rendering)) self->cycle++; // odd frame

        if (self->cycle == 0) {
            // Do nothing
        } else if ((1 <= self->cycle && self->cycle <= 256) || (321 <= self->cycle && self->cycle <= 336)) {
            if (self->scanline == -1 && self->cycle == 1) {
                self->status.vblank = 0;
                PPU_update_NMI(self);
                self->status.sprite_0_hit = 0;
                self->status.sprite_overflow = 0;
            }

            // Data fetching for this scanline if 1 <= cycle <= 256
            // else data fetching first 2 tiles for next scanline if 321 <= cycle <= 336
            if (self->cycle != 1) PPU_update_background_shifters(self);
            switch((self->cycle - 1) % 8) {
                case 0:
                    if (self->cycle != 1) PPU_load_background_shifters(self);
                    PPU_fetch_name_table_byte(self);
                    break;
                case 2:
                    PPU_fetch_attribute_table_byte(self);
                    break;
                case 4:
                    PPU_fetch_pattern_table_low(self);
                    break;
                case 6:
                    PPU_fetch_pattern_table_high(self);
                    break;
                case 7:
                    PPU_increment_coarse_x(self); // new tile
                    break;
            }
            if (self->cycle == 256) PPU_increment_y(self);

            /* Sprite Evaluation */
            if (1 <= self->cycle && self->cycle <= 256) PPU_update_sprite_shifters(self);
            if (1 <= self->cycle && self->cycle <= 64) { // clears secondary OAM to 0xFF
                if (self->cycle % 2 == 1) self->secondary_OAM.raw[(self->cycle - 1) / 2] = 0xFF;
            } else if (65 <= self->cycle && self->cycle <= 256) {
                if (self->cycle == 65) {
                    self->n_sprite = 0;
                    self->m_index = 0;
                    self->sprite_count = 0;
                    self->sprite_0_hit_possible = false;
                }
                if (self->cycle % 2 == 1) {
                    self->OAM_latch = self->OAM.raw[4 * (self->n_sprite % 64) + self->m_index];
                } else {
                    if (self->n_sprite < 64) {
                        int32 diff = self->scanline  - self->OAM.raw[4 * self->n_sprite]; // OAM[n][0]
                        byte sprite_height = self->control.sprite_size ? 16: 8;
                        if (self->sprite_count < 8) {
                            if (self->m_index == 0) {
                                if ((0 <= diff && diff < sprite_height)) {
                                    if (self->n_sprite == 0 && self->m_index == 0) self->sprite_0_hit_possible = true;
                                    self->secondary_OAM.raw[4 * self->sprite_count] = self->OAM_latch;
                                    self->m_index = 1;
 
                                } else {
                                    self->n_sprite++;
                                    self->m_index = 0;
                                }
                            } else {
                                self->secondary_OAM.raw[4 * self->sprite_count + self->m_index] = self->OAM_latch;
                                self->m_index++;
                                if (self->m_index == 4) {
                                    self->m_index = 0;
                                    self->n_sprite++;
                                    self->sprite_count++;
                                }
                            }
                        } else {
                            // Sprite overflow bug
                            if ((0 <= diff && diff < sprite_height) || self->m_index > 0) {
                                self->status.sprite_overflow = 1;
                                self->m_index++;
                                if (self->m_index == 4) {
                                    self->m_index = 0;
                                    self->n_sprite++;
                                }
                            } else {
                                self->n_sprite++;
                                self->m_index = (self->m_index + 1) % 4;
                            }
                        }
                    } else {
                        self->n_sprite++;
                    }
                }
            }

            // TODO: 321-340 sprite stuff
            // EDIT: no need to
        } else if (257 <= self->cycle && self->cycle <= 320) {
            if (self->cycle == 257) {
                PPU_load_background_shifters(self);
                PPU_transfer_address_x(self);
                self->active_sprite_count = self->sprite_count;
                self->active_sprite_0_hit_possible = self->sprite_0_hit_possible;
            }
            if (self->scanline == -1 && (280 <= self->cycle && self->cycle <= 304)) PPU_transfer_address_y(self);

            // Data fetching of tiles for sprites on next scanline
            byte sprite_idx = (self->cycle - 257) / 8;
            if (sprite_idx < self->sprite_count) {
                switch((self->cycle - 1) % 8) {
                    case 0:
                        break;
                    case 2:
                        break;
                    case 4:
                        self->next_sprite_pattern_table_low = PPU_read(self, PPU_calc_sprite_tile_addr_low(self, sprite_idx));
                        break;
                    case 6:
                        self->next_sprite_pattern_table_high = PPU_read(self, PPU_calc_sprite_tile_addr_low(self, sprite_idx) + 8);
                        break;
                    case 7:
                        PPU_load_sprite_shifters(self, sprite_idx);
                        break;
                }
            }
        } else if (337 <= self->cycle && self->cycle <= 340) {
            // Two bytes are fetched but use is unknown
            if (self->cycle == 337) {
                PPU_load_background_shifters(self);
                PPU_fetch_name_table_byte(self);
            }
            if (self->cycle == 340) {
                PPU_fetch_name_table_byte(self);
            }
        }
    } else if (self->scanline == 240) {
        // Do nothing: Post render scanline
    } else if (241 <= self->scanline && self->scanline <= 260) {
        // Vertical Blanking scanlines
        if (self->scanline == 241 && self->cycle == 1) {
            self->status.vblank = 1;
            PPU_update_NMI(self);
        }
    }

    // Background Pixel Eval
    byte bg_pixel = 0x00;
    byte bg_palette = 0x00;

    if (self->mask.enable_background_rendering) {
        uint16 bit_mux = 0x8000 >> self->x;

        byte p_low = (self->pattern_table_shifter_low & bit_mux) > 0;
        byte p_high = (self->pattern_table_shifter_high & bit_mux) > 0;
        bg_pixel = (p_high << 1) | p_low;

        byte pal_low = (self->attrib_table_shifter_low & bit_mux) > 0;
        byte pal_high = (self->attrib_table_shifter_high & bit_mux) > 0;
        bg_palette = (pal_high << 1) | pal_low;
    }

    // Foreground Pixel Eval
    byte fg_pixel = 0x00;
    byte fg_palette = 0x00;
    byte fg_priority = 0x00;
    bool sprite_0_being_rendered = false;

    if (self->mask.enable_sprite_rendering) {
        for (int32 i = 0; i < self->active_sprite_count; i++) {
            if (self->active_sprites.sprites[i].x_pos == 0) {
                byte p_low = (self->sprite_pattern_table_shifter_low[i] & 0x80) > 0;
                byte p_high = (self->sprite_pattern_table_shifter_high[i] & 0x80) > 0;
                fg_pixel = (p_high << 1) | p_low;
                fg_palette =  (self->active_sprites.sprites[i].attributes & 0x03) + 0x04;
                fg_priority = self->active_sprites.sprites[i].attributes & 0x20;

                if (fg_pixel != 0) { // in front of background
                    if (i == 0) {
                        sprite_0_being_rendered = true;
                    }
                    break;
                }
            }
        }
    }

    // Priority Multiplexer
    byte final_pixel = 0x00;
    byte final_palette = 0x00;
    if (bg_pixel == 0 && fg_pixel > 0) {
        final_pixel = fg_pixel;
        final_palette = fg_palette;
    } else if (bg_pixel > 0 && fg_pixel == 0) {
        final_pixel = bg_pixel;
        final_palette = bg_palette;
    } else if (bg_pixel > 0 && fg_pixel > 0) {
        if (!fg_priority) {
            final_pixel = fg_pixel;
            final_palette = fg_palette;
        } else {
            final_pixel = bg_pixel;
            final_palette = bg_palette;
        }
        if (sprite_0_being_rendered && self->active_sprite_0_hit_possible && self->cycle < 256) {
            if (!(self->mask.show_background_leftmost || self->mask.show_sprite_leftmost)) {
                if (self->cycle >= 9 && self->cycle <= 256) {
                    self->status.sprite_0_hit = 1;
                }
            } else {
                if (self->cycle >= 1 && self->cycle <= 256) {
                    self->status.sprite_0_hit = 1;
                }
            }
        }
    }

    PPU_set_pixel_color(self, self->scanline, self->cycle-1, PPU_get_color(self, final_palette, final_pixel));

    self->cycle++;
    if (self->cycle >= 341) {
        self->cycle = 0;
        self->scanline++;
        if (self->scanline >= 261) {
            self->is_odd_frame = !self->is_odd_frame;
            self->scanline = -1;
            self->core->frame_complete = true;
        }
    }
}

void PPU_connect_core(PPU *self, NES_CORE *core) {
    self->core = core;
}
