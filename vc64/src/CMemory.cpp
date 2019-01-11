//
// Created by valerino on 10/01/19.
//

#include "CMemory.h"
#include <stdio.h>
#include <stdlib.h>
#include <CBuffer.h>
#include <CLog.h>
#include <string.h>

int CMemory::readBytes(uint16_t address, uint8_t *b, uint16_t bufferSize, uint16_t readSize) {
    int res = 0;
    uint16_t s = readSize;
    if (readSize > bufferSize) {
        s = bufferSize;
        res = EOVERFLOW;
    }
    for (uint16_t i = 0; i < s; i++) {
        uint8_t c;
        readByte(address + i, &c);
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
    if (w) {
        uint16_t ww = (_mem[address + 1] << 8) | _mem[address];
        *w = ww;
        return 0;
    }
    return EINVAL;
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

int CMemory::loadBios() {
    char path[260];
    char bios[] = "./bios";

    // load kernal
    snprintf(path,sizeof(path),"%s/kernal.bin", bios);
    uint8_t* kernal;
    uint32_t kernalSize;
    int res = CBuffer::fromFile(path, &kernal, &kernalSize);
    if (res != 0) {
        CLog::error("%s: %s", strerror(res), path);
        return res;
    }
    CLog::print("Loaded kernal ROM: %s", path);

    // load charset
    uint8_t* charset;
    uint32_t charsetSize;
    snprintf(path,sizeof(path),"%s/charset.bin", bios);
    res = CBuffer::fromFile(path, &charset, &charsetSize);
    if (res != 0) {
        CLog::error("%s: %s", strerror(res), path);
        free(kernal);
        return res;
    }
    CLog::print("Loaded charset ROM: %s", path);

    // load basic
    uint8_t* basic;
    uint32_t basicSize;
    snprintf(path,sizeof(path),"%s/basic.bin", bios);
    res = CBuffer::fromFile(path, &basic, &basicSize);
    if (res != 0) {
        CLog::error("%s: %s", strerror(res), path);
        free(kernal);
        free(charset);
        return res;
    }
    CLog::print("Loaded basic ROM: %s", path);

    // copy to emulator memory
    writeBytes(MEMORY_BASIC_ADDRESS, basic, MEMORY_BASIC_SIZE);
    writeBytes(MEMORY_CHARSET_ADDRESS, charset, MEMORY_CHARSET_SIZE);
    writeBytes(MEMORY_KERNAL_ADDRESS, kernal, MEMORY_KERNAL_SIZE);

    // done
    free(kernal);
    free(charset);
    free(basic);
    return 0;
}

int CMemory::init() {
    memset(_mem,0,MEMORY_SIZE);

    // load bios
    return loadBios();
}
