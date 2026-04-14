#ifndef APU_H
#define APU_H

#include "types.h"
#include "helpers.h"

#define CPU_CYCLES_COUNT 7457
#define MAX_APU_BUFFER_CYCLES 40
#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_RING_BUFFER_SIZE 8192

typedef struct PulseChannel {
    Envelope envelope;
    SweepUnit sweep;
    Divider timer;
    Sequencer sequencer;
    byte length_counter;

    byte duty_cycle;
} PulseChannel;

typedef struct Triangle {
    Divider timer;
    Sequencer sequencer;
    byte length_counter;
    byte linear_counter;
    byte linear_counter_reload_val;

    bool linear_counter_reload;
    bool control;
} Triangle;

typedef struct Noise {
    Envelope envelope;
    Divider timer;
    byte length_counter;
    uint16 lsfr; // linear shift register

    bool mode;
} Noise;

typedef struct DMC {
    Divider timer;
    byte rate;
    byte output_level;

    bool irq_enabled;
    bool loop;
} DMC;

typedef struct APU {
    PulseChannel pulse_1;
    PulseChannel pulse_2;
    Triangle triangle;
    Noise noise;
    DMC dmc;

    // Frame Counter stuff
    struct FrameCounter {
        uint32 cycles_count;
        Sequencer sequencer;
        bool frame_interrupt;
        bool irq_inhibit;
    } frame_counter;

    union {
        struct {
            byte pulse_1: 1;
            byte pulse_2: 1;
            byte triangle: 1;
            byte noise: 1;
            byte enable_dmc: 1;
            byte unused: 3;
        };
        byte reg;
    } status;
    uint64 total_cycles;

    struct {
        struct {
            float32 data[AUDIO_RING_BUFFER_SIZE]; // Large enough to hold a few frames
            uint32 read_ptr;
            uint32 write_ptr;
            uint32 count;
        } ring_buffer;
        float32 accumulator;
        float32 last_sample;
        float32 low_pass_out;
        uint32 cycles_accumulated;
    } buffer;
} APU;

void APU_init(APU *self);
byte APU_cpu_read(APU *self, uint16 addr);
void APU_cpu_write(APU *self, uint16 addr, byte val);
void APU_clock(APU *self);

#endif