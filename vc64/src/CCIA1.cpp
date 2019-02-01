//
// Created by valerino on 19/01/19.
//

#include "CCIA1.h"

CCIA1::CCIA1(CMOS65xx* cpu) : CCIA(cpu) {

}

CCIA1::~CCIA1() {

}

int CCIA1::run(int cycleCount) {
    // TODO: properly implement timer, for now we trigger an irq at every screen refresh
    if (cycleCount <= 0) {
        // trigger an interrupt
        _cpu->irq();
    }
    return 0;
}

void CCIA1::read(uint16_t address, uint8_t *bt) {

    // default
    _cpu->memory()->readByte(address, bt);
}

void CCIA1::write(uint16_t address, uint8_t bt) {

    // default
    _cpu->memory()->writeByte(address, bt);
}
