//
// Created by valerino on 10/01/19.
//

#include "CMemory.h"
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <CBuffer.h>
#include <string.h>
#include "bitutils.h"

CMemory::CMemory() {
    // allocate main memory and the shadows
    _mem = (uint8_t *)calloc(1, MEMORY_SIZE);
    _kernalRom = (uint8_t *)calloc(1, MEMORY_KERNAL_SIZE);
    _basicRom = (uint8_t *)calloc(1, MEMORY_BASIC_SIZE);
    _charRom = (uint8_t *)calloc(1, MEMORY_CHARSET_SIZE);
}

CMemory::~CMemory(){SAFE_FREE(_mem) SAFE_FREE(_charRom) SAFE_FREE(_basicRom)
                        SAFE_FREE(_kernalRom)}

uint8_t CMemory::readByte(uint32_t address, uint8_t *b) {
    if (!b) {
        return EINVAL;
    }

    // check addresses
    if (address >= MEMORY_BASIC_ADDRESS &&
        address < MEMORY_BASIC_ADDRESS + MEMORY_BASIC_SIZE) {
        // accessing basic rom
        if (IS_BIT_SET(_mem[ZEROPAGE_REG_IO_PORT], 0)) {
            *b = _basicRom[address - MEMORY_BASIC_ADDRESS];
        } else {
            // basic rom is masked, returning ram
            *b = _mem[address];
        }
    } else if (address >= MEMORY_KERNAL_ADDRESS &&
               address < MEMORY_KERNAL_ADDRESS + MEMORY_KERNAL_SIZE) {
        // accessing kernal rom
        if (IS_BIT_SET(_mem[ZEROPAGE_REG_IO_PORT], 1)) {
            *b = _kernalRom[address - MEMORY_KERNAL_ADDRESS];
        } else {
            // kernal rom is masked, returning ram
            *b = _mem[address];
        }
    } else if (address >= MEMORY_CHARSET_ADDRESS &&
               address < MEMORY_CHARSET_ADDRESS + MEMORY_CHARSET_SIZE) {
        // accessing char rom
        if (IS_BIT_SET(_mem[ZEROPAGE_REG_IO_PORT], 2)) {
            // access the IO registers
            *b = _mem[address];
        } else {
            *b = _charRom[address - MEMORY_CHARSET_ADDRESS];
        }
    } else {
        // ram
        *b = _mem[address];
    }

    // TODO: handle other bankswitching types (see
    // https://www.c64-wiki.com/wiki/Bank_Switching)
    return 0;
}

int CMemory::writeByte(uint32_t address, uint8_t b) {
    // TODO: all writes to ram, is this right ?
    _mem[address] = b;
    return 0;
}

uint16_t CMemory::readWord(uint32_t address, uint16_t *w) {
    if (w) {
        uint8_t lo;
        readByte(address, &lo);
        uint8_t hi;
        readByte(address + 1, &hi);

        uint16_t ww = (hi << 8) | lo;
        *w = ww;
        return 0;
    }
    return EINVAL;
}

int CMemory::writeWord(uint32_t address, uint16_t w) {
    uint8_t lo = (w & 0xff);
    uint8_t hi = (uint8_t)(w >> 8);
    writeByte(address, lo);
    writeByte(address + 1, hi);
    return 0;
}

int CMemory::readBytes(uint32_t address, uint8_t *b, uint32_t bufferSize,
                       uint32_t readSize) {
    int res = 0;
    uint32_t s = readSize;

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

int CMemory::writeBytes(uint32_t address, uint8_t *b, uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        writeByte(address + i, b[i]);
    }
    return 0;
}

int CMemory::loadBios() {
    char path[260];
    char bios[] = "./bios";
    bool ok = false;
    int res = 0;
    do {
        // load kernal
        snprintf(path, sizeof(path), "%s/kernal.bin", bios);
        uint32_t kernalSize;
        res = CBuffer::fromFile(path, &_kernalRom, &kernalSize);
        if (res != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", strerror(res),
                         path);
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "loaded kernal ROM: %s, size=0x%0x", path, kernalSize);

        // load charset
        uint32_t charsetSize;
        snprintf(path, sizeof(path), "%s/charset.bin", bios);
        res = CBuffer::fromFile(path, &_charRom, &charsetSize);
        if (res != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", strerror(res),
                         path);
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "loaded charset ROM: %s, size=0x%0x", path, charsetSize);

        // load basic
        uint32_t basicSize;
        snprintf(path, sizeof(path), "%s/basic.bin", bios);
        res = CBuffer::fromFile(path, &_basicRom, &basicSize);
        if (res != 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s: %s", strerror(res),
                         path);
            break;
        }
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                    "loaded basic ROM: %s, size=0x%0x", path, basicSize);
        ok = true;
    } while (0);
    if (!ok) {
        SAFE_FREE(_charRom)
        SAFE_FREE(_kernalRom)
        SAFE_FREE(_basicRom)
        return res;
    }

    return 0;
}

int CMemory::init() {
    memset(_mem, 0, MEMORY_SIZE);

    // initialize zeropage data direction
    BIT_SET(_mem[ZEROPAGE_REG_DATA_DIRECTION], 0);
    BIT_SET(_mem[ZEROPAGE_REG_DATA_DIRECTION], 1);
    BIT_SET(_mem[ZEROPAGE_REG_DATA_DIRECTION], 2);

    // and io/port
    BIT_SET(_mem[ZEROPAGE_REG_IO_PORT], 0);
    BIT_SET(_mem[ZEROPAGE_REG_IO_PORT], 1);
    BIT_SET(_mem[ZEROPAGE_REG_IO_PORT], 2);

    // load bios
    return loadBios();
}

uint8_t *CMemory::ram() { return raw(); }

uint8_t *CMemory::raw(uint32_t *size) {
    if (size) {
        *size = MEMORY_SIZE;
    }
    return _mem;
}

uint8_t *CMemory::charset() { return _charRom; }

int CMemory::loadPrg(const char *path) {
    if (!path) {
        return EINVAL;
    }

    // load file
    uint8_t *buf;
    uint32_t size;
    int res = CBuffer::fromFile(path, &buf, &size);
    if (res != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "failed to load PRG: %s (%d)", path, res);
        return res;
    }

    // first 2 bytes is the load address
    uint8_t *p = (uint8_t *)buf;
    uint16_t address = *((uint16_t *)p);
    size -= sizeof(uint16_t);
    p += sizeof(uint16_t);
    uint16_t end = address + size;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION,
                "loading %s in RAM, start=$%08x, end=$%08x, size=%d", path,
                address, end, size);

    // copy in ram
    res = writeBytes(address, p, size);
    free(buf);
    if (res != 0) {
        return res;
    }

    if (address == BASIC_PRG_START_ADDRESS) {
        // set basic informations in the zeropage
        uint16_t end = address + size;
        writeWord(ZEROPAGE_BASIC_PROGRAM_START, address);
        writeWord(ZEROPAGE_BASIC_VARTAB, end);
        writeWord(ZEROPAGE_BASIC_ARYTAB, end);
        writeWord(ZEROPAGE_BASIC_STREND, end);
    }
    return res;
}
