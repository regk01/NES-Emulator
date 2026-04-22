#include <stdlib.h>
#include <stdio.h>
#include "core.h"
#include "cpu.h"
#include "cartridge.h"
#include "bus.h"

NES_CORE *NES_create() {
    NES_CORE *self = calloc(1, sizeof(NES_CORE));
    return self;
}

void NES_destroy(NES_CORE *self) {
    ma_device_uninit(&self->audio_device);
    ma_context_uninit(&self->audio_context);
    free(self);
}

void NES_enable_logging(NES_CORE *self) {
    self->loggingEnabled = true;
}

void NES_load_rom(NES_CORE *self, const char *name) {
    Cartridge_load_rom(&(self->cart), name);
}

void NES_apu_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    float32 *out = (float32 *)pOutput;
    NES_CORE *self = (NES_CORE *)pDevice->pUserData;

    for (uint32 i = 0; i < frameCount; i++) {
        if (self->apu.buffer.ring_buffer.count > 0) {
            self->apu.buffer.last_sample = self->apu.buffer.ring_buffer.data[self->apu.buffer.ring_buffer.read_ptr];
            out[i] = self->apu.buffer.ring_buffer.data[self->apu.buffer.ring_buffer.read_ptr];
            self->apu.buffer.ring_buffer.read_ptr = (self->apu.buffer.ring_buffer.read_ptr + 1) % AUDIO_RING_BUFFER_SIZE;
            self->apu.buffer.ring_buffer.count--;
        } else {
            out[i] = self->apu.buffer.last_sample;
        }
    }
}

void NES_reset(NES_CORE *self) {
    CPU_init(&(self->cpu));
    PPU_init(&(self->ppu));
    APU_init(&(self->apu));
    APU_connect_core(&(self->apu), self);
}

void NES_stop_audio(NES_CORE* self) {
    if (self == NULL) return;

    if (ma_device_is_started(&self->audio_device)) {
        ma_device_stop(&self->audio_device);
    }
    
    self->apu.buffer.ring_buffer.read_ptr = 0;
    self->apu.buffer.ring_buffer.write_ptr = 0;
    self->apu.buffer.ring_buffer.count = 0;
}

void NES_start_audio(NES_CORE* self) {
    if (self == NULL) return;

    if (!ma_device_is_started(&self->audio_device)) {
        ma_device_start(&self->audio_device);
    }
}

void NES_init(NES_CORE *self) {
    CPU_connect_core(&(self->cpu), self);
    PPU_connect_core(&(self->ppu), self);

    CPU_init(&(self->cpu));
    PPU_init(&(self->ppu));
    APU_init(&(self->apu));
    APU_connect_core(&(self->apu), self); // because of memset in APU_init, this has to be called after APU_init

    if (ma_context_init(NULL, 0, NULL, &self->audio_context) != MA_SUCCESS) {
        printf("Couldn't create miniaudio context\n");
        exit(1);
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_f32;
    config.playback.channels = 1;
    config.sampleRate = AUDIO_SAMPLE_RATE;
    config.periodSizeInFrames = 882; // AUDIO_SAMPLE_RATE * 0.02 so about 20ms of audio
    config.dataCallback = NES_apu_data_callback;
    config.pUserData = self;

    if (ma_device_init(&self->audio_context, &config, &self->audio_device) != MA_SUCCESS) {
        printf("Couldn't create miniaudio device\n");
        exit(1);
    }
    if (ma_device_start(&self->audio_device) != MA_SUCCESS) {
        printf("Failed to start miniaudio device\n");
        exit(1);
    }
}

void NES_set_cpu_pc(NES_CORE *self, int32 pc) {
    self->cpu.pc = pc;
}

void NES_set_cpu_ram(NES_CORE *self, uint16 addr, byte value) {
    self->cpu_ram[addr & 0x7FF] = value;
}

void NES_clock(NES_CORE *self) {
    PPU_clock(&(self->ppu));
    if (self->master_clock_count % 3 == 0){
        CPU_clock(&(self->cpu));
        APU_clock(&(self->apu));
    }
    self->master_clock_count++;
}

void NES_step(NES_CORE *self) {
    while (self->cpu.cycles == 0) {
        NES_clock(self);
    }
    while (self->cpu.cycles > 0) {
        NES_clock(self);
    }
}

void NES_step_frame(NES_CORE *self) {
    self->frame_complete = false;

    while (!self->frame_complete) {
        NES_clock(self);
    }
}

void NES_controller_push_state(NES_CORE *self, byte state) {
    Controller_push_state(&(self->controller), state);
}

