#ifndef HELPERS_H
#define HELPERS_H

#include "types.h"

typedef struct PulseChannel PulseChannel;

byte LengthCounter_calculate_table_idx(byte val);
void LengthCounter_clock(byte *self);

typedef struct Divider {
    uint16 period;
    uint16 counter;
} Divider;

void Divider_set_period_low(Divider *self, byte val);
void Divider_set_period_high(Divider *self, byte val);
bool Divider_clock(Divider *self);

typedef struct Sequencer {
    byte step;
    byte max_steps;
} Sequencer;

void Sequencer_set_max_steps(Sequencer *self, byte max_steps);
void Sequencer_clock(Sequencer *self);

typedef struct SweepUnit {
    bool enabled;
    bool negate;
    byte period;
    byte shift;

    byte divider;
    bool reload;
    bool is_muting;
} SweepUnit;

void SweepUnit_cpu_write(SweepUnit *self, uint16 addr, byte val);
uint16 SweepUnit_calculate_target_period(SweepUnit *self, uint16 period, bool is_pulse_1);
void SweepUnit_clock(SweepUnit *self, PulseChannel *pulse, bool is_pulse_1);

typedef struct Envelope {
    bool loop;
    bool constant_volume;
    byte volume; // also used as period

    bool start;
    byte divider;
    byte decay_level;
} Envelope;

byte Envelope_get_volume(Envelope *self);
void Envelope_cpu_write(Envelope *self, uint16 addr, byte val);
void Envelope_clock(Envelope *self);

#endif