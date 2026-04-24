#ifndef NES_CORE_H
#define NES_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"
#include "cpu.h"
#include "ppu.h"
#include "apu/apu.h"
#include "cartridge.h"
#include "controller.h"
#include "logger.h"
#include "miniaudio.h"

#define NES_CPU_RAM_SIZE 2 * 1024
#define NES_SCREEN_WIDTH 256
#define NES_SCREEN_HEIGHT 240

typedef struct Cartridge Cartridge;

typedef struct NES_CORE {
    bool loggingEnabled;
    bool nmi_pending;
    bool irq_pending;
    bool dma_active;
    bool frame_complete;
    uint32 master_clock_count;
    byte cpu_ram[NES_CPU_RAM_SIZE];
    uint32 screen[NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];
    ma_device audio_device;
    ma_context audio_context;

    CPU cpu;
    PPU ppu;
    APU apu;
    Cartridge cart;
    Controller controller;
} NES_CORE;

NES_CORE *NES_create();
void NES_destroy(NES_CORE *self);

void NES_load_rom(NES_CORE *self, const char *name);
void NES_free_rom(NES_CORE *self);
void NES_init(NES_CORE *self);
void NES_reset(NES_CORE *self);
void NES_step(NES_CORE *self);
void NES_step_frame(NES_CORE *self);

void NES_controller_push_state(NES_CORE *self, byte state);

void NES_stop_audio(NES_CORE* self);
void NES_start_audio(NES_CORE* self);

// Testing/debugging/hacks stuff
void NES_enable_logging(NES_CORE *self);
void NES_set_cpu_pc(NES_CORE *self, int32 pc);
void NES_set_cpu_ram(NES_CORE *self, uint16 addr, byte value);
void NES_set_current_ui_palette(NES_CORE *self, byte palette);

#ifdef __cplusplus
}
#endif

#endif