//
// Created by valerino on 19/01/19.
//

#include "CCIA2.h"
#include <SDL.h>
#include "bitutils.h"

CCIA2::CCIA2(CMOS65xx *cpu)
    : CCIABase::CCIABase(cpu, CIA2_REGISTERS_START, CIA_TRIGGERS_NMI) {}

int CCIA2::getVicBank(uint16_t *address) {
    // read PRA to get vic base
    int bank = 0;
    uint8_t dataReg;
    _cpu->memory()->readByte(0xdd00, &dataReg);
    if (!IS_BIT_SET(dataReg, 0) && !IS_BIT_SET(dataReg, 1)) {
        // xxxxxx00
        bank = 3;
        *address = 0xc000;
    } else if (!IS_BIT_SET(dataReg, 0) && IS_BIT_SET(dataReg, 1)) {
        // xxxxxx01
        bank = 2;
        *address = 0x8000;
    } else if (IS_BIT_SET(dataReg, 0) && !IS_BIT_SET(dataReg, 1)) {
        // xxxxxx10
        bank = 1;
        *address = 0x4000;
    } else if (IS_BIT_SET(dataReg, 0) && IS_BIT_SET(dataReg, 1)) {
        // xxxxxx11
        bank = 0;
        *address = 0x0;
    }

#ifdef DEBUG_CIA2
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, "VIC-II bank %d selected!\n",
                 bank);
#endif
    return bank;
}

void CCIA2::read(uint16_t address, uint8_t *bt) {
    switch (address) {
    default:
        // default processing with the base class
        CCIABase::read(address, bt);
    }
}

void CCIA2::write(uint16_t address, uint8_t bt) {
    switch (address) {

    default:
        // default processing with the base class
        CCIABase::write(address, bt);
    }
}
