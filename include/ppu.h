#ifndef PPU_H
#define PPU_H

#include "types.h"

#define PATTERN_TABLE_SIZE 128*128

typedef struct NES_CORE NES_CORE;

typedef union LoopyRegister {
    struct {
        uint16 coarse_x: 5;
        uint16 coarse_y: 5;
        uint16 name_table_select: 2;
        uint16 fine_y: 3;
        uint16 unused: 1;
    };
    uint16 reg;
} LoopyRegister;

typedef struct SpriteData {
    byte y_pos;
    byte tile_index;
    byte attributes;
    byte x_pos;
} SpriteData;

typedef struct PPU {
    byte name_tables[2][1024]; // 2KB Internal RAM
    byte palette_RAM_idxs[32]; // Palette Ram indexes

    union {
        SpriteData sprites[64];
        byte raw[256];
    } OAM; // Object Attribute Memory i.e Sprite stuff
    NES_CORE *core;

    // MMIO Registers
    union {
        struct {
            byte nametable_x: 1;
            byte nametable_y: 1;
            byte vram_increment: 1;
            byte pattern_sprite_addr: 1;
            byte pattern_background_addr: 1;
            byte sprite_size: 1;
            byte is_master_mode: 1;
            byte enable_vblank_nmi: 1;
        };
        byte reg;
    } control;
    union {
        struct {
            byte greyscale: 1;
            byte show_background_leftmost: 1;
            byte show_sprite_leftmost: 1;
            byte enable_background_rendering: 1;
            byte enable_sprite_rendering: 1;
            byte emphasize_red: 1;
            byte emphasize_green: 1;
            byte emphasize_blue: 1;
        };
        byte reg;
    } mask;
    union {
        struct {
            byte unused: 5;
            byte sprite_overflow: 1;
            byte sprite_0_hit: 1;
            byte vblank: 1;
        };
        byte reg;
    } status;
    byte oam_addr;

    // Internal registers
    LoopyRegister v; // vram address
    LoopyRegister t; // tram address
    byte ppu_data_buffer; // buffer for reads
    byte x; // fine x
    byte w; // write toggle

    int64 scanline;
    int64 cycle;
    bool is_odd_frame;

    // Background stuff; also used in sprite rendering
    byte next_tile_id;
    byte next_tile_attrib;
    byte next_tile_pattern_table_low;
    byte next_tile_pattern_table_high;

    uint16 pattern_table_shifter_low;
    uint16 pattern_table_shifter_high;
    uint16 attrib_table_shifter_low;
    uint16 attrib_table_shifter_high;

    // Sprite stuff
    union {
        SpriteData sprites[8];
        byte raw[32];
    } secondary_OAM; // Secondary OAM i.e Sprite stuff used during evaluation of next scanline
    union {
        SpriteData sprites[8];
        byte raw[32];
    } active_sprites; // i.e Sprite stuff used during rendering current scanline
    byte active_sprite_count; // how many sprites in secondary OAM
    byte sprite_count; // how many sprites in secondary OAM
    byte sprite_pattern_table_shifter_low[8];
    byte sprite_pattern_table_shifter_high[8];
    byte n_sprite; // current sprite in OAM; goes from 0-255
    byte m_index; // current index in OAM[n_sprite]; goes from 0-3
    byte next_sprite_pattern_table_low;
    byte next_sprite_pattern_table_high;
    byte OAM_latch;
    bool sprite_0_hit_possible;
    bool active_sprite_0_hit_possible;

    // For UI
    uint32 palette_ui[32];
    byte current_palette_ui;
    uint32 pattern_table_ui[2 * PATTERN_TABLE_SIZE];
    // To know when I need to redraw the palette ui and pattern table ui
    bool palette_ui_dirty;
    bool pattern_table_ui_dirty;
} PPU;

byte PPU_cpu_read(PPU *self, uint16 addr);
void PPU_cpu_write(PPU *self, uint16 addr, byte val);
byte PPU_read(PPU *self, uint16 addr);
void PPU_write(PPU *self, uint16 addr, byte val);
void PPU_init(PPU *self);

void PPU_connect_core(PPU *self, NES_CORE *core);
void PPU_clock(PPU *self);

// UI stuff for python
byte *PPU_get_ui_palette(PPU *self, byte idx);
byte *PPU_get_ui_pattern_table(PPU *self, byte idx);
void PPU_set_current_ui_palette(PPU *self, byte palette);

extern uint32 NES_palette[64];

#endif