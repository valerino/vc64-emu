//
// Created by valerino on 19/01/19.
//

#include "CCIA2.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <bitutils.h>
#include <CLog.h>
#include "CMemory.h"

#ifndef NDEBUG
// debug-only flag
//#define DEBUG_CIA2
#endif

CCIA2::CCIA2(CMOS65xx* cpu) {
    _cpu = cpu;
}

CCIA2::~CCIA2() {

}

int CCIA2::run(int cycleCount) {
    return 0;
}

int CCIA2::getVicBank(uint16_t* address) {
    int bank = 0;
    uint8_t dataReg;
    _cpu->memory()->readByte(CIA2_REG_DATA, &dataReg);
    if (IS_BIT_SET(dataReg, 1) && IS_BIT_SET(dataReg, 0)) {
        // xxxxxx10
        bank = 0;
        *address = 0;
    }
    else if (IS_BIT_SET(dataReg, 1) && !(IS_BIT_SET(dataReg, 0))) {
        // xxxxxx11
        bank = 1;
        *address = 0x4000;
    }
    else if (!(IS_BIT_SET(dataReg, 1)) && IS_BIT_SET(dataReg, 0)) {
        // xxxxxx01
        bank = 2;
        *address = 0x8000;
    }
    else if (!(IS_BIT_SET(dataReg, 1)) && !(IS_BIT_SET(dataReg, 0))) {
        // xxxxxx00
        bank = 3;
        *address = 0xc000;
    }

#ifdef DEBUG_CIA2
    CLog::printRaw("\tVIC-II bank %d selected!\n", bank);
#endif
    return bank;
}

void CCIA2::read(uint16_t address, uint8_t *bt) {
    _cpu->memory()->readByte(address,bt);
}

void CCIA2::write(uint16_t address, uint8_t bt) {
    if (address == CIA2_REG_DATA) {
        uint16_t base;
        int bank = getVicBank(&base);
        CMemory* ram = (CMemory*)_cpu->memory();
        if (bank == 0) {
            // shadow character rom at $1000
            memcpy(ram->raw() + 0x1000, ram->charset(), MEMORY_CHARSET_SIZE);
#ifdef DEBUG_CIA2
            CLog::printRaw("\tmirroring charset ROM in RAM at $1000\n");
#endif
        }
        else if (bank == 1) {
            // shadow character rom at $9000
            memcpy(ram->raw() + 0x9000, ram->charset(), MEMORY_CHARSET_SIZE);
#ifdef DEBUG_CIA2
            CLog::printRaw("\tmirroring charset ROM in RAM at $9000\n");
#endif
        }
    }

    // write
    _cpu->memory()->writeByte(address,bt);
}
