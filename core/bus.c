#include <stdio.h>
#include <inttypes.h>
#include "core.h"
#include "bus.h"
#include "controller.h"

byte BUS_cpu_read(NES_CORE *nes, uint16 addr) {
    byte data = 0x00;

    if (Cartridge_cpu_read(&(nes->cart), addr, &data)) {
    } else if (0x0000 <= addr && addr <= 0x1FFF) {
        data = nes->cpu_ram[addr & 0x07FF];
    } else if (0x2000 <= addr && addr <= 0x3FFF) {
        data = PPU_cpu_read(&(nes->ppu), addr);
    } else if (addr == 0x4015) {
        data = APU_cpu_read(&(nes->apu), addr);
    } else if (addr == 0x4016) { // Controller 1
        // TODO Controller 2
        data = Controller_cpu_read(&(nes->controller));
    }

    return data;
}

void BUS_cpu_write(NES_CORE *nes, uint16 addr, byte val) {
    if (Cartridge_cpu_write(&(nes->cart), addr, val)) {
    } else if (0x0000 <= addr && addr <= 0x1FFF) {
        nes->cpu_ram[addr & 0x07FF] = val;
    } else if (0x2000 <= addr && addr <= 0x3FFF) {
        PPU_cpu_write(&(nes->ppu), addr, val);
    } else if (0x4000 <= addr && addr <= 0x4013) {
        APU_cpu_write(&(nes->apu), addr, val);
    } else if (addr == 0x4014) { // DMA transfer
        nes->cpu.dma_page = val;
        nes->cpu.dma_addr = 0x00;
        nes->dma_active = true;
        nes->cpu.dma_dummy = true;
    } else if (addr == 0x4015) {
        APU_cpu_write(&(nes->apu), addr, val);
    } else if (addr == 0x4016) { // Controllers
        Controller_cpu_write(&(nes->controller), val);
    } else if (addr == 0x4017) {
        APU_cpu_write(&(nes->apu), addr, val);
    }
}
