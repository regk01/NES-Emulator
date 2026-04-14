#ifndef BUS_H
#define BUS_H

#include "types.h"
#include "cartridge.h"

byte BUS_cpu_read(NES_CORE *nes, uint16 addr);
void BUS_cpu_write(NES_CORE *nes, uint16 addr, byte val);

#endif