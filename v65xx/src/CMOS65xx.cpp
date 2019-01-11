//
// Created by valerino on 06/01/19.
//

#include <CMOS65xx.h>

// flags for the P register
#define P_FLAG_CARRY        0x01
#define P_FLAG_ZERO         0x02
#define P_FLAG_IRQ_DISABLE  0x04
#define P_FLAG_DECIMAL_MODE 0x08
#define P_FLAG_BRK_COMMAND  0x10
#define P_FLAG_UNUSED       0x20
#define P_FLAG_OVERFLOW     0x40
#define P_FLAG_NEGATIVE     0x80

// vectors
#define VECTOR_RESET 0xfffc
#define VECTOR_IRQ 0xfffe

void CMOS65xx::irq() {

}

void CMOS65xx::nmi() {

}

void CMOS65xx::run(int cycles) {

}

int CMOS65xx::reset() {
    int res = _memory->init();
    if (res != 0) {
        // initialization error
        return res;
    }

    // set registers to initial state
    _regA = 0;
    _regX = 0;
    _regY = 0;
    _regS = 0xfd;
    _regP |= P_FLAG_IRQ_DISABLE;
    _currentCycles = 8;
    _memory->readWord(VECTOR_RESET,&_regPC);
    return 0;
}

CMOS65xx::CMOS65xx(IMemory *mem) {
    _memory = mem;
}
