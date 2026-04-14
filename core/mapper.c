#include <stdio.h>
#include <stdlib.h>
#include "mapper.h"

Mapper *Mapper_create(uint8 id, uint8 n_prg_banks, uint8 n_chr_banks, Mirroring_Mode mirroring_mode) {
    Mapper *self = (Mapper *)calloc(1, sizeof(Mapper));
    if (self == NULL) {
        printf("CRITICAL: Malloc failed to allocate Mapper!\n");
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
            self->state = NULL;
            break;
        case 1:
            self->cpuMapRead = &mapper001_cpuMapRead;
            self->cpuMapWrite = &mapper001_cpuMapWrite;
            self->ppuMapRead = &mapper001_ppuMapRead;
            self->ppuMapWrite = &mapper001_ppuMapWrite;
            self->reset = &mapper001_reset;
            self->state = calloc(1, sizeof(Mapper001_State));

            self->reset(self);
            break;
        case 2:
            break;
    }

    return self;
}
