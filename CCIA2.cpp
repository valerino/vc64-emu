//
// Created by valerino on 19/01/19.
//

#include "CCIA2.h"
#include <SDL.h>
#include "bitutils.h"

CCIA2::CCIA2(CMOS65xx *cpu, CPLA *pla)
    : CCIABase::CCIABase(cpu, pla, CIA2_REGISTERS_START, CIA_TRIGGERS_NMI) {}

void CCIA2::setVicBank(uint8_t pra) {
    /*
        Bit 0..1: Select the position of the VIC-memory
        %00, 0: Bank 3: $C000-$FFFF, 49152-65535
        %01, 1: Bank 2: $8000-$BFFF, 32768-49151
        %10, 2: Bank 1: $4000-$7FFF, 16384-32767
        %11, 3: Bank 0: $0000-$3FFF, 0-16383 (standard)
        Bit 2: RS-232: TXD Output, userport: Data PA 2 (pin M)
        Bit 3..5: serial bus Output (0=High/Inactive, 1=Low/Active)
        Bit 3: ATN OUT
        Bit 4: CLOCK OUT
        Bit 5: DATA OUT
        Bit 6..7: serial bus Input (0=Low/Active, 1=High/Inactive)
        Bit 6: CLOCK IN
        Bit 7: DATA IN
    */

    if (!IS_BIT_SET(pra, 0) && !IS_BIT_SET(pra, 1)) {
        // xxxxxx00
        _vicBank = 3;
        _vicMemory = 0xc000;
    } else if (!IS_BIT_SET(pra, 0) && IS_BIT_SET(pra, 1)) {
        // xxxxxx10
        _vicBank = 2;
        _vicMemory = 0x4000;
    } else if (IS_BIT_SET(pra, 0) && !IS_BIT_SET(pra, 1)) {
        // xxxxxx01
        _vicBank = 1;
        _vicMemory = 0x8000;
    } else if (IS_BIT_SET(pra, 0) && IS_BIT_SET(pra, 1)) {
        // xxxxxx11
        _vicBank = 0;
        _vicMemory = 0;
    }
    SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION,
                 "VIC-II bank %d selected, VIC memory address=$%x!\n", _vicBank,
                 _vicMemory);
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
    case 0xdd00:
        // PRA
        // set the vic bank and address
        // @todo handle other bits
        setVicBank(bt);
        CCIABase::write(address, bt);
        break;

    default:
        // default processing with the base class
        CCIABase::write(address, bt);
        break;
    }
}
