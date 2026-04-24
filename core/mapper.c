#include <stdio.h>
#include <stdlib.h>
#include "mapper.h"

Mapper *Mapper_create(uint8 id, uint8 n_prg_banks, uint8 n_chr_banks, Mirroring_Mode mirroring_mode) {
    Mapper *self = (Mapper *)calloc(1, sizeof(Mapper));
    if (self == NULL) {
        printf("Malloc failed to allocate Mapper!\n");
        exit(1);
        return NULL;
    }
    self->id = id;
    self->n_prg_banks = n_prg_banks;
    self->n_chr_banks = n_chr_banks;
    self->mirroring_mode = mirroring_mode;

    switch (id) {
        case 0:
            self->cpuMapRead = &mapper000_cpuMapRead;
            self->cpuMapWrite = &mapper000_cpuMapWrite;
            self->ppuMapRead = &mapper000_ppuMapRead;
            self->ppuMapWrite = &mapper000_ppuMapWrite;
            self->reset = &mapper000_reset;
            self->scanline = &mapper000_scanline;
            self->state = NULL;
            break;
        case 1:
            self->cpuMapRead = &mapper001_cpuMapRead;
            self->cpuMapWrite = &mapper001_cpuMapWrite;
            self->ppuMapRead = &mapper001_ppuMapRead;
            self->ppuMapWrite = &mapper001_ppuMapWrite;
            self->reset = &mapper001_reset;
            self->scanline = &mapper001_scanline;
            self->state = calloc(1, sizeof(Mapper001_State));

            self->reset(self);
            break;
        case 2:
            self->cpuMapRead = &mapper002_cpuMapRead;
            self->cpuMapWrite = &mapper002_cpuMapWrite;
            self->ppuMapRead = &mapper002_ppuMapRead;
            self->ppuMapWrite = &mapper002_ppuMapWrite;
            self->reset = &mapper002_reset;
            self->scanline = &mapper002_scanline;
            self->state = calloc(1, sizeof(Mapper002_State));

            self->reset(self);
            break;
        case 4:
            self->cpuMapRead = &mapper004_cpuMapRead;
            self->cpuMapWrite = &mapper004_cpuMapWrite;
            self->ppuMapRead = &mapper004_ppuMapRead;
            self->ppuMapWrite = &mapper004_ppuMapWrite;
            self->reset = &mapper004_reset;
            self->scanline = &mapper004_scanline;
            self->state = calloc(1, sizeof(Mapper004_State));

            self->reset(self);
            break;
    }

    return self;
}

void Mapper_destroy(Mapper *self) {
    if (self->state != NULL) free(self->state);
    free(self);
}
