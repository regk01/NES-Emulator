#include "apu/apu.h"
#include <stdio.h>
#include <string.h>

const byte APU_length_table[32] = {
    10, 254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12,  16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

const byte APU_duty_table[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0}, // Duty 0: 12.5%
    {0, 1, 1, 0, 0, 0, 0, 0}, // Duty 1: 25%
    {0, 1, 1, 1, 1, 0, 0, 0}, // Duty 2: 50%
    {1, 0, 0, 1, 1, 1, 1, 1}  // Duty 3: 25% negated
};

const uint16 APU_noise_timer_periods[16] = {
    4, 8, 16, 32, 64, 96, 128, 160,
    202, 254, 380, 508, 762, 1016, 2034, 4068
};

float32 APU_pulse_table[31];

float32 APU_tnd_table[203];

void APU_precompute_pulse_table() {
    APU_pulse_table[0] = 0;
    for (int n = 1; n < 31; n++) {
        APU_pulse_table[n] = 95.52f / (8128.0f /(float32)n + 100.0f);
    }
};

void APU_precompute_tnd_table() {
    APU_tnd_table[0] = 0;
    for (int n = 1; n < 203; n++) {
        APU_tnd_table[n] = 163.67f / (24329.0f / (float32)n + 100.0f);
    }
};

void APU_init(APU *self) {
    APU_precompute_pulse_table();
    APU_precompute_tnd_table();

    memset(self, 0, sizeof(APU));
    self->frame_counter.sequencer.max_steps = 4;
    self->frame_counter.irq_inhibit = true;

    self->pulse_1.sequencer.max_steps = 8;
    self->pulse_2.sequencer.max_steps = 8;
    self->triangle.sequencer.max_steps = 32;
    self->noise.lsfr = 0x01;
}

// Pulse stuff

byte Pulse_get_output(PulseChannel *self) {
    if (self->length_counter == 0) return 0;
    if (self->sweep.is_muting) return 0;
    if (self->length_counter == 0) return 0;
    if (self->timer.period < 8) return 0;

    byte curr_waveform = APU_duty_table[self->duty_cycle][self->sequencer.step];
    if (curr_waveform == 0) return 0;
    return Envelope_get_volume(&(self->envelope));
}

void Pulse_cpu_write(PulseChannel *self, byte apu_status, uint16 addr, byte val) {
    addr &= 0x0003;
    switch (addr) {
        case 0x0000:
            self->duty_cycle = (val >> 6) & 0x0003;
            Envelope_cpu_write(&(self->envelope), addr, val);
            break;
        case 0x0001:
            SweepUnit_cpu_write(&(self->sweep), addr, val);
            break;
        case 0x0002:
            Divider_set_period_low(&(self->timer), val);
            break;
        case 0x0003:
            if (apu_status) self->length_counter = APU_length_table[LengthCounter_calculate_table_idx(val)];
            Divider_set_period_high(&(self->timer), val);
            self->sequencer.step = 0;
            self->envelope.start = true;
            break;
    }
}

// Triangle stuff

byte Triangle_get_output(Triangle *self) {
    if (self->linear_counter == 0 || self->length_counter == 0) return 0;
    if (self->sequencer.step <= 15) return 15 - self->sequencer.step;
    else return self->sequencer.step - 16;
}

void LinearCounter_clock(Triangle *self) {
    if (self->linear_counter_reload) {
        self->linear_counter = self->linear_counter_reload_val;
    } else {
        if (self->linear_counter > 0) self->linear_counter--;
    }
    if (!self->control) self->linear_counter_reload = false;
}

void Triangle_cpu_write(Triangle *self, byte apu_status, uint16 addr, byte val) {
    addr &= 0x001F;
    switch (addr) {
        case 0x0008:
            self->control = (val >> 7) & 0x01;
            self->linear_counter_reload_val = val & 0x3F;
            break;
        case 0x000A:
            Divider_set_period_low(&(self->timer), val);
            break;
        case 0x000B:
            if (apu_status) self->length_counter = APU_length_table[LengthCounter_calculate_table_idx(val)];
            Divider_set_period_high(&(self->timer), val);
            self->linear_counter_reload = true;
            break;
    }
}

// Noise stuff

byte Noise_get_output(Noise *self) {
    if (self->length_counter == 0) return 0;
    return (self->lsfr & 0x01) ? 0: Envelope_get_volume(&(self->envelope));
}

void LinearShiftRegister_clock(Noise *self) {
    uint16 bit0 = self->lsfr & 0x01;
    uint16 bitX = (self->lsfr >> (self->mode ? 6: 1)) & 0x01;

    uint16 feedback = bit0 ^ bitX;

    self->lsfr = (self->lsfr >> 1) | (feedback << 14);
}

void Noise_cpu_write(Noise *self, byte apu_status, uint16 addr, byte val) {
    addr &= 0x000F;
    switch (addr) {
        case 0x000C:
            Envelope_cpu_write(&(self->envelope), 0x0000, val);
            break;
        case 0x000E:
            self->mode = (val & 0x80) > 0;
            self->timer.period = APU_noise_timer_periods[val & 0x000F];
            break;
        case 0x000F:
            if (apu_status) self->length_counter = APU_length_table[LengthCounter_calculate_table_idx(val)];
            self->envelope.start = true;
            break;
    }
}

// DMC stuff
void DMC_cpu_write(DMC *self, byte apu_status, uint16 addr, byte val) {
    addr &= 0x001F;
    switch (addr) {
        case 0x0010:
            break;
        case 0x0011:
            self->output_level = val & 0x007F;
            break;
        case 0x0012:
            break;
        case 0x0013:
            break;

    }
}

// APU stuff

byte APU_cpu_read(APU *self, uint16 addr) {
    addr &= 0x001F;
    byte data = 0x00;

    if (addr == 0x0015) {
        byte status = 0;
        if (self->frame_counter.frame_interrupt) {
            status |= 0x40;
            self->frame_counter.frame_interrupt = false;
        }
        if (self->pulse_1.length_counter > 0) status |= 0x01;
        if (self->pulse_2.length_counter > 0) status |= 0x02;
        if (self->triangle.length_counter > 0) status |= 0x04;
        if (self->noise.length_counter > 0) status |= 0x08;
        data = status;
    }

    return data;
}

void APU_trigger_quarter(APU *self) {
    Envelope_clock(&(self->pulse_1.envelope));
    Envelope_clock(&(self->pulse_2.envelope));
    LinearCounter_clock(&(self->triangle));
    Envelope_clock(&(self->noise.envelope));
}

void APU_trigger_half(APU *self) {
    if (!self->pulse_1.envelope.loop) LengthCounter_clock(&(self->pulse_1.length_counter));
    if (!self->pulse_2.envelope.loop) LengthCounter_clock(&(self->pulse_2.length_counter));
    if (!self->triangle.control) LengthCounter_clock(&(self->triangle.length_counter));
    if (!self->noise.envelope.loop) LengthCounter_clock(&(self->noise.length_counter));

    SweepUnit_clock(&(self->pulse_1.sweep), &(self->pulse_1), true);
    SweepUnit_clock(&(self->pulse_2.sweep), &(self->pulse_2), false);
}

void APU_cpu_write(APU *self, uint16 addr, byte val) {
    addr &= 0x001F;
    if (0x0000 <= addr && addr <= 0x0003) Pulse_cpu_write(&(self->pulse_1), self->status.pulse_1, addr, val);
    else if (0x0004 <= addr && addr <= 0x0007) Pulse_cpu_write(&(self->pulse_2), self->status.pulse_2, addr, val);
    else if (0x0008 <= addr && addr <= 0x000B) Triangle_cpu_write(&(self->triangle), self->status.triangle, addr, val);
    else if (0x000C <= addr && addr <= 0x000F) Noise_cpu_write(&(self->noise), self->status.noise, addr, val);
    else if (0x0010 <= addr && addr <= 0x0013) DMC_cpu_write(&(self->dmc), self->status.enable_dmc, addr, val);
    else if (addr == 0x0015) {
        self->status.reg = val;
        if (!self->status.pulse_1) self->pulse_1.length_counter = 0;
        if (!self->status.pulse_2) self->pulse_2.length_counter = 0;
        if (!self->status.triangle) self->triangle.length_counter = 0;
        if (!self->status.noise) self->noise.length_counter = 0;
    } else if (addr == 0x0017) {
        self->frame_counter.irq_inhibit = (val >> 6) & 0x01;
        self->frame_counter.sequencer.step = 0;
        Sequencer_set_max_steps(&(self->frame_counter.sequencer), (val >> 7) & 0x01 ? 5: 4);
        if ((val >> 7) & 0x01) {
            APU_trigger_quarter(self);
            APU_trigger_half(self);
        }
    }
}

void APU_process_step(APU *self) {
    if (self->frame_counter.sequencer.max_steps == 4) {
        switch(self->frame_counter.sequencer.step) {
            case 0:
                APU_trigger_quarter(self);
                break;
            case 1:
                APU_trigger_quarter(self);
                APU_trigger_half(self);
                break;
            case 2:
                APU_trigger_quarter(self);
                break;
            case 3:
                APU_trigger_quarter(self);
                APU_trigger_half(self);
                if (!self->frame_counter.irq_inhibit) self->frame_counter.frame_interrupt = true;
                break;
        }
    } else { // 5 steps
        switch(self->frame_counter.sequencer.step) {
            case 0:
                APU_trigger_quarter(self);
                break;
            case 1:
                APU_trigger_quarter(self);
                APU_trigger_half(self);
                break;
            case 2:
                APU_trigger_quarter(self);
                break;
            case 3: // Nothing
                break;
            case 4:
                APU_trigger_quarter(self);
                APU_trigger_half(self);
                break;
        }
    }

    Sequencer_clock(&(self->frame_counter.sequencer));
}

float32 APU_get_output(APU *self) {
    byte pulse_1 = self->status.pulse_1 ? Pulse_get_output(&(self->pulse_1)) : 0;
    byte pulse_2 = self->status.pulse_2 ? Pulse_get_output(&(self->pulse_2)) : 0;
    byte triangle = self->status.triangle ? Triangle_get_output(&(self->triangle)) : 0;
    byte noise = self->status.noise ? Noise_get_output(&(self->noise)) : 0;
    byte dmc = self->dmc.output_level;

    float32 pulse_output = APU_pulse_table[pulse_1 + pulse_2];
    float32 tnd_output = APU_tnd_table[3 * triangle + 2 * noise + dmc];

    float32 output = pulse_output + tnd_output;
    return output;
}

void APU_clock(APU *self) {
    if (Divider_clock(&(self->triangle.timer))) {
        if (self->triangle.timer.period >= 2 && self->triangle.linear_counter > 0 && self->triangle.length_counter > 0) Sequencer_clock(&(self->triangle.sequencer));
    }
    if (Divider_clock(&(self->noise.timer))) LinearShiftRegister_clock(&(self->noise));
    if (self->total_cycles % 2 == 0) {
        if (Divider_clock(&(self->pulse_1.timer))) Sequencer_clock(&(self->pulse_1.sequencer));
        if (Divider_clock(&(self->pulse_2.timer))) Sequencer_clock(&(self->pulse_2.sequencer));
    }

    self->buffer.accumulator += APU_get_output(self);
    self->buffer.cycles_accumulated++;
    if (self->buffer.cycles_accumulated >= MAX_APU_BUFFER_CYCLES) {
        float32 final_sample = self->buffer.accumulator / self->buffer.cycles_accumulated;
        // Output to ring buffer
        self->buffer.ring_buffer.data[self->buffer.ring_buffer.write_ptr] = final_sample;
        self->buffer.ring_buffer.write_ptr = (self->buffer.ring_buffer.write_ptr + 1) % AUDIO_RING_BUFFER_SIZE;
        self->buffer.ring_buffer.count++;

        self->buffer.accumulator = 0;
        self->buffer.cycles_accumulated = 0;
    }

    self->frame_counter.cycles_count++;
    if (self->frame_counter.cycles_count >= CPU_CYCLES_COUNT) {
        self->frame_counter.cycles_count = 0;
        APU_process_step(self);
    }

    self->total_cycles++;
}
