#include "controller.h"

void Controller_push_state(Controller* self, byte state) {
    self->current_state = state;
}

byte Controller_cpu_read(Controller *self) {
    byte data = 0x00;
    if (self->strobe) {
        data = (self->current_state & 0x80) > 0;
    } else {
        data = (self->shift_register & 0x80) > 0;
        self->shift_register <<= 1;
    }

    return data;
}

void Controller_cpu_write(Controller *self, byte val) {
    self->strobe = val & 0x01;
    if (self->strobe) {
        self->shift_register = self->current_state;
    }
}