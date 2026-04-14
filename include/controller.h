#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "types.h"

typedef struct Controller {
    byte shift_register; // what cpu sees
    byte current_state; // what the frontend passes as current input
    bool strobe; // high or low
} Controller;

void Controller_push_state(Controller* self, byte state);
byte Controller_cpu_read(Controller *self);
void Controller_cpu_write(Controller *self, byte val);

#endif