//
// Created by valerino on 10/01/19.
//

#include "CMemory.h"
#include <stdio.h>
#include <stdlib.h>

int CMemory::readBytes(uint16_t address, uint8_t *b, uint16_t bufferSize, uint16_t readSize) {
    int res = 0;
    uint16_t s = readSize;
    if (readSize > bufferSize) {
        s = bufferSize;
        res = EOVERFLOW;
    }
    for (uint16_t i = 0; i < s; i++) {
        uint8_t c = readByte(address + i, &c);
        b[i] = c;
    }
    return res;
}

CMemory::CMemory() {
    _mem = (uint8_t *) calloc(1, MEMORY_SIZE);
}

CMemory::~CMemory() {
    if (_mem) {
        free (_mem);
    }
}

uint8_t CMemory::readByte(uint16_t address, uint8_t *b) {
    if (b) {
        *b = _mem[address];
        return 0;
    }
    return EINVAL;
}

uint16_t CMemory::readWord(uint16_t address, uint16_t *w) {
    uint16_t ww = (_mem[address + 1] << 8) | _mem[address];
    return ww;
}

int CMemory::writeByte(uint16_t address, uint8_t b) {
    _mem[address] = b;
    return 0;
}

int CMemory::writeWord(uint16_t address, uint16_t w) {
    _mem[address + 1] = (uint8_t)(w >> 8);
    _mem[address] = (uint8_t)(w & 0xff);
    return 0;
}

int CMemory::writeBytes(uint16_t address, uint8_t *b, uint16_t size) {
    for (uint16_t i = 0; i < size; i++) {
        writeByte(address + i, b[i]);
    }
    return 0;
}
