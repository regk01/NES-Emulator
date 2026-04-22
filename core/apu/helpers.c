#include "apu/helpers.h"
#include "apu/apu.h"

void LengthCounter_clock(byte *self) {
    if (*self > 0) (*self)--;
}

byte LengthCounter_calculate_table_idx(byte val) {
    return (val >> 3) & 0x001F;
};

void Divider_set_period_low(Divider *self, byte val) {
    self->period = (self->period & 0x0700) | val;
}

void Divider_set_period_high(Divider *self, byte val) {
    self->period = ((val & 0x07) << 8) | (self->period & 0x00FF);
}

bool Divider_clock(Divider *self) {
    if (self->counter == 0) {
        self->counter = self->period;
        return true;
    } else {
        self->counter--;
        return false;
    }
}

void Sequencer_set_max_steps(Sequencer *self, byte max_steps) {
    self->max_steps = max_steps;
}

void Sequencer_clock(Sequencer *self) {
    self->step++;

    if (self->step >= self->max_steps) {
        self->step = 0;
    }
}

void SweepUnit_cpu_write(SweepUnit *self, uint16 addr, byte val) {
    self->enabled = (val >> 7) & 0x0001;
    self->period = (val >> 4) & 0x0007;
    self->negate = (val >> 3) & 0x0001;
    self->shift = val & 0x0007;
    self->reload = true;
}

uint16 SweepUnit_calculate_target_period(SweepUnit *self, uint16 period, bool is_pulse_1) {
    uint16 change_amount = period >> self->shift;
    int32 target_period;
    if (self->negate) {
        target_period = period - change_amount;
        if (is_pulse_1) target_period--;
    } else {
        target_period = period + change_amount;
    }

    if (target_period < 0) target_period = 0;

    return (uint16)target_period;
}

void SweepUnit_clock(SweepUnit *self, PulseChannel *pulse, bool is_pulse_1) {
    uint16 target_period = SweepUnit_calculate_target_period(self, pulse->timer.period, is_pulse_1);
    self->is_muting = (pulse->timer.period < 8 || target_period > 0x07FF);

    if (self->divider == 0 && self->enabled && self->shift != 0 && !self->is_muting) {
        pulse->timer.period = target_period;
    }

    if (self->divider == 0 || self->reload) {
        self->divider = self->period;
        self->reload = false;
    } else {
        self->divider--;
    }
}

void Envelope_cpu_write(Envelope *self, uint16 addr, byte val) {
    self->loop = (val >> 5) & 0x0001;
    self->constant_volume = (val >> 4) & 0x0001;
    self->volume = val & 0x000F;
}

byte Envelope_get_volume(Envelope *self) {
    if (self->constant_volume) return self->volume;
    return self->decay_level;
}

void Envelope_clock(Envelope *self) {
    if (!self->start) {
        if (self->divider == 0) {
            self->divider = self->volume;
            if (self->decay_level > 0) {
                self->decay_level--;
            } else if (self->loop) {
                self->decay_level = 15;
            }
        } else {
            self->divider--;
        }
    } else {
        self->start = false;
        self->decay_level = 15;
        self->divider = self->volume;
    }
}
